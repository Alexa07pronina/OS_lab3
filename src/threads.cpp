
#include "threads.h"
#include "batcher.h"

#include <pthread.h>
#include <iostream>
#include <new>
#include <cstring>
#include <unistd.h>
#include <sys/syscall.h>

using namespace std;

struct SharedData
{
    int* arr;
    size_t n;
    size_t networkSize;
    int threadCount;

    pthread_barrier_t barrier;
    pthread_mutex_t mutex;
    pthread_cond_t condition;

    bool start;
    bool cancel;
};

struct ThreadData
{
    SharedData* shared;
    int id;
    long tid;
};

void fatalError(const char* name, int rc)
{
    cerr << name << ": " << strerror(rc) << endl;
    _exit(1);
}

bool initSync(SharedData* s)
{
    int rc = pthread_mutex_init(&s->mutex, nullptr);

    if (rc != 0)
    {
        cerr << "pthread_mutex_init: "
             << strerror(rc) << endl;
        return false;
    }

    rc = pthread_cond_init(&s->condition, nullptr);

    if (rc != 0)
    {
        cerr << "pthread_cond_init: "
             << strerror(rc) << endl;

        int r = pthread_mutex_destroy(&s->mutex);
        if (r != 0)
            fatalError("pthread_mutex_destroy", r);

        return false;
    }

    rc = pthread_barrier_init(
        &s->barrier, nullptr, s->threadCount
    );

    if (rc != 0)
    {
        cerr << "pthread_barrier_init: "
             << strerror(rc) << endl;

        int r = pthread_cond_destroy(&s->condition);
        if (r != 0)
            fatalError("pthread_cond_destroy", r);

        r = pthread_mutex_destroy(&s->mutex);
        if (r != 0)
            fatalError("pthread_mutex_destroy", r);

        return false;
    }

    return true;
}

void destroySync(SharedData* s)
{
    int rc = pthread_barrier_destroy(&s->barrier);

    if (rc != 0)
        fatalError("pthread_barrier_destroy", rc);

    rc = pthread_cond_destroy(&s->condition);

    if (rc != 0)
        fatalError("pthread_cond_destroy", rc);

    rc = pthread_mutex_destroy(&s->mutex);

    if (rc != 0)
        fatalError("pthread_mutex_destroy", rc);
}

void runSort(ThreadData* data)
{
    SharedData* s = data->shared;

    for (size_t p = 1; p < s->networkSize; p *= 2)
    {
        size_t k = p;

        while (k > 0)
        {
            compareStage(
                s->arr, s->n, p, k,
                data->id, s->threadCount
            );

            if (s->threadCount > 1)
            {
                int rc = pthread_barrier_wait(&s->barrier);

                if (rc != 0 &&
                    rc != PTHREAD_BARRIER_SERIAL_THREAD)
                {
                    fatalError("pthread_barrier_wait", rc);
                }
            }

            k /= 2;
        }
    }
}

void* worker(void* arg)
{
    ThreadData* data = (ThreadData*)arg;
    SharedData* s = data->shared;

    data->tid = syscall(SYS_gettid);

    int rc = pthread_mutex_lock(&s->mutex);

    if (rc != 0)
        fatalError("pthread_mutex_lock", rc);

    while (!s->start)
    {
        rc = pthread_cond_wait(
            &s->condition, &s->mutex
        );

        if (rc != 0)
            fatalError("pthread_cond_wait", rc);
    }

    bool cancel = s->cancel;

    rc = pthread_mutex_unlock(&s->mutex);

    if (rc != 0)
        fatalError("pthread_mutex_unlock", rc);

    if (!cancel)
        runSort(data);

    return nullptr;
}

void startWorkers(SharedData* s, bool cancel)
{
    int rc = pthread_mutex_lock(&s->mutex);

    if (rc != 0)
        fatalError("pthread_mutex_lock", rc);

    s->cancel = cancel;
    s->start = true;

    rc = pthread_cond_broadcast(&s->condition);

    if (rc != 0)
        fatalError("pthread_cond_broadcast", rc);

    rc = pthread_mutex_unlock(&s->mutex);

    if (rc != 0)
        fatalError("pthread_mutex_unlock", rc);
}

void joinWorkers(pthread_t* threads, int count)
{
    for (int i = 0; i < count; i++)
    {
        int rc = pthread_join(threads[i], nullptr);

        if (rc != 0)
            fatalError("pthread_join", rc);
    }
}

bool parallelSort(int* arr, size_t n, int threadCount)
{
    if (arr == nullptr || threadCount < 1)
        return false;

    size_t networkSize;

    if (!getNetworkSize(n, networkSize))
        return false;

    if (n < 2)
        return true;

    if ((size_t)threadCount > n)
        threadCount = (int)n;

    SharedData s = {};

    s.arr = arr;
    s.n = n;
    s.networkSize = networkSize;
    s.threadCount = threadCount;

    ThreadData mainData = {};
    mainData.shared = &s;
    mainData.id = 0;
    mainData.tid = syscall(SYS_gettid);

    if (threadCount == 1)
    {
        runSort(&mainData);
        return true;
    }

    if (!initSync(&s))
        return false;

    int workers = threadCount - 1;

    pthread_t* threads =
        new (nothrow) pthread_t[workers];

    ThreadData* data =
        new (nothrow) ThreadData[workers]();

    if (threads == nullptr || data == nullptr)
    {
        cerr << "Memory allocation failed" << endl;

        delete[] threads;
        delete[] data;

        destroySync(&s);
        return false;
    }

    int created = 0;
    bool success = true;

    for (int i = 0; i < workers; i++)
    {
        data[i].shared = &s;
        data[i].id = i + 1;

        int rc = pthread_create(
            &threads[i], nullptr, worker, &data[i]
        );

        if (rc != 0)
        {
            cerr << "pthread_create: "
                 << strerror(rc) << endl;

            success = false;
            break;
        }

        created++;
    }

    startWorkers(&s, !success);

    if (success)
        runSort(&mainData);

    joinWorkers(threads, created);

    cout << "Main TID: " << mainData.tid << endl;

    for (int i = 0; i < created; i++)
    {
        cout << "Worker " << data[i].id
             << " TID: " << data[i].tid << endl;
    }

    delete[] threads;
    delete[] data;

    destroySync(&s);

    return success;
}
