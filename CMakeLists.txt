cmake_minimum_required(VERSION 3.20)
project(my_rv32_sim VERSION 1.0)
set(SOURCES 
    src/main.cc 
    src/core.cc
    src/cpu_io.cc 
    src/cpu_exception.cc 
    src/plic.cc
)
add_executable(simulator ${SOURCES})
target_include_directories(simulator PRIVATE src)
aux_source_directory(src SOURCES)
add_compile_options(-O3 -march=native -flto)