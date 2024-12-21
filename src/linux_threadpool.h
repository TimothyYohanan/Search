// Credits:
// https://github.com/progschj/ThreadPool/blob/master/ThreadPool.h
// https://code-vault.net/lesson/j62v2novkv:1609958966824
// https://man7.org/linux/man-pages/man2/sched_setscheduler.2.html

// #include <thread>
#include <pthread.h> // use pthread instead of thread on linux to enable setting thread priority.
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

#define POLICY SCHED_FIFO // Set a scheduling policy at compile time. (You can set this at runtime instead. I just chose to do it this way.)
// #define LINUX_THREADPOOL_LOG_DEBUG

using namespace std;

class ThreadPool {
public:
    ThreadPool(size_t nThreads, int priority);

    template<class F, class... Args>
    inline auto Do(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type>;

    ~ThreadPool();

    void StopThreads();

protected:
    static vector<pthread_t> workers;
    static queue<function<void()>> tasks;
    static pthread_mutex_t queue_mutex;
    static pthread_cond_t condition;
    static bool stop;

    static void* LoopThread(void* args);
    static pthread_attr_t attr;
    static sched_param param;
};

template<class F, class... Args>
inline auto ThreadPool::Do(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type>
{
    using return_type = typename result_of<F(Args...)>::type;

    auto task = make_shared<packaged_task<return_type()>>(bind(forward<F>(f), forward<Args>(args)...));
        
    future<return_type> res = task->get_future();
    {
        pthread_mutex_lock(&queue_mutex);

        if(stop)
        {
            cerr << "ThreadPool::Do() - ERROR: Called Do() after the thread pool was stopped!" << endl;
            exit(EXIT_FAILURE);
        }

        tasks.emplace([task](){ (*task)(); }); // add a task to the queue
    }
    pthread_mutex_unlock(&queue_mutex); // unlock the mutex, which 'wakes up' all sleeping threads, but traps them inside the 'while (tasks.empty() && !stop)' loop until the mutex is locked again.
    pthread_cond_signal(&condition); // signal ONE worker, which continues the execution loop in ThreadPool::startThread() from the line 'pthread_cond_wait(&condition, &queue_mutex) due to tasks no longer being empty.
    return res;
}
