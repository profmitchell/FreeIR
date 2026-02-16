Cabinet IR Loader (4-IR Mixer + Auto Align)
Version: v1.0
Owner: Mitchell
Target Hosts: Ableton Live (VST3), Logic Pro (AU)
Stretch Targets: AUv3 (iOS), AAX (Pro Tools)
	1.	Product summary
A real-time cabinet impulse response (IR) loader plugin designed for guitar/bass tone shaping. It supports up to 4 IR slots mixed in parallel with independent level/pan/delay, plus an Auto Align feature that time-aligns IRs to avoid phase cancellation when blending. Includes a tone/EQ section and a waveform viewer for IR timing visualization.

Primary use case: load 2–4 cabinet IRs, blend them confidently, and avoid phase issues automatically.
	2.	Goals

	•	Recreate the workflow and feel of the reference UI: 4 slots, quick load, mute/solo, level faders, pan/delay knobs, and a clear “Auto Align” button.
	•	Make multi-IR blending sound “tight” and predictable via auto-alignment.
	•	Be stable, low-latency, and efficient for real-time guitar tracking.
	•	Provide a clean, recallable parameter set with presets.

Non-goals (v1)
	•	Amp modeling, distortion/saturation, mic modeling, room simulation
	•	IR capture tools (deconvolution, sweep generation)
	•	Advanced IR editing (windowing, minimum phase, normalization choices) beyond basics

	3.	Users & scenarios
Target users

	•	Guitarists using amp sims / NAM / Neural DSP / Ableton rigs
	•	Mix engineers blending multiple cab IRs
	•	Sound designers building layered guitar tones

Key scenarios
	•	Load one IR for a standard cab sound.
	•	Load 2–4 IRs and blend for “wide” or “complex” tone.
	•	Press Auto Align to fix phase and tighten the blend.
	•	Use lo/hi cuts + mid EQ to fit in a mix.
	•	Quickly A/B presets and slot combinations.

	4.	Functional requirements

4.1 IR slots (4 parallel)
Each slot (1–4) must support:
	•	Load IR from file (WAV/AIFF, mono/stereo)
	•	Display IR name in list
	•	Clear/unload slot
	•	Mute toggle (M)
	•	Solo toggle (S)
	•	Level fader (linear gain or dB; see DSP section)
	•	Pan knob (constant power)
	•	Delay knob (0 to N ms, for manual time offset)
	•	Slot enabled state (if empty, contributes nothing)

Slot interaction rules
	•	If any slots are Soloed, only soloed slots output.
	•	Mute overrides Solo only for that slot (standard behavior: soloed slots still play unless muted).
	•	Loading a new IR into a slot resets that slot’s delay to 0 ms unless “Keep Delay” preference is enabled (preference optional).

4.2 Auto Align
	•	Button: “Auto Align”
	•	Aligns all loaded slots to a reference slot (default: Slot 1, or “loudest energy” slot if Slot 1 empty; define behavior below).
	•	Must compute relative delays and apply compensating offsets per slot.
	•	Auto Align must update:
	•	Per-slot delay offsets (writes into each slot’s delay parameter)
	•	Waveform viewer overlay alignment

Auto Align behavior details
	•	Reference selection (v1 default):
	•	If Slot 1 loaded: reference = Slot 1
	•	Else reference = lowest-numbered loaded slot
	•	Alignment method:
	•	Time-domain cross-correlation on the IR signals (or windowed early portion) to find offset maximizing correlation.
	•	Limit search window: ±5 ms (configurable internally)
	•	Use oversampled correlation or sub-sample interpolation for accuracy (optional but recommended).
	•	If the computed offset exceeds the max delay parameter range, clamp and warn (UI can flash a small “clipped” icon, optional).
	•	Auto Align must run fast enough to feel instant (<100 ms typical), performed off the audio thread.

4.3 IR waveform viewer
	•	Shows time-domain waveform(s) of loaded IRs.
	•	Overlay at least 2 traces clearly; ideally up to 4 with distinct colors.
	•	Time axis: 0 ms to 5 ms (as in reference) with tick labels.
	•	Must visually reflect alignment offsets (post-align).
	•	Optional: allow zoom/time range selection (v1 can hardcode 0–5ms).

