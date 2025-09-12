#ifndef CWS_CONFIG_H
#define CWS_CONFIG_H

#include <stdbool.h>

typedef struct cws_vhost {
	char *domain;
	char *root;
	bool ssl;
	char *cert;
	char *key;
} cws_vhost_s;

typedef struct cws_config {
	char *hostname;
	char *port;
	cws_vhost_s *virtual_hosts;
	unsigned virtual_hosts_count;
} cws_config_s;

cws_config_s *cws_config_init(void);
void cws_config_free(cws_config_s *config);

#endif
