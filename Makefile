TARGET_EXEC ?= vulkan-triangle-v2

BUILD_DIR ?= ./obj
SRC_DIRS ?= src vendor

SRCS := $(shell find $(SRC_DIRS) -type f -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

INC_DIRS := vendor
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

LDFLAGS := -lm -lvulkan -lglfw -lpthread -fsanitize=address 

CC := clang
CFLAGS ?= $(INC_FLAGS) -std=gnu2x -MMD -MP -O0 -g3 -Wall -Weverything -pedantic -Wno-padded -Wno-switch-enum -fsanitize=address
 
#CC := clang
#CFLAGS ?= $(INC_FLAGS) $(LDFLAGS) -std=c11 --analyze -MMD -MP -O0 -g3 -Wall -Weverything -pedantic -Wno-padded -Wno-switch-enum 

#CC := gcc
#CFLAGS ?= $(INC_FLAGS) $(LDFLAGS) -fanalyzer -std=c11 -MMD -MP -O0 -g3 -Wall -pedantic -Wno-padded -Wno-switch-enum

#CC := afl-gcc
#CFLAGS ?= $(INC_FLAGS) $(LDFLAGS) -std=c11 -MMD -MP -O0 -g3 -Wall -pedantic -Wno-padded -Wno-switch-enum

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)


-include $(DEPS)

MKDIR_P ?= mkdir -p
