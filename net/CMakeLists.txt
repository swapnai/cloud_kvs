project(netLib)
add_library(net SHARED "EmulNet.cpp" "EmulNet.h")

include_directories(${utilLib_SOURCE_DIR})

target_link_libraries(net util)

install(TARGETS net DESTINATION lib)
install(FILES "EmulNet.h" DESTINATION include)
