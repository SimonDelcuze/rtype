# 👤 Système de Profil Utilisateur - Implémenté

## ✅ Fonctionnalité Ajoutée

Le système de **profil utilisateur** est maintenant implémenté et fonctionnel ! Il permet d'afficher les statistiques de jeu pour chaque joueur authentifié.

## 📊 Statistiques Disponibles

Votre profil affiche les informations suivantes :

- **Username** : Votre nom d'utilisateur
- **User ID** : Votre identifiant unique
- **Games Played** : Nombre total de parties jouées
- **Wins** : Nombre de victoires
- **Losses** : Nombre de défaites
- **Win Rate** : Pourcentage de victoires (calculé automatiquement)
- **Total Score** : Score cumulé sur toutes vos parties

## 🎯 Comment Accéder à Votre Profil

### Méthode Programmatique (Pour les développeurs)

Le ProfileMenu est maintenant disponible et peut être affiché n'importe où dans le client. Voici comment l'utiliser :

```cpp
#include "ui/ProfileMenu.hpp"

// Dans votre menu (LobbyMenu, etc.)
ProfileMenu profileMenu(fontManager, textureManager, lobbyConnection, username, userId);
Registry registry;
profileMenu.create(registry);

// Le menu affichera automatiquement :
// - Un overlay semi-transparent
// - Un panneau centré avec toutes les stats
// - Un bouton "Back" pour fermer
```

### Architecture Technique

#### 1. **Protocole Réseau**

Nouveau MessageType ajouté :
- `AuthGetStatsRequest` (0x59) : Client demande ses stats
- `AuthGetStatsResponse` (0x5A) : Serveur envoie les stats

#### 2. **Fichiers Créés**

**Shared (Protocole)**:
- [shared/include/network/StatsPackets.hpp](shared/include/network/StatsPackets.hpp) - Packets pour stats

**Server (Backend)**:
- Handler ajouté dans [server/src/lobby/LobbyServer.cpp](server/src/lobby/LobbyServer.cpp:542-599) - `handleGetStatsRequest()`

**Client (Frontend)**:
- [client/include/ui/ProfileMenu.hpp](client/include/ui/ProfileMenu.hpp) - Interface du menu profil
- [client/src/ui/ProfileMenu.cpp](client/src/ui/ProfileMenu.cpp) - Implémentation du menu
- [client/include/network/LobbyConnection.hpp](client/include/network/LobbyConnection.hpp:40) - Méthode `getStats()`
- [client/src/network/LobbyConnection.cpp](client/src/network/LobbyConnection.cpp:173-183) - Implémentation

## 🔧 Intégration dans un Menu

### Exemple : Ajouter un Bouton "Profile" dans LobbyMenu

Voici comment vous pouvez ajouter un bouton pour afficher le profil dans n'importe quel menu :

```cpp
// Dans LobbyMenu.cpp, ajoutez un bouton:
profileButtonEntity_ = createButton(registry, 1000.0F, 320.0F, 150.0F, 50.0F, "Profile", Color(100, 150, 200),
                                     [this]() { onProfileClicked(); });

// Ajoutez la méthode dans LobbyMenu:
void LobbyMenu::onProfileClicked()
{
    Logger::instance().info("[LobbyMenu] Profile button clicked");

    // Créer et afficher le ProfileMenu
    ProfileMenu profileMenu(fonts_, textures_, *lobbyConnection_, currentUsername_, currentUserId_);
    Registry profileRegistry;
    profileMenu.create(profileRegistry);

    // Boucle d'événements pour le profile
    while (window_.isOpen() && !profileMenu.isDone()) {
        window_.pollEvents([&](const Event& event) {
            profileMenu.handleEvent(profileRegistry, event);
            // Gérer ButtonSystem, etc.
        });

        window_.clear();
        profileMenu.render(profileRegistry, window_);
        window_.display();
    }

    profileMenu.destroy(profileRegistry);
}
```

## 🗄️ Base de Données

Les statistiques sont stockées dans la table `user_stats` :

```sql
CREATE TABLE user_stats (
    user_id INTEGER PRIMARY KEY,
    games_played INTEGER NOT NULL DEFAULT 0,
    wins INTEGER NOT NULL DEFAULT 0,
    losses INTEGER NOT NULL DEFAULT 0,
    total_score INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY(user_id) REFERENCES users(id)
);
```

### Mise à Jour des Stats

Les stats sont mises à jour automatiquement par le serveur à la fin de chaque partie via `UserRepository::updateUserStats()`.

## 🎨 Design du ProfileMenu

Le menu de profil utilise un design modal avec :

### Layout
```
┌─────────────────────────────────────────────────┐
│  [Overlay semi-transparent noir (180 alpha)]    │
│                                                  │
│  ┌──────────────────────────────────────────┐   │
│  │       USER PROFILE                       │   │
│  │                                          │   │
│  │  Username: charliinew                    │   │
│  │  User ID: 1                              │   │
│  │                                          │   │
│  │  Games Played: 15                        │   │
│  │  Wins: 8                                 │   │
│  │  Losses: 7                               │   │
│  │  Win Rate: 53.3%                         │   │
│  │  Total Score: 125000                     │   │
│  │                                          │   │
│  │            [Back]                        │   │
│  └──────────────────────────────────────────┘   │
│                                                  │
└─────────────────────────────────────────────────┘
```

