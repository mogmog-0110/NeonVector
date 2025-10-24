# src/CMakeLists.txt

find_package(directx-headers CONFIG REQUIRED)

# ソースファイルを収集
file(GLOB_RECURSE NEONVECTOR_HEADERS 
    "${CMAKE_SOURCE_DIR}/include/NeonVector/*.h"
)

file(GLOB_RECURSE NEONVECTOR_SOURCES
    "Core/*.cpp"
    "Graphics/*.cpp"
    # "Math/*.cpp"
    # "Effects/*.cpp"
    # "3D/*.cpp"
)

# シェーダーファイル
file(GLOB NEONVECTOR_SHADERS
    "${CMAKE_SOURCE_DIR}/shaders/*.hlsl"
)

# 静的ライブラリとして作成
add_library(NeonVector STATIC
    ${NEONVECTOR_HEADERS}
    ${NEONVECTOR_SOURCES}
)

# インクルードディレクトリ
target_include_directories(NeonVector
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# DirectX12ライブラリのリンク
target_link_libraries(NeonVector
    PUBLIC
        d3d12.lib
        dxgi.lib
        d3dcompiler.lib    # シェーダーコンパイル用
        Microsoft::DirectX-Headers
)

set(COMPILED_SHADERS "")

foreach(SHADER ${NEONVECTOR_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    
    # 頂点シェーダー
    set(VS_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}_VS.cso")
    add_custom_command(
        OUTPUT ${VS_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
        COMMAND fxc /T vs_5_1 /E VSMain /Fo "${VS_OUTPUT}" "${SHADER}"
        DEPENDS ${SHADER}
        COMMENT "Compiling vertex shader: ${SHADER_NAME}"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${VS_OUTPUT})
    
    # ピクセルシェーダー
    set(PS_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}_PS.cso")
    add_custom_command(
        OUTPUT ${PS_OUTPUT}
        COMMAND fxc /T ps_5_1 /E PSMain /Fo "${PS_OUTPUT}" "${SHADER}"
        DEPENDS ${SHADER}
        COMMENT "Compiling pixel shader: ${SHADER_NAME}"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${PS_OUTPUT})
endforeach()

# シェーダーコンパイルターゲット
add_custom_target(NeonVectorShaders ALL DEPENDS ${COMPILED_SHADERS})
add_dependencies(NeonVector NeonVectorShaders)

# 実行時にシェーダーをコピー
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    add_custom_command(TARGET NeonVector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory 
            "${CMAKE_BINARY_DIR}/bin/${CONFIG}/shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/shaders"
            "${CMAKE_BINARY_DIR}/bin/${CONFIG}/shaders"
        COMMENT "Copying shaders to output directory (${CONFIG})"
    )
endforeach()