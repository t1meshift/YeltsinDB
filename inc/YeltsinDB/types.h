#pragma once

#include <stdint.h>

/**
 * @file types.h
 * @brief A header with the engine types.
 */

/** @brief YeltsinDB error type. */
typedef int16_t YDB_Error;
/** @brief YeltsinDB data file offset type. */
typedef uint64_t YDB_Offset;
/** @brief YeltsinDB page size type. */
typedef uint16_t YDB_PageSize;
/** @brief YeltsinDB flags container type. */
typedef uint8_t YDB_Flags;
/** @brief YeltsinDB journal operation code type. */
typedef uint8_t YDB_OpCode;
/** @brief YeltsinDB Unix timestamp type. */
typedef int64_t YDB_Timestamp;
/** @brief YeltsinDB operation data size type. */
typedef uint32_t YDB_OpDataSize;