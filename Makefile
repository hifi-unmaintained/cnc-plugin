CC=i586-mingw32msvc-gcc
CFLAGS=-std=c99 -pedantic -Wall -O2 -s -Inpapi -Iext/include/ -DCURL_STATICLIB
WINDRES=i586-mingw32msvc-windres
LIBS=ext/lib/libcurl.a ext/lib/libz.a -lgdi32 -lws2_32
REV=$(shell sh -c 'git rev-parse --short @{0}')

all: dll

updater: src/updater.c
	gcc -std=c99 -pedantic -Wall -DOS_UNIX=1 -Inpapi -g -o updater src/updater.c -lcurl

dll:
	$(WINDRES) -J rc src/plugin.rc src/plugin.rc.o
	$(CC) $(CFLAGS) -mwindows -Wl,--enable-stdcall-fixup -s -shared -o npcncplugin.dll src/plugin.c src/updater.c src/launcher.c src/plugin.def src/plugin.rc.o $(LIBS)

clean:
	rm -f npcncplugin.dll src/plugin.rc.o
