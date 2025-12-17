# InternetScanner23 (fixed)

A small, single-threaded HTTP/1.1 probe (plain TCP, **no TLS**) intended for controlled testing on targets you own or have explicit permission to test.

## Build (Visual Studio)

Open the folder in Visual Studio (File → Open → Folder). Configure x64 Debug/Release and build.

## Build (CMake CLI)

```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Run

```powershell
./build/InternetScanner.exe --targets targets.txt
./build/InternetScanner.exe --targets targets.txt --port 80 --path /health
```

## Targets file format

One entry per line:
- `127.0.0.1`
- `example.com`
- `example.com:8080`
- `http://example.com/path?x=y`

Blank lines and lines starting with `#` are ignored.
