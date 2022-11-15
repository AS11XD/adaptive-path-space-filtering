add_subdirectory(glm)

set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory(glfw)

find_package(Vulkan REQUIRED)
add_library(imgui
	imgui/imgui.cpp
	imgui/imgui.h
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_internal.h
	imgui/imgui_orient.cpp
	imgui/imgui_orient.h
	imgui/implot-master/implot.h
	imgui/implot-master/implot.cpp
	imgui/implot-master/implot_demo.cpp
	imgui/implot-master/implot_internal.h
	imgui/implot-master/implot_items.cpp
	imgui/imGuiFileDialog/ImGuiFileDialog.h
	imgui/imGuiFileDialog/ImGuiFileDialog.cpp
	imgui/iconFontCppHeaders/IconsFontAwesome6.h
	imgui/imgui_widgets.cpp
	imgui/imstb_rectpack.h
	imgui/imstb_textedit.h
	imgui/imstb_truetype.h
	imgui/backends/imgui_impl_glfw.cpp
	imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PRIVATE imgui)
target_include_directories(imgui PRIVATE imgui/backends)
target_include_directories(imgui PRIVATE glfw/include)
target_include_directories(imgui PRIVATE ${Vulkan_INCLUDE_DIRS})

add_library(dds
	dds_loader/dds_loader.h
	dds_loader/dds_loader.cpp
)

add_library(optix INTERFACE)
target_include_directories(optix INTERFACE optix/include/)

add_subdirectory(assimp)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb/)

#add_library(vma INTERFACE)
#target_include_directories(vma INTERFACE VulkanMemoryAllocator/include)