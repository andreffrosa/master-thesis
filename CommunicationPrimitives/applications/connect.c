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

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "Yggdrasil.h"

#include "app_common.h"

#define APP_ID 400
#define APP_NAME "CONNECT APP"


int main(int argc, char* argv[]) {

    // Process Args
    hash_table* args = parse_args(argc, argv);

    // Interface and Hostname
    char interface[20] = {0}, hostname[30] = {0};
    unparse_host(hostname, 20, interface, 30, args);

    // Network
    NetworkConfig* ntconf = defineNetworkConfig2(interface, "AdHoc", 2462, 3, 1, "pis", YGG_filter);

    // Initialize ygg_runtime
    ygg_runtime_init_2(ntconf, hostname);

    hash_table_delete(args);
    args = NULL;

    // Register this app
    app_def* myApp = create_application_definition(APP_ID, APP_NAME);

    //queue_t* inBox = registerApp(myApp);

    // Start ygg_runtime
    ygg_runtime_start();

    return 0;
}
