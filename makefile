.SILENT

#watcom compile flags:
#-bm = multithread
#-zq = quiet
#-w4 = highest warning level
#-ox = various optimizations
#-dxxx = #defines
#-d2 = include debug

CC = wcc386
LINK = wlink

#debug version
#CFLAGS=-bm -d2 -zq -w4 -d__IBMC__=1 -d__IBMCPP__
#LINKOPS = debug all op quiet op symf sys os2v2_pm op stack=32k

#production
CFLAGS=-bm -zq -w4 -ox -d__IBMC__=1 -d__IBMCPP__=0
LINKOPS = sys os2v2_pm op quiet op stack=32k

all : flist.exe flist.res .SYMBOLIC
    @echo Adding resources...
    rc flist.res flist.exe > NUL
    @echo Done!

flist.exe : flist.obj menu.obj drag.obj util.obj flwin.obj list.obj
    @echo Linking...
    $(LINK) $(LINKOPS) name flist file flist,menu,drag,util,flwin,list

flist.obj : flist.c menu.h drag.h flwin.h list.h res.h util.h
   @echo Compiling flist.c...
   wcc386 $(CFLAGS) flist

menu.obj : menu.c menu.h flwin.h list.h res.h util.h
   @echo Compiling menu.c...
   wcc386 $(CFLAGS) menu

drag.obj : drag.c drag.h flwin.h list.h util.h
   @echo Compiling drag.c...
   wcc386 $(CFLAGS) drag

util.obj : util.c flwin.h list.h util.h
   @echo Compiling util.c...
   wcc386 $(CFLAGS) util

flwin.obj : flwin.c flwin.h list.h
   @echo Compiling flwin.c...
   wcc386 $(CFLAGS) flwin

list.obj : list.c list.h
   @echo Compiling list.c...
   wcc386 $(CFLAGS) list

flist.res : flist.rc flist.ico res.h flwin.h
   @echo Compiling resources...
   rc -r flist.rc flist.res > NUL
