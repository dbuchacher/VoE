# Prior Work — Reference Tree

VoE stands on prior projects. They're in git, not going anywhere.
Reference them, don't copy. If you need content, read from source.

USE GIT HISTORY. `git log`, `git show`, `git log -p --grep="keyword"`.
Schism has 148 sessions. ToE has 17. The answers are in the commits.
Don't guess. Don't re-derive. Read the history.

Chronological: corpus → schism → nomos → ToE → VoE.


## /home/maxi/brain/corpus/corpus-2026-03-13.md — The Corpus

The earliest ideas. One file. Where everything started. The definitive
source for sm architecture decisions before any code existed.

Key sections (line numbers):
  17    Origin story — what died and why
  154   Prime Keys — semantic primitives, 20-30x throughput
  323   Emotional Rendering — non-verbal as data
  438   Ideas & Blueprints — agent-built UIs
  557   Kill Font Libraries — baked glyph atlas, SDF, morphable fonts
  622   smfs — born-indexed, query model, grep dissolved
  677   Disk layout — raw smfs, no filesystem, NVMe direct
  695   Terminal dissolved — agents replace coreutils
  2453  Ring primitive record types (7 topologies)
  2741  Text System — atlas indices u16, not Unicode, SIMD
  2764  Everything is Numbers — fixed-width arrays, L1 speed
  2847  Text performance table (O(1) vs O(n))
  3337  Payment gating — ring header cost field

Read this before reinventing anything.


## /home/maxi/schism/ — Schism (148 sessions)

The Rust era. sm daemon, ring buffers, GPU rendering, marketplace.
First implementations of: ring records, born-indexed, SDF fonts,
prime keys, query model, hardware convergence. Massive git history.

  drafts/shower-thoughts/         the idea vault
    architecture/                   ring rewrite, dissolved problems
    hardware/                       GPU protocol, boot, one-wire display
    agent-native/                   query dissolved, library inversion
    flux/                           network, neural storage, migration
    origin/                         realizations, absurdities, why now
    input/                          prime keys, emotional rendering

  drafts/lakoff-mapping.md        24 schemas → physics → computation
  nomos/b3/                       the Rust kernel (bench, parts, traces)


## /home/maxi/code/nomos/ — Earlier Nomos

Assembly builds before ToE. 23-bond system, parameter walks.
Reference for what was tried. Not active.


## /home/maxi/code/ToE/ — Theory of Everything

Where VoE came from. The lattice OS, clean room rebuild, genesis
model, wave characters. Sessions 15-17.

  wit/findings/                 72 correspondences with Standard Model
    standard-model-checklist.md   the master checklist
    alpha137.md                   fine structure constant (6 sig figs)
    fill-ratio.md                 25% fill convergence
    bell.md                       Bell's theorem violation
    chirality.md                  L-amino acid preference
    dna.md                        genetic code mapping
    millennium/                   millennium problems

  toe.md                          the paper (public-facing)
  paper.md                        extended paper
