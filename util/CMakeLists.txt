project(utilLib)
add_library(util SHARED "Log.cpp" "Log.h"
                       "Params.cpp" "Params.h"
		       "Member.cpp" "Member.h")

install(TARGETS util DESTINATION lib)
install(FILES "Log.h" "Member.h" "Params.h" "Queue.h" DESTINATION include)
