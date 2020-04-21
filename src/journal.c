#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <YeltsinDB/types.h>
#include <YeltsinDB/error_code.h>
#include <YeltsinDB/constants.h>
#include <YeltsinDB/macro.h>
#include <YeltsinDB/journal/transaction.h>
#include <YeltsinDB/journal/transaction_op.h>
#include <YeltsinDB/journal.h>

struct __YDB_Journal {
  FILE *fd;
  YDB_Offset first_transaction_offset;
  YDB_Offset last_transaction_offset;

  YDB_Offset prev_transaction_offset;
  YDB_Offset curr_transaction_offset;
  YDB_Offset next_transaction_offset;

  YDB_Transaction* curr_transaction;

  uint8_t is_consistent;
  uint8_t in_use;
};

inline static YDB_Error __ydb_jrnl_truncate_incomplete(YDB_Journal* j, YDB_Offset last_complete_offset) {
  THROW_IF_NULL(j, YDB_ERR_JOURNAL_NOT_INITIALIZED);

  // The only transaction is incomplete
  if (last_complete_offset == 0) {
    j->prev_transaction_offset = 0;
    j->next_transaction_offset = 0;
    j->curr_transaction_offset = 0;
    j->first_transaction_offset = 0;
    j->last_transaction_offset = 0;

    fseeko(j->fd, YDB_jrnl_first_transaction_offset, SEEK_SET);
    fwrite(&j->first_transaction_offset, sizeof(YDB_Offset), 1, j->fd);
    fwrite(&j->last_transaction_offset, sizeof(YDB_Offset), 1, j->fd);

    ftruncate(fileno(j->fd), YDB_jrnl_data_offset);
    fsync(fileno(j->fd));
    return YDB_ERR_SUCCESS;
  }

  fseeko(j->fd, last_complete_offset + YDB_jrnl_transaction_prev_offset, SEEK_SET);

  YDB_Offset prev_offset;
  fread(&prev_offset, sizeof(YDB_Offset), 1, j->fd);
  REASSIGN_FROM_LE(prev_offset);

  YDB_Offset next_offset;
  fread(&next_offset, sizeof(YDB_Offset), 1, j->fd);
  REASSIGN_FROM_LE(next_offset);

  fseeko(j->fd, last_complete_offset + YDB_jrnl_transaction_next_offset, SEEK_SET);
  YDB_Offset new_next_offset = 0;
  fwrite(&new_next_offset, sizeof(YDB_Offset), 1, j->fd);

  j->last_transaction_offset = last_complete_offset;
  fseeko(j->fd, YDB_jrnl_last_transaction_offset, SEEK_SET);
  YDB_Offset last_complete_offset_le = TO_LE(last_complete_offset);
  fwrite(&last_complete_offset_le, sizeof(YDB_Offset), 1, j->fd);

  ftruncate(fileno(j->fd), next_offset);
  fsync(fileno(j->fd));

  return YDB_ERR_SUCCESS;
}

inline static YDB_TransactionOp* __ydb_read_op(YDB_Journal* j) {
  THROW_IF_NULL(j, NULL);
  YDB_TransactionOp* r = malloc(sizeof(YDB_TransactionOp));
  fread(&r->opcode, sizeof(YDB_OpCode), 1, j->fd);
  fread(&r->size, sizeof(YDB_OpDataSize), 1, j->fd);
  r->data = malloc(r->size);
  fread(r->data, r->size, 1, j->fd);
  return r;
}

inline static YDB_Error __ydb_jrnl_read(YDB_Journal* j, YDB_Offset off) {
  THROW_IF_NULL(j, YDB_ERR_JOURNAL_NOT_INITIALIZED);

  j->curr_transaction_offset = off;
  fseeko(j->fd, off, SEEK_SET);

  YDB_Offset prev_offset, next_offset;
  YDB_Timestamp timestamp;
  YDB_Flags flags;

  fread(&prev_offset, sizeof(YDB_Offset), 1, j->fd);
  fread(&next_offset, sizeof(YDB_Offset), 1, j->fd);
  fread(&timestamp, sizeof(YDB_Timestamp), 1, j->fd);
  fread(&flags, sizeof(YDB_Flags), 1, j->fd);

  j->prev_transaction_offset = prev_offset;
  j->next_transaction_offset = next_offset;

  YDB_Transaction* t = ydb_transaction_create();
  ydb_transaction_timestamp_set(t, timestamp);
  ydb_transaction_flags_set(t, flags);

  YDB_TransactionOp* op;
  do {
    op = __ydb_read_op(j);
    if (op->opcode != YDB_jrnl_op_complete) {
      ydb_transaction_push_op(t, op);
    }
  } while (op->opcode != YDB_jrnl_op_complete);

  return YDB_ERR_SUCCESS;
}

