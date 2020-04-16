#include "data.h"

Data::Data(ArgumentParser &parser)
{
    // read problem file
    const char* pro_file = parser.retrieve<std::string>("problem").c_str();
    char Buffer[N];
    std::ifstream fp;
    fp.rdbuf()->pubsetbuf(Buffer, N);
    fp.open(pro_file);

    std::string line;
    double all_pickup = 0.0;
    double all_delivery = 0.0;
    double all_dist = 0.0;
    double all_time = 0.0;
    bool flag = true;
    while (true)
    {
        if (flag)
        {
            if (!std::getline(fp, line)) break;
        }
        else flag = true;
        trim(line);
        if (line.size() == 0) continue;
        std::vector<std::string> results = split(line, ':');
        trim(results[0]);
        if (results.size() > 1) trim(results[1]);
        if (results[0] == "NAME")
        {
            printf("%s\n", line.c_str());
            this->problem_name = results[1];
        }
        else if (results[0] == "TYPE")
        {
            printf("%s\n", line.c_str());
        }
        else if (results[0] == "DIMENSION")
        {
            printf("%s\n", line.c_str());
            this->customer_num = stoi(results[1]) - 1;
            std::vector<double> tmp_v_1(this->customer_num+1, 0.0);
            std::vector<bool> tmp_v_2(this->customer_num+1, 0.0);
            std::vector<int> tmp_v_3(this->customer_num+1, 0);
            for (int i = 0; i <= this->customer_num; i++)
            {
                this->node.push_back({0, 0.0, 0.0, 0.0, 0.0, 0.0});
                this->dist.push_back(tmp_v_1);
                this->time.push_back(tmp_v_1);
                this->rm.push_back(tmp_v_1);
                this->rm_argrank.push_back(tmp_v_3);
                this->pm.push_back(tmp_v_2);
            }
        }
        else if (results[0] == "VEHICLES")
        {
            printf("%s\n", line.c_str());
            this->vehicle.max_num = stoi(results[1]);
        }
        else if (results[0] == "DISPATCHINGCOST")
        {
            printf("%s\n", line.c_str());
            this->vehicle.d_cost = stod(results[1]);
        }
        else if (results[0] == "UNITCOST")
        {
            printf("%s\n", line.c_str());
            this->vehicle.unit_cost = stod(results[1]);
        }
        else if (results[0] == "CAPACITY")
        {
            printf("%s\n", line.c_str());
            this->vehicle.capacity = stod(results[1]);
        }
        else if (results[0] == "EDGE_WEIGHT_TYPE")
        {
            printf("%s\n", line.c_str());
            if (results[1] != "EXPLICIT")
            {
                printf("Expect edge weight type: EXPLICIT, while accept type: %s\n", results[1].c_str());
                exit(-1);
            }
        }
        else if (results[0] == "NODE_SECTION")
        {
            while (getline(fp, line))
            {
                trim(line);
                if (line.size() == 0) continue;
                std::vector<std::string> r = split(line, ',');
                if (r.size() > 1)
                {
                    trim(r[0]);
                    int i = stoi(r[0]);
                    trim(r[1]);
                    this->node[i].delivery = stod(r[1]);
                    all_delivery += this->node[i].delivery;
                    trim(r[2]);
                    this->node[i].pickup = stod(r[2]);
                    all_pickup += this->node[i].pickup;
                    trim(r[3]);
                    this->node[i].start = stod(r[3]);
                    trim(r[4]);
                    this->node[i].end = stod(r[4]);
                    trim(r[5]);
                    this->node[i].s_time = stod(r[5]);
                }
                else
                {
                    flag = false;
                    break;
                }

            }
        }
        else if (results[0] == "DISTANCETIME_SECTION")
        {
            while (getline(fp, line))
            {
                trim(line);
                if (line.size() == 0)
                    continue;
                std::vector<std::string> r = split(line, ',');
                if (r.size() > 1)
                {
                    trim(r[0]);
                    int i = stoi(r[0]);
                    trim(r[1]);
                    int j = stoi(r[1]);
                    trim(r[2]);
                    double d = stod(r[2]);
                    trim(r[3]);
                    double t = stod(r[3]);
                    this->dist[i][j] = d;
                    all_dist += d;
                    this->time[i][j] = t;
                    all_time += t;
                    if (d < this->min_dist) this->min_dist = d;
                    if (d > this->max_dist) this->max_dist = d;
                }
                else
                {
                    flag = false;
                    break;
                }
            }
        }
        else if (results[0] == "DEPOT_SECTION")
        {
            getline(fp, line);
            trim(line);
            this->DC = stoi(line);
        }
    }
    fp.close();

    this->start_time = this->node[this->DC].start;
    this->end_time = this->node[this->DC].end;
    this->all_delivery = all_delivery;
    this->all_pickup = all_pickup;

    // print summary information
    printf("Avg pick-up/dilvery demand: %.4f,%.4f\n", this->all_pickup/this->customer_num, this->all_delivery/this->customer_num);
    printf("Starting/end time of DC: %.4f,%.4f\n", this->start_time, this->end_time);

    std::cout << '\n';
    if (parser.exists("random_seed"))
        this->seed = std::stoi(parser.retrieve<std::string>("random_seed"));
    this->rng.seed(this->seed);
    printf("Initial random seed: %d\n", this->seed);

    // set parameters
    if (parser.exists("pruning"))
    {
        printf("Pruning: on\n");
        this->pruning = true;
    }
    else printf("Pruning: off\n");

    if (parser.exists("output"))
    {
        this->if_output = true;
        this->output = parser.retrieve<std::string>("output");
        std::cout << "Write best solution to " << this->output << '\n';
    }

    if (parser.exists("time"))
        this->tmax = std::stoi(parser.retrieve<std::string>("time"));
    printf("Time limit: %d seconds\n", this->tmax);

    if (parser.exists("runs"))
        this->runs = std::stoi(parser.retrieve<std::string>("runs"));
    printf("Runs: %d\n", this->runs);

    if (parser.exists("ksize"))
        this->ksize = std::stoi(parser.retrieve<std::string>("ksize"));
    if (this->ksize == K)
        this->ksize = this->customer_num;
    printf("ksize: %d\n", this->ksize);

    if (parser.exists("init"))
        this->init = parser.retrieve<std::string>("init");
    printf("Insertion for initialization: %s\n", this->init.c_str());

    if (parser.exists("latin"))
    {
        double step = 1.0 / (LATIN_NUM-1);
        for (int i = 0; i < LATIN_NUM; i++)
        {
            for (int j = 0; j < LATIN_NUM; j++)
            {
                double lambda = std::min(1.0, step*i);
                double gamma = std::min(1.0, step*j);
                this->latin.push_back(std::make_tuple(lambda, gamma));
            }
        }
        std::shuffle(this->latin.begin(), this->latin.end(), this->rng);
    }
    printf("Latin sampling for Lambda and Gamma for rcrs insertion: on\n");
    if (parser.exists("O_1_eval"))
    {
        printf("O(1) evaluation: on\n");
        this->O_1_evl = true;
    }
    else
        printf("O(1) evaluation: off");

    if (parser.exists("two_opt"))
    {
        printf("2-opt: on\n");
        this->two_opt = true;
    }
    else
        printf("2-opt: off\n");

    if (parser.exists("two_opt_star"))
    {
        printf("2-opt*: on\n");
        this->two_opt_star = true;
    }
    else
        printf("2-opt*: off\n");

    if (parser.exists("or_opt"))
    {
        printf("or-opt: on\n");
        this->or_opt = true;
        this->or_opt_len = std::stoi(parser.retrieve<std::string>("or_opt"));
    }
    else
        printf("or-opt: off\n");

    if (parser.exists("two_exchange"))
    {
        printf("2-exchange: on\n");
        this->two_exchange = true;
        this->exchange_len = std::stoi(parser.retrieve<std::string>("two_exchange"));
    }
    else
        printf("2-exchange: off\n");

    if (parser.exists("elo"))
        this->escape_local_optima = std::stoi(parser.retrieve<std::string>("elo"));
    printf("escape local optima number: %d\n", this->escape_local_optima);

    if (parser.exists("random_removal"))
    {
        printf("random_removal: on\n");
        this->random_removal = true;
    }
    else
        printf("random_removal: off\n");

    if (parser.exists("related_removal"))
    {
        printf("related_removal: on\n");
        this->related_removal = true;
        if (parser.exists("alpha"))
            this->alpha = std::stod(parser.retrieve<std::string>("alpha"));
        this->r = this->alpha * (all_dist / all_time);
        printf("alpha: %f, relateness norm factor: %f\n", this->alpha, this->r);
    }
    else
        printf("related_removal: off\n");

    if (parser.exists("regret_insertion"))
    {
        printf("regret_insertion: on\n");
        this->regret_insertion = true;
    }
    else
        printf("regret_insertion: off\n");

    if (parser.exists("greedy_insertion"))
    {
        printf("greedy_insertion: on\n");
        this->greedy_insertion = true;
    }
    else
        printf("greedy_insertion: off\n");

    if (this->two_opt)
        small_opts.push_back("2opt");
    if (this->two_opt_star)
        small_opts.push_back("2opt*");
    if (this->or_opt)
        small_opts.push_back("oropt");
    if (this->two_exchange)
        small_opts.push_back("2exchange");
    if (this->random_removal)
        destroy_opts.push_back("random_removal");
    if (this->related_removal)
        destroy_opts.push_back("related_removal");
    if (this->regret_insertion)
        repair_opts.push_back("regret_insertion");
    if (this->greedy_insertion)
        repair_opts.push_back("greedy_insertion");

    if (parser.exists("bks"))
        this->bks = std::stod(parser.retrieve<std::string>("bks"));

    int c_num = this->customer_num;
    for (int i = 0; i <= c_num; i++)
    {
        for (int j = 0; j <= c_num; j++)
            {this->pm[i][j] = true;}
    }
    this->pre_processing();
}

