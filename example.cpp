#include <iostream>
#include <vector>
#include <chrono>

// #include "ThreadPool.h"
#include "ThreadPoolMe.h"

/* prototype
int main()
{
    
    ThreadPool pool(4);
    std::vector< std::future<int> > results;

    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue([i] {
                std::cout << "hello " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "world " << i << std::endl;
                return i*i;
            })
        );
    }

    for(auto && result: results)
        std::cout << result.get() << ' ';
    std::cout << std::endl;
    
    return 0;
}
*/

// my try
int DoTask(int thread_id) {
    int i = 0;
    for (; i < 5000; i++) {
        if (i % 1000 == 0) {
            std::cout << "Thread: " << thread_id << " num: " << i << std::endl;
        }
    }
    return i;
}

int main() {
    ThreadPool pool(4);
    std::vector<std::future<int>> results;

    for (int i = 0; i < 8; i++) {
        results.emplace_back(
            pool.enqueue(&DoTask, i)
        );
    }

    for (auto && result : results) {
        std::cout << "line: " << __LINE__ << " " << result.get() << ' ';
    }
    std::cout << std::endl;
    return 0;
}