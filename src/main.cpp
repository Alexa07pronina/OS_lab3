#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <new>
#include <unistd.h>

#include "batcher.h"
#include "threads.h"
#include "benchmark.h"

using namespace std;

void printArray(const int* arr, size_t n)
{
    for (size_t i = 0; i < n; i++)
        cout << arr[i] << " ";

    cout << endl;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cout << "Usage: ./lab3 <size> <threads>" << endl;
        return 1;
    }
    errno = 0;
    char* end;
    unsigned long long sizeValue = strtoull(argv[1], &end, 10);

    if (errno != 0 || end == argv[1] || *end != '\0'
        || argv[1][0] == '-' || sizeValue == 0
        || sizeValue > SIZE_MAX / sizeof(int))
    {
        cerr << "Invalid array size" << endl;
        return 1;
    }

    size_t n = (size_t)sizeValue;
    errno = 0;
    long threadsValue = strtol(argv[2], &end, 10);

    if (errno != 0 || end == argv[2] || *end != '\0'
        || threadsValue < 1 || threadsValue > INT_MAX)
    {
        cerr << "Invalid thread count" << endl;
        return 1;
    }

    int threadCount = (int)threadsValue;

    if ((size_t)threadCount > n)
        threadCount = (int)n;

    int* arr = new (nothrow) int[n];
    if (arr == nullptr)
    {
        cerr << "Memory allocation failed" << endl;
        return 1;
    }

    srand(123);
    for (size_t i = 0; i < n; i++)
        arr[i] = rand() % 20001 - 10000;

    cout << "PID: " << getpid() << endl;
    cout << "Array size: " << n << endl;
    cout << "Threads: " << threadCount << endl;

    if (n <= 30)
    {
        cout << "Before: ";
        printArray(arr, n);
    }

    timespec start, endTime;

    if (!getTime(start))
    {
        cerr << "Timer error" << endl;
        delete[] arr;
        return 1;
    }

    bool success = parallelSort(arr, n, threadCount);

    if (!getTime(endTime))
    {
        cerr << "Timer error" << endl;
        delete[] arr;
        return 1;
    }

    if (!success)
    {
        cerr << "Sorting failed" << endl;
        delete[] arr;
        return 1;
    }

    if (n <= 30)
    {
        cout << "After: ";
        printArray(arr, n);
    }

    bool sorted = isSorted(arr, n);
    if (sorted)
        cout << "Sorting: OK" << endl;
    else
        cout << "Sorting: FAILED" << endl;

    cout << "Time: " << getSeconds(start, endTime) << " seconds" << endl;
    delete[] arr;
    return sorted ? 0 : 1;
}   