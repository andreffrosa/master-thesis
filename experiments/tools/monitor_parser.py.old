#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on mon 20 jul 2020 21:25:40 WEST

@author: af.rosa
"""

import parse

def csv_line(l, delim=','):
    line = ''
    for x in l:
        line += x + delim
    return line

with open("../output/flooding.txt", "r") as file:
    tags = ['stability', 'density', 'in_traffic', 'out_traffic', 'misses']
    print(tags)
    delim = "[LOG]"

    for line in file:
        if "[BROADCAST APP] : [MAIN LOOP] END." in line:
            break;

        if delim in line:
            index = line.index(delim)+len(delim)+1
            clean_line = line[index:]
            pattern = 'stability: {0} changes/s density: {1} neighs in_traffic: {2} msgs/s out_traffic: {3} msgs/s misses {4}\n'
            result = parse.parse(pattern, clean_line)
            values=list(result)
            print(values)
