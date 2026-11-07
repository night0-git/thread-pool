BIN_DIR_RL = bin/release
BIN_DIR_DB = bin/debug

TEST_RL = $(BIN_DIR_RL)/tests
TEST_DB = $(BIN_DIR_DB)/tests
BENCH_RL = $(BIN_DIR_RL)/benchmarks

.PHONY: all release debug test bench stress clean

all: release

release:
	cmake --preset release
	cmake --build --preset release -j

debug:
	cmake --preset debug
	cmake --build --preset debug -j

test: release
	$(TEST_RL)

bench: release
	$(BENCH_RL)

stress: debug
	TSAN_OPTIONS="verbosity=1" $(TEST_DB) "[stress]"

clean:
	rm -rf bin build
