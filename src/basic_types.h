/*
 * Copyright 2020 transmission.aquitaine@yahoo.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef BASIC_TYPES_H_
#define BASIC_TYPES_H_

#include "addr_width.h"

#ifndef __cplusplus
#define inline /* inline replaced with nothing for the C compiler */
#endif



#ifdef _MSC_VER /* Microsoft compiler */

typedef   signed     __int8    int8;
typedef   signed     __int16   int16;
typedef   signed     __int32   int32;
typedef   signed     __int64   int64;
typedef   unsigned   __int8    uint8;
typedef   unsigned   __int16   uint16;
typedef   unsigned   __int32   uint32;
typedef   unsigned   __int64   uint64;

#define _64(x)     (x ## i64)

#endif

#ifdef __GNUC__ /* GNU compiler */

typedef   signed     char        int8;
typedef   signed     short       int16;
typedef   signed     int         int32;
typedef   signed     long long   int64;
typedef   unsigned   char        uint8;
typedef   unsigned   short       uint16;
typedef   unsigned   int         uint32;
typedef   unsigned   long long   uint64;

#define _64(x)     (x ## LL)

#endif

#define MAX_INT8   ((int8)(0x7F))
#define MIN_INT8   ((int8)(0x80))
#define MAX_INT16  ((int16)(0x7FFF))
#define MIN_INT16  ((int16)(0x8000))
#define MAX_INT32  ((int32)(0x7FFFFFFF))
#define MIN_INT32  ((int32)(0x80000000))
#define MAX_INT64  ((int64)(_64(0x7FFFFFFFFFFFFFFF)))
#define MIN_INT64  ((int64)(_64(0x8000000000000000)))

#define MAX_UINT8  ((uint8)(0xFF))
#define MIN_UINT8  ((uint8)(0))
#define MAX_UINT16 ((uint16)(0xFFFF))
#define MIN_UINT16 ((uint16)(0))
#define MAX_UINT32 ((uint32)(0xFFFFFFFF))
#define MIN_UINT32 ((uint32)(0))
#define MAX_UINT64 ((uint64)(_64(0xFFFFFFFFFFFFFFFF)))
#define MIN_UINT64 ((uint64)(0))



static inline uint64 alignLen512(uint64 len) { return len & 63 ? (len & ~((uint64)63)) + 64 : len; }
static inline uint32 alignLen512(uint32 len) { return len & 63 ? (len & ~((uint32)63)) + 64 : len; }
static inline uint64 alignLen256(uint64 len) { return len & 31 ? (len & ~((uint64)31)) + 32 : len; }
static inline uint32 alignLen256(uint32 len) { return len & 31 ? (len & ~((uint32)31)) + 32 : len; }
static inline uint64 alignLen128(uint64 len) { return len & 15 ? (len & ~((uint64)15)) + 16 : len; }
static inline uint32 alignLen128(uint32 len) { return len & 15 ? (len & ~((uint32)15)) + 16 : len; }
static inline uint64 alignLen64(uint64 len)  { return len &  7 ? (len & ~((uint64) 7)) +  8 : len; }
static inline uint32 alignLen64(uint32 len)  { return len &  7 ? (len & ~((uint32) 7)) +  8 : len; }
static inline uint64 alignLen32(uint64 len)  { return len &  3 ? (len & ~((uint64) 3)) +  4 : len; }
static inline uint32 alignLen32(uint32 len)  { return len &  3 ? (len & ~((uint32) 3)) +  4 : len; }

#if ADDR_WIDTH == 64
static inline uint64 alignLen(uint64 len) { return alignLen64(len); }
static inline uint32 alignLen(uint32 len) { return alignLen64(len); }
#else
static inline uint64 alignLen(uint64 len) { return alignLen32(len); }
static inline uint32 alignLen(uint32 len) { return alignLen32(len); }
#endif



static inline uint16 swapEndianness16(uint16 x)
    {
    char *p = (char*)&x;
    char c;
    c    = p[0];
    p[0] = p[1];
    p[1] = c;
    return x;
    }

static inline uint32 swapEndianness32(uint32 x)
    {
    char *p = (char*)&x;
    char c;
    c    = p[0];
    p[0] = p[3];
    p[3] = c;
    c    = p[1];
    p[1] = p[2];
    p[2] = c;
    return x;
    }

static inline uint64 swapEndianness64(uint64 x)
    {
    char *p = (char*)&x;
    char c;
    c    = p[0];
    p[0] = p[7];
    p[7] = c;
    c    = p[1];
    p[1] = p[6];
    p[6] = c;
    c    = p[2];
    p[2] = p[5];
    p[5] = c;
    c    = p[3];
    p[3] = p[4];
    p[4] = c;
    return x;
    }

static inline void *swapEndianness(void *x)
    {
#if ADDR_WIDTH == 64
    return (void*)swapEndianness64((uint64)x);
#else
    return (void*)swapEndianness32((uint32)x);
#endif
    }



#if ADDR_WIDTH == 64

#define IS64BIT (1)

typedef   int64    inta;
typedef   uint64   uinta;

#define MAX_INTA   MAX_INT64
#define MIN_INTA   MIN_INT64
#define MAX_UINTA  MAX_UINT64
#define MIN_UINTA  MIN_UINT64

#else /* assume ADDR_WIDTH == 32 */

#define IS32BIT (1)

typedef   int32    inta;
typedef   uint32   uinta;

#define MAX_INTA   MAX_INT32
#define MIN_INTA   MIN_INT32
#define MAX_UINTA  MAX_UINT32
#define MIN_UINTA  MIN_UINT32

#endif



typedef   uint8    bool8;
typedef   uint16   bool16;
typedef   uint32   bool32;
typedef   uint64   bool64;
typedef   uinta    boola;

#define YES (1)
#define NO  (0)



#endif /* BASIC_TYPES_H_ */
