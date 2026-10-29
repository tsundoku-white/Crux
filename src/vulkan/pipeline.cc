#include "pipeline.hh"
#include "src/vulkan/context.hh"
#include <fstream>
#include "src/core/vertex.hh"

// helper functions.
std::vector<char> Pipeline::readFile(const char *path)
{
  // read as a binary file
  std::ifstream file(path, std::ios::ate | std::ios::binary);

  // if can't open exit.
  if (!file.is_open())
  {
    throw std::runtime_error("failed to open file!");
  }

  // get file size.
  size_t fileSize = (size_t)file.tellg();
  // alloc a buffer byte size.
  std::vector<char> buffer(fileSize);
  // seek start of file.
  file.seekg(0);
  // go through all bytes.
  file.read(buffer.data(), fileSize);
  // close file.
  file.close();

  // return read buffer.
  return buffer;
}

VkShaderModule Pipeline::createShaderModule(const std::vector<char> &code)
{
  VkShaderModuleCreateInfo createInfo {};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("failed to create shader module!");
  }
  return shaderModule;
}

void Pipeline::init(Context &context, Descriptor &descriptor)
{
  // get device and store as a local var.
  m_device = context.getDevice();

  // store the code in a var.
  auto shaderCode = readFile(SHADER_DIR "shader.spv");

  // create a shader module.
  VkShaderModule shaderModule = createShaderModule(shaderCode);

  // address the main functions of the vertex shader.
  VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
  vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = shaderModule;
  vertShaderStageInfo.pName  = "vertexMain";

  // address the main functions of the fragment shader.
  VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
  fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = shaderModule;
  fragShaderStageInfo.pName  = "fragmentMain";

  // stage the shaders
  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  // set rendering draw direction (top to bottom row by row).
  std::vector<VkDynamicState> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT,
     VK_DYNAMIC_STATE_SCISSOR};

  // descriptor setup.
  auto bindingDescription    = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
  vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount   = 1;
  vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

  // declaring that all mesh will be drawn as triangles.
  VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
  inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // setting up center of screen and canvas size.
  VkViewport viewport {};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = (float)context.getExtent2D().width;
  viewport.height   = (float)context.getExtent2D().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  // setting screen offset
  VkRect2D scissor {};
  scissor.offset = {0, 0};
  scissor.extent = context.getExtent2D();

  // setting drawing state.
  VkPipelineDynamicStateCreateInfo dynamicState {};
  dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates    = dynamicStates.data();

  // view points to be drawn.
  VkPipelineViewportStateCreateInfo viewportState {};
  viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount  = 1;

  // rasterizer setup.
  VkPipelineRasterizationStateCreateInfo rasterizer {};
  rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp          = 0.0f;
  rasterizer.depthBiasSlopeFactor    = 0.0f;

  // multisampling setup.
  VkPipelineMultisampleStateCreateInfo multisampling {};
  multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable   = VK_FALSE;
  multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading      = 1.0f;
  multisampling.pSampleMask           = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable      = VK_FALSE;

  // coloring fo the image.
  VkPipelineColorBlendAttachmentState colorBlendAttachment {};
  colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable         = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
  colorBlendAttachment.blendEnable         = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

  // bleding of the colors (rgba).
  VkPipelineColorBlendStateCreateInfo colorBlending {};
  colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable     = VK_FALSE;
  colorBlending.logicOp           = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount   = 1;
  colorBlending.pAttachments      = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkDescriptorSetLayout descriptorLayout = descriptor.getLayout();

  // shader push consts.
  VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
  pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount         = 1;
  pipelineLayoutInfo.pSetLayouts            = &descriptorLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges    = nullptr;

  if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_layout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout");
  }

  // setting color format.
  VkFormat colorFormat = context.getImageFormat();

  // render attached images.
  VkPipelineRenderingCreateInfo renderingInfo {};
  renderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingInfo.colorAttachmentCount    = 1;
  renderingInfo.pColorAttachmentFormats = &colorFormat;

  // combining all parts to one frame.
  VkGraphicsPipelineCreateInfo info {};
  info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  info.pNext               = &renderingInfo;
  info.stageCount          = 2;
  info.pStages             = shaderStages;
  info.pVertexInputState   = &vertexInputInfo;
  info.pInputAssemblyState = &inputAssembly;
  info.pViewportState      = &viewportState;
  info.pRasterizationState = &rasterizer;
  info.pMultisampleState   = &multisampling;
  info.pColorBlendState    = &colorBlending;
  info.pDynamicState       = &dynamicState;
  info.layout              = m_layout;
  info.renderPass          = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &info, nullptr,
                                &m_pipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipeline");

  vkDestroyShaderModule(m_device, shaderModule, nullptr);
  std::print("pass: pipeline\n");
}

void Pipeline::destroy()
{
  // check if valid then destroy if valid.
  if (m_pipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
  if (m_layout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(m_device, m_layout, nullptr);
}
