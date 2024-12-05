// g++ -I./fastflow -O3 -fopenmp -o main_ffomp assignment4_ffomp.cpp && ./main_ffomp
// g++ -DNO_DEFAULT_MAPPING -I./fastflow -O3 -fopenmp -o main_ffomp_nodefm assignment4_ffomp.cpp && ./main_ffomp_nodefm

#include <iostream>
#include <stdlib.h>
#include <vector>
#include <omp.h>
using namespace std;

#include <ff/ff.hpp>
#include <ff/pipeline.hpp>
#include <ff/node.hpp>
using namespace ff;

using Matrix = vector<vector<float>>;

/*
1 - generates a stream of NxN matrices of floats
2 - changes each element in a matrix with its square
3 - implements 2 iterations of assignment three on each matrix
4 - just prints the first items of the resulting matrix
*/

const int N = 40;
const int NUM_ITER = 2;
const int NUM_MAT = 100;

// Node Stage 1: generate a stream ( -- NUM_MAT -- ) NxN matrix
struct Stage1 : ff_node_t<Matrix> {
    
    Matrix* svc(Matrix*) {
        
        for(int nm=0; nm<NUM_MAT; nm++){
            Matrix* m = new Matrix(N, vector<float>(N, 0.0f));
            
            for(int i=0; i<N; i++){
                for(int j=0; j<N; j++){
                    // identical matrices
                    (*m)[i][j] = ((j == 0) ? 0 : (i / static_cast<float>(j)) * 1000) + i + j;  
                }
            }
            ff_send_out(m);
        }
        
        return EOS;
    }
};

// Node Stage 2
struct Stage2 : ff_node_t<Matrix> {
    Matrix* svc(Matrix* m){
    
        #pragma omp parallel for collapse(2) schedule(static)
        for(int i=0; i<N; i++){            
            for(int j=0; j<N; j++){
                (*m)[i][j] *= (*m)[i][j];
            }
        }
        
        return m;

    }
};

// Node Stage 3 
struct Stage3 : ff_node_t<Matrix> {
    Matrix* svc(Matrix* m){

        Matrix tmp = *m;
        int it = 0;

        do{
            #pragma omp parallel for collapse(2) schedule(static) num_threads(8)
            for(int i=0; i<N; i++){            
                for(int j=0; j<N; j++){       
                    int i_up = (i - 1 + N) % N;
                    int i_down = (i + 1) % N;
                     
                    int j_left = (j - 1 + N) % N;
                    int j_right = (j + 1) % N;

                    float sum = 
                                (*m)[i_up][j_left]   + (*m)[i_up][j]   + (*m)[i_up][j_right]   +
                                (*m)[i][j_left]      + (*m)[i][j]      + (*m)[i][j_right]      +
                                (*m)[i_down][j_left] + (*m)[i_down][j] + (*m)[i_down][j_right];
                    
                    tmp[i][j] = sum / 9.0f; 
                }
            }

            m->swap(tmp);
        } while(++it < NUM_ITER); 

        return m;

    }
};

// Node Stage 4: print results
struct Stage4 : ff_node_t<Matrix> {
    Matrix* svc(Matrix* m){
        for(int i=0; i<N; i++){
            printf("%.2f\n", (*m)[i][0]);
        }
        printf("\n");

        delete m;
        return GO_ON;
    }
    
};


int main(){
   
    ff_Pipe<> pipe(new Stage1, new Stage2, new Stage3, new Stage4);

    if(pipe.run_and_wait_end() < 0){
        cerr << "Error running pipeline" << endl;
        return -1;
    }
   
    return 0;
}