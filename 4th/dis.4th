\ 
\ Set of words to disassemble compiled words back to
\ equivalent text.


: ' ( -- opcode) gw lu not if push0 then ;  \ gets next word, leaves its opcode if found, else 0
: colon 0x3a emit ;
: semicolon 0x3b emit ;
: space 0x20 emit ;
: cmp-print-next
    over == if drop 1 + dup p@ . push1 then ;
: cmp-done
    over == if drop push0 then ;
: print-opcode ( n opcode -- n done? )
    dup o2w
    20022 cmp-print-next \ if
    20032 cmp-print-next \ goto
    20035 cmp-print-next \ pushn
    20040 cmp-done       \ return
    not
    ;
: get-opcode ( n -- n opcode )
    dup p@ ;
: dis ( opcode -- )
    colon space dup o2w
    o2p
    dup if \ user defined
        cr
        push1 -
        begin
            push1 +
            dup .
            get-opcode
            print-opcode
            cr
        until
    else
        semicolon
    then
    cr drop drop ;
: prog-space ( -- ) \ printout program space
    h@ 0 do
        i dup .
        p@ dup .
        o2w cr
    loop ;

\ Examples of use
prog-space
' dis o2p
' dis dis
' print-opcode dis
20032 dis
bye

\ vim: ft=forth
