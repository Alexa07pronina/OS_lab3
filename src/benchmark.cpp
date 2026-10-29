#include "benchmark.h"

bool getTime(timespec& t)
{
    return clock_gettime(CLOCK_MONOTONIC, &t) == 0;
}

double getSeconds(const timespec& start, const timespec& end)
{
    double seconds = end.tv_sec - start.tv_sec;
    double nanoseconds = end.tv_nsec - start.tv_nsec;

    return seconds + nanoseconds / 1000000000.0;
}