CC       := gcc
CXX      := g++

CFLAGS   := -g
CXXFLAGS := -g
LDFLAGS  := -ltag -lm

SRC_DIR  := .
OBJ_DIR  := build
TARGET   := $(OBJ_DIR)/main

# ============================================================================
# Source files
# ============================================================================

C_SOURCES   := $(shell find $(SRC_DIR) -type f -name '*.c')
CPP_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.cpp')

OBJECTS := \
	$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES)) \
	$(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CPP_SOURCES))

DEPS := $(OBJECTS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

.PHONY: all clean

clean:
	rm -rf $(OBJ_DIR)
