This project implements a four-stage pipeline to process  N * N  matrices using FastFlow and OpenMP. Each file corresponds to a specific approach to implement the pipeline:
	1.	assignment4_ffseq.cpp: implements the pipeline sequentially, processing matrices one step at a time without parallelism.
	2.	assignment4_ff.cpp: uses FastFlow’s ParallelFor construct for matrix operations in stages 2 and 3.
	3.	assignment4_ffomp.cpp: combines FastFlow with OpenMP, applying OpenMP directives in stages 2 and 3 to optimize performance.

Compilation & Execution:
	1.	assignment4_ffseq.cpp:	g++ -I./fastflow -O3 -o main_ffseq assignment4_ffseq.cpp && ./main_ffseq
	2.	assignment4_ff.cpp:     g++ -I./fastflow -O3 -o main_ff assignment4_ff.cpp && ./main_ff
	3.	assignment4_ffomp.cpp:  g++ -I./fastflow -O3 -fopenmp -o main_ffomp assignment4_ffomp.cpp && ./main_ffomp


Experiments:
- With N=40 and processing 100 matrices, execution using FastFlow's parallelFor takes an average of 263.5 ms. If OpenMP is used instead of parallelFor, the execution time averages 337.7 ms.

- With N=1000 and processing 2500 matrices, execution using FastFlow's parallelFor takes an average of 40.5 sec. If OpenMP is used instead of parallelFor, the execution time averages 46 sec.

Using the -DNO_DEFAULT_MAPPING flag does not negatively impact the performance of my code with openMP. However, it increased the execution time when using parallelFor: the execution slows down during the final stage, taking approximately 1 minute overall.
