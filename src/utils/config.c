#include "utils/config.h"

#include <cyaml/cyaml.h>

static const cyaml_config_t cyaml_config = {
	.log_fn = cyaml_log,
	.mem_fn = cyaml_mem,
	.log_level = CYAML_LOG_WARNING,
};

static const cyaml_schema_field_t top_mapping_schema[] = {
	CYAML_FIELD_STRING_PTR("host", CYAML_FLAG_POINTER, cws_config, host, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("port", CYAML_FLAG_POINTER, cws_config, port, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("cert", CYAML_FLAG_POINTER, cws_config, cert, 0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR("key", CYAML_FLAG_POINTER, cws_config, key, 0, CYAML_UNLIMITED), CYAML_FIELD_END};

static const cyaml_schema_value_t top_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, cws_config, top_mapping_schema),
};

cws_config *cws_config_init(void) {
	char *path = "config.yaml";
	cws_config *config;

	cyaml_err_t err = cyaml_load_file(path, &cyaml_config, &top_schema, (cyaml_data_t **)&config, NULL);
	if (err != CYAML_OK) {
		return NULL;
	}

	return config;
}

void cws_config_free(cws_config *config) {
	cyaml_err_t err = cyaml_free(&cyaml_config, &top_schema, config, 0);
	if (err != CYAML_OK) {
		/* Handle */
	}
}

/*
	https://github.com/tlsa/libcyaml/blob/main/docs/guide.md
*/
