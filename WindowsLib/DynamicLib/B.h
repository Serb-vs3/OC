#pragma once

#ifdef _WIN32
  #ifdef BUILD_DYNAMIC_LIB
    #define DYN_API __declspec(dllexport)
  #else
    #define DYN_API __declspec(dllimport)
  #endif
#else
  #define DYN_API
#endif

extern "C" DYN_API int getB();
