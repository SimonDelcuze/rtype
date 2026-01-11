# Système d'Authentification R-Type

## Vue d'ensemble

Système complet d'authentification utilisateur avec :
- Base de données SQLite3
- Hachage de mots de passe PBKDF2 (équivalent bcrypt coût 12)
- Tokens JWT avec expiration 7 jours
- Menus client (Login/Register)
- Validation complète côté serveur

## Architecture

### Backend (Serveur)

#### 1. Base de Données (`server/src/auth/`)

**Database.hpp/cpp** - Wrapper SQLite3
```cpp
class Database {
    bool initialize(const std::string& dbPath);  // Ouvre "data/rtype.db"
    bool executeScript(const std::string& sql);   // Exécute migration
    std::optional<PreparedStatement> prepare(const std::string& sql);
    std::unique_ptr<Transaction> beginTransaction();
};
```

**Schéma SQL** (`001_initial_schema.sql`)
```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    last_login INTEGER
);

CREATE TABLE user_stats (
    user_id INTEGER PRIMARY KEY,
    games_played INTEGER DEFAULT 0,
    wins INTEGER DEFAULT 0,
    losses INTEGER DEFAULT 0,
    total_score INTEGER DEFAULT 0
);

CREATE TABLE session_tokens (
    id INTEGER PRIMARY KEY,
    user_id INTEGER NOT NULL,
    token_hash TEXT NOT NULL,
    expires_at INTEGER NOT NULL,
    created_at INTEGER NOT NULL
);
```

#### 2. Service d'Authentification (`server/src/auth/AuthService.cpp`)

```cpp
class AuthService {
    // Hachage PBKDF2 avec 4096 itérations (équivalent bcrypt cost 12)
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);

    // JWT avec HS256, expiration 7 jours
    std::string generateJWT(uint32_t userId, const std::string& username);
    std::optional<JWTPayload> validateJWT(const std::string& token);

    // SHA-256 pour stockage tokens
    std::string hashToken(const std::string& token);
};
```

#### 3. Repository Utilisateurs (`server/src/auth/UserRepository.cpp`)

```cpp
class UserRepository {
    // CRUD Utilisateurs
    std::optional<uint32_t> createUser(username, passwordHash);
    std::optional<User> getUserByUsername(username);
    std::optional<User> getUserById(userId);
    bool updateLastLogin(userId);
    bool updatePassword(userId, newPasswordHash);

    // Gestion Sessions
    bool storeSessionToken(userId, tokenHash, expiresAt);
    bool validateSessionToken(userId, tokenHash);
    void cleanupExpiredTokens();

    // Statistiques
    std::optional<UserStats> getUserStats(userId);
    bool updateUserStats(userId, games, wins, losses, score);
};
```

#### 4. Intégration LobbyServer (`server/src/lobby/LobbyServer.cpp`)

**Initialisation** (constructeur, lignes 22-43)
```cpp
// Initialise la base de données
database_ = std::make_shared<Database>();
database_->initialize("data/rtype.db");

// Charge le schéma SQL
std::ifstream schemaFile("server/src/auth/migrations/001_initial_schema.sql");
database_->executeScript(schema);

// Services d'authentification
userRepository_ = std::make_shared<UserRepository>(database_);
authService_ = std::make_shared<AuthService>("rtype-jwt-secret-key");
```

**Handlers de paquets** (lignes 377-535)
- `handleLoginRequest()` - Vérifie credentials, génère JWT
- `handleRegisterRequest()` - Valide et crée compte
- `handleChangePasswordRequest()` - Changement sécurisé avec JWT

**Session enrichie** (`server/include/core/Session.hpp:22-26`)
```cpp
struct ClientSession {
    // ... champs existants ...

    // Authentification
    bool authenticated = false;
    std::optional<uint32_t> userId;
    std::string username;
    std::string jwtToken;
};
```

### Protocole Réseau (Shared)

#### MessageType (0x50-0x58) `shared/include/network/PacketHeader.hpp:46-54`

```cpp
AuthLoginRequest           = 0x50,
AuthLoginResponse          = 0x51,
AuthRegisterRequest        = 0x52,
AuthRegisterResponse       = 0x53,
AuthChangePasswordRequest  = 0x54,
AuthChangePasswordResponse = 0x55,
AuthTokenRefreshRequest    = 0x56,
AuthTokenRefreshResponse   = 0x57,
AuthRequired               = 0x58,
```

