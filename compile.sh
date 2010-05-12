#!/bin/bash
gcc main.c -o playfile `pkg-config sdl --libs --static --cflags`
