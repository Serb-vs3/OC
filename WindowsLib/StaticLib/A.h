#pragma once

#ifdef _WIN32
  #ifdef BUILD_STATIC_LIB
    #define STATIC_API __declspec(dllexport)
  #else
    #define STATIC_API __declspec(dllimport)
  #endif
#else
  #define STATIC_API
#endif

extern "C" STATIC_API int getA();
