# Windows J2534 DLL Makefile, cross-compiled with MinGW.
# The DLL is PC-side only: pt_core plus transports. The virtual device runs in
# host/vdevice/vdevice_server.
# Build: make, make x86 or make x64.

SHARED := ../shared
PTCORE := ../ptcore
SRC := j2534_dll.c transport_select.c transport_winusb.c \
    $(PTCORE)/pt_core.c $(SHARED)/pt_frame.c $(SHARED)/pt_wire.c $(SHARED)/transport_tcp.c
DEF := j2534.def
CFLAGS := -std=c11 -O2 -Wall -Wextra -I. -I$(SHARED) -I$(PTCORE)
LIBS := -lws2_32 -lwinusb -lsetupapi -static-libgcc -static

CC32 := i686-w64-mingw32-gcc
CC64 := x86_64-w64-mingw32-gcc

.PHONY: all x86 x64 clean
all: x86 x64
x86: j2534_x86.dll
x64: j2534_x64.dll

j2534_x86.dll: $(SRC) $(DEF)
	$(CC32) -shared $(CFLAGS) -o $@ $(SRC) $(DEF) $(LIBS) -Wl,--enable-stdcall-fixup

j2534_x64.dll: $(SRC) $(DEF)
	$(CC64) -shared $(CFLAGS) -o $@ $(SRC) $(DEF) $(LIBS)

clean:
	rm -f j2534_x86.dll j2534_x64.dll
