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

    msgs = {}
    n_nodes = 0

    for [p, node_msgs] in results:
        #print(p)
        n_nodes += 1
        for m in node_msgs:
            msg_id = m[0]

            if msg_id in msgs:
                old_msg = msgs[msg_id]
                timestamp =  m[2] if m[1] else old_msg[0]

                deliveries = old_msg[1]
                deliveries.append(p)

                retransmitions = old_msg[2] + m[3]
                latency = max(old_msg[3], m[4])

                msgs[msg_id] = [timestamp, deliveries, retransmitions, latency]
            else:
                msgs[msg_id] = [m[2], [p], m[3], m[4]]

    total_bcast = len(msgs)
    cost = 0
    reliability = 0
    latency = 0

    deliveries_per_msg = [0] * (n_nodes+1)

    for msg_id, m in msgs.items():
        #print(msg_id, m)

        reliability += len(m[1]) / n_nodes
        cost += m[2]
        latency += m[3]

        deliveries_per_msg[len(m[1])] += 1


    reliability /= len(msgs)
    cost /= len(msgs)
    latency /= len(msgs)

    deliveries_per_msg = [x/len(msgs) for x in deliveries_per_msg]

    #print(deliveries_per_msg)

    return [total_bcast, reliability, cost, latency] + deliveries_per_msg

def parse_node(t, r, p, test_dir):

    #print("parse_node " + p)

    #t_start = datetime.datetime.now()

    log = test_dir + "broadcast.log"

    #msgs = []

    deliveries_out = None
    requests_out = None
    retransmitions_out = None
    latencies_out = None
    with open(log, 'r') as f:
        log_out = f.read().splitlines()
        deliveries_out = log_parser.grep("\[BROADCAST FRAMEWORK\] \[DELIVER\]", log_out)
        requests_out = log_parser.grep("\[BROADCAST FRAMEWORK\] \[BROADCAST REQ\]", log_out)
        retransmitions_out = log_parser.grep("\[BROADCAST FRAMEWORK\] \[RETRANSMIT\]", log_out)
        latencies_out = log_parser.grep("\[BROADCAST FRAMEWORK\] \[LATENCY\]", log_out)
        f.close()
        del log_out
        #log_out = None

    msg_ids = [log_parser.parse_line(x)[4].replace("[", "").replace("]","") for x in deliveries_out if x.strip()]
    #print(msg_ids)
    #ms = dict.fromkeys(msg_ids)
    ms = {mid: {"req": False, "timestamp": 0, "retransmitions": 0, "latency": 0} for mid in msg_ids}
    #print(ms)

    #print(len(msg_ids), len(requests_out))

    for req in requests_out:
        a = log_parser.parse_line(req)
        mid = a[4][1:37]
        timestamp = log_parser.parse_time(a[1])

        #print(mid)
        ms[mid]["req"] = True
        ms[mid]["timestamp"] = timestamp

    for ret in retransmitions_out:
        a = log_parser.parse_line(ret)
        mid = a[4][1:-1]

        if ms[mid].get("retransmitions") != None:
            ms[mid]["retransmitions"] += 1
        else:
            ms[mid]["retransmitions"] = 1

    for lat in latencies_out:
        a = log_parser.parse_line(lat)
        aux = a[4].split(" ")

        mid = aux[0][1:-1]
        ms[mid]["latency"] = int(aux[1])

    msgs = []
    for mid, m in ms.items():
        req = True if m.get("req") != None else False
        timestamp = m.get("timestamp") if m.get("timestamp") != None else 0
        retransmitions = m.get("retransmitions") if m.get("retransmitions") != None else 0
        latency = m.get("latency") if m.get("latency") != None else 0
        #print([mid, req, timestamp, retransmitions, latency], m)
        #quit()
        msgs.append([mid, req, timestamp, retransmitions, latency])

    #for mx in deliveries_out:
    #    m = log_parser.parse_line(mx)[4].replace("[", "").replace("]","")
    #    #print(m)

    #    req = False
    #    timestamp = 0
    #    res = log_parser.grep(m, requests_out)
    #    if len(res) > 0:
    #        #print(res)
    #        req = True
    #        a = log_parser.parse_line(res[0])[1]
    #        timestamp = log_parser.parse_time(a)

    #        #print(len(requests_out))
    #        requests_out = [x for x in requests_out if x not in res]
    #        #print(len(requests_out))


    #    res = log_parser.grep(m, retransmitions_out)
    #    retransmitions = len(res)
    #    retransmitions_out = [x for x in retransmitions_out if x not in res]

    #    latency = 0
    #    res = log_parser.grep(m, latencies_out)
    #    if len(res) > 0:
    #        a = log_parser.parse_line(res[0])
    #        latency = int(a[4][38:])

    #        latencies_out = [x for x in latencies_out if x not in res]

        #print("id = %s req = %s timestamp = %d retransmitions = %d latency = %d" % (m, req, timestamp, retransmitions, latency))

    #    msgs.append([m, req, timestamp, retransmitions, latency])

    #t_end = datetime.datetime.now()
    #elapsed_ms = int((t_end - t_start).total_seconds() * 1000)
    #print("elapsed_ms = %d ms" % (elapsed_ms))

    #print(p, msgs)

    return [p, msgs]

def on_init(tests,runs,pis):
    print("Init Bcast")

    columns = ['alg', 'total_bcast', 'delivery_ratio', 'cost', 'latency']
    save = False

    normal = pd.DataFrame(columns=columns)

    columns2 = ['alg'] + [str(x/len(pis)) for x in range(0,len(pis)+1)]
    deliveries_per_msg = pd.DataFrame(columns=columns2)

    return {"normal": normal, "deliveries_per_msg": deliveries_per_msg}

def on_test_results(t, i,res,args):
    #print(res)

    normal = res[0:4]
    deliveries_per_msg = res[4:]

    args["normal"].loc[i] = [t] + normal
    args["deliveries_per_msg"].loc[i] = [t] + deliveries_per_msg

def on_finish(args,output_dir,save):
    print("Finish")

    print(args["normal"].to_string(index=False))
    print(args["deliveries_per_msg"].to_string(index=False))

    if save:
        args["normal"].to_csv(output_dir + "normal" + ".csv", sep=";", index=False)
        args["deliveries_per_msg"].to_csv(output_dir + "deliveries_per_msg" + ".csv", sep=";", index=False)


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
