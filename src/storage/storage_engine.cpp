#include "storage/storage_engine.h"

#include <fstream>
#include <sstream>
#include <utility>

#include "common/string_utils.h"

namespace mini_dbms::storage {
namespace {

using common::Status;

constexpr char kMetaMagic[] = "MDBMETA1";
constexpr char kDataMagic[] = "MDBDATA1";
constexpr std::uint32_t kFormatVersion = 1;
constexpr std::uint8_t kRowLive = 1;
constexpr std::uint8_t kRowDeleted = 0;

Status IoError(const std::string& action, const std::filesystem::path& path) {
    return Status::IOError(action + ": " + path.string());
}

bool ReadExact(std::istream& input, char* data, std::size_t size) {
    input.read(data, static_cast<std::streamsize>(size));
    return input.good() || (size == 0 && !input.bad());
}

Status WriteBytes(std::ostream& output, const char* data, std::size_t size) {
    output.write(data, static_cast<std::streamsize>(size));
    if (!output.good()) {
        return Status::IOError("Failed to write file");
    }
    return Status::OK();
}

Status WriteUInt8(std::ostream& output, std::uint8_t value) {
    char byte = static_cast<char>(value);
    return WriteBytes(output, &byte, 1);
}

Status ReadUInt8(std::istream& input, std::uint8_t* value) {
    char byte = 0;
    if (!ReadExact(input, &byte, 1)) {
        return Status::IOError("Failed to read uint8");
    }
    *value = static_cast<std::uint8_t>(byte);
    return Status::OK();
}

Status WriteUInt32(std::ostream& output, std::uint32_t value) {
    char bytes[4];
    for (int i = 0; i < 4; ++i) {
        bytes[i] = static_cast<char>((value >> (i * 8)) & 0xffu);
    }
    return WriteBytes(output, bytes, 4);
}

Status ReadUInt32(std::istream& input, std::uint32_t* value) {
    char bytes[4];
    if (!ReadExact(input, bytes, 4)) {
        return Status::IOError("Failed to read uint32");
    }
    std::uint32_t result = 0;
    for (int i = 0; i < 4; ++i) {
        result |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[i])) << (i * 8);
    }
    *value = result;
    return Status::OK();
}

Status WriteInt32(std::ostream& output, int value) {
    return WriteUInt32(output, static_cast<std::uint32_t>(value));
}

Status ReadInt32(std::istream& input, int* value) {
    std::uint32_t raw = 0;
    Status status = ReadUInt32(input, &raw);
    if (!status.ok()) {
        return status;
    }
    *value = static_cast<int>(raw);
    return Status::OK();
}

Status WriteString(std::ostream& output, const std::string& value, std::uint32_t max_bytes) {
    if (value.size() > max_bytes) {
        return Status::InvalidArgument("String exceeds maximum length");
    }
    Status status = WriteUInt32(output, static_cast<std::uint32_t>(value.size()));
    if (!status.ok()) {
        return status;
    }
    return WriteBytes(output, value.data(), value.size());
}

Status ReadString(std::istream& input, std::uint32_t max_bytes, std::string* value) {
    std::uint32_t length = 0;
    Status status = ReadUInt32(input, &length);
    if (!status.ok()) {
        return status;
    }
    if (length > max_bytes) {
        return Status::InvalidArgument("Stored string length exceeds maximum");
    }
    value->assign(length, '\0');
    if (length == 0) {
        return Status::OK();
    }
    if (!ReadExact(input, value->data(), length)) {
        return Status::IOError("Failed to read string payload");
    }
    return Status::OK();
}

std::uint64_t CurrentOffset(std::istream& input) {
    return static_cast<std::uint64_t>(input.tellg());
}

std::uint64_t CurrentOffset(std::ostream& output) {
    return static_cast<std::uint64_t>(output.tellp());
}

Status WriteFileHeader(std::ostream& output, const char* magic) {
    Status status = WriteBytes(output, magic, 8);
    if (!status.ok()) {
        return status;
    }
    return WriteUInt32(output, kFormatVersion);
}

