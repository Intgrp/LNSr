#include "operator.h"
// for preventing memory allocation
Move tmp_move;
// map from string to opt;
std::map<std::string, std::function<void(Solution &, Data &, Move &)>>
small_opt_map = {
    {"2opt", two_opt},
    {"2opt*", two_opt_star},
    {"oropt", or_opt},
    {"2exchange", two_exchange}};
std::map<std::string, std::function<void(Solution &, Data &)>>
destroy_opt_map = {{"random_removal", random_removal},
                   {"related_removal", related_removal}};
std::map<std::string, std::function<void(Solution &, Data &)>>
repair_opt_map = {{"regret_insertion", regret_insertion},
                  {"greedy_insertion", greedy_insertion}};

void maintain_unrouted(int i, int node, int &index, std::vector<std::tuple<int, int>> &unrouted, double &unrouted_d, double &unrouted_p, Data &data)
{
    unrouted[i] = unrouted[index-1];
    index--;
    unrouted_d -= data.node[node].delivery;
    unrouted_p -= data.node[node].pickup;
}

double cal_tc(Route &r, double unrouted_d, double unrouted_p, Data &data)
{
    std::vector<int> &nl = r.node_list;
    int len = int(nl.size());
    double capacity = data.vehicle.capacity;

    // RDT and RPT for route
    double rdt = 0.0;
    double rpt = 0.0;
    double route_d = 0.0;
    double route_p = 0.0;
    double rd[MAX_NODE_IN_ROUTE];
    double rp[MAX_NODE_IN_ROUTE];
    double load[MAX_NODE_IN_ROUTE];
    double cd[MAX_NODE_IN_ROUTE];
    double cp[MAX_NODE_IN_ROUTE];

    for (int i = 1; i < len; i++)
    {
        route_d += data.node[nl[i]].delivery;
        route_p += data.node[nl[i]].pickup;
    }

    load[0] = route_d;
    cd[0] = 0.0;
    cp[len - 1] = 0.0;
    for (int i = 1; i < len - 1; i++)
    {
        load[i] = load[i - 1] - data.node[nl[i]].delivery + data.node[nl[i]].pickup;
        cd[i] = cd[i - 1] + data.dist[nl[i - 1]][nl[i]];
        cp[len - 1 - i] = cp[len - i] + data.dist[nl[len - i]][nl[len - 1 - i]];
    }

    rd[0] = capacity - route_d;
    rp[len - 2] = capacity - route_p;

    for (int i = 1; i < len - 1; i++)
    {
        rd[i] = std::min(rd[i - 1], capacity - load[i]);
        rp[len - 2 - i] = std::min(rp[len - 1 - i], capacity - load[len - 2 - i]);
    }

    double rdt_u = 0.0;
    double rdt_d = 0.0;
    double rpt_u = 0.0;
    double rpt_d = 0.0;
    for (int i = 0; i < len - 1; i++)
    {
        rdt_u += rd[i] * cd[i + 1];
        rdt_d += cd[i + 1];
        rpt_u += rp[i] * cp[i];
        rpt_d += cp[i];
    }
    rdt = rdt_u / rdt_d;
    rpt = rpt_u / rpt_d;

    // TC
    double tc = (unrouted_d / data.all_delivery) * (1 - rdt / capacity) + (unrouted_p / data.all_pickup) * (1 - rpt / capacity);
    return tc;
}

double criterion(Route &r, Data &data, int node, int pos, double unrouted_d, double unrouted_p)
{
    std::vector<int> &nl = r.node_list;
    double capacity = data.vehicle.capacity;
    // TD
    int pre = nl[pos-1];
    int suc = nl[pos];
    double td = data.dist[pre][node] + data.dist[node][suc] - data.dist[pre][suc];
    if (data.n_insert == TD) return td;

    // TC
    Route tmp_r(data);
    tmp_r = r;
    tmp_r.node_list.insert(tmp_r.node_list.begin() + pos, node);
    double tc = cal_tc(tmp_r, unrouted_d, unrouted_p, data);

    // RS
    double rs = data.dist[data.DC][node] + data.dist[node][data.DC];

    // RCRS
    double rcrs = td + std::get<0>(data.lambda_gamma) * tc * (2 * data.max_dist - data.min_dist) -
                  std::get<1>(data.lambda_gamma) * rs;

    return rcrs;
}

