/*
An efficient large-neighborhood search algorithm for solving the 
Simaultanes Delivery and Pick-up Vehicle Routing
Problem with Time Windows (VRPSDPTW)

Created on: 2020.3.13
by Shengcai Liu
*/
#include "lns.h"

Solution best_s;

void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    if (abs(best_s.cost - double(INFINITY)) < PRECISION)
        std::cout << "Not found any feasible solution yet.\n";
    else
        std::cout << best_s.build_output_str();
    exit(signum);
}

int main(int argc, char **argv)
{
    const rlim_t kStackSize = RLIM_INFINITY;
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            }
        }
    }

    best_s.cost = double(INFINITY);
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

    parser.addArgument("--random_seed", 1);

    // parse the command-line arguments - throws if invalid format
    parser.parse(argc, argv);
    Data data(parser);
    search_framework(data, best_s);

    std::cin.get();
    return 0;
}