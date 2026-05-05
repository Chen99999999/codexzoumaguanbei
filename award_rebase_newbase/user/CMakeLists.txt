cmake_minimum_required(VERSION 3.5.0)

include(cross.cmake)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(PARENT_DIR ${CURRENT_DIR}/.. ABSOLUTE)
get_filename_component(PROJ_NAME ${PARENT_DIR} NAME)
project(${PROJ_NAME} VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(COMMON_FLAGS "-O3 -march=loongarch64 -mtune=loongarch64 -fPIC -ffunction-sections -fdata-sections -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -pthread -Wno-delete-non-virtual-dtor -Wno-placement-new")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

set(CMAKE_PREFIX_PATH "/opt/ls_2k0300_env/opencv_4_10_build")
find_package(OpenCV REQUIRED)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ../user
    ../award_src/include
    ../libraries/zf_common
    ../libraries/zf_device
    ../libraries/zf_driver
    ../libraries/zf_components/seekfree_assistant
)

set(AWARD_SOURCES
    ../award_src/src/Fuzzy.cpp
    ../award_src/src/image.cpp
    ../award_src/src/Init.cpp
    ../award_src/src/motor.cpp
    ../award_src/src/Servo.cpp
)

aux_source_directory(../libraries/zf_common LIB_SRCS)
aux_source_directory(../libraries/zf_device LIB_SRCS)
aux_source_directory(../libraries/zf_driver LIB_SRCS)
aux_source_directory(../libraries/zf_components/seekfree_assistant LIB_SRCS)

add_executable(main
    ../user/main.cpp
    ../user/legacy_compat.cpp
    ${AWARD_SOURCES}
    ${LIB_SRCS}
)

set_target_properties(main PROPERTIES
    INSTALL_RPATH "/opt/ls_2k0300_env/opencv_4_10_build/lib"
    BUILD_WITH_INSTALL_RPATH ON
)

target_link_libraries(main
    ${OpenCV_LIBS}
    pthread
    m
    rt
    dl
)
