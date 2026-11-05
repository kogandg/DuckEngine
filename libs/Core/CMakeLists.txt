duck_library(Core DEPS glm::glm)

target_compile_definitions(Duck_Core PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)