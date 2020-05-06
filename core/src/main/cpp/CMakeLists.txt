cmake_minimum_required(VERSION 3.4.1)

set(PINE_ARCH_UNRELATED_SOURCES
        pine.cpp
        ruler.cpp
        android.cpp
        jni_bridge.cpp
        art/art_method.cpp
        art/thread.cpp
        art/jit.cpp
        trampoline/memory.cpp
        trampoline/trampoline_installer.cpp
        utils/elf_img.cpp
        utils/well_known_classes.cpp)

if (CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch armv7")
    set(PINE_ARCH_RELATED_SOURCES
            trampoline/arch/thumb2.cpp
            trampoline/arch/thumb2.S)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch arm64")
    set(PINE_ARCH_RELATED_SOURCES
            trampoline/arch/arm64.cpp
            trampoline/arch/arm64.S)
else()
    message(FATAL_ERROR "Unsupported cpu abi ${CMAKE_ANDROID_ARCH_ABI} (Only armeabi-v7a and arm64-v8a are now supported)!")
endif()

add_library(pine SHARED ${PINE_ARCH_UNRELATED_SOURCES} ${PINE_ARCH_RELATED_SOURCES})

find_library(log-lib log)

target_link_libraries(pine ${log-lib})

ENABLE_LANGUAGE(ASM)