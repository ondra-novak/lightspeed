
ACLOCAL_AMFLAGS = -I Build 

SUBDIRS = src .

noinst_LIBRARIES = liblightspeed.a


liblightspeed.a:FORCE
	$(MAKE) -C Build --file makefile all "GPP=g++ -O3 -ggdb -c -fno-strict-aliasing -Wall -fPIC"
	ln -f Build/liblightspeed.a ./
	
	
clean-local: clean-src
distclean-local: clean-src
.PHONY: clean-src

clean:
	$(MAKE) -k -C Build --file makefile clean
	rm -f ./liblightspeed.a
	
	
	
FORCE:

 