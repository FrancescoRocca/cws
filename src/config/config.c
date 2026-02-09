#include "config/config.h"

#include <stdlib.h>
#include <string.h>
#include <tomlc17.h>

#include "utils/debug.h"

static char *cws_strdup(const char *str) {
	if (!str) {
		return NULL;
	}

	size_t len = strlen(str) + 1;
	char *copy = malloc(sizeof *copy * len);
	if (!copy) {
		return NULL;
	}

	memcpy(copy, str, len);

	return copy;
}

static bool parse_vhosts(cws_config_s *config, toml_result_t result) {
	toml_datum_t vhosts = toml_seek(result.toptab, "virtual_hosts");
	config->virtual_hosts_count = vhosts.u.arr.size;
	config->virtual_hosts = malloc(sizeof *config->virtual_hosts * config->virtual_hosts_count);
	if (!config->virtual_hosts) {
		return false;
	}

	for (int i = 0; i < vhosts.u.arr.size; ++i) {
		cws_vhost_s *vh = &config->virtual_hosts[i];
		toml_datum_t elem = vhosts.u.arr.elem[i];
		toml_datum_t domain = toml_seek(elem, "domain");
		vh->domain = cws_strdup(domain.u.str.ptr);
		if (!vh->domain) {
			return false;
		}

		toml_datum_t root = toml_seek(elem, "root");
		vh->root = cws_strdup(root.u.str.ptr);
		if (!vh->root) {
			return false;
		}

		/* Pages */
		toml_datum_t pages = toml_seek(elem, "pages");
		vh->error_pages_count = pages.u.arr.size;
		vh->error_pages = malloc(sizeof *vh->error_pages * vh->error_pages_count);
		if (!vh->error_pages) {
			return false;
		}

		for (int j = 0; j < pages.u.arr.size; ++j) {
			toml_datum_t page = pages.u.arr.elem[i];
			toml_datum_t status = toml_seek(page, "status");
			vh->error_pages[i].status = cws_strdup(status.u.str.ptr);
			if (!vh->error_pages[i].status) {
				return false;
			}

			toml_datum_t path = toml_seek(page, "path");
			vh->error_pages[i].path = cws_strdup(path.u.str.ptr);
			if (!vh->error_pages[i].path) {
				return false;
			}
		}
	}

	return true;
}

static bool parse_toml(cws_config_s *config) {
	const char *path = "config.toml";

	toml_result_t result = toml_parse_file_ex(path);
	if (!result.ok) {
		cws_log_error("Unable to parse config.toml");

		return false;
	}

	toml_datum_t host = toml_seek(result.toptab, "server.host");
	config->host = cws_strdup(host.u.str.ptr);
	if (!config->host) {
		return false;
	}

	toml_datum_t port = toml_seek(result.toptab, "server.port");
	config->port = cws_strdup(port.u.str.ptr);
	if (!config->port) {
		return false;
	}

	toml_datum_t root = toml_seek(result.toptab, "server.root");
	config->root = cws_strdup(root.u.str.ptr);
	if (!config->root) {
		return false;
	}

	parse_vhosts(config, result);

	toml_free(result);

	return true;
}

cws_config_s *cws_config_init(void) {
	cws_config_s *config = malloc(sizeof *config);
	if (!config) {
		return NULL;
	}

	parse_toml(config);

	return config;
}

void cws_config_free(cws_config_s *config) {
	if (!config) {
		return;
	}

	if (config->host) {
		free(config->host);
	}

	if (config->port) {
		free(config->port);
	}

	if (config->root) {
		free(config->root);
	}

	for (unsigned i = 0; i < config->virtual_hosts_count; ++i) {
		cws_vhost_s *vh = &config->virtual_hosts[i];
		if (vh->domain) {
			free(vh->domain);
		}

		if (vh->root) {
			free(vh->root);
		}

		for (unsigned j = 0; j < vh->error_pages_count; ++j) {
			if (vh->error_pages[i].path) {
				free(vh->error_pages[i].path);
			}

			if (vh->error_pages[i].status) {
				free(vh->error_pages[i].status);
			}
		}
	}

	free(config);
}
