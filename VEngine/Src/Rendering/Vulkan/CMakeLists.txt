add_library(VEngineVulkan VulkanRenderer.cpp)

include_directories("C:/VulkanSDK/1.3.275.0/Include/vulkan")
include_directories("C:/VulkanSDK/1.3.275.0/Include/")
include_directories("C:/VulkanSDK/1.3.275.0/Include/vk_video")
target_precompile_headers(VEngineVulkan PRIVATE VeVPCH.h)
