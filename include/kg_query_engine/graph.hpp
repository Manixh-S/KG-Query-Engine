#pragma once

#include <cstddef>
#include <string>
#include <vector>

class Graph {
public:
    explicit Graph(bool directed = false);

    void loadFromEdgeList(const std::string& file_path);

    std::size_t totalNodeSlots() const;
    std::size_t activeNodeCount() const;
    std::size_t edgeCount() const;
    bool isDirected() const;

    std::vector<int> shortestPathBidirectional(int source, int target) const;

private:
    static int expandOneLayer(
        const std::vector<int>& frontier,
        std::vector<int>& next_frontier,
        const std::vector<std::vector<int>>& neighbors,
        std::vector<unsigned char>& visited_here,
        std::vector<int>& parent_here,
        const std::vector<unsigned char>& visited_other);

    static std::vector<int> buildPathFromParents(
        int meeting_node,
        int source,
        int target,
        const std::vector<int>& parent_from_source,
        const std::vector<int>& parent_from_target);

    std::vector<std::vector<int>> adjacency_;
    std::vector<std::vector<int>> reverse_adjacency_;
    std::size_t edge_count_ = 0;
    std::size_t active_node_count_ = 0;
    bool directed_ = false;
};
