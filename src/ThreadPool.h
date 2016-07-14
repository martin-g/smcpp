#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

template <typename T>
class Singleton
{
    protected:
        Singleton() = default;
        ~Singleton() = default;
    public:
        Singleton(const Singleton&) = delete; // no copies
        Singleton& operator=(const Singleton&) = delete; // no self-assignments
        Singleton(Singleton&&) = delete; // WHY?
        Singleton& operator=(Singleton&&) = delete; // WHY?
        static T& getInstance() // singleton
        {
            static T instance; // Guaranteed to be destroyed.
            // Instantiated on first use.
            // Thread safe in C++11
            return instance;
        }
};

class ThreadPool : public Singleton<ThreadPool>
{
    friend class Singleton<ThreadPool>;
    public:
        ThreadPool(size_t);
        ThreadPool() : ThreadPool(std::thread::hardware_concurrency() or 8) {}
        ~ThreadPool();
        template<class F, class... Args>
            auto enqueue(F&& f, Args&&... args)
            -> std::future<typename std::result_of<F(Args...)>::type>;
    private:
        // need to keep track of threads so we can join them
        std::vector< std::thread > workers;
        // the task queue
        std::queue< std::function<void()> > tasks;

        // synchronization
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
};

// the constructor just launches some amount of workers
    inline ThreadPool::ThreadPool(size_t threads)
:   stop(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
                [this]
                {
                for(;;)
                {
                std::function<void()> task;

                {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                this->condition.wait(lock,
                    [this]{ return this->stop || !this->tasks.empty(); });
                if(this->stop && this->tasks.empty())
                return;
                task = std::move(this->tasks.front());
                this->tasks.pop();
                }

                task();
                }
                }
                );
}

// add new work item to the pool
    template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

#endif
