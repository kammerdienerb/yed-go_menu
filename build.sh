#!/usr/bin/env bash
gcc -o go_menu.so go_menu.c $(yed --print-cflags --print-ldflags)
