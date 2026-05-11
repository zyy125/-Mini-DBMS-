cmake_minimum_required(VERSION 3.16)

project(mini_dbms LANGUAGES CXX)

option(MINI_DBMS_BUILD_TESTS "Build Mini DBMS unit tests" ON)
option(MINI_DBMS_ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mini_dbms_common INTERFACE)
target_include_directories(mini_dbms_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(mini_dbms_common INTERFACE
    -Wall
    -Wextra
    -Wpedantic
)

if(MINI_DBMS_ENABLE_ASAN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "MINI_DBMS_ENABLE_ASAN requires GCC or Clang")
    endif()
    target_compile_options(mini_dbms_common INTERFACE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -g
    )
    target_link_options(mini_dbms_common INTERFACE
        -fsanitize=address,undefined
    )
endif()

add_library(mini_dbms_sql
    src/sql/lexer.cpp
    src/sql/parser.cpp
)

target_link_libraries(mini_dbms_sql PUBLIC mini_dbms_common)

add_library(mini_dbms_storage
    src/storage/storage_engine.cpp
)

target_link_libraries(mini_dbms_storage PUBLIC mini_dbms_common mini_dbms_sql)

add_library(mini_dbms_index
    src/index/bplus_tree_index.cpp
)

target_link_libraries(mini_dbms_index PUBLIC mini_dbms_common mini_dbms_storage)

if(MINI_DBMS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
