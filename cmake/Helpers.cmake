if(MSVC)
  add_compile_options(/W4 /permissive- /MP /Zc:__cplusplus /utf-8)
endif()

# duck_library(core DEPS other_libs...)  ->  target duck_core, alias duck::core
function(duck_library name)
  cmake_parse_arguments(ARG "" "" "DEPS" ${ARGN})
  set(target Duck_${name})

  file(GLOB_RECURSE srcs CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
       ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

  add_library(${target} STATIC ${srcs})
  add_library(Duck::${name} ALIAS ${target})

  target_compile_definitions(${target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

  target_include_directories(${target}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(${target} PUBLIC ${ARG_DEPS})

  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${srcs})
  set_target_properties(${target} PROPERTIES FOLDER libs)
endfunction()

function(duck_app name)
  cmake_parse_arguments(ARG "" "" "DEPS" ${ARGN})

  file(GLOB_RECURSE srcs CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

  add_executable(${name} ${srcs})
  target_link_libraries(${name} PRIVATE ${ARG_DEPS})
  # Lets the app find assets/ regardless of the debugger's working directory
  target_compile_definitions(${name} PRIVATE DUCK_ROOT_DIR="${CMAKE_SOURCE_DIR}")

  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${srcs})
  set_target_properties(${name} PROPERTIES FOLDER apps)
endfunction()