#include <iostream>

#include "linux_threadpool.h"

vector<pthread_t> ThreadPool::workers = {};
queue<function<void()>> ThreadPool::tasks = queue<function<void()>>();
pthread_mutex_t ThreadPool::queue_mutex;
pthread_cond_t ThreadPool::condition;
bool ThreadPool::stop = false;
pthread_attr_t ThreadPool::attr;
sched_param ThreadPool::param;

ThreadPool::ThreadPool(size_t nThreads, int priority)
{
    int rc;
    workers.reserve(nThreads);

    rc = pthread_mutex_init(&queue_mutex, nullptr);
    if (rc != 0)
    {
        cerr << "ThreadPool::ThreadPool() - ERROR: Error initializing pthread mutex." << endl;
        exit(EXIT_FAILURE);
    }

    rc = pthread_cond_init(&condition, nullptr);
    if (rc != 0)
    {
        cerr << "ThreadPool::ThreadPool() - ERROR: Error initializing pthread condition." << endl;
        exit(EXIT_FAILURE);
    }

    rc = pthread_attr_init(&attr);
    if (rc != 0)
    {
        cerr << "ThreadPool::ThreadPool() - ERROR: Error initializing pthread attributes." << endl;
        exit(EXIT_FAILURE);
    }

    rc = pthread_attr_setschedpolicy (&attr, POLICY);
    if (rc != 0)
    {
        cerr << "ThreadPool::ThreadPool() - ERROR: Error setting scheduling policy." << endl;
        cerr << "ThreadPool::ThreadPool() - NOTE: Some scheduling policies require root permissions." << endl;
        exit(EXIT_FAILURE);
    }

#ifdef LINUX_THREADPOOL_LOG_DEBUG
    cout << "Min available priority for selected scheduling policy: " << sched_get_priority_min(POLICY) << endl;
    cout << "Max available priority for selected scheduling policy: " << sched_get_priority_max(POLICY) << endl;
#endif

    rc = pthread_attr_getschedparam(&attr, &param);
    if (rc != 0)
    {
        cerr << "ThreadPool::ThreadPool() - ERROR: Error getting scheduling parameters." << endl;
        exit(EXIT_FAILURE);
    }
    param.sched_priority = priority; 

    rc = pthread_attr_setschedparam(&attr, &param);
    if (rc != 0)
    {
        cerr << "ThreadPool::ThreadPool() - ERROR: Error setting scheduling priority." << endl;
        exit(EXIT_FAILURE);
    }

    for(size_t i = 0; i < nThreads; ++i)
    {
        rc = pthread_create(&workers[i], &attr, &ThreadPool::LoopThread, nullptr);
        if (rc != 0)
        {
            cerr << "ThreadPool::ThreadPool() - ERROR: Error creating thread." << endl;
            exit(EXIT_FAILURE);
        }
    }
}

ThreadPool::~ThreadPool()
{
    StopThreads();

    for(pthread_t &worker: workers)
    {
        pthread_join(worker, nullptr);
    }

    pthread_attr_destroy(&attr);
    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&queue_mutex);
}

void ThreadPool::StopThreads()
{
    pthread_mutex_lock(&queue_mutex);
    stop = true;
    pthread_mutex_unlock(&queue_mutex);
    pthread_cond_broadcast(&condition); // signal ALL workers
}

void* ThreadPool::LoopThread(void* args) 
{
    while(true)
    {
        function<void()> task; // create a blank task
        {
            pthread_mutex_lock(&queue_mutex);

            while (tasks.empty() && !stop) 
            {
                pthread_cond_wait(&condition, &queue_mutex); 
            }

            if (stop && tasks.empty()) 
            {
                pthread_mutex_unlock(&queue_mutex);
                pthread_exit(nullptr);
                return nullptr;
            }

            task = move(tasks.front()); // populate the blank task with a real task from the queue
            tasks.pop(); // remove the task from the queue

            pthread_mutex_unlock(&queue_mutex);
        }

        task(); // execute the task
    }
    return nullptr;
}
