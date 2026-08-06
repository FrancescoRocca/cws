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

static void config_free_fields(cws_config_s *config) {
	if (!config) {
		return;
	}

	free(config->host);
	free(config->port);
	free(config->root);

	for (unsigned i = 0; i < config->virtual_hosts_count; ++i) {
		cws_vhost_s *vh = &config->virtual_hosts[i];
		free(vh->domain);
		free(vh->root);

		for (unsigned j = 0; j < vh->error_pages_count; ++j) {
			free(vh->error_pages[j].path);
			free(vh->error_pages[j].status);
		}
		free(vh->error_pages);
	}
	free(config->virtual_hosts);
}

static bool parse_vhosts(cws_config_s *config, toml_result_t result) {
	toml_datum_t vhosts = toml_seek(result.toptab, "virtual_hosts");

	/* No virtual hosts is valid: the server falls back to server.root */
	if (vhosts.type == TOML_UNKNOWN) {
		return true;
	}

	if (vhosts.type != TOML_ARRAY) {
		cws_log_error("config: 'virtual_hosts' must be an array of tables");
		return false;
	}

	/* Retrieve virtual hosts counter */
	config->virtual_hosts_count = vhosts.u.arr.size;

	/* Allocate virtual hosts array */
	config->virtual_hosts = calloc(config->virtual_hosts_count, sizeof *config->virtual_hosts);
	if (!config->virtual_hosts) {
		return false;
	}

	/* Iterate for each virtual host */
	for (int i = 0; i < vhosts.u.arr.size; ++i) {
		cws_vhost_s *vh = &config->virtual_hosts[i];
		toml_datum_t elem = vhosts.u.arr.elem[i];

		/* Retrieve vh's domain */
		toml_datum_t domain = toml_seek(elem, "domain");
		if (domain.type != TOML_STRING) {
			cws_log_error("config: virtual host #%d is missing a 'domain' string", i);
			return false;
		}
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
		if (root.type != TOML_STRING) {
			cws_log_error("config: virtual host '%s' is missing a 'root' string", vh->domain);
			return false;
		}
		vh->root = strdup(root.u.str.ptr);
		if (!vh->root) {
			return false;
		}

		/* Pages (optional) */
		toml_datum_t pages = toml_seek(elem, "pages");
		if (pages.type == TOML_UNKNOWN) {
			vh->error_pages_count = 0;
			vh->error_pages = NULL;
		} else {
			if (pages.type != TOML_ARRAY) {
				cws_log_error("config: 'pages' of virtual host '%s' must be an array", vh->domain);
				return false;
			}

			vh->error_pages_count = pages.u.arr.size;

			/* Allocate error pages array */
			vh->error_pages = malloc(sizeof *vh->error_pages * vh->error_pages_count);
			if (!vh->error_pages) {
				return false;
			}

			/* Iterate for each page */
			for (int j = 0; j < pages.u.arr.size; ++j) {
				toml_datum_t page = pages.u.arr.elem[j];

				toml_datum_t status = toml_seek(page, "status");
				if (status.type != TOML_STRING) {
					cws_log_error("config: error page #%d of virtual host '%s' is missing a 'status' string", j,
								  vh->domain);
					return false;
				}
				vh->error_pages[j].status = strdup(status.u.str.ptr);
				if (!vh->error_pages[j].status) {
					return false;
				}

				toml_datum_t path = toml_seek(page, "path");
				if (path.type != TOML_STRING) {
					cws_log_error("config: error page #%d of virtual host '%s' is missing a 'path' string", j,
								  vh->domain);
					return false;
				}
				vh->error_pages[j].path = strdup(path.u.str.ptr);
				if (!vh->error_pages[j].path) {
					return false;
				}
			}
		}
	}

	return true;
}

static bool parse_toml(cws_config_s *config) {
	toml_result_t result = toml_parse_file_ex("config.toml");
	if (!result.ok) {
		cws_log_error("Unable to parse config.toml: %s", result.errmsg);
		return false;
	}

	toml_datum_t host = toml_seek(result.toptab, "server.host");
	if (host.type != TOML_STRING) {
		cws_log_error("%s", "config: missing string 'server.host'");
		toml_free(result);
		return false;
	}

	toml_datum_t port = toml_seek(result.toptab, "server.port");
	if (port.type != TOML_STRING) {
		cws_log_error("%s", "config: missing string 'server.port'");
		toml_free(result);
		return false;
	}

	toml_datum_t root = toml_seek(result.toptab, "server.root");
	if (root.type != TOML_STRING) {
		cws_log_error("%s", "config: missing string 'server.root'");
		toml_free(result);
		return false;
	}

	toml_datum_t workers = toml_seek(result.toptab, "server.workers");
	if (workers.type != TOML_INT64 || workers.u.int64 <= 0) {
		cws_log_error("%s", "config: 'server.workers' must be a positive integer");
		toml_free(result);
		return false;
	}

	config->host = strdup(host.u.str.ptr);
	config->port = strdup(port.u.str.ptr);
	config->root = strdup(root.u.str.ptr);
	config->workers = (int)workers.u.int64;
	if (!config->host || !config->port || !config->root) {
		toml_free(result);
		return false;
	}

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
		config_free_fields(config);
		free(config);
		return NULL;
	}

	return config;
}

void cws_config_free(cws_config_s *config) {
	if (!config) {
		return;
	}

	config_free_fields(config);
	free(config);
}

cws_vhost_s *config_get_vhost(cws_config_s *config, char *host) {
	for (unsigned i = 0; i < config->virtual_hosts_count; ++i) {
		cws_vhost_s *vh = config->virtual_hosts;
		if (!strcmp(vh[i].domain, host)) {
			return &vh[i];
		}
	}

	/* Return default domain */
	return config->default_vh;
}
