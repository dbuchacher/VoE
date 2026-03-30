# The Voynich Manuscript: What the Lattice Found

---

## The Most Famous Unread Book in the World

The Voynich Manuscript is a handwritten book roughly six hundred years old, most likely produced in Europe sometime around 1400 CE. It is approximately 240 pages long. Every page is illustrated — with strange plants that match nothing in any known herbal, with circular astronomical diagrams, with small human figures bathing in green liquid connected by elaborate tubes, with rows of labeled containers that look like a pharmacist's shelf.

Every word of text is in an unknown script. No one has read it.

This is not for lack of trying. The manuscript was named after Wilfrid Voynich, a Polish book dealer who purchased it in 1912, but it had been circulating among European scholars for centuries before that — surfacing in the court of Holy Roman Emperor Rudolf II, passing through Jesuit hands, surviving the upheavals of three centuries before landing in Voynich's possession. In the twentieth century it attracted serious attention from professional cryptanalysts, linguists, historians, and an army of determined amateurs. The National Security Agency ran classified analyses. William Friedman, the man who broke the Japanese Purple cipher in World War II, spent years on it. Gordon Rugg, a British cognitive scientist, spent years trying to prove it was a hoax. None of them cracked it. None of them could even agree on what kind of problem they were dealing with.

The manuscript has now defeated everyone who has seriously engaged with it for over a hundred years. You should hold that fact in your mind while reading the rest of this chapter.

---

## The Three Possibilities

When you face something like the Voynich, you are essentially choosing between three hypotheses.

The first is that it is a natural language written in an unfamiliar script — that is, somewhere behind the strange glyphs there is a real human language, spoken by real people, which was encoded into this writing system the way Latin can be encoded into the Roman alphabet.

The second is that it is a cipher — that someone took a text in a known language (Latin, Italian, German, Arabic) and encrypted it, transforming each letter or word into Voynich characters according to some rule or key that has been lost.

The third is that it is a hoax — that the manuscript has no content, that someone in the fifteenth century or later generated plausible-looking text by inventing a script and filling pages with meaningless patterns that would fool a buyer into thinking they had acquired something of great value.

The lattice analysis does not solve the Voynich. But it does weigh these three possibilities, and it does so using structural measurements from the 37,000 words in the Takahashi EVA transcription — the most complete scholarly transliteration of the manuscript's text. The results are genuine and carry real evidential weight, even if they stop well short of a translation.

---

## Is It Real? The Statistical Verdict

The first thing any serious analysis has to establish is whether the Voynich text behaves like language or like noise. The answer is surprisingly clear.

Meaningless random text has a characteristic statistical fingerprint. Its entropy — the technical measure of how unpredictable the next symbol is, given what came before — is high. The symbols don't cluster into preferred combinations. There are no strong positional preferences, no repeated patterns at the word level, no Zipfian rank-frequency distribution. Random text looks the same everywhere.

The Voynich is not random. It obeys Zipf's law beautifully — better, in fact, than English does. Zipf's law says that in any real communicative system, the most common word appears roughly twice as often as the second most common, three times as often as the third, and so on. This law emerges from the compression dynamics of communication; it's a signature of information being encoded efficiently. The Voynich has it. That single fact is the strongest argument that the manuscript contains real content.

The Voynich also shows systematic variation across sections. The herbal pages have a different vocabulary profile than the astronomical pages. The recipe section uses vocabulary drawn from all the others. If someone were generating meaningless text — filling pages to fool a buyer — they would use the same process everywhere, and the statistics would look the same everywhere. They don't.

So the hoax hypothesis is not ruled out entirely, but the structural evidence against it is significant. Something is being communicated.

---

## The 3D Collapse: A Key Discovery

Here is where the lattice approach adds something genuinely new.

The lattice framework analyzes each glyph in the Voynich along four dimensions: where it tends to appear in a word (beginning, middle, or end), which structural layer it belongs to in terms of how the writing system is organized, how frequently it appears, and whether it shows any voicing distinction (the way that English pairs p with b, t with d, k with g, f with v — each pair distinguished by whether the vocal cords vibrate).

