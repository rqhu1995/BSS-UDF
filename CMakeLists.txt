cmake_minimum_required(VERSION 3.10)
project(BikeStationUDF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Eigen package
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# 🔹 Add this block for OpenMP
find_package(OpenMP REQUIRED)

add_executable(BikeStationUDF
    src/main.cpp
    src/station.cpp
    src/udf_calculator.cpp
)

# 🔹 Include src directory and link dependencies
target_include_directories(BikeStationUDF PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(BikeStationUDF PRIVATE Eigen3::Eigen OpenMP::OpenMP_CXX)
