CC=i586-mingw32msvc-gcc
CFLAGS=-std=c99 -pedantic -Wall -O2 -s -Inpapi
WINDRES=i586-mingw32msvc-windres
LIBS=-lgdi32
REV=$(shell sh -c 'git rev-parse --short @{0}')

all: dll

dll:
	$(WINDRES) -J rc src/plugin.rc src/plugin.rc.o
	$(CC) $(CFLAGS) -mwindows -Wl,--enable-stdcall-fixup -s -shared -o npcncplugin.dll src/plugin.c src/plugin.def src/plugin.rc.o $(LIBS)

clean:
	rm -f npcncplugin.dll src/plugin.rc.o
