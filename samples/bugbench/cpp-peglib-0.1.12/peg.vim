
set commentstring=#\ %s

syn match pegBoth /=/
syn match pegM2U /->/
syn match pegU2M /<-/

syn match pegName /\v[a-zA-Z_][a-zA-Z0-9_]*/

syn match pegLineComment '#.*'

syn region pegStringD start=/\v"/ skip=/\v\\./ end=/\v"/
syn region pegStringS start=/\v'/ skip=/\v\\./ end=/\v'/
syn region pegClass start=/\v\[/ skip=/\v\\./ end=/\v]/

"syn match pegOperator /\(*\|?\|+\|!\|\.\|\~\)/

hi def link pegBoth Statement
hi def link pegM2U Statement
hi def link pegU2M Statement

hi def link pegName Identifier

hi def link pegLineComment Comment

hi def link pegStringD String
hi def link pegStringS String
hi def link pegClass String

let b:current_syntax = "peg"
