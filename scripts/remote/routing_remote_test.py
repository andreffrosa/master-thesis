#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tester

class RoutingTest(tester.Test):
    def __init__(self, tag, discovery="", broadcast="", topology="", sleep_ms=0, app="ping2_app"):
        self.tag = tag
        self.discovery = discovery
        self.broadcast = broadcast
        self.topology = topology
        self.sleep_ms = sleep_ms
        self.app = app

    def get_cmd(self, run, results_dir):
        cmd = "/home/andreffrosa/bin/ping2_test -r /home/andreffrosa/experiments/configs/routing/%s.conf" % (self.tag)

        if not self.discovery == "":
            cmd += " -d /home/andreffrosa/experiments/configs/discovery/%s.conf" % (self.discovery)

        if not self.broadcast == "":
            cmd += " -b /home/andreffrosa/experiments/configs/broadcast/%s.conf" % (self.broadcast)

        cmd += " -a /home/andreffrosa/experiments/configs/%s.conf" % (self.app)

        if not self.topology == "":
            cmd += " -o /home/andreffrosa/topologies/%s/" % (self.topology)

        if self.sleep_ms > 0:
            cmd += " -s " + str(self.sleep_ms)

        cmd += " -l %s%s-run%d/" % (results_dir, self.tag, run)

        return cmd

    def get_logs(self, run, results_dir):
        return "%s%s-run%d/" % (results_dir, self.tag, run)



duration_per_test_s = 10*60 + 10 # 10 min
wait_s = 2
n_tries = 1
n_runs = 3
bin_dir = "/home/andreffrosa/bin/"
n_nodes = 17

def getTests(app):
    return [
        RoutingTest("olsr", "OLSRDiscovery", "mpr", app=app),
        RoutingTest("batman", "BATMANDiscovery", "batmanflooding", app=app),
        RoutingTest("babel", "PeriodicDisjointDiscovery", "flooding", app=app),
        RoutingTest("aodv", "PeriodicJointDiscovery", "biflooding", app=app),
        RoutingTest("dsr", "PeriodicJointDiscovery", "biflooding", app=app)
        #RoutingTest("zone", "OLSRDiscovery", "mpr+biflooding")
        ]

def executeExp(results_dir, app, to_kill):

    tests = getTests(app)

    r_tester = tester.RemoteTester(tests, duration_per_test_s, wait_s, n_runs, results_dir, bin_dir, n_nodes, tries=n_tries, print_only=False, zip=False, clean_out=True)

    if to_kill != None:
        r_tester.set_to_kill(to_kill)

    r_tester.execute()



# Random No Faults
app = "routing_random_no_faults"
results_dir = "/home/andreffrosa/" + app + "_%s/"
to_kill = None
executeExp(results_dir, app, to_kill)

# Random 2 Faults
app = "routing_random_2_faults"
results_dir = "/home/andreffrosa/" + app + "_%s/"
time_to_kill = math.floor(duration_per_test_s/2)
to_kill = [
    [time_to_kill, "13"],
    [time_to_kill, "14"]
]
#executeExp(results_dir, app, to_kill)

# Random 5 Faults
app = "routing_random_5_faults"
results_dir = "/home/andreffrosa/" + app + "_%s/"
time_to_kill = math.floor(duration_per_test_s/2)
to_kill = [
    [time_to_kill, "13"],
    [time_to_kill, "14"],
    [time_to_kill, "8"],
    [time_to_kill, "10"],
    [time_to_kill, "12"]
]
#executeExp(results_dir, app, to_kill)


# Random 9 Faults
app = "routing_random_9_faults"
results_dir = "/home/andreffrosa/" + app + "_%s/"
time_to_kill = math.floor(duration_per_test_s/2)
to_kill = [
    [time_to_kill, "13"],
    [time_to_kill, "14"],
    [time_to_kill, "8"],
    [time_to_kill, "10"],
    [time_to_kill, "12"],
    [time_to_kill, "6"],
    [time_to_kill, "16"],
    [time_to_kill, "3"],
    [time_to_kill, "11"]
]
#executeExp(results_dir, app, to_kill)





# Session No Faults
app = "routing_session_no_faults"
results_dir = "/home/andreffrosa/" + app + "_%s/"
to_kill = None
#executeExp(results_dir, app, tests, to_kill)

# Session 2 Faults
app = "routing_session_2_faults"
results_dir = "/home/andreffrosa/" + app + "_%s/"
time_to_kill = math.floor(duration_per_test_s/2)
to_kill = [
    [time_to_kill, "13"],
    [time_to_kill, "14"]
]
#executeExp(results_dir, app, to_kill)

# Session 5 Faults
app = "routing_session_5_faults"
results_dir = "/home/andreffrosa/" + app + "_%s/"
time_to_kill = math.floor(duration_per_test_s/2)
to_kill = [
    [time_to_kill, "13"],
    [time_to_kill, "14"],
    [time_to_kill, "8"],
    [time_to_kill, "10"],
    [time_to_kill, "12"]
]
#executeExp(results_dir, app, to_kill)


# Session 9 Faults
app = "routing_session_9_faults"
results_dir = "/home/andreffrosa/" + app + "_%s/"
time_to_kill = math.floor(duration_per_test_s/2)
to_kill = [
    [time_to_kill, "13"],
    [time_to_kill, "14"],
    [time_to_kill, "8"],
    [time_to_kill, "10"],
    [time_to_kill, "12"],
    [time_to_kill, "6"],
    [time_to_kill, "16"],
    [time_to_kill, "3"],
    [time_to_kill, "11"]
]
#executeExp(results_dir, app, to_kill)
