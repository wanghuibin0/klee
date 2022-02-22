add_klee_unit_test(IntervalInfoTest
  IntervalInfoTest.cpp)
target_link_libraries(IntervalInfoTest PRIVATE kleeInterval)
target_include_directories(IntervalInfoTest BEFORE PUBLIC "../../lib/Module/")
