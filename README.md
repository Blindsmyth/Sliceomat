# Slice-O-Mat

Music effect (AU / VST3 / Standalone): live stereo input plus white noise through a mix, MIDI-triggered AD envelope into a VCA, then a resonant low-pass.

Any MIDI note-on on any channel retriggers the envelope. Note-off is ignored.

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE 8 is fetched automatically. On macOS the AU is copied to `~/Library/Audio/Plug-Ins/Components` and the VST3 to `~/Library/Audio/Plug-Ins/VST3`.

In Logic, load **Slice-O-Mat** as a MIDI-controlled Effect so it receives audio and MIDI.

## Parameters

| Knob | Role |
| --- | --- |
| Noise / Input | Mix gains |
| Attack / Decay | AD envelope times |
| Vol Mod | 1 = VCA follows env, 0 = VCA stays open |
| Pitch / Reso / Flt Env | Filter cutoff (MIDI note), resonance, envelope to cutoff |

## License

AGPL-3.0-or-later (JUCE). See [LICENSE](LICENSE).
