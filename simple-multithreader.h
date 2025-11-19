#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>

#include <time.h>

int user_main(int argc, char **argv);

union thread_arguments {
    struct {
      int low;
      int high;
      std::function<void(int)>* func;
    } a;

    struct {
      int low1, high1;
      int low2, high2;
      std::function<void(int, int)>* lambda;
    } b; 
};


enum arg_type_t {
    ARG_TYPE_1D,
    ARG_TYPE_2D
};

struct generic_thread_args {
    arg_type_t type;
    thread_arguments args_union;
};


void* generic_thread_routine(void* arg) {
    generic_thread_args* g_args = static_cast<generic_thread_args*>(arg);
    
    if (!g_args) {
        std::cerr << "[ERROR] generic_thread_routine received NULL arguments." << std::endl;
        return (void*)-1;
    }

    try {
        if (g_args->type == ARG_TYPE_1D) {
            thread_arguments* u_args = &g_args->args_union;
            for (int i = u_args->a.low; i < u_args->a.high; i++) {
                (*(u_args->a.func))(i);
            }

        } else if (g_args->type == ARG_TYPE_2D) {
            thread_arguments* u_args = &g_args->args_union;
            for (int i = u_args->b.low1; i < u_args->b.high1; i++) {
                for (int j = u_args->b.low2; j < u_args->b.high2; j++) {
                    (*(u_args->b.lambda))(i, j);
                }
            }

        } else {
            std::cerr << "[ERROR] unknown thread argument type in generic_thread_routine." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] exception in thread: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] unknown exception in thread." << std::endl;
    }

    delete g_args;
    return nullptr;
}


void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads) {
    clock_t c_start, c_end;
    time_t start, end;
    int range = high - low;

    if (range <= 0) {
        std::cout << "range has been given <=0, exiting thread creation" << std::endl;
        exit(-1);
    }
    if (numThreads <= 0) {
        std::cout << "number of threads <=0, exiting thread creation" << std::endl;
        exit(-1);
    }


    if (numThreads > range) {
        numThreads = range;
    }

    if (numThreads == 1) {
        c_start = clock();
        start = time(NULL);
        for (int i = low; i < high; i++) {
            lambda(i);
        }
        end = time(NULL);
        c_end = clock();
    } 

    else {
        pthread_t* threads = new pthread_t[numThreads];

        int chunk_size = range / numThreads; // Divide work among *all* threads
        int remaining = range % numThreads;
        int iteration_low = low;
        
        start = time(NULL);
        c_start = clock();
        for (int t = 0; t < numThreads; t++) {
            int thread_low = iteration_low;
            int thread_high = iteration_low + chunk_size + (t < remaining ? 1 : 0);
            iteration_low = thread_high;

            generic_thread_args* g_args = new generic_thread_args;
            g_args->type = ARG_TYPE_1D;
            g_args->args_union.a = {thread_low, thread_high, &lambda};
            
            int rc = pthread_create(&threads[t], nullptr, generic_thread_routine, g_args);
            if (rc) {
                std::cerr << "[ERROR] pthread_create failed with code " << rc << std::endl;
                exit(-1);
            }
        }
        

        for (int t = 0; t < numThreads; t++) {
            int rc = pthread_join(threads[t], nullptr);
            if (rc) {
                std::cerr << "[ERROR] pthread_join failed with code " << rc << std::endl;
                exit(-1);
            }
        }
        delete[] threads;
        end = time(NULL);
        c_end = clock();
    }
    std::cout << "ruunig threads count=" << numThreads << " took: "
        << (double)(end - start) << "s." << " cputime: " 
        << (double)(c_end - c_start)/CLOCKS_PER_SEC << std::endl;
}

void parallel_for(int low1, int high1, int low2, int high2, std::function<void(int, int)> &&lambda, int numThreads){
    clock_t c_start, c_end;
    time_t start, end;
    int range1 = high1 - low1; // We only parallelize the outer loop

    if (range1 <= 0) {
        std::cout << "outer loop range has been given <=0, exiting" << std::endl;
        return;
    }
    if (numThreads <= 0) {
        std::cout << "number of threads <=0, exiting thread creation" << std::endl;
        exit(-1);
    }
    if (numThreads > range1) {
        numThreads = range1;
    }

    if (numThreads == 1) {
            c_start = clock();
            start = time(NULL);
            for (int i = low1; i < high1; i++) {
                for (int j = low2; j < high2; j++) {
                    lambda(i, j);
                }
            }
            end = time(NULL);
            c_end = clock();
    } 
    else {
        pthread_t* threads = new pthread_t[numThreads];

        int chunk_size = range1 / numThreads;
        int remaining = range1 % numThreads;
        int iteration_low1 = low1;
        
        start = time(NULL);
        c_start = clock();
        for (int t = 0; t < numThreads; t++) {
            int thread_low1 = iteration_low1;
            int thread_high1 = iteration_low1 + chunk_size + (t < remaining ? 1 : 0);
            iteration_low1 = thread_high1;

            generic_thread_args* g_args = new generic_thread_args;
            g_args->type = ARG_TYPE_2D;
            g_args->args_union.b = {thread_low1, thread_high1, low2, high2, &lambda};
        
            int rc = pthread_create(&threads[t], nullptr, generic_thread_routine, g_args);
            if (rc) {
                std::cerr << "[ERROR] pthread_create failed with code " << rc << std::endl;
                exit(-1);
            }
        
        }
    
    for (int t = 0; t < numThreads; t++) {
        int rc = pthread_join(threads[t], nullptr);
        if (rc) {
            std::cerr << "[ERROR] pthread_join failed with code " << rc << std::endl;
            exit(-1);
        }
    }
    delete[] threads;
    end = time(NULL);
    c_end = clock();
  }

  std::cout << "ruunig threads count=" << numThreads << " took: "
    << (double)(end - start) << "s." << " cputime: " 
    << (double)(c_end - c_start)/CLOCKS_PER_SEC << std::endl;
}

int main(int argc, char **argv) {
  int rc = user_main(argc, argv);
  return rc;
}

#define main user_main



