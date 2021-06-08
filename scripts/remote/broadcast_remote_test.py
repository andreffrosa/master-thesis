#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tester

class BroadcastTest(tester.Test):
    def __init__(self, tag, discovery="", topology="", sleep_ms=0, app="broadcast_app"):
        self.tag = tag
        self.discovery = discovery
        self.topology = topology
        self.sleep_ms = sleep_ms
        self.app = app

    def get_cmd(self, run, results_dir):
        cmd = "/home/andreffrosa/bin/broadcast_test -b /home/andreffrosa/experiments/configs/broadcast/%s.conf" % (self.tag)

        if not self.discovery == "":
            cmd += " -d /home/andreffrosa/experiments/configs/discovery/%s.conf" % (self.discovery)

        cmd += " -a /home/andreffrosa/experiments/configs/%s.conf" % (self.app)

        if not self.topology == "":
            cmd += " -o /home/andreffrosa/topologies/%s/" % (self.topology)

        if self.sleep_ms > 0:
            cmd += " -s " + str(self.sleep_ms)

        cmd += " -l %s%s-run%d/" % (results_dir, self.tag, run)

        return cmd

    def get_logs(self, run, results_dir):
        return "%s%s-run%d/" % (results_dir, self.tag, run)


duration_per_test_s = 10*60 + 10
wait_s = 2
n_tries = 1
n_runs = 3
bin_dir = "/home/andreffrosa/bin/"
n_nodes = 18

def getTests(app):
    return [
        BroadcastTest("flooding", app=app),
        BroadcastTest("gossip1", app=app),
        BroadcastTest("gossip1_horizon", app=app),
        BroadcastTest("gossip2", "PeriodicHelloDiscovery", app=app),
        BroadcastTest("gossip3", app=app),
        BroadcastTest("counting", app=app),
        BroadcastTest("hop_count_aided", app=app),
        BroadcastTest("sba", "PeriodicJointDiscovery", app=app),
        BroadcastTest("mpr", "OLSRDiscovery", app=app),
        BroadcastTest("ahbp", "PeriodicJointDiscovery", app=app),
        BroadcastTest("lenwb", "LENWBDiscovery", app=app),
        BroadcastTest("hop_count_rad_extension", app=app),
        ]

def executeExp(results_dir, app, to_kill):

    tests = getTests(app)

    b_tester = tester.RemoteTester(tests, duration_per_test_s, wait_s, n_runs, results_dir, bin_dir, n_nodes, tries=n_tries, print_only=False, zip=False, clean_out=True)

    if to_kill != None:
        b_tester.set_to_kill(to_kill)

    b_tester.execute()


# Random No Faults
app = "broadcast_app"
results_dir = "/home/andreffrosa/" + app + "_%s/"
to_kill = None
executeExp(results_dir, app, to_kill)
