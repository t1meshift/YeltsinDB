#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <YeltsinDB/error_code.h>
#include <YeltsinDB/constants.h>
#include <YeltsinDB/macro.h>
#include <YeltsinDB/table_page.h>
#include <YeltsinDB/ydb.h>

struct __YDB_Engine {
  uint8_t ver_major; /**< A major version of loaded table. */
  uint8_t ver_minor; /**< A minor version of loaded table. */
  YDB_Offset first_page_offset; /**< A location of the first page in file. */
  YDB_Offset last_page_offset; /**< A location of the last page in file. */
  YDB_Offset last_free_page_offset; /**< A location of last free page in file. */

  YDB_Offset prev_page_offset; /**< A location of previous page in file. */
  YDB_Offset curr_page_offset; /**< A location of current page in file. */
  YDB_Offset next_page_offset; /**< A location of next page in file. */

  YDB_TablePage *curr_page; /**< A pointer to the current page. */

  uint8_t in_use; /**< "In use" flag. */
  char *filename; /**< Current table data file name. */
  FILE *fd; /**< Current table data file descriptor. */
};

YDB_Engine *ydb_init_instance() {
  YDB_Engine *new_instance = calloc(1, sizeof(YDB_Engine));
  return new_instance;
}

void ydb_terminate_instance(YDB_Engine *instance) {
  ydb_unload_table(instance);

  // And after all that, the instance could be freed
  free(instance);
}

// Internal usage only!
// Its only purpose to read current page data and set next_page and prev_page offsets.
static YDB_Error __ydb_read_page(YDB_Engine *inst) {
  THROW_IF_NULL(inst, YDB_ERR_INSTANCE_NOT_INITIALIZED);

  // Seek to current page
  fseek(inst->fd, inst->curr_page_offset, SEEK_SET);

  // Read page data
  char p_data[YDB_TABLE_PAGE_SIZE];
  size_t bytes_read = fread(p_data, 1, YDB_TABLE_PAGE_SIZE, inst->fd);
  // Throw error if failed to read exactly a page size.
  if (bytes_read != YDB_TABLE_PAGE_SIZE) {
    return YDB_ERR_TABLE_DATA_CORRUPTED;
  }

  // Read page flags, next and prev page offset and row count
  YDB_Flags page_flags = p_data[0];
  YDB_Offset next;
  YDB_Offset prev;
  YDB_PageSize row_count;
  memcpy(&next, p_data + YDB_v1_page_next_offset, sizeof(next));
  REASSIGN_FROM_LE(next);
  memcpy(&prev, p_data + YDB_v1_page_prev_offset, sizeof(prev));
  REASSIGN_FROM_LE(prev);
  memcpy(&row_count, p_data + YDB_v1_page_row_count_offset, sizeof(row_count));
  REASSIGN_FROM_LE(row_count);

  const YDB_PageSize meta_size = YDB_v1_page_data_offset;
  const YDB_PageSize data_size = YDB_TABLE_PAGE_SIZE - meta_size;

  YDB_TablePage *p = ydb_page_alloc(data_size);

  ydb_page_flags_set(p, page_flags);
  ydb_page_row_count_set(p, row_count);

  YDB_Error write_status = ydb_page_data_write(p, p_data + meta_size, data_size);
  if (write_status) return write_status;

  inst->curr_page = p;
  inst->prev_page_offset = prev;
  inst->next_page_offset = next;

  return YDB_ERR_SUCCESS;
}
// Moves file position to allocated block.
// Also changes last_free_page_offset.
static YDB_Offset __ydb_allocate_page_and_seek(YDB_Engine *inst) {
  // TODO change signature of the function. It should return an error code, I think.
  YDB_Offset result = 0;
  // If no free pages in the table, then...
  if (inst->last_free_page_offset == 0) {
    // Return the end of the file where a new page will be allocated
    fseek(inst->fd, 0, SEEK_END);
    result = ftell(inst->fd);
    YDB_Offset allocated_page_offset_le = TO_LE(result);

    // Allocate an empty chunk
    char* new_page_data = calloc(1, YDB_TABLE_PAGE_SIZE);
    // Write last page offset as prev page offset in this page
    YDB_Offset last_page_offset_le = TO_LE(inst->last_page_offset);
    memcpy(new_page_data + YDB_v1_page_prev_offset, &last_page_offset_le, sizeof(YDB_Offset));
    // Write it to file
    fwrite(new_page_data, YDB_TABLE_PAGE_SIZE, 1, inst->fd);
    free(new_page_data);
    // TODO throw error on fail

    // Write next page offset in the previous (last) page
    fseek(inst->fd, inst->last_page_offset + YDB_v1_page_next_offset, SEEK_SET);
    fwrite(&allocated_page_offset_le, sizeof(YDB_Offset), 1, inst->fd);

    // Write last page offset
    inst->last_page_offset = result;
    fseek(inst->fd, YDB_v1_last_page_offset, SEEK_SET);
    fwrite(&allocated_page_offset_le, sizeof(YDB_Offset), 1, inst->fd);
    fflush(inst->fd);
  } else {
    // Return last free page offset
    result = inst->last_free_page_offset;

    // Read last free page offset after allocation
    fseek(inst->fd, result + YDB_v1_page_next_offset, SEEK_SET); // Skip flag
    fread(&inst->last_free_page_offset, sizeof(YDB_Offset), 1, inst->fd);
    REASSIGN_FROM_LE(inst->last_free_page_offset);

    // Go back to our almost allocated page
    fseek(inst->fd, result, SEEK_SET);

    // Rewrite page header with last page offset as previous page
    char* page_header = calloc(1, YDB_v1_data_offset);
    YDB_Offset last_page_offset_le = TO_LE(inst->last_page_offset);
    memcpy(page_header + YDB_v1_page_prev_offset, &last_page_offset_le, sizeof(YDB_Offset));
    fwrite(page_header, YDB_v1_data_offset, 1, inst->fd);
    free(page_header);

    // Write last free page offset after allocation in file
    fseek(inst->fd, YDB_v1_last_free_page_offset, SEEK_SET);
    YDB_Offset lfp_offset_le = TO_LE(inst->last_free_page_offset);
    fwrite(&lfp_offset_le, sizeof(YDB_Offset), 1, inst->fd);
  }
  fflush(inst->fd);
  fseek(inst->fd, result, SEEK_SET);
  return result;
}

