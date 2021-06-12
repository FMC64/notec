#CXX = sh3eb-elf-gcc-11.1.1	# Recommanded C++ compiler
CXXFLAGS = -std=c++20 -Wall -Wextra -Wno-builtin-declaration-mismatch -O3 -m3 -mb -mrenesas -nostdlib

OUT = notec
OUT_ELF = $(OUT).elf
OUT_BIN = $(OUT).bin
OUT_G1A = $(OUT).g1a
SRC = src/main.cpp
OBJ = $(SRC:.cpp=.o)

all: $(OUT_G1A)

$(OUT_ELF): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -T lnk/notec.ld lnk/crt0.s -o $(OUT_ELF) -lgcc -lfx

$(OUT_BIN): $(OUT_ELF)
	sh-elf-objcopy -R .comment -R .bss -O binary $(OUT_ELF) $(OUT_BIN)

$(OUT_G1A): $(OUT_BIN)
	g1a-wrapper $(OUT_BIN) -o $(OUT_G1A) -i ./lnk/icon.bmp

clean:
	rm -f $(OBJ) $(OUT_ELF) $(OUT_BIN) $(OUT_G1A)

.PHONY: all clean