# System Requirements

## 1. Purpose

MidiVisualizer + BassEngine Studio is a single-user Windows 11 desktop practice application for physical USB-MIDI keyboards. It visualizes MIDI performance, plays local bass accompaniment from chord timelines, supports Chord Boundary correction, local looping, rehearsal marks, and Rootless Voicing practice.

## 2. Product boundaries

### Supported

- Windows 11 Home / Pro x64
- Native C++17 or later
- DirectX 11 rendering
- WASAPI audio
- Windows-native MIDI input/output APIs
- Physical USB-MIDI devices only
- Local files only
- Single local user

### Explicitly out of scope

- Windows 10 and earlier
- macOS / Linux / Wine / Proton
- Browser runtime
- Electron / Tauri
- Web MIDI / Web Audio
- Bluetooth MIDI
- RTP-MIDI / Network MIDI
- Cloud storage or sync
- Accounts, teams, collaboration, sales, subscriptions, licensing servers
- Telemetry, advertising, remote crash reporting, automatic update checks
- Any application runtime network dependency
- Touch, pen, swipe, pinch, or multi-touch UX

The final application must remain fully usable with Wi-Fi disabled and no network cable connected.

## 3. Runtime architecture

- Rendering: DirectX 11
- Audio output and master music clock: WASAPI
- MIDI: Windows-native MIDI API, physical USB device path only
- UI: Windows desktop mouse + physical keyboard

DirectX frame timing must never be the music clock. FPS changes must not alter BPM, bass timing, metronome timing, timeline position, or MARK position.

## 4. MIDI note convention

Human-facing octave naming is fixed to:

- MIDI 48 = C3
- MIDI 60 = C4
- MIDI 72 = C5

Libraries using another octave label convention must be translated at the boundary.

## 5. Metric resolution

V1 uses quarter-note denominator meters only.

- Sixteenth note = 1 tick
- Eighth note = 2 ticks
- Quarter note = 4 ticks
- Half note = 8 ticks
- Whole note = 16 ticks

Supported examples:

- 2/4 = 8 ticks
- 3/4 = 12 ticks
- 4/4 = 16 ticks
- 5/4 = 20 ticks

6/8, 9/8, 12/8 and other compound meters are V1 out of scope.

## 6. `measuresMap` metric SSOT

`measuresMap` is the single source of truth for measure geometry and cumulative musical time.

Each measure must expose at least:

```cpp
struct MeasureMetric {
    MeasureId id;
    int numerator;
    int denominator;
    Tick lengthTicks;
    AbsoluteTick startAbsoluteTick;
    AbsoluteTick endAbsoluteTick;
};
```

All playback, seek, previous/next measure, local loop, MARK resolution, Inspector location, and timeline rendering must query metric data.

Forbidden in those systems:

```cpp
measureIndex * FIXED_TICKS_PER_MEASURE
playbackTick / FIXED_TICKS_PER_MEASURE
playbackTick % FIXED_TICKS_PER_MEASURE
```

Required logical API surface includes equivalents of:

```cpp
findMeasureAtAbsoluteTick()
getMeasureStart()
getMeasureEnd()
getPreviousMeasure()
getNextMeasure()
```

Example:

```text
M1 4/4 =  0..15
M2 4/4 = 16..31
M3 2/4 = 32..39
M4 4/4 = 40..55
```

M4 begins at absolute tick 40, never at a fixed-multiply result.

## 7. Chord Event harmonic SSOT

The ordered Chord Event array is the harmonic timeline SSOT.

Conceptual model:

```cpp
struct ChordEvent {
    EventId id;
    ChordMusicalData musical;
    TimingData timing;
    Provenance provenance;
    Verification verification;
};
```

Chord Boundary is derived from adjacent Chord Events and is not an independently persisted SSOT object.

## 8. Musical data

At minimum, a chord event must retain:

- source/display chord symbol
- harmonic root pitch class, 0..11
- chord quality
- optional slash-bass pitch class
- explicit N.C. state

Minimum quality model:

- Major
- Minor
- Dominant
- Diminished
- Augmented

The source symbol may preserve extensions and alterations beyond those minimum semantic categories.

For slash chords, harmonic root and requested bass are distinct. Example `Am7/D`: harmonic root A, bass D.

## 9. Timing data

A Chord Event exposes at least:

```cpp
struct TimingData {
    MeasureId measure;
    Tick startTick;
    Tick durationTicks;
    AbsoluteTick startAbsoluteTick;
    AbsoluteTick endAbsoluteTick;
};
```

Derived timing fields must not be independently hand-edited by UI code.

## 10. Gap-free timeline invariant

From song start to song end:

```cpp
events[i].timing.endAbsoluteTick ==
events[i + 1].timing.startAbsoluteTick;
```

Required invariants:

- undefined gap = 0
- overlap = 0
- durationTicks > 0 for every event

## 11. N.C. definition

N.C. is a first-class Chord Event representing an intentionally silent harmonic interval.

It occupies time and has normal boundaries, timing, provenance, and verification state.

N.C. produces no harmonic root and no bass accompaniment.

