#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <YeltsinDB/types.h>

/**
 * @file table_page.h
 * @brief A header with the definition of table page and functions to work with it.
 */

struct __YDB_TablePage;

/** @brief A table page type. */
typedef struct __YDB_TablePage YDB_TablePage;

/**
 * @brief Allocate a new page.
 * @param size Page size.
 * @return A pointer to allocated page.
 * @sa ydb_page_free()
 */
YDB_TablePage* ydb_page_alloc(YDB_PageSize size);

/**
 * @brief De-allocate page.
 * @param page Page to be de-allocated.
 */
void ydb_page_free(YDB_TablePage* page);

/**
 * @brief Seek page data position to `pos`.
 * @param page A page.
 * @param pos A new position (absolute value).
 * @return Operation status.
 *
 * @todo Possible error codes.
 */
YDB_Error ydb_page_data_seek(YDB_TablePage* page, YDB_PageSize pos);

/**
 * @brief Get current position in the page data.
 * @param page A page.
 * @return Page data position.
 */
YDB_PageSize ydb_page_data_tell(YDB_TablePage* page);

/**
 * @brief Read `n` bytes from a page to `dst`.
 * @param src A page with the data to be read.
 * @param dst A pointer to destination.
 * @param n An amount of bytes to read.
 * @return Operation status.
 *
 * @todo Possible error codes.
 */
YDB_Error ydb_page_data_read(YDB_TablePage* src, void* dst, YDB_PageSize n);

/**
 * @brief Write `n` bytes from `dst` to a page.
 * @param dst A destination page.
 * @param src A pointer to the source data.
 * @param n An amount of bytes to write.
 * @return Operation status.
 *
 * @todo Possible error codes.
 */
YDB_Error ydb_page_data_write(YDB_TablePage* dst, void* src, YDB_PageSize n);

/**
 * @brief Get page flags.
 * @param page A page.
 * @return Page flags.
 * @sa ydb_page_flags_set()
 *
 * Notice that page flags are represented as an integer, so you should apply bit mask to extract them.
 */
YDB_Flags ydb_page_flags_get(YDB_TablePage* page);

/**
 * @brief Set page flags.
 * @param page A page.
 * @param flags New page flags
 * @sa ydb_page_flags_get()
 */
void ydb_page_flags_set(YDB_TablePage* page, YDB_Flags flags);

/**
 * @brief Get row count.
 * @param page A page.
 * @return Row count.
 * @sa ydb_page_row_count_set()
 */
YDB_PageSize ydb_page_row_count_get(YDB_TablePage* page);

/**
 * @brief Set row count.
 * @param page A page
 * @param row_count New row count value.
 * @sa ydb_page_row_count_get()
 */
void ydb_page_row_count_set(YDB_TablePage* page, YDB_PageSize row_count);

#ifdef __cplusplus
}
#endif