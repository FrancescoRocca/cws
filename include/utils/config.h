#ifndef CWS_CONFIG_H
#define CWS_CONFIG_H

#include <stdbool.h>

struct cws_virtual_host_t {
	char *domain;
	char *root;
	bool ssl;
	char *cert;
	char *key;
};
typedef struct cws_virtual_host_t cws_virtual_host;

struct cws_config_t {
	char *hostname;
	char *port;
	cws_virtual_host *virtual_hosts;
	unsigned virtual_hosts_count;
};
typedef struct cws_config_t cws_config;

cws_config *cws_config_init(void);
void cws_config_free(cws_config *config);

#endif
