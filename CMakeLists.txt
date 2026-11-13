# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.28)

option(DUCK_BUILD_TESTS "Build the unit tests" ON)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:EditAndContinue>$<$<CONFIG:RelWithDebInfo>:ProgramDatabase>")
endif()

project(DuckEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(CheckIPOSupported)
check_ipo_supported(RESULT duckIpoSupported OUTPUT duckIpoError)
if(duckIpoSupported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
else()
  message(STATUS "Link-time optimization not available: ${duckIpoError}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(cmake/Helpers.cmake)

find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

find_package(VulkanHeaders CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

add_subdirectory(libs/Core)
add_subdirectory(libs/Platform)
add_subdirectory(libs/VkCore)
add_subdirectory(libs/RayTraceCpu)

add_subdirectory(apps/Sandbox)
add_subdirectory(apps/Raytracer)

if(DUCK_BUILD_TESTS)
	enable_testing()
	find_package(Catch2 3 CONFIG REQUIRED)
	add_subdirectory(tests)
endif()