YDB_Error ydb_load_table(YDB_Engine *instance, const char *path) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(!instance->in_use, YDB_ERR_INSTANCE_IN_USE);

  if (access(path, F_OK) == -1) {
    // TODO: Windows does not check W_OK correctly, use other methods.
    // TODO: if can't read/write, throw other error
    return YDB_ERR_TABLE_NOT_EXIST;
  }
  instance->fd = fopen(path, "rb+");
  THROW_IF_NULL(instance->fd, YDB_ERR_UNKNOWN); // TODO file open error

  // Check file signature
  char signature[YDB_TABLE_FILE_SIGN_SIZE];
  fread(signature, 1, YDB_TABLE_FILE_SIGN_SIZE, instance->fd);
  int signature_match = memcmp(signature, YDB_TABLE_FILE_SIGN, 3) == 0;
  if (!signature_match) {
    return YDB_ERR_TABLE_DATA_CORRUPTED;
  }

  fread(&(instance->ver_major), sizeof(instance->ver_major), 1, instance->fd);
  fread(&(instance->ver_minor), sizeof(instance->ver_minor), 1, instance->fd);

  if (instance->ver_major != 1) { // TODO proper version check!
    return YDB_ERR_TABLE_DATA_VERSION_MISMATCH;
  }

  // Check consistency of a page
  switch (signature[3]) {
    case '!':
      break;
    case '?':
      // TODO mark table as possibly corrupted and start rollback from journal
      // FIXME
      return YDB_ERR_TABLE_DATA_CORRUPTED;
      break;
    default:
      return YDB_ERR_TABLE_DATA_CORRUPTED;
  }

  fread(&instance->first_page_offset, sizeof(YDB_Offset), 1, instance->fd);
  REASSIGN_FROM_LE(instance->first_page_offset);
  fread(&instance->last_page_offset, sizeof(YDB_Offset), 1, instance->fd);
  REASSIGN_FROM_LE(instance->last_page_offset);
  fread(&instance->last_free_page_offset, sizeof(YDB_Offset), 1, instance->fd);
  REASSIGN_FROM_LE(instance->last_free_page_offset);
  // TODO check offsets

  instance->in_use = -1; // unsigned value overflow to fill all the bits
  instance->filename = strdup(path);

  instance->curr_page_offset = instance->first_page_offset;
  return __ydb_read_page(instance);
}

