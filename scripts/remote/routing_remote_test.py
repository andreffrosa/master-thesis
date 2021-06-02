#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tester

class RoutingTest(tester.Test):
    def __init__(self, tag, discovery="", broadcast="", topology=""):
        self.tag = tag
        self.discovery = discovery
        self.broadcast = broadcast
        self.topology = topology

    def get_cmd(self, run, results_dir):
        cmd = "/home/andreffrosa/bin/ping2_test -r /home/andreffrosa/experiments/configs/routing/%s.conf" % (self.tag)

        if not self.discovery == "":
            cmd += " -d /home/andreffrosa/experiments/configs/discovery/%s.conf" % (self.discovery)

        if not self.broadcast == "":
            cmd += " -b /home/andreffrosa/experiments/configs/broadcast/%s.conf" % (self.broadcast)

        cmd += " -a /home/andreffrosa/experiments/configs/ping2_app.conf"

        if not self.topology == "":
            cmd += " -o /home/andreffrosa/topologies/%s/" % (self.topology)

        cmd += " -l %s%s-run%d/" % (results_dir, self.tag, run)

        return cmd

    def get_logs(self, run, results_dir):
        return "%s%s-run%d/" % (results_dir, self.tag, run)



duration_per_test_s = 3700
wait_s = 2
n_runs = 1
results_dir = "/home/andreffrosa/routing_tests_%s/"
bin_dir = "/home/andreffrosa/bin/"
n_nodes = 2

tests = [
    #RoutingTest("olsr", "OLSRDiscovery", "mpr"),
    RoutingTest("batman", "BATMANDiscovery", "batmanflooding"),
    #RoutingTest("babel", "PeriodicDisjointDiscovery", "flooding"),
    #RoutingTest("aodv", "PeriodicJointDiscovery", "biflooding"),
    #RoutingTest("dsr", "PeriodicJointDiscovery", "biflooding")
    #RoutingTest("zone", "OLSRDiscovery", "mpr+biflooding")
    ]

r_tester = tester.RemoteTester(tests, duration_per_test_s, wait_s, n_runs, results_dir, bin_dir, n_nodes, print_only=False, zip=True, clean_out=True)

r_tester.execute()
