#!/bin/bash
valgrind --tool=memcheck --leak-check=yes --undef-value-errors=no --log-file=valgrind.log ./servtest
