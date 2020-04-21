#pragma once

#include <stdint.h>
#include <YeltsinDB/types.h>

/**
 * @file constants.h
 * @brief A header with engine constants.
 * @todo Further documentation.
 */

#define YDB_TABLE_FILE_SIGN "TBL!"
#define YDB_TABLE_FILE_SIGN_SIZE (sizeof(YDB_TABLE_FILE_SIGN)-1)
#define YDB_TABLE_FILE_VER_MAJOR_SIZE (1)
#define YDB_TABLE_FILE_VER_MINOR_SIZE (1)
#define YDB_TABLE_FILE_DATA_START_OFFSET (YDB_TABLE_FILE_SIGN_SIZE + YDB_TABLE_FILE_VER_MAJOR_SIZE + \
                                          YDB_TABLE_FILE_VER_MINOR_SIZE)
#define YDB_TABLE_PAGE_SIZE (65536)

#define YDB_TABLE_PAGE_FLAG_DELETED (1)

// TODO static_assert for sizes

enum YDB_v1_sizes {
  YDB_v1_first_page_size = 8,
  YDB_v1_last_page_size = 8,
  YDB_v1_last_free_page_size = 8,
  YDB_v1_page_flags_size = 1,
  YDB_v1_page_next_size = 8,
  YDB_v1_page_prev_size = 8,
  YDB_v1_page_row_count_size = 2,
};

enum YDB_v1_offsets {
  YDB_v1_first_page_offset = YDB_TABLE_FILE_DATA_START_OFFSET,
  YDB_v1_last_page_offset = YDB_v1_first_page_offset + YDB_v1_first_page_size,
  YDB_v1_last_free_page_offset = YDB_v1_last_page_offset + YDB_v1_last_page_size,
  YDB_v1_data_offset = YDB_v1_last_free_page_offset + YDB_v1_last_free_page_size,
};

enum YDB_v1_page_offsets {
  YDB_v1_page_flags_offset = 0,
  YDB_v1_page_next_offset = YDB_v1_page_flags_offset + YDB_v1_page_flags_size,
  YDB_v1_page_prev_offset = YDB_v1_page_next_offset + YDB_v1_page_next_size,
  YDB_v1_page_row_count_offset = YDB_v1_page_prev_offset + YDB_v1_page_prev_size,
  YDB_v1_page_data_offset = YDB_v1_page_row_count_offset + YDB_v1_page_row_count_size,
};

#define YDB_JRNL_SIGN "JRNL"
#define YDB_JRNL_SIGN_SIZE (sizeof(YDB_JRNL_SIGN)-1)

enum YDB_jrnl_sizes {
  YDB_jrnl_first_transaction_size = 8,
  YDB_jrnl_last_transaction_size = 8,
  YDB_jrnl_transaction_prev_size = 8,
  YDB_jrnl_transaction_next_size = 8,
  YDB_jrnl_transaction_timestamp_size = 8,
  YDB_jrnl_transaction_flags_size = 1,
  YDB_jrnl_transaction_op_type_size = 1,
  YDB_jrnl_transaction_op_datasz_size = 4,
};

enum YDB_jrnl_offsets {
  YDB_jrnl_first_transaction_offset = YDB_JRNL_SIGN_SIZE,
  YDB_jrnl_last_transaction_offset = YDB_jrnl_first_transaction_offset + YDB_jrnl_first_transaction_size,
  YDB_jrnl_data_offset = YDB_jrnl_last_transaction_offset + YDB_jrnl_last_transaction_size,
};

enum YDB_jrnl_transaction_offsets {
  YDB_jrnl_transaction_prev_offset = 0,
  YDB_jrnl_transaction_next_offset = YDB_jrnl_transaction_prev_offset + YDB_jrnl_transaction_prev_size,
  YDB_jrnl_transaction_timestamp_offset = YDB_jrnl_transaction_next_offset + YDB_jrnl_transaction_next_size,
  YDB_jrnl_transaction_flags_offset = YDB_jrnl_transaction_timestamp_offset + YDB_jrnl_transaction_timestamp_size,
  YDB_jrnl_transaction_data_offset = YDB_jrnl_transaction_flags_offset + YDB_jrnl_transaction_flags_size,
};

enum YDB_jrnl_transaction_op_offsets {
  YDB_jrnl_transaction_op_type_offset = 0,
  YDB_jrnl_transaction_op_datasz_offset = YDB_jrnl_transaction_op_type_offset + YDB_jrnl_transaction_op_type_size,
  YDB_jrnl_transaction_op_data_offset = YDB_jrnl_transaction_op_datasz_offset + YDB_jrnl_transaction_op_datasz_size,
};

enum YDB_jrnl_transaction_ops {
  YDB_jrnl_op_page_alloc = 0x01,
  YDB_jrnl_op_page_modify = 0x02,
  YDB_jrnl_op_page_remove = 0x03,
  YDB_jrnl_op_rollback = 0xFE,
  YDB_jrnl_op_complete = 0xFF,
};