# 04_BloomDemo の CMakeLists.txt
add_executable(BloomDemo main.cpp)

target_link_libraries(BloomDemo PRIVATE NeonVector)

# シェーダーファイルを実行ファイルと同じディレクトリにコピー
add_custom_command(TARGET BloomDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying shaders from ${CMAKE_SOURCE_DIR}/shaders to $<TARGET_FILE_DIR:BloomDemo>/shaders"
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:BloomDemo>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:BloomDemo>/shaders
    COMMENT "Copying shader files to output directory"
)
