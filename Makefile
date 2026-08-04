DB_TARGET = bin/debug/tests
RL_TARGET = bin/release/tests

.PHONY: all test debug release

all: release

test: debug
	./$(DB_TARGET)

debug:
	cmake --build --preset debug

release:
	cmake --build --preset release

clean:
	rm -rf bin build