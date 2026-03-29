# Bind: Quantum Computing is `.and_then()`

You already write quantum code. You just don't know it.

## The 30-Second Version

```rust
// classical: read at every step
let a = step_1()?;    // read, collapse
let b = step_2(a)?;   // read, collapse
let c = step_3(b)?;   // read, collapse

// quantum: chain blind, read once at the end
let c = step_1()
    .and_then(step_2)  // don't look
    .and_then(step_3); // don't look

match c { ... }        // NOW look. one read. one collapse.
```

Same result. Same answer. The first version reads three times.
The second reads once. That's the entire difference between
a classical computer and a quantum computer.

**Classical = `?` after every step. Quantum = `.and_then()` chain.**


## What Are These Operations?

Four. That's it.

```
  P  =  read/write     =  ? (early return, force a value)
  C  =  transform      =  .map() / .and_then() (apply a function)
  R  =  repeat          =  .iter() / loop (keep going)
  W  =  test            =  Ok/Err, Some/None (did it work?)
```

Classical computation uses all four at every step: P+C+R+W.
Read, transform, repeat, test. Every cycle. Every instruction.

Quantum computation uses three: C+R+W. Transform, repeat, test.
**No P. No reading. No collapsing.** Until the end.

```
  classical = P+C+R+W = hylo  = read at every step
  quantum   = C+R+W   = bind  = read once at the end
```

Rust programmers: you call this `and_then`. Haskell calls it `>>=`.
JavaScript calls it `.then()` on a Promise. The quantum physics
community calls it "unitary evolution." It's all bind.


## Why Qubits Die

A qubit is a value inside an `.and_then()` chain that nobody
has read yet.

```rust
let qubit = step_1()
    .and_then(step_2)
    .and_then(step_3);

// qubit exists. answer is IN there. but nobody's looked.
// it's in superposition: all possible values simultaneously.
// because nobody called match/unwrap/? on it yet.
```

Decoherence = something reads the qubit before you're ready.

```rust
let qubit = step_1()
    .and_then(step_2)
    .inspect(|x| println!("{x}"))  // OOPS. you looked. wave collapsed.
    .and_then(step_3);             // too late. it's classical now.
```

That `println!` is a thermal photon bumping into your qubit.
An air molecule reading its state. A vibration forcing a value.
Any read = P = collapse = your quantum chain is now classical.

That's why quantum computers need millikelvin cooling. Not
because cold is magic. Because cold = fewer things around
to accidentally `.inspect()` your qubit.


## How Light Does It

A photon is a value that chains through spacetime without
anyone reading it.

Mathematically: `T^(½i) × D^(½i)` — imaginary exponents in
time and distance. Plug this into any language with complex
numbers. You get a plane wave. Light.

Light `.and_then()`s through space. It bounces off mirrors.
It interferes with other light. It transforms at every
surface. All without P. Nobody reads the photon's state
until a detector fires.

```
  photon in flight:    .and_then().and_then().and_then()...
  detector fires:      match { ... }
```

That's why photons make the best qubits. They're NATURALLY
bind. They don't interact with most matter. Nothing
accidentally `.inspect()`s them. They chain for billions
of kilometers without a single P.

Your eyeball is the `match` statement. The photon traveled
from a star for 1000 years of `.and_then()`. Your retina
fires. P. One read. Wave collapses. You see a dot.


## A Crystal That Runs Bind

Tantalum-181 crystal. 1mm. $500.

```
  photon enters crystal  →  .and_then(scatter off nucleus)
                         →  .and_then(scatter off nucleus)
                         →  .and_then(scatter off nucleus)
                         →  ... 10,000,000,000,000 times (Q = 10¹³)
                         →  photon exits
                         →  match { detector fires }  // P. one read.
```

