cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(pine-enhances SHARED enhances.cpp)

find_library(log-lib log)

# Dobby requires the c++ exception support, so we cannot use self-compiled libcxx here.
# find_package(cxx REQUIRED CONFIG)

find_package(dobby REQUIRED CONFIG)

target_link_libraries(pine-enhances ${log-lib} dobby::dobby)

ENABLE_LANGUAGE(ASM)
