# UI Requirements

## 1. UX goal

The UI exists to support piano practice with minimal attention shift from the physical USB-MIDI keyboard.

The user must be able to understand:

- current chord
- next chord
- current measure and beat
- Chord Boundary location
- syncopation/subdivision position
- BassEngine activity
- Rootless Voicing violations
- rehearsal MARK locations
- whether a score item is source-derived, inferred, edited, verified, or erroneous

Primary interaction loop:

```text
observe -> notice problem -> mark/select -> correct boundary/chord -> listen locally -> verify
```

## 2. Interaction devices

Supported UI input:

- mouse
- mouse wheel
- physical PC keyboard
- physical USB-MIDI keyboard/pedal events

Explicitly unsupported:

- touch
- tap/double-tap/long-press semantics
- swipe
- pinch zoom
- multi-touch
- pen/Windows Ink
- tablet-mode-specific controls

No required action may depend on touch behavior.

## 3. Target desktop layout

Primary design target: 1920x1080 Windows 11 desktop.

Support normal, maximized, minimized, and resizable windows.

Main practice window is divided into five persistent regions:

```text
Global Header
Chord / Measure Timeline
Main Practice / MIDI Waterfall
88-Key Piano Visualization
Status / Shortcut Footer
```

Inspector, score source, and settings are secondary panels that appear only when needed.

## 4. Global header

At minimum show:

- application/song identity
- BPM
- playback start/stop
- rehearsal MARK action and count
- loop state
- USB-MIDI connection state
- WASAPI/audio state
- score audit state

Do not fill the header with infrequently used settings.

## 5. MARK action

`Space` is the default rehearsal MARK shortcut when no text editor owns keyboard focus.

The action must be available while playing and must not interrupt playback.

A MARK stores the current metric-resolved location and appears on the timeline and in a MARK list.

## 6. Timeline

Timeline is the primary musical-time UI, not merely a chord list.

It must show:

- measure number
- time signature
- chord identity
- chord duration
- Chord Boundaries
- playback position/playhead
- state/provenance indication
- MARKs
- explicit N.C. regions

Measures are arranged horizontally.

## 7. Playhead

The visual playhead follows the authoritative WASAPI-derived playback position.

Rendering FPS must not define musical position.

## 8. Boundary visualization and editing

Chord Boundary is the principal timing-edit target.

A boundary must have:

- normal state
- mouse hover state
- selected state
- horizontal-resize cursor or equivalent
- hit region slightly wider than the visible line for desktop mouse usability

Dragging a boundary snaps to the 16th-note grid in V1.

Dragging a whole chord block in a way that can create an undefined gap or overlap is prohibited.

## 9. Boundary micro-adjustment

Boundary timing must also be editable without drag.

User-facing labels use musical terms:

- 16th note / quarter beat
- eighth note / half beat

Do not present `StartTick 6` as the normal editing vocabulary.

Examples of preferred location labels:

- Beat 1
- Beat 2 &
- Beat 3 a

## 10. Beat and subdivision geometry

For 4/4, beat boundaries are geometrically located at:

```text
0% 25% 50% 75% 100%
```

Do not position beat labels using text-distribution tricks such as `justify-between` and treat those positions as musical geometry.

Subdivision grid follows:

```text
1 e & a | 2 e & a | 3 e & a | 4 e & a
```

Major beats, eighth subdivisions, and sixteenth subdivisions must have visually distinct hierarchy.

Grid length follows the actual metric map. 2/4, 3/4 and 5/4 must not reuse a hard-coded four-beat geometry.

## 11. Context while editing

Boundary editing should show previous, current, and next measure context where practical.

The purpose is to expose transitions around barlines and syncopated entries rather than isolating one bar from its musical context.

## 12. Inspector

Selecting a chord shows at least:

- chord symbol
- harmonic root
- quality
- optional slash bass
- provenance
- verification
- musical position

Selecting a boundary shows at least:

- left chord
- right chord
- musical boundary position
- adjacent durations
- provenance/verification relevant to the edited timing

Raw internal IDs and absolute tick values belong in an optional developer/debug view, not the primary practice UI.

## 13. N.C. and invalid gaps

N.C. must be drawn as an explicit labeled timeline block.

It must never look like an accidental black hole between chords.

A genuine invalid/undefined gap should only exist as a validation failure and must be labeled as an error, not silently rendered as empty space.

## 14. Provenance and verification presentation

Internal state axes remain separate.

Recommended visual mapping:

- SourceExact + Unverified: cyan/source state
- Inferred + Unverified: amber/inferred state
- UserEdited + Unverified: distinct edited state
- Verified: green/verified state
- Error: red/error state

Color alone is insufficient. Text labels such as `SOURCE`, `INFERRED`, `EDITED`, `VERIFIED`, and `ERROR` must be available.

Only human-verified state may use the meaning "confirmed".

## 15. Audit view

Score audit must expose at least:

- total measures
- total Chord Events
- SourceExact count
- Inferred count
- UserEdited count
- Verified count
- Error count
- N.C. count
- unresolved/unchecked items

