cmake_minimum_required(VERSION 3.10)
project(Kyl)

# ------------------------------------------------
# 1. 基础设置
# ------------------------------------------------
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------
# 2. 交叉编译工具链设置 (LS2K0300 / loongarch64)
# ------------------------------------------------
message(STATUS "交叉编译模式: loongarch64")
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR loongarch64)

set(TOOLCHAIN_PATH "/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6")
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PATH}/bin/loongarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/bin/loongarch64-linux-gnu-g++)
set(CMAKE_AR           ${TOOLCHAIN_PATH}/bin/loongarch64-linux-gnu-ar)
set(CMAKE_RANLIB       ${TOOLCHAIN_PATH}/bin/loongarch64-linux-gnu-ranlib)
set(CMAKE_SYSROOT      ${TOOLCHAIN_PATH}/loongarch64-linux-gnu/sysroot)

set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ------------------------------------------------
# 3. OpenCV
# ------------------------------------------------
set(OPENCV_ROOT "/opt/ls_2k0300_env/opencv_4_10_build")

if(EXISTS "${OPENCV_ROOT}/lib/cmake/opencv4/OpenCVConfig.cmake")
    set(OpenCV_DIR "${OPENCV_ROOT}/lib/cmake/opencv4")
elseif(EXISTS "${OPENCV_ROOT}/lib64/cmake/opencv4/OpenCVConfig.cmake")
    set(OpenCV_DIR "${OPENCV_ROOT}/lib64/cmake/opencv4")
elseif(EXISTS "${OPENCV_ROOT}/share/OpenCV/OpenCVConfig.cmake")
    set(OpenCV_DIR "${OPENCV_ROOT}/share/OpenCV")
endif()

find_package(OpenCV REQUIRED)
message(STATUS ">>>> OpenCV_DIR = ${OpenCV_DIR}")
message(STATUS ">>>> 找到 OpenCV: ${OpenCV_LIBS}")

# ------------------------------------------------
# 4. 收集源文件
# ------------------------------------------------
file(GLOB ROOT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB SRC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/libraries/zf_common/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/libraries/zf_device/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/libraries/zf_driver/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/libraries/zf_components/*.cpp"
)

list(FILTER LIB_SOURCES EXCLUDE REGEX ".*_test\\.cpp$")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*/test/.*")

set(ALL_SOURCES
    ${ROOT_SOURCES}
    ${SRC_SOURCES}
    ${LIB_SOURCES}
)

# ------------------------------------------------
# 5. 创建目标
# ------------------------------------------------
add_executable(main ${ALL_SOURCES})

# 让编译器自己处理 OpenMP / libgomp
target_compile_options(main PRIVATE -fopenmp)
target_link_options(main PRIVATE -fopenmp)

# ------------------------------------------------
# 6. 头文件包含
# ------------------------------------------------
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/zf_common
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/zf_device
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/zf_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/zf_components
    ${OpenCV_INCLUDE_DIRS}
    ${OPENCV_ROOT}/include
    ${OPENCV_ROOT}/include/opencv4
    ${CMAKE_SYSROOT}/usr/include
)

# ------------------------------------------------
# 7. 链接库
# ------------------------------------------------
target_link_libraries(main PRIVATE
    ${OpenCV_LIBS}
    pthread
    m
    rt
    dl
)

target_compile_definitions(main PRIVATE
    _GLIBCXX_USE_CXX11_ABI=1
)
# Added for official Seekfree DRV8701 motor bridge
target_sources(main PRIVATE src/official_motor_bridge.cpp)
