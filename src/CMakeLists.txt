# CMakeList.txt : CMake project for AI CPP Load Balancing API, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (server "main.cpp" "encryption.h" "encoding.h" "request_handler.h" "auth.h")

# Include ASIO and Crow
target_include_directories(server PRIVATE ${ASIO_INCLUDE_DIR} ${crow_SOURCE_DIR}/include)

find_package(jwt-cpp REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(server PRIVATE OpenSSL::SSL OpenSSL::Crypto jwt-cpp::jwt-cpp)


# Link pthread (for multithreading) - Not required for Windows
if(NOT WIN32)
    target_link_libraries(SERVER_EXECUTABLE pthread)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET server PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
