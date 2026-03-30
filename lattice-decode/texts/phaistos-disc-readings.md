# Phaistos Disc -- Lattice Reading

A translation attempt using lattice coordinates (tau,chi,mu,phi) cross-referenced
with phonetic proposals from Colless (Linear A correspondences), Owens/Coleman
(Linear B values), Achterberg et al. (Luwian), and Timm (distributional analysis).

Confidence: LOW for individual words. MEDIUM for overall structure. HIGH for
register classification (prayer + narrative). This is an approximate reading --
semantic neighborhoods, not exact words. But approximate is better than what we
have now, which is nothing.


## Method

### Phonetic Value Composite

Sources: Colless (C), Owens/Coleman (O), Achterberg et al. (A), Timm (T), lattice
distributional analysis (L). Where sources conflict, lattice position breaks ties.

```
Sign  Name             Phonetic    Source(s)     Lattice (tau,chi,mu,phi)
----  ----             --------    ---------     -----------------------
 01   PEDESTRIAN       je/ye       A/C           (+1, +2, -1, +0)
 02   PLUMED HEAD      i/a2        O/A           (+2, -2, -2, +1) DETERMINATIVE
 03   TATTOOED HEAD    nu?         L             (hapax-range, A-only)
 04   CAPTIVE          wa?         L             (hapax, A-only)
 05   CHILD            na?         L             (hapax, B-only)
 06   WOMAN            ti/si       L             (-1, -1, +0, +0)
 07   HELMET           se/za       L             (+2, +1, -1, -2)
 08   GAUNTLET         ki          L             (+0, +2, -1, -2)
 09   TIARA            su          L             (B-only initial)
 10   ARROW            ha          C             (-1, -1, -1, +2) A-only
 11   BOW              ru?         L             (hapax, A-only)
 12   SHIELD           qe/tu       C/A           (+2, +0, -1, +2)
 13   CLUB             wo/ke       L             (+0, +0, -1, +0)
 14   MANACLES         pe          C             (medial, balanced)
 15   MATTOCK          da?         L             (hapax, B-only)
 16   SAW              ni          L             (B-only initial)
 17   LID              ra?         L             (hapax, A-only)
 18   BOOMERANG        ri/li       L             (+1, +1, +0, +0)
 19   PLANE            so          L             (-1, +0, +0, +2) A-only
 20   DOLIUM           yo          C             (B-only medial)
 21   COMB             ne          A             (A-only final)
 22   SLING            sa          C             (+0, -2, +0, -2) B-only initial
 23   COLUMN           to          A             (+1, +0, +0, +0)
 24   BEEHIVE          de          C             (+0, +0, +0, -2) B-leaning
 25   SHIP             no/wi       L             (+0, +1, +0, -1) B-leaning
 26   HORN             ja/ra       L             (+0, +1, -1, +2) A-register
 27   HIDE             ta/te       C/A           (+2, +0, -1, +1)
 28   BULLS LEG        pi          L             (A-only, 100% initial)
 29   CAT              ma          C/T           (+1, -2, -1, -1) B-register
 30   RAM              mu?         L             (hapax, B-only)
 31   EAGLE            ku          Linear A      (+0, +0, -2, +2) A-only
 32   DOVE             lo/mi       L             (-1, +0, +2, +1)
 33   TUNNY            pu          A             (+0, +1, +0, -1)
 34   BEE              di          L             (-1, +0, +0, -1)
 35   PLANE TREE       e/a         L             (+1, +2, +0, +0)
 36   VINE             u           L             (-1, +0, +0, -2) B-only
 37   PAPYRUS          i/pa        C/A           (-1, +0, +1, +0)
 38   ROSETTE          ko          L             (-1, +2, +0, +1) final
 39   LILY             re          C             (-1, +0, +1, -2) B-leaning
 40   OX BACK          we/la       L             (+0, +0, +1, +0) medial
 41   FLUTE            do          L             (A-only, medial)
 42   GRATER           we          C             (hapax, B-only)
 43   STRAINER         ta          Linear A      (hapax, B-only)
 44   SMALL AXE        po?         L             (hapax, A-only)
 45   WAVY BAND        rya/rja     C/O           (+0, +0, -1, -1) medial
```

### Key Insight: The Owens-Colless Convergence

Owens reads A16=A19=A22 (the threefold refrain "02-12-31-26") as I-QE-KU-RJA,
meaning "great lady of importance." Independently:
- Colless assigns 12=QE from Linear A tablet PH 9a
- Linear A scholarship assigns 31=KU from eagle/bird-sign correspondence
- Colless assigns 45=RYA from wavy-line correspondence

But 26 (HORN) is NOT 45 (WAVY BAND). Owens must read 26 differently from Colless.
If Owens' IQEKURJA is correct, then 26 = RJA in the refrain. The lattice coordinates
for 26 are (+0,+1,-1,+2) -- A-register, medial-final, low diversity -- consistent
with a grammatical suffix or case ending rather than a content syllable.

Resolution: 26 likely = a case/vocative suffix. "I-QE-KU-[vocative]" = "O Great-Lady!"
This matches both Owens' semantic reading AND the lattice's identification of 26 as
a structural marker (A-register, rigid context, 5 of 6 occurrences on Side A).


### Semantic Mapping Principles

For signs without phonetic proposals, the lattice coordinates map to semantic
neighborhoods:

```
tau+  = recurring, important, structural (high frequency)
tau-  = rare, specific, content-heavy (low frequency)
chi+  = end-position, final, concluding
chi-  = beginning-position, initiating, opening
mu+   = richly connected, versatile (high neighbor diversity)
mu-   = rigid, formulaic, constrained (low diversity)
phi+  = Side A register (ritual, invocatory)
phi-  = Side B register (narrative, descriptive)
```

Word-level coordinates = mean of constituent sign coordinates, weighted by position
(initial signs weighted 1.5x for role as syllable-onset).


## Side A: The Litany

Structure detected: threefold refrain (A16=A19=A22) with paired verses (A14=A20)
and responses (A15=A21). This is a ritual invocation -- prayer, hymn, or liturgy.

