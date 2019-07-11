CC=gcc

CFLAGS= -Wall
ifndef RELEASE
	CFLAGS += -g -DDEBUG
endif

CLIBS = -lcommons -pthread -lrt -lreadline

PATHS = $(shell find src/ test/ -type d -printf "%p ")

vpath %.h $(PATHS)
vpath %.c $(PATHS)

BUILD_DIR = build

COMMON_S = $(shell find src/commons/ -name "*.c" -type f -printf "%f ")
COMMON_O = $(patsubst %.c,$(BUILD_DIR)/%.o,$(COMMON_S))

KERNEL_S = $(shell find src/kernel/ -name "*.c" -type f -printf "%f ")
KERNEL_O = $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_S))

MEMORY_S = $(shell find src/memoria/ -name "*.c" -type f -printf "%f ")
MEMORY_O = $(patsubst %.c,$(BUILD_DIR)/%.o,$(MEMORY_S))

LSF_S = $(shell find src/lfs/ -type f -name "*.c" -printf "%f ")
LSF_O = $(patsubst %.c,$(BUILD_DIR)/%.o,$(LSF_S))

-include $(COMMON_O:.o=.d)
-include $(KERNEL_O:.o=.d)
-include $(MEMORY_O:.o=.d)
-include $(LSF_O:.o=.d)

.PHONY: all
all: kernel memoria lfs

kernel: $(COMMON_O) $(KERNEL_O)
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/$@.out $(CLIBS)

memoria: $(COMMON_O) $(MEMORY_O)
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/$@.out $(CLIBS)

lfs: $(COMMON_O) $(LSF_O)
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/$@.out $(CLIBS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(CLIBS)
	$(CC) -MM -MT '$(BUILD_DIR)/$*.o' $< > $(BUILD_DIR)/$*.d

clean:
	@rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d $(BUILD_DIR)/*.out *.out