bool cal_score(bool feasible_pos[][MAX_NODE_IN_ROUTE], std::vector<std::tuple<int, int>> &unrouted, double *score, int index, Route &r, double unrouted_d, double unrouted_p, Data &data)
{
    /* calculate insertion criterion for each node in unrouted, return
    false if no feasible insertion exists */
    if (index == 0) return false;
    int r_len = int(r.node_list.size());
    // filter all infeasible positions
    int count = 0;
    for (int i = 0; i < index; i++)
    {
        int node = std::get<0>(unrouted[i]);
        for (int pos = 1; pos < r_len; pos++)
        {
            Route tmp_r(data);
            tmp_r = r;
            tmp_r.node_list.insert(tmp_r.node_list.begin() + pos, node);
            bool flag = false;
            double cost = -1.0;
            chk_route_O_n(tmp_r, data, flag, cost);
            if (!flag) feasible_pos[i][pos] = false;
            else 
            {
                feasible_pos[i][pos] = true;
                count++;
            }
        }
    }
    if (count == 0) return false;

    // insertion criterion
    for (int i = 0; i < index; i++)
    {
        int node = std::get<0>(unrouted[i]);
        double best_score = double(INFINITY);
        int best_pos = -1;
        for (int pos = 1; pos < r_len; pos++)
        {
            if (!feasible_pos[i][pos]) continue;
            double utility = criterion(r, data, node, pos, unrouted_d,\
                                       unrouted_p);
            if (utility - best_score < -PRECISION)
            {
                best_score = utility;
                best_pos = pos;
            }
        }
        std::get<1>(unrouted[i]) = best_pos;
        score[i] = best_score;
    }
    return true;
}

void find_unrouted(Solution &s, std::vector<int> &record)
{
    int len = s.len();
    for (int i = 0; i < len; i++)
    {
        Route &r = s.get(i);
        for (auto node : r.node_list)
        {
            record[node] = 1;
        }
    }
}

void greedy_insertion(Solution &s, Data &data)
{
    regret_insertion(s, data);
}

