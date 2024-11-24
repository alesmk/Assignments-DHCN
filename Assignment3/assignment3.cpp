#include <iostream>
#include <stdlib.h>
#include <vector>
#include <omp.h>
#include <math.h>
using namespace std;

const int N = 500;
const int NUM_ITER = 1000;
const float EPSILON = 1e-3;
const int NUM_THREADS = omp_get_max_threads();


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

int main(){
    // Init 
    vector<vector<float>> m(N, vector<float>(N, 0.0f));
    srand(static_cast<unsigned>(time(0)));
    omp_set_num_threads(NUM_THREADS);

    for(int i=0; i<N; i++){
        for(int j=0; j<N; j++){
            m[i][j] = ((j == 0) ? 0 : (i / static_cast<float>(j)) * 1000) + i + j;  
        }
    }
    //print_matrix(m);

    // Stencil computation
    vector<vector<float>> tmp = m;
    float max_err, tmp_err; 
    int it = 0;

    do{
        max_err = 0.0f;

        #pragma omp parallel for reduction(max:max_err) schedule(static)
        for(int i=0; i<N; i++){
            int i_up = (i - 1 + N) % N;
            int i_down = (i + 1) % N;
            
            for(int j=0; j<N; j++){            
                int j_left = (j - 1 + N) % N;
                int j_right = (j + 1) % N;

                float sum = 
                            m[i_up][j_left]   + m[i_up][j]   + m[i_up][j_right]   +
                            m[i][j_left]      + m[i][j]      + m[i][j_right]      +
                            m[i_down][j_left] + m[i_down][j] + m[i_down][j_right];
                
                tmp[i][j] = sum / 9.0f;

                // Compute max error
                tmp_err = fabs(m[i][j] - tmp[i][j]);
                max_err = fmax(max_err, tmp_err);          
            }
        }

        m.swap(tmp);
    } while(max_err > EPSILON && ++it < NUM_ITER);

    //print_matrix(m, true);
    cout << "Number of iterations completed: " << it << endl;
    cout << "Final max error: " << max_err << endl;

    return 0;
}