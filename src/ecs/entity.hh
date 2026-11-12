#pragma once
#include <cstdint>

class Registry;

using EntityID = uint32_t;

class Entity
{
public:
  constexpr Entity() = default;
  constexpr Entity(Registry &registry, EntityID id) : m_registry(&registry), m_id(id) {}

  bool     isValid() const { return m_registry && m_id != 0; }
  EntityID id() const { return m_id; }
  void     invalidate()
  {
    m_registry = nullptr;
    m_id       = 0;
  }

  template <typename T> Entity &add(T component);
  template <typename T> Entity &remove();
  template <typename T> bool    has() const;
  template <typename T> T      &get();

private:
  Registry *m_registry = nullptr;
  EntityID  m_id       = 0;
};
