# Decision log

## D-001 — Product identity
**Decision:** Use the working product name **272MusicRecords INTELLIGENCE**.

**Reason:** Establish an independent product identity. Name availability and trademark clearance have not yet been checked.

## D-002 — Initial signal topology
**Decision:** Process the vocal on the main input and use the instrumental beat as an auxiliary sidechain/reference input. Output the processed vocal only.

**Reason:** This matches the intended vocal-track workflow and gives the detector a separate beat reference without mixing the beat into the output.

## D-003 — First host target
**Decision:** Prioritize VST3 validation in Cubase.

**Reason:** The user's current intended DAW workflow is Cubase. AU and other hosts can be evaluated after the first working prototype.

## D-004 — DSP before machine learning claims
**Decision:** Start with deterministic, measurable DSP and analysis. Do not market the first version as a self-learning AI.

**Reason:** Reliable detection, controllable processing and listening validation are prerequisites for any later intelligent/adaptive features.

## D-005 — Public repository handling
**Decision:** Continue development in the repository while it is public, as requested by the owner.

**Risk:** Public contents can be viewed and copied by others. Do not commit credentials, confidential business information, customer audio or unreleased proprietary implementation details that the owner intends to keep confidential. Repository visibility can be changed later by its owner.

## D-006 — No licence selected
**Decision:** Do not add an open-source licence at this stage.

**Reason:** The intended product is commercial and reuse/distribution terms require an explicit decision.
