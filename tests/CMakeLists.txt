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

add_executable(mini_dbms_sql_tests
    test_main.cpp
    sql/parser_test.cpp
)

target_link_libraries(mini_dbms_sql_tests PRIVATE mini_dbms_sql)

add_test(NAME sql_tests COMMAND mini_dbms_sql_tests)
