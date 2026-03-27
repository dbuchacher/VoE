# Prior Work — Reference Tree

165+ sessions. 690 git commits. 93MB of git history.
The answers are already in here. Search before you build.

  corpus    → the earliest ideas (one file, where it all started)
  schism    → 148 sessions, 370 commits, 32MB git (Rust era)
  nomos     → 284 commits, 58MB git (assembly era)
  ToE       → 17 sessions, 36 commits, 3MB git (lattice era)

USE GIT HISTORY. Every decision, every mistake, every discovery.

  git log --oneline                         see what happened
  git log -p --grep="keyword"               find when something was discussed
  git show <hash>                           see exactly what changed
  git log --all --oneline | grep "font"     find font-related work

Don't guess. Don't re-derive. 2 seconds to search. Hours to reinvent.


## prior/corpus/corpus-2026-03-13.md — The Corpus

The earliest ideas. One file. Where everything started before any
code existed. The DEFINITIVE source for the sm architecture.

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


## prior/schism/ — Schism (370 commits, 32MB git)

The Rust era. sm daemon, ring buffers, GPU rendering, marketplace.
First implementations of: ring records, born-indexed, SDF fonts,
prime keys, query model, hardware convergence.

  drafts/shower-thoughts/         the idea vault
    architecture/                   ring rewrite, dissolved problems
    hardware/                       GPU protocol, boot, one-wire display
    agent-native/                   query dissolved, library inversion
    flux/                           network, neural storage, migration
    origin/                         realizations, absurdities, why now
    input/                          prime keys, emotional rendering

  drafts/lakoff-mapping.md        24 schemas → physics → computation
  drafts/history/stone-archaeology/  raw session transcripts (20MB of text)
  nomos/b3/                       the Rust kernel (bench, parts, traces)

  Session 87: original character encoding (0-25=A-Z, 32-57=a-z)
  Session 88: VGA font, atlas indexing
  Session 141-147: assembly rewrite, the stone


## prior/nomos/ — Earlier Nomos (284 commits, 58MB git)

Assembly builds before ToE. 23-bond system, parameter walks.
Lots of experiments in the git history.


## prior/ToE/ — Theory of Everything (36 commits, 3MB git)

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
