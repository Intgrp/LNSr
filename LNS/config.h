#pragma once
#include "limits.h"

// The following definitions might need to be changed
// for different problems
const int MAX_POINT = 1250;
const int MAX_NODE_IN_ROUTE = 100;
const int DEFAULT_SEED = 42;

const int INF = INT_MAX;
const int N = 8192; //buffer size for raading problem
const int NO_LIMIT = -1; // default time limit
const int RUNS = 10; //default max restart times
const double PRECISION = 0.001; // difference smaller than this is considered the same
const bool DEFAULT_IF_OUTPUT = false;

/* pre-processing */
const bool DEFAULT_PRUNING = false;

/* local search */
const bool DEFAULT_O_1_EVAL = false;
const bool DEFAULT_2_OPT_STAR = true;
const bool DEFAULT_2_OPT = false;
const bool DEFAULT_OR_OPT = false;
const bool DEFAULT_2_EX = false;
const int DEFAULT_OR_OPT_LEN = 3;
const int DEFAUTL_EX_LEN = 2;

/* large neighborhood opts */
const int DEFAULT_ELO = 1;
const double DEFAULT_DESTROY_RATIO_L = 0.2;
const double DEFAULT_DESTROY_RATIO_U = 0.4;
const bool DEFAULT_RD_REMOVAL = false;
const bool DEFAULT_RT_REMOVAL = false;
const bool DEFAULT_GD_INSERTION = false;
const bool DEFAULT_RG_INSERTION = false;

/* insertion for initialization: insertion without considering existing routes,
each time building a new route with an arbitrary selected customer.
K is the number of the selected initial customers. The final solution
is the best one among the K solutions */
const std::string RCRS = "rcrs"; // RCRS insertion heuristic
const std::string TD = "td";  // travel-distance based insertion
const int K = -1; // the number of generated solutions for producing a solution
const std::string DEFAULT_INIT = RCRS;

/* removal and insertion */
const double DEFAULT_ALPHA = 1.0; // relatedness para
const double PENALTY_FACTOR = 10.0; // penalty factor for tw constraint

// The flags for a sequence status
const int INFEASIBLE = -1;