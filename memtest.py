#!/usr/bin/env python

import sys
from intelhex import IntelHex
from myhdl import intbv


# memtest.py code.hex memdump.txt

#
# helpers
#

def str2hex(x):
    return ''.join(map(lambda h: '%02x' % ord(h), x))

def int2str(value, width):
    if (width % 8) != 0:
        raise Exception, 'width must be multiple of 8'
    v = intbv(value, max=2**width)
    b = [v[i:i-8] for i in range(width, 0, -8)]
    return ''.join(map(chr, b))


h = IntelHex(sys.argv[1])



#code space in RAM, only get/send used block of bytes
a = [a for a in h.addresses() if (a >= 0x4000 and a < 0xff00)]
alen = max(a) - min(a) + 1
#alen = 32
code = h.gets(0x4000, alen)

# 0xffd0 -- 0xffff are interrupt vectors
# gcc et.al. for some reason do not give a value for 0xfffe word
#   the flash loader in ROM jumps to addr pointed here
#   force to start of RAM
#
# the flash uses 256-byte pages, include code from 0xff00 - 0xffcf
# as it was excluded above
vectors = ''.join([chr(h[i]) for i in range(0xff00, 0x10000)])


dump = []
for line in open(sys.argv[2]):
    s = line.split()
    v = s[1].strip()
    i = int(v, 16)
    ss = int2str(i, 16)
    dump.append(ss[1])
    dump.append(ss[0])

#print dump
#d2 = ''.join([int2str(d, 16).reverse() for d in dump])
print len(code), len(dump)
if 0:
    for i in range(len(code)):
        print i, code[i] == dump[i]

print 'verified:', all([code[i] == dump[i] for i in range(len(code))])

#print str2hex(code[:4])
#print str2hex(c2[:4])

