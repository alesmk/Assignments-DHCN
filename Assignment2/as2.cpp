#include <iostream>
#include <stdlib.h>     
#include <vector>
#include <future>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include "utimer.hpp"

using namespace std;

template <typename T>
class DivideAndConquer {
public:
    using Task = vector<T>;
    using IsBaseCaseFunc = function<bool(Task&)>;
    using BaseCaseFunc = function<Task(Task&)>;
    using DivideFunc = function<vector<Task>(Task&)>;
    using ConquerFunc = function<Task(vector<Task>&)>;

    DivideAndConquer(IsBaseCaseFunc isBaseCase, BaseCaseFunc baseCase, DivideFunc divide, ConquerFunc conquer)
        : isBaseCase(isBaseCase), baseCase(baseCase), divide(divide), conquer(conquer), stop(false) {

        // Initialize a thread pool to manage the maximum number of threads 
        // and improve load balancing (used with async)
        size_t num_threads = thread::hardware_concurrency();
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back(&DivideAndConquer::worker, this);
        }
    }

    ~DivideAndConquer() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        cond.notify_all();
        for (thread& worker : workers) {
                worker.join();
        }
    }

    Task computeSequential(Task& task) {
        if (isBaseCase(task)) return baseCase(task);

        vector<Task> subtasks = divide(task);
        vector<Task> subresults(subtasks.size());

        for (size_t i = 0; i < subtasks.size(); i++) {
            subresults[i] = computeSequential(subtasks[i]);
        }

        return conquer(subresults);
    }

    Task computeParallel(Task task, int depth = 0) {
        if (isBaseCase(task)) return baseCase(task);

        vector<Task> subtasks = divide(task);
        vector<future<Task>> futures(subtasks.size());
        vector<Task> subresults(subtasks.size());

        for (size_t i = 0; i < subtasks.size(); i++) {
            if (depth < maxDepth) {
                futures[i] = enqueueTask([this, task = subtasks[i], depth]() {
                    return computeParallel(task, depth + 1);
                });}
            else {
                subresults[i] = computeSequential(subtasks[i]);
            }
        }

        for (size_t i = 0; i < futures.size(); i++) {
            if (futures[i].valid()) {
                subresults[i] = futures[i].get();
            }
        }

        return conquer(subresults);
    }

    Task computeParallelThreads(Task task, int depth=0){
        if (isBaseCase(task)) return baseCase(task);

        vector<Task> subtasks = divide(task);
        vector<thread> threads(subtasks.size());
        vector<Task> subresults(subtasks.size());
        mutex mut;

        for (size_t i = 0; i < subtasks.size(); i++) {
            if (depth < maxDepth) {
                threads[i] = thread([this, &subresults, &mut, i, task=subtasks[i], depth] (){
                    Task result = computeParallel(task, depth + 1);
                    lock_guard<mutex> lock(mut);
                    subresults[i] = result;
                }); } 
            else {
                subresults[i] = computeSequential(subtasks[i]);
            }
        }

        for (thread& th : threads) {
                th.join();
        }

        return conquer(subresults);
    }

private:
    IsBaseCaseFunc isBaseCase;
    BaseCaseFunc baseCase;
    DivideFunc divide;
    ConquerFunc conquer;

    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex queue_mutex;
    condition_variable cond;
    bool stop;
    const int maxDepth = 6; // Maximum depth for parallel recursion control

    future<vector<int>> enqueueTask(function<vector<int>()> f) {

        auto task = make_shared<packaged_task<vector<int>()>>(f);
        future<vector<int>> res = task->get_future();

        {
            unique_lock<mutex> lock(queue_mutex);
            tasks.emplace([task]() { (*task)(); });
        }
        cond.notify_one();

        return res; // Return future for result
    }

    // Worker function for thread pool to continuously process tasks
    void worker() {
        while (true) {
            function<void()> task;
            {
                unique_lock<mutex> lock(queue_mutex);
                if (!cond.wait_for(lock, chrono::milliseconds(10), [this]() { 
                        return stop || !tasks.empty(); 
                    })) {
                    continue; // Check stop or if tasks are available
                }

                if (stop && tasks.empty()) return;

                task = move(tasks.front());
                tasks.pop();
            }

            if (task) {
                task();
            }
        }
    }
};

// This implementation is based on mergesort for testing purposes
int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " n" << endl;
        exit(EXIT_FAILURE);
    }

    vector<int> task;
    for (int i = 0; i < atoi(argv[1]); i++) {
        task.push_back(rand() % 100 + 1);
    }

    auto isBaseCase = [](vector<int>& t) {
        return t.size() <= 1;
    };

    auto baseCase = [](vector<int>& t) {
        return t;
    };

    auto divide = [](vector<int>& t) {
        size_t median = t.size() / 2;
        vector<vector<int>> subtasks;
        subtasks.push_back(vector<int>(t.begin(), t.begin() + median));
        subtasks.push_back(vector<int>(t.begin() + median, t.end()));
        return subtasks;
    };

    auto conquer = [](vector<vector<int>>& subresults) {
        vector<int> left = subresults[0];
        vector<int> right = subresults[1];
        vector<int> result;

        int i = 0, j = 0;
        while (i < left.size() && j < right.size()) {
            if (left[i] >= right[j]) {
                result.push_back(right[j++]);
            } else {
                result.push_back(left[i++]);
            }
        }
        while (i < left.size()) result.push_back(left[i++]);
        while (j < right.size()) result.push_back(right[j++]);

        return result;
    };

    DivideAndConquer<int> DAndC(isBaseCase, baseCase, divide, conquer);
    long t_par = 0, t_seq = 0, t_th = 0;

    {
        utimer sequential_timer("Sequential execution time", &t_seq);
        vector<int> sorted_sequence = DAndC.computeSequential(task);
    }

    {
        utimer parallel_timer("Parallel execution time async", &t_par);
        vector<int> sorted_sequence = DAndC.computeParallel(task);
    }

    {
        utimer parallel_timer("Parallel execution time threads", &t_th);
        vector<int> sorted_sequence = DAndC.computeParallelThreads(task);
    }

    cout << "[TSEQ] " << t_seq << " | [TPAR] " << t_par << " | [TTH] " << t_th << endl;
    double speedup_async = (double)(t_seq) / t_par;
    cout << "Speedup (async): " << speedup_async << endl;
    double speedup_threads = (double)(t_seq) / t_th;
    cout << "Speedup (threads): " << speedup_threads << endl;

    return 0;
}

// To achieve a speedup close to 1 we typically need
// at least 2200 elements, both with async and thread-based approaches
// (the latter being faster in most tests).

/* 
Test 1: n = 500
[TSEQ] 1485 | [TPAR] 7303 | [TTH] 4634
Speedup (async): 0.203341
Speedup (threads): 0.320457

Test 2: n = 2100
[TSEQ] 6367 | [TPAR] 6549 | [TTH] 4832
Speedup (async): 0.972209
Speedup (threads): 1.31767

Test 3: n = 2500
[TSEQ] 7693 | [TPAR] 7361 | [TTH] 5360
Speedup (async): 1.0451
Speedup (threads): 1.43526

Test 4: n = 3500
[TSEQ] 10654 | [TPAR] 9446 | [TTH] 4638
Speedup (async): 1.12788
Speedup (threads): 2.29711

Test 5: n = 5000
[TSEQ] 15469 | [TPAR] 8747 | [TTH] 6261
Speedup (async): 1.76849
Speedup (threads): 2.47069
*/