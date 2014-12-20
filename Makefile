TARGET = l_profiler.so

.PHONY: all clean

all: $(TARGET)

$(TARGET) : l_profiler.cpp profiler.cpp
	g++ -g -Wall -std=c++0x -bundle -undefined dynamic_lookup -o $@ $^

clean:
	@(rm -rf $(TARGET) *.so.dSYM)

