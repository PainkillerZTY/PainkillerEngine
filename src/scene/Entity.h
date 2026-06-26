#pragma once

#include "Component.h"
#include <string>
#include <unordered_map>

namespace nebula {

class Entity {
public:
    Entity() = default;
    explicit Entity(EntityID id, const std::string& name = "Entity")
        : m_id(id), m_name(name) {}

    EntityID GetID() const { return m_id; }
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    // Component mask for quick queries
    u32 GetComponentMask() const { return m_componentMask; }
    bool HasComponent(ComponentType type) const {
        return (m_componentMask & (1u << static_cast<u32>(type))) != 0;
    }
    void AddComponentFlag(ComponentType type) {
        m_componentMask |= (1u << static_cast<u32>(type));
    }
    void RemoveComponentFlag(ComponentType type) {
        m_componentMask &= ~(1u << static_cast<u32>(type));
    }

    bool IsValid() const { return m_id != kInvalidEntity; }

private:
    EntityID m_id = kInvalidEntity;
    std::string m_name = "Entity";
    u32 m_componentMask = 0;
};

} // namespace nebula
