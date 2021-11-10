EXE = test-shunting-yard
CORE_SRC = shunting-yard.cpp packToken.cpp functions.cpp containers.cpp
SRC = $(EXE).cpp $(CORE_SRC) builtin-features.cpp catch.cpp
OBJ = $(SRC:.cpp=.o)

LD ?= ld
CXX ?= g++
CFLAGS = -std=c++11 -Wall -pedantic -Wmissing-field-initializers -Wuninitialized
DEBUG = -g #-DDEBUG

ifeq ($(OS),Windows_NT) # is Windows_NT on XP, 2000, 7, Vista, 10...
	detected_OS := Windows
else
	detected_OS := $(shell uname)
endif

LDFLAGS=
ifeq ($(detected_OS), Linux)
	LDFLAGS += -O1
endif

release: $(CORE_SRC:.cpp=.o) builtin-features.cpp;
	$(CXX) -c -O3 $(CFLAGS) builtin-features.cpp
	$(LD) -r $(LDFLAGS) $(CORE_SRC:.cpp=.o) -o core-shunting-yard.o

all: $(EXE) release

$(EXE): $(OBJ); $(CXX) $(CFLAGS) $(DEBUG) $(OBJ) -o $(EXE)
%.o: %.cpp *.h %/*; $(CXX) $(CFLAGS) $(DEBUG) -c $< -o $@ $(DEBUG)
%.o: %.cpp *.h; $(CXX) $(CFLAGS) $(DEBUG) -c $< -o $@ $(DEBUG)

again: clean all

test: $(EXE); ./$(EXE) $(args)

check: $(EXE); valgrind --leak-check=full ./$(EXE) $(args)

simul: $(EXE); cgdb --args ./$(EXE) $(args)

clean: ; rm -f $(EXE) $(OBJ) core-shunting-yard.o full-shunting-yard.o
