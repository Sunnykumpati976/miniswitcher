# Plain-make fallback so the project builds on a box without CMake.
# CMake remains the primary build (scripts/build.sh); this exists so a
# reviewer can type `make && make test` and be done.

CC      ?= cc
CXX     ?= c++
WARN     = -Wall -Wextra -Wpedantic -Wshadow -Wconversion
CFLAGS   = -std=c11 -O2 -g $(WARN) -Ilibframe/include
CXXFLAGS = -std=c++20 -O2 -g $(WARN) -Ilibframe/include -Icore/include

# Some Command Line Tools installs leave the toolchain's libc++ headers only
# inside the SDK, so <memory> is not on the default search path. Point at the
# SDK copy when the toolchain copy is missing.
ifeq ($(shell uname -s),Darwin)
  SDK := $(shell xcrun --show-sdk-path 2>/dev/null)
  TOOLCHAIN_CXX := $(shell dirname $(shell xcrun -f clang++ 2>/dev/null))/../include/c++/v1
  ifeq ($(wildcard $(TOOLCHAIN_CXX)/memory),)
    CXXFLAGS += -isystem $(SDK)/usr/include/c++/v1
  endif
endif

BUILD    = build-make
LIB_SRC  = $(wildcard libframe/src/*.c)
LIB_OBJ  = $(patsubst libframe/src/%.c,$(BUILD)/%.o,$(LIB_SRC))
CORE_OBJ = $(BUILD)/transition.o
TOOLS    = $(BUILD)/gen_pattern $(BUILD)/switch_demo
TESTS    = $(BUILD)/test_pool $(BUILD)/test_ring $(BUILD)/test_transition

.PHONY: all test demo bench clean
all: $(TOOLS)

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/%.o: libframe/src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(CORE_OBJ): core/src/transition.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/gen_pattern: tools/gen_pattern.c $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/switch_demo: tools/switch_demo.cpp $(CORE_OBJ) $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

# -UNDEBUG keeps assert() live even in an optimised test binary.
$(BUILD)/test_pool: tests/test_pool.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -UNDEBUG $^ -o $@
$(BUILD)/test_ring: tests/test_ring.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -UNDEBUG $^ -o $@
$(BUILD)/test_transition: tests/test_transition.cpp $(CORE_OBJ) $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) -UNDEBUG $^ -o $@

test: $(TESTS)
	@for t in $(TESTS); do ./$$t || exit 1; done

demo: $(BUILD)/switch_demo
	./$(BUILD)/switch_demo --width 640 --height 360 -o demo.yuv

bench: $(BUILD)/switch_demo
	./$(BUILD)/switch_demo --bench --width 1920 --height 1080 --fps 60

clean:
	rm -rf $(BUILD) build build-asan demo.yuv
