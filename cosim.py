#!/usr/bin/env python

import os
import sys
import signal
import time

import pexpect


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


def print_side(a, b):
    alines = [x.rstrip() for x in a.split('\n')]
    blines = [x.rstrip() for x in b.split('\n')]

    alen = len(alines)
    blen = len(blines)

    alines.extend([''] * max(blen - alen, 0))
    blines.extend([''] * max(alen - blen, 0))

    for (aline, bline) in zip(alines, blines):
        if aline == bline:
            eq = '='
        else:
            eq = '!'

        s = '%-80s %s %-80s' % (aline, eq, bline)
        print s

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
    print_side(' '.join(atoi.args), ' '.join(pc.args))
    for line in open('tests.4th'):
        s = line.rstrip()
        s += '\r'

        for n in (pc, atoi):
        #for n in (pc, ):
            send(n, s)
            if s.startswith('bye'):
                #things don't match after this
                break
            prompt(n)

        if 0:
            print
            print 's   :', s.rstrip()
            print 'pc  :', pc.before
            print 'atoi:', atoi.before

        print_side(atoi.before, pc.before)


except pexpect.TIMEOUT:
    print '** TIMEOUT **'
    cleanup()

#interact(pc)
#interact(atoi)

cleanup()
