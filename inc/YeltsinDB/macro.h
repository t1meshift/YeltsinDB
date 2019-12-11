#pragma once

#include <stdint.h>
#include <endian.h>

/**
 * @file macro.h
 * @brief The macros used in the engine.
 */

/** @brief Return `err` if `x` is null. */
#define THROW_IF_NULL(x, err) if (!(x)) return (err)

/**
 * @def TO_LE(x)
 * @param x Integer value.
 * @brief Convert a value stored in memory to little-endian.
 * Passes the value if current architecture is little-endian.
 */
/**
 * @def FROM_LE(x)
 * @param x Integer value.
 * @brief Convert a little-endian value to architecture endian.
 * Passes the value if current architecture is little-endian.
 */
/**
  * @def REASSIGN_FROM_LE(x)
  * @param x Non-const integer variable.
  * @brief Reassign a variable with converted stored value.
  * Does nothing if current architecture is little-endian.
  */

#if BYTE_ORDER == BIG_ENDIAN
/*
 * These inline functions are used for TO_LE and FROM_LE macros.
 * The reason is a preprocessor can't include leXXtoh and htoleXX macros into them.
 */
static inline uint16_t __htole16(uint16_t x) { return htole16(x); }
static inline uint16_t __le16toh(uint16_t x) { return le16toh(x); }
static inline uint32_t __htole32(uint32_t x) { return htole32(x); }
static inline uint32_t __le32toh(uint32_t x) { return le32toh(x); }
static inline uint64_t __htole64(uint64_t x) { return htole64(x); }
static inline uint64_t __le64toh(uint64_t x) { return le64toh(x); }

#define TO_LE(x) _Generic((x), \
  uint16_t: __htole16, \
  int16_t: __htole16, \
  uint32_t: __htole32, \
  int32_t: __htole32, \
  uint64_t: __htole64, \
  int64_t: __htole64 \
)(x)

#define FROM_LE(x) _Generic((x), \
  uint16_t: __le16toh, \
  int16_t: __le16toh, \
  uint32_t: __le32toh, \
  int32_t: __le32toh, \
  uint64_t: __le64toh, \
  int64_t: __le64toh \
)(x)

#define REASSIGN_FROM_LE(x) (void)((x) = FROM_LE((x)))
#else
#define TO_LE(x) (x)
#define FROM_LE(x) (x)
#define REASSIGN_FROM_LE(x)
#endif
