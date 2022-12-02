#include "graph.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <omp.h>

// constructor
CompressedGraph::CompressedGraph(std::vector<int>* csrRowIdx, std::vector<int>* csrCol, std::vector<int>* cscRow, std::vector<int>* cscColIdx, int activeNodes, int nnz) {
    for (int i = 0; i < activeNodes + 1; i++) {
        this->csrRowIdx.push_back((*csrRowIdx)[i]);
        this->cscColIdx.push_back((*cscColIdx)[i]);
    }

    for (int i = 0; i < nnz; i++) {
        this->csrCol.push_back((*csrCol)[i]);
        this->cscRow.push_back((*cscRow)[i]);
    }

    this->activeNodesCount = activeNodes;
    for (int i = 0; i < this->activeNodesCount; i++) {
        this->activeNodes[i] = true;
    }
}

// get a vector of all active nodes
std::vector<int>* CompressedGraph::getActiveNodes() {
    std::vector<int>* currentActiveNodes = new std::vector<int>;
    for (const std::pair<const int, bool> &activeNodePair : this->activeNodes) {
        // if active
        if (activeNodePair.second) {
            // add to currentActiveNodes
            (*currentActiveNodes).push_back(activeNodePair.first);
        }
    }
    return currentActiveNodes;
}

bool CompressedGraph::isEmpty() {
    return this->activeNodesCount == 0;
}

void CompressedGraph::removeValue(int id) {
    this->activeNodes[id] = false;
    #pragma omp critical 
    {
    this->activeNodesCount--;
    }
}

void CompressedGraph::removeComponent(std::vector<int> *ids) {
    for (int i = 0; i < (*ids).size(); i++) {
        int id = (*ids)[i];
        this->removeValue(id);
    }
}

std::vector<int>* CompressedGraph::trimSingleConnectedComponents() {
    std::vector<int>* res = new std::vector<int>;
    std::vector<int> toBeRemoved;

    do {
        // remove all elements in toBeRemoved
        for(int i = 0; i < toBeRemoved.size(); i++) {
            int id = toBeRemoved[i];
            (*res).push_back(id);
            this->removeValue(id);
        }
        toBeRemoved.clear();

        // search to see if we have to add any more elements in toBeRemoved
        std::vector<int>* currentlyActiveNodes = this->getActiveNodes();
        for(int i = 0; i < (*currentlyActiveNodes).size(); i++) {
            int currentNode = (*currentlyActiveNodes)[i];
            std::vector<int>* nodesPointedFrom = this->pointedFrom(currentNode);
            int sizeFrom = (*nodesPointedFrom).size();
            if (std::find((*nodesPointedFrom).begin(), (*nodesPointedFrom).end(), currentNode) != (*nodesPointedFrom).end()) {
                sizeFrom--;
            }
            delete nodesPointedFrom;
            std::vector<int>* nodesPointsTo = this->pointsTo(currentNode);
            int sizeTo = (*nodesPointsTo).size();
            if (std::find((*nodesPointsTo).begin(), (*nodesPointsTo).end(), currentNode) != (*nodesPointsTo).end()) {
                sizeTo--;
            }
            delete nodesPointsTo;
            if (sizeFrom == 0 || sizeTo == 0) {
                toBeRemoved.push_back(currentNode);
            }
        }
        delete currentlyActiveNodes;

    } while(toBeRemoved.size() != 0);

    return res;
}

// read the column of id, use csc
std::vector<int>* CompressedGraph::pointedFrom(int id) {
    int idColIdxLow = this->cscColIdx[id];
    int idColIdxHigh = this->cscColIdx[id + 1];

    std::vector<int>* col = new std::vector<int>;

    // #pragma omp parallel for shared(col)
    for (int i = idColIdxLow; i < idColIdxHigh; i++) {
        // keep only active nodes
        int node = this->cscRow[i];
        if (this->activeNodes[node]) {
            // #pragma omp critical
            (*col).push_back(node);
        }
    }

    return col;
}

// read the row of id, use csr
std::vector<int>* CompressedGraph::pointsTo(int id) {
    int idRowIdxLow = this->csrRowIdx[id];
    int idRowIdxHigh = this->csrRowIdx[id + 1];

    std::vector<int>* row = new std::vector<int>;

    for (int i = idRowIdxLow; i < idRowIdxHigh; i++) {
        // keep only active nodes
        int node = this->csrCol[i];
        if (this->activeNodes[node]) {
            (*row).push_back(node);
        }
    }

    return row;
}