void regret_insertion(Solution &s, Data &data)
{
    int num_cus = data.customer_num;
    // find all unrouted nodes
    std::vector<int> record(num_cus + 1, 0);
    find_unrouted(s, record);
    // <node, route, pos, best_incur_cost, second_incur_cost, inserted_or_not>
    // route = -1: means a new route
    // route = -2: means not check yet
    std::vector<std::tuple<int,int,int,double,double,bool>> unrouted;
    unrouted.reserve(num_cus);
    for (int i = 0; i < num_cus + 1; i++)
    {
        if (i == data.DC) continue;
        if (record[i] == 0) unrouted.push_back(std::make_tuple(i, -2, 0, double(INFINITY), double(INFINITY), false));
    }
    int len = int(unrouted.size());
    while (len > 0)
    {
        // find the one with the max regret value
        double max_regret = -double(INFINITY);
        int best_index = -1;
        for (int i = 0; i < int(unrouted.size()); i++) 
        {
            auto &n_t = unrouted[i];
            if(std::get<5>(n_t)) continue;
            if (std::get<1>(n_t) == -2)
            {
                // first time in this loop
                double best_incur_cost = double(INFINITY);
                int best_route = -2;
                int best_pos = -1;
                double second_incur_cost = double(INFINITY);
                // build a new route with node {DC, node, DC}
                Route tmp_r(data);
                tmp_r.node_list.insert(tmp_r.node_list.begin() + 1, std::get<0>(n_t));
                bool flag = false;
                double cost = -1.0;
                chk_route_O_n(tmp_r, data, flag, cost);
                if (!flag)
                {
                    printf("Error: Detect not feasible 1-customer route: ");
                    for (auto &node : tmp_r.node_list)
                    {
                        std::cout << node;
                    }
                    exit(-1);
                }
                best_incur_cost = cost;
                best_route = -1;
                best_pos = 1;
                // try to insert into s
                for (int r_index = 0; r_index < s.len(); r_index++)
                {
                    Route &r = s.get(r_index);
                    double ori_cost = r.cal_cost(data);
                    for (int pos = 1; pos < int(r.node_list.size()); pos++)
                    {
                        Route tmp_r(data);
                        tmp_r = r;
                        tmp_r.node_list.insert(tmp_r.node_list.begin() + pos, std::get<0>(n_t));
                        bool flag = false;
                        double cost = -1.0;
                        chk_route_O_n(tmp_r, data, flag, cost);
                        if (flag)
                        {
                            double incur_cost = cost - ori_cost;
                            if (incur_cost - best_incur_cost < -PRECISION)
                            {
                                second_incur_cost = best_incur_cost;
                                best_incur_cost = incur_cost;
                                best_route = r_index;
                                best_pos = pos;
                            }
                            else if (incur_cost - second_incur_cost < -PRECISION)
                            {
                                second_incur_cost = incur_cost;
                            }
                        }
                    }
                }
                std::get<1>(n_t) = best_route;
                std::get<2>(n_t) = best_pos;
                std::get<3>(n_t) = best_incur_cost;
                std::get<4>(n_t) = second_incur_cost;
            }
            // second_incur_cost = INFINITY, no feasible insertion into s
            // except building a new route. In this case set regret = 0, giving
            // it the minimum priority for insertion
            double regret = std::get<4>(n_t) - std::get<3>(n_t);
            if (std::get<4>(n_t) == double(INFINITY)) regret = 0.0;
            if (regret - max_regret > PRECISION)
            {
                max_regret = regret;
                best_index = i;
            }
        }
        if (max_regret == -double(INFINITY))
        {
            std::cout << "Eroor: Max regret -INFINITY, this should not happen\n";
            exit(-1);
        }
        // all nodes have one feasible insertion: building new route
        // find the one with minimum best_incur_cost
        if (max_regret == 0.0)
        {
            max_regret = double(INFINITY);
            for (int i = 0; i < int(unrouted.size()); i++)
            {
                auto &n_t = unrouted[i];
                if (std::get<5>(n_t)) continue;
                if (std::get<3>(n_t) - max_regret < -PRECISION)
                {
                    max_regret = std::get<3>(n_t);
                    best_index = i;
                }
            }
        }
        // insert
        if (std::get<1>(unrouted[best_index]) == -1)
        {
            // build a new route
            Route r(data);
            r.node_list.insert(r.node_list.begin() + 1, std::get<0>(unrouted[best_index]));
            r.update(data);
            s.append(r);
        }
        else
        {
            Route &r = s.get(std::get<1>(unrouted[best_index]));
            r.node_list.insert(r.node_list.begin() + std::get<2>(unrouted[best_index]), std::get<0>(unrouted[best_index]));
            r.update(data);
        }
        // flag inserted node in unrouted
        std::get<5>(unrouted[best_index]) = true;
        len--;

        // update regret score in unrouted
        // find the changed route
        int r_index = std::get<1>(unrouted[best_index]);
        if (r_index == -1) r_index = s.len() - 1;
        Route &r = s.get(r_index);
        double ori_cost = r.cal_cost(data);

        for (int i = 0; i < int(unrouted.size()); i++)
        {
            auto &n_t = unrouted[i];
            if(std::get<5>(n_t)) continue;
            for (int pos = 1; pos < int(r.node_list.size()); pos++)
            {
                Route tmp_r(data);
                tmp_r = r;
                tmp_r.node_list.insert(tmp_r.node_list.begin() + pos, std::get<0>(n_t));
                bool flag = false;
                double cost = -1.0;
                chk_route_O_n(tmp_r, data, flag, cost);
                if (flag)
                {
                    double incur_cost = cost - ori_cost;
                    if (incur_cost - std::get<3>(n_t) < -PRECISION)
                    {
                        std::get<4>(n_t) = std::get<3>(n_t);
                        std::get<3>(n_t) = incur_cost;
                        std::get<1>(n_t) = r_index;
                        std::get<2>(n_t) = pos;
                    }
                    else if (incur_cost - std::get<4>(n_t) < -PRECISION)
                    {
                        std::get<4>(n_t) = incur_cost;
                    }
                }
            }
        }
    }
    s.cal_cost(data);
}

