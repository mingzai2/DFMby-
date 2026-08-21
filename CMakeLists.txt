cmake_minimum_required(VERSION 3.13)
project(libmingdfm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_definitions(-D__ANDROID_API__=24)
add_definitions(-fomit-frame-pointer)

if(NOT DEFINED ANDROID_ABI)
    set(ANDROID_ABI "arm64-v8a")
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCES
    ${SRC_DIR}/config_store.cpp
    ${SRC_DIR}/zygisk.cpp
)

add_library(mingdfm SHARED ${SOURCES})

target_include_directories(mingdfm PRIVATE ${SRC_DIR})

target_link_libraries(mingdfm
    PRIVATE
    log
    dl
)

set_target_properties(mingdfm PROPERTIES
    OUTPUT_NAME "mingdfm"
    PREFIX "lib"
    SUFFIX ".so"
)

if(ANDROID_ABI STREQUAL "arm64-v8a")
    target_compile_options(mingdfm PRIVATE -march=armv8-a)
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    target_compile_options(mingdfm PRIVATE -march=armv7-a -mfpu=neon)
elseif(ANDROID_ABI STREQUAL "x86_64")
    target_compile_options(mingdfm PRIVATE -march=x86-64)
elseif(ANDROID_ABI STREQUAL "x86")
    target_compile_options(mingdfm PRIVATE -march=i686)
endif()

install(TARGETS mingdfm LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})
