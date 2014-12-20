AR = ar
CC = g++
CCFLAGS = -g -Wall -std=c++0x
TARGET = glprof.a
SRC = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp, %.o, $(SRC))
INCLUDE = -I.

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) -r $@ $^

%.o: %.cpp
	$(CC) -c $(CCFLAGS) $< -o $@ $(INCLUDE)

clean:
	@(rm -rf $(TARGET) $(OBJS))

