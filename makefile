COMPILER = g++
SOURCES = main.cxx # $(shell find src -name '*.c')
OUTPUT = game.out
FLAGS = -Iinclude -Llib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

default: build

build:
	$(COMPILER) $(SOURCES) -o $(OUTPUT) $(FLAGS)

game: build
	./$(OUTPUT)

clean:
	rm -f $(OUTPUT)
