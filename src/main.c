#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "server/server.h"
#include "utils/colors.h"
#include "utils/config.h"

void cws_signal_handler(int signo) {
	fprintf(stdout, BLUE "[server] Cleaning up resources...\n" RESET);
	cws_server_run = false;
}

int main(int argc, char **argv) {
	int ret;

	cws_config *config = cws_config_init();
	if (config == NULL) {
		fprintf(stderr, RED BOLD "[server] Unable to read config file\n" RESET);
		return 1;
	}

	struct sigaction act = {
		.sa_handler = cws_signal_handler
	};
	ret = sigaction(SIGINT, &act, NULL);

	fprintf(stdout, BOLD GREEN "[server] Running cws on http://%s:%s...\n" RESET, config->host, config->port);

	ret = cws_server_start(config->host, config->port);
	if (ret < 0) {
		fprintf(stderr, BOLD RED "[server] Unable to start web server\n" RESET);
	}

	cws_config_free(config);

	return 0;
}
