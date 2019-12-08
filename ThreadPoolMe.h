#ifndef THREAD_POOL_ME_H
#define THREAD_POOL_ME_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
    ThreadPool(size_t);
    // 定义一个加入task的泛型函数
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        ->std::future<typename std::result_of < F(Args...) > ::type>;
    ~ThreadPool();

private:
    // 线程队列
    std::vector<std::thread> workers;
    // 任务队列
    std::queue<std::function<void()>> tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// the constructor just lauches some amount of workers
inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    // 构造多个线程, std::thread 可以用函数构造线程，所以lambda函数没问题。
    // 线程池里的线程跑线程池里的任务队列
    for (size_t i = 0; i < threads; i++) {
        workers.emplace_back(
            [this] {
                for (;;) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            }
        );
    }
}

// add new work item to the pool
// 这里的难点是如何将各种函数都封装成std::function<void()>
// 类模板 std::packaged_task 包装任何可调用 (Callable) 目标（函数、 lambda 表达式、 bind 表达式或其他函数对象），
// 使得能异步调用它。其返回值或所抛异常被存储于能通过 std::future 对象访问的共享状态中。
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {

    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task]() {(*task)(); });
    }
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers) {
        worker.join();
    }
}


#endif
