CXX_FLAGS = --std=c++11 -O3
CXX_OPENMP=/opt/homebrew/opt/llvm/bin/clang++
CXX_OPENCILK=xcrun /Users/eleft/Documents/school/parallel_and_distributed_systems/opencilk/bin/clang++

sequential: $(wildcard sequential_implementation/*.cpp)
	g++ $(CXX_FLAGS) $(wildcard sequential_implementation/*.cpp) -o out/sequentialColor	

pthreads: $(wildcard pthreads_implementation/*.cpp)
	g++ $(CXX_FLAGS) $(wildcard pthreads_implementation/*.cpp) -pthread -o out/pthreadsColor	

opencilk: $(wildcard opencilk_implementation/*.cpp)
	$(CXX_OPENCILK) $(CXX_FLAGS) $(wildcard opencilk_implementation/*.cpp) -fopencilk -o out/opencilkColor	

openmp: $(wildcard openmp_implementation/*.cpp)
	$(CXX_OPENMP) $(CXX_FLAGS) $(wildcard openmp_implementation/*.cpp) -Xpreprocesssor -fopenmp -lomp -I/usr/local/opt/libomp/include -o out/openmpColor