Valid:

```text
Gm9 -> C7 -> N.C. -> Am7
```

Invalid:

```text
Gm9 -> C7 -> undefined gap -> Am7
```

## 12. Chord Boundary editing

A boundary is the shared transition between adjacent events:

```cpp
left.endAbsoluteTick == right.startAbsoluteTick
```

The editor must move the shared boundary, not move one independent chord block and create holes or overlaps.

Boundary movement must preserve at least one tick duration for both adjacent events.

## 13. Timeline mutation contract

UI code must not directly manipulate derived timing fields.

All modifications go through a mutation layer with operations equivalent to:

```cpp
moveBoundary()
replaceChord()
insertNoChord()
deleteChord()
splitChord()
mergeChord()
```

Required processing sequence:

```text
User operation
-> Mutation API
-> normalizeTimeline()
-> validateTimeline()
-> publish playback snapshot
-> render updated state
```

Invalid timeline state must not reach normal playback.

## 14. Provenance and verification

These are independent axes.

```cpp
enum class Provenance {
    SourceExact,
    Inferred,
    UserEdited
};

enum class Verification {
    Unverified,
    Verified,
    Error
};
```

Parser output alone must never become `Verified`.

Human confirmation is required for promotion to `Verified`.

## 15. Local chord TXT workflow

The application reads local TXT chord sheets only.

A bar-grid source may be `SourceExact` when position is explicitly recoverable.
Lyrics-embedded chords without exact timing are `Inferred`.

When the user saves, V1 rewrites the whole score into an application canonical TXT representation suitable for deterministic reload.

Canonical round-trip guarantees:

- measure order
- time signature
- chord identity
- chord boundaries
- N.C.
- durations
- timeline order

Original lyric layout, arbitrary spaces, decorative comments, and hand-formatted line wrapping are not required to survive canonical rewrite.

User edits take precedence over parser inference.

## 16. BassEngine timing

Bass pattern timing is separate from chord correctness.

For 4/4 basic patterns:

- 8-Beat City Pop: candidate triggers at ticks 0,2,4,6,8,10,12,14
- Jazz Walking: candidate triggers at ticks 0,4,8,12
- Latin/Bossa baseline: candidate triggers include tick 0 and tick 10
- Root half-note baseline: candidate triggers at ticks 0 and 8

Pattern logic must respect actual measure length through metric data.

## 17. Rootless Voicing Trainer

Rootless violation evaluation occurs only on a new physical USB-MIDI NoteOn.

Do not re-trigger a violation solely because:

- a held note crosses a chord boundary
- CC64 sustain keeps a previous note sounding

Monitoring octaves 1 through 7 must be individually configurable. A Voicing preset for Oct 3 through 5 is required by the UX specification.

## 18. Rehearsal MARK and local playback

A MARK stores at least:

- absolute tick
- resolved measure
- beat/subdivision label
- current chord event

Measure/beat resolution must come from `measuresMap`.

Local playback modes include:

- current measure
- previous measure + current measure
- four-measure loop

Loop boundaries must use actual metric map boundaries, not fixed tick counts.

## 19. Real-time constraints

Do not perform heavy work in the real-time audio path, including:

- file I/O
- score parsing
- bulk logging
- UI rendering
- large allocations
- network operations

The WASAPI clock is authoritative; rendering follows it.

## 20. Persistence and safety

All persisted user data remains local.

Canonical TXT replacement should use a safe temporary-write/validate/replace strategy so a failed save cannot truncate a valid score to zero bytes.

## 21. Acceptance gates

Minimum V1 gates:

- `WINDOWS_ONLY_GATE`: Windows 11 x64 native application
- `OFFLINE_GATE`: normal operation with networking disabled
- `NO_NETWORK_GATE`: runtime does not initiate network communication
- `USB_MIDI_GATE`: physical USB-MIDI NoteOn/Off observed
- `GAP_GATE`: zero undefined timeline gaps
- `OVERLAP_GATE`: zero Chord Event overlap
- `NC_GATE`: silence represented by explicit N.C.
- `BOUNDARY_GATE`: `left.end == right.start` after edit
- `METRIC_GATE`: 4/4 -> 2/4 -> 4/4 remains exact
- `NO_FIXED_MEASURE_GATE`: seek/loop/MARK do not use fixed measure multiplication
- `NORMALIZE_GATE`: every timeline mutation normalized
- `VALIDATION_GATE`: invalid timeline rejected from playback
- `ROUNDTRIP_GATE`: canonical save/reload preserves chord, boundary, metric and N.C.
- `VERIFICATION_GATE`: parser cannot auto-promote to Verified
- `ROOTLESS_GATE`: monitored-octave root NoteOn detected
- `ROOTLESS_HOLD_GATE`: boundary crossing alone causes no new violation
- `SUSTAIN_GATE`: CC64-held note causes no boundary re-alert
- `AUDIO_CLOCK_GATE`: DX11 FPS variation does not alter musical timing
- `MARK_GATE`: MARK location matches metric SSOT
- `SAVE_SAFETY_GATE`: save failure does not corrupt existing canonical TXT
