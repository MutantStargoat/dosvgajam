obj =  mlkernel.obj mltimdos.obj mlapi.obj mlmemory.obj mldummy.obj  mlopl.obj &
	mlopl_io.obj mlmidi.obj mlmpu401.obj mlsbmidi.obj mlsbmix.obj mlmisc.obj

!ifdef __UNIX__
alib = ../../muslib.lib
!else
alib = ..\..\muslib.lib
!endif

def = -D__NO_DEFAULT_LIBS__

CC = wcc386
CFLAGS = $(cfg_dbg) $(cfg_opt) $(def) -zq -bt=dos

$(alib): $(obj)
	%write objects.lbc $(obj)
	wlib -q -b -n -c $@ @objects

.c.obj: .autodepend
	$(CC) -fo=$@ $(CFLAGS) $[*

!ifdef __UNIX__
clean: .symbolic
	rm -f $(obj)
	rm -f $(alib)
	rm -f objects.lbc
!else
clean: .symbolic
	del *.obj
	del objects.lbc
	del $(alib)
!endif
