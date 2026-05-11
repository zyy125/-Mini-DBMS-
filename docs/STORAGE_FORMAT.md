# Storage Format

Phase 4 uses one filesystem directory per database and two files per table. All long-term files are serialized field by field in little-endian order. The storage engine does not `fwrite` ordinary C++ structs and does not rely on struct padding or native alignment.

## Directory Layout

```text
data/
└── <database>/
    ├── <table>.meta
    ├── <table>.dat
    └── <table>.idx   # reserved for Phase 5 B+ tree indexes
```

Database, table, and column names use the same lowercase alphabetic identifier rule as the parser.

## Metadata File

`<table>.meta` stores schema information:

```text
bytes[8]  magic = "MDBMETA1"
uint32    format_version = 1
uint32    column_count
int32     primary_column_index, or -1 when no primary key exists

repeated column_count times:
  uint32  column_name_length
  bytes   column_name UTF-8 bytes
  uint8   column_type: 1 = int, 2 = string
  uint8   primary: 0 = false, 1 = true
```

String fields in metadata are length-prefixed. No packed structs are used.

## Data File

`<table>.dat` stores row records:

```text
bytes[8]  magic = "MDBDATA1"
uint32    format_version = 1

repeated until EOF:
  uint8   flags: 1 = live, 0 = deleted tombstone
  uint32  payload_length
  bytes   row_payload
```

`row_payload` serializes values in schema column order:

```text
int       int32 little-endian
string    uint32 byte_length followed by UTF-8 bytes, max 256 bytes
```

Deleted rows keep their payload bytes in place and are skipped by table scans.

## RowId / RecordId

`RowId` and `RecordId` are the same type in Phase 4:

```text
RowId {
  uint64 offset
}
```

`offset` is the byte offset of the row record header in `<table>.dat`, starting at the `flags` byte. It is stable while the row remains live and can be stored by the B+ tree index. `UINT64_MAX` means invalid.

Updates that fit inside the existing payload rewrite in place and preserve the same `RowId`. Updates with a larger payload mark the old row deleted and append a new row, returning the new `RowId` with `moved = true`; Phase 5/6 index and executor code must update index entries in that case.

## Index Reservation

`<table>.idx` is reserved for Phase 5. `drop table` removes it if present, but Phase 4 does not create or read it.
