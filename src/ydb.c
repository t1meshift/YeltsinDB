#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <YeltsinDB/error_code.h>
#include <YeltsinDB/constants.h>
#include <YeltsinDB/macro.h>
#include <YeltsinDB/table_page.h>
#include <YeltsinDB/ydb.h>

/** @todo These things:
 * - Page data (current, maybe cache next and prev too)
 * - MAYBE query caching
 */
struct __YDB_Engine {
  uint8_t ver_major; /**< A major version of loaded table. */
  uint8_t ver_minor; /**< A minor version of loaded table. */
  YDB_Offset first_page_offset; /**< A location of the first page in file. */
  YDB_Offset last_page_offset; /**< A location of the last page in file. */
  YDB_Offset last_free_page_offset; /**< A location of last free page in file. */

  YDB_Offset prev_page_offset; /**< A location of previous page in file. */
  YDB_Offset curr_page_offset; /**< A location of current page in file. */
  YDB_Offset next_page_offset; /**< A location of next page in file. */

  int64_t current_page_index; /**< Current page index in range [0, page_count). */

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
// Its only purpose to read current page data and set next_page offset.
// You should write prev_page offset on your own.
inline YDB_Error __ydb_read_page(YDB_Engine *inst) {
  THROW_IF_NULL(inst, YDB_ERR_INSTANCE_NOT_INITIALIZED);

  fseek(inst->fd, inst->curr_page_offset, SEEK_SET);

  char p_data[YDB_TABLE_PAGE_SIZE];
  size_t bytes_read = fread(p_data, 1, YDB_TABLE_PAGE_SIZE, inst->fd);
  if (bytes_read != YDB_TABLE_PAGE_SIZE) {
    return YDB_ERR_TABLE_DATA_CORRUPTED;
  }

  YDB_Flags page_flags = p_data[0];
  YDB_Offset next;
  YDB_PageSize row_count;
  memcpy(&next, p_data + sizeof(page_flags), sizeof(next));
  memcpy(&row_count, p_data + sizeof(page_flags) + sizeof(next), sizeof(row_count));

  //TODO move it to consts. Also page meta size could differ in different major versions.
  const YDB_PageSize meta_size = sizeof(page_flags) + sizeof(next) + sizeof(row_count);
  const YDB_PageSize data_size = YDB_TABLE_PAGE_SIZE - meta_size;

  YDB_TablePage *p = ydb_page_alloc(data_size);

  ydb_page_flags_set(p, page_flags);
  ydb_page_row_count_set(p, row_count);

  YDB_Error write_status = ydb_page_data_write(p, p_data + meta_size, data_size);
  if (write_status) return write_status;

  inst->curr_page = p;
  inst->next_page_offset = next;

  return YDB_ERR_SUCCESS;
}
// Moves file position to allocated block.
// Also changes last_free_page_offset.
inline YDB_Offset __ydb_allocate_page_and_seek(YDB_Engine *inst) {
  YDB_Offset result = 0;
  // If no free pages in the table, then...
  if (inst->last_free_page_offset == 0) {
    // Return the end of the file where a new page will be allocated
    fseek(inst->fd, 0, SEEK_END);
    result = ftell(inst->fd);

    // Write next page offset in the previous page
    fseek(inst->fd, inst->last_page_offset + 1, SEEK_SET); // Skip page flag
    fwrite(&result, sizeof(result), 1, inst->fd);

    // Allocate an empty chunk
    fseek(inst->fd, 0, SEEK_END);
    char* new_page_data = calloc(1, YDB_TABLE_PAGE_SIZE);
    fwrite(new_page_data, YDB_TABLE_PAGE_SIZE, 1, inst->fd);
    free(new_page_data);

    // Write last page offset
    inst->last_page_offset = result;
    fseek(inst->fd, 14, SEEK_SET); // TODO move magic offsets somewhere
    fwrite(&inst->last_page_offset, sizeof(YDB_Offset), 1, inst->fd);
    fflush(inst->fd);
  } else {
    // Return last free page offset
    result = inst->last_free_page_offset;

    // Read last free page offset after allocation
    fseek(inst->fd, result + 1, SEEK_SET); // Skip flag
    fread(&(inst->last_free_page_offset), sizeof(YDB_Offset), 1, inst->fd);
    fseek(inst->fd, -(long)(sizeof(YDB_Offset) + 1), SEEK_CUR);

    // Rewrite page header
    char page_header[] = "\x00" // No flags
        "\x00\x00\x00\x00\x00\x00\x00\x00" // No next page
        "\x00\x00"; // No rows
    fwrite(page_header, sizeof(page_header)-1, 1, inst->fd);

    // Write last free page offset after allocation in file
    fseek(inst->fd, 22, SEEK_SET); // Skip unneeded data TODO beautify it
    fwrite(&(inst->last_free_page_offset), sizeof(YDB_Offset), 1, inst->fd);
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

  char signature[4];
  fread(signature, 1, 4, instance->fd);
  int signature_match = memcmp(signature, YDB_TABLE_FILE_SIGN, 4) == 0;
  if (!signature_match) {
    return YDB_ERR_TABLE_DATA_CORRUPTED;
  }

  fread(&(instance->ver_major), sizeof(instance->ver_major), 1, instance->fd);
  fread(&(instance->ver_minor), sizeof(instance->ver_minor), 1, instance->fd);

  if (instance->ver_major != 0) { // TODO proper version check
    return YDB_ERR_TABLE_DATA_VERSION_MISMATCH;
  }

  fread(&(instance->first_page_offset), sizeof(YDB_Offset), 1, instance->fd);
  fread(&(instance->last_page_offset), sizeof(YDB_Offset), 1, instance->fd);
  fread(&(instance->last_free_page_offset), sizeof(YDB_Offset), 1, instance->fd);
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
  i->current_page_index = 0;
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

  if (access(instance->filename, F_OK) != -1) {
    // TODO: Windows does not check W_OK correctly, use other methods.
    // TODO: if can't read/write, throw other error
    return YDB_ERR_TABLE_EXIST;
  }

  FILE *f = fopen(path, "wb");

  char tpl[] = YDB_TABLE_FILE_SIGN
               "\x00\x01"
               "\x00\x00\x00\x00\x00\x00\x00\x1E"
               "\x00\x00\x00\x00\x00\x00\x00\x1E"
               "\x00\x00\x00\x00\x00\x00\x00\x00";
  fwrite(tpl, sizeof(tpl)-1, 1, f);

  char *first_page = calloc(1, YDB_TABLE_PAGE_SIZE);
  fwrite(first_page, YDB_TABLE_PAGE_SIZE, 1, f);
  free(first_page);

  fclose(f);

  return ydb_load_table(instance, path);
}

int64_t ydb_tell_page(YDB_Engine *instance) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);
  return instance->current_page_index;
}

