#include "client.h"
#include "colors.h"
#include "main.h"

int main(int argc, char **argv) {
	fprintf(stdout, BOLD GREEN "[client] Running client...\n" RESET);

	int ret = test_client_connection(argv[1], argv[2]);
	if (ret < 0) {
		fprintf(stderr, BOLD RED "Unable to start client\n");
	}

	return 0;
}
