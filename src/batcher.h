#ifndef BATCHER_H
#define BATCHER_H

#include <cstddef>

bool getNetworkSize(size_t n, size_t& networkSize);

void compareStage(int* arr, size_t n,
                  size_t p, size_t k,
                  int id, int threadCount);

bool isSorted(const int* arr, size_t n);

#endif