#### Structures de Paquets (`shared/include/network/AuthPackets.hpp`)

```cpp
enum class AuthErrorCode : uint8_t {
    Success = 0x00,
    InvalidCredentials = 0x01,
    UsernameTaken = 0x02,
    WeakPassword = 0x03,
    InvalidToken = 0x04,
    ServerError = 0x05,
    Unauthorized = 0x06
};

struct LoginRequestData {
    std::string username;
    std::string password;
};

struct LoginResponseData {
    bool success;
    uint32_t userId;
    std::string token;  // JWT
    AuthErrorCode errorCode;
};

struct RegisterRequestData {
    std::string username;
    std::string password;
};

struct RegisterResponseData {
    bool success;
    uint32_t userId;
    AuthErrorCode errorCode;
};
```

### Frontend (Client)

#### 1. Connexion Réseau (`client/src/network/LobbyConnection.cpp:134-171`)

```cpp
class LobbyConnection {
    std::optional<LoginResponseData> login(
        const std::string& username,
        const std::string& password
    );

    std::optional<RegisterResponseData> registerUser(
        const std::string& username,
        const std::string& password
    );

    std::optional<ChangePasswordResponseData> changePassword(
        const std::string& oldPassword,
        const std::string& newPassword,
        const std::string& token
    );
};
```

#### 2. Menu Login (`client/src/ui/LoginMenu.cpp`)

**Interface utilisateur:**
- Logo R-Type
- Champs: Username, Password (masqué avec des astérisques)
- Boutons: Login, Register, Quit
- Message d'erreur dynamique

**Validation:**
```cpp
void handleLoginAttempt(Registry& registry) {
    // Récupère username/password des InputFields
    // Validation champs vides
    // Appel lobbyConn_.login()
    // Gestion réponse:
    //   - Success: stocke token + userId + username
    //   - Failure: affiche message d'erreur approprié
}
```

#### 3. Menu Register (`client/src/ui/RegisterMenu.cpp`)

**Interface utilisateur:**
- Logo R-Type
- Champs: Username, Password (masqué), Confirm Password (masqué)
- Boutons: Register, Back to Login, Quit
- Message d'erreur dynamique

**Validation côté client:**
```cpp
bool validatePassword(const std::string& password, std::string& errorMsg) {
    // Longueur: 8-64 caractères
    // Doit contenir: majuscule, minuscule, chiffre
    // Messages d'erreur spécifiques
}

void handleRegisterAttempt(Registry& registry) {
    // Validation username: 3-32 chars, alphanumerique + underscore
    // Validation password: validatePassword()
    // Vérification: password == confirmPassword
    // Appel lobbyConn_.registerUser()
    // Gestion erreurs: UsernameTaken, WeakPassword, ServerError
}
```

## Règles de Validation

### Username
- **Longueur**: 3-32 caractères
- **Caractères autorisés**: lettres, chiffres, underscore
- **Unique**: vérifié en base de données
- **Case sensitive**: oui

### Password
- **Longueur**: 8-64 caractères
- **Complexité requise**:
  - Au moins une majuscule
  - Au moins une minuscule
  - Au moins un chiffre
- **Stockage**: PBKDF2 hash (jamais en clair)

### JWT Token
- **Algorithme**: HS256
- **Durée de vie**: 7 jours
- **Contenu**:
  ```json
  {
    "iss": "rtype-server",
    "userId": "123",
    "username": "player",
    "iat": 1234567890,
    "exp": 1235172690
  }
  ```

## Flux d'Utilisation

### 1. Inscription

```
Client                    LobbyServer              Database
  |                           |                        |
  |-- RegisterRequest ------->|                        |
  |   (username, password)    |                        |
  |                           |-- Validate username -->|
  |                           |<-- Check uniqueness ---|
  |                           |                        |
  |                           |-- Hash password        |
  |                           |-- INSERT user -------->|
  |                           |<-- userId -------------|
  |                           |-- INSERT user_stats -->|
  |<-- RegisterResponse ------|                        |
  |   (success, userId)       |                        |
```

### 2. Connexion

