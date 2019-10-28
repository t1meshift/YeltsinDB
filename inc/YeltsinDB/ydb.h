#pragma once

#include <stdint.h>
#include <YeltsinDB/types.h>
#include <YeltsinDB/table_page.h>

/**
 * @file ydb.h
 * @brief The main engine include file.
 */

struct __YDB_Engine;

/** @brief YDB engine instance type. */
typedef struct __YDB_Engine YDB_Engine;

/**
 * @brief Initialize YeltsinDB instance.
 *
 * @sa ydb_terminate_instance()
 */
YDB_Engine* ydb_init_instance();

/**
 * @brief Terminate YeltsinDB instance gracefully.
 *
 * @param instance A YeltsinDB instance.
 */
void ydb_terminate_instance(YDB_Engine* instance);

/**
 * @brief Load table data from file to the instance.
 *
 * @param instance A YeltsinDB instance.
 * @param path A path to the table data file.
 * @return Operation status.
 * @sa ydb_create_table(), ydb_unload_table()
 *
 * If the instance is not free (has already been loaded with table data), returns #YDB_ERR_INSTANCE_IN_USE.
 * @todo Possible error codes.
 */
YDB_Error ydb_load_table(YDB_Engine* instance, const char *path);

/**
 * @brief Create table data, write a file and load it to the instance.
 *
 * @param instance A YeltsinDB instance.
 * @param path A path to the table data file.
 * @return Operation status.
 * @sa ydb_load_table(), ydb_unload_table()
 *
 * If the instance is not free (has already been loaded with table data), returns #YDB_ERR_INSTANCE_IN_USE.
 * @todo Possible error codes.
 */
YDB_Error ydb_create_table(YDB_Engine* instance, const char *path);

/**
 * @brief Unload table instance, making it free for further table loading.
 * @param instance A *busy* YeltsinDB instance.
 * @return Operation status.
 *
 * @todo Possible error codes.
 */
YDB_Error ydb_unload_table(YDB_Engine* instance);

/**
 * @brief Seek to `index`th page.
 * @param instance A YeltsinDB instance.
 * @param index Page index.
 * @return Operation status.
 *
 * If an error was returned, the state of instance remains the same as it was before execution.
 * @todo Possible error codes.
 */
YDB_Error ydb_seek_page(YDB_Engine* instance, int64_t index);

/**
 * @brief Get current page index.
 * @param instance A YeltsinDB instance.
 * @return Current page index.
 * @sa error_code.h
 *
 * Could return error codes (negative values).
 * @todo Possible error codes.
 */
int64_t ydb_tell_page(YDB_Engine* instance);

/**
 * @brief Switch to next page.
 * @param instance A YeltsinDB instance.
 * @return Operation status.
 *
 * @todo Possible error codes.
 */
YDB_Error ydb_next_page(YDB_Engine* instance);

/**
 * @brief Replaces current page with `page` argument.
 * @param instance A YeltsinDB instance.
 * @param page A page to replace current one.
 * @return Operation status.
 *
 * @todo Possible error codes.
 */
YDB_Error ydb_replace_page(YDB_Engine* instance, YDB_TablePage* page);

/**
 * @brief Appends a page to a table.
 * @param instance A YeltsinDB instance.
 * @param page A page to be inserted.
 * @return Operation status.
 *
 * @todo Possible error codes.
 */
YDB_Error ydb_append_page(YDB_Engine* instance, YDB_TablePage* page);

/**
 * @brief Get current page object.
 * @param instance A YeltsinDB instance.
 * @return Current page object.
 *
 * Returns NULL on error.
 */
YDB_TablePage* ydb_get_current_page(YDB_Engine* instance);

/**
 * Delete current page.
 * @param instance A YeltsinDB instance.
 * @return Operation status.
 */
YDB_Error ydb_delete_current_page(YDB_Engine* instance);

// TODO: rebuild page offsets, etc.

/**
 * @mainpage YeltsinDB docs index page
 *
 * Welcome to docs page!
 *
 * These links could help you:
 *
 * - ydb.h
 *
 * - table_page.h
 *
 * - error_code.h
 *
 * - types.h
 *
 * And in some cases even [table file structure](table_file.md)
 */