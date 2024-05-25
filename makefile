# Variables
CXX = g++
CXXFLAGS = -O2 -Wall -Wextra -std=c++17
GIT_VERSION := $(shell git describe --always --tags)

# Adding conditional flags for Raspberry Pi/armv6
ifeq ($(shell uname -m),armv6l)  # or "ifneq (,$(findstring arm, $(shell uname -m)))" for more general check
CXXFLAGS += -Wl,--no-keep-memory
endif
LIBS = sqlite3.o -lpthread -ldl
TARGET = cpu-temp-logger 
SOURCES = cpu-temp-logger.cpp 

# Rules
all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -DGIT_VERSION=\"$(GIT_VERSION)\" -o $@ $(INCLUDES) $^ $(LIBS)

clean:
	rm -f $(TARGET)