Status ReadFileHeader(std::istream& input, const char* expected_magic) {
    char magic[8];
    if (!ReadExact(input, magic, 8)) {
        return Status::IOError("Failed to read file magic");
    }
    for (int i = 0; i < 8; ++i) {
        if (magic[i] != expected_magic[i]) {
            return Status::InvalidArgument("Invalid file magic");
        }
    }
    std::uint32_t version = 0;
    Status status = ReadUInt32(input, &version);
    if (!status.ok()) {
        return status;
    }
    if (version != kFormatVersion) {
        return Status::InvalidArgument("Unsupported storage file version");
    }
    return Status::OK();
}

Status WriteValue(std::ostream& output, const Value& value) {
    if (value.type == ColumnType::kInt) {
        return WriteInt32(output, value.int_value);
    }
    return WriteString(output, value.string_value, kMaxStringBytes);
}

Status ReadValue(std::istream& input, ColumnType type, Value* value) {
    value->type = type;
    if (type == ColumnType::kInt) {
        return ReadInt32(input, &value->int_value);
    }
    return ReadString(input, kMaxStringBytes, &value->string_value);
}

Status WriteRowPayload(std::ostream& output, const Row& row) {
    for (std::size_t i = 0; i < row.values.size(); ++i) {
        Status status = WriteValue(output, row.values[i]);
        if (!status.ok()) {
            return status;
        }
    }
    return Status::OK();
}

Status ReadRowPayload(std::istream& input, const Schema& schema, Row* row) {
    row->values.Clear();
    for (std::size_t i = 0; i < schema.columns.size(); ++i) {
        Value value;
        Status status = ReadValue(input, schema.columns[i].type, &value);
        if (!status.ok()) {
            return status;
        }
        status = row->values.PushBack(std::move(value));
        if (!status.ok()) {
            return status;
        }
    }
    return Status::OK();
}

Status SerializeRowToString(const Row& row, std::string* payload) {
    std::ostringstream output;
    Status status = WriteRowPayload(output, row);
    if (!status.ok()) {
        return status;
    }
    *payload = output.str();
    return Status::OK();
}

Status WriteSchemaFile(const std::filesystem::path& path, const Schema& schema) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return IoError("Failed to open metadata file for write", path);
    }
    Status status = WriteFileHeader(output, kMetaMagic);
    if (!status.ok()) {
        return status;
    }
    status = WriteUInt32(output, static_cast<std::uint32_t>(schema.columns.size()));
    if (!status.ok()) {
        return status;
    }
    status = WriteInt32(output, schema.primary_column_index);
    if (!status.ok()) {
        return status;
    }
    for (std::size_t i = 0; i < schema.columns.size(); ++i) {
        const Column& column = schema.columns[i];
        status = WriteString(output, column.name, 255);
        if (!status.ok()) {
            return status;
        }
        status = WriteUInt8(output, column.type == ColumnType::kInt ? 1 : 2);
        if (!status.ok()) {
            return status;
        }
        status = WriteUInt8(output, column.primary ? 1 : 0);
        if (!status.ok()) {
            return status;
        }
    }
    return output.good() ? Status::OK() : IoError("Failed to write metadata file", path);
}

SchemaResult ReadSchemaFile(const std::filesystem::path& path) {
    SchemaResult result;
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        result.status = IoError("Failed to open metadata file for read", path);
        return result;
    }
    result.status = ReadFileHeader(input, kMetaMagic);
    if (!result.status.ok()) {
        return result;
    }
    std::uint32_t column_count = 0;
    result.status = ReadUInt32(input, &column_count);
    if (!result.status.ok()) {
        return result;
    }
    if (column_count == 0) {
        result.status = Status::InvalidArgument("Metadata has no columns");
        return result;
    }
    int primary_index = -1;
    result.status = ReadInt32(input, &primary_index);
    if (!result.status.ok()) {
        return result;
    }
    result.schema.primary_column_index = primary_index;
    for (std::uint32_t i = 0; i < column_count; ++i) {
        Column column;
        result.status = ReadString(input, 255, &column.name);
        if (!result.status.ok()) {
            return result;
        }
        std::uint8_t type = 0;
        result.status = ReadUInt8(input, &type);
        if (!result.status.ok()) {
            return result;
        }
        if (type != 1 && type != 2) {
            result.status = Status::InvalidArgument("Invalid column type in metadata");
            return result;
        }
        column.type = type == 1 ? ColumnType::kInt : ColumnType::kString;
        std::uint8_t primary = 0;
        result.status = ReadUInt8(input, &primary);
        if (!result.status.ok()) {
            return result;
        }
        column.primary = primary != 0;
        result.status = result.schema.columns.PushBack(std::move(column));
        if (!result.status.ok()) {
            return result;
        }
    }
    result.status = Status::OK();
    return result;
}

