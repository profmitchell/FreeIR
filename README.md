# FreeIR

A free, open-source impulse response mixer plugin for music production. Load up to 4 IRs, blend them with independent level/pan/delay controls, auto-align phase, shape with a built-in EQ, and export the result as a single WAV.

![FreeIR Screenshot](Screenshot%202026-02-16%20at%204.57.09%20PM.png)

## Features

- **4-Slot IR Mixer** — Load `.wav` / `.aif` impulse responses into 4 independent slots
- **Per-Slot Controls** — Level fader, pan knob, delay knob, solo & mute
- **Auto Align** — One-click cross-correlation alignment to phase-lock your IRs
- **IR Waveform Viewer** — Overlaid waveform display with per-slot color coding
- **6-Band EQ** — Lo Cut, Hi Cut, Bass, Mid (Gain/Freq/Q), Treble, Air, Output
- **Header Tone Knobs** — Quick-access Bass, Treble, Air, and Volume
- **IR Export** — Bounce the mixed result to a single `.wav` file
- **Full Preset Support** — Save and recall all settings via DAW session state

## Building

### Prerequisites

- [JUCE](https://juce.com/) v7+ (place or symlink at `../JUCE` relative to this project, or adjust `CMakeLists.txt`)
- CMake 3.22+
- C++17 compiler (Xcode / Clang on macOS)

### Build Steps

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

The built plugin will be automatically installed to your system plugin folders:
- **VST3**: `~/Library/Audio/Plug-Ins/VST3/`
- **AU**: `~/Library/Audio/Plug-Ins/Components/`

## License

MIT
