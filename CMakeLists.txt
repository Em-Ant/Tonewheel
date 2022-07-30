cmake_minimum_required(VERSION 3.16.0)

project(tonewheel VERSION 1.0.0)

include_directories(src lib)

set(SOURCES 
    src/adsr.cpp
    src/perc_envelope.cpp
    src/sc_reverb.c
    src/tonewheel.cpp
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -O3)

link_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(tonewheel ${SOURCES})

target_link_libraries(tonewheel rtaudio rtmidi pthread asound)

add_custom_target(run
    COMMAND tonewheel
    DEPENDS tonewheel
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)