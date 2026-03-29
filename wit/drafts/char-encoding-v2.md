# Character Encoding v2 — Proposed for code2/

Status: PROPOSAL. Needs review by second wit for cross-checking.


## Why Rewrite

The v1 encoding (code/) had T=case (upper/lower). Three problems:
1. Case is visual convention, not phonetics. 60% of world scripts have no case.
2. b collided with m, d collided with n (only case distinguished them).
3. T was inconsistent with word-level T=temporal and physics T=time.

code2/ has no encoding yet. Clean start.


## The Constraint

The lattice locks the dimensional meanings. They MUST be consistent
across all three layers:

| Dim | Physics | Word (primes) | Character |
|---|---|---|---|
| T | time | temporal (when, change) | temporal property of the sound |
| D | distance | spatial (where, position) | spatial property of the sound |
| M | mass | substance (what, quantity) | substance property of the sound |
| Q | charge | quality (how, evaluation) | quality property of the sound |

There's only ONE phonetic property per dimension that satisfies this:
- T = duration (how long the sound persists in time)
- D = place of articulation (where in the vocal tract)
- M = manner / openness (how much substance flows through)
- Q = voicing (quality of laryngeal vibration)

These are the four independent articulatory phonetic features.
Hurwitz says there can't be 5. The IPA organizes consonants by
exactly these features (place × manner × voicing + secondary).


## Shell 1 Values

Three values per dimension: -1, 0, +1. One byte per character.
The partition must maximize discrimination while keeping collisions
between phonetically similar sounds.

### T = temporal dynamics

| Value | Meaning | Sounds | Why |
|---|---|---|---|
| +1 | sustained | vowels, fricatives, nasals | persist in time — you can hold them |
| 0 | transitional | approximants, glides (w, j, l, r) | neither sustained nor instant — transition sounds |
| -1 | transient | stops, affricates, clicks | burst and gone — cannot be held |

The tweak from naive T=sustained: approximants/glides go to T=0
instead of T=+1. This resolves collisions (l≠z, w≠a, r≠z) because
glides are phonetically transitional — they're not truly sustained
like fricatives, and not instantaneous like stops.

### D = place of articulation

| Value | Meaning | Sounds | Why |
|---|---|---|---|
| +1 | front | labial, dental (p,b,m,f,v,w) | near the lips — closest to outside |
| 0 | central | alveolar, palatal (t,d,n,s,z,l,r) | middle of the mouth — default position |
| -1 | back | velar, uvular, glottal (k,g,ng,h) | deep in the throat — farthest in |

+D = front = near (consistent with word-level D: HERE, NEAR, THIS).
-D = back = far (consistent with word-level D: FAR, BELOW).
D=0 = alveolar, the most common consonant position. Default at zero.

### M = manner / openness

| Value | Meaning | Sounds | Why |
|---|---|---|---|
| +1 | open | vowels (a,e,i,o,u) | maximum airflow — most substance |
| 0 | partial | fricatives (f,v,s,z,sh) | restricted airflow — partial substance |
| -1 | closed | stops (p,b,t,d,k,g), nasals (m,n,ng) | blocked airflow — minimal substance |

More openness = more M = more substance flowing through.
Vowels are the most "massive" sounds (carry the most energy).

### Q = voicing

| Value | Meaning | Sounds | Why |
|---|---|---|---|
| +1 | voiced | b,d,g,v,z,m,n,l,r, all vowels | vocal cords vibrate — active charge |
| 0 | neutral | (reserved for sounds without clear voicing) | |
| -1 | voiceless | p,t,k,f,s,sh,h | no vibration — neutral charge |

Q is the cleanest dimension. Voicing is binary in most languages.
Cross-linguistically the strongest phonetic-semantic link (sound
symbolism research: voiced=large/heavy, voiceless=small/light).


## English Consonant Map