YDB_Error ydb_next_page(YDB_Engine *instance) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);
  THROW_IF_NULL(instance->next_page_offset, YDB_ERR_NO_MORE_PAGES);

  instance->prev_page_offset = instance->curr_page_offset;
  instance->curr_page_offset = instance->next_page_offset;
  ++(instance->current_page_index);
  return __ydb_read_page(instance);
}

YDB_Error ydb_seek_page(YDB_Engine *instance, int64_t index) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);
  if (index < 0) {
    return YDB_ERR_PAGE_INDEX_OUT_OF_RANGE;
  }

  // Save current state
  YDB_Offset prev_page_offset = instance->prev_page_offset;
  YDB_Offset curr_page_offset = instance->curr_page_offset;
  YDB_Offset next_page_offset = instance->next_page_offset;
  int64_t curr_page_index = instance->current_page_index;

  if (index < instance->current_page_index) {
    instance->prev_page_offset = 0;
    instance->curr_page_offset = instance->first_page_offset;
    instance->current_page_index = 0;
    __ydb_read_page(instance);
  }

  while (instance->current_page_index != index) {
    YDB_Error status = ydb_next_page(instance);

    if (status != YDB_ERR_SUCCESS) {
      instance->prev_page_offset = prev_page_offset;
      instance->curr_page_offset = curr_page_offset;
      instance->next_page_offset = next_page_offset;
      instance->current_page_index = curr_page_index;
      __ydb_read_page(instance);
      return status;
    }
  }

  return YDB_ERR_SUCCESS;
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
  YDB_Flags f = ydb_page_flags_get(page);
  YDB_Offset next = 0;
  char d[YDB_TABLE_PAGE_SIZE - 11]; // TODO move magic numbers somewhere
  if (ydb_page_data_read(page, d, sizeof(d))) {
    return YDB_ERR_UNKNOWN; // FIXME
  }

  fwrite(&f, sizeof(f), 1, instance->fd);
  fwrite(&next, sizeof(next), 1, instance->fd);
  fwrite(&rc, sizeof(rc), 1, instance->fd);
  fwrite(d, sizeof(d), 1, instance->fd);

  fflush(instance->fd);

  return YDB_ERR_SUCCESS;
}

