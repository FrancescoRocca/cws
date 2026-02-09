#ifndef CWS_CONFIG_H
#define CWS_CONFIG_H

#include <stdbool.h>

typedef struct cws_page {
	char *status;
	char *path;
} cws_page_s;

typedef struct cws_vhost {
	char *domain;
	char *root;
	cws_page_s *error_pages;
	unsigned error_pages_count;
} cws_vhost_s;

typedef struct cws_config {
	char *host;
	char *port;
	char *root;
	cws_vhost_s *virtual_hosts;
	unsigned virtual_hosts_count;
} cws_config_s;

cws_config_s *cws_config_init(void);

void cws_config_free(cws_config_s *config);

#endif
