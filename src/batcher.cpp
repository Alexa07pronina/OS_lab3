#include "batcher.h"
#include <cstdint>

bool getNetworkSize(size_t n, size_t& networkSize)
{
    networkSize = 1;
    while (networkSize < n)
    {
        if (networkSize > SIZE_MAX / 2)
            return false;

        networkSize *= 2;
    }
    return true;
}

void compareStage(int* arr, size_t n, size_t p, size_t k, int id, int threadCount)
{
    for (size_t i = id; i < n; i += threadCount)
    {
        if (k >= n - i)
            continue;

        size_t pos = i % (2 * p);
        bool needCompare = false;

        if (k == p)
        {
            if (pos < p)
                needCompare = true;
        }
        else
        {
            if (pos >= k && pos < 2 * p - k)
            {
                if (i % (2 * k) >= k)
                    needCompare = true;
            }
        }

        if (needCompare)
        {
            if (arr[i] > arr[i + k])
            {
                int temp = arr[i];
                arr[i] = arr[i + k];
                arr[i + k] = temp;
            }
        }
    }
}

bool isSorted(const int* arr, size_t n)
{
    for (size_t i = 1; i < n; i++)
    {
        if (arr[i - 1] > arr[i])
            return false;
    }

    return true;
}