### Couleurs
- **Background Overlay** : Noir semi-transparent (0, 0, 0, 180)
- **Panel** : Gris foncé (30, 30, 40)
- **Text** : Blanc (255, 255, 255)
- **Back Button** : Gris (100, 100, 100)

## 🔐 Sécurité

- ✅ **Authentification requise** : `handleGetStatsRequest()` vérifie que l'utilisateur est authentifié
- ✅ **Session validation** : Utilise le JWT token de la session
- ✅ **User isolation** : Chaque user ne peut voir que ses propres stats
- ✅ **Stats par défaut** : Si aucune stats n'existe (nouveau user), retourne des stats à 0

## 📡 Flux de Requête

```
Client (ProfileMenu)                         Server (LobbyServer)
       │                                            │
       │  1. getStats()                            │
       │───────────────────────────────────────────>│
       │                                            │
       │                                     2. Vérifie auth
       │                                     3. getUserStats(userId)
       │                                     4. Construit réponse
       │                                            │
       │  5. GetStatsResponseData                  │
       │<───────────────────────────────────────────│
       │                                            │
  6. Affiche stats                                 │
```

## 🧪 Test du Profil

### Test Manuel

1. Démarrer le serveur :
```bash
./r-type_server
```

2. Lancer le client et se connecter :
```bash
./r-type_client
```

3. Pour tester le ProfileMenu programmatiquement, vous pouvez créer un test simple :

```cpp
// Dans n'importe quel menu après authentication
LobbyConnection lobbyConn(lobbyEndpoint);
lobbyConn.connect();

// Requête des stats
auto stats = lobbyConn.getStats();
if (stats.has_value()) {
    std::cout << "Games Played: " << stats->gamesPlayed << std::endl;
    std::cout << "Wins: " << stats->wins << std::endl;
    std::cout << "Losses: " << stats->losses << std::endl;
    std::cout << "Total Score: " << stats->totalScore << std::endl;
}
```

### Logs Serveur

Quand un client demande ses stats, le serveur affiche :
```
[LobbyServer] Get stats request from client
[LobbyServer] Sending stats for user charliinew: games=15, wins=8
```

### Logs Client

Le client affiche :
```
[ProfileMenu] Fetching stats for user charliinew
[ProfileMenu] Stats loaded: games=15, wins=8
```

## 📝 TODO: Intégration UI Complète

Pour rendre le profil accessible à l'utilisateur final, il reste à :

1. ✅ **Protocole réseau** - Implémenté
2. ✅ **Backend serveur** - Implémenté
3. ✅ **Frontend ProfileMenu** - Implémenté
4. ⏳ **Bouton dans LobbyMenu** - À ajouter (voir exemple ci-dessus)
5. ⏳ **Test utilisateur** - À faire

Le code est prêt, il suffit d'ajouter un bouton "Profile" dans le LobbyMenu ou n'importe quel autre menu pour que l'utilisateur puisse cliquer et voir ses stats !

## 🎯 Prochaines Étapes Suggérées

1. **Ajouter un bouton "Profile" dans LobbyMenu**
   - Position suggérée : À côté du bouton "Back"
   - Utiliser la méthode `onProfileClicked()` ci-dessus

2. **Alternative simple : Afficher les stats dans le lobby**
   - Au lieu d'un menu modal, afficher les stats directement dans un coin du LobbyMenu
   - Exemple : "Wins: 8 | Losses: 7 | Win Rate: 53.3%"

3. **Tester avec de vraies parties**
   - Jouer quelques parties
   - Vérifier que les stats se mettent à jour correctement
   - Tester avec plusieurs utilisateurs

## 🎨 Personnalisation

Vous pouvez facilement personnaliser le ProfileMenu :

- **Couleurs** : Modifier dans `ProfileMenu.cpp`
- **Position** : Ajuster `panelTransform.x` et `panelTransform.y`
- **Taille** : Modifier `panelBox.width` et `panelBox.height`
- **Stats supplémentaires** : Ajouter dans `UserStats` struct et database

## 📚 Références

- **Protocol**: [shared/include/network/PacketHeader.hpp](shared/include/network/PacketHeader.hpp:55-56)
- **Server Handler**: [server/src/lobby/LobbyServer.cpp](server/src/lobby/LobbyServer.cpp:542-599)
- **Client UI**: [client/src/ui/ProfileMenu.cpp](client/src/ui/ProfileMenu.cpp)
- **Database Schema**: [server/src/auth/migrations/001_initial_schema.sql](server/src/auth/migrations/001_initial_schema.sql)

---

✅ **Le système est fonctionnel et prêt à l'emploi !**

Pour voir votre profil, ajoutez simplement un bouton dans votre menu préféré ou utilisez directement `ProfileMenu` comme montré dans les exemples ci-dessus.
