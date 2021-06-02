#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on mon 22 jul 2020 21:25:40 WEST

@author: af.rosa
"""

import parse

def parse_line(line):
    pattern = '<{0}> TIME: {1} :: [{2}] : [{3}] {4}\n'
    result = parse.parse(pattern, line)
    if(result != None):
        values = list(result)
        return values
    else:
        return None
    # if tag in line:
        # time = line[line.index("TIME: ")+len("TIME: "):line.index(" :: ")]
        # msg = line[line.index(tag)+len(tag)+1:]
        # return time, msg
    # return None,None


with open("../output/flooding3.txt", "r") as file:
    for line in file:
        parsed_line = parse_line(line)
        if(parsed_line != None):
            if("LATENCY" in parsed_line[3]):
                #print(parsed_line[4])
            if("RECEIVED MESSAGE" in parsed_line[3]):

            if('Broadcast Framework' in parsed_line[2] and 'POLICY' in parsed_line[3]):
