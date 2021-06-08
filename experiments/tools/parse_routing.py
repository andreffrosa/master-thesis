#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import log_parser
import subprocess
import pandas as pd
import time
import datetime
import parse
import sys


def merge(results, t, r):
    #print(results)

    discovery_overhead = 0
    broadcast_overhead = 0
    routing_overhead = 0
    delivered_pings = 0
    lost_pings = 0
    avg_rtt = 0
    #route_acquisition_time = 0
    #n_routes = 0

    for a in results:
        d_oh, b_oh, r_oh, d_pings, l_pings, rtt = a
        #print("> ", d_oh, b_oh, r_oh)
        discovery_overhead += d_oh
        broadcast_overhead += b_oh
        routing_overhead += r_oh
        delivered_pings += d_pings
        lost_pings += l_pings
        avg_rtt += rtt
        #route_acquisition_time += rat
        #n_routes += r

    if delivered_pings > 0:
        avg_rtt /= delivered_pings
    else:
        avg_rtt = 0

    #route_acquisition_time /= n_routes

    return [discovery_overhead, broadcast_overhead, routing_overhead, delivered_pings, lost_pings, avg_rtt]


def parse_node(t, r, p, test_dir):
    #print("t: %s r: %s p: %s" % (t, r, p))
    #print(test_dir)

    discovery_log = test_dir + "discovery.log"
    broadcast_log = test_dir + "broadcast.log"
    routing_log = test_dir + "routing.log"
    app_log = test_dir + "app.log"



    discovery_overhead = 0
    with open(discovery_log, 'r') as f:
        log_out = f.read().splitlines()
        discovery_overhead = len(log_parser.grep("\[DISCOVERY FRAMEWORK\] \[SEND MESSAGE\]", log_out))
        f.close()
        del log_out
        #log_out = None

    broadcast_overhead = 0
    with open(broadcast_log, 'r') as f:
        log_out = f.read().splitlines()
        broadcast_overhead = len(log_parser.grep("\[BROADCAST FRAMEWORK\] \[RETRANSMIT\]", log_out))
        f.close()
        del log_out
        #log_out = None


    routing_overhead = 0
    with open(routing_log, 'r') as f:
        log_out = f.read().splitlines()
        routing_overhead = len(log_parser.grep("\[ROUTING FRAMEWORK\] \[FORWARD CTRL\]", log_out))
        f.close()
        del log_out
        #log_out = None

    #delivered_pings = 0
    avg_rtt = 0
    lost_pings = 0
    pings = []

    with open(app_log, 'r') as f:
        log_out = f.read().splitlines()
        pings = log_parser.grep("\[PING2 APP\]", log_out)
        f.close()
        del log_out
        #log_out = None

    reps = log_parser.grep("\[PING2 APP\] \[REP\]", pings)
    #print(reps)

    for x in reps:
        a = log_parser.parse_line(x)
        #print(a[4])
        pattern = '[{0}] from {1} : t={2} ms s={3} bytes'
        result = parse.parse(pattern, a[4])
        #print(result)
        if(result != None):
            values = list(result)
            #return values
            avg_rtt += int(values[2])

    delivered_pings = len(reps)

    if delivered_pings > 0:
        avg_rtt /= delivered_pings
    else:
        avg_rtt = 0
    #print(delivered_pings, avg_rtt)


    timeouts = log_parser.grep("\[PING2 APP\] \[TIMEOUT\]", pings)
    lost_pings = len(timeouts)

    """
    route_acquisition_time = 0
    n_routes = 0
    try:
        res = subprocess.check_output(["grep", "\[ROUTING FRAMEWORK\] \[ROUTING TABLE\] :: Added route to", routing_log], text=True, universal_newlines=True)
        #pint(res)
        l = [x for x in res.split('\n') if x.strip()]
        #print(len(l))
        for r in l:
            a = log_parser.parse_line(r)
            #print(a)

            t2 = timestamp = log_parser.parse_time(a[1])

            destination_id = a[4].replace("Added route to ", "")

            #print(t2, destination_id)

            dt = 0
            try:
                #res = subprocess.check_output(["grep", "\[ROUTING FRAMEWORK\] \[UNKNOWN ROUTE\]", routing_log], text=True, universal_newlines=True)
                #l2 = [x for x in res.split('\n') if x.strip()]

                res = subprocess.check_output(["egrep", "RREQ|RREP", routing_log], text=True, universal_newlines=True)
                l2 = [x for x in res.split('\n') if x.strip()]
                print(l2)
                quit()

                dt2 = 99999999999999999
                selected = False
                for r2 in l2: # find the closest

                    a2 = log_parser.parse_line(r)
                    #print(a)
                    t1 = timestamp = log_parser.parse_time(a2[1])

                    process = False
                    dest_id = None
                    src_id = None




                    pattern = '[{0}] to destination {1}'
                    result = parse.parse(pattern, a2[4])
                    #print(result)
                    if(result != None):
                        values = list(result)
                        #return values
                        dest_id = result[1]

                        if dest_id == destination_id:
                            # find the closest
                            if t1 <= t2:
                                dt2 = min(dt2, t2-t1)
                                selected = True

                if selected:
                    dt = dt2
                #else:
                #    dt = 0

                print("uisng RREQ")
            except:
                print("No RREQ")
                try:
                    res = subprocess.check_output(["grep", "\[ROUTING FRAMEWORK\] \[INIT\]", routing_log], text=True, universal_newlines=True)
                    #print(res)
                    t_init = log_parser.parse_time(log_parser.parse_line(res)[1])

                    dt = t2 - t_init

                    print("uisng init")
                except:
                    print("Init not found!")
                    #quit()

            route_acquisition_time += dt

        #if len(l) > 0:
        #    route_acquisition_time /= len(l)
        n_routes = len(l)
    except:
        print("No routes added!")
    """

    return [discovery_overhead, broadcast_overhead, routing_overhead, delivered_pings, lost_pings, avg_rtt]


###########

def on_init(tests,runs,pis):
    print("Init Routing")

    columns=['alg', 'discovery_overhead', 'broadcast_overhead', 'routing_overhead', 'delivered_pings', 'lost_pings', 'avg_rtt']

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


#logs_dir = "/home/andreffrosa/Projects/master-thesis/experiments/output/routing/routing_tests_2021.05.26-22.43.03/"
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
