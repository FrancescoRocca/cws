#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server/server.h"
#include "utils/colors.h"
#include "utils/config.h"

void cws_signal_handler(int signo) { cws_server_run = 0; }

int main(void) {
	struct sigaction act = {.sa_handler = cws_signal_handler, .sa_flags = 0, .sa_mask = {{0}}};
	if (sigaction(SIGINT, &act, NULL)) {
		CWS_LOG_ERROR("sigaction(): %s", strerror(errno));
		return EXIT_FAILURE;
	}

	cws_config *config = cws_config_init();
	if (!config) {
		CWS_LOG_ERROR("Unable to read config file");
		return EXIT_FAILURE;
	}

	CWS_LOG_INFO("Running cws on http://%s:%s...", config->hostname, config->port);
	int ret = cws_server_start(config);
	if (ret < 0) {
		CWS_LOG_ERROR("Unable to start web server");
	}

	CWS_LOG_INFO("Shutting down cws...");
	cws_config_free(config);

	return EXIT_SUCCESS;
}
