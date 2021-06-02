#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import subprocess
import pandas as pd
import parse
import time
import datetime
import concurrent.futures

import os

import sys
#from tempfile import NamedTemporaryFile
import re
#import glob

def grep(pattern,input):
    res = []
    for line in input:
        if re.search(pattern, line):
            res.append(line)
    return res

def parse_time(l):
    #l = log_parser.parse_line(line)[1]

    aux = l.split(' ')
    aux2 = aux[0] + " " + aux[1] #+ " " + str( round(int(aux[2]) / 1000) )

    #print("a: " + l)
    #print("b: " +  str( round(int(aux[2]) / 1000) ) )
    #print("c: " + aux2)

    el = datetime.datetime.strptime(aux2,"%Y:%m:%d %H:%M:%S")
    timestamp = int(round( el.timestamp()*1000 + int(aux[2])/1000000 )) # milli
    #print(timestamp)
    #tuple = el.timetuple()
    #timestamp = int(time.mktime(tuple))
    return timestamp


def parse_line(line):
    pattern = '{0}> [{1}] [{2}] [{3}] :: {4}'
    result = parse.parse(pattern, line)
    if(result != None):
        values = list(result)
        return values
    else:
        return None

def merge(results):
    return results[0]

def parse_node(t, r, p, test_dir):
    print("t: %s r: %s p: %s" % (t, r, p))
    print(test_dir)


def parse_run(t, r, pis, input_dir, parse_node, merge, n_threads):

    #results = []
    #for p in pis:
    #    test_dir = input_dir + p + "/" + t + "-run" + r + "/"
    #    res = parse_node(t, r,  p, test_dir)
    #    results.append(res)

    results = []

    processes = []
    thread_args = [(t,r,p,input_dir + p + "/" + t + "-run" + r + "/") for p in pis]
    #print(thread_args)
    with concurrent.futures.ThreadPoolExecutor(max_workers=n_threads) as executor:
        #processes.append(executor.map(lambda p: parse_node(*p), thread_args))

        processes = [executor.submit(parse_node, *vars) for vars in thread_args]
        #for out in concurrent.futures.as_completed(processes):
        #    print(out.result())

        results = [x.result() for x in processes]

    #for _ in concurrent.futures.as_completed(processes):
        #print('Result: ', _.result())
        #results.append(_.result())

    #print(len(results))

    row = merge(results)

    #print(row)

    return row


def parse_test(t, pis, runs, input_dir, parse_node, merge, n_threads):

    print("Processing " + t + " ...")

    all = []
    for r in runs:
        row = parse_run(t, r, pis, input_dir, parse_node, merge, n_threads)

        all.append(row)

    #print(all)

    return [sum(x)/len(runs) for x in zip(*all)] # avg per run

def on_init(tests,runs,pis):
    print("Init")

def on_test_results(t,i,res,args):
    print(res)

def on_finish(args,output_dir,save):
    print("Finish")

def parse_experience(input_dir, output_dir, on_init=on_init, on_test_results=on_test_results, on_finish=on_finish, parse_node=parse_node, merge=merge, save=False):

    # get pis
    #res = subprocess.check_output(["ls " + input_dir], text=True, universal_newlines=True, shell=True)
    #pis = [x for x in res.split('\n') if x.strip()]
    pis = os.listdir(input_dir)
    #print(pis)

    # get tests
    #res = subprocess.check_output(["ls " + input_dir + "/" + pis[0]], text=True, universal_newlines=True, shell=True)
    #aux = [x.split("-run") for x in res.split('\n') if x.strip()]
    #print(aux)

    #tests = set([x[0] for x in aux])
    #print(tests)

    #runs = set([x[1] for x in aux])
    #print(runs)

    tests = set()
    runs = set()

    #asd = os.listdir(input_dir + "/" + pis[0])
    for x in os.listdir(input_dir + "/" + pis[0]):
        a = x.split("-run")
        tests.add(a[0])
        runs.add(a[1])

    #print(tests)
    #print(runs)

    n_threads = min(len(pis), os.cpu_count())
    print("n_threads = " + str(n_threads))

    #df = pd.DataFrame(columns=columns)
    args = on_init(tests,runs,pis)

    i = 0
    for t in tests:
        row = parse_test(t, pis, runs, input_dir, parse_node, merge, n_threads)

        #df.loc[i] = [t] + row
        on_test_results(t, i, row, args)

        i += 1

    #print(df.to_string(index=False))

    #if save:
    #    df.to_csv(output_dir + t + ".csv", sep=";", index=False)

    on_finish(args, output_dir, save)
