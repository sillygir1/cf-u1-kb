#!/usr/bin/env bash

if gcc panasonic-kb.c -o panasonic-kb -Werror -fanalyzer
then
  ./panasonic-kb
fi