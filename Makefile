CC = cc
CFLAGS = -Wall -Wextra -std=c11 -O2 -Ilib
LDFLAGS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

# GLFW (via homebrew)
GLFW_CFLAGS = $(shell pkg-config --cflags glfw3 2>/dev/null || echo "-I/opt/homebrew/include")
GLFW_LIBS = $(shell pkg-config --static --libs glfw3 2>/dev/null || echo "-L/opt/homebrew/lib -lglfw")

CFLAGS += $(GLFW_CFLAGS)
LDFLAGS += $(GLFW_LIBS)

SRC = src/main.c src/lexer.c src/parser.c src/ast.c src/renderer.c src/editor.c
OBJ = $(SRC:.c=.o)
BIN = scenelang

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

run: $(BIN)
	./$(BIN) examples/demo.scene

edit: $(BIN)
	./$(BIN)

.PHONY: all clean run edit
