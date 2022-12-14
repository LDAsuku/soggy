// dispatch.cpp

// the dispatch server :)
// I don't really understand how the client's update checks work, but my dispatch implementation gets past them

#include "dispatch.hpp"

// C++
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iterator>
#include <string>

// cpp-httplib
#include <httplib.h>

// rapidjson
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// soggy
#include "log.hpp"
#include "configfile.hpp"
#include "binpack.hpp"

void do_query_region_list(const httplib::Request &req, httplib::Response &rsp) {
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

	writer.StartObject();
	writer.Key("retcode"); writer.Int(0);
	writer.Key("clientCustomConfig"); writer.String("{\"sdkenv\":\"2\",\"visitor\":false,\"devicelist\":null}");
	// ^ these are ALL of keys it checks for in clientCustomConfig
	// "sdkenv" MUST be >= 2
	// "devicelist" seems completely unused?

	writer.Key("regionList");
	writer.StartArray();

	writer.StartObject();
	writer.Key("dispatchUrl"); writer.String(soggy_cfg.dispatch_route_url + "/query_cur_region");
	writer.Key("name"); writer.String("region0");
	writer.Key("title"); writer.String(soggy_cfg.dispatch_region_title);
	writer.Key("type"); writer.String("DEV_PUBLIC");
	writer.EndObject();

	writer.EndArray();
	writer.EndObject();

	rsp.set_content(buffer.GetString(), "application/json");
}

void do_common_login(const httplib::Request &req, httplib::Response &rsp) {
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

	writer.StartObject();
	writer.Key("retcode"); writer.Int(0);
	writer.Key("data");
	writer.StartObject();
	writer.Key("uid"); writer.String(std::to_string(1));
	writer.Key("token"); writer.String("token1");
	writer.Key("email"); writer.String("soggy:)");
	writer.EndObject();
	writer.EndObject();

	rsp.set_content(buffer.GetString(), "application/json");
}

extern void lookup_account_by_name(std::string name);
extern void lookup_account_uid_and_token(int uid, std::string token);

void do_sdk_login(const httplib::Request &req, httplib::Response &rsp) {
	// ?account=lda:)&password=x&lang=en
	do_common_login(req, rsp);
}
void do_sdk_token_login(const httplib::Request &req, httplib::Response &rsp) {
	// ?uid=1&token=token1&lang=en
	do_common_login(req, rsp);
}

void do_query_cur_region(const httplib::Request &req, httplib::Response &rsp) {
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

	writer.StartObject();
	writer.Key("retcode"); writer.Int(0);
	writer.Key("regionInfo");
	writer.StartObject();
	writer.Key("gateserverIp"); writer.String(soggy_cfg.dispatch_game_addr);
	writer.Key("gateserverPort"); writer.Int(soggy_cfg.dispatch_game_port);
	writer.Key("resourceUrl"); writer.String(soggy_cfg.dispatch_route_url + "/game_res");
	writer.Key("dataUrl"); writer.String(soggy_cfg.dispatch_route_url + "/design_data");
	writer.Key("feedbackUrl"); writer.String(soggy_cfg.dispatch_feedback_url);
	writer.Key("bulletinUrl"); writer.String(soggy_cfg.dispatch_notices_url);
	writer.Key("handbookUrl"); writer.String(soggy_cfg.dispatch_guide_url);
	writer.Key("resVersion"); writer.Int(soggy_cfg.common_game_res_version);
	writer.Key("dataVersion"); writer.Int(soggy_cfg.common_design_data_version);
	writer.Key("clientCustomConfig"); writer.String("{}");
	writer.Key("regionCustomConfig"); writer.String("{}");
	writer.EndObject();
	writer.EndObject();

	rsp.set_content(buffer.GetString(), "application/json");
}

httplib::Server server;

bool dispatch_queued_shutdown = false;

void http_static_handler(const httplib::Request &req, httplib::Response &rsp, const std::string &key) {
	try {
		const Entry *entry = binpack_get_file(key);
		rsp.set_content((char *)entry->data, entry->len, server.find_content_type(key));
	} catch (std::out_of_range &) {
		rsp.status = 404;
	}
}

void dispatch_server_main() {
	httplib::Headers default_headers;
	default_headers.insert({ "Server", "broken pancake (real)" });
	server.set_default_headers(default_headers);

	server.Get("/static/(.*)", [](const httplib::Request &req, httplib::Response &rsp) {
		http_static_handler(req, rsp, req.matches[1].str());
	});

	server.Get("/query_region_list", do_query_region_list);
	server.Get("/sdk/login", do_sdk_login);
	server.Get("/sdk/token_login", do_sdk_token_login);
	server.Get("/query_cur_region", do_query_cur_region);

	server.Get("/game_res/cur_version.txt", [](const httplib::Request &req, httplib::Response &rsp) {
		rsp.set_content(std::to_string(soggy_cfg.common_game_res_version), "text/plain");
	});

	server.Get("/game_res/.*", [](const httplib::Request &req, httplib::Response &rsp) {});

	server.Get("/design_data/cur_version.txt", [](const httplib::Request &req, httplib::Response &rsp) {
		rsp.set_content(std::to_string(soggy_cfg.common_design_data_version), "text/plain");
	});

	server.Get("/", [](const httplib::Request &req, httplib::Response &rsp) {
		http_static_handler(req, rsp, "root.html");
	});

	soggy_log("dispatch listening on port %d", soggy_cfg.dispatch_bind_port);

	while (!dispatch_queued_shutdown) {
		server.listen(soggy_cfg.dispatch_bind_addr, soggy_cfg.dispatch_bind_port); // forever
	}

	server.stop();
}

void interrupt_dispatch_server() {
	server.stop();
	dispatch_queued_shutdown = true;
}