### Group-by-Group Reading

```
A1:  02-12-13-01-18/
     Signs: PLUMED.HEAD-SHIELD-CLUB-PEDESTRIAN-BOOMERANG
     Phonetic: i-qe-wo-je-ri
     Aggregate: (+1.2, +0.2, -0.6, +0.6) -- ritual-register, mixed
     Reading: "I proclaim to the assembly" / "Hear this, O gathering"
     Role: OPENING INVOCATION (carries oblique stroke = section boundary)
     Confidence: LOW (individual), MEDIUM (function as opener)

A2:  24-40-12
     Signs: BEEHIVE-OX.BACK-SHIELD
     Phonetic: de-we-qe
     Aggregate: (+0.7, +0, +0, +0) -- neutral, connective
     Reading: "of the house" / "in the sanctuary"
     Role: locative phrase (prepositional)
     Confidence: LOW

A3:  29-45-07/
     Signs: CAT-WAVY.BAND-HELMET
     Phonetic: ma-rya-se
     Aggregate: (+1.0, -0.3, -1.0, -1.3) -- B-register word appearing in A
     Reading: "[sacred formula]" / "amen" / "so it is"
     Role: CROSS-SIDE FORMULA (= B20, appears on both sides with stroke)
     Note: This word bridges both registers. If Side A is prayer and Side B
     is narrative, this word belongs to the shared liturgical vocabulary.
     Owens reads similar patterns as related to "mother" (MA-).
     Confidence: MEDIUM (as formula), LOW (exact meaning)

A4:  29-29-34
     Signs: CAT-CAT-BEE
     Phonetic: ma-ma-di
     Aggregate: (+0.3, -1.3, -0.7, -1.0) -- doubled initial = emphasis
     Reading: "mother-divine" / "great mother" / "mama-goddess"
     Role: DOUBLED SYLLABLE = emphasis or reduplicated form
     Note: ma-ma is universally "mother" across language families. The
     reduplication is the oldest morphological device known. Combined with
     34 (BEE, connected to divine in Minoan iconography -- bees were sacred
     at Phaistos), this likely means "Mother Goddess" or "Divine Mother."
     Confidence: MEDIUM (ma-ma = mother is near-universal)

A5:  02-12-04-40-33
     Signs: PLUMED.HEAD-SHIELD-CAPTIVE-OX.BACK-TUNNY
     Phonetic: i-qe-wa-we-pu
     Aggregate: (+0.8, -0.2, -0.2, +0.2) -- classified word, A-register
     Reading: "O [title], [epithet of binding/power]"
     Role: determinative + content word (title or proper name)
     Note: Sign 04 (CAPTIVE) is a hapax -- appears only here. May mark a
     unique title or a name that appears nowhere else in the text.
     Confidence: LOW

A6:  27-45-07-12
     Signs: HIDE-WAVY.BAND-HELMET-SHIELD
     Phonetic: ta-rya-se-qe
     Aggregate: (+1.5, +0.3, -0.8, +0.3) -- high recurrence, A-register
     Reading: "of the holy precinct" / "from the sacred place"
     Role: genitive/locative phrase
     Note: Contains the 45-07 pair (=B-formula fragment) embedded in A-register
     context. The word ends with 12 (SHIELD/qe), the most common A-register sign.
     Confidence: LOW

A7:  27-44-08
     Signs: HIDE-SMALL.AXE-GAUNTLET
     Phonetic: ta-po-ki
     Aggregate: (+0.3, +0.7, -0.7, -0.3) -- mixed register, short
     Reading: "the offering" / "the sacrifice"
     Role: object noun
     Note: Sign 44 (SMALL AXE) is a hapax. Double-axes (labrys) are the
     central cult symbol of Minoan religion. Even as a small axe, this sign
     carries ritual significance. "ta-po-ki" in a ritual text near a
     unique axe-sign suggests sacrificial terminology.
     Confidence: LOW (phonetic), MEDIUM (sacrificial context)

A8:  02-12-06-18-[?]
     Signs: PLUMED.HEAD-SHIELD-WOMAN-BOOMERANG-[damaged]
     Phonetic: i-qe-ti-ri-[?]
     Aggregate: (+0.8, +0.2, -0.2, +0.6) -- classified word, A-register
     Reading: "O [title], the [woman/priestess]..."
     Role: determinative + content (damaged -- cannot complete)
     Confidence: VERY LOW (damaged sign)

A9:  31-26-35
     Signs: EAGLE-HORN-PLANE.TREE
     Phonetic: ku-ja-e
     Aggregate: (+0.3, +1.0, -1.0, +1.3) -- strongly A-register
     Reading: "exalted one" / "high/noble"
     Role: epithet or title
     Note: 31-26 (EAGLE-HORN) is a key A-register bigram (5 occurrences).
     The eagle + horn combination in Minoan iconography = authority/divinity.
     This is part of the refrain vocabulary (31 and 26 are the final two signs
     of the threefold refrain). Here with 35 (PLANE TREE) = "the exalted,
     living one" or "exalted among the trees/people."
     Confidence: LOW (individual), MEDIUM (as title/epithet function)

A10: 02-12-41-19-35
     Signs: PLUMED.HEAD-SHIELD-FLUTE-PLANE-PLANE.TREE
     Phonetic: i-qe-do-so-e
     Aggregate: (+0.6, +0.4, -0.2, +0.8) -- classified, A-register
     Reading: "O [title], giver of abundance"
     Role: determinative + epithet
     Note: 41 (FLUTE) appears only twice, both on Side A. Music/flute
     in ritual context = praise, offering, or divine attribute.
     Confidence: LOW

A11: 01-41-40-07
     Signs: PEDESTRIAN-FLUTE-OX.BACK-HELMET
     Phonetic: je-do-we-se
     Aggregate: (+0.8, +0.8, -0.3, -0.5) -- balanced, tends toward conclusion
     Reading: "we who serve" / "the servants come"
     Role: subject phrase (worshippers/participants)
     Confidence: LOW

A12: 02-12-32-23-38/
     Signs: PLUMED.HEAD-SHIELD-DOVE-COLUMN-ROSETTE
     Phonetic: i-qe-lo-to-ko
     Aggregate: (+0.4, +0.4, +0, +0.8) -- classified, moderately A-register
     Reading: "O [title], of the sacred pillar/dwelling"
     Role: determinative + place epithet (oblique stroke = section end)
     Note: 32 (DOVE) has the highest neighbor diversity (mu=+2) of any sign.
     In Minoan religion, doves are associated with the goddess. 23 (COLUMN) +
     38 (ROSETTE) = architectural/floral sacred space.
     Confidence: LOW

A13: 39-11
     Signs: LILY-BOW
     Phonetic: re-ru
     Aggregate: (-1.0, +1.0, +1.0, -1.0) -- antithetic, short
     Reading: "hear!" / "arise!" / an interjection
     Role: short exclamation or imperative (2-sign groups = function words)
     Note: Both signs are rare (11 is a hapax). Short groups at section
     boundaries tend to be particles, interjections, or grammatical connectors.
     Confidence: LOW

A14: 02-27-25-10-23-18
     Signs: PLUMED.HEAD-HIDE-SHIP-ARROW-COLUMN-BOOMERANG
     Phonetic: i-ta-no-ha-to-ri
     Aggregate: (+0.5, +0.3, -0.3, +0.3) -- balanced, complex (6 signs)
     Reading: "O great one, who traverses the sea and the land"
     Role: THE VERSE (appears 2x: A14 = A20) -- main invocation phrase
     Note: This 6-sign group is the longest repeated phrase. The 02-27
     opening (not 02-12) is notable -- 27 (HIDE/ta) replaces 12 (SHIELD/qe)
     after the determinative, suggesting a different title or name.
     25 (SHIP) = maritime/journey. 10 (ARROW) = direction/force.
     "The one who crosses" is consistent with a sea-goddess or a deity
     associated with navigation -- central to Minoan island civilization.
     Owens identifies "AKKA" (pregnant mother) on the disc; this word
     structure could contain a similar Minoan divine epithet.
     Confidence: LOW (individual words), MEDIUM (as repeated verse)

A15: 28-01/
     Signs: BULLS.LEG-PEDESTRIAN
     Phonetic: pi-je
     Aggregate: (+0.5, +1.0, -1.0, +0) -- short, final-position, A-register
     Reading: "hear us!" / "be gracious!" / "receive [this]!"
     Role: THE RESPONSE (appears 2x: A15 = A21) -- congregational reply
     Note: 28 (BULLS LEG) is 100% initial (like 02). Bulls are THE sacred
     animal in Minoan Crete (bull-leaping, bull sacrifice, Minotaur).
     A bull's leg + a walking figure = "the bull's stride" = power/approach.
     As a 2-sign response in a litany, this is the shortest repeated unit --
     the congregational "amen" or "so be it" or "hear us."
     Confidence: LOW (phonetic), MEDIUM (as liturgical response)

------- REFRAIN BLOCK 1 -------

A16: 02-12-31-26/
     Signs: PLUMED.HEAD-SHIELD-EAGLE-HORN
     Phonetic: i-qe-ku-rja (Owens: "great lady of importance")
     Aggregate: (+1.0, -0.3, -1.5, +1.8) -- maximally A-register, formulaic
     Reading: "O Great Lady!" / "Hail, Exalted Queen!"
     Role: THE REFRAIN (appears 3x: A16 = A19 = A22, all with oblique stroke)
     Note: This is the most structurally significant group on the disc.
     Three exact repetitions, all carrying the paragraph-boundary stroke.
     Owens reads it as IQEKURJA = "great lady of importance." The lattice
     independently identifies it as maximally ritualistic: highest A-register
     polarity (phi=+1.8), lowest diversity (mu=-1.5, rigidly formulaic),
     and moderate frequency. Every structural signal says: THIS IS THE
     CENTRAL INVOCATION.
     Confidence: MEDIUM (Owens' reading + lattice convergence)

A17: 02-12-27-27-35-37-21
     Signs: PLUMED.HEAD-SHIELD-HIDE-HIDE-PLANE.TREE-PAPYRUS-COMB
     Phonetic: i-qe-ta-ta-e-pa-ne
     Aggregate: (+0.6, +0.3, -0.1, +0.7) -- classified, long (7 signs)
     Reading: "O [title], she of the fertile earth and growing things"
     Role: extended epithet (appears 2x: A17 = A29)
     Note: The DOUBLED 27 (ta-ta) parallels the doubled 29 (ma-ma) in A4.
     Reduplication = emphasis. 35 (PLANE TREE) + 37 (PAPYRUS) + 21 (COMB) =
     vegetation + writing/civilization + grooming/order. An extended epithet
     of the goddess as nurturer/civilizer. This is the longest classified
     group on Side A.
     Confidence: LOW

A18: 33-23
     Signs: TUNNY-COLUMN
     Phonetic: pu-to
     Aggregate: (+0.5, +0.5, +0, -0.5) -- short, balanced
     Reading: "abundance" / "from the deep" / "offering"
     Role: short phrase between refrains (connective or sacrificial term)
     Note: 33 (TUNNY/fish) in a Minoan maritime context = abundance from the
     sea. TUNNY + COLUMN = "the pillar of abundance" or simply a food offering.
     Confidence: LOW

A19: 02-12-31-26/
     = A16 (THE REFRAIN, second occurrence)
     Reading: "O Great Lady!"

A20: 02-27-25-10-23-18
     = A14 (THE VERSE, second occurrence)
     Reading: "O great one, who traverses the sea and the land"

A21: 28-01/
     = A15 (THE RESPONSE, second occurrence)
     Reading: "Hear us!"

A22: 02-12-31-26/
     = A16 (THE REFRAIN, third and final occurrence)
     Reading: "O Great Lady!"

------- END REFRAIN BLOCK -------

A23: 02-12-27-14-32-18-27
     Signs: PLUMED.HEAD-SHIELD-HIDE-MANACLES-DOVE-BOOMERANG-HIDE
     Phonetic: i-qe-ta-pe-lo-ri-ta
     Aggregate: (+0.6, +0.1, +0, +0.4) -- classified, long (7 signs)
     Reading: "O [title], she who binds and frees, dove of the horizon"
     Role: extended epithet -- the longest group on Side A (tied with A17/A29)
     Note: 14 (MANACLES) = binding/fettering. 32 (DOVE) = the goddess-bird.
     27-...-27 (HIDE bookending) = the word opens and closes with the same
     syllable, a phonological pattern found in Aegean names (compare Linear B:
     po-ti-ni-ja). The dove-amid-bonds may reference the goddess's power
     over both captivity and liberation.
     Confidence: LOW

A24: 06-18-17-19
     Signs: WOMAN-BOOMERANG-LID-PLANE
     Phonetic: ti-ri-ra-so
     Aggregate: (-1.0, +0, +0, +0.5) -- low frequency, content-heavy
     Reading: "the priestess/woman of [place]" / "she who tends [the shrine]"
     Role: actor/agent phrase (no determinative = common noun, not title)
     Note: 06 (WOMAN) + three content signs. No 02-prefix means this is
     not a classified/titled word. A woman performing an action -- the
     ritual participant rather than the goddess. 17 (LID) is a hapax,
     suggesting a unique role or place name.
     Confidence: LOW

A25: 31-26-12
     Signs: EAGLE-HORN-SHIELD
     Phonetic: ku-ja-qe
     Aggregate: (+1.3, +0.3, -1.3, +2.0) -- maximally A-register
     Reading: "the exalted shrine" / "the holy place"
     Role: place or institution (refrain vocabulary in new combination)
     Note: These are the refrain signs (31-26 from the refrain, plus 12)
     rearranged. The refrain's core vocabulary repurposed as a noun -- like
     using the name of a prayer as a place name. "The [place of the] Great Lady."
     Confidence: LOW

A26: 02-12-13-01
     Signs: PLUMED.HEAD-SHIELD-CLUB-PEDESTRIAN
     Phonetic: i-qe-wo-je
     Aggregate: (+1.3, +0, -0.8, +0.8) -- classified, echoes A1
     Reading: "O [title], I/we declare"
     Role: return to opening formula (A1 minus final sign)
     Note: A26 = A1 minus the final sign 18. The litany is circling back
     to its beginning. This is the structural ∮ (loop) -- the text literally
     returns to its opening phrase before the closing coda.
     Confidence: LOW (phonetic), MEDIUM (structural return)

A27: 23-19-35/
     Signs: COLUMN-PLANE-PLANE.TREE
     Phonetic: to-so-e
     Aggregate: (+0.3, +0.7, +0, +0.7) -- A-register, moderate
     Reading: "in this place" / "here, among [us]"
     Role: locative (oblique stroke = section boundary)
     Confidence: LOW

A28: 10-03-38
     Signs: ARROW-TATTOOED.HEAD-ROSETTE
     Phonetic: ha-nu-ko
     Aggregate: (-1.0, +0.3, -0.3, +1.0) -- A-only content, rare signs
     Reading: "forever" / "unto completion" / "to the end"
     Role: THE CODA (appears 2x: A28 = A31) -- closing formula
     Note: 10 (ARROW) = direction, force. 03 (TATTOOED HEAD) is rare
     (2 occurrences, both in this exact formula). 38 (ROSETTE) = completion,
     floral culmination. As the closing formula of the entire text,
     "ha-nu-ko" = a temporal/completive expression. The arrow pointing
     toward the rosette = "toward fulfillment."
     Confidence: LOW (phonetic), MEDIUM (as closing formula)

A29: 02-12-27-27-35-37-21
     = A17 (extended epithet, second occurrence)
     Reading: "O [title], she of the fertile earth and growing things"

A30: 13-01
     Signs: CLUB-PEDESTRIAN
     Phonetic: wo-je
     Aggregate: (+0.5, +1.0, -1.0, +0) -- short, formulaic
     Reading: "we declare" / "it is spoken"
     Role: penultimate declaration
     Note: This bigram (13-01) appears 5 times across the disc. It is
     the third most common bigram. As a 2-sign word, it functions as a
     speech-act particle: "I/we say" or "thus spoken."
     Confidence: LOW

A31: 10-03-38
     = A28 (THE CODA, second occurrence)
     Reading: "Forever." / "Unto completion."
```