void new_route_insertion(Solution &s, Data &data, int initial_node)
{
    double score[MAX_POINT];
    int score_argrank[MAX_POINT];
    int ties[MAX_POINT];
    bool feasible_pos[MAX_POINT][MAX_NODE_IN_ROUTE];
    double unrouted_d = data.all_delivery;
    double unrouted_p = data.all_pickup;

    int num_cus = data.customer_num;
    // find all unrouted customers
    // index points to the last position
    std::vector<int> record(num_cus + 1, 0);
    find_unrouted(s, record);

    // first dim: node, second dim: position
    std::vector<std::tuple<int, int>> unrouted(data.customer_num);
    int index = 0;
    for (int i = 0; i < num_cus + 1; i++)
    {
        if (i != data.DC && record[i] == 0)
        {
            std::get<0>(unrouted[index]) = i;
            index++;
        }
        else if (i != data.DC && record[i] == 1)
        {
            unrouted_d -= data.node[i].delivery;
            unrouted_p -= data.node[i].pickup;
        }
    }

    while (index > 0)
    {
        Route r(data);
        int selected = -1;
        int first_node = -1;
        if (index == data.customer_num) // the first time into this loop
        {
            first_node = initial_node;
            for (int i = 0; i < index; i++)
            {
                if (std::get<0>(unrouted[i]) == first_node)
                {
                    selected = i;
                    break;
                }
            }
        }
        else
        {
            selected = randint(0, index-1, data.rng);
            first_node = std::get<0>(unrouted[selected]);
        }
        maintain_unrouted(selected, first_node, index, unrouted, unrouted_d, unrouted_p, data);
        r.node_list.insert(r.node_list.begin() + 1, first_node);

        while (cal_score(feasible_pos, unrouted, score, index, r,
                         unrouted_d, unrouted_p, data))
        {
            argsort(score, score_argrank, index);
            double best_score = score[score_argrank[0]];
            ties[0] = score_argrank[0];
            int selected;
            int i = 1;
            for (; i < index; i++)
            {
                if (abs(best_score - score[score_argrank[i]]) < -PRECISION)
                    ties[i] = score_argrank[i];
                else
                    break;
            }
            if (i > 1) selected = ties[randint(0, i - 1, data.rng)];
            else selected = ties[0];
            // insert
            int node = std::get<0>(unrouted[selected]);
            int pos = std::get<1>(unrouted[selected]);

            r.node_list.insert(r.node_list.begin() + pos, node);
            maintain_unrouted(selected, node, index, unrouted, unrouted_d, unrouted_p, data);
        }
        r.update(data);
        s.append(r);
    }
    s.cal_cost(data);
}

void new_route_insertion(Solution &s, Data &data)
{
    int num_cus = data.customer_num;
    // find all unrouted customers
    // index points to the last position
    std::vector<int> record(num_cus + 1, 0);
    find_unrouted(s, record);

    // first dim: node, second dim: position
    std::vector<std::tuple<int, int>> unrouted(data.customer_num);
    int index = 0;
    for (int i = 0; i <= num_cus; i++)
    {
        if (i != data.DC && record[i] == 0)
        {
            std::get<0>(unrouted[index]) = i;
            index++;
        }
    }
    if (index == 0) return;
    if (data.ksize == 1)
    {
        int selected = randint(0, index - 1, data.rng);
        int node = std::get<0>(unrouted[selected]);
        new_route_insertion(s, data, node);
    }
    else
    {
        double best_cost = double(INFINITY);
        Solution best_s(data);
        for (int i = 0; i < std::min(data.ksize, index); i++)
        {
            int selected = randint(0, index-1-i, data.rng);
            int node = std::get<0>(unrouted[selected]);
            Solution tmp_s(data);
            tmp_s = s;
            new_route_insertion(tmp_s, data, node);
            if (tmp_s.cost - best_cost < -PRECISION)
            {
                best_s = tmp_s;
                best_cost = tmp_s.cost;
            }
            unrouted[selected] = unrouted[index-1-i];
            printf("Solution %d, Cost %.4f\n", i, tmp_s.cost);
        }
        s = best_s;
    }
}

