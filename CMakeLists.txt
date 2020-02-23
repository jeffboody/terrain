cmake_minimum_required(VERSION 3.4.1)

# Compiler options
add_compile_options(-Wall)

# Submodule library
add_library(terrain
            STATIC

            # Source
            terrain_tile.c
            terrain_util.c)

# Linking
target_link_libraries(terrain

                      # NDK libraries
                      z
                      log)