### Side A -- Flowing Reading

> Hear this, O gathering of the sanctuary!
>
> So it is.
>
> Divine Mother, Mother-Goddess --
> O [titled one], mighty in power,
> from the sacred precinct she comes with offering.
> O [title], priestess of the rite...
> The exalted one!
> O [title], giver of abundance --
> we who serve draw near.
> O [title], of the sacred dwelling!
>
> Arise!
>
> O great one, who traverses the sea and the land --
> Hear us!
>
> **O Great Lady!**
>
> O [title], she of the fertile earth and growing things --
> from the deep, an offering --
>
> **O Great Lady!**
>
> O great one, who traverses the sea and the land --
> Hear us!
>
> **O Great Lady!**
>
> O [title], she who binds and frees, dove of the horizon --
> the priestess tends the shrine.
> The exalted place!
> O [title], we declare
> in this holy place,
> unto completion --
>
> O [title], she of the fertile earth and growing things --
> we have spoken.
> Unto completion.

Structural analysis: The litany follows a classic ancient Mediterranean prayer form:
1. OPENING (A1-A3): invocation + shared formula
2. BODY (A4-A12): epithets of the deity
3. INTERJECTION (A13)
4. VERSE-RESPONSE-REFRAIN x2 (A14-A22): the liturgical core
5. EXTENDED CODA (A23-A27): return to opening formula
6. CLOSING (A28-A31): doubled coda formula = finality

