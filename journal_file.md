# Journal file structure

## Specification

1. `JRNL` file signature (4 bytes)
2. The offset to the first transaction (8 bytes) **(could be 0 if no transactions)**
3. The offset to the last transaction (8 bytes) **(could be 0 if no transactions)**
4. Transactions
    1. Previous transaction offset (8 bytes) **(could be 0 if the first one)**
    2. Next transaction offset (8 bytes) **(could be 0 if the last one)**
    3. Creation date in Unix timestamp format (8 bytes)
    4. Transaction flags (1 byte)
    5. Operations list
        1. Transaction type (1 byte)
        2. Transaction data size (4 bytes)
        3. Transaction data
        
## Transactions

### Flags

**TODO**. Now there is the only option -- `JRNL_COMPLETE` on bit 0.
It means that the transaction was completely written to the storage.

### Consistency

If the last transaction was not written completely, journal rollback is started.
That means that it will be cleaned since it has not even been written to storage.

If the incomplete transaction is the only one in journal, the journal file is truncated 
to 20 bytes and first and last transaction offsets are being nulled.

If the last transaction is complete in journal but not in storage (flag `JRNL_COMPLETE` is 
not set), redo is being done. That means the transaction starts again since the journal is 
consistent. 

### Operations

There are several operation types:

1. Page allocation (`0x01`)
2. Page modification (`0x02`)
3. Page removal (`0x03`)
4. Transaction rollback (`0xFE`)
5. Transaction completion (`0xFF`)

#### Page allocation

This operation contains this data:

1. Allocated page offset (8 bytes)
2. Last free page offset before operation (8 bytes)
3. Last free page offset after operation (8 bytes)

24 bytes in summary.

#### Page modification

This operation contains this data:

1. Modified page offset (8 bytes)
2. Modified page data offset (2 bytes)
3. Modified page data size (2 bytes)
4. 'Before' data image
5. 'After' data image

12 bytes on header + 2 * (`par. 3`) bytes in summary. 
Can't be more than 131084.

#### Page removal

This operation contains this data:

1. Removed page offset (8 bytes)
2. Last free page offset before operation (8 bytes)

16 bytes in summary.

#### Transaction rollback/completion

This operation *may* contain data. By default it does not. 