YDB_Error ydb_unload_table(YDB_Engine *instance) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);
  THROW_IF_NULL(instance->curr_page, YDB_ERR_PAGE_NOT_INITIALIZED);

  YDB_Engine *i = instance;

  i->ver_major = 0;
  i->ver_minor = 0;
  i->first_page_offset = 0;
  i->last_page_offset = 0;
  i->last_free_page_offset = 0;
  i->prev_page_offset = 0;
  i->curr_page_offset = 0;
  i->next_page_offset = 0;
  ydb_page_free(i->curr_page);
  i->curr_page = NULL;
  free(i->filename);
  fclose(i->fd);

  // Unset "in use" flag
  instance->in_use = 0;

  return YDB_ERR_SUCCESS;
}

YDB_Error ydb_create_table(YDB_Engine *instance, const char *path) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(!instance->in_use, YDB_ERR_INSTANCE_IN_USE);

  if (access(path, F_OK) != -1) {
    // TODO: Windows does not check W_OK correctly, use other methods.
    // TODO: if can't read/write, throw other error
    return YDB_ERR_TABLE_EXIST;
  }

  FILE *f = fopen(path, "wb");

  char tpl[] = YDB_TABLE_FILE_SIGN                 // File signature
               "\x01\x00"                          // File version FIXME magic
               "\x1E\x00\x00\x00\x00\x00\x00\x00"  // First page offset
               "\x1E\x00\x00\x00\x00\x00\x00\x00"  // Last page offset
               "\x00\x00\x00\x00\x00\x00\x00\x00"; // Last free page offset
  fwrite(tpl, sizeof(tpl)-1, 1, f);

  char *first_page = calloc(1, YDB_TABLE_PAGE_SIZE);
  fwrite(first_page, YDB_TABLE_PAGE_SIZE, 1, f);
  free(first_page);

  fclose(f);

  return ydb_load_table(instance, path);
}

YDB_Error ydb_prev_page(YDB_Engine *instance) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);
  THROW_IF_NULL(instance->prev_page_offset, YDB_ERR_NO_MORE_PAGES);

  instance->curr_page_offset = instance->prev_page_offset;
  return __ydb_read_page(instance);
}

YDB_Error ydb_next_page(YDB_Engine *instance) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);
  THROW_IF_NULL(instance->next_page_offset, YDB_ERR_NO_MORE_PAGES);

  instance->curr_page_offset = instance->next_page_offset;
  return __ydb_read_page(instance);
}

YDB_TablePage* ydb_get_current_page(YDB_Engine *instance) {
  THROW_IF_NULL(instance, NULL);
  THROW_IF_NULL(instance->in_use, NULL);

  return instance->curr_page;
}

YDB_Error ydb_append_page(YDB_Engine* instance, YDB_TablePage* page) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);

  YDB_Offset new_page_offset = __ydb_allocate_page_and_seek(instance);

  YDB_PageSize rc = ydb_page_row_count_get(page);
  YDB_PageSize rc_le = TO_LE(rc);
  YDB_Flags f = ydb_page_flags_get(page);

  char d[YDB_TABLE_PAGE_SIZE - YDB_v1_page_data_offset];
  ydb_page_data_seek(page, 0);
  if (ydb_page_data_read(page, d, sizeof(d))) {
    return YDB_ERR_UNKNOWN; // FIXME
  }

  fwrite(&f, sizeof(f), 1, instance->fd);
  fseek(instance->fd, new_page_offset + YDB_v1_page_row_count_offset, SEEK_SET);
  fwrite(&rc_le, sizeof(rc), 1, instance->fd);
  fwrite(d, sizeof(d), 1, instance->fd);

  fflush(instance->fd);

  __ydb_read_page(instance);

  return YDB_ERR_SUCCESS;
}

