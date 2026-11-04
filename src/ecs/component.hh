#pragma once
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>
#include "src/ecs/entity.hh"

constexpr uint32_t INVALID_COMP_ID = std::numeric_limits<uint32_t>::max();

struct IComponentPool { virtual ~IComponentPool() = default; };

template <typename T>
class Component : public IComponentPool
{
public:
  void add(EntityID id, T value)
  {
    if (has(id))
    {
      m_components[m_sparse[id]] = std::move(value);
      return;
    }
    if (id >= m_sparse.size())
      m_sparse.resize(id + 1, INVALID_COMP_ID);

    m_sparse[id] = static_cast<uint32_t>(m_dense.size());
    m_dense.push_back(id);
    m_components.push_back(std::move(value));
  }

  void remove(EntityID id)
  {
    if (!has(id))
      return;

    const uint32_t idx   = m_sparse[id];
    const uint32_t last  = static_cast<uint32_t>(m_dense.size() - 1);
    const EntityID moved = m_dense[last];

    if (idx != last)
    {
      m_dense[idx]      = moved;
      m_components[idx] = std::move(m_components[last]);
      m_sparse[moved]   = idx;
    }
    m_dense.pop_back();
    m_components.pop_back();
    m_sparse[id] = INVALID_COMP_ID;
  }

  bool has(EntityID id) const
  {
    return id < m_sparse.size() && m_sparse[id] != INVALID_COMP_ID;
  }

  T       *get(EntityID id) { return has(id) ? &m_components[m_sparse[id]] : nullptr; }
  const T *get(EntityID id) const { return has(id) ? &m_components[m_sparse[id]] : nullptr; }

  size_t                    size() const { return m_dense.size(); }
  std::span<const EntityID> entities() const { return m_dense; }
  std::span<T>              data() { return m_components; }

  std::vector<T>  &getComponents() {return m_components; }

private:
  std::vector<uint32_t> m_sparse;     // entity id -> dense index
  std::vector<EntityID> m_dense;      // dense index -> entity id
  std::vector<T>        m_components; // parallel to m_dense
};
