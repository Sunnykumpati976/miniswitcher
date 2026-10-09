add_library(core STATIC
    src/transition.cpp
)
target_include_directories(core PUBLIC include)
target_link_libraries(core PUBLIC frame)
set_target_properties(core PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
