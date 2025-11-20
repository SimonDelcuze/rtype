#include "ecs/Registry.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

EntityId Registry::createEntity()
{
    if (!freeIds_.empty()) {
        const EntityId id = freeIds_.back();
        freeIds_.pop_back();
        alive_[id] = true;
        resetSignature(id);
        return id;
    }
    const EntityId id = nextId_++;
    alive_.push_back(true);
    appendSignatureForNewEntity();
    return id;
}

void Registry::destroyEntity(EntityId id)
{
    if (id >= alive_.size() || !alive_[id]) {
        return;
    }
    alive_[id] = false;
    resetSignature(id);
    freeIds_.push_back(id);
    for (auto& [_, storage] : storages_) {
        storage->remove(id);
    }
}

bool Registry::isAlive(EntityId id) const
{
    return id < alive_.size() && alive_[id];
}

void Registry::clear()
{
    storages_.clear();
    freeIds_.clear();
    alive_.clear();
    signatures_.clear();
    nextId_ = 0;
}

void Registry::ensureSignatureWordCount(std::size_t componentIndex)
{
    const std::size_t requiredWords = componentIndex / SIGNATURE_WORD_BITS + 1;
    if (requiredWords <= signatureWordCount_)
        return;
    const std::size_t oldCount = signatureWordCount_;
    signatureWordCount_        = requiredWords;
    std::vector<SignatureWord> newSignatures(alive_.size() * signatureWordCount_, 0);
    for (EntityId id = 0; id < alive_.size(); ++id) {
        for (std::size_t word = 0; word < oldCount; ++word) {
            newSignatures[id * signatureWordCount_ + word] = signatures_[id * oldCount + word];
        }
    }
    signatures_ = std::move(newSignatures);
}

void Registry::appendSignatureForNewEntity()
{
    if (signatureWordCount_ == 0)
        return;
    signatures_.resize(signatures_.size() + signatureWordCount_, 0);
}

void Registry::resetSignature(EntityId id)
{
    if (signatureWordCount_ == 0 || id >= alive_.size())
        return;
    const std::size_t offset = signatureIndex(id, 0);
    auto* start              = signatures_.data() + offset;
    std::fill_n(start, signatureWordCount_, SignatureWord{0});
}

void Registry::setSignatureBit(EntityId id, std::size_t componentIndex)
{
    if (signatureWordCount_ == 0 || id >= alive_.size())
        return;
    const std::size_t word = componentIndex / SIGNATURE_WORD_BITS;
    const std::size_t bit  = componentIndex % SIGNATURE_WORD_BITS;
    signatures_[signatureIndex(id, word)] |= (SignatureWord{1} << bit);
}

void Registry::clearSignatureBit(EntityId id, std::size_t componentIndex)
{
    if (signatureWordCount_ == 0 || id >= alive_.size())
        return;
    const std::size_t word = componentIndex / SIGNATURE_WORD_BITS;
    const std::size_t bit  = componentIndex % SIGNATURE_WORD_BITS;
    signatures_[signatureIndex(id, word)] &= ~(SignatureWord{1} << bit);
}

bool Registry::hasSignatureBit(EntityId id, std::size_t componentIndex) const
{
    if (signatureWordCount_ == 0 || id >= alive_.size())
        return false;
    const std::size_t word = componentIndex / SIGNATURE_WORD_BITS;
    if (word >= signatureWordCount_)
        return false;
    const std::size_t bit = componentIndex % SIGNATURE_WORD_BITS;
    return (signatures_[signatureIndex(id, word)] & (SignatureWord{1} << bit)) != 0;
}

std::size_t Registry::signatureIndex(EntityId id, std::size_t word) const
{
    return id * signatureWordCount_ + word;
}
