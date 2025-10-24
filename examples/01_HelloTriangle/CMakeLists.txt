# HelloTriangle実行ファイル
add_executable(HelloTriangle
    main.cpp
)

# NeonVectorライブラリとリンク
target_link_libraries(HelloTriangle
    PRIVATE
        NeonVector
)

# 実行ファイルのプロパティ
set_target_properties(HelloTriangle PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

message(STATUS "HelloTriangle example configured")