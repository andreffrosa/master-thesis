#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tester

import math

class DiscoveryTest(tester.Test):
    def __init__(self, tag, topology="", sleep_ms=0, app="discovery_app"):
        self.tag = tag
        self.topology = topology
        self.sleep_ms = sleep_ms
        self.app = app

    def get_cmd(self, run, results_dir):
        cmd = "/home/andreffrosa/bin/discovery_test -d /home/andreffrosa/experiments/configs/discovery/%s.conf" % (self.tag)
        cmd += " -a /home/andreffrosa/experiments/configs/%s.conf" % (self.app)

        if not self.topology == "":
            cmd += " -o /home/andreffrosa/topologies/%s/" % (self.topology)

        if self.sleep_ms > 0:
            cmd += " -s " + str(self.sleep_ms)

        cmd += " -l %s%s-run%d/" % (results_dir, self.tag, run)

        return cmd

    def get_logs(self, run, results_dir):
        return "%s%s-run%d/" % (results_dir, self.tag, run)


#duration_per_test_s = 1900
duration_per_test_s = 120 +10
wait_s = 2
n_tries = 1
n_runs = 3
bin_dir = "/home/andreffrosa/bin/"
n_nodes = 18

def getTests(app):
    return [
        DiscoveryTest("PeriodicJointDiscovery", app=app),
        DiscoveryTest("PeriodicJointDiscovery2", app=app),
        DiscoveryTest("PeriodicDisjointDiscovery", app=app),
        DiscoveryTest("PeriodicDisjointDiscovery2", app=app),
        DiscoveryTest("EchoDiscovery1", app=app),
        DiscoveryTest("EchoDiscovery2", app=app)
        ]

def executeExp(results_dir, app, to_kill):

    tests = getTests(app)

    d_tester = tester.RemoteTester(tests, duration_per_test_s, wait_s, n_runs, results_dir, bin_dir, n_nodes, tries=n_tries, print_only=False, zip=False, clean_out=True)

    if to_kill != None:
        d_tester.set_to_kill(to_kill)

    d_tester.execute()

# Random No Faults
app = "discovery_app"
results_dir = "/home/andreffrosa/" + app + "_%s/"
to_kill = None
executeExp(results_dir, app, to_kill)