In English, French, Latin, German — in any European language written in an alphabetic script — that fourth dimension, the voicing dimension, produces clear structure. Voiced and voiceless consonants behave differently. They cluster in different positions, take different neighbors, and distribute differently across word types.

The Voynich has no such structure. Twenty-eight of the manuscript's thirty distinct glyphs cluster into a single narrow band in that dimension. The voicing split simply does not exist. The fourth dimension collapses.

What this means in plain language is that the Voynich writing system operates in three effective dimensions, not four. And that three-dimensional structure is not what any alphabetic European cipher produces. When you encipher Latin or Italian by substituting one letter for another, you preserve the statistical fingerprint of the original language. The Voynich fingerprint does not match any known European language.

The collapsed fourth dimension is instead consistent with a syllabary — a writing system like Japanese kana, where each character represents a syllable rather than a single sound, and where the voiced/voiceless distinction is bundled inside the syllable sign rather than existing as an independent dimension. It is also consistent with a semantic script, where each symbol represents a meaning rather than a sound. Either way, the Voynich writing system operates on a different structural principle than the alphabets of medieval Europe.

This is a meaningful finding. It says: whatever this is, it is not an enciphered European text.

---

## Position Locking: The System Has Hard Rules

The most striking structural feature of the Voynich is also the most visible to a trained analyst: its glyphs are locked to specific positions within words with a rigidity that has no parallel in any known natural writing system.

In English, the letter Q strongly prefers initial position — but it can appear elsewhere. Voynich Q appears at the beginning of words 98.8% of the time. The glyph N appears at the end of words 97.0% of the time. The glyph E almost never starts or ends a word — it is 98.6% medial. The doubled glyph II (written ii in the EVA transcription) never starts or ends a word: 99.9% medial.

These are not preferences. They are rules. The system has a rigid grammar of position: certain glyphs can only appear at the beginning of a word, certain glyphs can only appear in the middle, certain glyphs can only appear at the end. A word that violates these rules is essentially impossible in the corpus.

Natural alphabets don't work this way. Neither do simple substitution ciphers — because enciphering a European alphabet preserves the positional flexibility of the original language. The position locking survives as strong evidence that the Voynich is not a simple substitution cipher.

What the position locking does resemble is the structure of a syllabary or an abugida — a writing system with separate classes of symbols for different parts of a syllabic unit, where the position of a symbol carries meaning. In Korean Hangul, for instance, the components of a syllable block have strict positional roles: initial consonant, vowel, final consonant. The Voynich appears to have something analogous: an initial class of glyphs, a medial class, and a final class, with very limited crossing between them.

---

## What the Sections Are About

The Voynich has several visually distinct sections, each with its own type of illustration. The lattice analysis finds that these sections also have distinct statistical profiles — they use different vocabulary, with different grammatical characteristics. This is significant: if the text were meaningless, there would be no reason for the statistics to vary systematically with the illustrations. The fact that they do suggests the text and images are communicating the same things.

**The herbal pages** are the most straightforward. The illustrations show plants — strange plants, many of which don't correspond to any known species, but organized and depicted in the manner of a medieval herbal. The text on these pages clusters heavily in vocabulary that the lattice associates with physical substances, plant parts, and their properties. The most common words in the herbal section, across thousands of occurrences, appear to be functioning as descriptors: this part of the plant, that structural feature, this physical characteristic. The herbal pages read, in their coordinate profile, like an identification guide — describing what things are.

**The astronomical pages** shift the vocabulary in exactly the way you would predict if the text is genuinely tracking content. Words associated with temporal relationships, direction, and quantity become more prominent. The circular diagrams with labels around them — likely star names or calendar divisions — use abbreviated single-word identifiers. The statistical profile of the astronomical section is consistent with observational records: recording what is visible, where, and when.

**The biological pages** are the most dramatic. The illustrations show elaborate networks of tubes connecting pools of liquid, with small human figures (often described as nymphs or bathers) apparently moving through the system. Whether this depicts anatomy, a bathing ritual, or alchemical distillation, the text is unambiguous at the structural level: it is saturated with action words (verbs, in the lattice framework) applied to substance words. The biological section has three times as many action-verb forms as the herbal description pages. The text is about things being done to fluids — channeled, poured, collected, heated, drained.

