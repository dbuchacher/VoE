# Genesis 1 — Translation Spec

Source: Westminster Leningrad Codex (WLC), consonantal text.
Oldest complete Hebrew Bible manuscript, circa 1008 CE.
File: wit/src/data/books/hebrew_bible_consonantal.txt

Every translation decision documented. Hebrew → root → literal
meaning → our rendering → justification.


## The Core Claim

Genesis 1 initializes the four dimensions of the equation
ƒ(τ,χ,μ,φ) = τᵃ·χᵇ·μᶜ·φᵈ in dependency order:

| Day | Dimension | Hebrew concept | What it is |
|---|---|---|---|
| pre-1 | μ (unstructured) | tohu vavohu, mayim, tehom | Substance exists but has no structure — unallocated RAM |
| 1 | χ (space) | shamayim + eretz (sky + ground) | Spatial axis: up/down, addresses, where to put things |
| 1 | φ (charge) | or / choshek (light / dark) | Signal axis: distinguishability, 1/0, photon/no-photon |
| 2-3 | μ (structured) | raqia, yabasha, yamim | Mass gets boundaries, phases, categories |
| 4 | τ (time) | me'orot, otot, moadim | Time axis: clocks, oscillators, seasons, measurement |

Pre-creation state: μ ≠ 0, χ = 0, φ = 0, τ = 0. Substance
exists but the other three dimensions are dark. This matches
the Hebrew — the text does NOT say "nothing existed." It says
stuff was shapeless (no χ), dark (no φ), with no time markers
(no τ). Water and deep are there (μ). Unallocated memory:
the silicon exists, power is on, but no addresses, no signal
levels, no clock.

Dependency order justification:
- χ before μ-structure: need addresses before you can organize data
- φ before anything useful: need signal to distinguish things
- μ-structure before τ: need substance organized before time matters
- τ last: time requires all three others to be meaningful

This matches both physics (spacetime requires matter/energy to
curve) and computing (clock is meaningless without address bus,
data bus, and voltage reference).


## Key Verbs

