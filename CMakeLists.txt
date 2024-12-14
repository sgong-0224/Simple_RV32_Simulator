cmake_minimum_required(VERSION 3.20)
project(my_rv32_sim VERSION 0.6)
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
add_compile_options(-Ofast -march=native -mavx2 -flto=thin)