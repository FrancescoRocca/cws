#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config/config.h"
#include "core/server.h"
#include "utils/debug.h"
#include "utils/error.h"

void cws_signal_handler(int) {
	cws_server_run = 0;
}

int main(void) {
	cws_log_info("Running cws...");

	if (signal(SIGINT, cws_signal_handler) == SIG_ERR) {
		cws_log_error("signal()");
		CWS_LOG_ERROR("signal()");

		return EXIT_FAILURE;
	}

	cws_config_s *config = cws_config_init();
	if (!config) {
		cws_log_error("Unable to read config file");
		CWS_LOG_ERROR("Unable to read config file");

		return EXIT_FAILURE;
	}

	CWS_LOG_INFO("Virtual hosts count: %d", config->virtual_hosts_count);
	for (size_t i = 0; i < config->virtual_hosts_count; ++i) {
		cws_vhost_s *vh = config->virtual_hosts;
		CWS_LOG_DEBUG("%s (ssl: %d)", vh[i].domain, vh[i].ssl);

		for (size_t j = 0; j < vh[i].error_pages_count; j++) {
			cws_error_page *ep = &vh->error_pages[j];
			CWS_LOG_DEBUG("Error %u -> %s", ep->method, ep->path);
		}
	}

	cws_server_s server;
	cws_return ret;

	ret = cws_server_setup(&server, config);
	if (ret != CWS_OK) {
		cws_log_error("Unable to setup web server: %s", cws_error_str(ret));
		CWS_LOG_ERROR("Unable to setup web server: %s", cws_error_str(ret));
		cws_config_free(config);

		return EXIT_FAILURE;
	}

	CWS_LOG_INFO("Running cws on http://%s:%s...", config->hostname, config->port);
	ret = cws_server_start(&server);
	if (ret != CWS_OK) {
		cws_log_error("Unable to start web server: %s", cws_error_str(ret));
		CWS_LOG_ERROR("Unable to start web server: %s", cws_error_str(ret));
	}

	CWS_LOG_INFO("Shutting down cws...");
	cws_server_shutdown(&server);
	cws_config_free(config);

	return EXIT_SUCCESS;
}
