# MidiVisualizer + BassEngine Studio

Windows 11専用の個人練習用ネイティブアプリケーションです。

USB-MIDI鍵盤からの演奏入力を可視化し、コード譜に基づくベース伴奏、Chord Boundary編集、局所ループ、Rootless Voicing練習を行います。

## V1 scope

- Windows 11 x64 only
- Native C++17+
- DirectX 11 rendering
- WASAPI audio / master music clock
- Physical USB-MIDI input only
- Fully local runtime: no network, cloud, account, telemetry, or online dependency
- Mouse + physical keyboard UI only; no touch interaction
- Chord Event array as harmonic timeline SSOT
- `measuresMap` as metric SSOT
- Explicit `N.C.` event for silence; undefined timeline gaps are forbidden

## Project status

Initial requirements and bring-up preparation only. Application implementation has not started yet.

## Documents

- `docs/SYSTEM_REQUIREMENTS.md` - product/system architecture and invariants
- `docs/UI_REQUIREMENTS.md` - desktop UX/UI requirements
- `docs/USB_MIDI_BRINGUP_TEST.md` - minimum USB-MIDI console bring-up and validation gates
- `CLAUDE.md` - permanent project-wide instructions for Claude Code

## Runtime constraints

The final application must operate with the Windows 11 machine completely offline. Network connectivity is neither required nor permitted by the application runtime.

Claude Code and GitHub are development tools and are outside this runtime constraint.
