cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(pine-enhances SHARED enhances.cpp)

find_library(log-lib log)

add_library(dobby STATIC IMPORTED)

get_filename_component(current_source_dir ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

set(external_dir "${current_source_dir}/../../../../external")
get_filename_component(export_dir ${external_dir} ABSOLUTE)
set_target_properties(dobby PROPERTIES IMPORTED_LOCATION ${external_dir}/dobby/${ANDROID_ABI}/libdobby.a)

target_link_libraries(pine-enhances ${log-lib} dobby)

ENABLE_LANGUAGE(ASM)