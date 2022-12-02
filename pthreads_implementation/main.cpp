#include "graph.hpp"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <pthread.h>
#include <string>
#include <unordered_map>
#include <vector>

typedef struct {
    std::vector<int> *currentActiveNodes;
    CompressedGraph *g;
    std::unordered_map<int, int> *colors;
    bool *change;
    int start;
    int end;
} parallel_coloring_args;

#define NUM_THREADS 8
// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *parallel_coloring(void *args) {
    parallel_coloring_args *a = (parallel_coloring_args *)args;
    CompressedGraph *g = a->g;
    std::vector<int> *currentActiveNodes = a->currentActiveNodes;
    std::unordered_map<int, int> *colors = a->colors;
    bool *change = a->change;
    int start = a->start;
    int end = a->end;

    for (int i = start; i < end; i++) {
        int v = (*currentActiveNodes)[i];
        std::vector<int> *pointedValues = g->pointedFrom(v);
        for (int j = 0; j < (*pointedValues).size(); j++) {
            int u = (*pointedValues)[j];
            // pthread_mutex_lock(&lock);
            if ((*colors)[v] > (*colors)[u]) {
                (*colors)[v] = (*colors)[u];
                *change = true;
            }
            // pthread_mutex_unlock(&lock);
        }
        delete pointedValues;
    }
}

int main(int argc, char **argv) {
    // check for filename passed
    if (argc < 2) {
        printf("Usage: ./pthreadsColor <graph_file_name.mtx>\n");
        return 1;
    }

    // read filename and create graph object
    std::string filename = argv[1];
    std::time_t timeBeforeFile = std::time(nullptr);
    CompressedGraph *g = read_matrix_file_and_convert(filename);
    std::time_t timeAfterFile = std::time(nullptr);
    std::cout << "Reading matrix file took " << timeAfterFile - timeBeforeFile << " seconds" << std::endl;

    // perform algorithm
    std::unordered_map<int, std::vector<int> *> scc;
    // we arbitrarily assign negative color values to single components, to distinguish from the ids
    std::vector<int> *singleConnectedComponents = g->trimSingleConnectedComponents();
    int counter = -1;
    for (int i = 0; i < (*singleConnectedComponents).size(); i++) {
        std::vector<int> *res = new std::vector<int>;
        int id = (*singleConnectedComponents)[i];
        (*res).push_back(id);
        scc[counter--] = res;
    }

    delete singleConnectedComponents;

    while (!g->isEmpty()) {
        std::unordered_map<int, int> *colors = new std::unordered_map<int, int>;

        std::vector<int> *activeNodes = g->getActiveNodes();
        for (int i = 0; i < (*activeNodes).size(); i++) {
            int vid = (*activeNodes)[i];
            (*colors)[vid] = vid;
        }
        delete activeNodes;

        bool change = true;
        while (change) {
            change = false;
            std::vector<int> *currentActiveNodes = g->getActiveNodes();
            int activeThreads = NUM_THREADS;
            // if ((*currentActiveNodes).size() < NUM_THREADS) {
            //     activeThreads = 1;
            // }

            pthread_t threads[NUM_THREADS];
            parallel_coloring_args p[NUM_THREADS];

            // spawn the threads with the correct arguments
            for (int i = 0; i < activeThreads; i++) {
                // setup args
                p[i].currentActiveNodes = currentActiveNodes;
                p[i].g = g;
                p[i].colors = colors;
                p[i].change = &change;
                int step = (*currentActiveNodes).size() / activeThreads;
                p[i].start = i * step;
                p[i].end = (i + 1) * step;
                if (i == 0) {
                    p[i].start = 0;
                }
                if (i == activeThreads - 1) {
                    p[i].end = (*currentActiveNodes).size();
                }

                // spawn the thread
                void *args = (void *)&p[i];

                if(pthread_create(&threads[i], NULL, parallel_coloring, args) != 0) {
                    std::cout << "Error while creating thread " << i << std::endl;
                }
            }

            // join the threads
            for (int i = 0; i < activeThreads; i++) {
                if(pthread_join(threads[i], NULL) != 0){
                    std::cout << "Error while joining thread " << i << std::endl;
                }
            }

            delete currentActiveNodes;
        }

        std::vector<int> uniqueColors;
        for (const std::pair<const int, int> &keyValuePair : *colors) {
            int color = keyValuePair.second;
            if (std::find(uniqueColors.begin(), uniqueColors.end(), color) == uniqueColors.end()) {
                uniqueColors.push_back(color);
            }
        }

        for (int i = 0; i < uniqueColors.size(); i++) {
            int c = uniqueColors[i];
            int cv = c;
            scc[cv] = g->pred(cv, c, colors);
            g->removeComponent(scc[cv]);
        }
        delete colors;
    }

    delete g;

    std::time_t timeAfterEnd = std::time(nullptr);
    std::cout << "Completing the algorithm took " << timeAfterEnd - timeAfterFile << " seconds" << std::endl;

    // print out results
    // TODO: might want to print out the SCC in parseable format
    // to generate graphs via python script from output
    // for now the number of SCC is enough
    std::cout << "In total there are " << scc.size() << " SCC" << std::endl;

    for (const std::pair<const int, std::vector<int> *> &sccKeyValuePair : scc) {
        delete sccKeyValuePair.second;
    }

    return 0;
}