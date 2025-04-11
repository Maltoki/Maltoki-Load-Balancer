cmake_minimum_required(VERSION 3.8)
project(AI_CPP_Load_Balancing_API)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenSSL REQUIRED)
find_package(Asio REQUIRED)
find_package(jwt-cpp REQUIRED)
find_package(Crow REQUIRED)


add_subdirectory(src)

