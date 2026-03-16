#!/bin/python3

import sys
import re

if len(sys.argv) < 2:
    sys.exit(f'Usage: {sys.argv[0]} <filename>')

with open(sys.argv[1], mode='r', encoding='utf-8') as changelog:
    res = re.search(r"^\#\# \[\d+\.\d(?:\.\d)*\].*(?:\n*\#\#\# (?:Added|Changed|Fixed)(?:\n*- .*)*)*", changelog.read(), flags=re.M)
    print(res.group(0))
