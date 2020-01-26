// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <cstddef>

template <typename T, std::size_t N>
char (&array_size_helper(T (&array)[N]))[N];
#define ARRAYSIZE(array) (sizeof(::array_size_helper(array)))

#define DISALLOW_COPY(TypeName) TypeName(const TypeName &) = delete

#define DISALLOW_ASSIGN(TypeName) void operator=(const TypeName &) = delete

// A macro to disallow the copy constructor and operator= functions
// This is usually placed in the private: declarations for a class.
#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                     \
  TypeName(const TypeName &) = delete;                                         \
  void operator=(const TypeName &) = delete

#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName)                               \
  TypeName() = delete;                                                         \
  DISALLOW_COPY_AND_ASSIGN(TypeName)

// GCC can be told that a certain branch is not likely to be taken (for
// instance, a CHECK failure), and use that information in static analysis.
// Giving it this information can help it optimize for the common case in
// the absence of better information (ie. -fprofile-arcs).
#if defined(__GNUC__)
#define LIKELY(x) (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

// we only compile on linux. both clang & gcc support this
#if defined(__clang__) || defined(__GNUC__)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
#define ALWAYS_INLINE inline
#endif

#if defined(__clang__) || defined(__GNUC__)
#define NOINLINE __attribute__((__noinline__))
#else
#define NOINLINE
#endif