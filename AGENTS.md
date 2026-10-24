# AGENTS.md: notes for coding agents and contributors

This file is the hand-off document for anyone, human or AI agent, who
continues work on Tangier. Read it before changing anything. Update it when you
make a decision that the next person needs to know about.

## What Tangier is

A polyphonic virtual-analogue synth plugin (VST3 and standalone app) made with
JUCE. It is a port of the **Interzone** module from Valley Audio's
[ValleyRackFree](https://github.com/ValleyAudio/ValleyRackFree) VCV Rack plugin
(ported from commit `86f02e431136a7f5c96a872b99b7115b7e133e05`, file
`src/Interzone/Interzone.cpp`). Interzone resembles a Roland SH-101 voice made
polyphonic, with a Juno-style high-pass filter.

- Repository: https://github.com/duddex/tangier (public)
- Author identity for commits: `duddex <7746865+duddex@users.noreply.github.com>`

## Getting started on a new machine

```sh
git clone --recurse-submodules https://github.com/duddex/tangier.git
cd tangier
cmake -S . -B build                      # first configure builds juceaide (takes a few minutes)
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

- If you cloned without `--recurse-submodules`, run `git submodule update --init`.
- Requirements: CMake 3.22 or newer and a C++20 compiler. Development used
  Visual Studio 2026 (MSVC 14.51) on Windows. macOS and Linux should work with
  JUCE but have **not been tested**. On Linux, install JUCE's dependencies first
  (ALSA, X11, freetype, etc.; see JUCE's `docs/Linux Dependencies.md`).
- Outputs: `build/Tangier_artefacts/Release/VST3/Tangier.vst3` and
  `build/Tangier_artefacts/Release/Standalone/Tangier.exe`.
- The VST3 is **not** copied into the system folder automatically
  (`COPY_PLUGIN_AFTER_BUILD FALSE`, because that needs admin rights).

## Repository layout

| Path | Contents |
|---|---|
| `src/dsp/` | Header-only DSP building blocks ported from Valley's SSE code to scalar C++: `DirectOscillator` (PolyBLEP saw/pulse/sub), `OtaFilter` (4-stage TPT ladder, 2/4 pole), `OnePoleFilters`, `LoopingAdsr`, `Lfo` (7 waves), `Noise` (white/pink), `DspUtils` (polyBLEP, tanh approximation). No JUCE dependency. |
| `src/engine/SynthEngine.*` | Voice signal flow (port of `Interzone::getCV`/`tickSynth`), 16-voice allocation, MIDI handling, global LFO and noise. No JUCE dependency. |
| `src/Parameters.*` | All parameter IDs, ranges, defaults and display strings (APVTS layout). |
| `src/PluginProcessor.*` | JUCE `AudioProcessor`: reads parameters into `SynthControls`, splits each block at MIDI events, handles the optional external input bus, saves and restores state. |
| `src/PluginEditor.*` | Custom UI: sections (VCO, Mixer, VCA, Global / Filter, LFO, Envelope) and on-screen keyboard. |
| `tests/DspTests.cpp` | Self-contained test executable (no framework), run via CTest. Builds against `src/dsp` and `SynthEngine.cpp` without JUCE. |
| `libs/JUCE` | JUCE **9.0.4** as a git submodule (shallow). |
| `licenses/`, `LICENSE`, `NOTICE.md` | Licence texts and notices. See "Licensing rules" below. |
| `.github/workflows/build.yml` | Windows CI: configure, build, test, zip. A `v*` tag creates a GitHub release. |

## Key design decisions (do not change without good reason)

- **Plugin identity is fixed:** `PLUGIN_MANUFACTURER_CODE Ddex` (shared with
  other duddex plugins such as Aulos), `PLUGIN_CODE Tngr`,
  `COMPANY_NAME "duddex"`. Changing these after a release breaks saved DAW
  projects.
- **Parameter IDs are part of saved state.** Don't rename or remove IDs in
  `Parameters.h`. Add new ones instead. All IDs use `ParameterID { id, 1 }`. Use
  version `2` for parameters added later.
- **Units follow the original Interzone code:** pitch in V/oct (C4 = MIDI 60 =
  0 V), filter cutoff 0..10 (5 = 440 Hz), gate and trigger at 10 V, engine output
  in volts. Output is scaled by 1/10, like VCV Rack's audio module, then by
  *Volume*. Keep these units so behaviour can be compared with the original.
- **The DSP and engine code must not depend on JUCE**, so they stay testable
  in `TangierDspTests`.
- **Parameters are read once per block** (Interzone read them every 16
  samples). Nothing is smoothed yet.
- **MIDI mapping** that replaces Interzone's CV inputs is documented in
  `README.md`. Keep the README up to date if it changes.

### Intentional deviations from Interzone (also listed in NOTICE.md)

1. Cycle mode only loops while the gate is held. In the original it cycles
   forever, which would leave plugin voices sounding indefinitely.
2. The envelope starts in the "attacking" state. In VCV the gate is always low
   before the first note, which arms the attack; a plugin voice may receive a
   gate on its very first sample.
3. The OTA filter computes `g = tan(pi*f/fs)` directly instead of using Valley's
   1.1M-entry look-up tables (8.8 MB per filter).
4. Interzone's *Manual gate* button and the envelope/LFO output jacks are not
   ported. The unused `FILTER_INPUT` was never on Interzone's panel.
5. Additions: voice count, pitch-bend range, velocity sensitivity (default 0.5),
   LFO key sync, master volume, sustain pedal, mono legato note stack.

## Licensing rules (important)

- Tangier is **GPL-3.0-or-later** (derived from GPLv3 ValleyRackFree code). JUCE
  is used under **AGPLv3**. Never switch to a commercial JUCE licence: it
  wouldn't help, because the GPL code still requires the plugin to be open
  source.
- Every new source file needs the header used by the existing files:
  `// SPDX-License-Identifier: GPL-3.0-or-later` plus copyright. Files ported
  from Valley code must also credit "Dale Johnson, Valley Audio" and briefly
  state what was changed (GPLv3 section 5a).
- If you port more code from ValleyRackFree, VCV Rack or elsewhere, add it to
  `NOTICE.md`. Check its licence first: VCV's *visual designs* (panels) are
  CC BY-NC-ND and must **not** be used.
- Don't use the names "Valley", "Interzone" or "VCV" as product branding.
  "Based on Valley Interzone" is fine.
- Don't build AAX or VST2 targets (their SDK licences aren't GPL-compatible).
- Release zips must include `LICENSE`, `NOTICE.md` and `licenses/` (the CI
  workflow already does this).

