#pragma once

#include "component.hh"
#include "entity.hh"

#include "components/camera.hh"
#include "components/transform.hh"
#include "components/mesh.hh"
#include <cstdint>
#include <memory>
#include <typeindex>
#include <vector>
#include "src/core/pch.hh"

class Registry
{
public:
  Entity create()
  {
    EntityID id = nextID();
    std::println("Entity id: {}", id);
    return Entity(*this, id);
  }
  template <typename T>
  Component<T> &pool()
  {
    auto key = std::type_index(typeid(T));
    auto it  = m_pools.find(key);
    if (it == m_pools.end())
      it = m_pools.emplace(key, std::make_unique<Component<T>>()).first;
    return static_cast<Component<T> &>(*it->second);
  }

  template <typename T>
  std::vector<T> &getComponentView() { return pool<T>().getComponents();}
  std::vector<uint32_t> getEntityView() { return m_denseEntity; }

private:
  uint32_t m_nextID = 1;
  uint32_t nextID() { return m_nextID++; }

  std::vector<size_t>   m_sparseEntity = {};
  std::vector<uint32_t> m_denseEntity  = {};

  std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
};

template <typename T>
Entity &Entity::add(T component)
{
  m_registry->pool<T>().add(m_id, std::move(component));
  return *this;
}
template <typename T>
Entity &Entity::remove()
{
  m_registry->pool<T>().remove(m_id);
  return *this;
}
template <typename T>bool Entity::has() const
{
  return m_registry->pool<T>().has(m_id);
}

template <typename T>
T &Entity::get()
{
  T *component = m_registry->pool<T>().get(m_id);
  assert(component && "entity does not have this component");
  return *component;
}
