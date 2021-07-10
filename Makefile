CXXFLAGS = -std=c++20 -Wall -Wextra -O3 -Wno-char-subscripts $(CXXFLAGS_EXTRA) -I src
#SANITIZE = true

ifdef SANITIZE
CXXFLAGS += -g -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
endif

OUT = notec
OUT_ELF = $(OUT).elf
OUT_BIN = $(OUT).bin
OUT_G1A = $(OUT).g1a

COMMON_SRC = $(wildcard src/*.cpp)
OUT_SRC = $(wildcard src/crs/*.cpp)
TEST_SRC = $(wildcard test/*.cpp)
TEST_STL_SRC = $(wildcard test/stl/*.cpp)

COMMON_OBJ = $(COMMON_SRC:.cpp=.o)
OUT_OBJ = $(OUT_SRC:.cpp=.o)
TEST_OBJ = $(TEST_SRC:.cpp=.o)
TEST_STL_OBJ = $(TEST_STL_SRC:.cpp=.o)

OUT_ALL_OBJ = $(COMMON_OBJ) $(OUT_OBJ)
TEST_ALL_OBJ = $(COMMON_OBJ) $(TEST_OBJ) $(TEST_STL_OBJ)

TEST_OUT = test.exe

all: $(OUT_G1A)

define n


endef

check_sh_elf_cxx:
ifndef SH_ELF_CXX
	$(error "SH_ELF_CXX must be defined for cross-compilation.$nYou can still build 'test' target for running unit local tests")
endif
$(OUT_ELF): CXX = $(SH_ELF_CXX)
$(OUT_ELF): CXXFLAGS_EXTRA = -Wno-builtin-declaration-mismatch -m3 -mb -mrenesas -nostdlib
$(OUT_ELF): check_sh_elf_cxx $(OUT_ALL_OBJ)
	$(CXX) $(CXXFLAGS) $(OUT_ALL_OBJ) -T lnk/notec.ld lnk/crt0.s -o $(OUT_ELF) -lfx

$(OUT_BIN): $(OUT_ELF)
	sh-elf-objcopy -R .comment -R .bss -O binary $(OUT_ELF) $(OUT_BIN)

$(OUT_G1A): $(OUT_BIN)
	g1a-wrapper $(OUT_BIN) -o $(OUT_G1A) -i ./lnk/icon.bmp

$(TEST_OBJ) $(TEST_STL_OBJ): CXXFLAGS_EXTRA = -D CINT_HOST -I test
$(TEST_OUT): $(TEST_ALL_OBJ)
	$(CXX) $(CXXFLAGS) $(TEST_ALL_OBJ) -o $(TEST_OUT)

test: $(TEST_OUT)

clean:
	rm -f $(COMMON_OBJ) $(OUT_OBJ) $(TEST_OBJ) $(OUT_ELF) $(OUT_BIN) $(OUT_G1A) $(TEST_OUT)

clean_all: clean
	rm -f $(TEST_STL_OBJ)

.PHONY: all clean check_sh_elf_cxx