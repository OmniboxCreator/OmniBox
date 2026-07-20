# vdevice_server Makefile (native firmware core plus simulated ECU over TCP).
# Natif Linux + cross-compile Windows (MinGW). Self-contained (-static).
#
# Build : make      (binaire natif Linux : vdevice_server)
#     make win64    (vdevice_server.exe  — Windows 64-bit)
#     make win32    (vdevice_server_x86.exe — Windows 32-bit)
#     make windows   (les deux .exe)

SHARED := ../shared
FW   := ../../firmware/src
SRC := vdevice_server.c vdevice.c mock_bus.c vlog.c usb_mode_stub.c $(SHARED)/transport_tcp.c $(SHARED)/pt_frame.c $(SHARED)/pt_wire.c \
    $(FW)/j2534/j2534_core.c $(FW)/j2534/isotp.c $(FW)/j2534/j2534_filter.c \
    $(FW)/j2534/j2534_wire.c $(FW)/j2534/kline_asm.c $(FW)/j2534/trace.c \
    $(FW)/transport/protocol.c
CFLAGS := -std=c11 -O2 -Wall -I. -I$(SHARED)

CC32 := i686-w64-mingw32-gcc
CC64 := x86_64-w64-mingw32-gcc

.PHONY: all windows win32 win64 clean
all: vdevice_server
windows: win64 win32
win64: vdevice_server.exe
win32: vdevice_server_x86.exe

vdevice_server: $(SRC)
	cc $(CFLAGS) -o $@ $(SRC)

vdevice_server.exe: $(SRC)
	$(CC64) $(CFLAGS) -o $@ $(SRC) -lws2_32 -static

vdevice_server_x86.exe: $(SRC)
	$(CC32) $(CFLAGS) -o $@ $(SRC) -lws2_32 -static

clean:
	rm -f vdevice_server vdevice_server.exe vdevice_server_x86.exe
