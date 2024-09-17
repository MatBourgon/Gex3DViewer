cmake_minimum_required(VERSION 3.12) # 3.12 required for CONFIGURE_DEPENDS
project("Gex 2 Level Viewer")

# Main Project Source Files
file(GLOB g2viewer_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Imgui Source files
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui)
set(IMGUI_SRC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/imgui_impl_opengl3.cpp
  ${IMGUI_DIR}/imgui_impl_win32.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
)

# Vendor source files
set(g2viewer_VENDOR_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad.c
  ${IMGUI_SRC}
)

# Directories
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Compile targets
add_library(g2statics ${g2viewer_VENDOR_SRC})
add_executable (g2viewer ${g2viewer_SRC})

# Convenience
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT g2viewer)

# C++20 required for std::endian.
# Can probably be lowered if removed, if it's a problem or a solution is made.
set_property(TARGET g2viewer PROPERTY CXX_STANDARD 20)
set_property(TARGET g2statics PROPERTY CXX_STANDARD 20)

# VCPKG is a bane for local packages
set_target_properties(g2viewer PROPERTIES VS_USER_PROPS do_not_import_user.props) 
set_target_properties(g2viewer PROPERTIES VS_GLOBAL_VcpkgEnabled false)
set_target_properties(g2statics PROPERTIES VS_USER_PROPS do_not_import_user.props) 
set_target_properties(g2statics PROPERTIES VS_GLOBAL_VcpkgEnabled false)

# Include directories
target_include_directories(g2viewer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(g2statics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Static links
target_link_libraries(g2viewer
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw/glfw3.lib"
  PRIVATE "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/$<CONFIG>/g2statics.lib"
)