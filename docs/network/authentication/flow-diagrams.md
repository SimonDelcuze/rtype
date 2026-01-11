# R-Type Client Authentication & Navigation Flow

## Visual Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        CLIENT LAUNCH                             │
│                     ./r-type_client                              │
└────────────────────────────────┬────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                   AUTHENTICATION CHECK                           │
│              (Static variable: authenticated)                    │
└─────────────────┬──────────────────────────┬────────────────────┘
                  │                          │
         NOT authenticated           authenticated = true
                  │                          │
                  ▼                          │
┌─────────────────────────────────────────┐ │
│         LOGIN MENU                      │ │
│  ┌────────────────────────────────┐    │ │
│  │ Username: [________]           │    │ │
│  │ Password: [********]           │    │ │ (passwords masked)
│  │                                │    │ │
│  │  [Login]  [Register]  [Exit]  │    │ │
│  └────────────────────────────────┘    │ │
│                                         │ │
│  If Register clicked:                  │ │
│  ┌────────────────────────────────┐    │ │
│  │ REGISTER MENU                  │    │ │
│  │ Username: [________]           │    │ │
│  │ Password: [********]           │    │ │
│  │ Confirm:  [********]           │    │ │
│  │                                │    │ │
│  │ [Register]  [Back]  [Exit]    │    │ │
│  └────────────────────────────────┘    │ │
│           │                             │ │
│           └─► Back to Login             │ │
└─────────────────┬───────────────────────┘ │
                  │                          │
           Login Success                     │
                  │                          │
                  ▼                          │
         Set static vars:                    │
         authenticated = true               │
         authenticatedUsername = "..."      │
                  │                          │
                  └──────────┬───────────────┘
                             │
                             ▼
        ╔═════════════════════════════════════════════════╗
        ║     NAVIGATION LOOP (while window.isOpen())     ║
        ║                                                 ║
        ║  ┌────────────────────────────────────────┐    ║
        ║  │    CONNECTION MENU (Server Selection)   │    ║
        ║  │                                         │    ║
        ║  │  Server IP:   [127.0.0.1]              │    ║
        ║  │  Port:        [50010]                  │    ║
        ║  │                                         │    ║
        ║  │  [Use Default]  [Connect]  [Settings]  │    ║
        ║  │                                         │    ║
        ║  │  [Exit]                                 │    ║
        ║  └─────────────────┬───────────────────────┘    ║
        ║                    │                            ║
        ║             Connect clicked                     ║
        ║                    │                            ║
        ║                    ▼                            ║
        ║  ┌────────────────────────────────────────┐    ║
        ║  │    LOBBY MENU (Room List)              │    ║
        ║  │                                         │    ║
        ║  │  Available Rooms:                      │    ║
        ║  │  ┌─────────────────────────────────┐   │    ║
        ║  │  │ Room 1  [Players: 2/4] [Join]  │   │    ║
        ║  │  │ Room 2  [Players: 1/4] [Join]  │   │    ║
        ║  │  └─────────────────────────────────┘   │    ║
        ║  │                                         │    ║
        ║  │  [Create Room]  [Refresh]  [Back]     │    ║
        ║  └─────────────────┬───────────┬──────────┘    ║
        ║                    │           │                ║
        ║              Join Room      Back clicked       ║
        ║                    │           │                ║
        ║                    │           │                ║
        ║                    │           └────────────────╫──┐
        ║                    │                            ║  │
        ║                    ▼                            ║  │
        ║  ┌────────────────────────────────────────┐    ║  │
        ║  │    WAITING ROOM                        │    ║  │
        ║  │                                         │    ║  │
        ║  │  Players:                              │    ║  │
        ║  │  • Player1 ✓ Ready                    │    ║  │
        ║  │  • Player2 ⌛ Not Ready                │    ║  │
        ║  │                                         │    ║  │
        ║  │  [Ready]  [Leave]                      │    ║  │
        ║  └─────────────────┬────────────────────────┘  ║  │
        ║                    │                            ║  │
        ║              All Ready                          ║  │
        ║                    │                            ║  │
        ╚════════════════════╪═════════════════════════════╝  │
                             │                               │
                             ▼                               │
        ┌────────────────────────────────────────┐           │
        │           GAME SESSION                 │           │
        │                                         │           │
        │         [Playing R-Type...]            │           │
        │                                         │           │
        └─────────────────┬──────────────────────┘           │
                          │                                  │
                   Game Ends                                 │
                          │                                  │
                          ▼                                  │
        ┌────────────────────────────────────────┐           │
        │         GAME OVER MENU                 │           │
        │                                         │           │
        │  Score: 12500                          │           │
        │  Result: Victory!                      │           │
        │                                         │           │
        │  [Retry]  [Quit]                       │           │
        └─────┬───────────────────┬──────────────┘           │
              │                   │                          │
           Retry              Quit                           │
              │                   │                          │
              └───────┐   ┌───────┘                          │
                      │   │                                  │
                      ▼   ▼                                  │
                   Exit Client                               │
                                                              │
                 Loop back to                                │
               CONNECTION MENU ◄────────────────────────────┘
               (No re-authentication needed!)
