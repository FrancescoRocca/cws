#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "config/config.h"
#include "core/server.h"
#include "utils/debug.h"
#include "utils/error.h"

void cws_signal_handler(int signo) {
	(void)signo;
	cws_server_run = 0;
}

int main(void) {
	cws_log_init();

	if (signal(SIGINT, cws_signal_handler) == SIG_ERR) {
		cws_log_error("signal()");
		return EXIT_FAILURE;
	}

	cws_config_s *config = cws_config_init();
	if (!config) {
		cws_log_shutdown();
		return EXIT_FAILURE;
	}

	cws_server_s server;
	cws_return ret;

	ret = cws_server_setup(&server, config);
	if (ret != CWS_OK) {
		cws_log_error("Unable to setup web server: %s", cws_error_str(ret));
		cws_config_free(config);
		cws_log_shutdown();
		return EXIT_FAILURE;
	}

	cws_log_info("Running cws on http://%s:%s", config->host, config->port);
	ret = cws_server_start(&server);
	if (ret != CWS_OK) {
		cws_log_error("Unable to start web server: %s", cws_error_str(ret));
	}

	cws_log_info("Shutting down cws");
	cws_server_shutdown(&server);
	cws_config_free(config);
	cws_log_shutdown();

	return EXIT_SUCCESS;
}
