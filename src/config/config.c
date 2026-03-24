#include "config/config.h"

#include <stdlib.h>
#include <string.h>
#include <tomlc17.h>

#include "utils/debug.h"

static bool is_default(const char *domain) {
	if (!strcmp(domain, "default")) {
		return true;
	}

	return false;
}

static bool parse_vhosts(cws_config_s *config, toml_result_t result) {
	toml_datum_t vhosts = toml_seek(result.toptab, "virtual_hosts");

	/* Retrieve virtual hosts counter */
	config->virtual_hosts_count = vhosts.u.arr.size;

	/* Allocate virtual hosts array */
	config->virtual_hosts = malloc(sizeof *config->virtual_hosts * config->virtual_hosts_count);
	if (!config->virtual_hosts) {
		return false;
	}

	/* Iterate for each virtual host */
	for (int i = 0; i < vhosts.u.arr.size; ++i) {
		cws_vhost_s *vh = &config->virtual_hosts[i];
		toml_datum_t elem = vhosts.u.arr.elem[i];

		/* Retrieve vh's domain */
		toml_datum_t domain = toml_seek(elem, "domain");
		vh->domain = strdup(domain.u.str.ptr);
		if (!vh->domain) {
			return false;
		}

		/* Check if vh->domain is the default domain */
		if (is_default(vh->domain)) {
			config->default_vh = vh;
		}

		/* Retrieve vh's root folder */
		toml_datum_t root = toml_seek(elem, "root");
		vh->root = strdup(root.u.str.ptr);
		if (!vh->root) {
			return false;
		}

		/* Pages */
		toml_datum_t pages = toml_seek(elem, "pages");
		vh->error_pages_count = pages.u.arr.size;

		/* Allocate error pages array */
		vh->error_pages = malloc(sizeof *vh->error_pages * vh->error_pages_count);
		if (!vh->error_pages) {
			return false;
		}

		/* Iterate for each page */
		for (int j = 0; j < pages.u.arr.size; ++j) {
			toml_datum_t page = pages.u.arr.elem[i];

			toml_datum_t status = toml_seek(page, "status");
			vh->error_pages[j].status = strdup(status.u.str.ptr);
			if (!vh->error_pages[i].status) {
				return false;
			}

			toml_datum_t path = toml_seek(page, "path");
			vh->error_pages[j].path = strdup(path.u.str.ptr);
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
	config->host = strdup(host.u.str.ptr);
	if (!config->host) {
		return false;
	}

	toml_datum_t port = toml_seek(result.toptab, "server.port");
	config->port = strdup(port.u.str.ptr);
	if (!config->port) {
		return false;
	}

	toml_datum_t root = toml_seek(result.toptab, "server.root");
	config->root = strdup(root.u.str.ptr);
	if (!config->root) {
		return false;
	}

	toml_datum_t workers = toml_seek(result.toptab, "server.workers");
	config->workers = workers.u.int64;

	bool ret = parse_vhosts(config, result);
	toml_free(result);

	return ret;
}

cws_config_s *cws_config_init(void) {
	cws_config_s *config = calloc(1, sizeof *config);
	if (!config) {
		return NULL;
	}

	if (!parse_toml(config)) {
		return NULL;
	}

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

	if (config) {
		free(config);
	}
}
