# Claude Code Project Rules

## Platform contract

- Target Windows 11 x64 only.
- Native C++17+ application.
- DirectX 11 for rendering.
- WASAPI is the master music clock.
- Accept physical USB-MIDI input only.
- The final application runtime must not require or initiate network access.
- No browser runtime, Electron, Tauri, Web MIDI, or Web Audio.
- Desktop mouse + physical keyboard UX only. Do not design touch interactions.

## Timeline invariants

- The Chord Event array is the harmonic timeline SSOT.
- `measuresMap` is the metric SSOT.
- Never derive musical position using a fixed measure length.
- Undefined timeline gaps are forbidden.
- Silence is represented by an explicit `N.C.` Chord Event.
- Chord Boundary is a UI/editing concept derived from adjacent events, not an independently persisted SSOT object.
- Timeline mutations must preserve `left.end == right.start`, then normalize and validate before playback consumes the result.

## Verification contract

- Parser confidence and human verification are separate concepts.
- Provenance: `SourceExact`, `Inferred`, `UserEdited`.
- Verification: `Unverified`, `Verified`, `Error`.
- Parser output alone must never become `Verified`.

## Workflow

Before architecture changes, read `docs/SYSTEM_REQUIREMENTS.md`.
Before UX/UI changes, read `docs/UI_REQUIREMENTS.md`.
Before MIDI implementation, read `docs/USB_MIDI_BRINGUP_TEST.md`.

For multi-file work: inspect first, plan second, edit third, verify last.
Do not claim completion without build/test evidence.
Do not create speculative frameworks, networking, account systems, cloud integrations, telemetry, or touch UI.
Do not commit or push unless explicitly instructed by the user.
