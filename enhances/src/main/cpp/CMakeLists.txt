cmake_minimum_required(VERSION 3.4.1)

cmake_policy(SET CMP0079 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(C_FLAGS "-Wall -Wextra -fvisibility=hidden -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -flto=thin")
set(LINKER_FLAGS "-fuse-ld=lld -flto=thin -ffixed-x18 -Wl,--hash-style=both -Wl,--unresolved-symbols=ignore-all")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")

set(DOBBY_DEBUG OFF CACHE INTERNAL "" FORCE)
set(DOBBY_GENERATE_SHARED OFF CACHE INTERNAL "" FORCE)
# Don't need Dobby's SymbolResolver as we have our own
set(Plugin.SymbolResolver OFF CACHE INTERNAL "" FORCE)
set(NearBranch OFF CACHE INTERNAL "" FORCE)
add_subdirectory(external/dobby dobby)
get_property(DOBBY_INCLUDE_DIRECTORIES
        TARGET dobby
        PROPERTY INCLUDE_DIRECTORIES)
include_directories(
        ${DOBBY_INCLUDE_DIRECTORIES}
        $<TARGET_PROPERTY:dobby,INCLUDE_DIRECTORIES>
)

add_library(pine-enhances SHARED enhances.cpp)

find_library(log-lib log)

find_package(cxx REQUIRED CONFIG)

target_link_libraries(dobby cxx::cxx)

target_link_libraries(pine-enhances ${log-lib} cxx::cxx dobby)

ENABLE_LANGUAGE(ASM)
