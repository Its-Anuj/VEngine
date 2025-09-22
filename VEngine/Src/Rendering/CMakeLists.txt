add_library(VEngineVulkan ModernVulkan/VulkanRenderApi.cpp ModernVulkan/VulkanContext.cpp)

include_directories("C:/VulkanSDK/1.3.275.0/Include/vulkan")
include_directories("C:/VulkanSDK/1.3.275.0/Include/")
include_directories("C:/VulkanSDK/1.3.275.0/Include/vk_video")
include_directories(".")
include_directories("..")
include_directories("Vulkan/")
include_directories("../../Libs/VMA/include/")

target_precompile_headers(VEngineVulkan PRIVATE Vulkan/VeVPCH.h)
