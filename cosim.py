#!/usr/bin/env python2

import os
import sys
import signal
import time

import pexpect


if len(sys.argv) > 1:
    cmdfile = sys.argv[1]
else:
    cmdfile = 'tests.4th'

atoi = pexpect.spawn('miniterm.py',
        ['--rts=0', '/dev/ttyUSB3', '4800'],
        logfile=open('ns430.log', 'w'),
        timeout=5)

pc = pexpect.spawn('./pc4th',
        logfile=open('pc4th.log', 'w'),
        timeout=5)

time.sleep(0.2)
atoi.send(r'')
time.sleep(0.1)
atoi.send(r'')
time.sleep(0.1)

def cleanup():
    os.system('killall _pc4th > /dev/null 2>&1')
    atoi.close(force=True)
    pc.close(force=True)


def print_side(a, b, linenum=None, compare=True):
    if linenum is None:
        fmt = '%i%-80s %s %-80s'
    else:
        fmt = '%03i: %-80s %s %-80s'

    alines = [x.rstrip() for x in a.split('\n')]
    blines = [x.rstrip() for x in b.split('\n')]

    alen = len(alines)
    blen = len(blines)

    alines.extend([''] * max(blen - alen, 0))
    blines.extend([''] * max(alen - blen, 0))

    results = []
    for (aline, bline) in zip(alines, blines):
        eq = '='
        results.append(True)
        if compare and aline != bline:
            eq = '!'
            results[-1] = False

        s = fmt % (linenum, aline, eq, bline)
        print s

    return all(results)

def prompt(p):
    p.expect([r'\r\n>', pexpect.EOF])

def send(p, s):
    p.send(s)

def interact(p):
    print '*** Interacting with:', ' '.join(p.args)
    print '*** escape is ^]'
    p.interact()


try:
    # side-by-side output
    prompt(atoi)
    prompt(pc)
    print_side(' '.join(atoi.args),
            ' '.join(pc.args),
            linenum=0,
            compare=False)
    linenum = 1
    results = []
    for line in open('tests.4th'):
        if line.startswith('bye'):
            break

        s = line.rstrip()
        s += '\r'

        # .after is the prompt before which send() acts on
        a0 = atoi.after
        a1 = pc.after
        for n in (pc, atoi):
            send(n, s)
            prompt(n)

        same = print_side(str(a0.lstrip()) + atoi.before,
                str(a1.lstrip()) + pc.before,
                linenum)

        if not same:
            results.append(linenum)
            print linenum#; break

        linenum += 1

    if results:
        print 'Failing line numbers:'
        print results
    else:
        print 'All lines match.'

except pexpect.TIMEOUT:
    print '** TIMEOUT **'
    cleanup()

#interact(pc)
#interact(atoi)

cleanup()