```
Client                    LobbyServer              Database
  |                           |                        |
  |-- LoginRequest ---------->|                        |
  |   (username, password)    |                        |
  |                           |-- SELECT user -------->|
  |                           |<-- User data ----------|
  |                           |                        |
  |                           |-- Verify password      |
  |                           |-- Generate JWT         |
  |                           |-- UPDATE last_login -->|
  |                           |                        |
  |<-- LoginResponse ---------|                        |
  |   (success, userId,       |                        |
  |    token, errorCode)      |                        |
```

### 3. Changement de Mot de Passe

```
Client                    LobbyServer              Database
  |                           |                        |
  |-- ChangePasswordRequest ->|                        |
  |   (oldPwd, newPwd, token) |                        |
  |                           |-- Validate JWT         |
  |                           |-- SELECT user -------->|
  |                           |<-- User data ----------|
  |                           |                        |
  |                           |-- Verify oldPassword   |
  |                           |-- Hash newPassword     |
  |                           |-- UPDATE password ---->|
  |                           |                        |
  |<-- ChangePasswordResponse-|                        |
  |   (success, errorCode)    |                        |
```

## Sécurité

### Mesures Implémentées

1. **Mots de passe**
   - ✅ Hachage PBKDF2 avec 4096 itérations
   - ✅ Salt unique par utilisateur (dans le hash)
   - ✅ Jamais stockés ou loggés en clair
   - ✅ Validation robuste côté client et serveur

2. **Tokens JWT**
   - ✅ Signature HS256
   - ✅ Expiration 7 jours
   - ✅ Validation sur chaque requête sensible
   - ✅ Payload minimal (userId, username)

3. **Base de données**
   - ✅ Prepared statements (protection SQL injection)
   - ✅ Mode WAL pour meilleures performances
   - ✅ Foreign keys activées
   - ✅ Indexes sur colonnes fréquemment requêtées

4. **Protocole réseau**
   - ✅ CRC32 sur chaque paquet
   - ✅ Numéros de séquence
   - ✅ Validation taille et format

### Recommandations Production

⚠️ **Avant déploiement:**

1. **Changer le secret JWT**
   ```cpp
   // Dans LobbyServer.cpp:40
   authService_ = std::make_shared<AuthService>("CHANGEZ_CETTE_CLE_SECRETE");
   ```
   Utiliser une clé aléatoire de 32+ caractères.

2. **Configurer permissions fichiers**
   ```bash
   chmod 600 data/rtype.db
   chown rtype:rtype data/rtype.db
   ```

3. **Rate limiting** (à implémenter)
   - Limiter tentatives login: 5/minute par IP
   - Limiter registrations: 3/heure par IP

4. **Monitoring**
   - Logger tentatives échouées
   - Alertes sur activité suspecte
   - Rotation logs

5. **Backup base de données**
   ```bash
   # Cron job quotidien
   sqlite3 data/rtype.db ".backup 'data/rtype-$(date +%Y%m%d).db'"
   ```

## Tests

### Test Manuel

**1. Lancer le serveur**
```bash
./r-type_server
```

**2. Lancer le client**
```bash
./r-type_client
```

**3. Scénario de test**

a) **Inscription**
   - Username: `testuser`
   - Password: `Test1234`
   - Confirm: `Test1234`
   - ✅ Devrait réussir

b) **Inscription invalide**
   - Username: `ab` (trop court)
   - ❌ Erreur: "Username must be 3-32 characters"

   - Password: `weak` (trop simple)
   - ❌ Erreur: "Password must contain uppercase, lowercase, and digit"

c) **Connexion**
   - Username: `testuser`
   - Password: `Test1234`
   - ✅ Devrait réussir et afficher lobby

d) **Connexion invalide**
   - Username: `testuser`
   - Password: `wrongpass`
   - ❌ Erreur: "Invalid username or password"

e) **Username déjà pris**
   - Créer nouveau compte avec même username
   - ❌ Erreur: "Username is already taken"

### Vérification Base de Données

```bash
sqlite3 data/rtype.db

# Lister utilisateurs
SELECT id, username, created_at, last_login FROM users;

# Vérifier hash password (ne doit JAMAIS être en clair)
SELECT password_hash FROM users WHERE username='testuser';
# Format attendu: "<salt>:<hash>" (128 caractères hexadécimaux)

# Statistiques utilisateur
SELECT * FROM user_stats WHERE user_id=1;

# Tokens de session
SELECT user_id, expires_at, created_at FROM session_tokens;
```

