add_executable(gen_pattern gen_pattern.c)
target_link_libraries(gen_pattern PRIVATE frame)

add_executable(switch_demo switch_demo.cpp)
target_link_libraries(switch_demo PRIVATE core frame)
