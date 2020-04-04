#pragma once
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "data.h"

/* attribute definition */
struct Attr
{
    int num_cus;
    double dist;
    int s; // start point
    int e; // end point
    /* time window related */
    double T_D; // minimum duration time
    double T_E; // arriving at [T_E,T_L] could achive minimum duration time
    double T_L;
    /* capacity related*/
    double C_E; // initial capacity demand
    double C_H; // the highest capacity demand
    double C_L; // the final capacity demand
};

void attr_for_one_node(Data &data, int node, Attr &a);
Attr attr_for_one_node(Data &data, int node);

// connect tmp_a and tmp_b, return value
Attr connect(const Attr &tmp_a, const Attr &tmp_b, double dist_ij, double t_ij);

// connect tmp_a and tmp_b, write to merged_attr
void connect(const Attr &tmp_a, const Attr &tmp_b, Attr &merged_attr, double dist_ij, double t_ij);
void connect(Attr &merged_attr, const Attr &tmp_b, double dist_ij, double t_ij);

/* route class definition */
class Route
{
public:
    std::vector<int> node_list;
    double dep_time, ret_time, transcost;
    Attr attr[MAX_NODE_IN_ROUTE][MAX_NODE_IN_ROUTE];
    Attr self;

    Route(Data &data)
    {
        this->node_list.reserve(MAX_NODE_IN_ROUTE);
        this->node_list.push_back(data.DC);
        this->node_list.push_back(data.DC);
        this->update(data);
    }

    // attribute
    Attr &get_attr(int i, int j)
    {
        return this->attr[i][j];
    }

    void cal_attr(Data &data) //calculate complete attr matrix O(n^2)
    {
        int end_index = int(node_list.size()) - 1;
        // 1. attribute for each node
        for (int i = 0; i <= end_index; i++)
        {
            attr_for_one_node(data, node_list[i], this->attr[i][i]);
        }
        // 2. attribute for sub sequence with start point i, and end point j
        for (int i = 0; i < end_index; i++)
        {
            for (int j = i + 1; j <= end_index; j++)
            {
                connect(attr[i][j-1], attr[j][j], attr[i][j],\
                        data.dist[node_list[j-1]][node_list[j]],\
                        data.time[node_list[j-1]][node_list[j]]);
            }
        }
        this->self = this->attr[0][end_index];
        // 3. attribute for inverse sub sequence with length 2, need to check TW constraint
        for (int i = end_index - 1; i > 0; i--)
        {
            bool feasible = true;
            for (int j = i - 1; j > 0 && (i - j + 1) <= 2; j--)
            {
                if (!feasible)
                {
                    attr[i][j].num_cus = INFEASIBLE;
                    continue;
                }
                // check TW constraint
                if (attr[i][j+1].T_E + attr[i][j+1].T_D + data.time[node_list[j+1]][node_list[j]] - attr[j][j].T_L > 0)
                {
                    attr[i][j].num_cus = INFEASIBLE;
                    feasible = false;
                }
                else
                {
                    connect(attr[i][j+1], attr[j][j], attr[i][j],\
                            data.dist[node_list[j+1]][node_list[j]],
                            data.time[node_list[j+1]][node_list[j]]);
                }
            }
        }
    }

    // method used when applying move operator
    void update(Data &data)
    {
        // update this route, including attribute (if necessary), dep_time and ret_time
        this->cal_attr(data);
        // set dep_time and ret_time
        this->dep_time = this->self.T_E;
        this->ret_time = this->dep_time + this->self.T_D;
    }

    void set_node_list(const std::vector<int> &nl) 
    { 
        this->node_list = nl; 
    }

    // cost
    double cal_cost(Data &data)
    {
        this->transcost = this->self.dist * data.vehicle.unit_cost;
        double dispatchcost = 0.0;
        if (!this->isempty())
            dispatchcost = data.vehicle.d_cost;
        return this->transcost + dispatchcost;
    }

    // others
    bool isempty()
    {
        return this->self.num_cus == 0;
    }
};

/* solution class definition*/
class Solution
{
private:
    std::vector<Route> route_list;

public:
    double cost = 0.0;

    Solution(){}

    Solution(Data &data)
    { 
        this->route_list.reserve(data.vehicle.max_num);
    }

    void reserve(Data &data)
    {
        this->route_list.reserve(data.vehicle.max_num);
    }

    void append(Route &r)
    {
        this->route_list.push_back(r);
    }

    void del(int index)
    {
        this->route_list.erase(this->route_list.begin() + index);
    }

    Route &get(int index)
    {
        return this->route_list[index];
    }
    Route &get_tail()
    { 
        return *(route_list.end() - 1);
    }

    int len()
    {
        return int(this->route_list.size());
    }

    void update(Data &data)
    {
        // update all routes, and delete empty routes
        int len = this->len();
        for (int index = 0; index < len;)
        {
            Route &route = this->get(index);
            route.update(data);
            if (route.isempty())
            {
                // delete empty route
                this->del(index);
                len--;
            }
            else
                index++;
        }
    }

    void local_update(std::vector<int> &route_indice, Data &data)
    {
        // delete empty routes in route_indice, and maintain route_list
        // note route_indice would be altered to record all
        // changed routes

        int len = this->len();
        int empty_id = -1; // -1 means no empty route exists
        bool last_id_in = false; // true means last route changed
        for (int &item : route_indice)
        {
            // there are at most 2 route index in the array
            // and at most 1 route is emppty
            if (this->get(item).isempty())
                empty_id = item;
            if (item == len - 1)
                last_id_in = true;
        }
        if (empty_id != -1)
        {
            if (empty_id == len - 1)
            {
                this->route_list.pop_back();
            }
            else
            {
                this->route_list[empty_id] = *(this->route_list.end() - 1);
                this->route_list.pop_back();
                if (!last_id_in)
                    route_indice.push_back(len - 1);
            }
        }
    }

    void clear(Data &data)
    {
        // clear route_list
        this->route_list.clear();
        this->route_list.reserve(data.vehicle.max_num);
    }

    double cal_cost(Data &data)
    {
        this->cost = 0.0;
        for (auto route = this->route_list.begin(); route < this->route_list.end(); route++)
        {
            this->cost += route->cal_cost(data);
        }
        return this->cost;
    }

    std::string build_output_str()
    {
        std::string output_s = "Details of the solution:\n";
        int len = this->len();
        for (int i = 0; i < len; i++)
        {
            std::vector<int> &nl = this->route_list[i].node_list;
            output_s += "route " + std::to_string(i) +
                        ", node_num " + std::to_string(nl.size()) +
                        ", cost " + std::to_string(this->route_list[i].transcost) +
                        ", nodes:";
            for (int node : nl)
            {
                output_s += ' ' + std::to_string(node);
            }
            output_s += '\n';
        }
        output_s += "vehicle (route) number: " + std::to_string(len) + '\n';
        output_s += "Total cost: " + std::to_string(this->cost) + '\n';
        return output_s;
    }

    void output(Data &data)
    {
        std::string output_s = this->build_output_str();
        if (!data.if_output) std::cout << output_s;
        else
        {
            std::ofstream out(data.output.c_str());
            out << output_s;
        }
    }
};
