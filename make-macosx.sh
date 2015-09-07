#!/bin/bash

# Combine the dylibs into a single file ("cgame_mac") that ET reads

X86DIR=./build/release-darwin-x86/seals-et
X64DIR=./build/release-darwin-x86_64/seals-et
PPCDIR=./build/release-darwin-ppc/seals-et
OUTDIR=./build/release-darwin-ub/seals-et


mkdir -p "$OUTDIR"

lipo -create -o $OUTDIR/cgame_mac $X86DIR/cgame.mp.i386.dylib $X64DIR/cgame.mp.x86_64.dylib $PPCDIR/cgame.mp.ppc.dylib
lipo -create -o $OUTDIR/qagame_mac $X86DIR/qagame.mp.i386.dylib $X64DIR/qagame.mp.x86_64.dylib $PPCDIR/qagame.mp.ppc.dylib
lipo -create -o $OUTDIR/ui_mac $X86DIR/ui.mp.i386.dylib $X64DIR/ui.mp.x86_64.dylib $PPCDIR/ui.mp.ppc.dylib
