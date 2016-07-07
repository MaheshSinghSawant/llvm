#set (lli_cmd "lli")
#set (lli_arg "hello.ll")
#execute_process(COMMAND ${lli_cmd} ${lli_arg})

#set (git_cmd "git")
#set (git_arg "--version")
#message(STATUS "git cmd: ${git_cmd}")
#execute_process(COMMAND ${git_cmd} ${git_arg}
#        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
#        RESULT_VARIABLE git_result
#        OUTPUT_VARIABLE git_ver)
#
#message(STATUS "git ver[${git_result}]: ${git_ver}")

set (SOURCE_FILES try.c)
add_executable (Drac ${SOURCE_FILES})
