CXXFLAGS = -std=c++20 -Wall -Wextra -O3 -Os $(CXXFLAGS_EXTRA) -I src -I ../../src
#SANITIZE = true

ifdef SANITIZE
CXXFLAGS += -g -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
endif

OUT = fs.exe

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)

all: $(OUT)

$(OUT): CXXFLAGS_EXTRA = -D CINT_HOST -I test
$(OUT): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(OUT)

fs: $(OUT)

clean:
	rm -f $(OBJ) $(OUT)

.PHONY: all fs clean