## Fichiers Importants

### Serveur
```
server/
├── include/auth/
│   ├── Database.hpp            # Wrapper SQLite
│   ├── AuthService.hpp         # Hachage + JWT
│   ├── UserRepository.hpp      # CRUD users
│   └── User.hpp                # Models
├── src/auth/
│   ├── Database.cpp
│   ├── AuthService.cpp
│   ├── UserRepository.cpp
│   └── migrations/
│       └── 001_initial_schema.sql
└── src/lobby/
    └── LobbyServer.cpp         # Handlers auth (L377-535)
```

### Client
```
client/
├── include/
│   ├── network/
│   │   └── LobbyConnection.hpp # Méthodes auth (L33-36)
│   └── ui/
│       ├── LoginMenu.hpp
│       └── RegisterMenu.hpp
└── src/
    ├── network/
    │   └── LobbyConnection.cpp # Implémentation (L134-171)
    └── ui/
        ├── LoginMenu.cpp       # Menu login complet
        └── RegisterMenu.cpp    # Menu register complet
```

### Shared
```
shared/include/
├── network/
│   ├── PacketHeader.hpp        # MessageTypes (L46-54)
│   └── AuthPackets.hpp         # Structures auth
├── security/
│   └── JWTPayload.hpp          # JWT payload
└── src/network/
    └── AuthPackets.cpp         # Encodage/décodage paquets
```

## Dépendances

```cmake
# CMakeLists.txt additions:
find_package(SQLite3 REQUIRED)
CPMAddPackage(NAME jwt-cpp GITHUB_REPOSITORY Thalhammer/jwt-cpp GIT_TAG v0.7.0)
find_package(OpenSSL REQUIRED)

target_link_libraries(rtype_server_lib
    SQLite::SQLite3
    jwt-cpp::jwt-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
)
```

## Logs Serveur

Les logs montrent l'activité d'authentification:

```
[LobbyServer] Authentication system initialized
[LobbyServer] Register request from client
[LobbyServer] User testuser registered successfully
[LobbyServer] Login request from client
[LobbyServer] User testuser logged in successfully
[LobbyServer] Login failed: invalid password for testuser
```

## Intégration Client Complétée

Le système d'authentification est maintenant complètement intégré dans le flux client :

1. **Flux d'exécution** : LoginMenu → ConnectionMenu → LobbyMenu → WaitingRoom → Game
2. **Fichiers modifiés** :
   - `client/src/runtime/RunClientFlow.cpp` - Ajout de l'authentification avant ConnectionMenu
   - `client/src/runtime/ConnectionFlow.cpp` - Fonction `showAuthenticationMenu()`
   - `client/include/auth/AuthResult.hpp` - Structure de résultat d'authentification

3. **Comportement** :
   - Au démarrage du client, l'utilisateur voit d'abord l'écran de login
   - Si l'utilisateur n'a pas de compte, il peut cliquer sur "Register"
   - Après inscription réussie, l'utilisateur retourne au menu login
   - Après connexion réussie, l'utilisateur passe au ConnectionMenu (ou directement au lobby en mode default)

## Prochaines Étapes

### À implémenter (optionnel)

1. **Remember Me**
   - Stocker token JWT dans `~/.config/rtype/auth.json`
   - Auto-login au démarrage
   - Classe `ClientAuthConfig` pour gestion

2. **Profile Menu**
   - Afficher statistiques utilisateur
   - Username, games played, win/loss ratio
   - Bouton "Change Password"
   - Bouton "Logout"

3. **Intégration Flow Client**
   - Ajouter LoginMenu AVANT ConnectionMenu
   - Passer token JWT aux requêtes lobby
   - Gérer déconnexion/expiration token

4. **Rate Limiting**
   - Table `rate_limits` en DB
   - Limiter tentatives par IP
   - Cleanup automatique

5. **Email Verification** (avancé)
   - Table `email_verifications`
   - SMTP pour envoi emails
   - Liens de vérification

6. **Two-Factor Auth** (avancé)
   - TOTP avec QR codes
   - Table `totp_secrets`
   - Backup codes

## Support

Pour questions ou problèmes:
- Vérifier logs serveur dans terminal
- Vérifier `data/rtype.db` existe et a bonnes permissions
- Tester avec `sqlite3 data/rtype.db` en CLI
- Recompiler: `make clean && make`
