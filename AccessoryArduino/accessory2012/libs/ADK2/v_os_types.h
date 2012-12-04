#ifdef ADK_INTERNAL
/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.

 ********************************************************************/
#ifndef _OS_TYPES_H
#define _OS_TYPES_H

//defines since we cannot have a makefile...
#define BYTE_ORDER 1
#define LITTLE_ENDIAN 1
#define BIG_ENDIAN=2
#define _ARM_ASSEM_


#ifdef _LOW_ACCURACY_
#  define X(n) (((((n)>>22)+1)>>1) - ((((n)>>22)+1)>>9))
#  define LOOKUP_T const unsigned char
#else
#  define X(n) (n)
#  define LOOKUP_T const ogg_int32_t
#endif

/* make it easy on the folks that want to compile the libs with a
   different malloc than stdlib */
#if 1
#define _ogg_malloc  malloc
#define _ogg_calloc  calloc
#define _ogg_realloc realloc
#define _ogg_free    free
#else
#define _ogg_malloc(x) ({ \
	void *p = malloc(x); \
	dbgPrintf("M %d = %p\n", (x), p); \
	p; \
})
#define _ogg_calloc(x,y) ({ \
	void *p = calloc(x,y); \
	dbgPrintf("C %d %d \ %p\n", (x), (y), p); \
	p; \
})
#define _ogg_realloc(x,y) ({ \
	void *p = realloc(x,y); \
	dbgPrintf("R %d %d = %p\n", (x), (y), p); \
	p; \
})
#define _ogg_free(x) ({ \
	dbgPrintf("F %p\n", (x)); \
	free(x); \
})

#undef alloca
#define alloca(x) ({ \
	void *p = __builtin_alloca(x); \
	dbgPrintf("A %d F %p bot %p\n", (x), __builtin_frame_address(0), p); \
	p; \
})
#endif

#if 0
#define VT do { \
	dbgPrintf("%s:%d Frame %p\n", __func__, __LINE__, __builtin_frame_address(0)); \
} while (0)
#else
#define VT do { } while (0)
#endif

#ifdef _WIN32 

#  ifndef __GNUC__
   /* MSVC/Borland */
   typedef __int64 ogg_int64_t;
   typedef __int32 ogg_int32_t;
   typedef unsigned __int32 ogg_uint32_t;
   typedef __int16 ogg_int16_t;
   typedef unsigned __int16 ogg_uint16_t;
#  else
   /* Cygwin */
   #include <_G_config.h>
   typedef _G_int64_t ogg_int64_t;
   typedef _G_int32_t ogg_int32_t;
   typedef _G_uint32_t ogg_uint32_t;
   typedef _G_int16_t ogg_int16_t;
   typedef _G_uint16_t ogg_uint16_t;
#  endif

#elif defined(__MACOS__)

#  include <sys/types.h>
   typedef SInt16 ogg_int16_t;
   typedef UInt16 ogg_uint16_t;
   typedef SInt32 ogg_int32_t;
   typedef UInt32 ogg_uint32_t;
   typedef SInt64 ogg_int64_t;

#elif defined(__MACOSX__) /* MacOS X Framework build */

#  include <sys/types.h>
   typedef int16_t ogg_int16_t;
   typedef u_int16_t ogg_uint16_t;
   typedef int32_t ogg_int32_t;
   typedef u_int32_t ogg_uint32_t;
   typedef int64_t ogg_int64_t;

#elif defined(__BEOS__)

   /* Be */
#  include <inttypes.h>

#elif defined (__EMX__)

   /* OS/2 GCC */
   typedef short ogg_int16_t;
   typedef unsigned short ogg_uint16_t;
   typedef int ogg_int32_t;
   typedef unsigned int ogg_uint32_t;
   typedef long long ogg_int64_t;

#else

#  include <sys/types.h>
#  include "v_config_types.h"

#endif

#endif  /* _OS_TYPES_H */
#endif

