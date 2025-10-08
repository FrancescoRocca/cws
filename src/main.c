#define _XOPEN_SOURCE 1

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "server/server.h"
#include "utils/config.h"
#include "utils/debug.h"

void cws_signal_handler(int) { cws_server_run = 0; }

int main(void) {
	struct sigaction act = {.sa_handler = cws_signal_handler, .sa_flags = 0, .sa_mask = {{0}}};
	if (sigaction(SIGINT, &act, NULL)) {
		CWS_LOG_ERROR("sigaction()");
		return EXIT_FAILURE;
	}

	if (sigaction(SIGTERM, &act, NULL)) {
		CWS_LOG_ERROR("sigaction()");
		return EXIT_FAILURE;
	}

	cws_config_s *config = cws_config_init();
	if (!config) {
		CWS_LOG_ERROR("Unable to read config file");

		return EXIT_FAILURE;
	}

	CWS_LOG_INFO("Virtual hosts count: %d", config->virtual_hosts_count);
	for (size_t i = 0; i < config->virtual_hosts_count; ++i) {
		CWS_LOG_DEBUG("%s (ssl: %d)", config->virtual_hosts[i].domain, config->virtual_hosts[i].ssl);
	}

	cws_server_s server;

	cws_server_ret ret = cws_server_setup(&server, config);
	if (ret != CWS_SERVER_OK) {
		CWS_LOG_ERROR("Unable to setup web server");
	}

	CWS_LOG_INFO("Running cws on http://%s:%s...", config->hostname, config->port);
	ret = cws_server_start(&server);
	if (ret != CWS_SERVER_OK) {
		CWS_LOG_ERROR("Unable to start web server");
	}

	CWS_LOG_INFO("Shutting down cws...");
	cws_server_shutdown(&server);
	cws_config_free(config);

	return EXIT_SUCCESS;
}
