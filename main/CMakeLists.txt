project(mainApp)

add_executable(App main.cpp)

include_directories(${utilLib_SOURCE_DIR})
include_directories(${netLib_SOURCE_DIR})
include_directories(${nodeLib_SOURCE_DIR})

target_link_libraries(App "node" "net" "util")

install(TARGETS App DESTINATION bin)
