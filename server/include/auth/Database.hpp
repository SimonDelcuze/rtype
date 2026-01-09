#pragma once

#include <memory>
#include <optional>
#include <sqlite3.h>
#include <string>
#include <vector>

class PreparedStatement
{
  public:
    PreparedStatement(sqlite3_stmt* stmt);
    ~PreparedStatement();

    PreparedStatement(const PreparedStatement&)            = delete;
    PreparedStatement& operator=(const PreparedStatement&) = delete;
    PreparedStatement(PreparedStatement&& other) noexcept;
    PreparedStatement& operator=(PreparedStatement&& other) noexcept;

    bool bind(int index, const std::string& value);
    bool bind(int index, int value);
    bool bind(int index, std::uint32_t value);
    bool bind(int index, std::int64_t value);

    bool step();
    bool hasRow() const;
    void reset();

    std::optional<std::string> getColumnString(int index) const;
    std::optional<int> getColumnInt(int index) const;
    std::optional<std::uint32_t> getColumnUInt32(int index) const;
    std::optional<std::int64_t> getColumnInt64(int index) const;

  private:
    sqlite3_stmt* stmt_;
    int lastStepResult_;
};

class Transaction
{
  public:
    explicit Transaction(sqlite3* db);
    ~Transaction();

    Transaction(const Transaction&)            = delete;
    Transaction& operator=(const Transaction&) = delete;

    bool commit();
    void rollback();

  private:
    sqlite3* db_;
    bool committed_;
};

class Database
{
  public:
    Database();
    ~Database();

    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    bool initialize(const std::string& dbPath);
    bool executeScript(const std::string& sqlScript);
    bool execute(const std::string& sql);

    std::optional<PreparedStatement> prepare(const std::string& sql);
    std::unique_ptr<Transaction> beginTransaction();

    std::int64_t lastInsertRowId() const;
    std::string getLastError() const;

    bool isOpen() const;

  private:
    sqlite3* db_;
};
