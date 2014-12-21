TARGET = l_profiler.so

.PHONY: all clean

UNAME=$(shell uname)
SYS=$(if $(filter Linux%, $(UNAME)), linux,\
        $(if $(filter Darwin%, $(UNAME)), macos,\
	        undefined))

all: $(SYS)

undefined:
	@echo "please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "      macos linux"

linux: CFLAGS = -g -Wall -std=c++0x -fPIC --shared -o
linux: LIBS = /usr/local/lib/liblua.a
linux: $(TARGET)

macos: CFLAGS = -g -Wall -std=c++0x -bundle -undefined dynamic_lookup -o
macos: LIBS =
macos: $(TARGET)

$(TARGET) : l_profiler.cpp profiler.cpp
	g++ $(CFLAGS) $@ $^ $(LIBS)

clean:
	@(rm -rf $(TARGET) *.so.dSYM)

