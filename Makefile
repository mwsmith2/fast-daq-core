# Grab the targets and sources as two batches
OBJECTS = $(patsubst src/%.cxx, build/%.o, $(wildcard src/*.cxx))
OBJ_VME = $(patsubst include/vme/%.c, build/%.o, $(wildcard include/vme/*.c))
OBJ_DRS = $(patsubst src/drs/%.cpp, build/%.o, $(wildcard src/drs/*.cpp))
OBJ_DRS += $(patsubst src/drs/%.c, build/%.o, $(wildcard src/drs/*.c))

# Library info.
MAJOR=0
MINOR=1.0
SONAME=libuwlab.so
ARNAME=libuwlab.a
LIBNAME=$(SONAME).$(MAJOR).$(MINOR)
PREFIX=/usr/local


# Figure out the architecture
UNAME_S = $(shell uname -s)

# Clang compiler
ifeq ($(UNAME_S), Darwin)
	CXX = clang++
	CC  = clang
	FLAGS = -std=c++11 -DOS_DARWIN
endif

# Gnu compiler
ifeq ($(UNAME_S), Linux)
	CXX = g++
	CC  = gcc
	FLAGS = -std=c++0x -DOS_LINUX
endif

ifdef DEBUG
	FLAGS += -g -fPIC -O3
else
	FLAGS += -fPIC -O3
endif

# DRS flags
FLAGS += -DHAVE_USB -DHAVE_LIBUSB10 -DUSE_DRS_MUTEX

# ROOT libs and flags
FLAGS += $(shell root-config --cflags)
LIBS = $(shell root-config --libs)

# wxWidgets libs and flags
WXLIBS        = $(shell wx-config --libs)
WXFLAGS       = $(shell wx-config --cxxflags)

FLAGS += -Iinclude -Iinclude/drs
LIBS += -lm -lzmq -ljson_spirit -lCAENDigitizer -lusb-1.0 -lutil

all: $(OBJECTS) $(OBJ_VME) $(OBJ_DRS) $(TARGETS) lib/$(ARNAME)

fe_%: modules/fe_%.cxx $(OBJECTS) $(OBJ_VME) $(OBJ_DRS)
	$(CXX) $< -o $@  $(FLAGS) $(WXFLAGS) \
	$(OBJECTS) $(OBJ_VME) $(OBJ_DRS) $(LIBS) $(WXLIBS)

%_daq: modules/%_daq.cxx 
	$(CXX) $< -o $@  $(FLAGS) $(LIBS)

build:
	mkdir build

build/%.o: src/%.cxx
	$(CXX) -c $< -o $@ $(FLAGS)

build/%.o: include/vme/%.c
	$(CC) -c $< -o $@

build/%.o: src/drs/%.cpp
	$(CXX) -c $< -o $@ $(FLAGS) $(WXFLAGS)

build/%.o: src/drs/%.c
	$(CC) -c $< -o $@ $(FLAGS) $(WXFLAGS)

#lib/$(SONAME): $(OBJECTS) $(OBJ_VME) $(OJB_DRS)
#	$(CXX) -shared -fPIC $+ -o $@ $(LIBS)

lib/$(ARNAME): $(OBJECTS) $(OBJ_VME) $(OJB_DRS)
	$(AR) -rcs $@ $+

clean:
	rm -f $(TARGETS) $(OBJECTS) $(OBJ_VME) $(OBJ_DRS)
