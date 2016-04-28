
.PHONY : all agilent tek vxi clean

all : agilent tek vxi

tek: vxi
	cd tek && make

agilent: vxi
	cd agilent && make

vxi:
	cd vxi && make

clean:
	cd tek && make clean
	cd vxi && make clean
	cd agilent && make clean
	rm lib/libtek* lib/libvxi*
