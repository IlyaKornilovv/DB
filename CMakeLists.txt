cmake_minimum_required(VERSION 3.15)
project(DB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(Threads REQUIRED)

set(DB_CORE_SOURCES
    src/storage/catalog_store.cpp
    src/query/text_tools.cpp
    src/query/sql_engine.cpp
    src/wire/socket_compat.cpp
)

add_library(db_core STATIC ${DB_CORE_SOURCES})
target_link_libraries(db_core PUBLIC Threads::Threads)

add_library(db_native SHARED
    src/client/native_client.cpp
    src/wire/socket_compat.cpp
)
target_compile_definitions(db_native PRIVATE DB_NATIVE_EXPORTS)
if(WIN32)
  target_link_libraries(db_core PUBLIC ws2_32)
  target_link_libraries(db_native PRIVATE ws2_32)
endif()
set_target_properties(db_native PROPERTIES OUTPUT_NAME "db_native")

add_executable(customdb_server
    src/app/server_entry.cpp
    src/wire/server_runtime.cpp
)
target_link_libraries(customdb_server PRIVATE db_core)
if(WIN32)
  target_link_libraries(customdb_server PRIVATE ws2_32)
endif()

install(TARGETS customdb_server db_native
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

option(BUILD_TESTS "Build small validation executables" ON)
if(BUILD_TESTS)
  add_subdirectory(tests)
endif()
