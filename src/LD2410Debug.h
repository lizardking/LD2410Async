

#pragma once
#include "Arduino.h"

// =======================================================================================
// Debug level configuration
// =======================================================================================
// If not defined by the user/project, initialize to 0 (disabled).

#ifndef LD2410ASYNC_DEBUG_LEVEL
#define LD2410ASYNC_DEBUG_LEVEL 0
#endif

#ifndef LD2410ASYNC_DEBUG_DATA_LEVEL
#define LD2410ASYNC_DEBUG_DATA_LEVEL 0
#endif


// =======================================================================================
// Helper: Hex buffer print (only compiled if needed)
// =======================================================================================
#if (LD2410ASYNC_DEBUG_LEVEL > 1) || (LD2410ASYNC_DEBUG_DATA_LEVEL > 1)
static inline void printBuf(const byte* buf, byte size)
{
    for (byte i = 0; i < size; i++) {
        String bStr(buf[i], HEX);
        bStr.toUpperCase();
        if (bStr.length() == 1) bStr = "0" + bStr;
        Serial.print(bStr);
        Serial.print(' ');
    }
    Serial.println();
}
#endif

// =======================================================================================
// Normal debug macros
// =======================================================================================
#if LD2410ASYNC_DEBUG_LEVEL > 0
#define DEBUG_PRINT_MILLIS    { Serial.print(millis()); Serial.print(" "); }
#define DEBUG_PRINT(...)      { Serial.print(__VA_ARGS__); }
#define DEBUG_PRINTLN(...)    { Serial.println(__VA_ARGS__); }
#if LD2410ASYNC_DEBUG_LEVEL > 1
#define DEBUG_PRINTBUF(...) { printBuf(__VA_ARGS__); }
#else
#define DEBUG_PRINTBUF(...)
#endif
#else
#define DEBUG_PRINT_MILLIS
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#define DEBUG_PRINTBUF(...)
#endif

// =======================================================================================
// Data debug macros
// =======================================================================================
#if LD2410ASYNC_DEBUG_DATA_LEVEL > 0
#define DEBUG_PRINT_DATA(...)      { Serial.print(__VA_ARGS__); }
#define DEBUG_PRINTLN_DATA(...)    { Serial.println(__VA_ARGS__); }
#if LD2410ASYNC_DEBUG_DATA_LEVEL > 1
#define DEBUG_PRINTBUF_DATA(...) { printBuf(__VA_ARGS__); }
#else
#define DEBUG_PRINTBUF_DATA(...)
#endif
#else
#define DEBUG_PRINT_DATA(...)
#define DEBUG_PRINTLN_DATA(...)
#define DEBUG_PRINTBUF_DATA(...)
#endif