YDB_Error ydb_replace_current_page(YDB_Engine *instance, YDB_TablePage *page) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);
  THROW_IF_NULL(page, YDB_ERR_PAGE_NOT_INITIALIZED);

  if (instance->curr_page == page) {
    return YDB_ERR_SAME_PAGE_ADDRESS;
  }

  // Seek to current page in file
  fseek(instance->fd, instance->curr_page_offset, SEEK_SET);

  // Write flags
  YDB_Flags page_flags = ydb_page_flags_get(page);
  fwrite(&page_flags, sizeof(YDB_Flags), 1, instance->fd);

  // Skip next and prev page offsets
  fseek(instance->fd, YDB_v1_page_next_size + YDB_v1_page_prev_size, SEEK_CUR);

  // Write row count
  YDB_PageSize row_cnt = ydb_page_row_count_get(page);
  YDB_PageSize row_cnt_le = TO_LE(row_cnt);
  fwrite(&row_cnt_le, sizeof(row_cnt), 1, instance->fd);

  // Write data
  char page_data[YDB_TABLE_PAGE_SIZE - YDB_v1_page_data_offset];
  ydb_page_data_seek(page, 0);
  if (ydb_page_data_read(page, page_data, sizeof(page_data))) {
    return YDB_ERR_UNKNOWN; // FIXME
  }
  fwrite(page_data, sizeof(page_data), 1, instance->fd);

  // Flush buffer
  fflush(instance->fd);

  ydb_page_free(instance->curr_page);
  instance->curr_page = page;

  return YDB_ERR_SUCCESS;
}

YDB_Error ydb_delete_current_page(YDB_Engine *instance) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);

  if (instance->prev_page_offset == 0 && instance->next_page_offset == 0) {
    // Just clear page header. That's all.
    fseek(instance->fd, instance->curr_page_offset, SEEK_SET);
    char* empty_header = calloc(1, YDB_v1_page_data_offset);
    fwrite(empty_header, YDB_v1_page_data_offset, 1, instance->fd);
    free(empty_header);
    return YDB_ERR_SUCCESS;
  }

  // Mark page as deleted
  fseek(instance->fd, instance->curr_page_offset, SEEK_SET);
  fputc(YDB_TABLE_PAGE_FLAG_DELETED, instance->fd);

  // Write last_free_page_offset as the next page for current one
  YDB_Offset lfp_le = TO_LE(instance->last_free_page_offset);
  fwrite(&lfp_le, sizeof(YDB_Offset), 1, instance->fd);

  // Link the previous page with next one (could be null ptr)
  if (instance->prev_page_offset != 0) {
    fseek(instance->fd, instance->prev_page_offset + YDB_v1_page_next_offset, SEEK_SET);
  } else {
    // If it was the first page, rewrite first_page_offset with next_page_offset
    fseek(instance->fd, YDB_v1_first_page_offset, SEEK_SET);
  }
  YDB_Offset np_le = TO_LE(instance->next_page_offset);
  fwrite(&np_le, sizeof(YDB_Offset), 1, instance->fd);

  // Link the next page with previous one (could be null ptr)
  if (instance->next_page_offset != 0) {
    fseek(instance->fd, instance->next_page_offset + YDB_v1_page_prev_offset, SEEK_SET);
  } else {
    // If it was the last page, rewrite last_page_offset with prev_page_offset
    fseek(instance->fd, YDB_v1_last_page_offset, SEEK_SET);
  }
  YDB_Offset pp_le = TO_LE(instance->prev_page_offset);
  fwrite(&pp_le, sizeof(YDB_Offset), 1, instance->fd);

  // Rewrite last_free_page_offset with current offset
  fseek(instance->fd, YDB_v1_last_free_page_offset, SEEK_SET);
  YDB_Offset cp_le = TO_LE(instance->curr_page_offset);
  fwrite(&cp_le, sizeof(YDB_Offset), 1, instance->fd);

  // Flush buffer
  fflush(instance->fd);

  // Seek to the next page if it's not the last, else seek to the previous one
  if (instance->next_page_offset != 0) {
    instance->curr_page_offset = instance->next_page_offset;
  } else {
    instance->curr_page_offset = instance->prev_page_offset;
  }
  return __ydb_read_page(instance);
}

YDB_Error ydb_seek_to_begin(YDB_Engine *instance) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);

  if (instance->prev_page_offset == 0)
    return YDB_ERR_SUCCESS;

  instance->curr_page_offset = instance->prev_page_offset;
  return __ydb_read_page(instance);
}

YDB_Error ydb_seek_to_end(YDB_Engine *instance) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);

  if (instance->next_page_offset == 0)
    return YDB_ERR_SUCCESS;

  instance->curr_page_offset = instance->next_page_offset;
  return __ydb_read_page(instance);
}

#ifdef __cplusplus
}
#endif