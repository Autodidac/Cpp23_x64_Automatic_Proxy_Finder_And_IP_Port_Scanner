# InternetScanner23

A small, single-threaded HTTP/1.1 probe (plain TCP, **no TLS**) intended for controlled testing on targets you own or have explicit permission to test.

## What's new (v0.0.11)
- Target scan controls now live inside the Target Setup area for a tighter workflow.
- Target, proxy, and port sections share a unified layout system that removes padding gaps and keeps controls aligned when resizing.
- Version bumped to keep the window title and documentation in sync.

## Build (Visual Studio)

Option 1: Open the folder in Visual Studio (File → Open → Folder). Configure x64 Debug/Release and build.

Option 2: Generate a Visual Studio solution via CMake and open it:

```powershell
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc --config Release
```

Then open `build-msvc/InternetScanner23.sln` in Visual Studio if you prefer the full solution view.

## Build (CMake CLI)

```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Run

This application now ships as a GUI-only tool. After building, launch the generated executable (e.g., `build/InternetScanner.exe` on Windows) directly. Command-line arguments are ignored.

## Targets file format

One entry per line:
- `127.0.0.1`
- `example.com`
- `example.com:8080`
- `http://example.com/path?x=y`

Blank lines and lines starting with `#` are ignored.

## Versioning

The current application version is tracked in the root `VERSION` file and exposed to the codebase via the `version.ixx` module (`app_version::version()` and `app_version::title()`). Update both when cutting a new release so the window title and documentation stay in sync.
