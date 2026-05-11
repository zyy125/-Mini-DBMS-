add_executable(mini_dbms_common_tests
    test_main.cpp
    common/dynamic_array_test.cpp
    common/hash_table_test.cpp
    common/linked_list_test.cpp
    common/status_test.cpp
    common/string_utils_test.cpp
)

target_link_libraries(mini_dbms_common_tests PRIVATE mini_dbms_common)

add_test(NAME common_tests COMMAND mini_dbms_common_tests)
