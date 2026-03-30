#include "kg_query_engine/graph.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>

#ifdef _OPENMP
#include <omp.h>
#endif

class FastInput {
public:
    explicit FastInput(const std::string& file_path, std::size_t buffer_size = 1 << 20)
        : in_(file_path, std::ios::binary), buffer_(buffer_size, '\0') {
        if (!in_) {
            throw std::runtime_error("Unable to open file: " + file_path);
        }
    }

    bool readInt(int& out) {
        int sign = 1;
        long long value = 0;

        char c = 0;
        do {
            if (!readChar(c)) {
                return false;
            }
        } while (c <= ' ');

        if (c == '-') {
            sign = -1;
            if (!readChar(c)) {
                throw std::runtime_error("Malformed integer at end of file.");
            }
        }

        if (c < '0' || c > '9') {
            throw std::runtime_error("Malformed integer in edge list.");
        }

        while (c >= '0' && c <= '9') {
            value = value * 10 + (c - '0');
            if (value > static_cast<long long>(std::numeric_limits<int>::max())) {
                throw std::runtime_error("Integer value exceeds int range.");
            }
            if (!readChar(c)) {
                break;
            }
        }

        out = static_cast<int>(value) * sign;
        return true;
    }

private:
    bool refill() {
        in_.read(buffer_.data(), static_cast<std::streamsize>(buffer_.size()));
        bytes_in_buffer_ = static_cast<std::size_t>(in_.gcount());
        cursor_ = 0;
        return bytes_in_buffer_ > 0;
    }

    bool readChar(char& c) {
        if (cursor_ >= bytes_in_buffer_ && !refill()) {
            return false;
        }
        c = buffer_[cursor_++];
        return true;
    }

    std::ifstream in_;
    std::vector<char> buffer_;
    std::size_t bytes_in_buffer_ = 0;
    std::size_t cursor_ = 0;
};

Graph::Graph(bool directed) : directed_(directed) {}

void Graph::loadFromEdgeList(const std::string& file_path) {
    std::size_t max_node_id = 0;
    edge_count_ = 0;

    {
        FastInput input(file_path);
        int u = 0;
        int v = 0;
        while (input.readInt(u)) {
            if (!input.readInt(v)) {
                throw std::runtime_error("Odd number of integers in input. Every edge must be a pair.");
            }

            if (u < 0 || v < 0) {
                throw std::runtime_error("Negative node IDs are not supported in this phase.");
            }

            max_node_id = std::max(max_node_id, static_cast<std::size_t>(u));
            max_node_id = std::max(max_node_id, static_cast<std::size_t>(v));
            ++edge_count_;
        }
    }

    adjacency_.assign(max_node_id + 1, {});
    reverse_adjacency_.assign(max_node_id + 1, {});
    active_node_count_ = 0;
    std::vector<unsigned char> seen(max_node_id + 1, 0);

    {
        FastInput input(file_path);
        int u = 0;
        int v = 0;
        while (input.readInt(u)) {
            if (!input.readInt(v)) {
                throw std::runtime_error("Odd number of integers in input. Every edge must be a pair.");
            }

            const std::size_t su = static_cast<std::size_t>(u);
            const std::size_t sv = static_cast<std::size_t>(v);

            adjacency_[su].push_back(v);
            if (!directed_ && su != sv) {
                adjacency_[sv].push_back(u);
            } else if (directed_) {
                reverse_adjacency_[sv].push_back(u);
            }

            if (!seen[su]) {
                seen[su] = 1;
                ++active_node_count_;
            }
            if (!seen[sv]) {
                seen[sv] = 1;
                ++active_node_count_;
            }
        }
    }
}

std::size_t Graph::totalNodeSlots() const { return adjacency_.size(); }

std::size_t Graph::activeNodeCount() const { return active_node_count_; }

std::size_t Graph::edgeCount() const { return edge_count_; }

bool Graph::isDirected() const { return directed_; }

