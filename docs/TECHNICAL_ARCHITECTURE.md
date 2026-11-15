# Technical architecture — initial proposal

## 1. Target platform

- First host format: VST3.
- First host for validation: Steinberg Cubase.
- Framework candidate: JUCE.
- Language candidate: modern C++.
- AU support may be considered after the VST3 signal path and host-sidechain behaviour are validated.

These are design choices under evaluation, not a claim that a build system or plugin binary already exists.

## 2. Plugin topology

**Main input:** lead vocal (mono or stereo).

**Auxiliary input:** instrumental beat, delivered through the host's sidechain bus.

**Output:** processed vocal only.

The sidechain should be declared through the plugin format/framework and explicitly enabled/routed in the host. The implementation must not assume that every DAW exposes sidechain buses in the same way.

## 3. Processing blocks

1. Input conditioning and channel-layout handling.
2. Analysis tap for vocal.
3. Analysis tap for beat sidechain.
4. Windowing and spectral transform.
5. Temporal smoothing and frequency-band feature extraction.
6. Masking/overlap estimator.
7. Correction target and depth limiter.
8. Smooth dynamic filter/EQ stage.
9. Output and bypass management.
10. Metering/visualization data handoff to the UI.

The analysis path and audio path should be decoupled so that UI rendering cannot block audio processing.

## 4. DSP design principles

- Use bounded processing and preallocated buffers.
- Avoid allocations, mutex waits, logging, disk access and network calls in the audio callback.
- Smooth control changes to avoid zipper noise.
- Bound maximum attenuation and rate of change.
- Keep detector behaviour deterministic and testable.
- Treat spectral overlap as a useful feature, not proof of perceptual masking.
- Validate any phase, latency and transient trade-offs with measurement and listening.

## 5. Analysis candidates

A short-time Fourier transform (STFT) is a reasonable prototype basis. Window size, hop size, window function, smoothing constants and frequency resolution must be evaluated against latency and transient response.

Potential features:
- vocal magnitude spectrum;
- sidechain magnitude spectrum;
- band-wise relative energy;
- temporal persistence of overlap;
- confidence/stability estimate.

No final detector formula is selected yet. It should be developed and tested against representative vocal/beat material.

## 6. Dynamic correction candidates

A first prototype can use a bank of smoothed peaking filters or another bounded dynamic-EQ structure. Filter count, centre-frequency mapping, bandwidth, maximum attenuation and attack/release behaviour remain experimental parameters.

The correction should be applied to the vocal only. A conservative maximum correction and a global depth control are required.

## 7. Real-time and host requirements

- Support mono and stereo main input layouts.
- Support an optional mono/stereo sidechain layout where practical.
- Handle sidechain silence and disconnection.
- Handle changing sample rates and maximum block sizes.
- Report plugin latency to the host if the selected analysis/processing design introduces it.
- Implement robust bypass and state save/restore.
- Test automation and parameter changes under host control.

## 8. UI communication

The UI should show:
- vocal spectrum;
- beat reference spectrum;
- regions where the detector recommends or applies correction;
- correction amount;
- sidechain connection/activity state;
- bypass and A/B controls.

UI updates must use a thread-safe, non-blocking data exchange pattern. The audio thread must never wait for the UI.

## 9. Validation plan

- Unit tests for detector and filter behaviour.
- Offline test signals: tones, noise, sweeps, impulses and controlled overlaps.
- Real-world test sessions with isolated vocal and beat stems.
- Null/latency and gain-staging checks where applicable.
- Listening tests at matched loudness.
- Host tests in Cubase across common buffer sizes and sample rates.

## 10. Build and delivery

A reproducible build pipeline is required before a distributable plugin can be promised. The project may use a remote build runner to reduce local development setup, but the resulting plugin still needs installation and real-world testing on the target Mac and DAW.
