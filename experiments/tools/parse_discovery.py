#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import log_parser
import subprocess
import pandas as pd
import time
import datetime
import sys

def merge(results, t, r):
    #print(results)

    total_cost = 0
    total_piggybacked = 0
    total_latency = 0

    # link_latency = max(t1, t2) - max(init, neigh_init)
    #print("link_latency=" + str(link_latency))
    #total_latency += link_latency

    links = {} #dic

    for a in results:
        total_cost += a[2]
        total_piggybacked += a[3]

        for b in a[4]:
            key = a[0] + b[0] if a[0] > b[0] else b[0] + a[0]

            x = links.get(key)
            if x:
                #print(x)
                links[key] = [x[0], x[1], a[1], b[1]]
            else:
                links[key] = [a[1], b[1], 0, 0]

    for k,l in links.items():
        link_latency = max(l[1], l[3]) - max(l[0], l[2])
        total_latency += link_latency

        #print(k, l, link_latency, "ms")

    if len(links) > 0:
        total_latency /= len(links)

    return [total_cost, total_piggybacked, total_latency]

def parse_node(t, r, p, test_dir):

    log = test_dir + "discovery.log"
    #print(log)

    init_out = None
    send_out = None
    piggy_out = None
    bi_out  = None
    with open(log, 'r') as f:
        log_out = f.read().splitlines()
        init_out = log_parser.grep("\[DISCOVERY FRAMEWORK\] \[INIT\]", log_out)
        send_out = log_parser.grep("\[DISCOVERY FRAMEWORK\] \[SEND MESSAGE\]", log_out)
        piggy_out = log_parser.grep("\[DISCOVERY FRAMEWORK\] \[PIGGYBACK MESSAGE\]", log_out)
        bi_out = log_parser.grep("\[DISCOVERY FRAMEWORK\] \[BECAME BI\]", log_out)
        f.close()
        del log_out
        #log_out = None

    init_t = log_parser.parse_time(log_parser.parse_line(init_out[0])[1])
    #print(init_t)

    cost = len(send_out)
    piggybacked = len(piggy_out)

    neighs = []
    for neigh_line in bi_out:
        #print(neigh_line)
        a = log_parser.parse_line(neigh_line)
        t1 = log_parser.parse_time(a[1])
        neigh_id = a[4]
        raspi = "raspi-" + neigh_id[-2] + neigh_id[-1]

        #print(p, init_t, t1)
        assert init_t <= t1

        neighs.append([raspi, t1])

    return [p, init_t, cost, piggybacked, neighs]

def on_init(tests,runs,pis):
    print("Init Discovery")

    columns=['alg', 'total_cost', 'piggybacked', 'avg_latency']
    save = False

    normal = pd.DataFrame(columns=columns)

    return {"normal": normal}

def on_test_results(t, i,res,args):
    #print(res)

    normal = res

    args["normal"].loc[i] = [t] + normal

def on_finish(args,output_dir,save):
    print("Finish")

    print(args["normal"].to_string(index=False))

    if save:
        args["normal"].to_csv(output_dir + "normal" + ".csv", sep=";", index=False)

######################################################################3

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
