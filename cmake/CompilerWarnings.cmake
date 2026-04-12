function(set_project_warnings target WARNINGS_AS_ERRORS)
    if (MSVC)
        set(WARNINGS
            /W4
            /permissive-
        )

        if (WARNINGS_AS_ERRORS)
            list(APPEND WARNINGS /WX)
        endif()

    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set(WARNINGS
            -Wall
            -Wextra
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
            -Wimplicit-fallthrough
        )

        if (WARNINGS_AS_ERRORS)
            list(APPEND WARNINGS -Werror)
        endif()
    endif()

    target_compile_options(${target} INTERFACE ${WARNINGS})
endfunction()