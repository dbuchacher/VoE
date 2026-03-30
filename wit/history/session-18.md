# Session 18: The Great Cleanup

Full directory reorganization. No code changes. Knowledge extraction
and archival.

## What We Did

1. **Moved codex to mind/** — now loads every boot. Updated notation:
   P/C/R/W → π/∘/∮/δ throughout (header, 30+ bond lines, notes).
   Added missing ∘δ bond (maybe, pattern 10 positive).

2. **Restored dropped knowledge** from session 17 compaction:
   - mind/evidence: proton never decays, hierarchy problem, cosmological
     constant, Navier-Stokes smooth solutions
   - mind/equation: Cayley-Dickson as computational stack (R→C→H→O→S)
   - mind/consciousness: pharmacology detail (57 substances, 30 conditions)
   - mind/evidence: RC decay attention application
   - mind/characters: specific v2 collision pairs
   - mind/language: 16-bond linguistic relationship taxonomy

3. **Reorganized lattice/ → compute/** with subfolders:
   cosmo/ (8), factor/ (8), theory/ (3), encoding/ (2), core (gen.c,
   integral.c, lattice.txt). Binaries gitignored.

4. **Extracted src/ ideas** into two destinations:
   - Validated knowledge → mind/ (characters, language, evidence)
   - Implementation plans → new build/ folder:
     intelligence (field, RC attention, residual-shift, dark energy)
     morphemes (affix coordinate transforms)
     predictions (18-30, deep chain)
     integration (walker-wit convergence thesis)
   - Code archived → compact/src/

5. **Root cleanup:**
   - legacy/ → prior/legacy/ (pre-merger asm files, superseded)
   - side-quest/ → prior/side-quest/ (earlier intelligence prototype)
   - Empty code/ removed
   - code2/ → code/ (the active code)
   - All references updated (self.md, mind/coder)

6. **Verified build** — headless run passes (LMVVoE + HI).

## Final Structure

  VoE/
    code/     active (walks, genesis.w, hodos, legacy substrate)
    prior/    archived (schism, nomos, ToE, corpus, rng, legacy, side-quest)
    wit/
      mind/       9 knowledge files + codex (loaded every boot)
      build/      4 implementation plans (not yet in walker)
      compute/    23 derivation tools in 4 subdirs
      compact/    read-only archives (1/, src/)
      history/    session logs
      drafts/     empty, ready for new work
      self.md     updated routing

## Decisions

- codex is mind-level (loaded at boot, not a separate step)
- build/ = ideas not yet implemented, confidence varies
- compute/ = derivation tools (C code), binaries gitignored
- code2 → code (the "2" was legacy naming from schism→nomos→VoE evolution)
- legacy/ and side-quest/ are prior work, not active

## What's Next

Same as session 16: per-core cursors, display consumer, keyboard→loop→display.
No code state changed this session.