| Sound | IPA | T | D | M | Q | Hex | Notes |
|---|---|---|---|---|---|---|---|
| p | /p/ | -1 | +1 | -1 | -1 | 0xDF | voiceless labial stop |
| b | /b/ | -1 | +1 | -1 | +1 | 0xDD | voiced labial stop |
| m | /m/ | +1 | +1 | -1 | +1 | 0x5D | voiced labial nasal (T separates from b) |
| f | /f/ | +1 | +1 | 0 | -1 | 0x53 | voiceless labial fricative |
| v | /v/ | +1 | +1 | 0 | +1 | 0x51 | voiced labial fricative |
| w | /w/ | 0 | +1 | +1 | +1 | 0x15 | labial glide (T=0 separates from vowels) |
| t | /t/ | -1 | 0 | -1 | -1 | 0xCF | voiceless alveolar stop |
| d | /d/ | -1 | 0 | -1 | +1 | 0xCD | voiced alveolar stop |
| n | /n/ | +1 | 0 | -1 | +1 | 0x4D | voiced alveolar nasal (T separates from d) |
| s | /s/ | +1 | 0 | 0 | -1 | 0x43 | voiceless alveolar fricative |
| z | /z/ | +1 | 0 | 0 | +1 | 0x41 | voiced alveolar fricative |
| l | /l/ | 0 | 0 | 0 | +1 | 0x01 | alveolar lateral (T=0 separates from z) |
| r | /ɹ/ | 0 | 0 | 0 | +1 | 0x01 | alveolar approximant (collides with l — shell 2 separates) |
| k | /k/ | -1 | -1 | -1 | -1 | 0xFF | voiceless velar stop |
| g | /g/ | -1 | -1 | -1 | +1 | 0xFD | voiced velar stop |
| ng | /ŋ/ | +1 | -1 | -1 | +1 | 0x7D | voiced velar nasal |
| sh | /ʃ/ | +1 | -1 | 0 | -1 | 0x73 | voiceless postalveolar fricative |
| zh | /ʒ/ | +1 | -1 | 0 | +1 | 0x71 | voiced postalveolar fricative |
| h | /h/ | +1 | -1 | 0 | -1 | 0x73 | voiceless glottal fricative (collides with sh — both back voiceless fricative) |
| th | /θ/ | +1 | +1 | 0 | -1 | 0x53 | voiceless dental fricative (collides with f — both front voiceless fricative) |
| dh | /ð/ | +1 | +1 | 0 | +1 | 0x51 | voiced dental fricative (collides with v — same reason) |
| j | /dʒ/ | -1 | -1 | -1 | +1 | 0xFD | voiced postalveolar affricate (collides with g) |
| ch | /tʃ/ | -1 | -1 | -1 | -1 | 0xFF | voiceless postalveolar affricate (collides with k) |
| y | /j/ | 0 | -1 | +1 | +1 | 0x35 | palatal glide |


## English Vowel Map

| Sound | IPA | T | D | M | Q | Hex | Notes |
|---|---|---|---|---|---|---|---|
| a | /æ/ | +1 | +1 | +1 | +1 | 0x55 | front open voiced (= 'a' in v1) |
| e | /ɛ/ | +1 | 0 | +1 | +1 | 0x45 | central-ish open voiced |
| i | /ɪ/ | +1 | +1 | +1 | +1 | 0x55 | front open voiced (collides with a — shell 2: height) |
| o | /ɒ/ | +1 | -1 | +1 | +1 | 0x75 | back open voiced |
| u | /ʊ/ | +1 | -1 | +1 | +1 | 0x75 | back open voiced (collides with o — shell 2: height) |

Vowels mostly collide at shell 1 because they're all sustained, open,
voiced. D (front/back) is the main discriminator. Height (close vs open)
needs shell 2 — it's a finer M distinction (+2 vs +1).


## Collision Analysis