std::vector<int>* CompressedGraph::pred(int id, int color, std::unordered_map<int, int> *colors) {
    std::set<int> scc;
    scc.insert(id);
    std::set<int> visited;

    while (scc.size() != visited.size()) {
        // find all nodes that have not been visited
        std::set<int> nonVisited;
        for (int v : scc) {
            // if v not in visited
            if (visited.count(v) == 0) {
                nonVisited.insert(v);
            }
        }

        // append all non_visited to visited since we will "visit" them
        visited.insert(nonVisited.begin(), nonVisited.end());

        // find all pointed nodes by them and filter by color
        std::set<int> newNodes;
        for (int v : nonVisited) {
            std::vector<int>* pointedNodes = this->pointedFrom(v);
            // nodes = self.pointed_from_value(v)
            std::set<int> validNodes;
            for (int node : *pointedNodes) {
                if ((*colors)[node] == color) {
                    validNodes.insert(node);
                }
            }
            delete pointedNodes;
            newNodes.insert(validNodes.begin(), validNodes.end());
        }

        // append them to scc
        if (newNodes.size() > 0) {
            scc.insert(newNodes.begin(), newNodes.end());
        }
    }

    // put all elements of scc to vector, for compatibility
    std::vector<int>* result = new std::vector<int>(scc.begin(), scc.end());

    return result;
}

CompressedGraph* read_matrix_file_and_convert(std::string filename) {
    int rows = 0;
    int cols = 0;
    int nnz = 0;

    std::vector<int> cooRowV;
    std::vector<int> cooColV;
    std::vector<int> csrRowIdxV;
    std::vector<int> csrColV;
    std::vector<int> cscRowV;
    std::vector<int> cscColIdxV;

    // read file as coo
    std::ifstream f(filename);
    std::string line;
    while (std::getline(f, line)) {
        // ignore comment lines
        if (line[0] == '%' || line[0] == ' ' || line[0] == '\n') {
            continue;
        }

        // parse non comment line. three integers
        if (rows == 0 && cols == 0 && nnz == 0) {
            // initial line
            std::istringstream splittedLine(line);
            int a;
            int b;
            int c;
            splittedLine >> a >> b >> c;
            rows = a;
            cols = b;
            nnz = c;
        } else {
            std::istringstream splittedLine(line);
            int a;
            int b;
            splittedLine >> a >> b;
            cooRowV.push_back(a - 1);
            cooColV.push_back(b - 1);
        }
    }

    std::cout << "Rows: " << rows << ", columns: " << cols << ", nnz: " << nnz << "." << std::endl;

    // first sort by column
    std::vector<std::pair<int, int> > cooSorted;
    for (int i = 0; i < nnz; i++) {
        cooSorted.push_back(std::make_pair(cooColV[i], cooRowV[i]));
    }

    // std::sort(cooSorted, cooSorted + nnz);
    std::sort(cooSorted.begin(), cooSorted.end());

    for (int i = 0; i < nnz; i++) {
        cooColV[i] = cooSorted[i].first;
        cooRowV[i] = cooSorted[i].second;
    }

    // convert from coo to csc
    for (int i = 0; i < nnz; i++) {
        cscRowV.push_back(cooRowV[i]);
    }

    std::unordered_map<int, int> cscCounter;
    for (int value : cooColV) {
        if (cscCounter.find(value) == cscCounter.end()) {
            // not found
            cscCounter[value] = 1;
        } else {
            cscCounter[value]++;
        }
    }

    for (int i = 0; i < rows; i++) {
        int start = 0;
        if (i != 0) {
            start = cscColIdxV[i - 1];
            start += cscCounter[i - 1];
        }
        cscColIdxV.push_back(start);
    }

    cscColIdxV.push_back(nnz);

    // sort by row to convert to csr
    for (int i = 0; i < nnz; i++) {
        cooSorted[i].first = cooRowV[i];
        cooSorted[i].second = cooColV[i];
    }

    // std::sort(cooSorted, cooSorted + nnz);
    std::sort(cooSorted.begin(), cooSorted.end());

    for (int i = 0; i < nnz; i++) {
        cooRowV[i] = cooSorted[i].first;
        cooColV[i] = cooSorted[i].second;
    }

    // convert coo to csr
    for (int i = 0; i < nnz; i++) {
        csrColV.push_back(cooColV[i]);
    }

    std::unordered_map<int, int> csrCounter;
    for (int value : cooRowV) {
        if (csrCounter.find(value) == csrCounter.end()) {
            // not found
            csrCounter[value] = 1;
        } else {
            csrCounter[value]++;
        }
    }

    for (int i = 0; i < rows; i++) {
        int start = 0;
        if (i != 0) {
            start = csrRowIdxV[i - 1];
            start += csrCounter[i - 1];
        }
        csrRowIdxV.push_back(start);
    }

    csrRowIdxV.push_back(nnz);

    CompressedGraph* g = new CompressedGraph(&csrRowIdxV, &csrColV, &cscRowV, &cscColIdxV, rows, nnz);
    return g;
}