cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(fae)

option(FAE_BUILD_DOCS "Whether to build fae documentation" ON)
option(FAE_BUILD_TESTS "Whether to build fae unit tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(fae STATIC)
target_sources(fae PRIVATE fae.hpp fae.cpp)
target_include_directories(fae PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (FAE_BUILD_DOCS)
  find_package(Doxygen REQUIRED)
  doxygen_add_docs(
    doxygen
    ${PROJECT_SOURCE_DIR}
    COMMENT "Generating docs"
  )
endif()

if (FAE_BUILD_TESTS)
  add_subdirectory(test)
endif()
