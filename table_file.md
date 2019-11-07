# Table file structure

## Table file version changelog

### v0.2
+ Added table "write incomplete" flag.

### v0.1
Initial version.

## Overall specification

1. `TBL!` file signature (4 bytes) **could be `TBL?` if an operation on a table is incompleted**
2. Table file version (2 bytes)
3. The offset to the first page in a table. (8 bytes)
4. The offset to the last page in a table. (8 bytes)
5. The offset to the last available *free* page (8 bytes)
6. Pages (64 KiB each)
    1. Page flags (1 byte)
    2. Next page offset (8 bytes) **could be 0 if last page**
    3. Row count (2 bytes)
    4. Rows
        1. Row flags (1 byte)
        2. Row data

## Table file version specification

In the further time, it's planned to rewrite the file structure, so it's useful to have a version marker.

|      7-4      |      3-0      |
|---------------|---------------|
| Major version | Minor version |

That means that the first byte defines major version, and the second one defines minor version.

If tables with different versions are binary-compatible with each other, their major version must be the same. 

## Page flags specification

|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
|-----|-----|-----|-----|-----|-----|-----|-----|
| RSV | RSV | RSV | RSV | RSV | RSV | RSV | DEL |

- **RSV** -- reserved for further usage.
- **DEL** -- free page flag. 

## Row flags specification

See "Page flags specification" above.

## Free pages

A page can be called *free* iff all its rows are deleted. 
If there is a free page, there actions are being done:

1. Set `DEL` page flag ((6.3) |= FLAG_DEL)
2. If a page is **not** the last one, replace next page offset in the previous page with a value in current page 
((previous 6.1) = (6.1))
3. Put last available free page as the next page ((6.1) = 5)
4. Set current page offset as the offset to the last available free page ((5) = current_page_offset)

## File signature

*Since v0.2* a file signature could be `TBL?`, which signals for incomplete table write operation.
If that signature is detected, the state of a table should be reverted to that it was before failed
transaction.