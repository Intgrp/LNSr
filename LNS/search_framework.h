/*
Leave as much space as possible for adding more implementation options of each
function, such that maybe we could use this as an algorithm
framework for further automatic algorithm design
*/

#pragma once
#include "stdio.h"
#include "data.h"
#include "solution.h"
#include "time.h"
#include "operator.h"
#include "config.h"
using namespace std;

void search_framework(Data &data, Solution &best_s);

void initialization(Solution &s, Data &data);