void two_opt(Solution &s, Data &data, Move &m)
{
    // inverse a 2-sequence in a route
    m.delta_cost = double(INFINITY);
    int s_len = int(s.len());
    for (int r_index = 0; r_index < s_len; r_index++)
    {
        Route &r = s.get(r_index);
        auto &n_l = r.node_list;
        int len = int(n_l.size());
        if (len < 4) continue;
        for (int start = 1; start <= len - 3; start++)
        {
            if (data.pruning &&
                (!data.pm[n_l[start-1]][n_l[start+1]] ||
                 !data.pm[n_l[start+1]][n_l[start]] ||
                 !data.pm[n_l[start]][n_l[start+2]]))
                continue;
            tmp_move.r_indice[0] = r_index;
            tmp_move.r_indice[1] = -2;
            tmp_move.len_1 = 3;
            tmp_move.seqList_1[0] = {r_index, 0, start-1};
            tmp_move.seqList_1[1] = {r_index, start+1, start};
            tmp_move.seqList_1[2] = {r_index, start+2, len-1};
            tmp_move.len_2 = 0;
            if (eval_move(s, tmp_move, data) && tmp_move.delta_cost < m.delta_cost)
            {
                m = tmp_move;
            }
        }
    }
}

void two_opt_star(Solution &s, Data &data, Move &m)
{
    m.delta_cost = double(INFINITY);
    int s_len = int(s.len());
    for (int r_index_1 = 0; r_index_1 < s_len; r_index_1++)
    {
        Route &r_1 = s.get(r_index_1);
        auto &n_l_1 = r_1.node_list;
        int len_1 = int(n_l_1.size());
        for (int r_index_2 = r_index_1 + 1; r_index_2 < s_len; r_index_2++)
        {
            Route &r_2 = s.get(r_index_2);
            auto &n_l_2 = r_2.node_list;
            int len_2 = int(n_l_2.size());
            for (int pos_1 = 1; pos_1 <= len_1 - 1; pos_1++)
            {
                for (int pos_2 = 1; pos_2 <= len_2 - 1; pos_2++)
                {
                    if ((pos_1 == 1 && pos_2 == 1) || (pos_1 == len_1-1 && pos_2 == len_2-1))
                        continue;
                    if (data.pruning &&
                        (!data.pm[n_l_1[pos_1-1]][n_l_2[pos_2]]) ||\
                         !data.pm[n_l_2[pos_2-1]][n_l_1[pos_1]])
                         continue;
                    tmp_move.r_indice[0] = r_index_1;
                    tmp_move.r_indice[1] = r_index_2;
                    tmp_move.len_1 = 2;
                    tmp_move.seqList_1[0] = {r_index_1, 0, pos_1-1};
                    tmp_move.seqList_1[1] = {r_index_2, pos_2, len_2-1};
                    tmp_move.len_2 = 2;
                    tmp_move.seqList_2[0] = {r_index_2, 0, pos_2-1};
                    tmp_move.seqList_2[1] = {r_index_1, pos_1, len_1-1};
                    if (eval_move(s, tmp_move, data) && tmp_move.delta_cost < m.delta_cost)
                    {
                        m = tmp_move;
                    }
                }
            }
        }
    }
}