std::vector<int> Graph::shortestPathBidirectional(int source, int target) const {
    if (source < 0 || target < 0) {
        return {};
    }

    const std::size_t s = static_cast<std::size_t>(source);
    const std::size_t t = static_cast<std::size_t>(target);
    if (s >= adjacency_.size() || t >= adjacency_.size()) {
        return {};
    }

    if (source == target) {
        return {source};
    }

    const std::size_t n = adjacency_.size();
    std::vector<int> parent_from_source(n, -1);
    std::vector<int> parent_from_target(n, -1);
    std::vector<unsigned char> visited_from_source(n, 0);
    std::vector<unsigned char> visited_from_target(n, 0);

    std::vector<int> frontier_source{source};
    std::vector<int> frontier_target{target};
    std::vector<int> next_frontier_source;
    std::vector<int> next_frontier_target;

    visited_from_source[s] = 1;
    visited_from_target[t] = 1;
    parent_from_source[s] = source;
    parent_from_target[t] = target;

    int meeting_node = -1;
    while (!frontier_source.empty() && !frontier_target.empty()) {
        meeting_node = expandOneLayer(
            frontier_source,
            next_frontier_source,
            adjacency_,
            visited_from_source,
            parent_from_source,
            visited_from_target);
        if (meeting_node != -1) {
            break;
        }

        frontier_source.swap(next_frontier_source);

        const std::vector<std::vector<int>>& backward_neighbors =
            directed_ ? reverse_adjacency_ : adjacency_;

        meeting_node = expandOneLayer(
            frontier_target,
            next_frontier_target,
            backward_neighbors,
            visited_from_target,
            parent_from_target,
            visited_from_source);
        if (meeting_node != -1) {
            break;
        }

        frontier_target.swap(next_frontier_target);
    }

    if (meeting_node == -1) {
        return {};
    }

    return buildPathFromParents(meeting_node, source, target, parent_from_source, parent_from_target);
}

int Graph::expandOneLayer(
    const std::vector<int>& frontier,
    std::vector<int>& next_frontier,
    const std::vector<std::vector<int>>& neighbors,
    std::vector<unsigned char>& visited_here,
    std::vector<int>& parent_here,
    const std::vector<unsigned char>& visited_other) {
    next_frontier.clear();

#ifdef _OPENMP
    const int thread_count = omp_get_max_threads();
    std::vector<std::vector<int>> local_nodes(static_cast<std::size_t>(thread_count));
    std::vector<std::vector<int>> local_parents(static_cast<std::size_t>(thread_count));

#pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        std::vector<int>& nodes = local_nodes[static_cast<std::size_t>(tid)];
        std::vector<int>& parents = local_parents[static_cast<std::size_t>(tid)];

#pragma omp for schedule(dynamic, 64) nowait
        for (int i = 0; i < static_cast<int>(frontier.size()); ++i) {
            const int node = frontier[static_cast<std::size_t>(i)];
            const std::size_t snode = static_cast<std::size_t>(node);
            for (int next : neighbors[snode]) {
                nodes.push_back(next);
                parents.push_back(node);
            }
        }
    }

    for (int tid = 0; tid < thread_count; ++tid) {
        std::vector<int>& nodes = local_nodes[static_cast<std::size_t>(tid)];
        std::vector<int>& parents = local_parents[static_cast<std::size_t>(tid)];
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            const int next = nodes[i];
            const int parent = parents[i];
            const std::size_t snext = static_cast<std::size_t>(next);
            if (visited_here[snext]) {
                continue;
            }

            visited_here[snext] = 1;
            parent_here[snext] = parent;
            next_frontier.push_back(next);

            if (visited_other[snext]) {
                return next;
            }
        }
    }

    return -1;
#else

    for (int node : frontier) {
        const std::size_t snode = static_cast<std::size_t>(node);
        for (int next : neighbors[snode]) {
            const std::size_t snext = static_cast<std::size_t>(next);
            if (visited_here[snext]) {
                continue;
            }

            visited_here[snext] = 1;
            parent_here[snext] = node;
            next_frontier.push_back(next);

            if (visited_other[snext]) {
                return next;
            }
        }
    }

    return -1;
#endif
}

std::vector<int> Graph::buildPathFromParents(
    int meeting_node,
    int source,
    int target,
    const std::vector<int>& parent_from_source,
    const std::vector<int>& parent_from_target) {
    std::vector<int> left;
    for (int node = meeting_node; node != source; node = parent_from_source[static_cast<std::size_t>(node)]) {
        left.push_back(node);
    }
    left.push_back(source);
    std::reverse(left.begin(), left.end());

    std::vector<int> right;
    int node = meeting_node;
    while (node != target) {
        node = parent_from_target[static_cast<std::size_t>(node)];
        if (node < 0) {
            return {};
        }
        right.push_back(node);
    }

    left.insert(left.end(), right.begin(), right.end());
    return left;
}
