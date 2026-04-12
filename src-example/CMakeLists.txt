add_executable(BinaryStream_example main.cpp)
target_link_libraries(BinaryStream_example PRIVATE BinaryStream)

if (TARGET BinaryStream_warnings)
    target_link_libraries(BinaryStream_example PRIVATE BinaryStream_warnings)
endif()