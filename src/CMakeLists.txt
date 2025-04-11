

add_executable(server "main.cpp" "encoding.h" "auth.h" "encryption.h" "request_handler.h")

target_link_libraries(server PRIVATE
    OpenSSL::Crypto
    OpenSSL::SSL
    jwt-cpp::jwt-cpp
)

target_include_directories(server PRIVATE
    ${OPENSSL_INCLUDE_DIR}
    ${CROW_INCLUDE_DIR}
    ${Asio_INCLUDE_DIR}
    ${JWT_CPP_INCLUDE_DIR}
)


# Link pthread (for multithreading) - Not required for Windows
if(NOT WIN32)
    target_link_libraries(server pthread)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET server PROPERTY CXX_STANDARD 20)
endif()
