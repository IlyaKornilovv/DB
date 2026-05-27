add_executable(unit_tests unit_tests.cpp)
target_link_libraries(unit_tests PRIVATE db_core)
set_target_properties(unit_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

add_executable(integration_tests integration_tests.cpp)
target_link_libraries(integration_tests PRIVATE db_core)
set_target_properties(integration_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
