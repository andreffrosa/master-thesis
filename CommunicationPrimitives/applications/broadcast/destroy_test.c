/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Author:
 * André Rosa (af.rosa@campus.fct.unl.pt
 * Under the guidance of:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2020
 *********************************************************/

#include "process_args.h"

int main(int argc, char** argv) {
    topology_discovery_args* d_args = malloc(sizeof(topology_discovery_args));
    new_topology_discovery_args(d_args, 2, 500, 5000, 60*1000, 3, OLRSQuality(0.5), 0.1);
    RetransmissionContext* neighbors_context = NeighborsContext(10, d_args);
    RetransmissionContext* monitor_context = MonitorContext(5000, "wma",
        ComposeContext(7,
            neighbors_context,
            DynamicProbabilityContext(0.5, 0.3, 0.7, 0.1, 500),
            EmptyContext(),
            HopCountAwareRADExtensionContext(500),
            HopsContext(),
            MultiPointRelayContext(neighbors_context, 0.3, 0.7),
            ParentsContext(3)));

    destroyRetransmissionContext(monitor_context, NULL);

    return 0;
}
