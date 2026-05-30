# MPVPlayer

Minimal mpv-based player focused on **decoded media ↔ display synchronization** from day one.

## Goals
- Keep the codebase small and easy to reason about.
- Start with only:
  - `mpvplayer.h`
  - `mpvplayer.cpp`
  - a very small transport control UI
- Make display sync a mandatory design requirement.

## Sync-first defaults
The initial player config enables mpv settings aimed at stable presentation timing and display-synced playback, including:
- `video-sync=display-resample`
- `interpolation=yes`
- `keep-open=yes`
- `hr-seek=yes`
- `audio-wait-open=no`

These can be tuned later per device and content type.

## Files
- `mpvplayer.h` — small wrapper interface around libmpv
- `mpvplayer.cpp` — implementation and default sync-oriented options
- `main.cpp` — tiny CLI transport demo
- `CMakeLists.txt` — simple build entrypoint

## Build
Example:

```bash
cmake -S . -B build
cmake --build build
```

You will need libmpv development headers/libraries installed on your system.
