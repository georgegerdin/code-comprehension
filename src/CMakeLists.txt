set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_library(code-comprehension
        filedb.cc
        codecomprehensionengine.cc
        cpp/cppcomprehensionengine.cc
)

add_executable(test
    main.cc
)

target_link_libraries(code-comprehension PUBLIC cpp-parser)
target_link_libraries(test PUBLIC code-comprehension)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/project_source_dir.txt" "${PROJECT_SOURCE_DIR}")