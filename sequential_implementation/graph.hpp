#include <string>
#include <unordered_map>
#include <vector>

// class to represent a graph in both compressed row
// and compressed column state
class CompressedGraph {
    public:

    CompressedGraph(std::vector<int>* csrRowIdx, std::vector<int>* csrCol, std::vector<int>* cscRow, std::vector<int>* cscColIdx, int activeNodes, int nnz);

    std::vector<int>* getActiveNodes();

    bool isEmpty();

    void removeValue(int id);

    void removeComponent(std::vector<int>* ids);

    std::vector<int>* trimSingleConnectedComponents();

    std::vector<int>* pointedFrom(int id);

    std::vector<int>* pointsTo(int id);

    std::vector<int>* pred(int id, int color, std::unordered_map<int, int>* colors);

    private:

    std::vector<int> csrRowIdx;
    std::vector<int> csrCol;
    std::vector<int> cscRow;
    std::vector<int> cscColIdx;
    std::unordered_map<int, bool> activeNodes;
    int activeNodesCount;
};

CompressedGraph* read_matrix_file_and_convert(std::string filename);