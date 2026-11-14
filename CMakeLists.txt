# ============================================================================
#  《合金弹头》Metal Slug —— CMake 构建
#
#  用法（Windows + Visual Studio）：
#    1) 安装 EasyX，并把路径填进下面的 EASYX_ROOT（默认 C:/EasyX）
#    2) mkdir build && cd build
#    3) cmake .. -G "Visual Studio 17 2022" -A x64
#    4) cmake --build . --config Release
#
#  可执行文件会直接输出到源码根目录（和素材放在一起），
#  因为游戏用相对路径加载素材，工作目录必须是素材所在目录。
# ============================================================================
cmake_minimum_required(VERSION 3.15)
project(MetalSlug LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- EasyX 路径 ----
set(EASYX_ROOT "C:/EasyX" CACHE PATH "EasyX 安装目录（包含 include 与 lib）")
set(EASYX_LIB  "EasyXw" CACHE STRING "EasyX 库：Unicode 用 EasyXw，ANSI 用 EasyXa")

if(NOT EXISTS "${EASYX_ROOT}/include/graphics.h")
    message(WARNING "没找到 ${EASYX_ROOT}/include/graphics.h，请设置 EASYX_ROOT")
endif()

add_executable(HeJinDanTou
    HeJinDanTou.cpp
)

target_include_directories(HeJinDanTou PRIVATE
    "${EASYX_ROOT}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_directories(HeJinDanTou PRIVATE "${EASYX_ROOT}/lib/x64")
target_link_libraries(HeJinDanTou PRIVATE "${EASYX_LIB}" Winmm)

# 游戏按相对路径加载素材，可执行文件必须和素材放在一起
set_target_properties(HeJinDanTou PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Windows 专用：多字节字符集警告关掉，用不到
if(MSVC)
    target_compile_options(HeJinDanTou PRIVATE /W3 /wd4996 /utf-8)
endif()
