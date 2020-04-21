#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <YeltsinDB/macro.h>

/**
 * @file journal/transaction_op.h
 * @todo Doxygen.
 */

struct __YDB_TransactionOp {
  YDB_OpCode opcode;
  YDB_OpDataSize size;
  void* data;
};
typedef struct __YDB_TransactionOp YDB_TransactionOp;

void ydb_transaction_op_destroy(YDB_TransactionOp* op) {
  free(op->data);
  free(op);
}

YDB_TransactionOp* ydb_transaction_op_clone(YDB_TransactionOp* op) {
  THROW_IF_NULL(op, NULL);
  YDB_TransactionOp* result = malloc(sizeof(YDB_TransactionOp));
  result->opcode = op->opcode;
  result->size = op->size;
  result->data = malloc(op->size);
  memcpy(result->data, op->data, op->size);
  return result;
}

#ifdef __cplusplus
}
#endif