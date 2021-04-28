cmake_minimum_required(VERSION 3.14)
project(M6SS)
set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(M6SS main.cpp syncparameters.cpp syncparameters.h simulator.cpp simulator.h model.cpp model.h modelvalidation.cpp modelvalidation.h timeinterval.cpp timeinterval.h)
target_link_libraries(M6SS LINK_PUBLIC sqlite3)