The crystal is a resonant cavity. Photons bounce inside it
10 TRILLION times before escaping. Each bounce is a
`.and_then()` — a transform without reading. The Mössbauer
resonance means only the exact right frequency survives
(that's the W test — `Ok` for resonant, `Err` for off-frequency).

10¹³ steps of bind. In a crystal. At room temperature.
No cryogenics. No vacuum. No billion-dollar lab.

And: 3,000,000 nuclei in the beam path all contribute
IN PHASE. That's `.and_then()` across 3 million parallel
paths that constructively interfere. When paths agree:
signal amplifies (N² = 9 × 10¹²). When they disagree:
signal cancels. That's quantum interference. In a rock.


## Loading a Program

The crystal doesn't know what you're computing. It runs
photons in loops. The program is the input signal.

### Program 1: Zero-Point Energy

```rust
// pump photons until field exceeds Schwinger threshold
let energy = photon_source()
    .and_then(|p| cavity_amplify(p))       // N² coherent boost
    .and_then(|p| accumulate_field(p))      // build to critical
    .and_then(|f| schwinger_pairs(f));      // vacuum → particles

match energy {
    Ok(pairs)  => collect_energy(pairs),
    Err(below) => pump_more_photons(),
}
```

Output: electron-positron pairs from vacuum. Energy.

### Program 2: Factor a Number

```rust
// pump photons, encode modular exponentiation in phase
let factors = photon_source()
    .and_then(|p| encode_base(p, a))        // set up a^x mod N
    .and_then(|p| cavity_iterate(p, n))     // 10¹³ cycles = frequency sweep
    .and_then(|p| interference_peaks(p));   // QFT from crystal structure

match factors {
    Ok(period) => {
        let s = n + 1 - 2 * period;        // formula: p+q = N+1-kL
        let p = (s - sqrt(s*s - 4*n)) / 2;
        let q = n / p;
        println!("{n} = {p} × {q}");
    }
    Err(_) => try_different_base(),
}
```

Output: the factors of N. Same crystal. Different input.

### Program 3: Satoshi's Wallet

```rust
// same as program 2 but on secp256k1 curve order
let private_key = photon_source()
    .and_then(|p| encode_curve_point(p, public_key))
    .and_then(|p| cavity_iterate(p, curve_order))
    .and_then(|p| interference_peaks(p));

match private_key {
    Ok(k) => transfer_bitcoin(k, my_wallet),
    Err(_) => need_bigger_crystal(),
}
```

Output: Satoshi's private key. Same crystal. Bigger crystal
for bigger numbers. Tantalum is $150/kg.


## Why This Works (And Classical Can't)

Classical computer: `?` at every clock cycle. Every register
read is a P. Every memory access is a P. 2^128 steps means
2^128 reads. Each read takes one clock cycle. Total: 10¹⁹ years.

Photon in crystal: `.and_then()` for 10¹³ cycles. No P. The
photon holds superposition the entire time. ALL values of the
computation exist simultaneously inside the cavity. Interference
kills wrong answers, amplifies right answers. One P at the end.

```
  classical:  2^128 × P         = heat death of universe
  crystal:    10¹³ × and_then   = seconds
```

Not because the crystal is faster per step. Because it doesn't
READ between steps. Each `.and_then()` preserves the full
superposition. Each `?` destroys it.

The `?` operator is the most expensive operation in the universe.
Every time you read, you destroy all the parallel answers except one.
Quantum computing = stop using `?`. Use `.and_then()`. Read once.


## Rosetta Stone

```
  physicist says          programmer says         lattice says
  ──────────────          ──────────────          ────────────
  superposition           unmatched Result        wave (unread)
  wave collapse           ? / unwrap              +P (read)
  unitary evolution       .and_then()             bind (+C+R+W)
  decoherence             accidental .inspect()   unwanted P
  entanglement            shared reference        non_local
  measurement             match                   hylo (+P+C+R+W)
  quantum computer        chain that won't ?      bind machine
  classical computer      ? at every step         hylo machine
  qubit                   Option<T>               wave(fiat)
  gate                    .map(f)                 bond (C)
  circuit                 iterator chain          path (walk)
  error correction        Result<T, E>            W (test)
  interference            fold with cancellation  complex phase
  photon                  value in flight         T^(½i)×D^(½i)
  decoherence time        chain length before ?   Q factor
```

Three fields. Same concepts. Different jargon.


## The Point

Quantum computing isn't exotic. It's not mysterious. It's not
about parallel universes or cats in boxes.

It's `.and_then()`.

Chain operations without reading. Let interference do the work.
Read once at the end. That's it.

Every Rust programmer who ever wrote an `Option` chain did quantum
computing on classical hardware. The chain is bind. The `match`
is measurement. The `.inspect()` bug is decoherence.

The expensive part isn't the math. It's building hardware that
can chain 10¹³ operations without accidentally reading. Silicon
transistors can't — electrons force a read at every gate. Photons
in a crystal can — they bounce 10¹³ times without collapsing.

$500 of tantalum. $5000 of x-ray source. `.and_then()` × 10¹³.
`match` once. Answer.

The quantum computer isn't coming. Programmers already have the
software pattern. The crystal already exists. The photon source
already exists. The only thing missing is connecting them.

```rust
let quantum_computer = photon_source()
    .and_then(crystal_cavity)
    .and_then(your_program);

match quantum_computer {
    Ok(answer) => answer,     // the universe computed it for you
    Err(_) => bigger_crystal(),
}
```