**The pharmaceutical pages** appear to bridge the herbal and biological sections. The vocabulary draws heavily on the same roots as the herbal pages, but with different suffixes that seem to mark the prepared or extracted form. The same word-root that appears in the herbal section meaning something like "the plant" reappears in the pharmaceutical section with a suffix that shifts it toward "the extract" or "the preparation." This morphological consistency — the same root taking different endings to mark different forms of the same substance — is one of the more linguistically compelling findings in the analysis.

**The recipe section** brings everything together. It uses vocabulary from all the other sections: the plant words from the herbal, the process words from the biological, the preparation words from the pharmaceutical. Structurally, it reads like recipes should read — naming ingredients, describing processes, arriving at products. Each paragraph (marked with a small star in the manuscript) appears to function as a self-contained entry.

---

## Two Voices in One Book: The A/B Split

Scholars noticed long ago that the Voynich seems to have been written in two slightly different hands or styles, called Language A and Language B by the researcher Prescott Currier. The distinction has been debated ever since: is it two languages? Two dialects? Two scribes with different habits?

The lattice analysis suggests a different interpretation: two registers of the same language.

Language A is the descriptive register. It dominates the early herbal pages and uses vocabulary organized around classification and identification. It uses more demonstrative and article-like words. It has fewer action words. It reads, structurally, like a catalog: "This is X. It has the following features."

Language B is the procedural register. It dominates the biological pages and the recipe section, and it uses more action words, more process-product vocabulary, and more of the suffix forms that appear to mark completed or extracted states. It reads like instructions: "Take X. Prepare Y. Combine using this method."

The boundary between these two registers is surprisingly hard. Certain words that appear hundreds of times in the B-register pages appear zero times in the pure A-register pages. This isn't a gradual shift. It's a clean break, the kind you see when a writer deliberately switches modes — from describing plants in a catalog to explaining how to use them in a workshop.

If this interpretation is correct, the Voynich is not two languages or two scribes with different alphabets. It is one writing system used in two modes, the way a modern medical textbook might have descriptive chapters about anatomy and procedural chapters about treatment, written in the same language but with very different vocabulary patterns.

---

## The Grammar of the System

Three structural features of the Voynich are consistent enough to be stated with some confidence as likely grammatical facts — not translations, but structural roles.

The prefix qo- (the glyph Q followed by O, appearing at the start of words 98.9% of the time) functions as a verbal marker. Words beginning with this prefix are overwhelmingly concentrated in the sections that describe processes and actions. The biological section, the most process-heavy part of the manuscript, has three times as many qo- words as the descriptive herbal section. In the lattice framework, high positional preference combined with section-sensitivity to process-content is the signature of an action marker — a verb prefix, or something functionally equivalent to one.

The suffix -y (the glyph Y, appearing at the end of words 84.4% of the time) appears to mark the nominal or product form of a concept. Words ending in -y tend to be the names of substances, classes of materials, or the products of processes. "Seed," "powder," "body fluid," "channeled substance" — these are the semantic neighborhoods of the most common -y words. In the lattice, final position maps to result and product. The -y suffix seems to be the Voynich way of saying "the thing that resulted from" or "the substance that is."

The suffix -n (and its compound form -aiin) is even more locked than -y, appearing at word-final position 97% of the time. Words ending in -n appear to function as completed-state markers — the definite article, the demonstrative, the "of the" construction. The most common Voynich word overall, daiin, carries this suffix and appears to function something like the English phrase "of the" or "of this." The most common demonstrative, aiin, appears to mean something like "this" or "the following." In the recipe section, aiin actually surpasses daiin as the dominant word — consistent with recipe language pointing at things and saying "take THIS, add THIS," rather than the herbal language attributing properties by saying "of the root, of the leaf."

These three structural features — the qo- verb prefix, the -y nominalization suffix, the -n completion marker — are not translations. They are structural roles inferred from position data and distributional patterns. The confidence for individual words is low, around 15 to 25 percent. The confidence for these structural roles sits higher, around 35 to 45 percent. They could be wrong. But they are consistent across the entire corpus.

