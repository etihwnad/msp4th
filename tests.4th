\ vim: ft=forth
: ..
    dup . ;

: fail ( -- )
    0x46 emit 0x41 emit 0x49 emit 0x4c emit cr ;

: cmp ( a b -- )
    == not if fail s. then ;

: scmp ( x*2n n -- ) \ verify stack contents match
    0 swap do
    i roll cmp
    -1 +loop ;
5 6 7 8 5 6 7 8 4 scmp

\    case  1: // bye
\ exit interpreter, not testable

\    case  2: // +  ( a b -- a+b )
32767 1 +
-32768 cmp

\    case  3: // -  ( a b -- a-b )
-32768 1 -
32767 cmp

\    case  4: // *  ( a b -- reshi reslo )
32767 1 *
0 32767 2 scmp

\    case  5: // /%  ( a b -- a/b a%b )
1000 3 /%
333 1 2 scmp

\    case  6: // .  ( a -- )
0x8000 .
depth 0 cmp

\    case  7: // dup  ( a -- a a )
42 dup
42 42 2 scmp

\    case  8: // drop  ( a -- )
15 16 drop
15 cmp

\    case  9: // swap  ( a b -- b a )
99 100 swap
100 99 2 scmp

\    case 10: // <  ( a b -- a<b )
1 2 <
1 cmp

-1 0 <
1 cmp

-32768 32767 <
1 cmp

32767 -32768 <
0 cmp

\    case 11: // >  ( a b -- a>b )
1 2 >
0 cmp

-1 -2 >
1 cmp

\    case 12: // ==  ( a b -- a==b )
1 dup ==
1 cmp

53 64 ==
0 cmp

\    case 13: // hb.  ( a -- )
: hw2b dup 8 0 do /2 loop hb. hb. ;
0xabcd hw2b

\    case 14: // gw  ( -- ) \ get word from input
: test-gw gw pwrd ;
test-gw theword

\    case 15: // dfn  ( -- ) \ create opcode and store word to cmdList
\ TODO
?

\    case 16: // abs  ( a -- |a| ) \ -32768 is unchanged
-32768 abs
-32768 cmp

1 abs
1 cmp

-1 abs
1 cmp

\    case 17: // ,  ( opcode -- ) \ push opcode to prog space
20056 ,
h@ 1 - p@
20056 cmp

\    case 18: // p@  ( opaddr -- opcode )
\    case 19: // p!  ( opcode opaddr -- )
20006 h@ p!
h@ p@
20006 cmp

\    case 20: // not  ( a -- !a ) \ logical not
1 not
0 cmp

-1 not
0 cmp

0 not
1 cmp

\    case 21: // list  ( -- ) \ show defined words
list

\    case 22: // if  ( flag -- )
\    case 23: // then      ( trapped in ':')
\    case 24: // else      ( trapped in ':')
: if-not ( flag -- !flag )
    if push0 else push 1 then ;
1 if-not
0 cmp

0 if-not
1 cmp

\    case 25: // begin  ( -- ) ( -a- pcnt )
\    case 26: // until  ( flag -- ) ( addr -a- )
: test-begin-until ( n -- )
    push0
    swap
    begin
        push1 - ..
        swap .. cr push1 + swap
        dup push0 ==
    until drop drop ;
20 test-begin-until

\    case 27: // depth  ( -- n ) \ math stack depth
0 0 0 0 0
depth
5 cmp
depth ndrop

\    case 28: // h.  ( a -- )
0xcafe 0xbeef h. h.

\    case 29: // ] ( trapped in interp )
\ no test

\    case 30: // num  ( -- n flag ) \ is word in buffer a number?
: test-num
    gw num ;
test-num 235
235 1 2 scmp

\    case 31: // push0  ( -- 0 )
push0 0 cmp

\    case 32: // goto   ( for internal use only )
\ no test

