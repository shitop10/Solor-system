cmake_minimum_required(VERSION 3.20)
project(SolarSystem VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: treat all source files as UTF-8 (Chinese console output)
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ——— GLAD (OpenGL loader, C source) ———
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

# ——— Executable (Windows GUI, no console) ———
add_executable(solar_system main.cpp)

target_include_directories(solar_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/rendering
    ${CMAKE_SOURCE_DIR}/src/scene
    ${CMAKE_SOURCE_DIR}/src/utils
)

if(WIN32)
    target_link_libraries(solar_system PRIVATE glad opengl32 gdi32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(solar_system PRIVATE glad OpenGL::GL)
endif()

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Post-build: copy shaders to output directory
add_custom_command(TARGET solar_system POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:solar_system>/shaders"
)

# Set VS debugger working directory
set_property(TARGET solar_system PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:solar_system>")