void or_opt(Solution &s, Data &data, Move &m)
{
    m.delta_cost = double(INFINITY);
    // relocate a sequence
    int s_len = int(s.len());
    for (int r_index = 0; r_index < s_len; r_index++)
    {
        Route &r = s.get(r_index);
        auto &n_l = r.node_list;
        int len = int(n_l.size());
        for (int start = 1; start <= len - 2; start++)
        {
            for (int seq_len = 1; seq_len <= data.or_opt_len; seq_len++)
            {
                int end = start + seq_len - 1;
                if (end >= len - 1) continue;
                if (data.pruning &&
                    (!data.pm[n_l[start-1]][n_l[end+1]]))
                    continue;
                // relocate to the same route
                for (int pos = 1; pos <= start - 1; pos++)
                {
                    if (data.pruning &&
                        (!data.pm[n_l[pos-1]][n_l[start]] ||\
                         !data.pm[n_l[end]][n_l[pos]]))
                         continue;
                    tmp_move.r_indice[0] = r_index;
                    tmp_move.r_indice[1] = -2;
                    tmp_move.len_1 = 4;
                    tmp_move.seqList_1[0] = {r_index, 0, pos-1};
                    tmp_move.seqList_1[1] = {r_index, start, end};
                    tmp_move.seqList_1[2] = {r_index, pos, start-1};
                    tmp_move.seqList_1[3] = {r_index, end+1, len-1};
                    tmp_move.len_2 = 0;
                    if (eval_move(s, tmp_move, data) && tmp_move.delta_cost < m.delta_cost)
                    {
                        m = tmp_move;
                    }
                }
                for (int pos = end + 2; pos <= len - 1; pos++)
                {
                    if (data.pruning &&
                        (!data.pm[n_l[pos-1]][n_l[start]] ||
                         !data.pm[n_l[end]][n_l[pos]]))
                        continue;
                    tmp_move.r_indice[0] = r_index;
                    tmp_move.r_indice[1] = -2;
                    tmp_move.len_1 = 4;
                    tmp_move.seqList_1[0] = {r_index, 0, start-1};
                    tmp_move.seqList_1[1] = {r_index, end+1, pos-1};
                    tmp_move.seqList_1[2] = {r_index, start, end};
                    tmp_move.seqList_1[3] = {r_index, pos, len-1};
                    tmp_move.len_2 = 0;
                    if (eval_move(s, tmp_move, data) && tmp_move.delta_cost < m.delta_cost)
                    {
                        m = tmp_move;
                    }
                }
                // relocate to other route
                for (int r_index_2 = 0; r_index_2 < s_len; r_index_2++)
                {
                    if (r_index == r_index_2) continue;
                    Route &r_2 = s.get(r_index_2);
                    auto &n_l_2 = r_2.node_list;
                    int len_2 = int(n_l_2.size());
                    for (int pos = 1; pos <= len_2 - 1; pos++)
                    {
                        if (data.pruning &&
                            (!data.pm[n_l_2[pos-1]][n_l[start]])||\
                             !data.pm[n_l[end]][n_l_2[pos]])
                             continue;
                        tmp_move.r_indice[0] = r_index;
                        tmp_move.r_indice[1] = r_index_2;
                        tmp_move.len_1 = 2;
                        tmp_move.seqList_1[0] = {r_index, 0, start-1};
                        tmp_move.seqList_1[1] = {r_index, end+1, len-1};
                        tmp_move.len_2 = 3;
                        tmp_move.seqList_2[0] = {r_index_2, 0, pos-1};
                        tmp_move.seqList_2[1] = {r_index, start, end};
                        tmp_move.seqList_2[2] = {r_index_2, pos, len_2-1};
                        if (eval_move(s, tmp_move, data) && tmp_move.delta_cost < m.delta_cost)
                        {
                            m = tmp_move;
                        }
                    }
                }
                // relocate to a new route
                tmp_move.r_indice[0] = r_index;
                tmp_move.r_indice[1] = -1;
                tmp_move.len_1 = 2;
                tmp_move.seqList_1[0] = {r_index, 0, start-1};
                tmp_move.seqList_1[1] = {r_index, end+1, len-1};
                tmp_move.len_2 = 3;
                tmp_move.seqList_2[0] = {-1, data.DC, data.DC};
                tmp_move.seqList_2[1] = {r_index, start, end};
                tmp_move.seqList_2[2] = {-1, data.DC, data.DC};
                if (eval_move(s, tmp_move, data) && tmp_move.delta_cost < m.delta_cost)
                {
                    m = tmp_move;
                }
            }
        }
    }
}