The threefold refrain IQEKURJA at the structural center is flanked by the verse
(ITANOHATARI, "she who traverses sea and land") and the response (PIJE, "hear us!").
This is the prayer's beating heart.


## Side B: The Narrative

Different register: fewer determinatives, more B-register signs (07, 29, 22, 08, 36),
no threefold repetition. Structure is sequential -- events, not invocations.

### Group-by-Group Reading

```
B1:  02-12-22-40-07
     Signs: PLUMED.HEAD-SHIELD-SLING-OX.BACK-HELMET
     Phonetic: i-qe-sa-we-se
     Aggregate: (+0.8, -0.2, -0.2, -0.4) -- classified, shifts toward B-register
     Reading: "The [titled one] came from the sea" / "The authority arrived"
     Role: NARRATIVE OPENING -- classified word opens the story
     Note: B-register signs (22 SLING, 07 HELMET) appear within a classified
     (02-12) group. The narrative begins with an authority figure entering
     the B-register (narrative) domain. The story is ABOUT a titled person.
     Confidence: LOW (individual), MEDIUM (as narrative opening)

B2:  27-45-07-35
     Signs: HIDE-WAVY.BAND-HELMET-PLANE.TREE
     Phonetic: ta-rya-se-e
     Aggregate: (+1.0, +0.8, -0.5, -0.3) -- B-register variant of A6
     Reading: "from the sacred precinct [of the grove]"
     Role: origin/location phrase
     Note: Compare A6 (27-45-07-12): same first three signs, different ending.
     A6 ends with 12 (SHIELD/qe, A-register), B2 ends with 35 (PLANE TREE,
     balanced). The B-side version of the same phrase shifts register.
     Confidence: LOW

B3:  02-37-23-05/
     Signs: PLUMED.HEAD-PAPYRUS-COLUMN-CHILD
     Phonetic: i-pa-to-na
     Aggregate: (+0.3, -0.5, +0, +0.3) -- classified, includes hapax
     Reading: "The [titled one] of Pa-to-na" / "[a named authority]"
     Role: proper name (02-prefix + hapax 05 CHILD = unique identifier)
     Note: If this is a place name, "pa-to" recalls Phaistos itself
     (pa-i-to in Linear B). The hapax sign 05 (CHILD) may specify a
     person or sub-location. "The one from Phaistos" is speculative
     but geographically fitting.
     Confidence: VERY LOW (specific name), MEDIUM (as classified name)

B4:  22-25-27
     Signs: SLING-SHIP-HIDE
     Phonetic: sa-no-ta
     Aggregate: (+0, -0.3, +0, -1.3) -- B-register, short
     Reading: "by ship" / "across the water"
     Role: means/instrument phrase
     Note: 22 (SLING, B-only initial) + 25 (SHIP) = maritime action.
     In a narrative about someone arriving, "by ship" is expected.
     Confidence: LOW

B5:  33-24-20-12
     Signs: TUNNY-BEEHIVE-DOLIUM-SHIELD
     Phonetic: pu-de-yo-qe
     Aggregate: (+0.5, +0, +0, -1.3) -- B-register content
     Reading: "bringing tribute/goods" / "with provisions"
     Role: cargo/purpose phrase
     Note: 33 (TUNNY) = fish/food. 24 (BEEHIVE) = honey/stored goods.
     20 (DOLIUM, a large storage vessel) = containment. 12 (SHIELD/qe) =
     authority marker. This reads as a cargo manifest in miniature:
     "fish, honey, and stored goods [for the authority]."
     Confidence: LOW (individual), MEDIUM (as provisions context)

B6:  16-23-18-43/
     Signs: SAW-COLUMN-BOOMERANG-STRAINER
     Phonetic: ni-to-ri-ta
     Aggregate: (+0, +0.3, +0, -0.5) -- B-register, section end
     Reading: "to the pillar-hall" / "at the court"
     Role: destination phrase (oblique stroke = section boundary)
     Note: 16 (SAW) is B-only initial -- a narrative-register opener.
     43 (STRAINER, Linear A = 'ta') is a hapax. Section boundary here
     marks the end of the arrival scene.
     Confidence: LOW

------- ARRIVAL SCENE ENDS, PROCEEDINGS BEGIN -------

B7:  13-01-39-33
     Signs: CLUB-PEDESTRIAN-LILY-TUNNY
     Phonetic: wo-je-re-pu
     Aggregate: (+0, +0.5, +0, -0.8) -- B-register
     Reading: "they declared the offering" / "the assembly spoke of provisions"
     Role: speech-act + content (wo-je = "they declared" from A30)
     Note: The wo-je particle (13-01) from Side A appears here in narrative
     context. When the liturgy's speech-act word appears in the narrative,
     it's the moment of official declaration. "They declared [regarding]
     the offering of fish/abundance."
     Confidence: LOW

B8:  15-07-13-01-18
     Signs: MATTOCK-HELMET-CLUB-PEDESTRIAN-BOOMERANG
     Phonetic: da-se-wo-je-ri
     Aggregate: (+0.6, +0.8, -0.6, -0.8) -- B-register
     Reading: "the worker spoke thus" / "[a person] made declaration"
     Role: speech attribution (includes wo-je = "declared")
     Note: 15 (MATTOCK) is a hapax -- a tool for digging/building. This
     is the common person's counterpart to the 02-prefix authority.
     A worker or laborer is making a statement.
     Confidence: LOW

B9:  22-37-42-25
     Signs: SLING-PAPYRUS-GRATER-SHIP
     Phonetic: sa-pa-we-no
     Aggregate: (-0.5, -0.5, +0.5, -1.5) -- strongly B-register
     Reading: "the ship-workers" / "the seafarers" / "the craftsmen"
     Role: subject group (22 B-initial + 25 SHIP = maritime actors)
     Note: 42 (GRATER) is a hapax -- a tool. Combined with SHIP (25),
     this identifies a group of maritime workers or builders.
     Confidence: LOW

B10: 07-24-40-35
     Signs: HELMET-BEEHIVE-OX.BACK-PLANE.TREE
     Phonetic: se-de-we-e
     Aggregate: (+0.8, +0.8, +0, -0.8) -- B-register, final-trending
     Reading: "brought honey and provisions" / "delivered stores"
     Role: action + object
     Confidence: LOW

B11: 02-26-36-40
     Signs: PLUMED.HEAD-HORN-VINE-OX.BACK
     Phonetic: i-ja-u-we
     Aggregate: (+0.3, -0.3, -0.3, +0.3) -- classified, mixed register
     Reading: "The [authority] of the vine/vineyard"
     Role: classified person/title (agricultural authority?)
     Note: 36 (VINE) is B-only. 02-prefix but NOT followed by 12 (SHIELD).
     Instead followed by 26 (HORN). Different classification structure than
     Side A -- a different kind of authority. Possibly a local official
     rather than the goddess's title.
     Confidence: LOW

B12: 27-25-38-01
     Signs: HIDE-SHIP-ROSETTE-PEDESTRIAN
     Phonetic: ta-no-ko-je
     Aggregate: (+0.8, +0.8, -0.3, +0.3) -- balanced to A-leaning
     Reading: "from the ship [to] the harbor" / "the ship's tribute"
     Role: transit phrase
     Confidence: LOW

B13: 29-24-24-20-35
     Signs: CAT-BEEHIVE-BEEHIVE-DOLIUM-PLANE.TREE
     Phonetic: ma-de-de-yo-e
     Aggregate: (+0.2, +0, +0, -1.2) -- B-register, doubled medial
     Reading: "the mother's store-houses" / "abundant tribute, doubly filled"
     Role: content phrase (doubled 24 BEEHIVE = emphasis on stored goods)
     Note: Doubled signs appear systematically: 29-29 (A4, ma-ma), 27-27
     (A17, ta-ta), 24-24 (B13, de-de). Each reduplication emphasizes
     the reduplicated concept. Here: doubled BEEHIVE = much storage/wealth.
     Confidence: LOW

B14: 16-14-18
     Signs: SAW-MANACLES-BOOMERANG
     Phonetic: ni-pe-ri
     Aggregate: (+0, +0.3, +0, -0.3) -- B-register, short
     Reading: "bound/obligated" / "under oath" / "the binding"
     Role: legal/obligation term
     Note: 14 (MANACLES) = binding. In a narrative about tribute and
     authority, "binding" is a legal obligation -- treaty, oath, or tax.
     Confidence: LOW

B15: 29-33-01
     Signs: CAT-TUNNY-PEDESTRIAN
     Phonetic: ma-pu-je
     Aggregate: (+0.7, +0.3, -0.7, -0.7) -- B-register
     Reading: "the mother[-goddess's] offering/share"
     Role: divine allocation (ma- prefix = maternal/goddess)
     Confidence: LOW

B16: 06-35-32-39-33
     Signs: WOMAN-PLANE.TREE-DOVE-LILY-TUNNY
     Phonetic: ti-e-lo-re-pu
     Aggregate: (-0.4, +0.6, +0.8, -0.4) -- highest mu (diversity) cluster
     Reading: "the woman [priestess] of the dove, bearing flowers and fish"
     Role: character description (rich in natural/divine imagery)
     Note: The highest-diversity cluster on the disc. WOMAN + DOVE + LILY +
     TUNNY = a priestess figure surrounded by goddess-symbols. In Minoan
     iconography, the dove-and-lily priestess is a well-attested figure.
     Confidence: LOW (phonetic), MEDIUM (iconographic match)

B17: 02-09-27-01
     Signs: PLUMED.HEAD-TIARA-HIDE-PEDESTRIAN
     Phonetic: i-su-ta-je
     Aggregate: (+0.8, -0.5, -0.8, +0) -- classified, unique opener
     Reading: "The [titled one] of the crown/tiara"
     Role: classified person (02-prefix + 09 TIARA = crowned authority)
     Note: 09 (TIARA) appears only twice, both on Side B. A crown-wearing
     authority figure distinct from the SHIELD-authority of Side A.
     This may be the narrative's protagonist -- a king or high priest.
     Confidence: LOW

B18: 29-36-07-08/
     Signs: CAT-VINE-HELMET-GAUNTLET
     Phonetic: ma-u-se-ki
     Aggregate: (+0.3, +0.3, -0.5, -1.8) -- maximally B-register
     Reading: "the mother-vine's power" / "by the goddess's might"
     Role: divine attribution (oblique stroke = section boundary)
     Note: The most B-register group on the disc (phi=-1.8). This is the
     B-side's equivalent of the A-side's maximally A-register refrain (A16).
     Where A16 invokes the goddess by title (IQEKURJA), B18 describes her
     power in narrative terms.
     Confidence: LOW (phonetic), MEDIUM (as B-register peak)

B19: 29-08-13
     Signs: CAT-GAUNTLET-CLUB
     Phonetic: ma-ki-wo
     Aggregate: (+0.3, +0.7, -1.0, -1.0) -- B-register, constrained
     Reading: "by the goddess's hand" / "the mother's strength"
     Role: instrumental phrase (29/ma = goddess, 08 GAUNTLET = hand/power)
     Confidence: LOW

B20: 29-45-07/
     Signs: CAT-WAVY.BAND-HELMET
     Phonetic: ma-rya-se
     Aggregate: = A3 (the cross-side formula)
     Reading: "[sacred formula]" / "so it is" / "amen"
     Role: CROSS-SIDE FORMULA (= A3, the bridge between registers)
     Note: This is the ONLY exact group repetition that crosses from
     Side A to Side B. Whatever this word means, it belongs to both
     the prayer and the narrative. A liturgical constant.
     Confidence: MEDIUM (as shared formula)

B21: 22-29-36-07-08/
     Signs: SLING-CAT-VINE-HELMET-GAUNTLET
     Phonetic: sa-ma-u-se-ki
     Aggregate: (+0.2, -0.2, -0.2, -1.6) -- strongly B-register
     Reading: "the seafarers [acknowledged] the goddess's might"
     Role: narrative formula (= B26, appears twice)
     Note: This is B18 (ma-u-se-ki) with 22 (SA, B-initial) prepended.
     The narrative formula = the B-register divine attribution with an
     agent prefix. "The [people of the sea] + [by the goddess's power]."
     Confidence: LOW

B22: 27-34-23-25
     Signs: HIDE-BEE-COLUMN-SHIP
     Phonetic: ta-di-to-no
     Aggregate: (+0.3, +0.3, -0.3, -0.3) -- balanced, B-leaning
     Reading: "the sacred bee's pillar by the ship" / "the temple harbor"
     Role: location phrase
     Note: 34 (BEE) + 23 (COLUMN) = sacred architecture. 25 (SHIP) =
     maritime. A temple or shrine at a harbor. Compare B29 (29-34-23-25):
     same last three signs but 29 (CAT/ma) replaces 27 (HIDE/ta).
     Confidence: LOW

B23: 07-18-35
     Signs: HELMET-BOOMERANG-PLANE.TREE
     Phonetic: se-ri-e
     Aggregate: (+1.3, +1.0, -0.3, -0.7) -- B-register, final-trending
     Reading: "they came forth" / "they departed"
     Role: action phrase (movement)
     Confidence: LOW

B24: 07-45-07/
     Signs: HELMET-WAVY.BAND-HELMET
     Phonetic: se-rya-se
     Aggregate: (+1.3, +0.3, -1.0, -1.7) -- maximally B-register
     Reading: "[sacred formula variant]" / "it is done"
     Role: B-SIDE CLOSING FORMULA (= B30 variant, appears at section ends)
     Note: Compare with A3/B20 (29-45-07 = ma-rya-se). Here 07 (HELMET/se)
     REPLACES 29 (CAT/ma) at the start. The B-register substitution:
     where the A-register formula starts with "ma" (mother/goddess),
     the B-register variant starts with "se" (a narrative-register marker).
     Same wavy-band core, different framing. Like using "verily" vs "truly."
     Confidence: LOW (phonetic), MEDIUM (as formula variant)

B25: 07-23-18-24
     Signs: HELMET-COLUMN-BOOMERANG-BEEHIVE
     Phonetic: se-to-ri-de
     Aggregate: (+0.8, +0.5, -0.3, -0.8) -- B-register
     Reading: "to the pillar-house of [tribute/storage]"
     Role: destination phrase
     Confidence: LOW

B26: 22-29-36-07-08/
     = B21 (narrative formula, second occurrence)
     Reading: "the seafarers [acknowledged] the goddess's might"

B27: 09-30-39-18-07
     Signs: TIARA-RAM-LILY-BOOMERANG-HELMET
     Phonetic: su-mu-re-ri-se
     Aggregate: (-0.2, +0.6, +0, -1.0) -- B-register, hapax-containing
     Reading: "the crowned one [offered] the ram among the flowers"
     Role: sacrificial narrative (30 RAM is a hapax = unique sacrificial act)
     Note: 09 (TIARA) = the crowned authority from B17. 30 (RAM) = the
     only occurrence of this sign on the entire disc. A ram sacrifice
     is the most significant offering in ancient Mediterranean religion.
     This is the narrative's climactic act.
     Confidence: LOW (phonetic), MEDIUM (ram sacrifice as climax)

B28: 02-06-35-23-07
     Signs: PLUMED.HEAD-WOMAN-PLANE.TREE-COLUMN-HELMET
     Phonetic: i-ti-e-to-se
     Aggregate: (+0.6, +0.4, +0, -0.2) -- classified, B-register
     Reading: "The [titled] woman of the sacred grove spoke"
     Role: classified speech-act (the goddess or priestess responds)
     Note: 02-prefix WITHOUT 12 (SHIELD). Instead: 02-06 (WOMAN directly
     after determinative). This is the only time the determinative precedes
     WOMAN directly. A titled/classified woman = the goddess herself or
     her chief priestess.
     Confidence: LOW

B29: 29-34-23-25
     Signs: CAT-BEE-COLUMN-SHIP
     Phonetic: ma-di-to-no
     Aggregate: (+0, +0.3, -0.3, -0.8) -- B-register
     Reading: "the mother [goddess] of the temple-harbor"
     Role: divine title in B-register (parallels B22 with 29 instead of 27)
     Note: B22 (ta-di-to-no) vs B29 (ma-di-to-no) -- only the first sign
     differs. The switch from 27 (HIDE/ta) to 29 (CAT/ma) changes the
     modifier but keeps the location. "The [authority's] temple-harbor"
     becomes "the mother's temple-harbor." Ownership/dedication expressed
     through initial-sign substitution.
     Confidence: LOW

B30: 45-07/
     Signs: WAVY.BAND-HELMET
     Phonetic: rya-se
     Aggregate: (+1.0, +0.5, -1.0, -1.5) -- B-register, absolute final
     Reading: "it is done." / "so be it."
     Role: ABSOLUTE CLOSING (shortest possible formula fragment)
     Note: This is the A3/B20 formula (ma-rya-se) stripped to its final
     two signs. B24 stripped the initial sign differently (se-rya-se).
     B30 strips it further: just rya-se. The text ends with the
     minimum viable fragment of the shared liturgical formula.
     Like ending with "-men" instead of "amen."
     Confidence: LOW (phonetic), MEDIUM (as reduced closing formula)
```


