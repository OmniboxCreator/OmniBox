# j2534mode Makefile.
# Native Linux build uses TCP for the virtual device.
# Windows build uses TCP plus WinUSB for real hardware.

SHARED := ../shared
DLL  := ../dll
CFLAGS := -std=c11 -O2 -Wall -Wextra -I$(SHARED)

# Native: TCP only. WinUSB is Windows-only.
NATIVE_SRC := j2534mode.c $(SHARED)/pt_frame.c $(SHARED)/transport_tcp.c

# Windows: add the WinUSB transport used by the DLL.
WIN_SRC := j2534mode.c $(SHARED)/pt_frame.c $(SHARED)/transport_tcp.c \
      $(DLL)/transport_winusb.c
WIN_CFLAGS := $(CFLAGS) -I$(DLL)
WIN_LIBS := -lws2_32 -lwinusb -lsetupapi -static

CC32 := i686-w64-mingw32-gcc
CC64 := x86_64-w64-mingw32-gcc

GUI_SRC := j2534mode_gui.c $(SHARED)/pt_frame.c $(SHARED)/transport_tcp.c $(DLL)/transport_winusb.c

.PHONY: all windows win32 win64 gui clean
all: j2534mode
windows: win64 win32 gui
win64: j2534mode.exe
win32: j2534mode_x86.exe
gui: j2534mode_gui.exe

j2534mode_gui.exe: $(GUI_SRC)
	$(CC64) $(WIN_CFLAGS) -mwindows -o $@ $(GUI_SRC) $(WIN_LIBS)

j2534mode: $(NATIVE_SRC)
	cc $(CFLAGS) -o $@ $(NATIVE_SRC)

j2534mode.exe: $(WIN_SRC)
	$(CC64) $(WIN_CFLAGS) -o $@ $(WIN_SRC) $(WIN_LIBS)

j2534mode_x86.exe: $(WIN_SRC)
	$(CC32) $(WIN_CFLAGS) -o $@ $(WIN_SRC) $(WIN_LIBS)

clean:
	rm -f j2534mode j2534mode.exe j2534mode_x86.exe j2534mode_gui.exe
