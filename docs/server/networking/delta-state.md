#Delta - State Packet

Paquet binaire serveur → client contenant uniquement les entités modifiées depuis le dernier tick (mouvements, spawns, destructions implicites via absence ou message dédié).

***

## Format global

```
[PacketHeader] (15 bytes: magic + version + packetType + messageType + sequenceId + tickId + payloadSize)
[entryCount]  (uint16, big-endian)
[entries...]  (entryCount times 24 bytes)
```

`packetType` doit être `ServerToClient`, `messageType` = Snapshot, `payloadSize` = `2 + 24 * entryCount`. Taille minimale: 17 octets. Pas de padding implicite.

***

## Entrée delta (24 bytes, big-endian)

| Champ     | Type      | Taille | Description                                      |
|-----------|-----------|--------|--------------------------------------------------|
| `entityId`| `uint32`  | 4      | Identifiant unique de l'entité                   |
| `pos.x`   | `float32` | 4      | Position X                                       |
| `pos.y`   | `float32` | 4      | Position Y                                       |
| `vel.x`   | `float32` | 4      | Vitesse X                                        |
| `vel.y`   | `float32` | 4      | Vitesse Y                                        |
| `hp`      | `int32`   | 4      | Points de vie courants (≤ 0 si mort)             |

Encode float via bits IEEE 754 (bit_cast). Tous les champs multioctets sont en big-endian.

***

## Règles d'encodage/décodage

- `messageType` = `Snapshot`.
- `entryCount` = nombre d'entrées encodées; si 0, le payload s'arrête après les 2 octets de compte.
- Refuser tout paquet dont la taille ≠ `7 + 2 + entryCount * 24`.
- Décoder en ignorant toute entrée partielle (ne pas dépasser le buffer).
- Le protocole ne transmet pas ici de destructions explicites;
une entité absente peut être considérée comme inchangée.Un message dédié “destroy” peut coexister si nécessaire.

            ***

        ##C++(shared)

            Emplacement : `shared
        / include / network / DeltaStatePacket.hpp`

    - `DeltaEntry` struct(fields ci - dessus).- `DeltaStatePacket
{
    PacketHeader header;
    std::vector<DeltaEntry> entries;
    encode() / decode()
}
` - `encode()` retourne `std::vector<std::uint8_t>` prêt à envoyer
        .- `decode()` retourne `std::optional<DeltaStatePacket>` en validant la taille.

               ***

           ##Intégration

    - Le serveur construit `entries` à partir du DirtyTracker(mouvements / spawns).-
    Le client applique les entrées aux entités existantes ou crée si absent(selon sa logique de réplication)
        .

            ***

    ##Compatibilité

    - Big - endian partout,
    tailles fixes, aucun padding ou `sizeof` implicite.- Identique Windows / Linux / macOS.
