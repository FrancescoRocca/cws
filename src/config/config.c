#include "config/config.h"

#include <cyaml/cyaml.h>
#include <stdio.h>

#include "utils/debug.h"

static const cyaml_config_t cyaml_config = {
	.log_fn = cyaml_log,
	.mem_fn = cyaml_mem,
	.log_level = CYAML_LOG_WARNING,
};

static const cyaml_schema_field_t error_page_fields[] = {
	CYAML_FIELD_INT("method", CYAML_FLAG_DEFAULT, cws_error_page, method),
	CYAML_FIELD_STRING_PTR("path", CYAML_FLAG_POINTER, cws_error_page, path, 0, CYAML_UNLIMITED),
	CYAML_FIELD_END,
};

static cyaml_schema_value_t error_page_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, cws_error_page, error_page_fields),
};

static const cyaml_schema_field_t virtual_hosts_fields[] = {
	CYAML_FIELD_STRING_PTR("domain", CYAML_FLAG_POINTER, struct cws_vhost, domain, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("root", CYAML_FLAG_POINTER, struct cws_vhost, root, 0, CYAML_UNLIMITED),
	CYAML_FIELD_BOOL("ssl", CYAML_FLAG_DEFAULT, struct cws_vhost, ssl),
	CYAML_FIELD_STRING_PTR("cert", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL, struct cws_vhost, cert, 0,
						   CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("key", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL, struct cws_vhost, key, 0, CYAML_UNLIMITED),

	CYAML_FIELD_SEQUENCE("error_pages", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL, struct cws_vhost, error_pages,
						 &error_page_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END,
};

static cyaml_schema_value_t virtual_hosts_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, struct cws_vhost, virtual_hosts_fields),
};

static const cyaml_schema_field_t top_schema_fields[] = {
	CYAML_FIELD_STRING_PTR("hostname", CYAML_FLAG_POINTER, struct cws_config, hostname, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("port", CYAML_FLAG_POINTER, struct cws_config, port, 0, CYAML_UNLIMITED),
	CYAML_FIELD_SEQUENCE("virtual_hosts", CYAML_FLAG_POINTER, struct cws_config, virtual_hosts, &virtual_hosts_schema,
						 0, CYAML_UNLIMITED),
	CYAML_FIELD_END,
};

static cyaml_schema_value_t top_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, struct cws_config, top_schema_fields),
};

cws_config_s *cws_config_init(void) {
	char *path = "config.yaml";
	cws_config_s *config;

	cyaml_err_t err = cyaml_load_file(path, &cyaml_config, &top_schema, (cyaml_data_t **)&config, NULL);
	if (err != CYAML_OK) {
		cws_log_error("%s", cyaml_strerror(err));

		return NULL;
	}

	return config;
}

void cws_config_free(cws_config_s *config) {
	cyaml_err_t err = cyaml_free(&cyaml_config, &top_schema, config, 0);
	if (err != CYAML_OK) {
		return;
	}
}
