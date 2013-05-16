\ vim: ft=forth
: fail ( -- ) 0x46 emit 0x41 emit 0x49 emit 0x4c emit cr ;
: star ( -- ) 0x2a emit ;
: <sp> ( -- ) 0x20 emit ;
: cmp ( a b -- ) == not if fail then ;
: tloop do i star <sp> . cr loop ;
bye