### Resolved from v1
| v1 collision | v1 reason | v2 status | How |
|---|---|---|---|
| b = m | both labial closed voiced | **separated** | T: b=-1 (stop), m=+1 (nasal) |
| d = n | both alveolar closed voiced | **separated** | T: d=-1 (stop), n=+1 (nasal) |
| d = j | both alveolar closed voiced | **separated** | T: d=-1, j=-1 BUT D: d=0, j=-1 |
| a = w | both front open voiced | **separated** | T: a=+1 (sustained), w=0 (glide) |
| l = z | both central partial voiced | **separated** | T: l=0 (approximant), z=+1 (fricative) |

### Remaining at shell 1
| Collision | Why | Correct? |
|---|---|---|
| l = r | both alveolar approximant voiced | YES — manner distinction (lateral vs rhotic) is shell 2 |
| k = ch | both back voiceless stop/affricate | YES — affricate = stop + fricative, finer distinction |
| g = j (dʒ) | both back voiced stop/affricate | YES — same reason |
| sh = h | both back voiceless fricative | MOSTLY — h is glottal (further back), shell 2 separates |
| f = th | both front voiceless fricative | YES — dental vs labiodental is a D precision issue |
| v = dh | both front voiced fricative | YES — same |
| a = i | both front sustained open voiced | YES — height (close vs open) is M precision, shell 2 |
| o = u | both back sustained open voiced | YES — same |
| k=c=q=x | all /k/ | YES — English has 4 letters for one phoneme |

Every remaining collision is between sounds that are genuinely
phonetically similar. Shell 2 extends the relevant dimension
to separate them. No incorrect collisions.


## Shell 2+ Extensions

Shell 2 uses extended coordinates (magnitude 2+) for finer distinctions:

| Feature | Dimension | Shell 1 | Shell 2 | Example |
|---|---|---|---|---|
| Vowel height | M | +1 (all vowels) | +2=close, +1=open | i(M=+2) vs a(M=+1) |
| Aspiration | Q | +1/-1 (voiced/voiceless) | +2=aspirated | pʰ(Q=-2) vs p(Q=-1) |
| Retroflexion | D | 0 (central) | +2=retroflex | ɖ(D=+2) vs d(D=0) |
| Pharyngealization | D | -1 (back) | -2=pharyngeal | ʕ(D=-2) |
| Tone (flat) | T | +1 (sustained) | +2=high flat | mā (T=+2) |
| Tone (falling) | T | +1 (sustained) | -2=falling | mà (T=-2) |
| Tone (rising) | T | +1 (sustained) | +3=rising | má (T=+3) |
| Tone (dip) | T | +1 (sustained) | -3=dipping | mǎ (T=-3) |
| Nasality | M | +1 (open) | -2=nasal vowel | ã (M=-2) |
| Length | T | +1 (sustained) | +4=extra long | aː (T=+4) |
| Click | D | — | +3=dental click | ǀ (D=+3) |
| Ejective | Q | -1 (voiceless) | -3=ejective | kʼ (Q=-3) |
| Creaky voice | Q | +1 (voiced) | +3=creaky | a̰ (Q=+3) |


## Cross-Language Coverage

| Script | Shell 1 handles | Shell 2 needed for | Shell 3 needed for |
|---|---|---|---|
| Latin (English) | most consonants, vowel front/back | vowel height, th/f, l/r | — |
| Greek | same sounds as Latin subset | θ/φ distinction | — |
| Cyrillic | same phonetic space | palatalized consonants | — |
| Arabic | basic consonant grid | pharyngeals, emphatics | — |
| Devanagari | stops, nasals, vowels | aspirates, retroflexes | — |
| Hangul | jamo components directly | tense/lax/aspirated 3-way | — |
| Japanese | kana = consonant + vowel | vowel length, palatalized | — |
| Mandarin | consonant + vowel | 4 tones (T extended) | — |
| Vietnamese | consonant + vowel | 6 tones (T extended) | — |
| Thai | consonant + vowel | 5 tones + vowel length | — |
| Zulu/Xhosa | basic consonants | click types (D extended) | — |
| Navajo | basic consonants | nasal vowels, tone | — |

Every language fits within shell 2. Shell 3 (7^4 = 2401 coordinates)
has room for all IPA symbols plus CJK syllable blocks.


## Case