YDB_Error ydb_replace_current_page(YDB_Engine *instance, YDB_TablePage *page) {
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);
  THROW_IF_NULL(page, YDB_ERR_PAGE_NOT_INITIALIZED);

  // Seek to current page in file
  fseek(instance->fd, instance->curr_page_offset, SEEK_SET);

  // Write flags
  YDB_Flags page_flags = ydb_page_flags_get(page);
  fwrite(&page_flags, sizeof(YDB_Flags), 1, instance->fd);

  // Skip next page offset
  fseek(instance->fd, sizeof(YDB_Offset), SEEK_CUR);

  // Write row count
  YDB_PageSize row_cnt = ydb_page_row_count_get(page);
  fwrite(&row_cnt, sizeof(row_cnt), 1, instance->fd);

  // Write data
  char page_data[YDB_TABLE_PAGE_SIZE - 11]; // TODO move magic numbers somewhere
  if (ydb_page_data_read(page, page_data, sizeof(page_data))) {
    return YDB_ERR_UNKNOWN; // FIXME
  }
  fwrite(page_data, sizeof(page_data), 1, instance->fd);

  // Flush buffer
  fflush(instance->fd);

  return YDB_ERR_SUCCESS;
}

YDB_Error ydb_delete_current_page(YDB_Engine *instance) {
  // TODO check if it's only page in table
  THROW_IF_NULL(instance, YDB_ERR_INSTANCE_NOT_INITIALIZED);
  THROW_IF_NULL(instance->in_use, YDB_ERR_INSTANCE_NOT_IN_USE);

  // Mark page as deleted
  fseek(instance->fd, instance->curr_page_offset, SEEK_SET);
  fputc(YDB_TABLE_PAGE_FLAG_DELETED, instance->fd);

  // Write last_free_page_offset as the next page for current one
  fwrite(&(instance->last_free_page_offset), sizeof(YDB_Offset), 1, instance->fd);

  // Link the previous page with next one (could be null ptr)
  fseek(instance->fd, instance->prev_page_offset + 1, SEEK_SET); // Skip flags
  fwrite(&(instance->next_page_offset), sizeof(YDB_Offset), 1, instance->fd);

  // If it was the last page, rewrite last_page_offset with prev_page_offset
  if (instance->next_page_offset == 0) {
    fseek(instance->fd, 14, SEEK_SET);
    fwrite(&(instance->prev_page_offset), sizeof(YDB_Offset), 1, instance->fd);
  }

  // Rewrite last_free_page_offset with current offset
  fseek(instance->fd, 22, SEEK_SET);
  fwrite(&(instance->curr_page_offset), sizeof(YDB_Offset), 1, instance->fd);

  // Flush buffer
  fflush(instance->fd);

  // Set previous page
  int64_t ind = --(instance->current_page_index);
  if (ydb_seek_page(instance, ind + 1)) {
    return YDB_ERR_UNKNOWN;
  }

  return YDB_ERR_SUCCESS;
}
