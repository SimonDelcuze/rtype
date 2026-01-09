# Server Implementation

The server-side authentication system manages user accounts, sessions, and statistics using SQLite database and JWT tokens.

## Architecture Overview

```
┌─────────────────┐
│  LobbyServer    │
│                 │
│  handleLogin    │──────┐
│  handleRegister │      │
│  handleGetStats │      ├──> UserRepository
│                 │      │    (Database Layer)
└─────────────────┘      │
         │               │
         │               ▼
         │        ┌──────────────┐
         │        │   SQLite DB  │
         │        │              │
         │        │  - users     │
         │        │  - sessions  │
         │        │  - stats     │
         │        └──────────────┘
         │
         ▼
  ┌──────────────┐
  │SessionManager│
  │              │
  │  JWT tokens  │
  │  validation  │
  └──────────────┘
```

## Database Schema

### Users Table
```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    last_login INTEGER DEFAULT 0
);
```

### Sessions Table
```sql
CREATE TABLE sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    token_hash TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    expires_at INTEGER NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id)
);
```

### User Stats Table
```sql
CREATE TABLE user_stats (
    user_id INTEGER PRIMARY KEY,
    games_played INTEGER DEFAULT 0,
    wins INTEGER DEFAULT 0,
    losses INTEGER DEFAULT 0,
    total_score INTEGER DEFAULT 0,
    FOREIGN KEY (user_id) REFERENCES users(id)
);
```

## Key Components

### UserRepository

Location: `server/src/auth/UserRepository.cpp`

Manages all database operations:

```cpp
class UserRepository {
public:
    // User management
    std::optional<User> createUser(const std::string& username,
                                   const std::string& passwordHash);
    std::optional<User> getUserByUsername(const std::string& username);

    // Session management
    bool createSession(std::uint32_t userId, const std::string& tokenHash,
                      std::uint64_t expiresAt);
    bool validateSessionToken(std::uint32_t userId, const std::string& tokenHash);
    void cleanupExpiredTokens();

    // Statistics
    std::optional<UserStats> getUserStats(std::uint32_t userId);
    bool updateUserStats(std::uint32_t userId, std::uint32_t gamesPlayed,
                        std::uint32_t wins, std::uint32_t losses,
                        std::uint64_t totalScore);
};
```

### JWT Token Generation

Location: `server/src/auth/JWTHandler.cpp`

Generates and validates JWT tokens:

```cpp
// Generate token
std::string token = JWTHandler::generateToken(userId, username, secretKey);

// Validate token
auto claims = JWTHandler::validateToken(token, secretKey);
if (claims.has_value()) {
    std::uint32_t userId = claims->userId;
    std::string username = claims->username;
}
```

### Session Management

Location: `server/include/core/Session.hpp`

The server maintains active sessions per UDP endpoint:

```cpp
struct ClientSession {
    IpEndpoint endpoint;
    bool authenticated{false};
    std::optional<std::uint32_t> userId;
    std::string username;
    std::string jwtToken;
    std::chrono::steady_clock::time_point lastActivity;
};

// Session storage
std::unordered_map<std::string, ClientSession> lobbySessions_;
```

## Request Handlers

### handleRegisterRequest

Location: `server/src/lobby/LobbyServer.cpp:395`

1. Parse username and password from packet
2. Validate username length and format
3. Hash password with bcrypt
4. Create user in database
5. Initialize user stats
6. Send success/error response

```cpp
void LobbyServer::handleRegisterRequest(const PacketHeader& hdr,
                                       const IpEndpoint& from) {
    auto data = parseRegisterRequestPacket(...);

    // Hash password
    std::string passwordHash = BCrypt::generateHash(data->password);

    // Create user
    auto user = userRepository_->createUser(data->username, passwordHash);

    if (user.has_value()) {
        sendRegisterSuccess(from, hdr.sequenceId);
    } else {
        sendRegisterError(from, "Username already exists", hdr.sequenceId);
    }
}
```

### handleLoginRequest

Location: `server/src/lobby/LobbyServer.cpp:450`

1. Parse username and password
2. Fetch user from database
3. Verify password with bcrypt
4. Generate JWT token
5. Create session in database
6. Store session in memory
7. Send token to client

```cpp
void LobbyServer::handleLoginRequest(const PacketHeader& hdr,
                                    const IpEndpoint& from) {
    auto data = parseLoginRequestPacket(...);

    // Get user
    auto user = userRepository_->getUserByUsername(data->username);

    // Verify password
    if (BCrypt::validatePassword(data->password, user->passwordHash)) {
        // Generate JWT
        std::string token = JWTHandler::generateToken(user->id, user->username);

        // Create session
        std::string tokenHash = hashToken(token);
        userRepository_->createSession(user->id, tokenHash, expiresAt);

        // Store in memory
        ClientSession session;
        session.authenticated = true;
        session.userId = user->id;
        session.username = user->username;
        session.jwtToken = token;
        lobbySessions_[endpointKey] = session;

        // Send token
        sendLoginSuccess(from, token, user->id, hdr.sequenceId);
    }
}
```

### handleGetStatsRequest

Location: `server/src/lobby/LobbyServer.cpp:542`

1. Check if session is authenticated
2. Get userId from session
3. Fetch stats from database
4. Send stats response

```cpp
void LobbyServer::handleGetStatsRequest(const PacketHeader& hdr,
                                       const IpEndpoint& from) {
    // Check authentication
    if (!isAuthenticated(from)) {
        sendAuthRequired(from);
        return;
    }

    // Get stats
    auto session = getSession(from);
    auto stats = userRepository_->getUserStats(session.userId.value());

    // Send response
    if (stats.has_value()) {
        sendStatsResponse(from, *stats, hdr.sequenceId);
    } else {
        sendEmptyStats(from, session.userId.value(), hdr.sequenceId);
    }
}
```

## Authentication Flow

1. **Client connects** to lobby server
2. **Client sends** login/register request
3. **Server validates** credentials
4. **Server generates** JWT token
5. **Server creates** session (database + memory)
6. **Server sends** token to client
7. **Client stores** token
8. **Client sends** token with subsequent requests
9. **Server validates** token on each authenticated request

## Security Measures

### Password Security
- **bcrypt hashing** with automatic salt generation
- **Cost factor**: 12 (adjustable)
- **No plaintext** storage

### Token Security
- **JWT tokens** with signature
- **Secret key** stored securely
- **Token expiration** (7 days default)
- **Token hashing** in database

### Session Security
- **Per-endpoint** sessions
- **Automatic cleanup** of expired sessions
- **Validation** on every authenticated request

## Configuration

Key configuration values in `server/src/lobby/LobbyServer.cpp`:

```cpp
const std::string JWT_SECRET = "your-secret-key-here"; // Change in production!
const std::uint64_t TOKEN_EXPIRY_SECONDS = 7 * 24 * 60 * 60; // 7 days
const int BCRYPT_COST = 12; // Password hashing cost
```

## Database Location

The SQLite database is stored at: `data/rtype.db`

Initialize with:
```bash
mkdir -p data
sqlite3 data/rtype.db < server/schema.sql
```

## Future Improvements

- Add password reset functionality
- Implement email verification
- Add rate limiting for login attempts
- Support for multiple concurrent sessions
- Admin panel for user management
