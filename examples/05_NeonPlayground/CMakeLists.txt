# 05_NeonPlayground の CMakeLists.txt
add_executable(NeonPlayground main.cpp)

target_link_libraries(NeonPlayground PRIVATE NeonVector)

# シェーダーファイルを実行ファイルと同じディレクトリにコピー
add_custom_command(TARGET NeonPlayground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:NeonPlayground>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:NeonPlayground>/shaders
    COMMENT "Copying shader files to output directory"
)
