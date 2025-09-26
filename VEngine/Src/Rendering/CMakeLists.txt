add_library(VEngineVulkan ModernVulkan/VulkanRenderApi.cpp ModernVulkan/VulkanResourceFactory.cpp ModernVulkan/VulkanContext.cpp ModernVulkan/VulkanDevice.cpp)

include_directories("C:/VulkanSDK/1.3.275.0/Include/vulkan")
include_directories("C:/VulkanSDK/1.3.275.0/Include/")
include_directories("C:/VulkanSDK/1.3.275.0/Include/vk_video")
include_directories(".")
include_directories("..")
include_directories("Vulkan/")
include_directories("../../Libs/VMA/include/")
include_directories("../../Libs/stb/")

target_precompile_headers(VEngineVulkan PRIVATE ModernVulkan/VeVPCH.h)
target_link_libraries(VEngineVulkan PUBLIC VEngine)
