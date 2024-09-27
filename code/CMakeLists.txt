
cmake_minimum_required(VERSION 3.23)

# Set the CMP0111 policy
cmake_policy(SET CMP0111 NEW)

# Your existing CMake configuration
project(VulkanTutorial)

# Set target properties
#set_target_properties(glslang::validator PROPERTIES <property> <value>)
find_package (glfw3 REQUIRED)
find_package (glm REQUIRED)
find_package (Vulkan REQUIRED)
find_package (tinyobjloader REQUIRED)
set(VULKAN_SDK "/home/cj/Documents/Vulkan-2024/LunarG-4cj-/1.3.290.0/x86_64")
message( STATUS  " VULKAN_SDK :${VULKAN_SDK}")

#find_package (PkgConfig)
#pkg_get_variable (STB_INCLUDEDIR stb includedir)
set(STB_INCLUDEDIR ${CMAKE_SOURCE_DIR}/../STB_INCLUDEDIR_4cj_)
if (NOT STB_INCLUDEDIR)
  unset (STB_INCLUDEDIR)
  find_path (STB_INCLUDEDIR stb_image.h PATH_SUFFIXES stb)
endif ()
if (NOT STB_INCLUDEDIR)
  message (FATAL_ERROR "stb_image.h not found")
endif ()

#set_target_properties(glslang::validator PROPERTIES IMPORTED_LOCATION_DEBUG "/home/cj/Documents/Vulkan-2024/LunarG-4cj-/1.3.290.0/source/glslang/build")
# Extracted constant for the package name
set(GLSLANG_PACKAGE_NAME "glslang")
set(glslang_DIR ${VULKAN_SDK}/lib/cmake/glslang)
message(STATUS ${glslang_DIR})
# Find the required package
find_package(${GLSLANG_PACKAGE_NAME} REQUIRED)
# Assuming you have a find_package for glslang
find_package(glslang REQUIRED)



