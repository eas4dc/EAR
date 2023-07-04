/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/output/verbose.h>
#include <common/string_enhanced.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/database/db_helper.h>

static char *paint[6] = { STR_RED, STR_GRE, STR_YLW, STR_BLU, STR_MGT, STR_CYA };
static unsigned int opt_p;
static unsigned int opt_c;
static unsigned int opt_o;

void usage(int argc, char *argv[])
{
	int i = 0;

	if (argc < 2)
	{
		verbose(0, "Usage: %s node.id [OPTIONS]\n", argv[0]);
		verbose(0, "  The node.id of the node to display the information.Use \"all\" to show all the nodes");
		verbose(0, "\nOptions:");
		verbose(0, "\t-P <num>\tPrints the output with a different color,");
		verbose(0, "\t\tcan be used when displaying different batch of");
		verbose(0, "\t\tapplications by script.");
		verbose(0, "\t-C\tShows other jobs of the same application,");
		verbose(0, "\t\tnode, policy and number of processes.");
		verbose(0, "\t-O\tgenerate csv file with name 'learning_show.<node_id>.csv'");
		exit(1);
	}

	for (i = 2; i < argc; ++i) {
		if (!opt_c)
			opt_c = (strcmp(argv[i], "-C") == 0);
		if (!opt_o)
			opt_o = (strcmp(argv[i], "-O") == 0);
		if (!opt_p) {
			opt_p = (strcmp(argv[i], "-P") == 0);
			if (opt_p) {
				opt_p = atoi(argv[i + 1]) % 6;
			}
		}
	}
}

int main(int argc,char *argv[])
{
	char buffer[256];
	char *node_name = NULL;
	cluster_conf_t my_conf;
	application_t *apps;
	int total_apps = 0;
	int num_apps = 0;
	int i;
	double VPI;

	//
	usage(argc, argv);

	//
	node_name = argv[1];

  // CSV output 
  char *csv_name;
  char *csv_name_full;
  FILE *fd;
  
	if (opt_o) {
    csv_name = "learning_show.";
    csv_name_full = malloc(strlen(csv_name)+strlen(node_name)+4);
    strcpy(csv_name_full, csv_name);
    strcat(csv_name_full, node_name);
    strcat(csv_name_full, ".csv"); 
  }
  
  if (strcmp(argv[1], "all") == 0) node_name = NULL;	

	if (get_ear_conf_path(buffer) == EAR_ERROR) {
		printf("ERROR while getting ear.conf path\n");
		exit(0);
	}else{
		printf("Reading from %s\n",buffer);
	}

	//
	read_cluster_conf(buffer, &my_conf);

	//
	init_db_helper(&my_conf.database);

	//
	num_apps = db_read_applications(&apps, 1, 50, node_name);

	// 
	if (!opt_c){
		tprintf_init(fdout, STR_MODE_COL, "17 10 17 10 10 8 8 8 8 8 8 8");

		tprintf("Node name||JID||App name||Def. F.||Avg. F.||Seconds||Watts||GBS||CPI||TPI||Gflops||VPI");
		tprintf("---------||---||--------||-------||-------||-------||-----||---||---||---||------||---");
	} else {
		verbose(0, "Node name;JID;App name;Def. F.;Avg. F.;Seconds;Watts;GBS;CPI;TPI;Gflops;VPI");
	}
  
  // header
  if (opt_o){
    fd = fopen(csv_name_full, "w");	
		fprintf(fd, "node_id\tjob_id\tapp_id\tdef_f\tavg_f\ttime\tpower\tGBS\tCPI\tTPI\tGflops\tVPI\n");
	} 

	while (num_apps > 0){
		total_apps += num_apps;
		
    for (i = 0; i < num_apps; i++){
			if (strcmp(buffer, apps[i].node_id) != 0){
				strcpy(buffer, apps[i].node_id);
			}

			if (strlen(apps[i].node_id) > 15){
				apps[i].node_id[15] = '\0';
			}

			compute_sig_vpi(&VPI , &apps[i].signature);
			if (opt_c) {
				verbose(0, "%s;%lu;%s;%lu;%lu;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf",
                    apps[i].node_id, apps[i].job.id,apps[i].job.app_id, apps[i].job.def_f, apps[i].signature.avg_f,
                    apps[i].signature.time, apps[i].signature.DC_power,
                    apps[i].signature.GBS, apps[i].signature.CPI, apps[i].signature.TPI, apps[i].signature.Gflops, VPI);
			} 
      else{
				tprintf("%s%s||%lu||%s||%lu||%lu||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf", paint[opt_p],
					apps[i].node_id,apps[i].job.id, apps[i].job.app_id, apps[i].job.def_f, apps[i].signature.avg_f,
					apps[i].signature.time, apps[i].signature.DC_power,
					apps[i].signature.GBS, apps[i].signature.CPI, apps[i].signature.TPI, apps[i].signature.Gflops, VPI);
			}			

      if (opt_o){
				fprintf(fd, "%s\t%lu\t%s\t%lu\t%lu\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\n",
                    apps[i].node_id, apps[i].job.id,apps[i].job.app_id, apps[i].job.def_f, apps[i].signature.avg_f,
                    apps[i].signature.time, apps[i].signature.DC_power,
                    apps[i].signature.GBS, apps[i].signature.CPI, apps[i].signature.TPI, apps[i].signature.Gflops, VPI);
			}
	  }
    free(apps);  
		num_apps = db_read_applications(&apps, 1, 50, node_name);
  }
  
  if (opt_o){
    fclose(fd);
  }

	return 0;
}
