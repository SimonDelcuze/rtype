#include "auth/Database.hpp"

#include <cstring>
#include <filesystem>
#include <iostream>

PreparedStatement::PreparedStatement(sqlite3_stmt* stmt) : stmt_(stmt), lastStepResult_(SQLITE_OK) {}

PreparedStatement::~PreparedStatement()
{
    if (stmt_ != nullptr) {
        sqlite3_finalize(stmt_);
    }
}

PreparedStatement::PreparedStatement(PreparedStatement&& other) noexcept
    : stmt_(other.stmt_), lastStepResult_(other.lastStepResult_)
{
    other.stmt_ = nullptr;
}

PreparedStatement& PreparedStatement::operator=(PreparedStatement&& other) noexcept
{
    if (this != &other) {
        if (stmt_ != nullptr) {
            sqlite3_finalize(stmt_);
        }
        stmt_           = other.stmt_;
        lastStepResult_ = other.lastStepResult_;
        other.stmt_     = nullptr;
    }
    return *this;
}

bool PreparedStatement::bind(int index, const std::string& value)
{
    return sqlite3_bind_text(stmt_, index, value.c_str(), static_cast<int>(value.length()), SQLITE_TRANSIENT) ==
           SQLITE_OK;
}

bool PreparedStatement::bind(int index, int value)
{
    return sqlite3_bind_int(stmt_, index, value) == SQLITE_OK;
}

bool PreparedStatement::bind(int index, std::uint32_t value)
{
    return sqlite3_bind_int64(stmt_, index, static_cast<std::int64_t>(value)) == SQLITE_OK;
}

bool PreparedStatement::bind(int index, std::int64_t value)
{
    return sqlite3_bind_int64(stmt_, index, value) == SQLITE_OK;
}

bool PreparedStatement::step()
{
    lastStepResult_ = sqlite3_step(stmt_);
    return lastStepResult_ == SQLITE_ROW || lastStepResult_ == SQLITE_DONE;
}

bool PreparedStatement::hasRow() const
{
    return lastStepResult_ == SQLITE_ROW;
}

void PreparedStatement::reset()
{
    sqlite3_reset(stmt_);
    lastStepResult_ = SQLITE_OK;
}

std::optional<std::string> PreparedStatement::getColumnString(int index) const
{
    if (!hasRow()) {
        return std::nullopt;
    }

    const unsigned char* text = sqlite3_column_text(stmt_, index);
    if (text == nullptr) {
        return std::nullopt;
    }

    return std::string(reinterpret_cast<const char*>(text));
}

std::optional<int> PreparedStatement::getColumnInt(int index) const
{
    if (!hasRow()) {
        return std::nullopt;
    }

    return sqlite3_column_int(stmt_, index);
}

std::optional<std::uint32_t> PreparedStatement::getColumnUInt32(int index) const
{
    if (!hasRow()) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(sqlite3_column_int64(stmt_, index));
}

std::optional<std::int64_t> PreparedStatement::getColumnInt64(int index) const
{
    if (!hasRow()) {
        return std::nullopt;
    }

    return sqlite3_column_int64(stmt_, index);
}

Transaction::Transaction(sqlite3* db) : db_(db), committed_(false)
{
    sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
}

Transaction::~Transaction()
{
    if (!committed_) {
        rollback();
    }
}

bool Transaction::commit()
{
    if (committed_) {
        return false;
    }

    int result = sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
    if (result == SQLITE_OK) {
        committed_ = true;
        return true;
    }
    return false;
}

void Transaction::rollback()
{
    if (!committed_) {
        sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        committed_ = true;
    }
}

Database::Database() : db_(nullptr) {}

Database::~Database()
{
    if (db_ != nullptr) {
        sqlite3_close(db_);
    }
}

bool Database::initialize(const std::string& dbPath)
{
    std::filesystem::path path(dbPath);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    int result = sqlite3_open(dbPath.c_str(), &db_);
    if (result != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    execute("PRAGMA journal_mode=WAL");

    execute("PRAGMA foreign_keys=ON");

    return true;
}

bool Database::executeScript(const std::string& sqlScript)
{
    char* errMsg = nullptr;
    int result   = sqlite3_exec(db_, sqlScript.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        std::cerr << "SQL script execution failed: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool Database::execute(const std::string& sql)
{
    char* errMsg = nullptr;
    int result   = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        std::cerr << "SQL execution failed: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

std::optional<PreparedStatement> Database::prepare(const std::string& sql)
{
    sqlite3_stmt* stmt = nullptr;
    int result         = sqlite3_prepare_v2(db_, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);

    if (result != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return std::nullopt;
    }

    return PreparedStatement(stmt);
}

std::unique_ptr<Transaction> Database::beginTransaction()
{
    return std::make_unique<Transaction>(db_);
}

std::int64_t Database::lastInsertRowId() const
{
    return sqlite3_last_insert_rowid(db_);
}

std::string Database::getLastError() const
{
    return sqlite3_errmsg(db_);
}

bool Database::isOpen() const
{
    return db_ != nullptr;
}
