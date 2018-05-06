cmake_minimum_required(VERSION 3.4.1)

# Compiler options
add_compile_options(-Wall)

# Submodule library
add_library(pak
            STATIC

            # Source
            pak_file.c
            pak_log.c)

# Linking
target_link_libraries(pak

                      # NDK libraries
                      log)
