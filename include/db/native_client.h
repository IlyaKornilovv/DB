#pragma once

#ifdef _WIN32
  #ifdef DB_NATIVE_EXPORTS
    #define DB_API __declspec(dllexport)
  #else
    #define DB_API __declspec(dllimport)
  #endif
#else
  #define DB_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

DB_API void* db_connect(const char* host, int port);
DB_API const char* db_execute(void* connection, const char* query);
DB_API void db_disconnect(void* connection);
DB_API void db_free_string(const char* str);

#ifdef __cplusplus
}
#endif