void Data::pre_processing()
{
    printf("--------------------------------------------\n");
    if (this->related_removal)
    {
        int c_num = this->customer_num;
        int DC = this->DC;
        for (int i = 0; i <= c_num; i++)
        {
            if (i == DC) continue;
            for (int j = 0; j <= c_num; j++)
            {
                if(j == DC || j == i)
                    this->rm[i][j] = double(INFINITY);
                else
                {
                    auto &node_i = this->node[i];
                    auto &node_j = this->node[j];
                    double tmp_1 = this->r * std::max(node_j.start - node_i.s_time - this->time[i][j] - node_i.end, 0.0);
                    double tmp_2 = this->r * PENALTY_FACTOR * std::max(node_i.start + node_i.s_time + this->time[i][j] - node_j.end, 0.0);
					double tmp_3 = this->dist[i][j];
                    this->rm[i][j] = tmp_3 + tmp_1 + tmp_2;
                }
            }
            argsort(this->rm[i], this->rm_argrank[i], c_num+1);
        }
    }
    if (this->pruning)
    {
        printf("Do Pruning\n");
        int c_num = this->customer_num;
        int DC = this->DC;
        int count_tw = 0;
        int count_c = 0;
        for (int i = 0; i <= c_num; i++)
        {
            if (i == DC) continue;
            for (int j = 0; j <= c_num; j++)
            {
                if (j == DC || j == i) continue;

                double a_i = this->node[i].start;
                double s_i = this->node[i].s_time;
                double d_i = this->node[i].delivery;
                double p_i = this->node[i].pickup;

                double b_j = this->node[j].end;
                double d_j = this->node[j].delivery;
                double p_j = this->node[j].pickup;
                double time_ij = this->time[i][j];

                if (a_i + s_i + time_ij > b_j)
                {
                    this->pm[i][j] = false;
                    count_tw++;
                }
                if (d_i + d_j > this->vehicle.capacity || p_i + p_j > this->vehicle.capacity)
                {
                    this->pm[i][j] = false;
                    count_c++;
                }
            }
        }
        int sum = c_num*(c_num-1);
        printf("Total edges %d, prune by time window %d(%.4f%%),prune by capacity %d(%.4f%%)\n",\
                sum, count_tw, 100.0*double(count_tw)/sum, count_c, 100.0*double(count_c)/sum);
    }
}