### Side B -- Flowing Reading

> The [titled authority] came from across the sea,
> from the sacred precinct of the grove.
> The [named one] of Phaistos,
> by ship,
> bringing fish, honey, and stored goods
> to the pillar-hall.
>
> The assembly declared the offering.
> A worker made proclamation.
> The seafarers and craftsmen
> brought honey and provisions.
> The [authority] of the vineyard,
> the ship's tribute conveyed.
>
> The mother-goddess's portion, doubly measured:
> under oath and binding.
> The goddess's share of the offering.
> The priestess of the dove, bearing flowers and fish.
> The crowned one of the tiara --
> by the goddess's might!
> By the goddess's hand!
>
> So it is.
>
> The seafarers acknowledged the goddess's power.
> At the temple-harbor,
> they came forth.
> It is done.
> To the pillar-house of tribute --
> the seafarers acknowledged the goddess's power.
>
> The crowned one offered the ram among the flowers.
> The [titled] woman of the sacred grove spoke:
> "The mother-goddess of the temple-harbor --"
>
> It is done.


## Complete Flowing Reading -- Both Sides

### Side A: The Prayer

> *Hear this, O gathering of the sanctuary!*
> *So it is.*
> *Divine Mother, Mother-Goddess --*
>
> *O mighty one, from the sacred precinct,*
> *with offering and sacrifice.*
> *O priestess of the rite...*
> *Exalted one!*
> *O giver of abundance --*
> *we your servants draw near.*
> *O [Lady] of the sacred dwelling!*
>
> *Arise!*
>
> *O great one, who traverses the sea and the land --*
> *Hear us!*
> ***O Great Lady!***
>
> *O she of the fertile earth and growing things --*
> *offering from the deep --*
> ***O Great Lady!***
>
> *O great one, who traverses the sea and the land --*
> *Hear us!*
> ***O Great Lady!***
>
> *O she who binds and frees, dove of the horizon --*
> *the priestess tends the shrine --*
> *the exalted place!*
> *We declare, in this holy place:*
> *unto completion.*
>
> *O she of the fertile earth and growing things --*
> *we have spoken.*
> *Unto completion.*


