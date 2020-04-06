/*
An efficient large-neighborhood search algorithm for solving the 
Simaultanes Delivery and Pick-up Vehicle Routing
Problem with Time Windows (VRPSDPTW)

Created on: 2020.3.13
by Shengcai Liu
*/
#include "lns.h"

Solution best_s;
clock_t find_best_time;
clock_t find_bks_time;
bool find_better;

void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    if (std::abs(best_s.cost - double(INFINITY)) < PRECISION)
        std::cout << "Not found any feasible solution yet.\n";
    else
    {
        std::cout << best_s.build_output_str();
        std::cout << "Time to find this solution: " << find_best_time <<"\n";
        if (find_better)
            std::cout << "Better than BKS. ";
        else
            std::cout << "Worse than BKS. ";
        std::cout << "Time to surpass BKS: " << find_bks_time <<"\n";
    }
    exit(signum);
}

int main(int argc, char **argv)
{
    best_s.cost = double(INFINITY);
    find_best_time = find_bks_time = 0;
    find_better = false;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    ArgumentParser parser;
    // required arguments
    parser.addArgument("--problem", 1, false);

    // optional
    parser.addArgument("--pruning");
    parser.addArgument("--output", 1);
    parser.addArgument("--time", 1);
    parser.addArgument("--runs", 1);
    parser.addArgument("--ksize", 1);
    parser.addArgument("--init", 1);
    parser.addArgument("--O_1_eval");
    parser.addArgument("--two_opt");
    parser.addArgument("--two_opt_star");
    parser.addArgument("--or_opt");
    parser.addArgument("--two_exchange");
    parser.addArgument("--elo", 1);
    parser.addArgument("--random_removal");
    parser.addArgument("--related_removal");
    parser.addArgument("--regret_insertion");
    parser.addArgument("--greedy_insertion");
    parser.addArgument("--bks", 1);

    parser.addArgument("--random_seed", 1);

    // parse the command-line arguments - throws if invalid format
    parser.parse(argc, argv);
    Data data(parser);
    search_framework(data, best_s);

    return 0;
}