#!/usr/bin/python

import re
import sys

def filter_file(inf):
    ts_chunkstart = re.compile("diff --git ")
    ts_redhat = re.compile(".*\/redhat\/")
    ts_gitfile = re.compile(".*\/\.git")
    skip = False
    f = open(inf,"r")
    for line in f.readlines():
        if ts_chunkstart.match(line) is not None:
            if ts_redhat.match(line) is not None or ts_gitfile.match(line) is not None:
                skip = True
            else:
                skip = False
        if skip == False:
            sys.stdout.write(line)
    f.close()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: %s <filename>" % sys.argv[0])
        exit(1)
    filter_file(sys.argv[1])