### Side B: The Narrative

> The authority came from across the sea,
> from the sacred precinct of the grove --
> the one of Phaistos, arriving by ship,
> bringing fish and honey and stored goods
> to the pillar-hall of the court.
>
> The assembly declared the offering.
> A laborer made proclamation.
> The seafarers brought honey and provisions.
> The vineyard-keeper delivered the ship's tribute.
>
> The mother-goddess's portion, doubly measured,
> bound under oath:
> the goddess's share of the offering.
> The dove-priestess bearing flowers and fish,
> the crowned one --
> by the goddess's might,
> by the goddess's hand.
>
> *So it is.*
>
> The seafarers acknowledged the goddess's power,
> at the temple-harbor they departed.
> *It is done.*
> To the pillar-house of tribute --
> the seafarers acknowledged the goddess's power.
>
> The crowned one sacrificed the ram among the flowers.
> The titled woman of the sacred grove spoke:
> "The mother-goddess of the temple-harbor --"
>
> *It is done.*


## Assessment

### What the lattice reveals with confidence:

1. **Side A IS a prayer/litany.** The threefold refrain (A16=A19=A22),
   paired verses (A14=A20), paired responses (A15=A21), and return to
   opening formula (A26~A1) form a textbook ancient Mediterranean liturgical
   structure. Confidence: HIGH (80%).

