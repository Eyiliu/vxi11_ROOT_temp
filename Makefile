
.PHONY : all tek vxi clean

all : tek vxi

tek: vxi
	cd tek && make

vxi:
	cd vxi && make

clean:
	cd tek && make clean
	cd vxi && make clean
	rm lib/libtek* lib/libvxi*