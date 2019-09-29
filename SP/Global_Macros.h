#ifndef GLOBAL_MACROS_H
#define GLOBAL_MACROS_H

#pragma once

#define SP_DEBUG_SWITCH 0
#define SP_TRACE_SWITCH 0

#ifdef __ANDROID__
#include <android/log.h>

#  define TAG "SP_STREAM"
#  define SP_LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__)
#  define SP_LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__)
#  if (SP_TRACE_SWITCH == 1)
#    define SP_TRACE(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__)
#  else
#    define SP_TRACE(...)
#  endif
#else
#  define SP_LOGI(...) printf(__VA_ARGS__)
#  define SP_LOGE(...) printf(__VA_ARGS__)
#  if (SP_TRACE_SWITCH == 1)
#    define SP_TRACE(...) printf(__VA_ARGS__)
#  else
#    define SP_TRACE(...)
#  endif
#  if (SP_DEBUG_SWITCH == 1)
#    define SP_DEBUG(...) printf(__VA_ARGS__)
#  else
#    define SP_DEBUG(...)
#  endif
#endif

// ----------------------------------------------------------------

#    define SP_NEW_RETURN(POINTER,CONSTRUCTOR,RET_VAL) \
   do { POINTER = new CONSTRUCTOR; \
     if (POINTER == 0) { return RET_VAL; } \
   } while (0)
#    define SP_NEW(POINTER,CONSTRUCTOR) \
   do { POINTER = new CONSTRUCTOR; \
     if (POINTER == 0) { return; } \
   } while (0)
#    define SP_NEW_NORETURN(POINTER,CONSTRUCTOR) \
   do { POINTER = new CONSTRUCTOR; \
     if (POINTER == 0) { } \
   } while (0)

// ----------------------------------------------------------------

# define SP_DES(POINTER) \
   do { \
        if (POINTER) \
          { \
            delete (POINTER); \
            POINTER = 0; \
          } \
      } \
   while (0)

# define SP_DES_FREE(POINTER,DEALLOCATOR,CLASS) \
   do { \
        if (POINTER) \
          { \
            (POINTER)->~CLASS (); \
            DEALLOCATOR (POINTER); \
          } \
      } \
   while (0)

# define SP_DES_ARRAY(POINTER) \
      do { \
           if (POINTER) \
             { \
               delete[] (POINTER); \
               POINTER = 0; \
             } \
         } \
      while (0)


#endif // !GLOBAL_MACROS_H