add_executable (glslang::validator IMPORTED)
find_program (GLSLANG_VALIDATOR "glslangValidator" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
set_property (TARGET glslang::validator PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}")

## Set the imported location for the Debug configuration
#set_target_properties(glslang::validator PROPERTIES
#        IMPORTED_LOCATION_DEBUG "/home/cj/glslang/validator/debug/version"
#        IMPORTED_LOCATION_RELEASE "/home/cj/glslang/validator/release/version"
#        # You can add more configurations if needed
#)

function (add_shaders_target TARGET)
  cmake_parse_arguments ("SHADER" "" "CHAPTER_NAME" "SOURCES" ${ARGN})
  set (SHADERS_DIR ${SHADER_CHAPTER_NAME}/shaders)
  add_custom_command (
    OUTPUT ${SHADERS_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )
  set (SHADERS ${SHADERS_DIR}/frag.spv ${SHADERS_DIR}/vert.spv)
  # Some chapters may have compute shaders in addition to vertex and fragment shaders,
  # so we conditionally check this and add them to the target
  string(FIND "${SHADER_SOURCES}" "${CHAPTER_SHADER}.comp" COMPUTE_SHADER_INDEX)
  if (${COMPUTE_SHADER_INDEX} GREATER -1)  
    set (SHADERS ${SHADERS} ${SHADERS_DIR}/comp.spv)
  endif()

  message(STATUS "in add_shaders_target()")
  add_custom_command (
    OUTPUT ${SHADERS}
    COMMAND glslang::validator
    ARGS --target-env vulkan1.0 ${SHADER_SOURCES} --quiet
    WORKING_DIRECTORY ${SHADERS_DIR}
    DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
    COMMENT "Compiling Shaders"
    VERBATIM
  )
  add_custom_target (${TARGET} DEPENDS ${SHADERS})
endfunction ()

function (add_chapter CHAPTER_NAME)
  cmake_parse_arguments (CHAPTER "" "SHADER" "LIBS;TEXTURES;MODELS" ${ARGN})

  add_executable (${CHAPTER_NAME} ${CHAPTER_NAME}.cpp)
  set_target_properties (${CHAPTER_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CHAPTER_NAME})
  set_target_properties (${CHAPTER_NAME} PROPERTIES CXX_STANDARD 17)
  target_link_libraries (${CHAPTER_NAME} Vulkan::Vulkan glfw)
  target_include_directories (${CHAPTER_NAME} PRIVATE ${STB_INCLUDEDIR})

  if (DEFINED CHAPTER_SHADER)
    message(STATUS chapter_name:${CHAPTER_NAME})
    set (CHAPTER_SHADER_TARGET ${CHAPTER_NAME}_shader)
#    message(STATUS CHAPTER_SHADER_TARGET:${CHAPTER_SHADER_TARGET})
#    message(STATUS SHADER_SOURCES:${SHADER_SOURCES})
#    message(STATUS ${CHAPTER_SHADER}.frag)
#    message(STATUS ${CHAPTER_SHADER}.vert)
    file (GLOB SHADER_SOURCES ${CHAPTER_SHADER}.frag ${CHAPTER_SHADER}.vert ${CHAPTER_SHADER}.comp)
    message(STATUS "will add_shaders_target")
    add_shaders_target (${CHAPTER_SHADER_TARGET} CHAPTER_NAME ${CHAPTER_NAME} SOURCES ${SHADER_SOURCES})
#    message(FATAL_ERROR 1234)
    message(STATUS, "will add_dependencies")
    add_dependencies (${CHAPTER_NAME} ${CHAPTER_SHADER_TARGET})
  endif ()
  if (DEFINED CHAPTER_LIBS)
    message(STATUS "will target_link_libraries")
    target_link_libraries (${CHAPTER_NAME} ${CHAPTER_LIBS})
    message(STATUS "exit target_link_libraries")
  endif ()
  message(STATUS "DEFINED CHAPTER_MODELS")
  if (DEFINED CHAPTER_MODELS)
    file (COPY ${CHAPTER_MODELS} DESTINATION ${CMAKE_BINARY_DIR}/${CHAPTER_NAME}/models)
  endif ()

  if (DEFINED CHAPTER_TEXTURES)
    file (COPY ${CHAPTER_TEXTURES} DESTINATION ${CMAKE_BINARY_DIR}/${CHAPTER_NAME}/textures)
  endif ()

endfunction ()

add_chapter (00_base_code)

add_chapter (01_instance_creation)
add_chapter (01_instance_creation_zcj)

add_chapter (02_validation_layers)
add_chapter (02_validation_layers_zcj)
add_chapter (02_validation_layers-2)

add_chapter (03_physical_device_selection)
add_chapter (03_physical_device_selection_zcj)
add_chapter (03_physical_device_selection-cj-03)

add_chapter (04_logical_device)
add_chapter (04_logical_device_zcj)
#add_chapter (04_logical_device-cj-04)

add_chapter (05_window_surface)
add_chapter (05_window_surface_zcj)
#add_chapter (05_cj_window_surface-cj-05)

add_chapter (06_swap_chain_creation)
add_chapter (06_swap_chain_creation_zcj)

add_chapter (07_image_views)
add_chapter (07_image_views_zcj)

add_chapter (08_graphics_pipeline)
add_chapter (08_graphics_pipeline_zcj)

add_chapter (09_shader_modules
  SHADER 09_shader_base)

add_chapter (09_shader_modules_zcj
        SHADER 09_shader_base)

add_chapter (10_fixed_functions
  SHADER 09_shader_base)
add_chapter (10_fixed_functions_zcj
        SHADER 09_shader_base)

add_chapter (11_render_passes
  SHADER 09_shader_base)
add_chapter (11_render_passes_zcj
        SHADER 09_shader_base)

add_chapter (12_graphics_pipeline_complete
  SHADER 09_shader_base)
add_chapter (12_graphics_pipeline_complete_zcj
        SHADER 09_shader_base)

add_chapter (13_framebuffers
  SHADER 09_shader_base)
add_chapter (13_framebuffers_zcj
        SHADER 09_shader_base)

add_chapter (14_command_buffers
  SHADER 09_shader_base)
add_chapter (14_command_buffers_zcj
        SHADER 09_shader_base)

#
add_chapter (15_hello_triangle
        SHADER 09_shader_base)
add_chapter (15_hello_triangle_zcj
        SHADER 09_shader_base)

add_chapter (16_frames_in_flight
        SHADER 09_shader_base)
add_chapter (16_frames_in_flight_zcj
        SHADER 09_shader_base)

#
add_chapter (17_swap_chain_recreation
  SHADER 09_shader_base)
add_chapter (17_swap_chain_recreation_zcj
        SHADER 09_shader_base)

add_chapter (18_vertex_input
  SHADER 18_shader_vertexbuffer
  LIBS glm::glm)
add_chapter (18_vertex_input_zcj
        SHADER 18_shader_vertexbuffer
        LIBS glm::glm)

add_chapter (19_vertex_buffer
  SHADER 18_shader_vertexbuffer
  LIBS glm::glm)
add_chapter (19_vertex_buffer_zcj
        SHADER 18_shader_vertexbuffer
        LIBS glm::glm)

add_chapter (20_staging_buffer
  SHADER 18_shader_vertexbuffer
  LIBS glm::glm)
add_chapter (20_staging_buffer_zcj
        SHADER 18_shader_vertexbuffer
        LIBS glm::glm)

add_chapter (21_index_buffer
  SHADER 18_shader_vertexbuffer
  LIBS glm::glm)
add_chapter (21_index_buffer_zcj
        SHADER 18_shader_vertexbuffer
        LIBS glm::glm)

add_chapter (22_descriptor_set_layout
  SHADER 22_shader_ubo
  LIBS glm::glm)
add_chapter (22_descriptor_set_layout_zcj
        SHADER 22_shader_ubo
        LIBS glm::glm)

add_chapter (23_descriptor_sets
  SHADER 22_shader_ubo
  LIBS glm::glm)
add_chapter (23_descriptor_sets_zcj
        SHADER 22_shader_ubo
        LIBS glm::glm)

add_chapter (24_texture_image
  SHADER 22_shader_ubo
  TEXTURES ../images/texture.jpg
  LIBS glm::glm)
add_chapter (24_texture_image_zcj
        SHADER 22_shader_ubo
        TEXTURES ../images/texture.jpg
        LIBS glm::glm)

add_chapter (25_sampler
  SHADER 22_shader_ubo
  TEXTURES ../images/texture.jpg
  LIBS glm::glm)
add_chapter (25_sampler_zcj
        SHADER 22_shader_ubo
        TEXTURES ../images/texture.jpg
        LIBS glm::glm)

add_chapter (26_texture_mapping
  SHADER 26_shader_textures
  TEXTURES ../images/texture.jpg
  LIBS glm::glm)
add_chapter (26_texture_mapping_zcj
        SHADER 26_shader_textures
        TEXTURES ../images/texture.jpg
        LIBS glm::glm)

add_chapter (27_depth_buffering
  SHADER 27_shader_depth
  TEXTURES ../images/texture.jpg
  LIBS glm::glm)
add_chapter (27_depth_buffering_zcj
        SHADER 27_shader_depth
        TEXTURES ../images/texture.jpg
        LIBS glm::glm)

add_chapter (28_model_loading
  SHADER 27_shader_depth
  MODELS ../resources/viking_room.obj
  TEXTURES ../resources/viking_room.png
  LIBS glm::glm tinyobjloader::tinyobjloader)
add_chapter (28_model_loading_zcj
        SHADER 27_shader_depth
        MODELS ../resources/viking_room.obj
        TEXTURES ../resources/viking_room.png
        LIBS glm::glm tinyobjloader::tinyobjloader)

add_chapter (29_mipmapping
  SHADER 27_shader_depth
  MODELS ../resources/viking_room.obj
  TEXTURES ../resources/viking_room.png
  LIBS glm::glm tinyobjloader::tinyobjloader)
add_chapter (29_mipmapping_zcj
        SHADER 27_shader_depth
        MODELS ../resources/viking_room.obj
        TEXTURES ../resources/viking_room.png
        LIBS glm::glm tinyobjloader::tinyobjloader)

add_chapter (30_multisampling
  SHADER 27_shader_depth
  MODELS ../resources/viking_room.obj
  TEXTURES ../resources/viking_room.png
  LIBS glm::glm tinyobjloader::tinyobjloader)
add_chapter (30_multisampling_zcj
        SHADER 27_shader_depth
        MODELS ../resources/viking_room.obj
        TEXTURES ../resources/viking_room.png
        LIBS glm::glm tinyobjloader::tinyobjloader)

add_chapter (31_compute_shader
  SHADER 31_shader_compute
  LIBS glm::glm)
add_chapter (31_compute_shader_zcj
        SHADER 31_shader_compute
        LIBS glm::glm)

if(NOT DEFINED SOME_REQUIRED_VARIABLE)
  message(STATUS "FINAL ENDING!!")
endif()