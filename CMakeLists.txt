cmake_minimum_required(VERSION 3.0.0)
project(ffn VERSION 0.1.0)

include(CTest)
enable_testing()

# this set project C++ standart
set(CMAKE_CXX_STANDARD 17)

include_directories(/usr/local/include)

add_executable(ffn ffn.cpp FFNGame.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
