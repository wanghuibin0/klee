add_klee_unit_test(IntervalInfoTest
  IntervalInfoTest.cpp)
target_link_libraries(IntervalInfoTest)
target_include_directories(IntervalInfoTest BEFORE PUBLIC "../../lib/Module/")
