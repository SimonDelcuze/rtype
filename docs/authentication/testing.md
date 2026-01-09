# Authentication Flow Test Guide

This guide walks through testing the complete authentication and navigation flow that was implemented.

## Prerequisites

1. Server is running: `./r-type_server`
2. Client is built: `./r-type_client`

## Test Scenarios

### 1. New User Registration Flow

**Steps:**
1. Launch client: `./r-type_client`
2. **Expected**: Login screen appears (first time in this session)
3. Click "Register" button
4. **Expected**: Registration screen appears
5. Enter username (3-32 chars, alphanumeric + underscore)
6. Enter password (8+ chars with uppercase, lowercase, digit)
   - **Expected**: Password displayed as asterisks (****)
7. Enter confirm password (must match)
   - **Expected**: Password displayed as asterisks (****)
8. Click "Register" button
9. **Expected**: Success message, returns to login screen
10. Login with newly created credentials
11. **Expected**: Proceeds to connection menu (server selection)

### 2. Existing User Login Flow

**Steps:**
1. Launch client: `./r-type_client`
2. **Expected**: Login screen appears
3. Enter existing username
4. Enter password
   - **Expected**: Password displayed as asterisks (****)
5. Click "Login" button
6. **Expected**: Proceeds to connection menu (server selection)

### 3. Back Navigation Flow (Critical Fix)

**Steps:**
1. Login successfully (as per scenario 2)
2. **Expected**: Connection menu appears
3. Either:
   - Click "Use Default" (127.0.0.1:50010), OR
   - Enter server IP/port and click "Connect"
4. **Expected**: Lobby menu appears with room list
5. Click "Back" button
6. **Expected**: Returns to connection menu (NOT crash)
7. Repeat steps 3-6 multiple times
8. **Expected**: Can navigate back and forth without crashes

### 4. Complete Game Flow

**Steps:**
1. Login successfully
2. Select server (connection menu)
3. In lobby menu: Create or join a room
4. Wait for game to start
5. Play the game
6. After game ends (victory/defeat):
   - Click "Retry" to return to lobby
   - OR Click "Quit" to exit

### 5. Password Validation

**Test registration with invalid passwords:**

| Password | Expected Result |
|----------|-----------------|
| `short` | Error: Too short (< 8 chars) |
| `nouppercase123` | Error: Must contain uppercase |
| `NOLOWERCASE123` | Error: Must contain lowercase |
| `NoDigits` | Error: Must contain digit |
| `ValidPass123` | Success |

### 6. Username Validation

**Test registration with invalid usernames:**

| Username | Expected Result |
|----------|-----------------|
| `ab` | Error: Too short (< 3 chars) |
| `this_is_a_very_long_username_that_exceeds_limit` | Error: Too long (> 32 chars) |
| `user@name` | Error: Invalid characters (only alphanumeric + underscore) |
| `valid_user123` | Success |

## Expected Behaviors

### Password Masking
- All password input fields display asterisks (****) instead of actual characters
- Applies to: Login password, Register password, Register confirm password

### Authentication State
- Authentication happens ONCE per client session at startup
- After successful login, user stays authenticated until client restart
- No re-authentication required when navigating back from lobby

### Navigation Loop
- User can cycle between Connection Menu and Lobby Menu indefinitely
- "Back" button in lobby returns to connection menu (not crash/exit)
- Flow: Login → Connection → Lobby → [Back] → Connection → Lobby → ...

### Error Handling
- "Connection error: Unable to reach server" → Check server is running
- "Registration failed: no response from server" → Check server is running
- Mutex crash on "Back" button → FIXED (should not occur)

## Known Fixed Issues

1. ✅ **Password fields showing plain text** → Now masked with asterisks
2. ✅ **Authentication screens not showing** → Now shows login screen on startup
3. ✅ **Connection error during auth** → Fixed by calling `lobbyConn.connect()`
4. ✅ **Mutex crash on Back button** → Fixed with proper cleanup and disconnect
5. ✅ **Back button causes crash instead of navigation** → Fixed with navigation loop

## Server Logs to Monitor

When testing, watch server logs for:
- `[Auth] User authenticated: <username>`
- `[ConnectionFlow] Lobby returned game endpoint: port <port>`
- `[Nav] Returning to connection menu` (when clicking Back)
- `[Net] Disconnected from waiting room: <reason>` (if connection issues)

## Quick Test Commands

```bash
# Terminal 1 - Start server
./r-type_server

# Terminal 2 - Start client
./r-type_client

# To kill server
lsof -ti:50010 | xargs kill -9
```

## Test Database Location

User accounts are stored in: `data/rtype.db`

To reset the database (delete all users):
```bash
rm -f data/rtype.db
./r-type_server  # Will recreate database on startup
```

## Architecture Notes

### Authentication Flow
```
Client Start
    ↓
LoginMenu (once per session)
    ↓ (login successful)
Authentication State Saved (static variables)
    ↓
while (window.isOpen()) {
    ConnectionMenu (server selection)
        ↓
    LobbyMenu (room list)
        ↓ (if Back clicked)
    [Loop back to ConnectionMenu]
}
```

### Key Files
- [client/src/runtime/RunClientFlow.cpp](client/src/runtime/RunClientFlow.cpp:32-96) - Navigation loop
- [client/src/runtime/ConnectionFlow.cpp](client/src/runtime/ConnectionFlow.cpp:26-92) - Auth menu
- [client/src/ui/LoginMenu.cpp](client/src/ui/LoginMenu.cpp) - Login UI
- [client/src/ui/RegisterMenu.cpp](client/src/ui/RegisterMenu.cpp) - Register UI
- [client/include/components/InputFieldComponent.hpp](client/include/components/InputFieldComponent.hpp:25-32) - Password masking
