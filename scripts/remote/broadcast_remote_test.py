#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tester

class BroadcastTest(tester.Test):
    def __init__(self, tag, discovery="", topology=""):
        self.tag = tag
        self.discovery = discovery
        self.topology = topology

    def get_cmd(self, run, results_dir):
        cmd = "/home/andreffrosa/bin/broadcast_test -b /home/andreffrosa/experiments/configs/broadcast/%s.conf" % (self.tag)

        if not self.discovery == "":
            cmd += " -d /home/andreffrosa/experiments/configs/discovery/%s.conf" % (self.discovery)

        cmd += " -a /home/andreffrosa/experiments/configs/broadcast_app.conf"

        if not self.topology == "":
            cmd += " -o /home/andreffrosa/topologies/%s/" % (self.topology)

        cmd += " -l %s%s-run%d/" % (results_dir, self.tag, run)

        return cmd

    def get_logs(self, run, results_dir):
        return "%s%s-run%d/" % (results_dir, self.tag, run)


duration_per_test_s = 70
wait_s = 2
n_runs = 2
results_dir = "/home/andreffrosa/broadcast_tests_%s/"
bin_dir = "/home/andreffrosa/bin/"
n_nodes = 2

tests = [
    BroadcastTest("flooding"),
    BroadcastTest("gossip1"),
    #BroadcastTest("gossip1_horizon"),
    #BroadcastTest("gossip2", "PeriodicHelloDiscovery"),
    #BroadcastTest("gossip3"),
    #BroadcastTest("counting"),
    #BroadcastTest("hop_count_aided"),
    BroadcastTest("sba", "PeriodicJointDiscovery"),
    #BroadcastTest("mpr", "OLSRDiscovery"),
    #BroadcastTest("ahbp", "PeriodicJointDiscovery"),
    #BroadcastTest("lenwb", "LENWBDiscovery"),
    #BroadcastTest("hop_count_rad_extension"),
    ]

b_tester = tester.RemoteTester(tests, duration_per_test_s, wait_s, n_runs, results_dir, bin_dir, n_nodes, print_only=False, zip=True, clean_out=True)

b_tester.execute()
