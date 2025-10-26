# src/CMakeLists.txt

find_package(directx-headers CONFIG REQUIRED)

# ソースファイルを収集
file(GLOB_RECURSE NEONVECTOR_HEADERS 
    "${CMAKE_SOURCE_DIR}/include/NeonVector/*.h"
)

file(GLOB_RECURSE NEONVECTOR_SOURCES
    "Core/*.cpp"
    "Graphics/*.cpp"
    "Effects/*.cpp"
)

# 通常のシェーダーファイル（標準のVSMain/PSMainを使う）
set(STANDARD_SHADERS
    "${CMAKE_SOURCE_DIR}/shaders/Line.hlsl"
)

# Bloomシェーダー（複数エントリーポイント）
set(BLOOM_SHADER "${CMAKE_SOURCE_DIR}/shaders/Bloom.hlsl")

# ガウシアンブラーシェーダー
set(BLUR_SHADER "${CMAKE_SOURCE_DIR}/shaders/GaussianBlur.hlsl")
set(BLUR_HQ_SHADER "${CMAKE_SOURCE_DIR}/shaders/GaussianBlur_HighQuality.hlsl")

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
        d3dcompiler.lib
        Microsoft::DirectX-Headers
)

set(COMPILED_SHADERS "")

# ===========================================
# 標準シェーダーのコンパイル（Line.hlsl等）
# ===========================================
foreach(SHADER ${STANDARD_SHADERS})
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

# ===========================================
# ガウシアンブラーシェーダー
# ===========================================
if(EXISTS ${BLUR_SHADER})
    # 頂点シェーダー（共通）
    set(VS_OUTPUT "${CMAKE_BINARY_DIR}/shaders/GaussianBlur_VS.cso")
    add_custom_command(
        OUTPUT ${VS_OUTPUT}
        COMMAND fxc /T vs_5_1 /E VSMain /Fo "${VS_OUTPUT}" "${BLUR_SHADER}"
        DEPENDS ${BLUR_SHADER} "${CMAKE_SOURCE_DIR}/shaders/GaussianBlur_Common.hlsli"
        COMMENT "Compiling GaussianBlur vertex shader"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${VS_OUTPUT})
    
    # ピクセルシェーダー
    set(PS_OUTPUT "${CMAKE_BINARY_DIR}/shaders/GaussianBlur_PS.cso")
    add_custom_command(
        OUTPUT ${PS_OUTPUT}
        COMMAND fxc /T ps_5_1 /E PSMain /Fo "${PS_OUTPUT}" "${BLUR_SHADER}"
        DEPENDS ${BLUR_SHADER} "${CMAKE_SOURCE_DIR}/shaders/GaussianBlur_Common.hlsli"
        COMMENT "Compiling GaussianBlur pixel shader"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${PS_OUTPUT})
endif()

# 高品質版
if(EXISTS ${BLUR_HQ_SHADER})
    # 頂点シェーダー
    set(VS_OUTPUT "${CMAKE_BINARY_DIR}/shaders/GaussianBlur_HighQuality_VS.cso")
    add_custom_command(
        OUTPUT ${VS_OUTPUT}
        COMMAND fxc /T vs_5_1 /E VSMain /Fo "${VS_OUTPUT}" "${BLUR_HQ_SHADER}"
        DEPENDS ${BLUR_HQ_SHADER} "${CMAKE_SOURCE_DIR}/shaders/GaussianBlur_Common.hlsli"
        COMMENT "Compiling GaussianBlur_HighQuality vertex shader"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${VS_OUTPUT})
    
    # ピクセルシェーダー
    set(PS_OUTPUT "${CMAKE_BINARY_DIR}/shaders/GaussianBlur_HighQuality_PS.cso")
    add_custom_command(
        OUTPUT ${PS_OUTPUT}
        COMMAND fxc /T ps_5_1 /E PSMain /Fo "${PS_OUTPUT}" "${BLUR_HQ_SHADER}"
        DEPENDS ${BLUR_HQ_SHADER} "${CMAKE_SOURCE_DIR}/shaders/GaussianBlur_Common.hlsli"
        COMMENT "Compiling GaussianBlur_HighQuality pixel shader"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${PS_OUTPUT})
