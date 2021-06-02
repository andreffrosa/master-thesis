#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import subprocess
from datetime import datetime, timedelta
import random
import traceback

class Test:
    #def __init__(self, tag, cmd):
    #    self.tag = tag
    #    self.cmd = cmd

    def get_tag(self):
        return self.tag

    def get_cmd(self, run, results_dir):
        return "ERROR"

    def get_logs(self, run, results_dir):
        return "ERROR"


def seconds_to_string(t):
    num = t

    sec = 0
    min = 0
    hour = 0
    day = 0

    if num > 59:
        sec = num%60
        num = num/60
        if num > 59:
            min = num%60
            num = num/60
            if num > 23:
                hour = num%24
                day = num/24
            else:
                hour = num
        else:
            min = num
    else:
        sec = num

    #return day+"d " + hour+"h " + min+"m " + sec+"s"
    return "%dd %dh %dm %ds" % (day, hour, min, sec)

class RemoteTester:

    def __init__(self, tests, duration_per_test_s, wait_s, n_runs, results_dir, bin_dir, n_nodes, print_only=False, zip=False, clean_out=False):
        self.tests = tests
        self.duration_per_test_s = duration_per_test_s
        self.wait_s = wait_s
        self.n_runs = n_runs
        self.results_dir = results_dir
        self.bin_dir = bin_dir
        self.n_nodes = n_nodes
        self.print_only = print_only
        self.zip = zip
        self.clean_out = clean_out
        self.nodes_to_kill = []

    def __print_configs(self, start_time):
        print("------------------------------------------------------")
        print("---------------------- BEGIN -------------------------")
        print("")

        print("Current Time: " + start_time.strftime("%Y/%m/%d %H:%M:%S"))

        print("Duration per test: " + seconds_to_string(self.duration_per_test_s))
        print("Number of runs: " + str(self.n_runs))

        total_time = (self.n_runs * len(self.tests) * (self.duration_per_test_s + 3*self.wait_s)) + 3*self.wait_s
        print("Total execution time: " + seconds_to_string(total_time))
        print("End Time: " + (start_time + timedelta(0,total_time)).strftime("%Y/%m/%d %H:%M:%S"))
        print("")

    def __check_command(self, value):

        if isinstance(value, int):
            value = str(value)
        elif isinstance(value, str):
            value = value.strip()

        if not self.print_only:
            if int(value) == self.n_nodes:
                return "OK (" + value + "/" + str(self.n_nodes) + " nodes)"
            else:
                print("ERROR (" + value + "/" + str(self.n_nodes) + " nodes)")
                #quit()
        else:
            return "OK (" + str(self.n_nodes) + "/" + str(self.n_nodes) + " nodes)"

    def __setup_tree(self):
        print("Setting Up Tree...")
        if not self.print_only:
            res = subprocess.check_output([self.bin_dir+"cmdbuildtree", "127.0.0.1", "5000"], text=True)
            #print(res)

            time.sleep(self.wait_s)

            res = subprocess.check_output([self.bin_dir+"cmdchecktree", "127.0.0.1", "5000"], text=True, universal_newlines=True)
            #print(res)
            res = subprocess.check_output(["grep", "raspi"], text=True, universal_newlines=True, input=res)
            #print(res)
            res = subprocess.check_output(["wc", "-l"], text=True, universal_newlines=True, input=res)
            #print(res)

            print("Check Support Tree: " + self.__check_command(res))

            time.sleep(self.wait_s)
        else:
            print("Check Support Tree: " + self.__check_command(0))

        print("")

    def __run_command(self, cmd):
        if not self.print_only:
            #toks = [str(x) for x in cmd.split(' ') if x.strip()]
            #print(toks)
            #print('"' + cmd + '"')
            try:
                res = subprocess.check_output([self.bin_dir+"cmdexecuteexperience", "127.0.0.1", "5000", cmd], text=True, universal_newlines=True)
                #print(res)
                res = subprocess.check_output(["grep", "raspi"], text=True, universal_newlines=True, input=res)
                #print(res)
                res = subprocess.check_output(["wc", "-l"], text=True, universal_newlines=True, input=res)
                #print(res)

                print('Run cmd: ' + self.__check_command(res))
            except:
                e = traceback.format_exc()
                print(e)
                print('Run cmd: ' + self.__check_command(0))
        else:
            print('Run cmd: ' + self.__check_command(0))


    def __start_experience(self, cmd):
        #run_command "bin/cmdexecuteexperience 127.0.0.1 5000" "$1"
        #echo "Start Experience: "$(check_command $?)
        if not self.print_only:
            try:
                res = subprocess.check_output([self.bin_dir+"cmdexecuteexperience", "127.0.0.1", "5000", cmd], text=True, universal_newlines=True)
                #print(res)
                res = subprocess.check_output(["grep", "raspi"], text=True, universal_newlines=True, input=res)
                #print(res)
                res = subprocess.check_output(["wc", "-l"], text=True, universal_newlines=True, input=res)
                #print(res)
                print("Start Experience: " + self.__check_command(res))
            except:
                e = traceback.format_exc()
                print(e)
                print("Start Experience: " + self.__check_command(0))
        else:
            print("Start Experience: " + self.__check_command(0))

    def __stop_experience(self, str):
        if not self.print_only:
            try:
                res = subprocess.check_output([self.bin_dir+"cmdterminateexperience", "127.0.0.1", "5000", str], text=True, universal_newlines=True)

                #print(res)
                res = subprocess.check_output(["grep", "raspi"], text=True, universal_newlines=True, input=res)
                #print(res)
                res = subprocess.check_output(["wc", "-l"], text=True, universal_newlines=True, input=res)
                #print(res)

                print("Stop Experience: " + self.__check_command(res))
            except:
                e = traceback.format_exc()
                print(e)
                print("Stop Experience: " + self.__check_command(0))
        else:
            print("Stop Experience: " + self.__check_command(0))

    def __kill_nodes(self, str, nodes):
        nodes_str = " ".join(nodes)

        if not self.print_only:
            try:
                res = subprocess.check_output([self.bin_dir+"cmdterminateexperience", "127.0.0.1", "5000", str + " " + nodes_str], text=True, universal_newlines=True)

                #print(res)
                res = subprocess.check_output(["grep", "raspi"], text=True, universal_newlines=True, input=res)
                #print(res)
                res = subprocess.check_output(["wc", "-l"], text=True, universal_newlines=True, input=res)
                #print(res)

                print("Kill " + nodes_str + ": " + self.__check_command(res))
            except:
                e = traceback.format_exc()
                print(e)
                print("Kill " + nodes_str + ": " + self.__check_command(0))
        else:
            print("Kill " + nodes_str + ": " + self.__check_command(0))


    def set_to_kill(self, nodes_to_kill):
        self.nodes_to_kill = nodes_to_kill


    def execute(self):
        current_time = datetime.now()
        start_time = current_time.strftime("%Y.%m.%d-%H.%M.%S")

        self.__print_configs(current_time)

        self.__setup_tree()

        self.results_dir = self.results_dir % (start_time)
        print("Creating results dir %s" % (self.results_dir))
        self.__run_command("sudo mkdir " + self.results_dir)

        print("")

        #quit()

        print("------------------------------------------------------")
        print("---------------------- START -------------------------")
        print("------------------------------------------------------")
        print("")

        for r in range(1, self.n_runs+1):
            print("------------------------------------------------------")
            print("---------------------- RUN %d/%d -----------------------" % (r, self.n_runs))
            print("------------------------------------------------------")
            print("")

            print("Shuffling tests ...")
            #random.shuffle(self.tests)
            s_tests = random.sample(self.tests, len(self.tests))

            print("")

            for t in s_tests:
                print("")
                print("------------ %s RUN %d/%d -----------" % (t.get_tag(), r, self.n_runs))
                print("")

                self.__setup_tree()

                cmd = t.get_cmd(r, self.results_dir)

                print("executing experience: \n" + cmd)

                self.__start_experience(cmd)

                print("")

                out = "%s%s-run%d.log" % (self.results_dir, t.get_tag(), r)

                elapsed_s = 0
                nodes_to_kill = [[x, y] for [x, y] in self.nodes_to_kill] #clone
                while len(nodes_to_kill) > 0:
                    to_kill_now = []

                    #print(self.nodes_to_kill[0])
                    while len(nodes_to_kill) > 0 and (nodes_to_kill[0])[0] <= elapsed_s:
                        node = nodes_to_kill.pop(0)
                        to_kill_now.append( node[1] )

                    if len(to_kill_now) > 0:
                        print("[%d s] Killing: %s" % (elapsed_s, " ".join(to_kill_now) ))
                        self.__kill_nodes(out, to_kill_now)

                    to_sleep = 0
                    if len(nodes_to_kill) > 0:
                        to_sleep = (nodes_to_kill[0])[0]

                        print("Sleep %s s" % (to_sleep))
                        if not self.print_only:
                            time.sleep(to_sleep)
                        elapsed_s += to_sleep

                to_sleep = self.duration_per_test_s - elapsed_s
                if to_sleep > 0:
                    print("Sleep %s s" % (to_sleep))
                    if not self.print_only:
                        time.sleep(to_sleep)
                    elapsed_s += to_sleep

                print("")

                self.__stop_experience(out)
                print(out)

                print("")

                cmd = "sudo chmod 777 " + self.results_dir + "%s%s-run%d/" % (self.results_dir, t.get_tag(), r)
                self.__run_command(cmd)

                if self.zip:
                    zip = "%s%s-run%d.tar.gz" % (self.results_dir, t.get_tag(), r)
                    print("Zipping... " + zip)
                    cmd = "env GZIP=-9 tar -czf " + zip + " -C " + self.results_dir + " " + t.get_logs(r, "")
                    #print(cmd)
                    self.__run_command(cmd)
                    print("Deleting uncompressd...")
                    self.__run_command("sudo rm -r " + t.get_logs(r, self.results_dir))

                if not self.print_only:
                    time.sleep(self.wait_s)

                if self.clean_out:
                    print("sudo rm " + out)
                    self.__run_command("sudo rm " + out)

                print("")

                print("")
                print("--------- %s RUN %d/%d ended ---------" % (t.get_tag(), r, self.n_runs))
                print("")
