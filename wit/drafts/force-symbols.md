# Force Symbols — Universal Pictographs

The current symbols (π ∘ ∮ δ) are borrowed Greek. They carry wrong
connotations: π means 3.14 to everyone, ∮ means contour integral,
δ means change. None of these match what the forces actually DO.

Method: look up what WORDS live at each force's coordinates (from
primes.md), then find symbols that already represent those words
universally — no explanation needed.


## The Discovery: Media Controls ARE Forces

Tape transport controls (▶ ⏸ ⏹ ⏩ ⏪ ⏺) are understood by every
human on earth. No language. No training. A kid in Tokyo and a
grandma in Lagos both know what ▶ means. No standards body decreed
this — the symbols just worked because the operations are fundamental.

A tape IS a loop. A cassette IS a ring buffer.
Play = read from the buffer. Record = write to the buffer.
FF = advance the cursor. Rewind = retreat the cursor.

Combined with ✅/❌ (also universal), 6 of 8 forces are covered.


## Full Mapping

| # | Force | Old | Coords | Words at coords | New symbol | Why it works |
|---|---|---|---|---|---|---|
| 0 | wave | — | (0,0,0,0) | silence, identity, nothing | ⏸ or 🌊 | pause = nothing happening. wave = the shape itself |
| 1 | read | π | (+1,0,0,0) | DO, NOW, HAPPEN | ▶ | play = read from tape. DO = press play |
| 2 | write | π̄ | (-1,0,0,0) | emit, output | ⏺ | record = write to tape |
| 3 | call | ∘ | (0,+1,0,0) | NEAR, approach | 📞 or 👉 | phone = reach toward. pointing = spatial direction |
| 4 | return | ∘̄ | (0,-1,0,0) | INSIDE, back | 📵 or 👈 | hang up = come back. point back = return |
| 5 | advance | ∮ | (0,0,+1,0) | ONE, THING, step | ⏩ | fast forward = advance through substance |
| 6 | rewind | ∮̄ | (0,0,-1,0) | SMALL, remove | ⏪ | rewind = step backward. literally the word |
| 7 | test | δ | (0,0,0,+1) | GOOD, SAME, TRUE | ✅ | checkmark = pass = SAME = TRUE |
| 8 | guard | δ̄ | (0,0,0,-1) | BAD, NOT, OTHER | ❌ | cross = reject = NOT = BAD |


## Why Current Symbols Are Wrong

| Current | What people read | What it IS | Mismatch |
|---|---|---|---|
| π | 3.14, circles, ratio | DO, NOW, HAPPEN | nothing circular about reading |
| ∘ | function composition | NEAR, approach | abstract vs concrete |
| ∮ | contour integral | ONE, THING, step forward | calculus for "one step"? |
| δ | delta = change | GOOD, SAME, TRUE | "change" for "sameness"? opposite |

Overbars (π̄, ∘̄, etc.) work fine — antimatter = inverse is real notation.
The base glyphs are the problem.


## Call/Return — The Gap

Call and return were the only two without obvious media-control
symbols. But phones filled it:

  📞 green = CALL = reach out toward = ∘
  📵 red = HANG UP = come back from = ∘̄

Every smartphone on earth. Green phone = connect (spatial reach).
Red phone = disconnect (come back). Already universal.

Alternative: pointing (👉/👈). Understood from age ~1. Infants point
before they speak. Pure spatial direction. One of the first image
schemas (PATH/DIRECTION). Exactly χ = spatial reach.


## Compound Bonds — From Words

| Bond | Old | Words at coords | Candidate shapes |
|---|---|---|---|
| fold | π∘ | MOVE, PATH | 🚶 walking, arrow/path |
| slurp | π∮ | LIVE, CYCLE | 🔄 cycle, heartbeat |
| filter | πδ | SEE, HEAR | 👁 eye — perception IS filter |
| fix | ∘∮ | BE SOMEWHERE, BODY | 📍 pin on map |
| maybe | ∘δ | THIS, HERE, WHERE | 👆 point here |
| take_while | ∮δ | I, KNOW, SELF | 🪞 mirror? self-aware |
| bind | ∘∮δ | TOUCH, contact | 🤝 handshake, linked |
| hylo | π∘∮δ | everything, 'a' | ☀️ sun, all radiating |

These need more work. The 8 fundamentals are the priority.


## Notes

- The media industry independently discovered force symbols and made
  them universal without a spec. Same operations the lattice calls forces.
- When SDF rendering exists, glyphs can be computed from coordinates
  directly — the shape IS the coordinate evaluated as a distance field.
- Wave characters (the character encoding) will eventually need designed
  glyphs. These force symbols are the seed — if the forces have good
  shapes, compounds compose visually the same way bonds compose.
- Interim: emoji work NOW in code comments, docs, discussions.
  Better than Greek that means the wrong thing.


## Open Questions

1. Call/return: phone (📞/📵) or pointing (👉/👈)? Phone is more
   "call"-like. Pointing is more spatial (matches the χ axis meaning).
2. Wave: ⏸ (pause, universal) or 🌊 (wave, literal)? Different
   emphasis — operational vs pictographic.
3. Do compounds need their own symbols or do they compose from the
   4 base symbols visually? (Like how 👁 = ▶+✅ if filter = read+test)
4. How do these map to the eventual SDF rendering? Can the coordinate
   itself generate a shape that resembles the emoji?
