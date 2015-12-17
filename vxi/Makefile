

# DEBUG        := -g
CXX          := g++
CC           := g++
LD           := g++
CXXFLAGS     := $(DEBUG) -Wall -fPIC -Wl,--version-script=linker.version
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


.PHONY : clean dist

UNAME:=$(shell uname -s)

ifneq ($(UNAME),SunOS)
	LDFLAGS:=$(LDFLAGS) -Wl,--version-script=linker.version
endif

libvxi11.so: vxi11_user.o vxi11_clnt.o vxi11_xdr.o libvxi11.o
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libvxi11.so $^ $(GLIBS) -o $@
	cp libvxi11.so ../lib/libvxi11.so
	cp libvxi11_rdict.pcm ../lib/libvxi11_rdict.pcm

libvxi11.cc:  vxi11_user.h linkdef.h
	$(ROOTCINT)  -f $@ -c $^

libvxi11.o: libvxi11.cc
	$(CXX)  $(CXXFLAGS) -c $< -o $@

vxi11_user.o: vxi11_user.c vxi11.h
	$(CC) $(CFLAGS) -c $< -o $@

vxi11_clnt.o : vxi11_clnt.c
	$(CC) $(CFLAGS) -c $< -o $@

vxi11_xdr.o : vxi11_xdr.c
	$(CC) $(CFLAGS) -c $< -o $@

vxi11.h vxi11_clnt.c vxi11_xdr.c : vxi11.x
	rpcgen -M vxi11.x

clean:
	rm -f *.o vxi11.h vxi11_svc.c vxi11_xdr.c vxi11_clnt.c libvxi11* 
