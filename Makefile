!ifdef __UNIX__
dosobj = src/dos/main.obj src/dos/vga.obj src/dos/keyb.obj src/dos/mouse.obj &
	src/dos/timer.obj src/dos/logger.obj
appobj = src/app.obj src/options.obj src/treestor.obj src/ts_text.obj &
	src/dynarr.obj src/util.obj src/scr_menu.obj src/scr_game.obj src/lut.obj &
	src/xmath.obj src/xmath_s.obj src/image.obj src/tiles.obj src/level.obj &
	src/rend.obj
g3dobj = src/g3d/g3d.obj src/g3d/polyfill.obj

incpath = -Isrc -Isrc/dos -Ilibs -Ilibs/imago/src
libpath = libpath libs/dos
!else
dosobj = src\dos\main.obj src\dos\vga.obj src\dos\keyb.obj src\dos\mouse.obj &
	src\dos\timer.obj src\dos\logger.obj
appobj = src\app.obj src\options.obj src\treestor.obj src\ts_text.obj &
	src\dynarr.obj src\util.obj src\scr_menu.obj src\scr_game.obj src\lut.obj &
	src\xmath.obj src\xmath_s.obj src\image.obj src\tiles.obj src\level.obj &
	src\rend.obj
g3dobj = src\g3d\g3d.obj src\g3d\polyfill.obj

incpath = -Isrc -Isrc\dos -Ilibs -Ilibs\imago\src
libpath = libpath libs\dos
!endif

obj = $(dosobj) $(appobj) $(g3dobj)
bin = game.exe

!include watcfg.mk

def = $(cfg_def)
libs = imago.lib assfile.lib

AS = nasm
CC = wcc386
LD = wlink
ASFLAGS = -fobj
CFLAGS = $(cfg_dbg) $(cfg_opt) $(cfg_prof) $(def) -s -zq -bt=dos $(incpath)
LDFLAGS = option map $(libpath) library { $(libs) }

$(bin): cflags.occ $(obj) $(libs)
	%write objects.lnk $(obj)
	%write ldflags.lnk $(LDFLAGS)
	$(LD) debug all name $@ system dos4g file { @objects } @ldflags

.c: src;src/g3d;src/dos
.asm: src;src/g3d;src/dos

cflags.occ: Makefile watcfg.mk
	%write $@ $(CFLAGS)

.c.obj: .autodepend
	$(CC) -fo=$@ @cflags.occ $[*

.asm.obj:
	nasm $(ASFLAGS) -o $@ $[*.asm


!ifdef __UNIX__
clean: .symbolic
	rm -f $(obj)
	rm -f $(bin)
	rm -f cflags.occ *.lnk

imago.lib:
	cd libs/imago
	wmake -f Makefile
	cd ../..

assfile.lib:
	cd libs/assfile
	wmake -f Makefile
	cd ../..

cleanlibs: .symbolic
	cd libs/imago
	wmake clean
	cd ../assfile
	wmake clean
	cd ../..

!else
clean: .symbolic
	del src\*.obj
	del src\dos\*.obj
	del *.lnk
	del cflags.occ
	del $(bin)

imago.lib:
	cd libs\imago
	wmake -f Makefile
	cd ..\..

assfile.lib:
	cd libs\assfile
	wmake -f Makefile
	cd ..\..


cleanlibs: .symbolic
	cd libs\imago
	wmake clean
	cd ..\assfile
	wmake clean
	cd ..\..

!endif
