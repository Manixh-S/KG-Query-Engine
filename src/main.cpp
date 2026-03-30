#include "kg_query_engine/graph.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <edge_list.txt> <source> <target> [directed: 0|1]\n";
        return 1;
    }

    const std::string file_path = argv[1];
    const int source = std::stoi(argv[2]);
    const int target = std::stoi(argv[3]);
    const bool directed = (argc >= 5) ? (std::stoi(argv[4]) != 0) : false;

    try {
        auto t0 = std::chrono::high_resolution_clock::now();
        Graph graph(directed);
        graph.loadFromEdgeList(file_path);
        auto t1 = std::chrono::high_resolution_clock::now();
        const std::vector<int> path = graph.shortestPathBidirectional(source, target);
        auto t2 = std::chrono::high_resolution_clock::now();

        const auto load_elapsed_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        const auto bfs_elapsed_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

        std::cout << "Graph load complete.\n";
        std::cout << "Mode: " << (graph.isDirected() ? "Directed" : "Undirected") << "\n";
    #ifdef _OPENMP
        std::cout << "OpenMP: enabled (max threads=" << omp_get_max_threads() << ")\n";
    #else
        std::cout << "OpenMP: disabled (compiled without OpenMP)\n";
    #endif
        std::cout << "Node slots (max_node_id + 1): " << graph.totalNodeSlots() << "\n";
        std::cout << "Active nodes seen in edge list: " << graph.activeNodeCount() << "\n";
        std::cout << "Input edge count: " << graph.edgeCount() << "\n";
        std::cout << "Load elapsed: " << load_elapsed_ms << " ms\n";
        std::cout << "BFS elapsed: " << bfs_elapsed_ms << " ms\n";

        if (path.empty()) {
            std::cout << "No path found between " << source << " and " << target << ".\n";
        } else {
            std::cout << "Shortest path length (edges): " << (path.size() - 1) << "\n";
            std::cout << "Path: ";
            for (std::size_t i = 0; i < path.size(); ++i) {
                std::cout << path[i];
                if (i + 1 < path.size()) {
                    std::cout << " -> ";
                }
            }
            std::cout << "\n";
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 2;
    }
}
