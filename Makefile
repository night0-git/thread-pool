DB_TARGET = bin/debug/tests
RL_TARGET = bin/release/tests

.PHONY: all test debug release tsan

all: release

debug:
	cmake --build --preset debug -j

tsan:
	cmake --build --preset tsan -j

release:
	cmake --build --preset release -j

run: $(DB_TARGET)
	./$(DB_TARGET)

clean:
	rm -rf bin build