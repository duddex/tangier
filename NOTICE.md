# Notices and third-party licences

Tangier is free software, licensed under the GNU General Public License,
version 3 or (at your option) any later version. See [`LICENSE`](LICENSE).

Copyright (C) 2026 duddex

## Derived work: Valley Interzone

Tangier is a port of the **Interzone** module from the
[ValleyRackFree](https://github.com/ValleyAudio/ValleyRackFree) plugin for
VCV Rack, ported from commit `86f02e431136a7f5c96a872b99b7115b7e133e05`.

    ValleyRackFree
    Copyright (C) 2020, Valley Audio Soft, Dale Johnson
    Licensed under the GNU General Public License v3.0 or later

The full licence notice is in
[`licenses/ValleyRackFree-LICENSE.md`](licenses/ValleyRackFree-LICENSE.md).

Changes made in Tangier (2026), as required by GPLv3 section 5(a):

- Rewrote the synth engine and its DSP building blocks (oscillator, OTA
  filter, one-pole filters, looping ADSR, LFO, noise) for a standalone audio
  plugin. The original SSE/VCV Rack SIMD code became portable scalar C++.
- The OTA filter computes its coefficients directly instead of using a large
  look-up table.
- Replaced the VCV Rack CV/gate inputs with MIDI note handling, voice
  allocation, pitch bend, mod wheel, velocity and aftertouch.
- In cycle mode the envelope loops only while a note is held, so voices can be
  released. In the original, the envelope keeps cycling after the gate goes low.
- Added plugin-specific parameters: voice count, pitch-bend range, velocity
  sensitivity, LFO key sync and master volume.
- New JUCE user interface. No Valley panel graphics are used.

"Valley" and "Interzone" are names of the original author. Tangier is not an
official Valley Audio product and is not endorsed by Valley Audio or Dale
Johnson.

## VCV Fundamental (ADSR)

Interzone's looping ADSR (`VecLoopingADSR`) is based on the ADSR module from
the [VCV Fundamental](https://github.com/VCVRack/Fundamental) plugin.

    Source code copyright (c) 2016-2023 VCV
    Licensed under the GNU General Public License v3.0 or later

See [`licenses/VCV-Fundamental-LICENSE.md`](licenses/VCV-Fundamental-LICENSE.md).
Tangier does not use any VCV logos, names or visual designs.

## JUCE

Tangier is built with the [JUCE framework](https://juce.com), included as a
git submodule in `libs/JUCE`. Tangier uses JUCE under the
**GNU Affero General Public License v3.0** (one of JUCE's two licence options).
See [`licenses/AGPL-3.0.txt`](licenses/AGPL-3.0.txt) and `libs/JUCE/LICENSE.md`.

    Copyright (c) Raw Material Software Limited

GPLv3 section 13 allows combining GPLv3 code with AGPLv3 code. The
Tangier source code stays under GPLv3-or-later, and the JUCE parts stay under
AGPLv3. Distributed binaries contain both, so whoever distributes them must
follow both licences. In particular, the complete corresponding source code
must be made available.

JUCE bundles more third-party software, listed in `libs/JUCE/JUCE.spdx.json`.

## VST3 SDK

The VST3 plugin is built with the Steinberg VST3 SDK, which JUCE bundles and
which is licensed under the MIT licence. See
`libs/JUCE/modules/juce_audio_processors_headless/format_types/VST3_SDK/` in the JUCE
submodule for its licence text.

VST is a registered trademark of Steinberg Media Technologies GmbH.
