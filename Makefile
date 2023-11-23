CXX      := gcc
LD       := gcc
CXXFLAGS := -std=c17 -Wall -Wextra -Wpedantic -g
LDFLAGS  := -lSDL2_ttf -lSDL2_mixer -lSDL2_image -lSDL2
TARGET   := $(shell basename $(CURDIR))
CPPFILES := $(wildcard src/*.c) $(wildcard src/*/*.c)
OBJFILES := $(CPPFILES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJFILES)
	@echo "linking $@..."
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	@echo "compiling $<..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<
