#include "main.h"

#include "server/server.h"
#include "utils/colors.h"

int main(int argc, char **argv) {
	fprintf(stdout, BOLD GREEN "[server] Running cws on http://localhost:%s...\n" RESET, "3030");

	int ret = start_server(NULL, "3030");
	if (ret < 0) {
		fprintf(stderr, BOLD RED "Unable to start web server\n");
	}

	return 0;
}