Status EnsureDataFile(const std::filesystem::path& path) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return IoError("Failed to create data file", path);
    }
    return WriteFileHeader(output, kDataMagic);
}

Status ReadRowRecord(std::istream& input,
                     const Schema& schema,
                     std::uint8_t* flags,
                     std::uint32_t* payload_length,
                     Row* row) {
    Status status = ReadUInt8(input, flags);
    if (!status.ok()) {
        return status;
    }
    status = ReadUInt32(input, payload_length);
    if (!status.ok()) {
        return status;
    }
    std::streampos payload_start = input.tellg();
    if (*flags == kRowLive) {
        status = ReadRowPayload(input, schema, row);
        if (!status.ok()) {
            return status;
        }
    }
    input.seekg(payload_start + static_cast<std::streamoff>(*payload_length));
    if (!input.good()) {
        return Status::IOError("Failed to skip row payload");
    }
    return Status::OK();
}

Status AppendRowRecord(const std::filesystem::path& path, const Row& row, RowId* row_id) {
    std::string payload;
    Status status = SerializeRowToString(row, &payload);
    if (!status.ok()) {
        return status;
    }
    std::ofstream output(path, std::ios::binary | std::ios::app);
    if (!output.is_open()) {
        return IoError("Failed to open data file for append", path);
    }
    row_id->offset = CurrentOffset(output);
    status = WriteUInt8(output, kRowLive);
    if (!status.ok()) {
        return status;
    }
    status = WriteUInt32(output, static_cast<std::uint32_t>(payload.size()));
    if (!status.ok()) {
        return status;
    }
    status = WriteBytes(output, payload.data(), payload.size());
    if (!status.ok()) {
        return status;
    }
    return output.good() ? Status::OK() : IoError("Failed to append row", path);
}

Status MarkDeleted(const std::filesystem::path& path, RowId row_id) {
    std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
        return IoError("Failed to open data file for delete", path);
    }
    file.seekp(static_cast<std::streamoff>(row_id.offset));
    if (!file.good()) {
        return Status::InvalidArgument("RowId offset is outside data file");
    }
    Status status = WriteUInt8(file, kRowDeleted);
    if (!status.ok()) {
        return status;
    }
    return file.good() ? Status::OK() : IoError("Failed to mark row deleted", path);
}

}  // namespace

const Column* Schema::FindColumn(const std::string& name) const {
    int index = FindColumnIndex(name);
    return index < 0 ? nullptr : &columns[static_cast<std::size_t>(index)];
}