void two_exchange(Solution &s, Data &data, Move &m)
{
    m.delta_cost = double(INFINITY);
    // exchange two sequences with seqs
    int s_len = int(s.len());
    for(int r_index_1 = 0; r_index_1 < s_len; r_index_1++)
    {
        Route &r_1 = s.get(r_index_1);
        auto &n_l_1 = r_1.node_list;
        int len_1 = int(n_l_1.size());
        for(int r_index_2 = r_index_1 + 1; r_index_2 < s_len; r_index_2++)
        {
            Route &r_2 = s.get(r_index_2);
            auto &n_l_2 = r_2.node_list;
            int len_2 = int(n_l_2.size());
            for (int start_1 = 1; start_1 <= len_1 - 2; start_1++)
            {
                for (int seq_len_1 = 1; seq_len_1 <= data.exchange_len; seq_len_1++)
                {
                    int end_1 = start_1 + seq_len_1 - 1;
                    if (end_1 >= len_1 - 1) continue;
                    for (int start_2 = 1; start_2 <= len_2 - 2; start_2++)
                    {
                        for (int seq_len_2 = 1; seq_len_2 <= data.exchange_len; seq_len_2++)
                        {
                            int end_2 = start_2 + seq_len_2 - 1;
                            if (end_2 >= len_2 - 1) continue;
                            if (data.pruning &&
                                (!data.pm[n_l_1[start_1-1]][n_l_2[start_2]] ||\
                                 !data.pm[n_l_2[end_2]][n_l_1[end_1+1]] ||\
                                 !data.pm[n_l_2[start_2-1]][n_l_1[start_1]] ||\
                                 !data.pm[n_l_1[end_1]][n_l_2[end_2+1]]))
                                continue;
                            tmp_move.r_indice[0] = r_index_1;
                            tmp_move.r_indice[1] = r_index_2;
                            tmp_move.len_1 = 3;
                            tmp_move.seqList_1[0] = {r_index_1, 0, start_1-1};
                            tmp_move.seqList_1[1] = {r_index_2, start_2, end_2};
                            tmp_move.seqList_1[2] = {r_index_1, end_1+1, len_1-1};
                            tmp_move.len_2 = 3;
                            tmp_move.seqList_2[0] = {r_index_2, 0, start_2-1};
                            tmp_move.seqList_2[1] = {r_index_1, start_1, end_1};
                            tmp_move.seqList_2[2] = {r_index_2, end_2+1, len_2-1};
                            if (eval_move(s, tmp_move, data) && tmp_move.delta_cost < m.delta_cost)
                            {
                                m = tmp_move;
                            }
                        }
                    }
                }
            }
        }
    }
}

void related_removal(Solution &s, Data &data)
{
    random_removal(s, data);
}

void random_removal(Solution &s, Data &data)
{
    // randomly delete customers from s
    std::vector<int> customers(data.customer_num);
    std::vector<int> indice(data.customer_num+1, 0);
    int count = 0;
    for(int i = 0; i < data.customer_num + 1; i++)
    {
        if (i == data.DC) continue;
        customers[count] = i;
        count++;
    }
    shuffle(customers.begin(), customers.end(), data.rng);
    int boundray = int(round(double(data.customer_num) *\
                       rand(data.destroy_ratio_l, data.destroy_ratio_u, data.rng)));
    for (int i = 0; i < boundray + 1; i++) {indice[customers[i]] = 1;}

    int s_len = s.len();
    for (int r_index = 0; r_index < s_len; r_index++)
    {
        Route &r = s.get(r_index);
        auto &n_l = r.node_list;
        int len = int(n_l.size());
        for (int i = 0; i < len; i++)
        {
            if (indice[n_l[i]] == 1)
            {
                n_l.erase(n_l.begin()+i);
                i--;
                len--;
            }
        }
    }
    s.update(data);
}

