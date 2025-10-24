cmake_minimum_required(VERSION 3.20)
project(NeonVector VERSION 0.1.0 LANGUAGES CXX)

# C++20を必須に設定
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# デフォルトのビルドタイプ
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# 出力ディレクトリの設定
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# プロジェクトオプション
option(NEONVECTOR_BUILD_EXAMPLES "Build example projects" ON)
option(NEONVECTOR_BUILD_TESTS "Build tests" OFF)

# MSVC固有の設定
if(MSVC)
    # 警告レベルと最適化
    add_compile_options(
        /W4           # 警告レベル4
        /MP           # マルチプロセスビルド
        /permissive-  # 標準準拠モード
    )
    
    # マクロ定義
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )
    
    # デバッグビルドの設定
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /Zi)
    endif()
endif()

# サブディレクトリ
add_subdirectory(src)

if(NEONVECTOR_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(NEONVECTOR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ステータス出力
message(STATUS "=================================")
message(STATUS "NeonVector Engine Configuration")
message(STATUS "=================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build examples: ${NEONVECTOR_BUILD_EXAMPLES}")
message(STATUS "Build tests: ${NEONVECTOR_BUILD_TESTS}")
message(STATUS "=================================")