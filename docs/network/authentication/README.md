# Authentication & User Management

The R-Type game includes a complete authentication system with user registration, login, session management, and user statistics tracking.

## Overview

The authentication system provides:
- **User Registration**: Create new accounts with username and password
- **Secure Login**: JWT-based authentication with bcrypt password hashing
- **Session Management**: Persistent sessions across the application
- **User Statistics**: Track games played, wins, losses, and scores
- **Profile Display**: View user statistics in the lobby

## Architecture

The authentication system is built with:
- **Server-side**: SQLite database, JWT tokens, session management
- **Client-side**: Login/Register UI, authenticated network requests
- **Network Protocol**: Custom UDP packets for auth requests/responses

## Key Features

### 1. User Registration
New users can create accounts with:
- Unique username validation
- Secure password hashing (bcrypt)
- Automatic stats initialization

### 2. User Login
Secure authentication with:
- JWT token generation
- Session persistence
- Token validation on requests

### 3. User Statistics
Track player performance:
- Games played
- Wins and losses
- Win rate calculation
- Total score accumulation

### 4. Profile Display
Beautiful stats box in lobby showing:
- Player name
- Game statistics
- Win rate percentage
- Total score

## Documentation Pages

- [Network Protocol](protocol.md) - Authentication packets and data structures
- [Server Implementation](server-implementation.md) - Database, JWT, and session management
- [Client Implementation](client-implementation.md) - UI components and network integration
- [User Statistics](user-statistics.md) - Stats tracking and display system
- [Flow Diagrams](flow-diagrams.md) - Visual representation of authentication flows
- [Testing Guide](testing.md) - How to test the authentication system

## Security Features

- **Password Hashing**: bcrypt with salt
- **JWT Tokens**: Secure session tokens
- **Session Validation**: Server-side authentication checks
- **SQL Injection Prevention**: Prepared statements

## Future Enhancements

The statistics system is ready but not yet integrated with gameplay:
- Stats will be updated at the end of each game
- Win/loss tracking will be connected to game results
- Score accumulation will reflect player performance

## Quick Start

See the [Testing Guide](testing.md) for instructions on testing the authentication system.