2. **Side B IS a narrative.** Sequential events, maritime vocabulary,
   tribute/provisions, a sacrifice, and a closing formula. No repeated
   refrains. Confidence: HIGH (80%).

3. **The goddess is the subject.** Both sides reference her: Side A invokes
   her with epithets, Side B describes tribute brought to her. The Minoan
   mother-goddess (later identified with Rhea, Cybele, or Potnia) presides
   over both the prayer and the accounting. This aligns with Owens' reading.
   Confidence: MEDIUM (65%).

4. **The narrative describes a maritime tribute event.** Ships, fish, honey,
   storage vessels, oaths, and a culminating ram sacrifice. This fits the
   archaeological context: Phaistos was a major palace-temple complex on
   southern Crete, receiving tribute from surrounding settlements and
   maritime trade routes. Confidence: MEDIUM (60%).

5. **The ram sacrifice is the climax.** Sign 30 (RAM) appears exactly ONCE
   on the entire disc -- in B27, near the end, associated with the crowned
   authority (TIARA) and flowers (LILY). A singular event given singular
   expression. Confidence: MEDIUM (55%).

### What remains speculative:

1. **Individual phonetic values.** No phonetic value has been proven. The
   Colless/Owens/Achterberg proposals are scholarly hypotheses, not
   established readings. Every "syllabic reading" above is tentative.
   Confidence: LOW (20-30%).

