#pragma once

#include "ecs/ComponentTypeId.hpp"
#include "errors/ComponentNotFoundError.hpp"
#include "errors/RegistryError.hpp"

template <typename... Components> class View;

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

using EntityId = std::uint32_t;

struct ComponentStorageBase
{
    virtual ~ComponentStorageBase()  = default;
    virtual void remove(EntityId id) = 0;
};

template <typename Component> struct ComponentStorage : ComponentStorageBase
{
    static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();
    std::vector<EntityId> dense;
    std::vector<std::size_t> sparse;
    std::vector<Component> data;
    template <typename... Args> Component& emplace(EntityId id, Args&&... args);
    bool contains(EntityId id) const;
    Component& fetch(EntityId id);
    const Component& fetch(EntityId id) const;
    void remove(EntityId id) override;
};

class Registry
{
  public:
    EntityId createEntity();
    void destroyEntity(EntityId id);
    bool isAlive(EntityId id) const;
    void clear();
    EntityId entityCount() const;

    template <typename Component, typename... Args> Component& emplace(EntityId id, Args&&... args);
    template <typename Component> bool has(EntityId id) const;
    template <typename Component> Component& get(EntityId id);
    template <typename Component> const Component& get(EntityId id) const;
    template <typename Component> void remove(EntityId id);

    template <typename... Components> View<Components...> view();

  private:
    template <typename Component> ComponentStorage<Component>* findStorage();
    template <typename Component> const ComponentStorage<Component>* findStorage() const;
    template <typename Component> ComponentStorage<Component>* ensureStorage();

    using SignatureWord                              = std::uint64_t;
    static constexpr std::size_t SIGNATURE_WORD_BITS = sizeof(SignatureWord) * 8;
    std::size_t signatureWordCount_                  = 0;
    std::vector<SignatureWord> signatures_;

    void ensureSignatureWordCount(std::size_t componentIndex);
    void appendSignatureForNewEntity();
    void resetSignature(EntityId id);
    void setSignatureBit(EntityId id, std::size_t componentIndex);
    void clearSignatureBit(EntityId id, std::size_t componentIndex);
    std::size_t signatureIndex(EntityId id, std::size_t word) const;

  public:
    bool hasSignatureBit(EntityId id, std::size_t componentIndex) const;

    std::vector<EntityId> freeIds_;
    std::vector<uint8_t> alive_;
    EntityId nextId_ = 0;
    std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>> storages_;
};

#include "ecs/Registry.tpp"