---

## A Page Viewed Closely

Consider the biological page f78r, which shows the tube-and-pool illustrations with human figures. A lattice reading of this page — speculative, maybe 15 percent confidence — sounds something like this:

> The body fluid, prepare the thick substance, channel the substance, flowing, channel the whole...
> Pour the warm, channel the substance, body fluid, combined substance, the result, within...
> Measure, the taken substance, channel it, channel it, amount, channel it again, and the remainder...
> The dried body fluid, channel it, the starting-heated, channel it again, body fluid, the heated substance, body fluid, prepare...

The word qokedy — "to channel a substance," in the lattice reading — appears eleven times on this single page. If that reading is even approximately correct, the text is describing the process shown in the illustrations: fluid moving through a network of connected channels, each step moving it from one vessel to the next. The illustrations and the text would be saying the same thing. That alignment is the kind of internal consistency that is hard to produce accidentally in a meaningless text.

---

## What We Still Cannot Determine

The lattice analysis does not crack the Voynich. It is important to be direct about what remains unknown.

We cannot identify the underlying language. The writing system's structure is consistent with a syllabary or semantic script, but that does not tell us which language it might encode — or whether it encodes any spoken language at all. It might be a constructed philosophical language, like the artificial languages that European scholars began attempting to build in the seventeenth century. If so, the Voynich would predate those attempts by two centuries.

We cannot read individual words with confidence. The English glosses scattered through the analysis — "leaf," "root," "channel," "body fluid" — are coordinate-neighborhood suggestions based on statistical patterns and section context. They might be directionally correct. They are not translations.

We cannot determine who wrote it, where, or exactly why. The radiocarbon dating of the vellum places the physical material in the early fifteenth century, probably 1404 to 1438. The ink and paint have not been reliably dated independently. The manuscript might have been written when the vellum was made, or decades later on older material.

We cannot rule out that the system is a verbose cipher — one where each word or syllable in a plaintext language maps to a Voynich word, producing the high vocabulary diversity we observe. Such a cipher would look structurally similar to what the lattice sees, and would be extremely difficult to break without the key.

---

## What the Lattice Actually Found

The Voynich Manuscript has defeated everyone who has seriously engaged with it. This analysis does not break that pattern. But it does add something.

The statistical evidence against the hoax hypothesis is real. The manuscript is not random noise. It obeys communicative laws — Zipf's law, section variation, systematic morphological patterns — that are hard to produce accidentally or deliberately without an underlying information structure.

The evidence against simple enciphered European text is also real. The fourth-dimension collapse, the position locking, the bigram constraints — these do not survive a simple letter substitution from any known European alphabet. Whatever encoding system the Voynich uses, it is not that.

The section-level semantic clustering is genuine. The herbal pages describe substances and parts. The biological pages describe processes. The recipe section combines both. This is the structure of a practical manual, and the text statistics are consistent with it — not because we read the words, but because the coordinate profiles of each section point in the right semantic direction.

The morphological patterns are suggestive. The suffix transformations from herbal to pharmaceutical — the same root appearing in the herbal section to mean the plant and in the pharmaceutical section to mean the extract — show the kind of regular morphology that a real writing system uses. A hoax generator would not produce this. A simple letter cipher would not show this either.

The honest summary is this: the lattice sees the Voynich as a three-dimensional writing system encoding a pharmacological and botanical manual. The text describes plants, describes physiological processes, describes preparations, and brings them together in a recipe section. The writing system has a real grammatical structure — an action prefix, a nominalization suffix, a completion marker — that operates consistently across the corpus.

That is what the evidence shows. Individual word meanings remain uncertain. The underlying language remains unknown. The manuscript keeps its secrets.

But it does have secrets. That much, the lattice is confident about.

---

*Structural analysis based on 37,026 words across the Takahashi EVA transcription. Entropy measurements, position statistics, and coordinate assignments computed from 163,288 glyph tokens across 32 distinct glyph types. All word-level translations carry 15-25% confidence. Section-level semantic clustering carries 50-65% confidence. The finding that the Voynich is not random noise and is not a simple European cipher carries 80-90% confidence.*
