#include "search_framework.h"

void update_best_solution(Solution &s, Solution &best_s)
{
    if (s.cost - best_s.cost < -PRECISION)
    {
        best_s = s;
        printf("Best solution update: %.4f\n", best_s.cost);
    }
}

void search_framework(Data &data, Solution &best_s)
{
    clock_t stime = clock();
    clock_t used = 0;

    /* main body */
    bool time_exhausted = false;
    int run = 1;
    for (; run <= data.runs; run++)
    {
        printf("---------------------------------Run %d---------------------------\n", run);
        int no_improve = 0;
        Solution s(data);
        initialization(s, data);
        // do local search
        do_local_search(s, data);
        update_best_solution(s, best_s);
        used = (clock() - stime) / CLOCKS_PER_SEC;
        if (data.tmax != NO_LIMIT && used > clock_t(data.tmax))
        {
            time_exhausted = true;
            break;
        }

        while (no_improve < data.escape_local_optima)
        {
            int tmp_solution_num = int(data.destroy_opts.size()) * int(data.repair_opts.size());

            vector<Solution> s_vector(tmp_solution_num, s);
            perturb(s_vector, data);

            int best_index = -1;
            double best_cost = double(INFINITY);
            for (int i = 0; i < tmp_solution_num; i++)
            {
                do_local_search(s_vector[i], data);
                if (s_vector[i].cost - best_cost < -PRECISION)
                {
                    best_index = i;
                    best_cost = s_vector[i].cost;
                }
            }
            if (s_vector[best_index].cost - s.cost < - PRECISION)
            {
                s = s_vector[best_index];
                printf("Found improvement. Cost %.4f\n", s.cost);
                no_improve = 0;
                update_best_solution(s_vector[best_index], best_s);
            }
            else
            {
                no_improve++;
                printf("Not found improvement for %d iterations.\n", no_improve);
            }

            if (data.tmax != NO_LIMIT && used > clock_t(data.tmax))
            {
                time_exhausted = true;
                break;
            }
        }
        printf("Run %d finishes, the best soltuion found in this run and the global best solution are: %.4f %.4f\n", run, s.cost, best_s.cost);
        data.rng.seed(data.seed + run);
        if (time_exhausted) break;
    }
    // output best solution
    printf("------------Summary-----------\n");
    printf("Total %d runs, total consumed %d sec\n", run, used);
    best_s.output(data);
    best_s.check(data);
}

void initialization(Solution &s, Data &data)
{
    printf("Initialization, using %s method. ", data.init.c_str());
    if (data.init == RCRS)
    {
        data.n_insert = RCRS;
        double lambda = rand(0, 1, data.rng);
        double gamma = rand(0, 1, data.rng);
        printf("lambda, gamma: %f, %f\n", lambda, gamma);
        get<0>(data.lambda_gamma) = lambda;
        get<1>(data.lambda_gamma) = gamma;
        new_route_insertion(s, data);
    }
    else if (data.init == TD)
    {
        data.n_insert = TD;
        new_route_insertion(s, data);
    }
    else
    {
        /* more insertion heuristic */
    }
    printf("Initialization done. Cost %.4f\n", s.cost);
}