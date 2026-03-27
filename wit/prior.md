# Prior Work — Reference Tree

VoE stands on prior projects. They're in git, not going anywhere.
Reference them, don't copy. If you need content, read from source.

USE GIT HISTORY. The answers are in commits, diffs, and session logs.
`git log`, `git show`, `git log --all -p --grep="keyword"`. If you
can't find something in the files, it's in the git history. Every
decision, every mistake, every discovery — committed. Don't guess.
Don't re-derive. Read the history.

## /home/maxi/code/ToE/ — Theory of Everything

Where VoE came from. Sessions 15-17. The lattice OS, clean room rebuild,
genesis model, wave characters. Still has the git history.

  wit/findings/                 72 correspondences with Standard Model
    standard-model-checklist.md   the master checklist (72 confirmed, 0 contradictions)
    alpha137.md                   fine structure constant from lattice (6 sig figs)
    fill-ratio.md                 25% fill convergence (3 independent equations)
    bell.md                       Bell's theorem — lattice violates classical bound
    chirality.md                  quaternion handedness → L-amino acids
    dna.md                        genetic code mapping (4 bases = 4 dimensions)
    amino.md                      20 amino acids = C(6,3) = Riemann components
    periodic-table.md             element count from dimensional analysis
    completeness.md               functional completeness = confinement
    quantum.md                    QM from (M,Q) dimensions
    lattice-bonds.md              16 bond patterns verified
    pi.md                         π as fixpoint of geometric mean
    q-topology.md                 Q factor and complexity
    neural.md                     neural/pharmacology mappings
    superconductor.md             superconductor predictions
    water.md                      water structure from lattice
    ns-vortex.md                  Navier-Stokes from lattice
    millennium/                   millennium problems (P≠NP, Navier-Stokes, etc.)

  wit/drafts/
    genesis-model.md              how we got from system.asm to genesis
    font-arc.md                   font philosophy: same answer three times
    config-origin.md              configuration as lattice coordinates
    oil-synthesis.md              jet fuel catalyst pore diameter
    oil-pore-derivation.md        derivation of 568pm
    energy-density.md             energy density calculations

  wit/mind/
    mind                          the lattice — how everything works
    coder/design                  unified VoE architecture (the big one)
    coder/guide                   lattice language spec
    open-questions                unresolved questions

  wit/history/
    prior                         pre-ToE history (schism → nomos → ToE)
    session-15.md                 gates, font extraction, 2x text
    session-16.md                 .ha file type, lattice restructure
    session-17.md                 clean room, genesis, wave characters

  nomos/                          the code (bootloader + equation + genesis)
  nomos/nfg/                      graveyard (old system.asm, old walks, MBR)

  toe.md                          the paper (public-facing explanation)
  paper.md                        extended paper

  lattice/codex                   reference table — grep for formulas
  lattice/lattice                 all coordinates — grep for tuples
  lattice/gen.c                   lattice generator


## /home/maxi/schism/ — Schism (148 sessions)

The Rust era. sm daemon, ring buffers, GPU rendering, marketplace.
Where the core ideas were first implemented: ring records, born-indexed,
SDF fonts, prime keys, query model, hardware convergence.

  drafts/shower-thoughts/         the idea vault
    architecture/                   ring rewrite, dissolved problems
    hardware/                       GPU protocol, boot, one-wire display
    agent-native/                   query dissolved, library inversion
    flux/                           network, neural storage, migration
    philosophy/                     two worlds
    origin/                         realizations, absurdities, why now
    input/                          prime keys, emotional rendering
    identity/                       wireless security

  drafts/lakoff-mapping.md        24 schemas → physics → computation
  drafts/standard-model-checklist.md  original SM mapping

  nomos/b3/                       the Rust kernel (bench, parts, traces)
  genesis/                         assembler, sort, video gen

  git log has 148 sessions of history.


## /home/maxi/brain/corpus/ — The Corpus

Key sections (line numbers for corpus-2026-03-13.md):
  154   Prime Keys — semantic primitives, 20-30x throughput
  323   Emotional Rendering — non-verbal as data
  557   Kill Font Libraries — baked glyph atlas, SDF
  622   smfs — born-indexed, query model, grep dissolved
  677   Disk layout — raw smfs, no filesystem
  2453  Ring primitive record types (7 topologies)
  2741  Text System — atlas indices, u16, not Unicode
  2764  Everything is Numbers — fixed-width, L1 speed
  2847  Text performance table (O(1) vs O(n))


## /home/maxi/code/nomos/ — Earlier Nomos

Assembly builds before ToE. 23-bond system, parameter walks.
Reference for what was tried. Not active.


## /home/maxi/brain/corpus/ — The Corpus

Accumulated design knowledge. The DEFINITIVE source for sm
architecture decisions, shower thoughts, benchmarks, and the
ideas that became VoE. If it was discussed, it's here.

  corpus-2026-03-13.md    latest snapshot (most complete)

Key sections (line numbers for corpus-2026-03-13.md):
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

These files also have git history at /home/maxi/brain/.
The corpus IS the shower thoughts + specs + benchmarks + decisions
that shaped everything. Read it before reinventing anything.
