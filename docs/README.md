# 📚 Documentation R-Type

Bienvenue dans la documentation officielle du projet **R-Type**. Ce guide est conçu pour vous aider à comprendre l'architecture, le fonctionnement et les protocoles de notre moteur de jeu multijoueur.

---

## 🗺️ Navigation Rapide

Pour une exploration efficace, la documentation est divisée en 5 piliers majeurs :

### 🚀 [Installation & Setup](installation/README.md)
Tout ce dont vous avez besoin pour compiler et lancer le projet sur n'importe quelle plateforme.
- Guide d'installation rapide
- Configuration multi-instance

### 🏗️ [Architecture Globale](architecture/README.md)
Le cœur technique du projet.
- **ECS (Entity Component System)** : Notre moteur maison ultra-performant.
- **Core Components** : Les briques de base de l'engine.
- **JSON Wrapper** : Gestion flexible des données.

### 🌐 [Network & Protocoles](network/README.md)
Comment le client et le serveur communiquent.
- **Protocoles UDP/TCP** : Détails des paquets et de la synchronisation.
- **Authentification** : Système de lobby et sécurité des comptes.
- **Collision Masks** : Gestion des masques de collision réseau.

### 🖥️ [Serveur (Authoritative)](server/README.md)
La logique "Master" du jeu.
- **Game Instance Management** : Gestion dynamique des parties.
- **Systems & Components** : Logique serveur pure.
- **Level System** : Design et runtime des niveaux.

### 🕹️ [Client (Visuals & Prediction)](client/README.md)
L'expérience utilisateur.
- **Rendering Pipeline** : Gestion des graphismes SFML 3.0.
- **Prediction & Reconciliation** : Comment nous gérons la latence.
- **UI Module** : Architecture des menus et interactions.

---

## 🛠️ Outils Supplémentaires

- **[Level Editor](architecture/README.md)** : Documentation sur l'outil de création de niveaux.
- **[Comparative Study](architecture/comparative-study.md)** : Analyse technique des choix d'implémentation.

---

<p align="center">
  <i>Besoin d'aide ? Consultez notre <a href="../README.md">README principal</a> ou ouvrez une issue sur GitHub.</i>
</p>
