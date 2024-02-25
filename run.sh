#!/usr/bin/env bash

if gcc panasonic-kb.c -o panasonic-kb -O3 -Werror -fanalyzer
then
  ./panasonic-kb
fi