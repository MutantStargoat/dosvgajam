src = $(wildcard src/*.c) $(wildcard src/g3d/*.c) $(wildcard src/crossgl/*.c)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = game

def = -DNO_ASM
incdir = -Isrc -Ilibs
libdir = -Llibs/imago -Llibs/assfile

CFLAGS = -std=gnu89 -pedantic -Wall -g $(def) $(incdir) -MMD
LDFLAGS = -static-libgcc $(libdir) -limago -lassfile -lGL -lX11 -lXext -lm

$(bin): $(obj) libs
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

.PHONY: libs
libs: imago assfile

.PHONY: imago
imago:
	$(MAKE) -C libs/imago

.PHONY: assfile
assfile:
	$(MAKE) -C libs/assfile

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: cleanlibs
cleanlibs:
	$(MAKE) -C libs/imago clean
	$(MAKE) -C libs/assfile clean
