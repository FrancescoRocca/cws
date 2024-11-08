#include "main.h"
#include "colors.h"
#include "server.h"

int main(int argc, char **argv) {
	fprintf(stdout, BOLD GREEN "[server] Running cws...\n" RESET);

	int ret = start_server(NULL, "3030");
	if (ret < 0) {
		fprintf(stderr, BOLD RED "Unable to start web server\n");
	}

	return 0;
}
