#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import division

import log_parser
import subprocess
import pandas as pd
import time
import datetime
import sys

links = {} # dic

def merge(results, t, r):
    #print(results)

    for [p, neighbors] in results:
        #print(p, neighbors)

        for n in neighbors:
            qaz = p < n[0]
            key = p + n[0] if qaz else n[0] + p
            x = links.get(key)
            if x:
                #print(x)
                if qaz:
                    assert x[0] == p and x[1] == n[0]
                    links[key] = [x[0], x[1], n[1], n[2], x[4], x[5], x[6] and n[3], x[7] or n[4], x[8] or n[5]]
                else:
                    assert x[1] == p and x[0] == n[0]
                    links[key] = [x[0], x[1], x[2], x[3], n[1], n[2], x[6] and n[3], x[7] or n[4], x[8] or n[5]]
            else:
                if qaz:
                    links[key] = [p, n[0], n[1], n[2], 0, 0, n[3], n[4], n[5]]
                else:
                    links[key] = [n[0], p, 0, 0, n[1], n[2], n[3], n[4], n[5]]

    return [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

def parse_node(t, r, p, test_dir):

    #print("parse_node " + p)

    #t_start = datetime.datetime.now()

    discovery_log = test_dir + "discovery.log"
    neighbors_log = test_dir + "neighbors.log"

    discovery_out = None
    neighbors_out = None

    with open(discovery_log, 'r') as f:
        discovery_out = f.read().splitlines()
        f.close()

    with open(neighbors_log, 'r') as f:
        neighbors_out = f.read().splitlines()
        f.close()

    aux = log_parser.grep("\[DISCOVERY FRAMEWORK\] \[NEW NEIGHBOR\]", discovery_out)
    found_neighbors = set([log_parser.parse_line(x)[4].replace("[", "").replace("]","") for x in aux if x.strip()])

    neighbors = []

    for n in found_neighbors:
        # Bi
        aux = log_parser.grep("\[DISCOVERY FRAMEWORK\] \[BECAME BI\] :: " + n, discovery_out)
        bi = len(aux) > 0

        # Lost Bi
        aux = log_parser.grep("\[DISCOVERY FRAMEWORK\] \[LOST BI\] :: " + n, discovery_out)
        lost_bi = len(aux) > 0

        # Lost
        aux = log_parser.grep("\[DISCOVERY FRAMEWORK\] \[LOST NEIGHBOR\] :: " + n, discovery_out)
        lost = len(aux) > 0

        # RX LQs
        aux = log_parser.grep(n + "   b8", neighbors_out)
        aux = [[y for y in x.split(" ") if y.strip()][10] for x in aux if x.strip()]
        lqs = [float(x) if "-" not in x else 0.0 for x in aux]

        max_lq = max(lqs)
        #min_lq = min(lqs)
        #avg_lq = 0.0 if len(lqs) == 0 else sum(lqs)/len(lqs)
        last_lq = lqs[-1]

        neigh_host = "raspi-" + n[-2] + n[-1]

        neighbors.append([neigh_host, max_lq, last_lq, bi, lost_bi, lost])

    return [p, neighbors]

def on_init(tests,runs,pis):
    print("Init Graph")

    columns = ['node1', 'node2', 'max lq1', 'last lq1', 'max lq2', 'last lq2', 'became bi', 'lost bi', 'lost']
    save = False

    normal = pd.DataFrame(columns=columns)

    return {"normal": normal}

def on_test_results(t, i,res,args):
    #print(res)
    #args["normal"].loc[i] = [t] + res

    j = i
    for k,v in links.items():
        args["normal"].loc[j] = v
        j += 1

def on_finish(args,output_dir,save):
    print("Finish")

    args["normal"].sort_values(["node1", "node2"])

    print(args["normal"].to_string(index=False))

    if save:
        print(output_dir + "graph" + ".csv")
        args["normal"].to_csv(output_dir + "graph" + ".csv", sep=";", index=False)

#logs_dir = "/home/andreffrosa/Projects/master-thesis/broadcast_tests_2021.05.25-21.06.37/"
#output_dir = logs_dir

if len(sys.argv) < 2:
    print("No args passed!\nUsage: " + sys.argv[0] + " <logs_dir> [save] [output_dir]")
    quit()

logs_dir = sys.argv[1]

if not logs_dir.endswith("/"):
    logs_dir += "/"

save = False
if len(sys.argv) >= 2:
    #save = sys.argv[2] == "True" || sys.argv[2] == "true" || sys.argv[2] == "T"
    save = sys.argv[2].lower() in ['true', '1', 't', 'y', 'yes', 'ok', 'save']


output_dir = logs_dir
if len(sys.argv) >= 3:
    output_dir = sys.argv[3]
    if not output_dir.endswith("/"):
        output_dir += "/"

log_parser.parse_experience(logs_dir, output_dir, on_init, on_test_results, on_finish, parse_node, merge, save)