4.4 Tone / EQ section
Match the reference layout conceptually:
Required controls (v1)
	•	Lo Cut (HPF): freq range 20–500 Hz
	•	Hi Cut (LPF): freq range 2 kHz–20 kHz
	•	Mid band:
	•	Gain: -12 dB to +12 dB
	•	Freq: 200 Hz–6 kHz
	•	Q: 0.3–10
	•	Bass: shelf (suggest 80–200 Hz center), -12 to +12 dB
	•	Treble: shelf (suggest 3–8 kHz center), -12 to +12 dB
	•	Air: high shelf (suggest 8–16 kHz), -12 to +12 dB
	•	Output (Out): final gain -inf to +6 dB (or -24 to +6 dB)

Note: The reference shows Bass/Treble/Air and also a mid module; we will implement all above for parity.

4.5 Master controls
	•	Volume knob on top bar (maps to Output)
	•	Bypass (optional; hosts already provide bypass, but internal bypass is useful)
	•	Preset navigation:
	•	Previous/Next arrows
	•	Preset name display
	•	Settings (gear icon) opens a small preferences panel (optional in v1; can be stubbed)

4.6 Presets & state
	•	Must save/recall:
	•	Loaded IR file paths (or embedded IR data if you choose; see below)
	•	All slot parameters
	•	All EQ parameters
	•	Two acceptable approaches:
A) Store file paths only (fast; but breaks if files move)
B) Embed IR sample data in preset/state (bigger; but portable)
v1 recommendation:
	•	Store file paths by default + provide an option “Embed IRs in preset” (stretch).

4.7 Automation & parameter exposure
	•	All knobs/faders must be automatable in DAWs.
	•	Mute/Solo should be automatable (optional but valuable).
	•	Delay must be automatable.
	•	IR load/unload is not automatable (host limitation), but slot enable toggles are.

	5.	DSP requirements

5.1 Audio I/O
	•	Stereo in / stereo out.
	•	Accept mono input by duplicating to stereo or treating as mono->stereo (host handles channel config; define behavior):
	•	If input is mono: process as mono and output dual-mono.

5.2 Convolution engine
	•	Real-time convolution per slot.
	•	Must support:
	•	Mono IR -> applies same response to L/R (or treat as mono convolution per channel)
	•	Stereo IR -> true stereo convolution (L->L, R->R) at minimum; (crossfeed stereo optional but not required)
	•	Latency:
	•	Should report latency accurately to host if using partitioned convolution.
	•	Goal: near-zero perceived tracking latency; acceptable plugin latency depends on implementation but should be minimized.

Implementation note (not code, just requirements):
	•	Use partitioned convolution (FFT-based) for efficiency with longer IRs.
	•	Provide max IR length handling (see below).

5.3 IR length handling
	•	Support IR lengths at least up to:
	•	v1 minimum: 2048 samples @ 48k (~42ms)
	•	recommended: up to 8192 or 16384 samples with partitioning
	•	If longer IR loaded:
	•	Either truncate to max length with a warning
	•	Or support full length if engine can handle it

5.4 Per-slot processing
Slot chain:
Input -> Convolution(IR slot) -> Delay offset -> Pan -> Gain -> Sum bus

Notes:
	•	Delay can be implemented as fractional delay for sub-sample accuracy (preferred), or sample delay with interpolation.
	•	Pan should be constant-power for musical blending.

5.5 Post-sum processing
Sum bus -> Lo Cut -> Hi Cut -> Bass shelf -> Mid peak -> Treble shelf -> Air shelf -> Output gain -> Output

Filter quality targets:
	•	Stable, low CPU, no zipper noise on parameter changes.
	•	Smooth parameter ramping (e.g., 10–50ms smoothing).