Provide direct navigation to the next inferred/error/unverified location.

## 16. Main practice / waterfall

The central practice area primarily visualizes MIDI notes.

The horizontal position of each waterfall note must align with the corresponding piano key.

User MIDI and generated BassEngine notes must remain distinguishable.

Rootless violations may use a separate warning state.

## 17. 88-key piano

Display the standard A0 through C8 range, MIDI 21 through 108.

Middle C is C4 / MIDI 60.

White/black key geometry and octave positions must be musically correct.

The on-screen keyboard may accept mouse clicks for diagnostics, but it is not the primary performance device.

No touch-performance features are required.

## 18. Rootless Trainer UI

When enabled, display enough information to understand:

- current chord
- prohibited root pitch class
- monitored octave range

A violation on a new physical NoteOn should provide immediate, non-modal feedback including:

- offending note name
- octave
- current chord
- affected visual key

Do not interrupt practice with an acknowledgement modal.

Monitoring Oct 1 through 7 is individually configurable, with a preset for Oct 3 through 5.

## 19. MARK review and local listening

MARK list entries show at least:

- measure
- beat/subdivision
- current chord

Selecting a MARK jumps directly to its timeline location.

From the correction context, provide direct access to:

- listen to current measure
- previous + current measure
- four-measure loop
- human confirmation / `VERIFIED`

## 20. Local TXT editing

Canonical TXT editing belongs in a secondary panel/window, not permanently in the main practice area.

Text edits must have an explicit unapplied state. Reparse occurs on deliberate user action, not on every keystroke.

Unsaved timeline/chord edits must be visibly indicated.

## 21. Desktop keyboard/mouse conventions

Required baseline shortcuts:

- `Ctrl+O`: open local score
- `Ctrl+S`: save canonical score
- `Ctrl+Z`: undo where available
- `Ctrl+Y`: redo where available
- `Space`: MARK outside text input

Focus handling must prevent global practice shortcuts from firing while a text input owns the keystroke.

Hover and tooltips may be used because this is desktop-only UX, but critical functionality must not be hover-only.

Context menus may provide secondary actions, never the sole access to a primary task.

## 22. Rendering and resizing

Window resizing must preserve:

- readable timeline
- visible Inspector placement
- valid piano geometry
- waterfall-to-key horizontal alignment

Set a sensible minimum window size rather than allowing the primary practice layout to collapse into unusable geometry.

## 23. Modal policy

Normal timeline editing, verification, MARK, loop and boundary operations must not use modal dialogs.

Modal dialogs are reserved for exceptional cases such as destructive operations, unrecoverable initialization failures, or file-save failure requiring user action.

Undoable actions should execute immediately rather than asking repetitive confirmation questions.

## 24. Visual style

A dark desktop theme is the baseline because the application contains persistent animated musical visualization.

The hierarchy between background, panel, selection, active playback, verification, and error states must remain clear.

Chord symbols must make `m`, `M`, `b`, `#`, alterations, and slash basses easy to distinguish.

Display may render flats/sharps as musical symbols while the canonical storage format remains ASCII.

## 25. Debug overlay

Development builds may optionally expose:

- absolute tick
- measure ID
- audio clock
- DX11 FPS
- raw MIDI short message
- latency observations
- Event ID

These values are diagnostic only and should not dominate the normal practice UI.

## 26. UI acceptance gates

- `DESKTOP_ONLY_GATE`: every feature completable without touch
- `NO_TOUCH_GATE`: no required touch gesture/control exists
- `MOUSE_GATE`: boundary/chord correction and review work with mouse
- `KEYBOARD_GATE`: file shortcuts, MARK and undo work from physical keyboard
- `MIDI_INPUT_UI_GATE`: physical USB-MIDI NoteOn/Off matches visible key/waterfall
- `TIMELINE_GEOMETRY_GATE`: beat/subdivision positions are geometrically correct
- `VARIABLE_METER_UI_GATE`: 2/4, 3/4, 4/4, 5/4 display actual beat counts
- `BOUNDARY_UI_GATE`: shared boundary is directly selectable/editable
- `NO_GAP_VISUAL_GATE`: valid timeline has no unlabeled undefined holes
- `NC_VISUAL_GATE`: N.C. is explicitly visible
- `MUSICAL_LABEL_GATE`: normal UI uses musical location labels rather than raw ticks
- `CONTEXT_GATE`: editing provides adjacent measure context
- `VERIFIED_UI_GATE`: human confirmation is explicit and readable without color alone
- `MARK_UI_GATE`: MARK jumps to the correct metric location
- `ROOTLESS_UI_GATE`: violating root, octave, and chord are immediately understandable
- `WATERFALL_ALIGNMENT_GATE`: note waterfall aligns with 88-key geometry
- `CLOCK_UI_GATE`: rendered position follows the audio/master timeline
- `FOCUS_GATE`: text editing suppresses conflicting global shortcuts
- `RESIZE_GATE`: normal FHD/maximized and resized layouts remain usable
