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