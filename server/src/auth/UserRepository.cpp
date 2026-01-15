#include "auth/UserRepository.hpp"

#include <chrono>
#include <iostream>

UserRepository::UserRepository(std::shared_ptr<Database> db) : db_(std::move(db)) {}

std::optional<std::uint32_t> UserRepository::createUser(const std::string& username, const std::string& passwordHash)
{
    auto stmt = db_->prepare("INSERT INTO users (username, password_hash) VALUES (?, ?)");
    if (!stmt.has_value()) {
        return std::nullopt;
    }

    stmt->bind(1, username);
    stmt->bind(2, passwordHash);

    if (!stmt->step()) {
        std::cerr << "Failed to create user: " << db_->getLastError() << std::endl;
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(db_->lastInsertRowId());
}

std::optional<User> UserRepository::getUserByUsername(const std::string& username)
{
    auto stmt =
        db_->prepare("SELECT id, username, password_hash, created_at, last_login FROM users WHERE username = ?");
    if (!stmt.has_value()) {
        return std::nullopt;
    }

    stmt->bind(1, username);

    if (!stmt->step() || !stmt->hasRow()) {
        return std::nullopt;
    }

    User user;
    user.id           = stmt->getColumnUInt32(0).value_or(0);
    user.username     = stmt->getColumnString(1).value_or("");
    user.passwordHash = stmt->getColumnString(2).value_or("");
    user.createdAt    = stmt->getColumnInt64(3).value_or(0);
    user.lastLogin    = stmt->getColumnInt64(4).value_or(0);

    return user;
}

std::optional<User> UserRepository::getUserById(std::uint32_t userId)
{
    auto stmt = db_->prepare("SELECT id, username, password_hash, created_at, last_login FROM users WHERE id = ?");
    if (!stmt.has_value()) {
        return std::nullopt;
    }

    stmt->bind(1, userId);

    if (!stmt->step() || !stmt->hasRow()) {
        return std::nullopt;
    }

    User user;
    user.id           = stmt->getColumnUInt32(0).value_or(0);
    user.username     = stmt->getColumnString(1).value_or("");
    user.passwordHash = stmt->getColumnString(2).value_or("");
    user.createdAt    = stmt->getColumnInt64(3).value_or(0);
    user.lastLogin    = stmt->getColumnInt64(4).value_or(0);

    return user;
}

bool UserRepository::updateLastLogin(std::uint32_t userId)
{
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    auto stmt = db_->prepare("UPDATE users SET last_login = ? WHERE id = ?");
    if (!stmt.has_value()) {
        return false;
    }

    stmt->bind(1, static_cast<std::int64_t>(time));
    stmt->bind(2, userId);

    return stmt->step();
}

bool UserRepository::storeSessionToken(std::uint32_t userId, const std::string& tokenHash, std::int64_t expiresAt)
{
    auto stmt = db_->prepare("INSERT INTO session_tokens (user_id, token_hash, expires_at) VALUES (?, ?, ?)");
    if (!stmt.has_value()) {
        return false;
    }

    stmt->bind(1, userId);
    stmt->bind(2, tokenHash);
    stmt->bind(3, expiresAt);

    return stmt->step();
}

bool UserRepository::validateSessionToken(std::uint32_t userId, const std::string& tokenHash)
{
    auto now     = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);

    auto stmt =
        db_->prepare("SELECT COUNT(*) FROM session_tokens WHERE user_id = ? AND token_hash = ? AND expires_at > ?");
    if (!stmt.has_value()) {
        return false;
    }

    stmt->bind(1, userId);
    stmt->bind(2, tokenHash);
    stmt->bind(3, static_cast<std::int64_t>(nowTime));

    if (!stmt->step() || !stmt->hasRow()) {
        return false;
    }

    auto count = stmt->getColumnInt(0).value_or(0);
    return count > 0;
}

void UserRepository::cleanupExpiredTokens()
{
    auto now     = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);

    auto stmt = db_->prepare("DELETE FROM session_tokens WHERE expires_at < ?");
    if (!stmt.has_value()) {
        return;
    }

    stmt->bind(1, static_cast<std::int64_t>(nowTime));
    stmt->step();
}

std::optional<UserStats> UserRepository::getUserStats(std::uint32_t userId)
{
    auto stmt =
        db_->prepare("SELECT user_id, games_played, wins, losses, total_score, elo FROM user_stats WHERE user_id = ?");
    if (!stmt.has_value()) {
        return std::nullopt;
    }

    stmt->bind(1, userId);

    if (!stmt->step() || !stmt->hasRow()) {
        return std::nullopt;
    }

    UserStats stats;
    stats.userId      = stmt->getColumnUInt32(0).value_or(0);
    stats.gamesPlayed = stmt->getColumnUInt32(1).value_or(0);
    stats.wins        = stmt->getColumnUInt32(2).value_or(0);
    stats.losses      = stmt->getColumnUInt32(3).value_or(0);
    stats.totalScore  = static_cast<std::uint64_t>(stmt->getColumnInt64(4).value_or(0));
    stats.elo         = stmt->getColumnInt(5).value_or(1000);

    return stats;
}

bool UserRepository::updateUserStats(std::uint32_t userId, std::uint32_t gamesPlayed, std::uint32_t wins,
                                     std::uint32_t losses, std::uint64_t totalScore, std::int32_t elo)
{
    auto stmt = db_->prepare("INSERT OR REPLACE INTO user_stats (user_id, games_played, wins, losses, total_score, elo) "
                             "VALUES (?, ?, ?, ?, ?, ?)");
    if (!stmt.has_value()) {
        return false;
    }

    stmt->bind(1, userId);
    stmt->bind(2, gamesPlayed);
    stmt->bind(3, wins);
    stmt->bind(4, losses);
    stmt->bind(5, static_cast<std::int64_t>(totalScore));
    stmt->bind(6, elo);

    return stmt->step();
}

bool UserRepository::updatePassword(std::uint32_t userId, const std::string& newPasswordHash)
{
    auto stmt = db_->prepare("UPDATE users SET password_hash = ? WHERE id = ?");
    if (!stmt.has_value()) {
        return false;
    }

    stmt->bind(1, newPasswordHash);
    stmt->bind(2, userId);

    return stmt->step();
}
