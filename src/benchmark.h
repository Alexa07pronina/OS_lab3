#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <time.h>

bool getTime(timespec& t);
double getSeconds(const timespec& start, const timespec& end);

#endif