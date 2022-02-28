#define MY_DEBUG 1

#if MY_DEBUG
#define MY_KLEE_DEBUG(X) do { X; } while (0);
#else
#define MY_KLEE_DEBUG(X) do {} while (0);
#endif
