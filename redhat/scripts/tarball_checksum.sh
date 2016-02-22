#!/bin/sh

sha1sum < $1 | cut -d ' ' -f 1
