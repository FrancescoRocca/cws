#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "server/server.h"
#include "utils/colors.h"
#include "utils/config.h"

void cws_signal_handler(int signo) {
	/* TODO */
	fprintf(stdout, BLUE "[server] Cleaning up resources...\n" RESET);
	fprintf(stdout, BLUE "[server] Closing...\n" RESET);
	_exit(1);
}

int main(int argc, char **argv) {
	cws_config *config = cws_config_init();
	if (config == NULL) {
		fprintf(stderr, RED BOLD "[server] Unable to read config file\n" RESET);
		return 1;
	}

	if (signal(SIGINT, cws_signal_handler) == SIG_ERR) {
		fprintf(stderr, BOLD RED "[server] Unable to setup signal()\n" RESET);
		return 1;
	}

	fprintf(stdout, BOLD GREEN "[server] Running cws on http://%s:%s...\n" RESET, config->host, config->port);

	int ret = cws_server_start(config->host, config->port);
	if (ret < 0) {
		fprintf(stderr, BOLD RED "[server] Unable to start web server\n" RESET);
	}

	cws_config_free(config);

	return 0;
}
