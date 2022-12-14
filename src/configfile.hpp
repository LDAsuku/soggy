// configfile.hpp

// C++
#include <string>

struct SoggyCfg {
	// [common]
	int common_game_res_version = 138541;
	int common_design_data_version = 138541;

	// [dispatch]
	std::string dispatch_bind_addr;
	int dispatch_bind_port = 0;
	std::string dispatch_route_url;
	std::string dispatch_game_addr;
	int dispatch_game_port = 0;
	std::string dispatch_region_title;
	std::string dispatch_feedback_url;
	std::string dispatch_notices_url;
	std::string dispatch_guide_url;

	// [game]
	std::string game_bind_addr;
	int game_bind_port = 0;
	int game_max_clients = 0;
};

extern SoggyCfg soggy_cfg;

extern int soggy_load_config_file(const char *filename);
