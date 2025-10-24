# 03_BasicShapes のCMakeLists.txt

add_executable(BasicShapes
    main.cpp
)

target_link_libraries(BasicShapes
    PRIVATE
        NeonVector
)

# 実行ファイルの出力先
set_target_properties(BasicShapes PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Visual Studioでのフォルダ分け
set_target_properties(BasicShapes PROPERTIES
    FOLDER "Examples"
)
