add_executable(test_pool test_pool.c)
target_link_libraries(test_pool PRIVATE frame)
add_test(NAME pool COMMAND test_pool)

add_executable(test_ring test_ring.c)
target_link_libraries(test_ring PRIVATE frame)
add_test(NAME ring COMMAND test_ring)

add_executable(test_transition test_transition.cpp)
target_link_libraries(test_transition PRIVATE core frame)
add_test(NAME transition COMMAND test_transition)

# Tests assert their invariants, so keep assert() live even in optimised builds.
foreach(target test_pool test_ring test_transition)
    target_compile_options(${target} PRIVATE -UNDEBUG)
endforeach()
