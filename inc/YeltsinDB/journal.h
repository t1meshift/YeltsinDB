#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <YeltsinDB/types.h>
#include <YeltsinDB/journal/transaction.h>

/**
 * @file journal.h
 * @todo Doxygen.
 */

struct __YDB_Journal;
typedef struct __YDB_Journal YDB_Journal;

YDB_Journal* ydb_journal_init();

void ydb_journal_destroy(YDB_Journal* jrnl);

YDB_Error ydb_journal_file_create(YDB_Journal* jrnl, char* path);

YDB_Error ydb_journal_file_open(YDB_Journal* jrnl, char* path);

YDB_Error ydb_journal_file_close(YDB_Journal* jrnl);

YDB_Error ydb_journal_file_check_consistency(YDB_Journal* jrnl);

YDB_Error ydb_journal_seek_to_begin(YDB_Journal* jrnl);

YDB_Error ydb_journal_seek_to_end(YDB_Journal* jrnl);

YDB_Error ydb_journal_prev_transaction(YDB_Journal* jrnl);

YDB_Error ydb_journal_next_transaction(YDB_Journal* jrnl);

YDB_Error ydb_journal_append_transaction(YDB_Journal* jrnl, YDB_Transaction* transaction);

YDB_Transaction* ydb_journal_get_current_transaction(YDB_Journal* jrnl);

#ifdef __cplusplus
}
#endif