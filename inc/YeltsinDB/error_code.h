#pragma once
/**
 * @file error_code.h
 * @brief A header with engine's error codes.
 */

/**
 * @defgroup ydb_error_codes YeltsinDB error codes
 * The codes for errors that could occur during engine's work.
 * @{
 */

/**
 * @brief The operation has been done successfully.
 */
#define YDB_ERR_SUCCESS                     ( 0)
/**
 * @brief The table does *not* exist.
 */
#define YDB_ERR_TABLE_NOT_EXIST             (-1)
/**
 * @brief The table does already exist.
 */
#define YDB_ERR_TABLE_EXIST                 (-2)
/**
 * @brief No more memory left in the page.
 */
#define YDB_ERR_PAGE_NO_MORE_MEM            (-3)
/**
 * @brief The table data is corrupted.
 */
#define YDB_ERR_TABLE_DATA_CORRUPTED        (-4)
/**
 * @brief Table data version mismatch.
 */
#define YDB_ERR_TABLE_DATA_VERSION_MISMATCH (-5)
/**
 * @brief The engine instance is not initialized.
 */
#define YDB_ERR_INSTANCE_NOT_INITIALIZED    (-6)
/**
 * @brief The engine instance is in use.
 */
#define YDB_ERR_INSTANCE_IN_USE             (-7)
/**
 * @brief The engine instance is not in use.
 */
#define YDB_ERR_INSTANCE_NOT_IN_USE         (-8)
/**
 * @brief Page index is out of range.
 */
#define YDB_ERR_PAGE_INDEX_OUT_OF_RANGE     (-9)
/**
 * @brief The page is not initialized.
 */
#define YDB_ERR_PAGE_NOT_INITIALIZED        (-10)
/**
 * @brief The destination pointer is NULL.
 */
#define YDB_ERR_WRITE_TO_NULLPTR            (-11)
/**
 * @brief An attempt to read/write 0 bytes has occurred.
 */
#define YDB_ERR_ZERO_SIZE_RW                (-11)
/**
 * @brief No more pages left in the file.
 */
#define YDB_ERR_NO_MORE_PAGES               (-12)
/**
 * @brief An unknown error has occurred.
 */
#define YDB_ERR_UNKNOWN                     (-32768)

/** @} */ // End of @defgroup