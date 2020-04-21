#ifdef __cplusplus
extern "C" {
#endif

#include <YeltsinDB/types.h>
#include <YDB_ext/vec.h>
#include <YeltsinDB/journal/transaction_op.h>
#include <YeltsinDB/macro.h>
#include <YeltsinDB/error_code.h>
#include <YeltsinDB/journal/transaction.h>

typedef vec_t(YDB_TransactionOp*) vec_op_t;

struct __YDB_Transaction {
  YDB_Timestamp timestamp;
  YDB_Flags flags;
  vec_op_t ops;
};

YDB_Transaction *ydb_transaction_create() {
  YDB_Transaction* result = calloc(1, sizeof(YDB_Transaction));
  vec_init(&result->ops);
  return result;
}

void ydb_transaction_destroy(YDB_Transaction *t) {
  THROW_IF_NULL(t, (void)0);
  int i; YDB_TransactionOp* op;
  vec_foreach(&t->ops, op, i) {
      ydb_transaction_op_destroy(t->ops.data[i]);
  }
  vec_deinit(&t->ops);
  free(t);
}
YDB_Timestamp ydb_transaction_timestamp_get(YDB_Transaction *t) {
  THROW_IF_NULL(t, 1 << sizeof(YDB_Timestamp)); // NOLINT minimal signed int value
  return t->timestamp;
}
YDB_Error ydb_transaction_timestamp_set(YDB_Transaction *t, YDB_Timestamp timestamp) {
  THROW_IF_NULL(t, YDB_ERR_TRANSACTION_NOT_INITIALIZED);
  t->timestamp = timestamp;
  return YDB_ERR_SUCCESS;
}

YDB_Flags ydb_transaction_flags_get(YDB_Transaction *t) {
  THROW_IF_NULL(t, 0); // NOLINT minimal signed int value
  return t->flags;
}

YDB_Error ydb_transaction_flags_set(YDB_Transaction *t, YDB_Flags flags) {
  THROW_IF_NULL(t, YDB_ERR_TRANSACTION_NOT_INITIALIZED);
  t->flags = flags;
  return YDB_ERR_SUCCESS;
}

uint32_t ydb_transaction_ops_size_get(YDB_Transaction *t) {
  THROW_IF_NULL(t, 0);
  return t->ops.length;
}

YDB_Error ydb_transaction_push_op(YDB_Transaction* t, YDB_TransactionOp *op) {
  THROW_IF_NULL(t, YDB_ERR_TRANSACTION_NOT_INITIALIZED);
  YDB_TransactionOp* op_copy = ydb_transaction_op_clone(op);
  if(vec_push(&t->ops, op_copy)) {
    return YDB_ERR_TRANSACTION_OP_PUSH_FAILED;
  }
  return YDB_ERR_SUCCESS;
}

YDB_Error ydb_transaction_pop_op(YDB_Transaction *t) {
  THROW_IF_NULL(t, YDB_ERR_TRANSACTION_NOT_INITIALIZED);
  uint32_t vec_back = t->ops.length - 1;
  ydb_transaction_op_destroy(t->ops.data[vec_back]);
  vec_pop(&t->ops);
  return YDB_ERR_SUCCESS;
}

YDB_TransactionOp *ydb_transaction_op_at(YDB_Transaction *t, uint32_t index) {
  THROW_IF_NULL(t, NULL);
  if (index > ydb_transaction_ops_size_get(t)) {
    return NULL;
  }
  return t->ops.data[index];
}

YDB_Transaction *ydb_transaction_clone(YDB_Transaction *t) {
  THROW_IF_NULL(t, NULL);
  YDB_Transaction* r = ydb_transaction_create();
  r->timestamp = t->timestamp;
  r->flags = t->flags;
  int i; YDB_TransactionOp* op;
  vec_foreach(&t->ops, op, i) {
    vec_push(&r->ops, ydb_transaction_op_clone(op));
  }

  return r;
}

#ifdef __cplusplus
}
#endif