Not in the coordinate. 'A' = 'a' at the character level.

Case is a property of TEXT not SOUND. When you speak, there is no
uppercase. Case is handled as:
- Walk-level property (position in sentence determines rendering)
- Metadata in the text loop (not in the wave byte)
- Shell 2 extension only if a system absolutely needs it in the coordinate

This is correct for all scripts. Chinese, Arabic, Hindi, Japanese,
Korean have no case. Latin, Greek, Cyrillic do. The encoding shouldn't
penalize 60% of scripts for a feature only 40% use.


## Properties Preserved from v1

| Property | v1 | v2 | Status |
|---|---|---|---|
| is_letter | M ≠ 0 | M ≠ 0 | same |
| is_vowel | M == +1 | M == +1 | same |
| is_stop | M == -1 AND T == -1 | M == -1 AND T == -1 | more precise (was just M==-1) |
| is_nasal | — | M == -1 AND T == +1 | NEW (stops vs nasals distinguished) |
| is_fricative | — | M == 0 AND T == +1 | NEW |
| is_glide | — | T == 0 | NEW |
| is_voiced | Q > 0 | Q > 0 | same |
| is_front | D > 0 | D > 0 | same |
| case_flip | XOR 0x80 | N/A (no case) | removed (walk-level) |
| voice_flip | XOR 0x02 | XOR 0x02 | same |
| stop↔nasal | — | XOR 0x80 (T flip) | NEW |


## Wave Byte Encoding

Same as v1. 2 bits per dimension, 1 byte total:

  bits [7:6] = T    [5:4] = D    [3:2] = M    [1:0] = Q
  00 = 0    01 = +1    10 = extended    11 = -1

Shell 1: 1 byte. Shell 2: 1 + N extension bytes (1 per extended dim).
On a ternary processor: fixed-width 4 trits, no shell distinction.


## Discrimination Count

| Encoding | Effective regions at shell 1 | Why |
|---|---|---|
| v1 (T=case) | ~18 (T=±1 for case, D×M×Q = 3×3×2) | T wasted on non-phonetic feature |
| v2 (T=duration) | ~36 (T×D×M×Q ≈ 2×3×3×2) | T contributes real discrimination |
| Theoretical max | 81 (3^4) | requires 3 equally populated values per dim |

v2 doubles the effective shell-1 discrimination by making T phonetically
meaningful. The gap to 81 is because T and Q are binary-ish (most sounds
are either sustained/transient, voiced/voiceless) rather than ternary.
Putting glides at T=0 helps (18→36 is the jump from T binary to T ternary).


## Open Questions for Review

1. **Tone encoding at shell 2**: The proposed T magnitude values for tones
   (+2=high, -2=falling, +3=rising, -3=dipping) are first-pass. Should
   tone magnitudes encode pitch HEIGHT or pitch MOVEMENT? Height is more
   like M (magnitude), movement is more like D/T (velocity). Needs
   phonological analysis.

2. **h placement**: Currently D=-1 (back). Phonetically h is glottal
   (the most back sound). Should it be D=-1 at shell 1 and D=-2 at
   shell 2 to separate from sh? Or is the collision acceptable?

3. **Vowel height at shell 2**: Proposed M=+2 for close vowels, M=+1
   for open vowels. But "close" vowels have LESS openness = LESS
   substance = should be LOWER M. Inversion issue. Maybe: M=+1=close
   (just barely open enough to be vowel), M=+2=open (very open)?
   The sign convention matters for born-index proximity.

4. **Silence**: (0,0,0,0) = identity = silence = space character.
   Confirmed: same as v1.

5. **Digits and symbols**: Not phonemes. Need a different coordinate
   scheme. Probably geometric/structural properties instead of
   articulatory. D=shape, M=complexity, Q=category? TBD.

6. **Bottom-up signal**: With all 4 dimensions consistent, the sum
   of character coordinates weakly predicts word meaning (~1-5% from
   sound symbolism). Is this actually useful for the born-index, or
   just a nice property? Needs measurement.
