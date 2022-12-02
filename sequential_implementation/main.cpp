#include "graph.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>

int main(int argc, char **argv) {
    // check for filename passed
    if (argc < 2) {
        printf("Usage: ./sequentialColor <graph_file_name.mtx>\n");
        return 1;
    }

    // read filename and create graph object
    std::string filename = argv[1];
    std::time_t timeBeforeFile = std::time(nullptr);
    CompressedGraph* g = read_matrix_file_and_convert(filename);
    std::time_t timeAfterFile = std::time(nullptr);
    std::cout << "Reading matrix file took " << timeAfterFile - timeBeforeFile << " seconds" << std::endl;

    // perform algorithm
    std::unordered_map<int, std::vector<int>* > scc;
    // we arbitrarily assign negative color values to single components, to distinguish from the ids
    std::vector<int>* singleConnectedComponents = g->trimSingleConnectedComponents();
    int counter = -1;
    for(int i = 0; i < (*singleConnectedComponents).size(); i++) {
        std::vector<int>* res = new std::vector<int>;
        int id = (*singleConnectedComponents)[i];
        (*res).push_back(id);
        scc[counter--] = res;
    }

    delete singleConnectedComponents;

    while (!g->isEmpty()) {
        std::unordered_map<int, int> colors;

        std::vector<int>* activeNodes = g->getActiveNodes();
        for(int i = 0; i < (*activeNodes).size(); i++) {
            int vid = (*activeNodes)[i];
            colors[vid] = vid;
        }
        delete activeNodes;

        bool change = true;
        while (change) {
            change = false;
            std::vector<int>* currentActiveNodes = g->getActiveNodes();
            for(int i = 0; i < (*currentActiveNodes).size(); i++) {
                int v = (*currentActiveNodes)[i];
                std::vector<int>* pointedValues = g->pointedFrom(v);
                for(int j = 0; j < (*pointedValues).size(); j++) {
                    int u = (*pointedValues)[j];
                    if (colors[v] > colors[u]) {
                        colors[v] = colors[u];
                        change = true;
                    }
                }
                delete pointedValues;
            }
            delete currentActiveNodes;
        }


        std::vector<int> uniqueColors;
        for (const std::pair<const int, int> &keyValuePair : colors) {
            int color = keyValuePair.second;
            if (std::find(uniqueColors.begin(), uniqueColors.end(), color) == uniqueColors.end()) {
                uniqueColors.push_back(color);
            }
        }

        for(int i = 0; i < uniqueColors.size(); i++) {
            int c = uniqueColors[i];
            int cv = c;
            scc[cv] = g->pred(cv, c, &colors);
            g->removeComponent(scc[cv]);
        }
    }

    delete g;

    std::time_t timeAfterEnd = std::time(nullptr);
    std::cout << "Completing the algorithm took " << timeAfterEnd - timeAfterFile << " seconds" << std::endl;

    // print out results
    // TODO: might want to print out the SCC in parseable format
    // to generate graphs via python script from output
    // for now the number of SCC is enough
    std::cout << "In total there are " << scc.size() << " SCC" << std::endl;

    for (const std::pair<const int, std::vector<int>*> &sccKeyValuePair : scc) {
        delete sccKeyValuePair.second;
    }

    return 0;
}