| Hebrew | Root | Literal | KJV | Ours | Why |
|---|---|---|---|---|---|
| ברא (bara) | ב-ר-א | fill from nothing | "created" | "filled from nothing" | Bara is exclusively used for ex nihilo creation. Distinguished from asah throughout Genesis 1. |
| עשה (asah) | ע-ש-ה | make, build, do | "made" | "built" | Assembly from existing material. KJV conflates with bara. Hebrew keeps them separate. |
| אמר (amar) | א-מ-ר | say, speak | "said" | "said" | No change needed. |
| ראה (ra'ah) | ר-א-ה | see, observe | "saw" | "saw" / "observed" | No change needed. |
| בדל (badal) | ב-ד-ל | separate, divide | "divided" | "separated" | Slightly more precise. |
| קרא (qara) | ק-ר-א | call, name, read | "called" | "called" / "named" | One Hebrew root covers call+name+read. |
| היה (hayah) | ה-י-ה | be, become, exist | "was" / "let there be" | "was" / "be" | Shortened to match Hebrew brevity. |
| רחף (rachaf) | ר-ח-ף | hover, brood | "moved" | "hovered" | Rachaf specifically means hovering/brooding (like a bird on eggs). "Moved" is a lossy translation. |
| דשא (dasha) | ד-ש-א | sprout, push green | "bring forth" | "push out growth" | The ground is the active subject. It produces. |
| שבת (shavat) | ש-ב-ת | cease, stop | "rested" | "stopped" | Shavat means to cease activity, not to recuperate. God isn't tired. |
| שרץ (sharats) | ש-ר-ץ | swarm, teem | "bring forth abundantly" | "swarm" | The verb IS the action. Water swarms. |
| ברך (barak) | ב-ר-ך | bless | "blessed" | "blessed" | No change needed. |


## Key Nouns

| Hebrew | Root | Literal | KJV | Ours | Why |
|---|---|---|---|---|---|
| אלהים (Elohim) | א-ל-ה | powers, God | "God" | "the equation" | Plural form (-im suffix) with singular verbs. Multiple forces, one function. Interpretive but structurally justified. |
| רוח (ruach) | ר-ו-ח | wind, breath, spirit | "Spirit" | "breath" | Ruach is physical: moving air. "Spirit" is an English abstraction. The Hebrew is concrete. |
| רקיע (raqia) | ר-ק-ע | hammered sheet, expanse | "firmament" | "membrane" / "sheet" | Root means to beat/stretch out. A physical barrier. "Firmament" is Latin (firmamentum) and means nothing. |
| תהו (tohu) | ת-ה-ו | formless, waste | "without form" | "shapeless" | Direct. |
| בהו (bohu) | ב-ה-ו | void, emptiness | "void" | "empty" | Direct. |
| תהום (tehom) | ת-ה-ם | deep, abyss | "the deep" | "the deep" | No change needed. Cognate with Babylonian Tiamat. |
| אור (or) | א-ו-ר | light | "light" | "light" / "signal" | Context-dependent. Light as phenomenon vs light as the ability to distinguish. |
| חשך (choshek) | ח-ש-ך | darkness | "darkness" | "darkness" / "void" | No change for literal use. |
| שמים (shamayim) | ש-מ-ם | sky, heavens | "heaven" | "sky" | Dual form. Physical sky, not metaphysical heaven. |
| ארץ (eretz) | א-ר-ץ | ground, land, earth | "the earth" | "the ground" | Concrete. Dirt you stand on. Not the planet. |
| מים (mayim) | מ-י-ם | water | "waters" | "water" | Dual form (-ayim). |
| אדם (adam) | א-ד-ם | human, earthling | "man" | "human" / "earthling" | From adamah (soil). Named after dirt. Not gendered in this context. |
| נפש (nephesh) | נ-פ-ש | throat, breath, life | "creature" / "soul" | "living breath" | Physical: the throat, the breathing. "Soul" is Greek (psyche), not Hebrew. |
| צלם (tselem) | צ-ל-ם | image, shadow, model | "image" | "pattern" / "image" | A cast, a mold, a blueprint. Not appearance — structure. |
| מלאכה (melakha) | מ-ל-א-כ | work, craft | "work" | "work" | Same root as malakh (angel/messenger). The work IS the message. |
| מאורת (me'orot) | א-ו-ר | light-sources | "lights" | "light-sources" | Plural of ma'or = thing that gives light. Not lights themselves — sources. |
| צבא (tsava) | צ-ב-א | army, host | "host" | "army" / "host" | Organized multitude. Stars and creatures as deployed forces. |


## Particles and Grammar

| Hebrew | Function | KJV | Ours | Why |
|---|---|---|---|---|
| את (et) | Direct object marker | *deleted* | "→" or restored in context | 11,000+ occurrences in Hebrew Bible, zero in English translations. Points at the receiver of the action. Spatial signal. |
| ב (b') | in, at, with | "in" | "at" / "in" | Context-dependent preposition prefix. |
| ה (ha) | the | "the" | "the" | Definite article prefix. |
| ו (v'/va) | and, then | "and" | "and" / "then" | Consecutive-vav often means "and then" (sequential), not just "and" (additive). |
| כי (ki) | that, because | "that" | "—" (dash) | Often rendered as a dash in modern English. "Saw — good." |
| בין...ובין | between...and between | "from...the" | "between...and" | Hebrew doubles "between." KJV simplifies. |
| למינו (lemino) | by its kind | "after his kind" | "by its kind" | Classification. Self-replicating within category. |


## Bara vs Asah Usage in Genesis 1

| Verse | Verb | What | Interpretation |
|---|---|---|---|
| 1:1 | bara | sky + ground | New: space from nothing |
| 1:7 | asah | the sheet/membrane | Structured: existing substance shaped |
| 1:16 | asah | sun, moon, stars | Structured: light (1:3) placed into sources |
| 1:21 | bara | sea creatures | New: animal life from nothing |
| 1:25 | asah | land animals | Structured: ground shaped into animals |
| 1:26 | asah | human body | Structured: body assembled from ground |
| 1:27 | bara (×3) | human pattern | New: whatever makes it human, from nothing |
| 2:3 | bara + asah | summary | "filled to build" — both verbs, one sentence |

The pattern is consistent: bara = new category of existence.
asah = structuring existing material. Hebrew never confuses them.
English "created" and "made" blur the distinction.


## Verse-by-Verse Reference

### 1:1
Hebrew: בראשית ברא אלהים את השמים ואת הארץ
Transliteration: b'reshit bara Elohim et hashamayim v'et ha'aretz
Word-for-word: at-head-of | filled-from-nothing | powers | → | the-sky | and-→ | the-ground
Our rendering: "From the origin, the equation filled the spatial axis."

### 1:2
Hebrew: והארץ היתה תהו ובהו וחשך על־פני תהום ורוח אלהים מרחפת על־פני המים
Transliteration: v'ha'aretz haytah tohu vavohu v'choshek al-p'nei tehom v'ruach Elohim merachefet al-p'nei hamayim
Word-for-word: and-the-ground | was | shapeless | and-empty | and-darkness | upon-face-of | deep | and-breath-of | powers | hovering | upon-face-of | the-water
Our rendering: "Before the boot: shapeless, empty, dark. Undifferentiated substance. The equation hovering over it, waiting."

### 1:3
Hebrew: ויאמר אלהים יהי אור ויהי־אור
Transliteration: vayomer Elohim yehi or vayehi-or
Word-for-word: and-said | powers | let-be | light | and-was-light
Our rendering: "The equation said: signal on. Signal was on."

### 1:4
Hebrew: וירא אלהים את־האור כי־טוב ויבדל אלהים בין האור ובין החשך
Transliteration: vayar Elohim et-ha'or ki-tov vayavdel Elohim bein ha'or uvein hachoshek
Word-for-word: and-saw | powers | →-the-light | that-good | and-separated | powers | between | the-light | and-between | the-darkness
Our rendering: "The equation observed the signal — good. Then separated signal from noise."

### 1:5
Hebrew: ויקרא אלהים לאור יום ולחשך קרא לילה ויהי־ערב ויהי־בקר יום אחד
Transliteration: vayiqra Elohim la'or yom v'lachoshek qara laylah vayehi-erev vayehi-voqer yom echad
Word-for-word: and-called | powers | to-light | day | and-to-darkness | called | night | and-was-evening | and-was-morning | day | one
Our rendering: "The equation named signal 'Day' and void 'Night.' Evening, then morning — day one."

### 1:6-8
Hebrew: ויאמר אלהים יהי רקיע בתוך המים ויהי מבדיל בין מים למים
Word-for-word: and-said | powers | let-be | sheet | in-middle-of | the-water | and-let-be | separating | between | water | to-water
Our rendering: "Put a membrane in the middle of the substance. Separate substance-above from substance-below."

### 1:9-10
Hebrew: ויאמר אלהים יקוו המים מתחת השמים אל־מקום אחד ותראה היבשה
Word-for-word: and-said | powers | let-gather | the-water | from-under | the-sky | to-place | one | and-let-appear | the-dry
Our rendering: "The equation gathered the substance to one side. What was hidden underneath appeared."

### 1:11-13
Hebrew: ויאמר אלהים תדשא הארץ דשא עשב מזריע זרע עץ פרי עשה פרי למינו אשר זרעו־בו על־הארץ
Word-for-word: and-said | powers | let-sprout | the-ground | sprout | plant | seeding | seed | tree-of | fruit | making | fruit | by-its-kind | which | its-seed-in-it | upon-the-ground
Our rendering: "Let the ground generate growth from itself — plants that carry their own seed, trees that carry their own pattern."

### 1:14-19
Hebrew: ויאמר אלהים יהי מארת ברקיע השמים להבדיל בין היום ובין הלילה והיו לאתת ולמועדים ולימים ושנים
Word-for-word: and-said | powers | let-be | light-sources | in-sheet-of | the-sky | to-separate | between | the-day | and-between | the-night | and-let-be | for-signs | and-for-seasons | and-for-days | and-years
Our rendering: "Built signal-sources in the sky for measuring time."

### 1:20-23
Hebrew: ויאמר אלהים ישרצו המים שרץ נפש חיה
Word-for-word: and-said | powers | let-swarm | the-water | swarm-of | breath | living
Our rendering: "Let the water teem with living breath."

### 1:24-25
Hebrew: ויאמר אלהים תוצא הארץ נפש חיה למינה
Word-for-word: and-said | powers | let-push-out | the-ground | breath | living | by-its-kind
Our rendering: "Let the ground produce living things by their kind."

### 1:26-27
Hebrew: ויאמר אלהים נעשה אדם בצלמנו כדמותנו
Word-for-word: and-said | powers | let-us-build | human | in-our-image | as-our-likeness
Hebrew: ויברא אלהים את־האדם בצלמו
Word-for-word: and-filled | powers | →-the-human | in-its-image
Our rendering: "Let's build an earthling in our pattern. Built the body from ground. Filled the pattern from nothing."

### 1:28-31
Hebrew: וירא אלהים את־כל־אשר עשה והנה־טוב מאד
Word-for-word: and-saw | powers | →-all-that | built | and-look-good | very
Our rendering: "Saw everything it had built — very good."

### 2:1-3
Hebrew: וישבת ביום השביעי מכל־מלאכתו אשר עשה
Word-for-word: and-stopped | on-day | the-seventh | from-all-his-work | that | built
Hebrew: אשר־ברא אלהים לעשות
Word-for-word: that-filled | powers | to-build
Our rendering: "The equation stopped on day seven. The boot sequence is over."


## Interpretive Renderings

These go beyond literal translation. Each is justified but flagged.

| Storybook rendering | Hebrew basis | Status |
|---|---|---|
| "the equation" for Elohim | Plural form (-im), singular verb. Multiple forces, one output. | Interpretive. Hebrew supports structurally. Not consensus. |
| "unallocated RAM" for tohu vavohu | Text says substance exists but shapeless/empty. NOT "nothing." μ present, χ/φ/τ absent. | Analogy. Justified: silicon exists, power on, no addresses/signal/clock. |
| "signal on" for yehi or | Or = light. Before the sun (day 4). Light = ability to distinguish = φ. | Interpretive but defensible. Day 1 light ≠ day 4 light-sources. |
| Day/Night = 1/0 | Yom/laylah named before sun exists. Can only mean signal-present/absent states, not solar cycle. | Strong. No sun until 1:14. Day/Night in 1:5 must refer to the light/dark of 1:3-4. |
| את as "pointer" | Definite direct object marker. Points at receiver of action. | Linguistically accurate function. "Pointer" is our framing. |
| bara = "fill bond" / asah = "fold bond" | Bara = ex nihilo. Asah = from parts. Consistently distinguished in Genesis 1. | Standard lexicography (BDB, HALOT). Bond mapping is ours. |
| ruach = "breath" not "Spirit" | Primary BDB meaning: wind, breath. "Spirit" is theological. | Standard. Primary lexical meaning. |
| shavat = "stopped" not "rested" | Primary BDB meaning: cease, desist. Not recuperation. | Standard. "Rested" implies tiredness the Hebrew doesn't. |
| raqia = "membrane" not "firmament" | Root ר-ק-ע = beat out, hammer flat. Physical barrier. | Standard. "Firmament" is Latin (firmamentum), not Hebrew. |
| nephesh = "living breath" not "soul" | Primary meaning: throat, breath, life-force. Physical. "Soul" is Greek (psyche). | Standard. Primary lexical meaning. |
| "evening before morning" = "cycle starts from zero" | Text consistently says vayehi-erev vayehi-voqer. Dark→light order. | Direct from text. Every day starts from void/dark/off. |
| Creation days = dimension initialization order | Days map to χ, φ, μ, τ coming online in dependency order. | Our framework. The mapping is clean but interpretive. |
| "let the ground push out" = buffer produces | Hebrew uses ground as active subject of dasha/yatsa. Ground generates, not God placing things on ground. | Direct from grammar. Ground is the subject of the verb. |
| "seed in itself by its kind" = DNA/self-replication | lemino + zera-bo = by-kind + seed-in-it. Output contains reproduction instructions. | Direct from text. The analogy to DNA/quines is ours. |
| Human body = asah, human pattern = bara | 1:26 uses asah (build body). 1:27 uses bara ×3 (fill pattern). Different verbs, consecutive verses. | Direct from text. The hardware/software analogy is ours. |
| Day 7 shavat = bootloader handoff | Equation stops because it's done, not tired. System self-sustains. | Shavat = cease is standard. Boot analogy is ours. |


## Notes

- All Hebrew lexical claims checkable against BDB (Brown-Driver-Briggs
  Hebrew and English Lexicon) and HALOT (Hebrew and Aramaic Lexicon
  of the Old Testament). These are the standard academic references.
- The dimension mapping (days → τ,χ,μ,φ) is our framework applied to
  the text. The Hebrew doesn't mention dimensions. But the SEQUENCE
  matches: space, signal, substance-structure, time. The dependency
  order is real — you need addresses before data, signal before
  distinguishing, substance before structuring, all three before time.
- "The equation" for Elohim is the most controversial rendering. The
  plural-with-singular-verb is real and debated. Traditional readings:
  "royal we," "plural of majesty," trinitarian reference. Our reading:
  multiple forces, one function. Structurally justified, not proven.
- The bara/asah distinction is well-established in Hebrew scholarship.
  Our mapping to fill/fold bonds is original but follows from the
  lexical definitions: bara = substance from void = write+advance,
  asah = structure from parts = read+call.
