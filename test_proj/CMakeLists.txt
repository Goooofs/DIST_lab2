add_executable(test_proj main.cpp)
target_link_libraries(test_proj PRIVATE allocator)
target_include_directories(test_proj PRIVATE ${CMAKE_SOURCE_DIR}/allocator)
