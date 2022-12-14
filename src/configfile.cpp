// configfile.cpp

#include "configfile.hpp"

// C++
#include <string_view>

// inih
#include <ini.h>

SoggyCfg soggy_cfg;

static int load(void *user, const char *section, const char *name, const char *value) {
	std::string_view section_sv(section);
	std::string_view name_sv(name);
	if (section_sv == "common") {
		if (name_sv == "game_res_version") {
			soggy_cfg.common_game_res_version = std::stoi(value);
		} else if (name_sv == "design_data_version") {
			soggy_cfg.common_design_data_version = std::stoi(value);
		}
	} else if (section_sv == "dispatch") {
		if (name_sv == "bind_addr") {
			soggy_cfg.dispatch_bind_addr.assign(value);
		} else if (name_sv == "bind_port") {
			soggy_cfg.dispatch_bind_port = std::stoi(value);
		} else if (name_sv == "route_url") {
			soggy_cfg.dispatch_route_url.assign(value);
		} else if (name_sv == "game_addr") {
			soggy_cfg.dispatch_game_addr.assign(value);
		} else if (name_sv == "game_port") {
			soggy_cfg.dispatch_game_port = std::stoi(value);
		} else if (name_sv == "region_title") {
			soggy_cfg.dispatch_region_title.assign(value);
		} else if (name_sv == "feedback_url") {
			soggy_cfg.dispatch_feedback_url.assign(value);
		} else if (name_sv == "notices_url") {
			soggy_cfg.dispatch_notices_url.assign(value);
		} else if (name_sv == "guide_url") {
			soggy_cfg.dispatch_guide_url.assign(value);
		}
	} else if (section_sv == "game") {
		if (name_sv == "bind_addr") {
			soggy_cfg.game_bind_addr.assign(value);
		} else if (name_sv == "bind_port") {
			soggy_cfg.game_bind_port = std::stoi(value);
		} else if (name_sv == "max_clients") {
			soggy_cfg.game_max_clients = std::stoi(value);
		}
	}
	return 1;
}

int soggy_load_config_file(const char *filename) {
	return ini_parse(filename, load, nullptr);
}
