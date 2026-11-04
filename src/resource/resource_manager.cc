#include "resource_manager.hh"
#include "src/ecs/components/transform.hh"
#include "src/vulkan/UBO.hh"
#include "src/vulkan/command.hh"
#include "src/vulkan/context.hh"
#include "src/vulkan/descriptor.hh"
#include <cstddef>
#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
#include <cgltf.h>

void ResourceManager::updateCamera()
{
  auto &cameras    = m_registry->pool<Camera>();
  auto &transforms = m_registry->pool<Transform>();

  for (EntityID id : cameras.entities())
  {
    const Camera    *camera    = cameras.get(id);
    const Transform *transform = transforms.get(id);

    if (!camera || !transform)
      continue;

    if (m_prespectiveUpdate)
    {
      m_aspectRatio          = static_cast<float>(m_context->getExtent2D().width) /
                               static_cast<float>(m_context->getExtent2D().height);
      m_ubo.projectionMatrix = glm::perspective(glm::radians(camera->fov), m_aspectRatio, camera->near, camera->far);
      m_ubo.projectionMatrix[1][1] *= -1.f;
    }
    m_ubo.viewMatrix = glm::lookAt(transform->location, transform->location + transform->forward, transform->up);
    m_descriptor->uploadToGPU(m_currentFrame, &m_ubo, sizeof(UBO), 0);
    break;
  }
}

void ResourceManager::updateTransform()
{
  auto &transforms = m_registry->pool<Transform>();
  for (EntityID id : transforms.entities())
  {
    Transform *transform = transforms.get(id);
    {
      transform->rotation = glm::normalize(transform->rotation);
      transform->forward  = transform->rotation * glm::vec3(0, 0, -1);
      transform->right    = transform->rotation * glm::vec3(1, 0, 0);
      transform->up       = transform->rotation * glm::vec3(0, 1, 0);
    }
  }
}
void ResourceManager::createModels()
{
  auto &meshs = m_registry->pool<Mesh>();
  for (EntityID id : meshs.entities())
  {
    Mesh *mesh = meshs.get(id);
    if (!mesh || mesh->indexBuffer.valid())
      continue; // already uploaded

    if (!loadMeshData(mesh->vertexData, mesh->indexData, MODEL_DIR "cube.glb"))
    {
      std::print("Failed to load Mesh\n");
    }
    mesh->indexCount = mesh->indexData.size();

    mesh->vertexBuffer.createStatic(m_context, m_command, BufferKind::Vertex,
                                    mesh->vertexData.data(), sizeof(Vertex) * mesh->vertexData.size());
    mesh->indexBuffer.createStatic(m_context, m_command, BufferKind::Index,
                                   mesh->indexData.data(), sizeof(uint32_t) * mesh->indexData.size());
  }
}

void ResourceManager::buildDrawCalls()
{
  m_drawCalls.clear();
  auto &meshes     = m_registry->pool<Mesh>();
  auto &transforms = m_registry->pool<Transform>();

  for (EntityID id : meshes.entities())
  {
    const Mesh      *mesh      = meshes.get(id);
    const Transform *transform = transforms.get(id);
    if (!mesh || !transform || !mesh->indexBuffer.valid())
      continue;

    DrawCall d;
    d.vertexBuffer = mesh->vertexBuffer.handle();
    d.indexBuffer  = mesh->indexBuffer.handle();
    d.indexCount   = mesh->indexCount;
    d.model        = glm::translate(glm::mat4(1.f), transform->location) * glm::mat4_cast(transform->rotation) * glm::scale(glm::mat4(1.f), transform->scale);

    m_ubo.model = d.model; // single mesh only
    m_drawCalls.push_back(d);
  }
}

void ResourceManager::init(Registry &registry, Context &context, Descriptor &descriptor, Command &command)
{
  m_registry   = &registry;
  m_context    = &context;
  m_descriptor = &descriptor;
  m_command    = &command;
  m_entities   = registry.getEntityView();

  createModels();
}

void ResourceManager::update(uint32_t currentFrame)
{
  m_currentFrame = currentFrame;
  createModels();
  updateTransform();
  buildDrawCalls();
  updateCamera();

  if (m_prespectiveUpdate)
    m_prespectiveUpdate = false;
}

void ResourceManager::destroy()
{
  if (m_registry)
  {
    auto &meshes = m_registry->pool<Mesh>();
    for (EntityID id : meshes.entities())
    {
      if (Mesh *mesh = meshes.get(id))
      {
        mesh->vertexBuffer.destroy();
        mesh->indexBuffer.destroy();
      }
    }
  }

  m_entities.clear();
  m_drawCalls.clear();
}

bool ResourceManager::loadMeshData(std::vector<Vertex>   &vertices,
                                   std::vector<uint32_t> &indices,
                                   const std::string     &path)
{
  cgltf_options options = {};
  cgltf_data   *data    = nullptr;

  if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
    return false;

  if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success ||
      cgltf_validate(data) != cgltf_result_success)
  {
    cgltf_free(data);
    return false;
  }

  for (size_t m = 0; m < data->meshes_count; ++m)
  {
    const cgltf_mesh &mesh = data->meshes[m];

    for (size_t p = 0; p < mesh.primitives_count; ++p)
    {
      const cgltf_primitive &prim = mesh.primitives[p];
      if (prim.type != cgltf_primitive_type_triangles)
        continue;

      // Position accessor decides the vertex count
      const cgltf_accessor *posAcc = nullptr;
      for (size_t a = 0; a < prim.attributes_count; ++a)
        if (prim.attributes[a].type == cgltf_attribute_type_position)
          posAcc = prim.attributes[a].data;
      if (!posAcc)
        continue;

      const uint32_t baseVertex = static_cast<uint32_t>(vertices.size());
      vertices.resize(baseVertex + posAcc->count);

      glm::vec3 baseColor {1.f};
      if (prim.material && prim.material->has_pbr_metallic_roughness)
      {
        const float *c = prim.material->pbr_metallic_roughness.base_color_factor;
        baseColor      = {c[0], c[1], c[2]};
      }

      for (size_t i = 0; i < posAcc->count; ++i)
        vertices[baseVertex + i].color = baseColor;

      for (size_t a = 0; a < prim.attributes_count; ++a)
      {
        const cgltf_attribute &attr = prim.attributes[a];
        const cgltf_accessor  *acc  = attr.data;

        for (size_t i = 0; i < acc->count; ++i)
        {
          Vertex &v    = vertices[baseVertex + i];
          float   f[4] = {0, 0, 0, 1};
          if (!cgltf_accessor_read_float(acc, i, f, 4))
            continue;

          switch (attr.type)
          {
          case cgltf_attribute_type_position:
            v.pos = {f[0], f[1], f[2]};
            break;
          case cgltf_attribute_type_normal:
            v.normal = {f[0], f[1], f[2]};
            break;
          case cgltf_attribute_type_texcoord:
            if (attr.index == 0)
              v.uv = {f[0], f[1]};
            break;
          case cgltf_attribute_type_color:
            if (attr.index == 0)
              v.color = baseColor * glm::vec3(f[0], f[1], f[2]);
            break;
          default:
            break;
          }
        }
      }

      // Indices (offset by baseVertex so multiple primitives work)
      if (prim.indices)
      {
        for (size_t i = 0; i < prim.indices->count; ++i)
          indices.push_back(baseVertex + static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, i)));
      }
      else // non-indexed primitive
      {
        for (size_t i = 0; i < posAcc->count; ++i)
          indices.push_back(baseVertex + static_cast<uint32_t>(i));
      }
    }
  }

  cgltf_free(data);
  return true;
}
