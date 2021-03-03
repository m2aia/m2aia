message(STATUS "Moving DLLs to bin directory")

# file(GLOB OpenSlide_dlls bin/*.dll)

# execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory bin)

# foreach(OpenSlide_dll ${OpenSlide_dlls})
#   execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${OpenSlide_dll} ../bin)
#   execute_process(COMMAND ${CMAKE_COMMAND} -E remove ${OpenSlide_dll})
# endforeach()
