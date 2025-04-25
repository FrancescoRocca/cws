#ifndef CWS_CONFIG_H
#define CWS_CONFIG_H

typedef struct cws_config_t {
	char *host;
	char *port;

	char *cert;
	char *key;
} cws_config;

cws_config *cws_config_init(void);
void cws_config_free(cws_config *config);

#endif