## Conventions

- C++20, JUCE coding style (spaces before parentheses in calls, `camelCase`,
  braces on new lines), 4-space indentation.
- Comments only where behaviour isn't obvious from the code.
- Build must stay free of warnings (`juce_recommended_warning_flags` are on).
- Make small, focused commits, one per feature step, with an imperative
  subject line, e.g. "Add preset browser". Run the tests before committing.
- Line endings: files are committed with LF. Git on Windows prints
  "LF will be replaced by CRLF" warnings; these are harmless.

## Verifying changes

1. `cmake --build build --config Release` with no warnings.
2. `ctest --test-dir build -C Release --output-on-failure` passes. Add tests to
   `tests/DspTests.cpp` for new DSP or engine behaviour.
3. For UI changes, run the standalone app and look at it. On Windows, an agent
   can screenshot the window with the Win32 `PrintWindow` API. Normal screen
   capture fails because the window usually isn't in the foreground.
4. CI must be green after pushing (`gh run list --repo duddex/tangier`).

## Current state (2026-10-24)

- Version 0.1.0, not yet tagged or released.
- Builds and tests (12 tests) pass locally and in CI.
- Checked: the DSP unit tests and the standalone UI layout (via screenshot).
- **Not yet checked:** listening tests against Interzone in VCV Rack, loading
  the VST3 in a DAW, and pluginval validation.

## Ideas / open tasks

Roughly in priority order:

1. Validate the VST3 with [pluginval](https://github.com/Tracktion/pluginval)
   (strictness 5+) and test in a DAW (automation, state save and reload, sample
   rate changes, large block sizes).
2. A/B-compare the sound with Interzone in VCV Rack. Same settings should sound
   the same apart from the intentional deviations listed above.
3. Parameter smoothing for zipper-free automation (cutoff, levels, volume).
4. Presets: factory presets and/or a simple preset menu (`getNumPrograms`
   currently returns 1).
5. Optional legato mode in mono (currently every new note retriggers the
   envelope, as with VCV's MIDI-CV retrigger output).
6. Performance: the filter calls `std::tan` per voice per sample. Could update
   coefficients every N samples or use SIMD across voices (like the original).
7. Resizable or scalable editor.
8. CI builds for macOS (VST3/AU) and Linux. Note that AU needs an Apple
   developer setup for signing.
9. Add a `.gitattributes` to normalise line endings.
