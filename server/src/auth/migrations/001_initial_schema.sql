-- Initial database schema for user authentication system

-- Users table: stores user account information
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    last_login INTEGER DEFAULT 0
);

-- Create index on username for faster lookups
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);

-- User stats table: tracks game performance metrics
CREATE TABLE IF NOT EXISTS user_stats (
    user_id INTEGER PRIMARY KEY,
    games_played INTEGER NOT NULL DEFAULT 0,
    wins INTEGER NOT NULL DEFAULT 0,
    losses INTEGER NOT NULL DEFAULT 0,
    total_score INTEGER NOT NULL DEFAULT 0,
    total_ranked_score INTEGER NOT NULL DEFAULT 0,
    elo INTEGER NOT NULL DEFAULT 1000,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Session tokens table: stores JWT token hashes for "Remember Me" functionality
CREATE TABLE IF NOT EXISTS session_tokens (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    token_hash TEXT NOT NULL,
    expires_at INTEGER NOT NULL,
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Create index on user_id for faster token lookups
CREATE INDEX IF NOT EXISTS idx_session_tokens_user_id ON session_tokens(user_id);

-- Create index on token_hash for validation
CREATE INDEX IF NOT EXISTS idx_session_tokens_hash ON session_tokens(token_hash);

-- Trigger to automatically create user_stats entry when user is created
CREATE TRIGGER IF NOT EXISTS create_user_stats_trigger
AFTER INSERT ON users
BEGIN
    INSERT INTO user_stats (user_id, games_played, wins, losses, total_score, total_ranked_score, elo)
    VALUES (NEW.id, 0, 0, 0, 0, 0, 1000);
END;

-- Trigger to clean up expired tokens (runs on INSERT to session_tokens)
CREATE TRIGGER IF NOT EXISTS cleanup_expired_tokens_trigger
AFTER INSERT ON session_tokens
BEGIN
    DELETE FROM session_tokens WHERE expires_at < strftime('%s', 'now');
END;
