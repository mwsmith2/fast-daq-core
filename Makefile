# Grab the targets and sources as two batches
OBJECTS = $(patsubst src/%.cxx, build/%.o, $(wildcard src/*.cxx))
OBJ_VME = $(patsubst include/vme/%.c, build/%.o, $(wildcard include/vme/*.c))
OBJ_DRS = $(patsubst src/drs/%.cpp, build/%.o, $(wildcard src/drs/*.cpp))
OBJ_DRS += $(patsubst src/drs/%.c, build/%.o, $(wildcard src/drs/*.c))
HEADERS = $(wildcard include/*)
DATADEF = include/common.hh include/common_extdef.hh
LOGFILE = /var/log/lab-daq/fast-daq.log
CONFDIR = /usr/local/opt/lab-daq/config

# Library info.
MAJOR=0
MINOR=2.0
SONAME=liblabdaq.so
ARNAME=liblabdaq.a
LIBNAME=$(SONAME).$(MAJOR).$(MINOR)
PREFIX=/usr/local

# Figure out the architecture
UNAME_S = $(shell uname -s)

# Clang compiler
ifeq ($(UNAME_S), Darwin)
	CXX = clang++
	CC  = clang
	CPPFLAGS = -DOS_DARWIN
	CXXFLAGS = -std=c++11
endif

# Gnu compiler
ifeq ($(UNAME_S), Linux)
	CXX = g++
	CC  = gcc
	CPPFLAGS = -DOS_LINUX
	CXXFLAGS = -std=c++0x
endif

ifdef DEBUG
	CXXFLAGS += -g -pg -fPIC -O3 -pthread
else
	CXXFLAGS += -fPIC -O3 -pthread
endif

# DRS flags
CPPFLAGS += -DHAVE_USB -DHAVE_LIBUSB10 -DUSE_DRS_MUTEX

# ROOT libs and flags
CPPFLAGS += $(shell root-config --cflags)
LIBS = $(shell root-config --libs)

# wxWidgets libs and flags
CXXFLAGS += $(shell wx-config --cxxflags)
LIBS += $(shell wx-config --libs)

CPPFLAGS += -Iinclude -Iinclude/drs
LIBS += -lm -lzmq -ljson_spirit -lCAENDigitizer -lusb-1.0 -lutil -lpthread

all: $(OBJECTS) $(OBJ_VME) $(OBJ_DRS) $(TARGETS)  $(DATADEF) \
	$(LOGFILE) $(CONFDIR) lib/$(ARNAME) #lib/$(LIBNAME)

$(LOGFILE):
	@mkdir -p $(@D)
	@touch $@

$(CONFDIR):
	@mkdir -p $(@D)
	@cp -r config $(@D)/

include/%.hh: include/.default_%.hh
	cp $+ $@

build/%.o: src/%.cxx $(DATADEF)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

build/%.o: include/vme/%.c $(DATADEF)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

build/%.o: src/drs/%.cpp $(DATADEF)
	$(CXX)  $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

build/%.o: src/drs/%.c $(DATADEF)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

fe_%: modules/fe_%.cxx $(OBJECTS) $(OBJ_VME) $(OBJ_DRS) $(DATADEF)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ \
	$(OBJECTS) $(OBJ_VME) $(OBJ_DRS) $(LIBS) $(WXLIBS)

%_daq: modules/%_daq.cxx $(DATADEF)
	$(CXX) $< -o $@  $(CXXFLAGS) $(CPPFLAGS) $(LIBS)

lib/$(ARNAME): $(OBJECTS) $(OBJ_VME) $(OJB_DRS) $(DATADEF)
	$(AR) -rcs $@ $+

# lib/$(LIBNAME): $(OBJECTS) $(OBJ_VME) $(OJB_DRS) $(DATADEF)
# 	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -shared -fPIC $+ -o $@ $(LIBS)

install:
	mkdir -p $(PREFIX)/lib
#	cp lib/$(LIBNAME) $(PREFIX)/lib
	cp lib/$(ARNAME) $(PREFIX)/lib
	mkdir -p $(PREFIX)/include/labdaq
	cp -r $(HEADERS) $(PREFIX)/include/labdaq
#	ln -sf $(PREFIX)/lib/$(LIBNAME) $(PREFIX)/lib/$(SONAME)
#	ln -sf $(PREFIX)/lib/$(ARNAME) $(PREFIX)/lib/$(ARNAME)
	$(LDCONFIG)

uninstall:
	rm -f $(PREFIX)/lib/$(SONAME)*
	rm -rf $(patsubst include/%,$(PREFIX)/include/%,$(HEADERS))

clean:
	rm -f $(TARGETS) $(OBJECTS) $(OBJ_VME) $(OBJ_DRS)
