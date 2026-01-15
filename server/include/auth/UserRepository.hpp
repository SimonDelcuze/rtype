#pragma once

#include "auth/Database.hpp"
#include "auth/User.hpp"

#include <memory>
#include <optional>
#include <string>

class UserRepository
{
  public:
    explicit UserRepository(std::shared_ptr<Database> db);

    std::optional<std::uint32_t> createUser(const std::string& username, const std::string& passwordHash);
    std::optional<User> getUserByUsername(const std::string& username);
    std::optional<User> getUserById(std::uint32_t userId);
    bool updateLastLogin(std::uint32_t userId);

    bool storeSessionToken(std::uint32_t userId, const std::string& tokenHash, std::int64_t expiresAt);
    bool validateSessionToken(std::uint32_t userId, const std::string& tokenHash);
    void cleanupExpiredTokens();

    std::optional<UserStats> getUserStats(std::uint32_t userId);
    bool updateUserStats(std::uint32_t userId, std::uint32_t gamesPlayed, std::uint32_t wins, std::uint32_t losses,
                         std::uint64_t totalScore, std::uint64_t totalRankedScore, std::int32_t elo);

    struct LeaderboardEntryRow
    {
        std::string username;
        std::int32_t value;
    };

    std::vector<LeaderboardEntryRow> getTopElo(std::uint32_t limit);
    std::vector<LeaderboardEntryRow> getTopScore(std::uint32_t limit);

    bool updatePassword(std::uint32_t userId, const std::string& newPasswordHash);

  private:
    std::shared_ptr<Database> db_;
};
