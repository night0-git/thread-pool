DB_TARGET = bin/debug/tests
RL_TARGET = bin/release/tests

.PHONY: all test debug release

all: release

test: debug
	./$(DB_TARGET)

debug:
	cmake --build --preset debug -j

release:
	cmake --build --preset release -j

clean:
	rm -rf bin build