int Schema::FindColumnIndex(const std::string& name) const {
    for (std::size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Value Value::Int(int value) {
    Value result;
    result.type = ColumnType::kInt;
    result.int_value = value;
    return result;
}

Value Value::String(const std::string& value) {
    Value result;
    result.type = ColumnType::kString;
    result.string_value = value;
    return result;
}

StorageEngine::StorageEngine(std::filesystem::path data_root)
    : data_root_(std::move(data_root)), current_database_() {}

Status StorageEngine::Initialize() {
    std::error_code ec;
    std::filesystem::create_directories(data_root_, ec);
    if (ec) {
        return Status::IOError("Failed to create data root: " + ec.message());
    }
    return Status::OK();
}

Status StorageEngine::CreateDatabase(const std::string& database_name) {
    Status status = ValidateIdentifier(database_name, "database name");
    if (!status.ok()) {
        return status;
    }
    std::filesystem::path path = DatabasePath(database_name);
    if (std::filesystem::exists(path)) {
        return Status::AlreadyExists("Database already exists: " + database_name);
    }
    std::error_code ec;
    if (!std::filesystem::create_directories(path, ec) || ec) {
        return Status::IOError("Failed to create database directory: " + ec.message());
    }
    return Status::OK();
}

Status StorageEngine::DropDatabase(const std::string& database_name) {
    Status status = ValidateIdentifier(database_name, "database name");
    if (!status.ok()) {
        return status;
    }
    std::filesystem::path path = DatabasePath(database_name);
    if (!std::filesystem::exists(path)) {
        return Status::NotFound("Database not found: " + database_name);
    }
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    if (ec) {
        return Status::IOError("Failed to drop database: " + ec.message());
    }
    if (current_database_ == database_name) {
        current_database_.clear();
    }
    return Status::OK();
}

Status StorageEngine::UseDatabase(const std::string& database_name) {
    Status status = ValidateIdentifier(database_name, "database name");
    if (!status.ok()) {
        return status;
    }
    if (!std::filesystem::is_directory(DatabasePath(database_name))) {
        return Status::NotFound("Database not found: " + database_name);
    }
    current_database_ = database_name;
    return Status::OK();
}

Status StorageEngine::CreateTable(const std::string& table_name, const Schema& schema) {
    Status status = RequireCurrentDatabase();
    if (!status.ok()) {
        return status;
    }
    status = ValidateIdentifier(table_name, "table name");
    if (!status.ok()) {
        return status;
    }
    status = ValidateSchema(schema);
    if (!status.ok()) {
        return status;
    }
    std::filesystem::path meta_path = TableMetaPath(table_name);
    std::filesystem::path data_path = TableDataPath(table_name);
    if (std::filesystem::exists(meta_path) || std::filesystem::exists(data_path)) {
        return Status::AlreadyExists("Table already exists: " + table_name);
    }
    status = WriteSchemaFile(meta_path, schema);
    if (!status.ok()) {
        return status;
    }
    status = EnsureDataFile(data_path);
    if (!status.ok()) {
        std::error_code ec;
        std::filesystem::remove(meta_path, ec);
        return status;
    }
    return Status::OK();
}

Status StorageEngine::CreateTable(
    const std::string& table_name,
    const common::DynamicArray<sql::ColumnDefinition>& columns) {
    Schema schema;
    Status status = ConvertSqlColumnsToSchema(columns, &schema);
    if (!status.ok()) {
        return status;
    }
    return CreateTable(table_name, schema);
}

Status StorageEngine::DropTable(const std::string& table_name) {
    Status status = RequireCurrentDatabase();
    if (!status.ok()) {
        return status;
    }
    status = ValidateIdentifier(table_name, "table name");
    if (!status.ok()) {
        return status;
    }
    std::filesystem::path meta_path = TableMetaPath(table_name);
    std::filesystem::path data_path = TableDataPath(table_name);
    if (!std::filesystem::exists(meta_path) && !std::filesystem::exists(data_path)) {
        return Status::NotFound("Table not found: " + table_name);
    }
    std::error_code ec;
    std::filesystem::remove(meta_path, ec);
    if (ec) {
        return Status::IOError("Failed to remove table metadata: " + ec.message());
    }
    std::filesystem::remove(data_path, ec);
    if (ec) {
        return Status::IOError("Failed to remove table data: " + ec.message());
    }
    std::filesystem::remove(CurrentDatabasePath() / (table_name + ".idx"), ec);
    return Status::OK();
}

SchemaResult StorageEngine::LoadSchema(const std::string& table_name) const {
    SchemaResult result;
    result.status = RequireCurrentDatabase();
    if (!result.status.ok()) {
        return result;
    }
    result.status = ValidateIdentifier(table_name, "table name");
    if (!result.status.ok()) {
        return result;
    }
    std::filesystem::path path = TableMetaPath(table_name);
    if (!std::filesystem::exists(path)) {
        result.status = Status::NotFound("Table not found: " + table_name);
        return result;
    }
    return ReadSchemaFile(path);
}

InsertResult StorageEngine::InsertRow(const std::string& table_name, const Row& row) {
    InsertResult result;
    SchemaResult schema = LoadSchema(table_name);
    result.status = schema.status;
    if (!result.status.ok()) {
        return result;
    }
    result.status = ValidateRowMatchesSchema(row, schema.schema);
    if (!result.status.ok()) {
        return result;
    }
    result.status = AppendRowRecord(TableDataPath(table_name), row, &result.row_id);
    return result;
}

ScanResult StorageEngine::ScanTable(const std::string& table_name) const {
    ScanResult result;
    SchemaResult schema = LoadSchema(table_name);
    result.status = schema.status;
    if (!result.status.ok()) {
        return result;
    }
    std::ifstream input(TableDataPath(table_name), std::ios::binary);
    if (!input.is_open()) {
        result.status = IoError("Failed to open data file for scan", TableDataPath(table_name));
        return result;
    }
    result.status = ReadFileHeader(input, kDataMagic);
    if (!result.status.ok()) {
        return result;
    }
    while (input.peek() != std::char_traits<char>::eof()) {
        RowId row_id(CurrentOffset(input));
        std::uint8_t flags = 0;
        std::uint32_t payload_length = 0;
        Row row;
        result.status = ReadRowRecord(input, schema.schema, &flags, &payload_length, &row);
        if (!result.status.ok()) {
            return result;
        }
        if (flags == kRowLive) {
            StoredRow stored;
            stored.row_id = row_id;
            stored.row = std::move(row);
            result.status = result.rows.rows.PushBack(std::move(stored));
            if (!result.status.ok()) {
                return result;
            }
        }
    }
    result.status = Status::OK();
    return result;
}

ReadResult StorageEngine::ReadRow(const std::string& table_name, RowId row_id) const {
    ReadResult result;
    if (!row_id.IsValid()) {
        result.status = Status::InvalidArgument("Invalid RowId");
        return result;
    }
    SchemaResult schema = LoadSchema(table_name);
    result.status = schema.status;
    if (!result.status.ok()) {
        return result;
    }
    std::ifstream input(TableDataPath(table_name), std::ios::binary);
    if (!input.is_open()) {
        result.status = IoError("Failed to open data file for read", TableDataPath(table_name));
        return result;
    }
    input.seekg(static_cast<std::streamoff>(row_id.offset));
    if (!input.good()) {
        result.status = Status::InvalidArgument("RowId offset is outside data file");
        return result;
    }
    std::uint8_t flags = 0;
    std::uint32_t payload_length = 0;
    result.status = ReadRowRecord(input, schema.schema, &flags, &payload_length, &result.row);
    if (!result.status.ok()) {
        return result;
    }
    if (flags != kRowLive) {
        result.status = Status::NotFound("Row has been deleted");
        return result;
    }
    result.status = Status::OK();
    return result;
}

UpdateResult StorageEngine::UpdateRow(const std::string& table_name, RowId row_id, const Row& row) {
    UpdateResult result;
    if (!row_id.IsValid()) {
        result.status = Status::InvalidArgument("Invalid RowId");
        return result;
    }
    SchemaResult schema = LoadSchema(table_name);
    result.status = schema.status;
    if (!result.status.ok()) {
        return result;
    }
    result.status = ValidateRowMatchesSchema(row, schema.schema);
    if (!result.status.ok()) {
        return result;
    }
    std::string payload;
    result.status = SerializeRowToString(row, &payload);
    if (!result.status.ok()) {
        return result;
    }
    std::fstream file(TableDataPath(table_name), std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
        result.status = IoError("Failed to open data file for update", TableDataPath(table_name));
        return result;
    }
    file.seekg(static_cast<std::streamoff>(row_id.offset));
    if (!file.good()) {
        result.status = Status::InvalidArgument("RowId offset is outside data file");
        return result;
    }
    std::uint8_t flags = 0;
    std::uint32_t old_payload_length = 0;
    result.status = ReadUInt8(file, &flags);
    if (!result.status.ok()) {
        return result;
    }
    result.status = ReadUInt32(file, &old_payload_length);
    if (!result.status.ok()) {
        return result;
    }
    if (flags != kRowLive) {
        result.status = Status::NotFound("Row has been deleted");
        return result;
    }
    if (payload.size() <= old_payload_length) {
        file.seekp(static_cast<std::streamoff>(row_id.offset + 1 + 4));
        result.status = WriteBytes(file, payload.data(), payload.size());
        if (!result.status.ok()) {
            return result;
        }
        if (payload.size() < old_payload_length) {
            std::string padding(old_payload_length - payload.size(), '\0');
            result.status = WriteBytes(file, padding.data(), padding.size());
            if (!result.status.ok()) {
                return result;
            }
        }
        result.row_id = row_id;
        result.moved = false;
        result.status = Status::OK();
        return result;
    }
    result.status = MarkDeleted(TableDataPath(table_name), row_id);
    if (!result.status.ok()) {
        return result;
    }
    result.status = AppendRowRecord(TableDataPath(table_name), row, &result.row_id);
    result.moved = result.status.ok();
    return result;
}

Status StorageEngine::DeleteRow(const std::string& table_name, RowId row_id) {
    ReadResult read = ReadRow(table_name, row_id);
    if (!read.status.ok()) {
        return read.status;
    }
    return MarkDeleted(TableDataPath(table_name), row_id);
}

std::filesystem::path StorageEngine::DatabasePath(const std::string& database_name) const {
    return data_root_ / database_name;
}

std::filesystem::path StorageEngine::CurrentDatabasePath() const {
    return DatabasePath(current_database_);
}

std::filesystem::path StorageEngine::TableMetaPath(const std::string& table_name) const {
    return CurrentDatabasePath() / (table_name + ".meta");
}

std::filesystem::path StorageEngine::TableDataPath(const std::string& table_name) const {
    return CurrentDatabasePath() / (table_name + ".dat");
}

Status StorageEngine::RequireCurrentDatabase() const {
    if (current_database_.empty()) {
        return Status::InvalidArgument("No database selected");
    }
    if (!std::filesystem::is_directory(CurrentDatabasePath())) {
        return Status::NotFound("Current database directory is missing");
    }
    return Status::OK();
}

Status StorageEngine::ValidateIdentifier(const std::string& name, const std::string& kind) const {
    if (!common::StringUtils::IsLowerAlphaIdentifier(name)) {
        return Status::InvalidArgument("Invalid " + kind + ": " + name);
    }
    return Status::OK();
}

Status StorageEngine::ValidateSchema(const Schema& schema) const {
    if (schema.columns.empty()) {
        return Status::InvalidArgument("Table schema must contain at least one column");
    }
    int primary_count = 0;
    for (std::size_t i = 0; i < schema.columns.size(); ++i) {
        Status status = ValidateIdentifier(schema.columns[i].name, "column name");
        if (!status.ok()) {
            return status;
        }
        for (std::size_t j = i + 1; j < schema.columns.size(); ++j) {
            if (schema.columns[i].name == schema.columns[j].name) {
                return Status::InvalidArgument("Duplicate column name: " + schema.columns[i].name);
            }
        }
        if (schema.columns[i].primary) {
            ++primary_count;
            if (schema.primary_column_index != static_cast<int>(i)) {
                return Status::InvalidArgument("Primary column index does not match column flags");
            }
            if (schema.columns[i].type != ColumnType::kInt) {
                return Status::InvalidArgument("Primary key must be int for B+ tree index");
            }
        }
    }
    if (primary_count > 1) {
        return Status::InvalidArgument("Only one primary column is supported");
    }
    if (primary_count == 0 && schema.primary_column_index != -1) {
        return Status::InvalidArgument("Primary column index set without primary column");
    }
    return Status::OK();
}

Status StorageEngine::ValidateRowMatchesSchema(const Row& row, const Schema& schema) const {
    if (row.values.size() != schema.columns.size()) {
        return Status::InvalidArgument("Row value count does not match table schema");
    }
    for (std::size_t i = 0; i < row.values.size(); ++i) {
        if (row.values[i].type != schema.columns[i].type) {
            return Status::InvalidArgument("Row value type does not match column type");
        }
        if (row.values[i].type == ColumnType::kString &&
            row.values[i].string_value.size() > kMaxStringBytes) {
            return Status::InvalidArgument("String value exceeds 256 bytes");
        }
    }
    return Status::OK();
}

Status ConvertSqlColumnsToSchema(const common::DynamicArray<sql::ColumnDefinition>& columns,
                                 Schema* schema) {
    schema->columns.Clear();
    schema->primary_column_index = -1;
    for (std::size_t i = 0; i < columns.size(); ++i) {
        Column column;
        column.name = columns[i].name;
        column.type = columns[i].type == sql::DataType::kInt ? ColumnType::kInt
                                                             : ColumnType::kString;
        column.primary = columns[i].primary;
        if (column.primary) {
            if (schema->primary_column_index >= 0) {
                return Status::InvalidArgument("Only one primary column is supported");
            }
            schema->primary_column_index = static_cast<int>(i);
        }
        Status status = schema->columns.PushBack(std::move(column));
        if (!status.ok()) {
            return status;
        }
    }
    return Status::OK();
}

}  // namespace mini_dbms::storage
