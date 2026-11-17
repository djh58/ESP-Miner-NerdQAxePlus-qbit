cmake_minimum_required(VERSION 3.16)
project(qbit_host_tests CXX)

# Host-side tests for the firmware's qbit helpers (main/qbit), which have no
# ESP-IDF dependency:
#   cmake -S test/host -B test/host/build && cmake --build test/host/build && test/host/build/qbit_host_tests
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FW_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/../../main)
file(GLOB QBIT_SOURCES CONFIGURE_DEPENDS ${FW_MAIN}/qbit/*.cpp)

add_executable(qbit_host_tests test_main.cpp test_qbit_chain.cpp test_qbit_address.cpp ${QBIT_SOURCES})
target_include_directories(qbit_host_tests PRIVATE ${FW_MAIN}/qbit ${FW_MAIN}/../components/arduinojson
                           ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(qbit_host_tests PRIVATE -Wall -Wextra)