5.6 Performance targets
	•	CPU:
	•	Must run 4 IRs at 48kHz with acceptable CPU on typical laptops (target: under ~5–10% per instance on modern Mac; define later via profiling)
	•	No audio dropouts; no memory allocations on audio thread.
	•	Threading:
	•	File loading, FFT plan creation, and auto-align computations must occur off audio thread.

	6.	UX/UI requirements

6.1 Overall layout fidelity
	•	UI should closely match the reference:
	•	Dark brushed/metal panel aesthetic
	•	Top bar: title left, Bass/Treble/Air knobs center, Volume right
	•	4 vertical strips left: slot controls, big Pan knob, Delay knob, Level fader
	•	Right side: list of loaded IRs, waveform display, Auto Align button, EQ knobs row

6.2 Slot strip UI
	•	Slot number prominent
	•	Trash icon to clear slot
	•	M / S buttons
	•	Pan knob label “Pan”
	•	Delay knob label “Delay”
	•	Level fader label “Level”
	•	Visual hint for “empty” slots

6.3 IR list area
	•	Shows:
	•	“1. ”
	•	“2. ”
	•	“3. Click to load impulse response…”
	•	etc.
	•	Clicking an empty line opens file dialog to load that slot.
	•	Clicking a loaded IR line opens:
	•	reveal in Finder (optional)
	•	reload/replace
	•	clear

6.4 Waveform area
	•	Must update instantly when IR is loaded/unloaded.
	•	Must update when Auto Align is pressed (traces shift/align).

6.5 Interaction polish
	•	Parameter smoothing to prevent zipper noise.
	•	Tooltips on all controls (short).
	•	Double-click resets knobs/faders to default.
	•	Shift-drag for fine adjustment (desktop).

	7.	Platform requirements

7.1 Desktop (v1)
	•	macOS: AU + VST3
	•	Windows: VST3 (optional but recommended)

7.2 iOS (stretch)
	•	AUv3
	•	Touch-friendly UI scaling
	•	File access constraints (IR file import via Files app)

	8.	Edge cases & error handling

	•	If IR file missing on load:
	•	Slot shows “Missing file” and is bypassed
	•	User can relink or clear
	•	If IR sample rate differs:
	•	Resample IR to session sample rate on load (offline)
	•	If user loads mono IR into stereo slot:
	•	Treat as dual-mono
	•	If Auto Align fails (e.g., silence/flat IR):
	•	No changes, show brief notification “Auto Align unavailable”
	•	If multiple solos/mutes create silence:
	•	Output should be silence (no surprises)

	9.	Parameter list (v1)
Per Slot (1–4)

	•	SlotX_Enable (bool) [optional; empty implies disabled]
	•	SlotX_Mute (bool)
	•	SlotX_Solo (bool)
	•	SlotX_Level (dB: -inf to 0 dB default -6 dB; or linear 0..1 default 0.7)
	•	SlotX_Pan (-100..+100, default 0)
	•	SlotX_DelayMs (0..10 ms default 0) (recommend 0..10ms)

Global
	•	LoCutHz (20..500 default 80)
	•	HiCutHz (2000..20000 default 12000)
	•	BassGainDb (-12..+12 default 0)
	•	TrebleGainDb (-12..+12 default 0)
	•	AirGainDb (-12..+12 default 0)
	•	MidGainDb (-12..+12 default 0)
	•	MidFreqHz (200..6000 default 1000)
	•	MidQ (0.3..10 default 1.0)
	•	OutputGainDb (-24..+6 default 0)
	•	Bypass (bool) optional

	10.	Acceptance criteria

	•	Load 4 IRs and blend without phasing issues after Auto Align.
	•	Auto Align produces audible tightening (reduced comb filtering) when mixing IRs.
	•	Plugin state recalls reliably across DAW sessions.
	•	CPU remains stable; no glitches on parameter automation.
	•	UI matches reference layout and feels responsive.

	11.	Milestones
M1: Convolution prototype (single IR) + output gain
M2: 4-slot mixer (level/pan/mute/solo) + basic UI layout
M3: Auto Align + waveform viewer overlay
M4: EQ/tone section + parameter smoothing + preset/state
M5: Visual polish pass (assets, typography, final layout) + release candidate