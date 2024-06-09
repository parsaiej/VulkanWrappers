# Misc. Packages.
# -----------------------

find_package(glfw3  REQUIRED)
find_package(Vulkan REQUIRED)
find_package(volk   REQUIRED)

# Add VMA
# -----------------------

add_subdirectory(VulkanMemoryAllocator)

# Library
# -----------------------

add_library(VulkanWrappers SHARED
    "VmaUsage.cpp" 
    "Device.cpp" 
    "Window.cpp"
    "Image.cpp"
    "Buffer.cpp"
    "Shader.cpp"
)
    
# Include
# -----------------------

target_include_directories(VulkanWrappers PRIVATE ${GLFW_INCLUDE_DIRS})
target_include_directories(VulkanWrappers PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(VulkanWrappers PRIVATE "Include/")

# Link
# -----------------------

target_link_libraries(VulkanWrappers PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(VulkanWrappers PRIVATE glfw)
target_link_libraries(VulkanWrappers PRIVATE GPUOpen::VulkanMemoryAllocator)
target_link_libraries(VulkanWrappers PRIVATE volk::volk)