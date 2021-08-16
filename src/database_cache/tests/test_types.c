#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <common/sizes.h>
#include <common/output/verbose.h>
#include <database_cache/eardbd_api.h>

char cmnd[SZ_PATH];
char opt1[SZ_PATH];
char opt2[SZ_PATH];
cluster_conf_t conf_clus;
my_node_conf_t conf_node;
application_t app;

int is(char *buf, char *string)
{
	return strcmp(buf, string) == 0;
}

state_t eardbd_fakefail(int who);

int main(int argc, char *argv[])
{
	char *col = "        ";
	state_t s;
	int i;
	int j;

	if (get_ear_conf_path(opt1) == EAR_ERROR) {
		error("while getting ear.conf path '%s' '%s' (%d, '%s')", opt1, getenv("EAR_ETC"), errno, strerror(errno));
		return 0;
	}

	read_cluster_conf(opt1, &conf_clus);

	while (1)
	{
		dprintf(verb_channel, "command: ");
		scanf("%s", cmnd);
		//scanf("%s %s %s", cmnd, opt1, opt2);

		if (is(cmnd, "connect")) {
			scanf("%s %s", opt1, opt2);
			if (!is(opt1, "-")) strcpy(conf_node.db_ip, opt1);
			if (!is(opt2, "-")) strcpy(conf_node.db_sec_ip, opt2);
			s = eardbd_connect(&conf_clus, &conf_node);
			verbose(0, "%s connection result (%d, %s)", col, s, state_msg);
		} else if (is(cmnd, "disconnect")) {
			eardbd_disconnect();
		} else if (is(cmnd, "reconnect")) {
			s = eardbd_reconnect(&conf_clus, &conf_node);
			verbose(0, "%s reconnection result (%d, %s)", col, s, state_msg);
		} else if (is(cmnd, "fake")) {
			scanf("%d", &i);
			eardbd_fakefail(i);
		} else if (is(cmnd, "send")) {
			scanf("%d", &i);
			scanf("%d", &j);

			sprintf(app.job.app_id, "monitor");
			gethostname(app.node_id, 128);
			app.is_learning = 0;
			app.job.id = 1000000;
			app.job.step_id = j;
			app.is_mpi = 1;

			for (; i > 0; --i) {
				app.job.step_id += 1;
				s = eardbd_send_application(&app);
				verbose(0, "%s send app result (%d)", col, s);
			}
		} else if (is(cmnd, "exit")) {
			return 0;
		}
	}

	return 0;
}