endif()

# ===========================================
# Bloomシェーダー（複数エントリーポイント）
# ===========================================
if(EXISTS ${BLOOM_SHADER})
    # 頂点シェーダー（共通）
    set(VS_OUTPUT "${CMAKE_BINARY_DIR}/shaders/Bloom_VS.cso")
    add_custom_command(
        OUTPUT ${VS_OUTPUT}
        COMMAND fxc /T vs_5_1 /E VSMain /Fo "${VS_OUTPUT}" "${BLOOM_SHADER}"
        DEPENDS ${BLOOM_SHADER}
        COMMENT "Compiling Bloom vertex shader"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${VS_OUTPUT})
    
    # 輝度抽出パス
    set(PS_BRIGHT_OUTPUT "${CMAKE_BINARY_DIR}/shaders/Bloom_PSBrightPass.cso")
    add_custom_command(
        OUTPUT ${PS_BRIGHT_OUTPUT}
        COMMAND fxc /T ps_5_1 /E PSBrightPass /Fo "${PS_BRIGHT_OUTPUT}" "${BLOOM_SHADER}"
        DEPENDS ${BLOOM_SHADER}
        COMMENT "Compiling Bloom bright pass shader"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${PS_BRIGHT_OUTPUT})
    
    # 合成パス
    set(PS_COMPOSITE_OUTPUT "${CMAKE_BINARY_DIR}/shaders/Bloom_PSComposite.cso")
    add_custom_command(
        OUTPUT ${PS_COMPOSITE_OUTPUT}
        COMMAND fxc /T ps_5_1 /E PSComposite /Fo "${PS_COMPOSITE_OUTPUT}" "${BLOOM_SHADER}"
        DEPENDS ${BLOOM_SHADER}
        COMMENT "Compiling Bloom composite shader"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${PS_COMPOSITE_OUTPUT})
    
    # コピーパス
    set(PS_COPY_OUTPUT "${CMAKE_BINARY_DIR}/shaders/Bloom_PSCopy.cso")
    add_custom_command(
        OUTPUT ${PS_COPY_OUTPUT}
        COMMAND fxc /T ps_5_1 /E PSCopy /Fo "${PS_COPY_OUTPUT}" "${BLOOM_SHADER}"
        DEPENDS ${BLOOM_SHADER}
        COMMENT "Compiling Bloom copy shader"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${PS_COPY_OUTPUT})
    
    # ダウンサンプリング
    set(PS_DOWNSAMPLE_OUTPUT "${CMAKE_BINARY_DIR}/shaders/Bloom_PSDownsample.cso")
    add_custom_command(
        OUTPUT ${PS_DOWNSAMPLE_OUTPUT}
        COMMAND fxc /T ps_5_1 /E PSDownsample /Fo "${PS_DOWNSAMPLE_OUTPUT}" "${BLOOM_SHADER}"
        DEPENDS ${BLOOM_SHADER}
        COMMENT "Compiling Bloom downsample shader"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${PS_DOWNSAMPLE_OUTPUT})
    
    # アップサンプリング
    set(PS_UPSAMPLE_OUTPUT "${CMAKE_BINARY_DIR}/shaders/Bloom_PSUpsample.cso")
    add_custom_command(
        OUTPUT ${PS_UPSAMPLE_OUTPUT}
        COMMAND fxc /T ps_5_1 /E PSUpsample /Fo "${PS_UPSAMPLE_OUTPUT}" "${BLOOM_SHADER}"
        DEPENDS ${BLOOM_SHADER}
        COMMENT "Compiling Bloom upsample shader"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${PS_UPSAMPLE_OUTPUT})
endif()

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
