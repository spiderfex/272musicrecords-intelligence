# Development roadmap

Status labels: **Planned**, **In progress**, **Validated**, **Blocked**.

## Phase 0 — Product and architecture
**Status: In progress**

- [x] Define product purpose and intended workflow.
- [x] Choose vocal main input plus beat sidechain topology.
- [x] Establish real-time DSP constraints.
- [ ] Confirm exact host and plugin-format requirements.
- [x] Select JUCE 9.0.5 as the initial framework version and add a CMake VST3 project skeleton.
- [ ] Verify a successful macOS CI build and Cubase sidechain routing.
- [ ] Confirm the appropriate JUCE licence tier before any closed-source commercial distribution.

**Exit criteria:** reviewed technical specification and a reproducible empty plugin project.

## Phase 1 — V0.1: audio analysis
**Status: In progress**

- [x] Create minimal VST3 plugin shell (pass-through foundation; build validation pending).
- [ ] Receive vocal main input and beat sidechain.
- [ ] Implement independent spectral analysis.
- [ ] Display stable analysis data.
- [ ] Handle missing sidechain and mono/stereo layouts.

**Exit criteria:** analysis is stable, measurable and does not disrupt audio.

## Phase 2 — V0.2: overlap and masking estimation
**Status: Planned**

- [ ] Define and document overlap features.
- [ ] Add temporal persistence and smoothing.
- [ ] Create test fixtures with known spectral conflicts.
- [ ] Validate detector output against listening examples.

**Exit criteria:** detector identifies useful candidate regions without presenting every overlap as audible masking.

## Phase 3 — V0.3: dynamic correction
**Status: Planned**

- [ ] Implement bounded dynamic EQ on vocal.
- [ ] Add correction depth and smoothing.
- [ ] Add safe bypass and matched-level comparison workflow.
- [ ] Test artefacts, latency and transient response.

**Exit criteria:** audible improvement in selected cases without unacceptable tonal damage.

## Phase 4 — V0.4: controls and assisted operation
**Status: Planned**

- [ ] Add manual and assisted modes.
- [ ] Add user-adjustable correction limits.
- [ ] Add parameter automation and state persistence.
- [ ] Test edge cases and host automation.

**Exit criteria:** user remains in control and can understand the processing.

## Phase 5 — V0.5: production UI
**Status: Planned**

- [ ] Create 272MusicRecords visual identity.
- [ ] Add spectrum and correction visualization.
- [ ] Add clear input/sidechain status.
- [ ] Accessibility, scaling and usability pass.

**Exit criteria:** interface is readable, responsive and communicates the DSP accurately.

## Phase 6 — V1.0: release readiness
**Status: Planned**

- [ ] Compatibility matrix and regression testing.
- [ ] Performance and stability testing.
- [ ] Dependency and licensing audit.
- [ ] Installer, signing and macOS notarization plan.
- [ ] User documentation and support process.
- [ ] Commercial, trademark and distribution review.

**Exit criteria:** repeatable release build with documented compatibility and known limitations.