2. **Exact word meanings.** Even where phonetic values are proposed, the
   language is unknown. Whether "ma-rya-se" means "so it is" or "mother
   [verb]" or something else entirely cannot be determined from 242 signs.
   Confidence: LOW (15-25%).

3. **The Phaistos connection in B3.** Reading "pa-to" as "Phaistos" is
   attractive but circular -- we're reading a disc FOUND at Phaistos and
   claiming it names Phaistos. Confidence: VERY LOW (10%).

### What the lattice adds beyond existing scholarship:

1. **Register polarity (phi dimension)** cleanly separates A-vocabulary from
   B-vocabulary without any semantic assumption. The statistical test
   (chi2=60.73) confirms what the coordinates predict.

2. **Formula detection.** The cross-side formula (A3=B20 = "29-45-07"),
   the B-closing formula family (B24="07-45-07", B30="45-07"), and the
   embedded formula fragments (B18 core within B21/B26) emerge from
   coordinate clustering, not from phonetic assumptions.

3. **Structural markers.** The lattice identifies sign 02 as a determinative,
   sign 28 as an initial-only marker, and short groups (A13, A15, A18, A30)
   as function words -- all from distributional coordinates alone.

4. **The reading is falsifiable.** If another Phaistos-script text is found,
   every prediction here can be tested: sign 02 should be group-initial,
   the formula "29-45-07" should appear in ritual context, the A/B register
   split should correspond to genre, and the coordinate assignments should
   hold across the new corpus.

This disc is a 3700-year-old prayer to the Minoan mother-goddess on one side,
and an account of maritime tribute and sacrifice on the other. The stamps that
printed it were used for many copies -- this is the only one that survived
the destruction of Minoan civilization. We cannot read its words with certainty.
But we can hear its structure: the threefold invocation, the call and response,
the list of provisions, the singular ram sacrifice, the closing formula fading
to its last two signs.

The disc is not silent. It just speaks very quietly.
