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
#define YDB_ERR_ZERO_SIZE_RW                (-12)
/**
 * @brief No more pages left in the file.
 */
#define YDB_ERR_NO_MORE_PAGES               (-13)
/**
 * @brief The addresses of pages are the same.
 */
#define YDB_ERR_SAME_PAGE_ADDRESS           (-14)
/**
 * @brief The transaction is not initialized.
 */
#define YDB_ERR_TRANSACTION_NOT_INITIALIZED (-15)
/**
 * @brief Transaction operation push failed.
 */
#define YDB_ERR_TRANSACTION_OP_PUSH_FAILED  (-16)
/**
 * @brief Index of transaction operation is out of range.
 */
#define YDB_ERR_TRANSACTION_OP_OUT_OF_RANGE (-17)
/**
 * @brief The journal is not initialized.
 */
#define YDB_ERR_JOURNAL_NOT_INITIALIZED     (-18)
/**
 * @brief The journal is in use.
 */
#define YDB_ERR_JOURNAL_IN_USE              (-19)
/**
 * @brief The journal is not in use.
 */
#define YDB_ERR_JOURNAL_NOT_IN_USE          (-20)
/**
 * @brief The journal file does not exist.
 */
#define YDB_ERR_JOURNAL_NOT_EXIST           (-21)
/**
 * @brief The journal file does already exist.
 */
#define YDB_ERR_JOURNAL_EXIST               (-22)
/**
 * @brief The journal file is corrupted.
 */
#define YDB_ERR_JOURNAL_FILE_CORRUPTED      (-23)
/**
 * @brief The journal is not consistent.
 */
#define YDB_ERR_JOURNAL_NOT_CONSISTENT      (-24)
/**
 * @brief No more transactions left in the journal.
 */
#define YDB_ERR_NO_MORE_TRANSACTIONS        (-25)
/**
 * @brief The journal is empty.
 */
#define YDB_ERR_JOURNAL_EMPTY               (-26)
/**
 * @brief An unknown error has occurred.
 */
#define YDB_ERR_UNKNOWN                     (-32768)

/** @} */ // End of @defgroup