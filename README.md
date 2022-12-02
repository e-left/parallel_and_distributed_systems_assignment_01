# Parallel and Distributed Systems, Assignment 01

By: Αλεξανδρίδης Φώτιος, ΑΕΜ: 9953

Requirements:
- g++/clang++
- OpenCilk 
- OpenMP

### How to run

Configure the compilers for OpenMP and OpenCilk on the first lines of the `Makefile`

Run `make sequential|openmp|opencilk|pthreads` to generate the executables in the folder `out`

Run `out/sequentialColor|out/openmpColor|out/opencilkColor|out/pthreadsColor <graph_file.mtx>` to run the specified implementation with the provided matrix file. 
