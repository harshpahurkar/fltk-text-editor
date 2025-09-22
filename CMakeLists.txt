cmake_minimum_required(VERSION 3.16)
project(fltk_text_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(FLTK REQUIRED)

add_executable(fltk_text_editor
    src/editor.cpp
)

if(TARGET FLTK::FLTK)
    target_link_libraries(fltk_text_editor PRIVATE FLTK::FLTK)
else()
    target_include_directories(fltk_text_editor PRIVATE ${FLTK_INCLUDE_DIR})
    target_link_libraries(fltk_text_editor PRIVATE ${FLTK_LIBRARIES})
endif()