YDB_Journal *ydb_journal_init() {
  YDB_Journal* result = calloc(1, sizeof(YDB_Journal));
  return result;
}

void ydb_journal_destroy(YDB_Journal *jrnl) {
  ydb_journal_file_close(jrnl);
  free(jrnl);
}

YDB_Error ydb_journal_file_create(YDB_Journal *jrnl, char *path) {
  THROW_IF_NULL(jrnl, YDB_ERR_JOURNAL_NOT_INITIALIZED);
  THROW_IF_NULL(!jrnl->in_use, YDB_ERR_JOURNAL_NOT_IN_USE);

  if (access(path, F_OK) != -1) {
    // TODO: Windows does not check W_OK correctly, use other methods.
    // TODO: if can't read/write, throw other error
    return YDB_ERR_JOURNAL_EXIST;
  }

  FILE *f = fopen(path, "wb");

  char tpl[] = YDB_JRNL_SIGN          // File signature
  "\x00\x00\x00\x00\x00\x00\x00\x00"  // First transaction offset
  "\x00\x00\x00\x00\x00\x00\x00\x00"; // Last transaction offset
  fwrite(tpl, sizeof(tpl)-1, 1, f);

  fclose(f);

  return ydb_journal_file_open(jrnl, path);
}

YDB_Error ydb_journal_file_open(YDB_Journal *jrnl, char *path) {
  THROW_IF_NULL(jrnl, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(!jrnl->in_use, YDB_ERR_INSTANCE_IN_USE);

  if (access(path, F_OK) == -1) {
    // TODO: Windows does not check W_OK correctly, use other methods.
    // TODO: if can't read/write, throw other error
    return YDB_ERR_JOURNAL_NOT_EXIST;
  }

  jrnl->fd = fopen(path, "rb+");
  THROW_IF_NULL(jrnl->fd, YDB_ERR_UNKNOWN); // TODO file open error

  char signature[YDB_JRNL_SIGN_SIZE];
  fread(signature, 1, YDB_JRNL_SIGN_SIZE, jrnl->fd);
  int sig_match = memcmp(signature, YDB_JRNL_SIGN, YDB_JRNL_SIGN_SIZE) == 0;
  if (!sig_match) {
    return YDB_ERR_JOURNAL_FILE_CORRUPTED;
  }

  // TODO check for read error
  fread(&jrnl->first_transaction_offset, sizeof(YDB_Offset), 1, jrnl->fd);
  REASSIGN_FROM_LE(jrnl->first_transaction_offset);
  fread(&jrnl->last_transaction_offset, sizeof(YDB_Offset), 1, jrnl->fd);
  REASSIGN_FROM_LE(jrnl->last_transaction_offset);

  jrnl->in_use = -1;
  jrnl->curr_transaction_offset = jrnl->first_transaction_offset;

  return ydb_journal_file_check_consistency(jrnl);
}

YDB_Error ydb_journal_file_close(YDB_Journal *jrnl) {
  THROW_IF_NULL(jrnl, YDB_ERR_JOURNAL_NOT_INITIALIZED);
  THROW_IF_NULL(jrnl->in_use, YDB_ERR_JOURNAL_NOT_IN_USE);

  jrnl->first_transaction_offset = 0;
  jrnl->last_transaction_offset = 0;
  jrnl->prev_transaction_offset = 0;
  jrnl->curr_transaction_offset = 0;
  jrnl->next_transaction_offset = 0;
  ydb_transaction_destroy(jrnl->curr_transaction);
  jrnl->curr_transaction = NULL;
  fclose(jrnl->fd);
  jrnl->in_use = 0;

  return YDB_ERR_SUCCESS;
}

YDB_Error ydb_journal_file_check_consistency(YDB_Journal *jrnl) {
  THROW_IF_NULL(jrnl, YDB_ERR_JOURNAL_NOT_INITIALIZED);
  THROW_IF_NULL(jrnl->in_use, YDB_ERR_JOURNAL_NOT_IN_USE);

  // Empty journal is always consistent lol
  if (jrnl->last_transaction_offset == 0) {
    jrnl->is_consistent = -1;
    return YDB_ERR_SUCCESS;
  }

  YDB_Offset last_complete_offset = 0;
  YDB_Offset prev_offset = 0;
  YDB_Offset next_offset = 0;

  fseeko(jrnl->fd, jrnl->last_transaction_offset, SEEK_SET);
  if (fread(&prev_offset, sizeof(YDB_Offset), 1, jrnl->fd) != sizeof(YDB_Offset)) {
    if (feof(jrnl->fd)) {
      clearerr(jrnl->fd);
      perror("Mozhno nenado...\n");
      abort();
      // TODO try to find last transaction from begin
    } else {
      return YDB_ERR_UNKNOWN; // FIXME
    }
  }
  REASSIGN_FROM_LE(prev_offset);

  last_complete_offset = prev_offset;

  if (fread(&next_offset, sizeof(YDB_Offset), 1, jrnl->fd) != sizeof(YDB_Offset)) {
    if (feof(jrnl->fd)) {
      clearerr(jrnl->fd);
      __ydb_jrnl_truncate_incomplete(jrnl, last_complete_offset);
      jrnl->is_consistent = -1;
      return YDB_ERR_SUCCESS;
    } else {
      return YDB_ERR_UNKNOWN; // FIXME
    }
  }
  REASSIGN_FROM_LE(next_offset);

  if (next_offset != 0) {
    perror("Kavo? Ne ponyal shyas...\n");
    abort(); // Аборт -- это грех, кста
  }

  fseeko(jrnl->fd, jrnl->last_transaction_offset + YDB_jrnl_transaction_flags_offset, SEEK_SET);
  if (feof(jrnl->fd)) {
    __ydb_jrnl_truncate_incomplete(jrnl, last_complete_offset);
    jrnl->is_consistent = -1;
    return YDB_ERR_SUCCESS;
  } else if (ferror(jrnl->fd)) {
    return YDB_ERR_UNKNOWN; //FIXME
  }

  YDB_Flags flags = 0;
  fread(&flags, sizeof(YDB_Flags), 1, jrnl->fd);

  if (flags & 1) { //FIXME magic
    // A transaction has been written to the storage.
    return YDB_ERR_SUCCESS;
  } else {
    YDB_OpCode op = 0;

    while (op != YDB_jrnl_op_complete) {
      YDB_OpDataSize size;
      fread(&op, sizeof(YDB_TransactionOp), 1, jrnl->fd);
      fread(&size, sizeof(YDB_OpDataSize), 1, jrnl->fd);
      REASSIGN_FROM_LE(size);

      if (feof(jrnl->fd) || ferror(jrnl->fd)) break;
      fseeko(jrnl->fd, size, SEEK_CUR);
    }

    if (op == YDB_jrnl_op_complete) {
      //fseeko(jrnl->fd, jrnl->last_transaction_offset + YDB_jrnl_transaction_flags_offset, SEEK_SET);
      //fwrite(flags | 1, sizeof(YDB_Flags), 1, jrnl->fd); //FIXME magic // Also flag should be written when the data in storage is consistent
      //fsync(fileno(jrnl->fd));
      jrnl->is_consistent = -1;
      return YDB_ERR_SUCCESS;
    }
    __ydb_jrnl_truncate_incomplete(jrnl, last_complete_offset);
  }

  jrnl->is_consistent = -1;
  YDB_Error e = ydb_journal_seek_to_begin(jrnl);
  if (e == YDB_ERR_SUCCESS || e == YDB_ERR_JOURNAL_EMPTY) {
    return YDB_ERR_SUCCESS;
  }
}

YDB_Error ydb_journal_seek_to_begin(YDB_Journal *jrnl) {
  THROW_IF_NULL(jrnl, YDB_ERR_JOURNAL_NOT_INITIALIZED);
  THROW_IF_NULL(jrnl->in_use, YDB_ERR_JOURNAL_NOT_IN_USE);
  THROW_IF_NULL(jrnl->is_consistent, YDB_ERR_JOURNAL_NOT_CONSISTENT);
  THROW_IF_NULL(jrnl->first_transaction_offset, YDB_ERR_JOURNAL_EMPTY);

  return __ydb_jrnl_read(jrnl, jrnl->first_transaction_offset);
}

YDB_Error ydb_journal_seek_to_end(YDB_Journal *jrnl) {
  THROW_IF_NULL(jrnl, YDB_ERR_JOURNAL_NOT_INITIALIZED);
  THROW_IF_NULL(jrnl->in_use, YDB_ERR_JOURNAL_NOT_IN_USE);
  THROW_IF_NULL(jrnl->is_consistent, YDB_ERR_JOURNAL_NOT_CONSISTENT);
  THROW_IF_NULL(jrnl->last_transaction_offset, YDB_ERR_JOURNAL_EMPTY);

  return __ydb_jrnl_read(jrnl, jrnl->last_transaction_offset);
}

YDB_Error ydb_journal_prev_transaction(YDB_Journal *jrnl) {
  THROW_IF_NULL(jrnl, YDB_ERR_JOURNAL_NOT_INITIALIZED);
  THROW_IF_NULL(jrnl->in_use, YDB_ERR_JOURNAL_NOT_IN_USE);
  THROW_IF_NULL(jrnl->is_consistent, YDB_ERR_JOURNAL_NOT_CONSISTENT);

  if (jrnl->prev_transaction_offset != 0) {
    __ydb_jrnl_read(jrnl, jrnl->prev_transaction_offset);
    return YDB_ERR_SUCCESS;
  }
  return YDB_ERR_NO_MORE_TRANSACTIONS;
}

YDB_Error ydb_journal_next_transaction(YDB_Journal *jrnl) {
  THROW_IF_NULL(jrnl, YDB_ERR_JOURNAL_NOT_INITIALIZED);
  THROW_IF_NULL(jrnl->in_use, YDB_ERR_JOURNAL_NOT_IN_USE);
  THROW_IF_NULL(jrnl->is_consistent, YDB_ERR_JOURNAL_NOT_CONSISTENT);

  if (jrnl->next_transaction_offset != 0) {
    __ydb_jrnl_read(jrnl, jrnl->prev_transaction_offset);
    return YDB_ERR_SUCCESS;
  }
  return YDB_ERR_NO_MORE_TRANSACTIONS;
}

YDB_Error ydb_journal_append_transaction(YDB_Journal *jrnl, YDB_Transaction *transaction) {
  THROW_IF_NULL(jrnl, YDB_ERR_JOURNAL_NOT_INITIALIZED);
  THROW_IF_NULL(jrnl->in_use, YDB_ERR_JOURNAL_NOT_IN_USE);
  THROW_IF_NULL(jrnl->is_consistent, YDB_ERR_JOURNAL_NOT_CONSISTENT);

  YDB_Transaction* t = ydb_transaction_clone(transaction);

  fseeko(jrnl->fd, 0, SEEK_END);
  YDB_Offset file_end = ftello(jrnl->fd);
  YDB_Offset file_end_le = TO_LE(file_end);

  YDB_Offset prev = jrnl->last_transaction_offset;
  YDB_Offset prev_le = TO_LE(prev);
  YDB_Offset next_le = 0;
  YDB_Timestamp ts_le = TO_LE(ydb_transaction_timestamp_get(t));
  YDB_Flags flags = ydb_transaction_flags_get(t);

  fseeko(jrnl->fd, file_end, SEEK_SET);
  fwrite(&prev_le, sizeof(YDB_Offset), 1, jrnl->fd);
  fwrite(&next_le, sizeof(YDB_Offset), 1, jrnl->fd);
  fwrite(&ts_le, sizeof(YDB_Timestamp), 1, jrnl->fd);
  fwrite(&flags, sizeof(YDB_Flags), 1, jrnl->fd);

  jrnl->last_transaction_offset = file_end;
  fseeko(jrnl->fd, YDB_jrnl_last_transaction_offset, SEEK_SET);
  fwrite(&file_end_le, sizeof(YDB_Offset), 1, jrnl->fd);

  fsync(fileno(jrnl->fd)); // Checkpoint

  if (prev != 0) {
    fseeko(jrnl->fd, prev + YDB_jrnl_transaction_next_offset, SEEK_SET);
  } else {
    fseeko(jrnl->fd, YDB_jrnl_first_transaction_offset, SEEK_SET);
  }
  fwrite(&file_end_le, sizeof(YDB_Offset), 1, jrnl->fd);

  fsync(fileno(jrnl->fd)); // Checkpoint

  fseeko(jrnl->fd, file_end, SEEK_SET);
  uint32_t op_cnt = ydb_transaction_ops_size_get(t);
  for (uint32_t i = 0; i < op_cnt; ++i) {
    YDB_TransactionOp* op = ydb_transaction_op_at(t, i);
    fwrite(&op->opcode, sizeof(YDB_OpCode), 1, jrnl->fd);
    fwrite(&op->size, sizeof(YDB_OpDataSize), 1, jrnl->fd);
    fwrite(&op->data, op->size, 1, jrnl->fd);
    fsync(fileno(jrnl->fd));
  }
  // Write completion op
  fputc(YDB_jrnl_op_complete, jrnl->fd);
  YDB_OpDataSize sz = 0;
#ifndef NO_EASTER_EGGS
  char easter[] = "Andrew is a pi-door-ass";
  sz = sizeof(easter) - 1;
  fwrite(&sz, sizeof(YDB_OpDataSize), 1, jrnl->fd);
  fwrite(easter, sz, 1, jrnl->fd);
#else
  fwrite(&sz, sizeof(YDB_OpDataSize), 1, jrnl->fd);
#endif
  fsync(fileno(jrnl->fd));

  return __ydb_jrnl_read(jrnl, file_end);
}

YDB_Transaction *ydb_journal_get_current_transaction(YDB_Journal *jrnl) {
  THROW_IF_NULL(jrnl, NULL);
  THROW_IF_NULL(jrnl->in_use, NULL);
  THROW_IF_NULL(jrnl->is_consistent, NULL);
  THROW_IF_NULL(jrnl->first_transaction_offset, NULL);

  return jrnl->curr_transaction;
}

#ifdef __cplusplus
}
#endif