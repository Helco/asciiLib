#!/bin/bash
emcc -s EXPORTED_FUNCTIONS="['_main']" --shell-file ./shell.html --js-library ../../asciiLib.js -o ./index.html -I ../../ ./*.c ../../asciiLib.c
