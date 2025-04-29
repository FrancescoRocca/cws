#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server/server.h"
#include "utils/colors.h"
#include "utils/config.h"

void cws_signal_handler(int signo) { cws_server_run = false; }

int main(int argc, char **argv) {
	int ret;

	cws_config *config = cws_config_init();
	if (config == NULL) {
		CWS_LOG_ERROR("[server] Unable to read config file");
		return 1;
	}

	struct sigaction act = {.sa_handler = cws_signal_handler};
	ret = sigaction(SIGINT, &act, NULL);

	CWS_LOG_INFO("[server] Running cws on http://%s:%s...", config->host, config->port);

	ret = cws_server_start(config->host, config->port);
	if (ret < 0) {
		CWS_LOG_ERROR("[server] Unable to start web server");
	}

	cws_config_free(config);

	return 0;
}
