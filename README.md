# KG Query Engine

High-performance Knowledge Graph shortest-path engine written in Modern C++17 with OpenMP acceleration.

This project ingests massive unweighted edge-list files and computes the exact shortest relational path between two entities using a Bidirectional BFS design.

## Stanford SNAP Test Dataset

This project was tested with data from the Stanford Network Analysis Project (SNAP), specifically the Amazon product co-purchasing network with ground-truth communities.

- Network file: `com-amazon.ungraph.txt` (unweighted graph used by the engine)
- Community file: `com-amazon.top5000.cmty.txt` (ground-truth communities, useful for validation and analysis)

Reference: https://snap.stanford.edu/data/com-Amazon.html

Quick test steps:

1. Download the Amazon SNAP dataset from the page above.
2. Place `com-amazon.ungraph.txt` in the local `data/` folder.
3. Build the project (see Build section below).
4. Run a shortest-path query, for example:

```bash
./build/kg_query_engine data/com-amazon.ungraph.txt 1 10000 0
```

On Windows with a `.exe` build artifact:

```bash
.\build\kg_query_engine.exe data\com-amazon.ungraph.txt 1 10000 0
```

## Features

- Memory-optimized adjacency list graph representation
- Fast buffered edge-list parser for large text files
- Bidirectional BFS with exact path reconstruction
- OpenMP-parallel frontier expansion (with non-OpenMP fallback)
- Supports directed and undirected graph traversal modes

## Project Structure

```text
KG-Query-Engine/
|- include/
|  `- kg_query_engine/
|     `- graph.hpp          # Graph API
|- src/
|  |- graph.cpp             # Graph ingestion + bidirectional BFS core
|  `- main.cpp              # CLI entrypoint
|- data/                    # Optional place for local datasets (not committed by default)
|- CMakeLists.txt           # CMake build setup
`- README.md
```

## Requirements

- C++17 compatible compiler
  - GCC 9+
  - Clang 10+
  - MSVC 2019+ (with OpenMP support enabled)
- CMake 3.16+
- OpenMP runtime (recommended for Phase 3 performance)

## Input Format

The engine reads a plain text edge list where each line has two space-separated integer node IDs:

```text
u v
```

Example:

```text
0 1
1 2
2 3
0 4
4 3
```

## Build

### Option 1: CMake (Recommended)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Executable output:

- Linux/macOS: `build/kg_query_engine`
- Windows (single-config generators): `build/kg_query_engine.exe`
- Windows (multi-config generators): `build/Release/kg_query_engine.exe`

### Option 2: Direct compiler command

#### GCC / MinGW

```bash
g++ -std=c++17 -O3 -fopenmp src/main.cpp src/graph.cpp -Iinclude -o kg_query_engine
```

#### Clang

```bash
clang++ -std=c++17 -O3 -fopenmp src/main.cpp src/graph.cpp -Iinclude -o kg_query_engine
```

## Run

```bash
kg_query_engine <edge_list.txt> <source> <target> [directed: 0|1]
```

Examples:

```bash
kg_query_engine data/test_graph.txt 1 10000 0
kg_query_engine data/test_graph.txt 5 42 1
```

Arguments:

- `<edge_list.txt>`: path to input edge-list file
- `<source>`: source node ID (integer)
- `<target>`: target node ID (integer)
- `[directed: 0|1]`:
  - `0` (default): treat graph as undirected
  - `1`: treat graph as directed

## Output

The program prints:

- Graph mode and OpenMP status
- Node slot count, active node count, edge count
- Load time and BFS time
- Exact shortest path and hop count (or no-path result)

## Algorithm Summary

### Phase 1

- Two-pass ingestion over the edge list:
  - Pass 1 computes max node ID and edge count
  - Pass 2 populates adjacency vectors

### Phase 2

- Bidirectional BFS from source and target
- Parent tracking on both search directions
- Exact shortest path reconstruction through the meeting node

### Phase 3

- OpenMP-parallel neighbor collection for each BFS layer
- Thread-local staging to avoid write contention
- Deterministic sequential commit to visited/parent state for correctness

## Performance Notes

- Use `-O3` and OpenMP for large graphs
- Place dataset files on SSD for faster ingestion
- For extremely large graphs, monitor RAM and tune OpenMP thread count using `OMP_NUM_THREADS`

## GitHub Upload Checklist

1. Initialize repository (if needed):

```bash
git init
```

2. Stage files:

```bash
git add .
```

3. Commit:

```bash
git commit -m "Initial commit: KG Query Engine with OpenMP bidirectional BFS"
```

4. Add remote and push:

```bash
git branch -M main
git remote add origin https://github.com/<your-username>/KG-Query-Engine.git
git push -u origin main
```