\    case 33: // exec  ( opcode -- )
20043 exec
h@ cmp

\    case 34: // lu  ( -- opcode 1 | 0 )
: test-lu
    gw lu ;

test-lu foo
0 cmp

test-lu lu
20034 1 2 scmp

\    case 35: // pushn   ( internal use only )
\ no test

\    case 36: // over  ( a b -- a b a )
1 2 over
1 2 1 3 scmp

\    case 37: // push1  ( -- 1 )
push1 1 - push0 cmp

\    case 38: // pwrd  ( -- ) \ print word buffer
: test-pwrd
    gw pwrd ;
test-pwrd bar

\    case 39: // emit  ( c -- )
0x5b emit 0x60 emit 0x73 emit 0x75 emit 0x70 emit 0x5d emit cr

\    case 40: // ;  ( pcnt -a- ) \ return from inner word
\ no test

\    case 41: // @  ( addr -- val ) \ read directly from memory address
\    case 42: // !  ( val addr -- ) \ write directly to memory address words only!
42 0xff00 !
0xff00 @
42 cmp

\    case 43: // h@  ( -- progIdx ) \ get end of program code space
\ no test

\    case 44: // do  ( limit cnt -- ) ( -a- limit cnt pcnt )
\    case 45: // loop  ( -- ) ( limit cnt pcnt -a- | limit cnt+1 pcnt )
\    case 46: // +loop  ( n -- ) ( limit cnt pcnt -a- | limit cnt+n pcnt ) \ decrement loop if n<0
\    case 47: // i  ( -- cnt ) \ loop counter value
\    case 48: // j  ( -- cnt ) \ next outer loop counter value
\    case 49: // k  ( -- cnt ) \ next next outer loop counter value
\    case 50: // ~  ( a -- ~a ) \ bitwise complement
\    case 51: // ^  ( a b -- a^b ) \ bitwise xor
\    case 52: // &  ( a b -- a&b ) \ bitwise and
\    case 53: // |  ( a b -- a|b ) \bitwise or
\    case 54: // */  ( a b c -- (a*b)/c ) \ 32b intermediate
\    case 55: // key  ( -- c ) \ get a key from input .... (wait for it)
\    case 56: // cr  ( -- )
\    case 57: // 2*  ( a -- a<<1 )
\    case 58: // 2/  ( a -- a>>1 )
\    case 59: // call0  ( &func -- *func() )
\    case 60: // call1  ( a &func -- *func(a) )
\    case 61: // call2  ( a b &func -- *func(a,b) )
\    case 62: // call3  ( a b c &func -- *func(a,b,c) )
\    case 63: // call4  ( a b c d &func -- *func(a,b,c,d) )
\    case 64: // ndrop  ( (x)*n n -- ) \ drop n math stack cells
\    case 65: // swpb  ( n -- n ) \ byteswap TOS
\    case 66: // +!  ( n addr -- ) \ *addr += n
\    case 67: // roll  ( n -- ) \ nth stack removed and placed on top
\    case 68: // pick  ( n -- ) \ nth stack copied to top
\    case 69: // tuck  ( a b -- b a b ) \ insert copy TOS to after NOS
\    case 70: // max  ( a b -- c ) \ c = a ? a>b : b
\    case 71: // min  ( a b -- c ) \ c = a ? a<b : b
\    case 72: // s.  ( -- ) \ print stack contents, TOS on right
\    case 73: // sh.  ( -- ) \ print stack contents in hex, TOS on right
\    case 74: // neg  ( a -- -a ) \ twos complement
\    case 75: // echo  ( bool -- ) \ ?echo prompts and terminal input?
\    case 76: // init  ( &config -- ) \ clears buffers and calls msp4th_init
\    case 77: // o2w  ( opcode -- ) \ leaves name of opcode in wordBuffer
\    case 78: // o2p  ( opcode -- progIdx ) \ lookup opcode definition, 0 if builtin

bye
