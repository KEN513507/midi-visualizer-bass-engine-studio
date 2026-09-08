# USB-MIDI Bring-up Test

## 1. Purpose

Before connecting MIDI to DirectX UI, BassEngine, timeline parsing, Rootless Trainer, or WASAPI playback logic, prove the physical USB-MIDI path independently with a console diagnostic harness.

Target environment:

- Windows 11 x64
- physical USB-MIDI keyboard or physical MIDI keyboard through a USB-MIDI interface
- Windows-native MIDI API
- no Bluetooth or network MIDI

## 2. Connection direction

Keyboard to PC:

```text
Keyboard MIDI OUT
-> USB-MIDI interface MIDI IN
-> Windows 11
```

PC to keyboard/external sound generator:

```text
Windows 11
-> USB-MIDI interface MIDI OUT
-> Keyboard/device MIDI IN
```

Cable labels can be perspective-dependent. Confirm actual signal direction by observation rather than label assumptions.

## 3. Minimum bring-up sequence

Run the following tests before integrating MIDI into the application:

| Test | Observation | PASS |
|---|---|---|
| MIDI-01 | enumerate input/output devices | expected physical device/ports visible |
| MIDI-02 | keyboard -> PC NoteOn | note, velocity, channel decoded |
| MIDI-03 | keyboard -> PC NoteOff | matching release decoded |
| MIDI-04 | CC64 sustain | controller 64 and value observed |
| MIDI-05 | PC -> MIDI OUT NoteOn/Off | connected device receives intended note |
| MIDI-06 | emergency note release | All Notes Off/reset leaves no stuck note |
| MIDI-07 | unplug | no crash or unhandled exception |
| MIDI-08 | reconnect/reopen | device can be enumerated/opened again |
| MIDI-09 | event timing | receive timestamp is logged |
| MIDI-10 | channel | channel 1..16 decoded correctly |

## 4. Console logging contract

Use a stable prefix so logs are searchable:

```text
[MIDI][ENUM]
[MIDI][OPEN]
[MIDI][RX]
[MIDI][TX]
[MIDI][WARN]
[MIDI][ERROR]
[MIDI][CLOSE]
```

Representative normal log:

```text
[MIDI][ENUM][IN ][0] USB MIDI Interface
[MIDI][ENUM][OUT][0] USB MIDI Interface
[MIDI][OPEN][IN ] id=0 OK
[MIDI][OPEN][OUT] id=0 OK
[MIDI][RX] ts=8123ms ch=1 NOTE_ON  note=60 C4 vel=102
[MIDI][RX] ts=8368ms ch=1 NOTE_OFF note=60 C4 vel=0
[MIDI][RX] ts=9011ms ch=1 CC controller=64 value=127 SUSTAIN=ON
[MIDI][RX] ts=9544ms ch=1 CC controller=64 value=0 SUSTAIN=OFF
[MIDI][TX] ch=1 NOTE_ON  note=60 C4 vel=96
[MIDI][TX] ch=1 NOTE_OFF note=60 C4 vel=0
```

## 5. MIDI short-message decoding

For a three-byte short message:

```text
status = byte 0
data1  = byte 1
data2  = byte 2
channel = (status & 0x0F) + 1
command = status & 0xF0
```

Minimum commands to decode:

- `0x80`: Note Off
- `0x90`: Note On
- `0xB0`: Control Change
- `0xE0`: Pitch Bend, diagnostic visibility only in V1 bring-up

A Note On with velocity zero must be normalized to Note Off behavior:

```cpp
if ((status & 0xF0) == 0x90 && velocity == 0) {
    // treat as Note Off
}
```

## 6. Note-name convention gate

Human-facing convention is fixed:

```text
MIDI note 60 = C4
```

Diagnostic log for pressing physical Middle C should therefore contain:

```text
NOTE_ON note=60 C4
```

