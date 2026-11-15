# Plugin project

This directory contains the initial JUCE/CMake VST3 project shell.

## Current behaviour

- Main input bus: vocal, mono or stereo.
- Auxiliary input bus: beat sidechain, mono or stereo when enabled by the host.
- Output bus: vocal only.
- Audio behaviour: pass-through; independent 2048-point FFT analysis runs on vocal and beat buses. No EQ or gain change is applied.
- Plugin format: VST3 only.

The second input bus is declared as an optional auxiliary bus. Its exact sidechain presentation and routing must be verified in Cubase during host validation. The editor currently displays separate live spectra; this is analysis only, not masking detection.

## Build prerequisites

- macOS with a supported Xcode command-line toolchain.
- CMake 3.22 or newer.
- Git and network access to fetch the pinned JUCE source.
- JUCE is fetched at configure time; its source is not copied into this repository.

## Build

From the repository root:

```sh
cmake -S plugin -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The VST3 bundle is generated under the JUCE artefacts directory inside `build`.

## Scope

This is a buildable-project starting point, not a release binary. Host validation, sidechain routing verification, masking detection, dynamic correction, state persistence, parameter automation, signing and notarization remain outstanding. The current spectral analyzer uses fixed-size preallocated buffers and publishes magnitudes through atomics for the editor.
