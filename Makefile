CXXFLAGS = -std=c++20 -Wall -Wextra -O3 $(CXXFLAGS_EXTRA)

OUT = notec
OUT_ELF = $(OUT).elf
OUT_BIN = $(OUT).bin
OUT_G1A = $(OUT).g1a

COMMON_SRC = 
OUT_SRC = src/main.cpp
TEST_SRC = src/test/token.cpp

COMMON_OBJ = $(COMMON_SRC:.cpp=.o)
OUT_OBJ = $(OUT_SRC:.cpp=.o)
TEST_OBJ = $(TEST_SRC:.cpp=.o)

OUT_ALL_OBJ = $(COMMON_OBJ) $(OUT_OBJ)
TEST_ALL_OBJ = $(COMMON_OBJ) $(TEST_OBJ)

TEST_OUT = test.exe

all: $(OUT_G1A)

define n


endef

$(OUT_ELF):
ifndef SH_ELF_CXX
	$(error "SH_ELF_CXX must be defined for cross-compilation.$nYou can still build 'test' target for running unit local tests")
endif
$(OUT_ELF): CXX = $(SH_ELF_CXX)
$(OUT_ELF): CXXFLAGS_EXTRA = -Wno-builtin-declaration-mismatch -m3 -mb -mrenesas -nostdlib
$(OUT_ELF): $(OUT_ALL_OBJ)
	$(CXX) $(CXXFLAGS) $(OUT_ALL_OBJ) -T lnk/notec.ld lnk/crt0.s -o $(OUT_ELF) -lfx

$(OUT_BIN): $(OUT_ELF)
	sh-elf-objcopy -R .comment -R .bss -O binary $(OUT_ELF) $(OUT_BIN)

$(OUT_G1A): $(OUT_BIN)
	g1a-wrapper $(OUT_BIN) -o $(OUT_G1A) -i ./lnk/icon.bmp

$(TEST_OUT): $(TEST_ALL_OBJ)
	$(CXX) $(CXXFLAGS) $(TEST_ALL_OBJ) -o $(TEST_OUT) -lboost_test_exec_monitor-mt

test: $(TEST_OUT)

clean:
	rm -f $(COMMON_OBJ) $(OUT_OBJ) $(TEST_OBJ) $(OUT_ELF) $(OUT_BIN) $(OUT_G1A) $(TEST_OUT)

.PHONY: all clean