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

add_executable(mini_dbms_storage_tests
    test_main.cpp
    storage/storage_engine_test.cpp
)

target_link_libraries(mini_dbms_storage_tests PRIVATE mini_dbms_storage)

add_test(NAME storage_tests COMMAND mini_dbms_storage_tests)

add_executable(mini_dbms_index_tests
    test_main.cpp
    index/bplus_tree_index_test.cpp
)

target_link_libraries(mini_dbms_index_tests PRIVATE mini_dbms_index)

add_test(NAME index_tests COMMAND mini_dbms_index_tests)

add_executable(mini_dbms_executor_tests
    test_main.cpp
    executor/executor_test.cpp
)

target_link_libraries(mini_dbms_executor_tests PRIVATE mini_dbms_executor)

add_test(NAME executor_tests COMMAND mini_dbms_executor_tests)

add_executable(mini_dbms_network_tests
    test_main.cpp
    network/protocol_test.cpp
)

target_link_libraries(mini_dbms_network_tests PRIVATE mini_dbms_network)

add_test(NAME network_tests COMMAND mini_dbms_network_tests)

add_test(NAME e2e_acceptance
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/e2e_acceptance.sh
            $<TARGET_FILE:db_server>
            $<TARGET_FILE:db_client>
            ${CMAKE_CURRENT_BINARY_DIR}/e2e_acceptance_data
)
