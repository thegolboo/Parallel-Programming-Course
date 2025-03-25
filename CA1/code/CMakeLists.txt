cmake_minimum_required(VERSION 3.0)
project(CA1)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Include directories from OpenCV
include_directories(${OpenCV_INCLUDE_DIRS})

# Create an executable
add_executable(Q1 code_Q1.cpp)
add_executable(Q4 code_Q4.cpp)
# Link OpenCV libraries
target_link_libraries(Q1 ${OpenCV_LIBS})
target_link_libraries(Q4 ${OpenCV_LIBS})
