" walk syntax — bond symbols, θ constants, labels, control flow

if exists("b:current_syntax")
  finish
endif

" comments
syn match walkComment ';.*$'

" labels
syn match walkLabel '^:\s*\w\+'

" walker control
syn keyword walkControl skip_z skip_nz loop_back

" bond symbols — forces
syn match walkBond 'π̄∮̄₃'
syn match walkBond 'π̄δ̄₃'
syn match walkBond 'π̄δ̄'
syn match walkBond 'π̄δ'
syn match walkBond 'πδ̄'
syn match walkBond 'πδ₃'
syn match walkBond 'πδ'
syn match walkBond 'π∘∮δ'
syn match walkBond 'π∘δ'
syn match walkBond 'π∘∮'
syn match walkBond 'π∘'
syn match walkBond 'π̄₇'
syn match walkBond 'π̄₃'
syn match walkBond 'π₇'
syn match walkBond 'π₃'
syn match walkBond 'π̄'
syn match walkBond 'π'
syn match walkBond '∘∮δ'
syn match walkBond '∘̄'
syn match walkBond '∘∮'
syn match walkBond '∘δ'
syn match walkBond '∘'
syn match walkBond '∮̄'
syn match walkBond '∮δ'
syn match walkBond 'δ̄₁₃'
syn match walkBond 'δ̄₁₁'
syn match walkBond 'δ̄₇'
syn match walkBond 'δ̄₅'
syn match walkBond 'δ̄₃'
syn match walkBond 'δ₁₃'
syn match walkBond 'δ₁₁'
syn match walkBond 'δ₇'
syn match walkBond 'δ₅'
syn match walkBond 'δ₃'
syn match walkBond 'δ̄'
syn match walkBond 'δ'
syn match walkBond 'τχ'

" θ constants
syn match walkTheta 'θ[ρ¹²³·]\+'

" numbers
syn match walkNumber '0x[0-9a-fA-F]\+'
syn match walkNumber '\<[0-9]\+\>'

" character literals
syn match walkChar "'[^']*'"

" highlighting
hi def link walkComment  Comment
hi def link walkLabel    Label
hi def link walkControl  Statement
hi def link walkBond     Keyword
hi def link walkTheta    Type
hi def link walkNumber   Number
hi def link walkChar     String

let b:current_syntax = "walk"
