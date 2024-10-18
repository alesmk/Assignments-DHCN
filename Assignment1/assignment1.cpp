#include <iostream>
#include <vector>
#include <stdlib.h>     
#include <time.h>  
#include <math.h>     
#include <thread>

#include "utimer.hpp" 

using namespace std;

/*
 * Using ONLY C++ threads, write a simple program that:
 * 
 * 1. Initializes a vector of n items with random numbers.
 * 2. Defines a function f computing k times the sin() on the input parameter 
 *    (e.g., return sin(sin(sin(...sin(x))))).
 * 3. Implements a map(f) over the vector.
 * 4. Measures the speedup, varying k, n, and the parallelism degree nw 
 *    (all of them passed on the command line).
 */

void init_vector(vector<float> &v, int n){

    for(int i=0; i<n; i++){
        //Generate a random number between 0 and 2π
        float r = static_cast<float>(rand()) / RAND_MAX * 2 * M_PI;
        v.push_back(r);
    }

    if (v.size() != n) {
        cerr << "ERROR: vector size != n";
        exit(EXIT_FAILURE);
    }
}

void print_vector(vector<float> v){
    for(int i=0; i<v.size(); i++){
        cout << v[i] << " ";
    }
    cout << endl;
}

float f(float x, int k){
    for(int i=0; i<k; i++){
        x = sin(x);
    }

    return x;
}

void body_t(const vector<float> &v, vector<float> &res, int s, int e, int k){
    for(int i=s; i<e; i++){
        res[i] = f(v[i], k);
    }
}

int main(int argc, char* argv[]){

    if(argc != 4){
        cerr << "Usage: " << argv[0] << " k n nw" << endl;
        exit(EXIT_FAILURE);
    }

    int k = atoi(argv[1]); // Number of times to apply sin
    int n = atoi(argv[2]); // Number of items in the vector
    int nw = atoi(argv[3]); // Number of workers

    if(k < 1 || n < 1 || nw < 1){
        cerr << "k n nw must be greater than 0" << endl;
        exit(EXIT_FAILURE);        
    }

    vector<float> v;
    vector<float> res(n);

    srand (time(NULL));
    init_vector(v, n);
    //cout << "Generated vector: ";
    //print_vector(v);

    long t_par = 0, t_seq = 0;
    
    // Parallel execution
    {  
        utimer parallel_timer("Parallel execution time", &t_par); 
        vector<thread> t;
        int start, end;
        for(int i=0; i<nw; i++) { 
            start = i * (v.size() / nw);
            end = (i == nw - 1) ? v.size() : (i+1) * (v.size()/nw);
            t.push_back(thread([&v, &res, start, end, k](){
                body_t(v, res, start, end, k);
            }));
        }

        for(int i=0; i<nw; i++) { 
            t[i].join();
        }
    }

    // Sequential execution
    {
        utimer sequential_timer("Sequential execution time", &t_seq);  
        body_t(v, res, 0, v.size(), k);  
    }

    // Compute speedup
    float speedup = static_cast<float>(t_seq) / t_par;
    cout << "[TSEQ] " << t_seq << " | [TPAR] " << t_par << endl;
    cout << "Speedup: " << speedup << endl;
    
    return 0;    
 }