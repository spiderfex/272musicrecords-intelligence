# 272MusicRecords INTELLIGENCE

An original vocal/beat interaction analysis and processing plugin.

The product is intended to identify frequency-domain competition between a lead vocal and its instrumental beat, then help the producer make controlled, musical corrections.

## Direction
- Original DSP design and product identity; not a copy of another commercial plugin.
- First target: VST3 in Cubase.
- Main signal: vocal; reference signal: beat via host sidechain.
- Output: processed vocal only.
- First milestone: reliable analysis and transparent, controllable processing—not claims of autonomous AI learning.

## Planned capabilities
- Real-time spectral analysis of vocal and beat sidechain.
- Detection of persistent overlap and potential masking regions.
- Adjustable frequency-selective dynamic correction.
- Manual and assisted modes, correction-depth control, bypass and A/B.
- Visual explanation of proposed and active changes.

## Project status
**Phase: product and technical specification.** No compiled plugin or working DSP implementation is claimed yet.

## Documents
- [Product specification](docs/PRODUCT_SPEC.md)
- [Technical architecture](docs/TECHNICAL_ARCHITECTURE.md)
- [Roadmap](docs/ROADMAP.md)
- [Decision log](docs/DECISIONS.md)

## Technology
JUCE and modern C++ are leading candidates. A reproducible build pipeline must be established before a plugin binary can be promised. Remote builds can reduce local setup, but the plugin still needs installation and testing on the target Mac and DAW.

## Public repository notice
This repository is public. Do not commit credentials, confidential business information, customer audio or unreleased proprietary details intended to remain confidential. Review third-party dependency licences before distribution.

## Licensing
No open-source licence has been selected. Do not assume this repository grants permission to reuse, redistribute or commercialize its contents.
