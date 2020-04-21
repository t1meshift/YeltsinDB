#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file journal/transaction.h
 * @todo Doxygen.
 */

#include <YeltsinDB/journal/transaction_op.h>

struct __YDB_Transaction;
typedef struct __YDB_Transaction YDB_Transaction;

/**
 * @brief Create a new transaction.
 * @return A transaction instance.
 */
YDB_Transaction* ydb_transaction_create();

/**
 * @brief Destroy a transaction object.
 * @param t Transaction object.
 */
void ydb_transaction_destroy(YDB_Transaction* t);

YDB_Timestamp ydb_transaction_timestamp_get(YDB_Transaction* t);

YDB_Error ydb_transaction_timestamp_set(YDB_Transaction* t, YDB_Timestamp timestamp);

YDB_Flags ydb_transaction_flags_get(YDB_Transaction* t);

YDB_Error ydb_transaction_flags_set(YDB_Transaction* t, YDB_Flags flags);

uint32_t ydb_transaction_ops_size_get(YDB_Transaction* t);

YDB_Error ydb_transaction_push_op(YDB_Transaction* t, YDB_TransactionOp* op);

YDB_Error ydb_transaction_pop_op(YDB_Transaction* t);

YDB_TransactionOp* ydb_transaction_op_at(YDB_Transaction* t, uint32_t index);

YDB_Transaction* ydb_transaction_clone(YDB_Transaction* t);

#ifdef __cplusplus
}
#endif