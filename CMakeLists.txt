# Packages.
# -----------------------

find_package(glfw3 REQUIRED)

# OS-specific needs.
# -----------------------

if (APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(QUARTZ_CORE QuartzCore REQUIRED)
    add_library(MetalUtility SHARED "MetalUtility/MetalUtility.m")
    target_link_libraries(MetalUtility ${QUARTZ_CORE} ${COCOA_LIBRARY} glfw)
endif ()

# Library
# -----------------------

add_library(Wrappers SHARED "RenderInstance.cpp" "Pipeline.cpp")

# Include
# -----------------------

target_include_directories(Wrappers PRIVATE ${Vulkan_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS})

# Link
# -----------------------

target_link_libraries(Wrappers PRIVATE ${Vulkan_LIBRARIES} glfw MetalUtility)