void apply_move(Solution &s, Move &m, Data data)
{
    std::vector<int> r_indice;
    r_indice.push_back(m.r_indice[0]);
    if (m.r_indice[1] != -2)
        r_indice.push_back(m.r_indice[1]);

    // handle the first route
    if (r_indice[0] == -2 || r_indice[0] == -1)
    {
        printf("Error: detect -1 or -2 in r_indice[0] in move\n");
        exit(-1);
    }
    Route &r = s.get(r_indice[0]);
    std::vector<int> target_n_l;
    target_n_l.reserve(MAX_NODE_IN_ROUTE);

    for (int i = 0; i < m.len_1; i++)
    {
        auto &seq = m.seqList_1[i];
        auto &source_n_l = s.get(seq.r_index).node_list;
        for (int index = seq.start_point; index <= seq.end_point; index++)
            {target_n_l.push_back(source_n_l[index]);}
    }

    // handle the second route
    if (int(r_indice.size()) == 2)
    {
        std::vector<int> target_n_l_2;
        target_n_l_2.reserve(MAX_NODE_IN_ROUTE);

        for (int i = 0; i < m.len_2; i++)
        {
            auto &seq = m.seqList_2[i];
            if (seq.r_index == -1)
            {
                target_n_l_2.push_back(data.DC);
                continue;
            }
            auto &source_n_l = s.get(seq.r_index).node_list;
            for (int index = seq.start_point; index <= seq.end_point; index++)
            {
                target_n_l_2.push_back(source_n_l[index]);
            }
        }
        if (r_indice[1] == -1)
        {
            Route r(data);
            r.node_list = target_n_l_2;
            r.update(data);
            s.append(r);
            r_indice[1] = s.len() - 1;
        }
        else
        {
            Route &r = s.get(r_indice[1]);
            r.node_list = target_n_l_2;
            r.update(data);
        }
    }
    // set node list only at the end
    r.node_list = target_n_l;
    r.update(data);

    s.local_update(r_indice, data);
}


void find_local_optima(Solution &s, Data &data)
{
    // record the best solution in the neighborhood of each small opt
    std::vector<Move> move_list(int(data.small_opts.size()));

    for (int i = 0; i < int(move_list.size()); i++)
        {small_opt_map[data.small_opts[i]](s, data, move_list[i]);}

    while (true)
    {
        int best_index = -1;
        double min_delta_cost = double(INFINITY);
        for (int i = 0; i < int(move_list.size()); i++)
        {
            if (move_list[i].delta_cost - min_delta_cost < -PRECISION)
            {
                best_index = i;
                min_delta_cost = move_list[i].delta_cost;
            }
        }
        if (min_delta_cost < -PRECISION)
        {
            // apply move
            apply_move(s, move_list[best_index], data);
            // update move_list
            for (int i = 0; i < int(move_list.size()); i++)
                {small_opt_map[data.small_opts[i]](s, data, move_list[i]);}
        }
        else break;
    }
}

void do_local_search(Solution &s, Data &data)
{
    if (int(data.small_opts.size() == 0))
    {
        printf("No small stepsize operator used, directly return.\n");
        return;
    }
    find_local_optima(s, data);
    s.cal_cost(data);
    std::cout << "Found local optima. Cost " << s.cost << " " << std::endl;
}

void perturb(std::vector<Solution> &s_vector, Data &data)
{
    int count = 0;
    for (int i = 0; i < int(data.destroy_opts.size()); i++)
    {
        for (int j = 0; j < int(data.repair_opts.size()); j++)
        {
            destroy_opt_map[data.destroy_opts[i]](s_vector[count], data);
            repair_opt_map[data.repair_opts[i]](s_vector[count], data);
            count++;
        }
    }
}