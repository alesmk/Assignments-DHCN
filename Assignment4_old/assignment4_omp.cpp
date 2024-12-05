// g++ -fopenmp -o main_omp assignment4_omp.cpp && ./main_omp
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <omp.h>
using namespace std;

using Matrix = vector<vector<float>>;

/*
1 - generates a stream of NxN matrices of floats
2 - changes each element in a matrix with its square
3 - implements 2 iterations of assignment three on each matrix
4 - just prints the first items of the resulting matrix

USING OPENMP
*/

const int N = 4;
const int NUM_ITER = 2;

void print_matrix(vector<vector<float>> matrix, bool is_final=false){

    for(int i=0; i<N; i++){
        for(int j=0; j<N; j++){
            printf("%.2f\t", matrix[i][j]);
        }
        cout << endl;
    }

    if(!is_final)
        printf("------------------------------------\n");
}


Matrix stage1(int dim){
    vector<vector<float>> matrix(N, vector<float>(N, 0.0f));

    for(int i=0; i<N; i++){
        for(int j=0; j<N; j++){
            matrix[i][j] = ((j == 0) ? 0 : (i / static_cast<float>(j)) * 1000) + i + j;  
        }
    }

    return matrix;
}

void stage2(Matrix& matrix){

    #pragma omp parallel for collapse(2) schedule(static)
    for(int i=0; i<N; i++){
        for(int j=0; j<N; j++){
            matrix[i][j] *= matrix[i][j];
        }
    }

}

void stage3(Matrix& m){
    Matrix tmp = m;
    //float max_err, tmp_err; 
    int it = 0;

    do{
        //max_err = 0.0f;

        #pragma omp parallel for collapse(2) schedule(static) num_threads(8)
        for(int i=0; i<N; i++){
            
            for(int j=0; j<N; j++){    
                int i_up = (i - 1 + N) % N;
                int i_down = (i + 1) % N;    

                int j_left = (j - 1 + N) % N;
                int j_right = (j + 1) % N;

                float sum = 
                            m[i_up][j_left]   + m[i_up][j]   + m[i_up][j_right]   +
                            m[i][j_left]      + m[i][j]      + m[i][j_right]      +
                            m[i_down][j_left] + m[i_down][j] + m[i_down][j_right];
                
                tmp[i][j] = sum / 9.0f;

                // Compute max error
                //tmp_err = fabs(m[i][j] - tmp[i][j]);
                //max_err = fmax(max_err, tmp_err);          
            }
        }

        m.swap(tmp);
    } while(++it < NUM_ITER);
}

void stage4(Matrix& matrix){
    for(int i=0; i<N; i++){
        printf("%.2f\n", matrix[i][0]);
    }
}



int main(){
    Matrix m = stage1(N);
    print_matrix(m);
    stage2(m);
    print_matrix(m);
    stage3(m);
    print_matrix(m);
    cout << "Result: " << endl;
    stage4(m);    

    return 0;
}