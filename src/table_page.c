#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <string.h>
#include <YeltsinDB/constants.h>
#include <YeltsinDB/table_page.h>
#include <YeltsinDB/error_code.h>
#include <YeltsinDB/macro.h>

/**
 * @struct __YDB_TablePage
 * @brief A struct that defines a page of a table.
 */

struct __YDB_TablePage {
  char* data; /**< Raw page data. */
  YDB_PageSize size; /**< The amount of memory in a page. */
  YDB_PageSize pos; /**< The current position of page data I/O stream. TODO: rewrite it */
  YDB_PageSize row_count; /**< Row count in a page. */
  YDB_Flags flags; /**< The flags of a page. */
};

YDB_TablePage* ydb_page_alloc(YDB_PageSize size) {
  YDB_TablePage* new_page = malloc(sizeof(YDB_TablePage));
  new_page->pos = 0;
  new_page->flags = 0; // No flags set
  new_page->size = size;
  new_page->data = calloc(1, new_page->size);
  return new_page;
}

void ydb_page_free(YDB_TablePage* page) {
  free(page->data);
  free(page);
}

YDB_Error ydb_page_data_seek(YDB_TablePage *page, YDB_PageSize pos) {
  THROW_IF_NULL(page, YDB_ERR_PAGE_NOT_INITIALIZED);

  if ((pos < 0) || (pos >= page->size)) {
    return YDB_ERR_PAGE_INDEX_OUT_OF_RANGE;
  }
  page->pos = pos;

  return YDB_ERR_SUCCESS;
}

YDB_PageSize ydb_page_data_tell(YDB_TablePage *page) {
  THROW_IF_NULL(page, YDB_ERR_PAGE_NOT_INITIALIZED);
  return page->pos;
}

YDB_Error ydb_page_data_read(YDB_TablePage *src, void *dst, YDB_PageSize n) {
  THROW_IF_NULL(src, YDB_ERR_PAGE_NOT_INITIALIZED);
  THROW_IF_NULL(dst, YDB_ERR_WRITE_TO_NULLPTR);
  THROW_IF_NULL(n, YDB_ERR_ZERO_SIZE_RW);

  if (src->pos == src->size) {
    return YDB_ERR_PAGE_INDEX_OUT_OF_RANGE;
  }

  // Trait as int64_t to prevent overflow.
  int64_t new_pos = src->pos + n;
  if (new_pos > (int64_t) src->size) {
    return YDB_ERR_PAGE_INDEX_OUT_OF_RANGE;
  }

  void* data_start = src->data + src->pos;
  memcpy(dst, data_start, n);
  src->pos += n;

  return YDB_ERR_SUCCESS;
}

YDB_Error ydb_page_data_write(YDB_TablePage *dst, const void *src, YDB_PageSize n) {
  THROW_IF_NULL(src, YDB_ERR_WRITE_TO_NULLPTR);
  THROW_IF_NULL(dst, YDB_ERR_PAGE_NOT_INITIALIZED);
  THROW_IF_NULL(n, YDB_ERR_ZERO_SIZE_RW);

  if (dst->pos == dst->size) {
    return YDB_ERR_PAGE_NO_MORE_MEM;
  }

  // Trait as int64_t to prevent overflow.
  int64_t new_pos = dst->pos + n;
  if (new_pos > (int64_t) dst->size) {
    return YDB_ERR_PAGE_INDEX_OUT_OF_RANGE;
  }

  void* data_start = dst->data + dst->pos;
  memcpy(data_start, src, n);
  dst->pos += n;

  return YDB_ERR_SUCCESS;
}

YDB_Flags ydb_page_flags_get(YDB_TablePage *page) {
  THROW_IF_NULL(page, 0);
  return page->flags;
}

void ydb_page_flags_set(YDB_TablePage *page, YDB_Flags flags) {
  if (!page) return;
  page->flags = flags;
}

YDB_PageSize ydb_page_row_count_get(YDB_TablePage *page) {
  THROW_IF_NULL(page, 0);
  return page->row_count;
}

void ydb_page_row_count_set(YDB_TablePage *page, YDB_PageSize row_count) {
  if (!page) return;
  page->row_count = row_count;
}

YDB_TablePage *ydb_page_clone(const YDB_TablePage *page) {
  YDB_TablePage* result = malloc(sizeof(YDB_TablePage));
  memcpy(result, page, sizeof(YDB_TablePage));
  result->data = malloc(result->size);
  memcpy(result->data, page->data, result->size);
  return result;
}

#ifdef __cplusplus
}
#endif

