# Turing Machine Track Mode

Buchla-inspired looping shift register sequencer as a new PEW|FORMER track type.

## Why this is different from existing random features

| Existing Feature | What it does |
|---|---|
| `RunMode::Random` | Picks a completely random step each tick — no memory |
| `RunMode::RandomWalk` | Moves ±1 step, no pattern memory |
| `Modulator::Shape::Random` | CV modulation source, not a pitch sequencer |
| `RandomGenerator` | One-shot offline pattern fill — not live |
| Per-step `GateProbability` / `NoteVariationProbability` | Step-level probability on a fixed sequence |

None of these give a looping, evolving melodic phrase. The Turing Machine is different: at 0% probability it locks into an exact repeating phrase. At ~50% it slowly mutates. At 100% it's chaotic.

## Algorithm

A 16-bit recirculating shift register:

```cpp
// On each clock tick:
bool feedbackBit = (shiftReg >> (loopLen - 1)) & 1;  // MSB of active window
if (random(1000) < (int)(flipProb * 1000))
    feedbackBit = !feedbackBit;  // probabilistic flip
shiftReg = (shiftReg << 1) | feedbackBit;
uint8_t cvBits = (shiftReg >> (loopLen - 8)) & 0xFF;
outputCV(quantize(cvBits, scale, rootNote));
```

## Parameters

| Parameter | Range | Effect |
|---|---|---|
| `loopLen` | 2–16 steps | Phrase length |
| `probability` | 0–100% | 0%=locked loop, 50%=evolving, 100%=chaos |
| `scale` + `rootNote` | existing scales | Quantize output to musical scale |
| `gateMode` | MSB / probability | How gates are generated |

## What to reuse

- `core/utils/Random.h` — LCG for bit generation
- `NoteSequence` scale/rootNote quantization — 1V/oct, already works
- `TrackEngine` base class — new `TuringTrackEngine` extends this
- `Track::TrackMode` enum — add `Turing` entry
- CV routing infrastructure — `probability` can be CV-routable

## Estimated scope

| Component | File(s) | Effort |
|---|---|---|
| Model | `model/TuringTrack.h` | Small |
| Engine | `engine/TuringTrackEngine.h/.cpp` | Medium |
| Track type | `model/Track.h` | Trivial |
| UI page | `ui/pages/TuringTrackPage.cpp` | Medium |

RAM: ~30 bytes per track × 8 tracks = ~240 bytes. Current free RAM: ~32KB. Fine.

## References

- [HAGIWO MOD1 Turing Machine](https://github.com/rob-scape/hgw-mod1-firmwares/blob/39749a9/mod1-turingmachine/mod1-turingmachine.ino)
- [Shroud of Turing](https://github.com/seanrieger/shroud-of-turing)
- [Music Thing Modular original](https://github.com/TomWhitwell/TuringMachine)
