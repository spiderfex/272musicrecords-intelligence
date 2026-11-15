# Product specification — 272MusicRecords INTELLIGENCE

## 1. Product purpose

Help music producers make a lead vocal sit clearly and naturally inside an already-produced instrumental beat by detecting and managing frequency-domain masking.

The intended first audience is rap and melodic-vocal producers, with modern German rap as an important reference use case.

## 2. User problem

A vocal and a dense, often already-mastered beat can compete in similar frequency regions. Static EQ may solve one moment but make another moment sound thin or unnatural. The user needs a processor that compares the vocal with the beat and makes restrained, explainable corrections.

## 3. Intended signal flow

1. Insert the plugin on the vocal track.
2. Route the instrumental beat to the plugin's auxiliary/sidechain input using the DAW's supported routing.
3. Analyze the vocal as the main input and the beat as the reference.
4. Estimate relevant overlap/masking regions.
5. Apply controlled dynamic correction to the vocal.
6. Monitor the result with bypass and A/B comparison.

The beat is a reference signal; it is not mixed into the plugin output.

## 4. MVP requirements

### Analysis
- Analyze main vocal input and beat sidechain independently.
- Provide stable, smoothed spectral information.
- Distinguish signal energy overlap from a definitive perceptual masking judgement; the latter requires listening validation.
- Handle missing or inactive sidechain gracefully.

### Processing
- Apply bounded, frequency-selective dynamic attenuation to the vocal.
- Provide a global correction amount/depth control.
- Avoid abrupt parameter changes, clicks and unstable gain modulation.
- Include a reliable bypass path.

### User control
- Provide an assisted mode and a manual/limited mode.
- Make active correction regions visible.
- Support A/B comparison.
- Provide sensible defaults and prevent extreme settings from being the default.

### Reliability
- No memory allocation, blocking locks, file I/O or unpredictable operations on the real-time audio thread.
- Handle mono and stereo vocal inputs.
- Handle sample-rate and block-size changes safely.
- Report and document latency accurately.

## 5. Out of scope for the first prototype

- Claims that the plugin learns a user's taste or trains itself.
- Cloud processing or account-based activation.
- Mastering an entire mix automatically.
- Replacing the producer's judgement.
- Guaranteed compatibility with every DAW before host testing.

## 6. Initial user workflow

- Insert on lead vocal.
- Enable the host sidechain and route the beat to it.
- Start with conservative correction.
- Compare bypass and processed signal at matched perceived loudness.
- Adjust depth and target behaviour by ear.
- Check the vocal in the full mix and on alternate monitoring systems.

## 7. Success criteria

The prototype is useful only if controlled listening tests show that it can:
- reduce clearly audible vocal/beat competition in relevant passages;
- preserve vocal body, intelligibility and natural dynamics;
- avoid pumping, lisping, phase artefacts or obvious tonal holes;
- remain stable at common audio buffer sizes;
- behave predictably when the sidechain is absent or changes.

## 8. Product positioning

An original 272MusicRecords tool for vocal/beat interaction analysis and controlled dynamic tonal correction. The product must establish its own DSP behaviour, interface, naming and visual identity.
