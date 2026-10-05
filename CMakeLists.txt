cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(DAMSA_CsI)

find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
add_executable(DAMSA_CsI sim.cc ${sources})
target_link_libraries(DAMSA_CsI ${Geant4_LIBRARIES})


file(GLOB macro_files "${PROJECT_SOURCE_DIR}/macros/*.mac")
add_custom_target(copy_macros ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${macro_files} ${PROJECT_BINARY_DIR}
  COMMENT "Copying macro files to the build directory"
)
