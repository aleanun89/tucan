cmake_minimum_required(VERSION 3.10)
project(tucan C)
# Envoltorio: compila con el Makefile (la fuente de la verdad) y deja el binario en build/portable/.
add_custom_target(tucan ALL COMMAND ${CMAKE_COMMAND} -E env make -C ${CMAKE_SOURCE_DIR} -j4
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} VERBATIM)
