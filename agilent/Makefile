

# DEBUG        := -g
CXX          := g++
CC           := g++
LD           := g++
CXXFLAGS     := $(DEBUG) -Wall -fPIC
LDFLAGS      := $(DEBUG)
SOFLAGS      := -shared



# ROOTCINT     := rootcint
ROOTCINT     := rootcling
ROOTCONFIG   := root-config
ROOTCFLAGS   := $(shell $(ROOTCONFIG) --cflags)
ROOTLDFLAGS  := $(shell $(ROOTCONFIG) --ldflags)
ROOTLIBS     := $(shell $(ROOTCONFIG) --libs)
ROOTGLIBS    := $(shell $(ROOTCONFIG) --glibs)
HASTHREAD    := $(shell $(ROOTCONFIG) --has-thread)

LIBS         := $(ROOTLIBS) $(SYSLIBS)
GLIBS        := $(ROOTGLIBS) $(SYSLIBS)

CXXFLAGS     += $(ROOTCFLAGS)
LDFLAGS      += $(ROOTLDFLAGS)
CFLAGS       := $(CXXFLAGS)



libname=libagilentscopevxi11
full_libname=$(libname).so

.PHONY : all clean

all : $(full_libname)

$(full_libname) : agilent_vxi11.o libagilentscopevxi11.o
	$(CXX) ${LDFLAGS} -shared -Wl,-soname,$(full_libname) $^ -o $@ -L../lib -lvxi11
	cp $(full_libname) ../lib/$(full_libname)
	cp $(libname)_rdict.pcm ../lib/$(libname)_rdict.pcm

libagilentscopevxi11.cc:  agilent_vxi11.h linkdef.h
	$(ROOTCINT)  -f $@ -c $^

libagilentscopevxi11.o: $(libname).cc
	$(CXX)  $(CXXFLAGS) -c $< -o $@

agilent_vxi11.o: agilent_vxi11.cc agilent_vxi11.h
	$(CXX) -fPIC $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(full_libname) $(libname)*

