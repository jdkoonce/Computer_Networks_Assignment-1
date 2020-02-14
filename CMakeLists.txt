cmake_minimum_required(VERSION 3.13)  # CMake version check
project(activation)               # Create project "activation"
set(CMAKE_CXX_STANDARD 17)            # Enable c++17 standard

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(Client.exe Client.cpp)
add_executable(Server.exe Server.cpp)