If the physical keyboard is known to be untransposed and another note number is observed, investigate keyboard transpose/octave settings or input selection before changing application note naming.

## 7. Velocity observation

Test repeated weak and strong key presses.

Expected behavior is a meaningful range, for example:

```text
weak:   21 34 29
strong: 101 118 124
```

If every event is fixed to a single velocity, inspect the keyboard's fixed-velocity/curve setting before compensating in software.

Do not silently normalize away evidence during bring-up.

## 8. Sustain / CC64

Log controller 64 explicitly.

Typical binary pedal behavior:

```text
value=127 SUSTAIN=ON
value=0   SUSTAIN=OFF
```

Application interpretation:

```cpp
sustainOn = value >= 64;
```

Intermediate values may be sent by some pedals/controllers and should remain observable in diagnostics.

## 9. PC-to-device output test

Send one conservative test note, e.g. C4 / MIDI 60 / velocity 96, then send an explicit Note Off after a short duration.

Always log TX separately from RX.

Do not assume output success proves input success, or vice versa.

## 10. Stuck-note protection

At shutdown and on output-failure cleanup, attempt safe release operations appropriate to the chosen Windows MIDI API.

For classic MIDI output, CC123 All Notes Off may be sent before/reset during teardown.

Diagnostic shutdown should make stuck notes highly unlikely even after an exception.

## 11. Exception and callback boundary rules

No C++ exception may escape a MIDI callback boundary.

Callback logic should catch all unexpected exceptions and emit a compact error record such as:

```text
[MIDI][ERROR] exception inside MIDI callback
```

External API return values must be checked and translated into readable operation-specific errors.

Example categories:

```text
[MIDI][ERROR] midi input open failed code=... message=...
[MIDI][ERROR] midi output send failed code=... message=...
[MIDI][WARN] device unavailable during reopen
```

## 12. USB unplug test

With the application/diagnostic running, disconnect the physical USB-MIDI device.

PASS behavior:

- no access violation
- no unhandled exception
- no process crash
- connection state transitions away from Connected
- an observable warning/error is emitted
- later enumeration/reopen is possible after reconnect

FAIL behavior includes silent stale state, crash, or permanent unrecoverable device state requiring process restart without a documented platform reason.

## 13. Runtime connection-state model

Use an explicit state model or equivalent:

```cpp
enum class MidiConnectionState {
    Disconnected,
    Enumerating,
    Opening,
    Connected,
    Error
};
```

USB unplug/replug is a normal runtime state transition, not an impossible edge case.

## 14. Timing observation

Log receive timestamps for NoteOn events during bring-up.

Do not interpret human performance jitter as USB transport jitter.

For transport/round-trip measurement, use a deliberate physical MIDI loopback and compare TX and RX timestamps under controlled conditions.

The production music master clock remains WASAPI. MIDI callback timestamps are diagnostic/input observations, not a replacement master music clock.

## 15. Bring-up isolation rule

Do not integrate the MIDI path into:

- DX11 piano rendering
- waterfall rendering
- BassEngine
- Rootless Trainer
- score parser
- timeline mutation

until the minimum console gates pass.

The intended dependency flow is:

```text
physical USB-MIDI
-> Windows MIDI API
-> console diagnostic proof
-> MidiInput abstraction
-> timeline/rootless/application state
-> DX11 visualization
```

## 16. Required bring-up gates

```text
USB_MIDI_ENUM_GATE        = PASS
NOTE_ON_GATE              = PASS
NOTE_OFF_GATE             = PASS
VELOCITY_GATE             = PASS
CC64_GATE                 = PASS
MIDI_OUT_GATE             = PASS
ALL_NOTES_OFF_GATE        = PASS
UNPLUG_NO_CRASH_GATE      = PASS
REOPEN_GATE               = PASS
MIDI60_IS_C4_GATE         = PASS
```

Do not claim USB-MIDI bring-up complete until evidence exists for every required gate applicable to the connected hardware.
