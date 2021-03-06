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
#define TO_LE(x) _Generic((x), \
  uint16_t: htole16, \
  int16_t: htole16, \
  uint32_t: htole32, \
  int32_t: htole32, \
  uint64_t: htole64, \
  int64_t: htole64, \
)(x)

#define FROM_LE(x) _Generic((x), \
  uint16_t: le16toh, \
  int16_t: le16toh, \
  uint32_t: le32toh, \
  int32_t: le32toh, \
  uint64_t: le64toh, \
  int64_t: le64toh, \
)(x)

#define REASSIGN_FROM_LE(x) (x) = FROM_LE((x))
#else
#define TO_LE(x) (x)
#define FROM_LE(x) (x)
#define REASSIGN_FROM_LE(x)
#endif