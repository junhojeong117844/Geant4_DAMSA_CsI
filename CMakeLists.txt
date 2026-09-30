cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(DAMSA_CsI)

find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB runtime_files
    ${PROJECT_SOURCE_DIR}/macros/*.mac
    ${PROJECT_SOURCE_DIR}/macros/*.csv
)

add_executable(DAMSA_CsI sim.cc ${sources})
target_link_libraries(DAMSA_CsI ${Geant4_LIBRARIES})

foreach(runtime_file ${runtime_files})
    get_filename_component(runtime_name ${runtime_file} NAME)
    configure_file(
        ${runtime_file}
        ${PROJECT_BINARY_DIR}/${runtime_name}
        COPYONLY
    )
endforeach()
