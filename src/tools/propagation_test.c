#include <stdio.h>
#include <stdlib.h>
#include <common/system/time.h>
#include <common/messaging/msg_conf.h>
#include <common/messaging/msg_internals.h>


int main(int argc, char *argv[])
{
    char **nodes;
    int *ips;
    int num_nodes = 0, i = 0;
    request_t req = {0};

    if (argc < 2) {
        printf("Error, usage is \n\tpropagation_test numnodes\n");
        exit(0);
    }

    num_nodes = atoi(argv[1]);
    if (num_nodes < 1) {
        printf("Error, number of nodes cannot be lower than 1\n");
        exit(0);
    }

    //alloc
    nodes = calloc(num_nodes, sizeof(char*));
    ips = calloc(num_nodes, sizeof(int));
    
    for (i = 0; i < num_nodes; i++) {
        nodes[i] = calloc(32, sizeof(char));
        sprintf(nodes[i], "node%d", i);
        ips[i] = i;
        //printf("set node %s\n", nodes[i]);
    }

    req.nodes = ips;
    req.num_nodes = num_nodes;

    send_command_nodelist(&req, NULL);


    //dealloc
    for (i = 0; i < num_nodes; i++) free(nodes[i]);
    free(nodes);
}
