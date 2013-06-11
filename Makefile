EXE=test-shunting-yard
SRC=$(EXE).cpp shunting-yard.cpp
OBJ=$(SRC:.cpp=.o)

CXX=g++
CFLAGS=-g -Wall -pedantic
all: $(EXE)

$(EXE): $(OBJ); $(CXX) $(CFLAGS) $(OBJ) -o $(EXE)
%.o: %.cpp; $(CXX) $(CFLAGS) -c $< -o $@ $(DEBUG)

clean: ; rm -f $(EXE) $(OBJ)
