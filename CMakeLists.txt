cmake_minimum_required(VERSION 3.16)
project(KGQueryEngine VERSION 1.0.0 LANGUAGES CXX)

option(KGQE_ENABLE_OPENMP "Enable OpenMP parallelization" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(kg_query_engine
    src/main.cpp
    src/graph.cpp
)

target_include_directories(kg_query_engine PRIVATE include)

if(KGQE_ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(kg_query_engine PRIVATE OpenMP::OpenMP_CXX)
        target_compile_definitions(kg_query_engine PRIVATE KGQE_OPENMP_ENABLED=1)
    endif()
endif()
