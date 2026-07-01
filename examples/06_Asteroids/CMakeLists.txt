# 06_Asteroids の CMakeLists.txt
add_executable(Asteroids main.cpp)

target_link_libraries(Asteroids PRIVATE NeonVector)

# シェーダーファイルを実行ファイルと同じディレクトリにコピー
add_custom_command(TARGET Asteroids POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Asteroids>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:Asteroids>/shaders
    COMMENT "Copying shader files to output directory"
)
