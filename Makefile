CXX ?= clang++
# -MMD generates .d file for each .o file
# -MP creates dummy targets for headers so deleting one does not break make
CXXFLAGS := -std=c++23 -MMD -MP -Wall -Wextra

TARGET := bin/out
BUILD_DIR := build
SRC_DIR := src

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: clean run debug all

all: release

debug: CXXFLAGS += -g -O0
release: CXXFLAGS += -O3

debug release: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(dir $(TARGET))

# The leading ensures make will not fail if the .d files do not exist yet
-include $(DEPS)