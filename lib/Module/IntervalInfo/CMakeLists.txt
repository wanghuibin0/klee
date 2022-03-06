set(KLEE_INTERVALINFO_SRCS
  IntervalInfo.cpp
  Interval.cpp
  )

klee_add_component(kleeInterval
  ${KLEE_INTERVALINFO_SRCS}
  )

set(LLVM_COMPONENTS
  support
  )

klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(kleeInterval PUBLIC ${LLVM_LIBS})
target_link_libraries(kleeInterval PRIVATE
  kleeSupport
  )

