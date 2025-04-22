#include <stdio.h>

#include "server/server.h"
#include "utils/colors.h"

int main(int argc, char **argv) {
	const char *default_port = "3030";

	fprintf(stdout, BOLD GREEN "[server] Running cws on http://localhost:%s...\n" RESET, default_port);

	int ret = start_server(NULL, default_port);
	if (ret < 0) {
		fprintf(stderr, BOLD RED "Unable to start web server\n");
	}

	return 0;
}
