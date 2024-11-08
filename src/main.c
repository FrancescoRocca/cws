#include "main.h"
#include "colors.h"
#include "server.h"
#include "utils.h"

int main(int argc, char **argv) {
	fprintf(stdout, BOLD GREEN "Running cws...\n" RESET);

	int ret = start_server();
	if (ret < 0) {
		fprintf(stderr, BOLD RED "Unable to start web server\n");
	}

	print_ips("google.com", "80");

	return 0;
}
