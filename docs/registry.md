# ECS Registry Detailed Design

La registry partagée définie dans `shared/include/ecs/Registry.hpp` fournit la fondation ECS commune au client et au serveur. Ce document en décrit précisément les composants internes.

## Cycle de vie des entités

- `EntityId` est un entier 32 bits (`using EntityId = std::uint32_t`).
- `createEntity()` réutilise un ID libre si possible, sinon en alloue un nouveau, marque la case vivante et remet sa signature à zéro.
- `destroyEntity(id)` ignore silencieusement les IDs morts/invalides, sinon marque l’entité morte, supprime tous les composants (via les storages) et place l’ID dans `freeIds_`.
- `isAlive(id)` consulte le tableau `alive_`.
- `clear()` vide les entités, signatures, storages et remet `nextId_` à zéro.

## Identifiants de type de composant

- `ComponentTypeId::value<T>()` retourne un index unique par type grâce à un compteur atomique interne.
- Cet index détermine **le bit** à activer dans la signature d’une entité et permet d’adresser le bon stockage.

## Signatures par entité

- Chaque entité possède un bitset stocké dans `signatures_` (tableau aplati de `uint64_t`).
- `signatureWordCount_` représente le nombre de mots (64 bits) par entité.
- Lorsqu’un nouveau composant apparaît, `ensureSignatureWordCount` agrandit les signatures existantes pour allouer un bit supplémentaire.
- `setSignatureBit`, `clearSignatureBit` et `hasSignatureBit` manipulent les bits. Les méthodes publiques (`emplace`, `has`, `get`, `remove`) s’appuient sur ces helpers pour faire des vérifications O(1) avant d’accéder au stockage.

## Stockage sparse-set des composants

Pour chaque type `T`, la registry maintient un `ComponentStorage<T>` :

- `dense` : liste compacte des entités possédant `T`.
- `data` : tableau des instances `T`, aligné sur `dense`.
- `sparse` : vecteur indexé par `EntityId` qui pointe vers l’index dans `dense` (`npos` signifie “absent”).

### Opérations

- `emplace<T>(entity, args...)` :
  1. Vérifie que l’entité est vivante.
  2. Récupère l’index du composant, agrandit les signatures si nécessaire.
  3. Crée ou récupère le `ComponentStorage<T>`.
  4. Ajoute/mets à jour l’entrée dans `dense/data` (sparse-set).
  5. Active le bit dans la signature de l’entité.
- `has<T>(entity)` : consulte la signature. Si le bit est à zéro, aucun stockage n’est touché.
- `get<T>(entity)` : vérifie `alive` + signature, puis lit `data[sparse[id]]`. Lève `ComponentNotFoundError` si absent.
- `remove<T>(entity)` : s’arrête si l’entité est morte ou si le bit vaut 0. Sinon supprime dans le sparse-set (swap avec la dernière entrée) et éteint le bit.
- `destroyEntity(entity)` : réinitialise tous les mots de sa signature puis appelle `remove` sur chaque stockage via `ComponentStorageBase::remove`.

## API publique et erreurs

| Méthode | Comportement | Exceptions |
| --- | --- | --- |
| `emplace<C>(EntityId, Args&&...)` | Ajoute/remplace `C`, retourne la référence stockée. | `RegistryError` (entité morte) |
| `has<C>(EntityId)` | Vérifie la présence de `C`. | Aucune |
| `get<C>(EntityId)` / `const get` | Récupère `C`. | `RegistryError` (entité morte) ou `ComponentNotFoundError` |
| `remove<C>(EntityId)` | Supprime `C` si présent. | Aucune |

Erreurs (dans `shared/include/errors/`) :
- `IError` : interface de base.
- `RegistryError` : usage invalide de la registry (entité morte, etc.).
- `ComponentNotFoundError` : composant demandé absent.

## Tests

`tests/shared/ECS/RegistryTests.cpp` et `MovementComponentTests.cpp` couvrent :
- Recyclage d’IDs et suppression automatique des composants.
- Bon fonctionnement de `emplace/get/remove`.
- Exceptions lors des accès à des entités mortes ou à des composants manquants.
- Stockage sparse-set via les tests de composant (vérification des données).

## Exemple

```cpp
Registry registry;
EntityId e = registry.createEntity();

registry.emplace<Position>(e, 10.f, 20.f);
registry.emplace<Health>(e, 100);

if (registry.has<Position>(e)) {
    auto& pos = registry.get<Position>(e);
    // ...
}

registry.remove<Health>(e);
registry.destroyEntity(e);
```

## Remarques

- La registry n’est pas thread-safe. Toute mutation simultanée nécessite une synchronisation externe.
- Les signatures et le sparse-set sont les fondations requises pour des `view<Component...>` et un scheduler de systèmes : ils seront ajoutés dans les étapes suivantes mais reposent entièrement sur cette infrastructure partagée.
