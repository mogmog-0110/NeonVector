add_executable(LineDrawing
    main.cpp
)

target_link_libraries(LineDrawing
    PRIVATE
        NeonVector
)

# 実行ファイルの出力先
set_target_properties(LineDrawing PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)