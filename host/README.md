# OmniBox host software

This directory contains the PC-side J2534 stack:

- `shared/`: framing, wire serialization and transport abstractions.
- `ptcore/`: portable PassThru logic.
- `dll/`: Windows J2534 DLL wrapper, WinUSB transport and registry template.
- `tools/`: USB identity mode selector.
- `vdevice/`: virtual device for host-side testing without hardware.
- `test/`: native tests for framing and J2534 end-to-end flows.

## Virtual device

```bash
cd host
make -C vdevice
./vdevice/vdevice_server 9000
```

Then point the DLL or tools to it:

```bash
J2534_TCP=127.0.0.1:9000 tools/j2534mode get
```

## Windows registration

Edit `dll/j2534_register.reg` so `FunctionLibrary` points to your built DLLs,
then import it as administrator. The template registers both 32-bit and 64-bit
J2534 entries under `PassThruSupport.04.04`.
