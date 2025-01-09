cmake_minimum_required(VERSION 3.10)
project(gpu_power)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify Python version explicitly
find_package(Python 3.12 EXACT REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)
find_library(ZE_LOADER_LIB ze_loader REQUIRED)

pybind11_add_module(gpu_power gpu_power_bindings.cpp)

target_include_directories(gpu_power PRIVATE
    /usr/include/pybind11
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(gpu_power PRIVATE ${ZE_LOADER_LIB})