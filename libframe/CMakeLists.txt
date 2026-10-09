add_library(frame STATIC
    src/frame.c
    src/ring.c
    src/clock.c
    src/pattern.c
)
target_include_directories(frame PUBLIC include)
set_target_properties(frame PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