```

## Key Points

### 🔐 Authentication (Once per Session)
- Happens **only once** when client launches
- Uses **static variables** to remember authentication state
- No re-login required when navigating back from lobby

### 🔒 Password Security
- All password fields display **asterisks (****)** instead of plain text
- Passwords hashed with **PBKDF2** (4096 iterations) before storage
- **JWT tokens** (7-day expiration) for session management

### 🔄 Navigation Loop
- After authentication, enters **infinite loop** (while window.isOpen())
- User can go: Connection Menu ↔️ Lobby Menu ↔️ Connection Menu
- **Back button** in Lobby Menu returns to Connection Menu (no crash!)
- Loop continues until user explicitly exits or joins a game

### 🛡️ Resource Management
- **Explicit cleanup**: `lobbyConn.disconnect()` called before returning
- **RAII pattern**: Uses `break` instead of early returns to ensure cleanup
- **No mutex errors**: Proper lock handling prevents crashes

## Navigation Paths

### Path 1: Quick Start (Default)
```
Login → [Use Default] → Lobby → [Join Room] → Waiting Room → Game
```

### Path 2: Custom Server
```
Login → [Enter IP/Port] → [Connect] → Lobby → [Create Room] → Waiting Room → Game
```

### Path 3: Back Navigation (Fixed!)
```
Login → Connection → Lobby → [Back] → Connection → Lobby → [Back] → ...
                                ↑________________________↓
                           (Can cycle indefinitely without crashes)
```

### Path 4: New User Registration
```
Login → [Register] → Registration Menu → [Register] → Login → ...
```

## Code Locations

| Component | File | Line |
|-----------|------|------|
| Navigation Loop | [client/src/runtime/RunClientFlow.cpp](client/src/runtime/RunClientFlow.cpp) | 70-93 |
| Authentication Flow | [client/src/runtime/ConnectionFlow.cpp](client/src/runtime/ConnectionFlow.cpp) | 26-92 |
| Password Masking | [client/src/systems/InputFieldSystem.cpp](client/src/systems/InputFieldSystem.cpp) | 31-34 |
| Login UI | [client/src/ui/LoginMenu.cpp](client/src/ui/LoginMenu.cpp) | - |
| Register UI | [client/src/ui/RegisterMenu.cpp](client/src/ui/RegisterMenu.cpp) | - |
| Auth State | [client/include/auth/AuthResult.hpp](client/include/auth/AuthResult.hpp) | - |

## Testing Checklist

- [ ] Login screen appears on client launch
- [ ] Passwords show as asterisks in all fields
- [ ] Can register new account successfully
- [ ] Can login with existing credentials
- [ ] Connection menu appears after login
- [ ] Lobby menu appears after selecting server
- [ ] **Back button returns to connection menu (NO CRASH)**
- [ ] Can navigate back to lobby again
- [ ] Can repeat back-and-forth multiple times
- [ ] No mutex errors in console
- [ ] No authentication errors

## Error Messages (If They Occur)

| Error | Cause | Solution |
|-------|-------|----------|
| "Connection error: Unable to reach server" | Server not running | Start `./r-type_server` |
| "mutex lock failed" | Resource cleanup issue | Should be fixed! Report if occurs |
| "no response from server" | Server not listening | Check `lsof -i :50010` |
| Window closes on Back | Navigation loop broken | Should be fixed! Report if occurs |

## Server Requirements

The server must be running on port 50010:
```bash
# Check server status
lsof -i :50010

# Start server
./r-type_server

# Kill server
lsof -ti:50010 | xargs kill -9
```

## Database Schema

```
users
├── id (PRIMARY KEY)
├── username (UNIQUE)
├── password_hash (PBKDF2)
├── created_at (UNIX timestamp)
└── last_login (UNIX timestamp)

user_stats
├── user_id (FOREIGN KEY → users.id)
├── games_played
├── wins
├── losses
└── total_score

session_tokens
├── id (PRIMARY KEY)
├── user_id (FOREIGN KEY → users.id)
├── token_hash (JWT)
├── expires_at (UNIX timestamp)
└── created_at (UNIX timestamp)
```

## Static Variables (Authentication State)

```cpp
// In resolveServerEndpoint() - RunClientFlow.cpp:37-38
static bool authenticated = false;
static std::string authenticatedUsername;

// Set once on successful login, persist across navigation cycles
```

This ensures:
1. Authentication happens only once per client session
2. No re-login when navigating back from lobby
3. Username remembered for logging/debugging
