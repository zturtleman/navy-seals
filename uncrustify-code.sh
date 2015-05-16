#!/bin/sh

uncrustify -c uncrustify.cfg --no-backup --mtime code/ui/*.c
uncrustify -c uncrustify.cfg --no-backup --mtime code/ui/*.h
uncrustify -c uncrustify.cfg --no-backup --mtime code/game/*.c
uncrustify -c uncrustify.cfg --no-backup --mtime code/game/*.h
uncrustify -c uncrustify.cfg --no-backup --mtime code/cgame/*.c
uncrustify -c uncrustify.cfg --no-backup --mtime code/cgame/*.h


