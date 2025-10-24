# ソースファイルを収集
set(NEONVECTOR_SOURCES
    # Core
    Core/Application.cpp
    Core/DX12Context.cpp
    
    # Math (実装が必要な場合のみ)
    # Math/Vector2.cpp
    
    # Graphics (将来追加)
    # Graphics/LineBatcher.cpp
    # Graphics/Renderer.cpp
)

# ヘッダーファイルを収集
set(NEONVECTOR_HEADERS
    ${CMAKE_SOURCE_DIR}/include/NeonVector/NeonVector.h
    ${CMAKE_SOURCE_DIR}/include/NeonVector/Core/Application.h
    ${CMAKE_SOURCE_DIR}/include/NeonVector/Core/Types.h
    ${CMAKE_SOURCE_DIR}/include/NeonVector/Math/Vector2.h
)

# 内部ヘッダー
set(NEONVECTOR_PRIVATE_HEADERS
    DX12Context.h
)

# 静的ライブラリとして作成
add_library(NeonVector STATIC
    ${NEONVECTOR_SOURCES}
    ${NEONVECTOR_HEADERS}
    ${NEONVECTOR_PRIVATE_HEADERS}
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
)

# プリコンパイル済みヘッダー（オプション、後で追加可能）
# target_precompile_headers(NeonVector PRIVATE pch.h)

message(STATUS "NeonVector library configured")