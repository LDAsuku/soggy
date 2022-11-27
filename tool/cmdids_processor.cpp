// cmdids_processor.cpp

// over-engineered perchance

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

const char *hpp_template = R"(// generated file

#pragma once

#include <google/protobuf/descriptor.h>

@DEFINES@

namespace cmdids {
	struct CmdIdDetail {
		const google::protobuf::Descriptor *descriptor;
		std::string name;
		unsigned short cmdid;
		int enet_channel_id;
		bool enet_is_reliable;
		bool is_allow_client;
	};

	extern std::unordered_map<std::string, const CmdIdDetail *> cmd_name_to_detail;
	extern std::unordered_map<unsigned short, const CmdIdDetail *> cmd_id_to_detail;
}
)";

const char *cpp_template = R"(// generated file

#include "cmdids.hpp"

namespace cmdids {
	std::unordered_map<std::string, const CmdIdDetail *> cmd_name_to_detail;
	std::unordered_map<unsigned short, const CmdIdDetail *> cmd_id_to_detail;
}

@INCLUDES@

struct init {
	init() {
@INIT_CODE@
	}
};

static init _init;
)";

void string_replace(std::string *s, const std::string_view search, const std::string_view replace) {
	s->replace(s->find(search), search.length(), replace);
}

int main(int argc, char *argv[]) {
	assert(argc == 4);

	std::ifstream csvf(argv[1]);
	std::ofstream cppf(argv[2]);
	std::ofstream hppf(argv[3]);

	std::string headerline;
	std::getline(csvf, headerline, '\n');

	std::ostringstream hpp_defines;
	std::ostringstream cpp_includes;
	std::ostringstream cpp_init_code;

	std::string cmdname, cmdid, channel_id, reliable, allow_client;
	while (true) {
		std::getline(csvf, cmdname, ',');
		std::getline(csvf, cmdid, ',');
		std::getline(csvf, channel_id, ',');
		std::getline(csvf, reliable, ',');
		std::getline(csvf, allow_client, '\n');
		if (csvf.eof()) break;

		if (cmdname == "DebugNotify") {
			continue;
		}

		hpp_defines << "#define CMD_ID_" << cmdname << " " << cmdid << "\n";

		cpp_includes << "#include <proto/" << cmdname << ".pb.h>\n";

		cpp_init_code << "\t\tstatic const cmdids::CmdIdDetail detail_" << cmdname << " = cmdids::CmdIdDetail{ " << cmdname << "::GetDescriptor(), \"" << cmdname << "\", " << cmdid << ", " << channel_id << ", " << reliable << ", " << allow_client << " };\n";

		cpp_init_code << "\t\tcmdids::cmd_id_to_detail.insert({ (unsigned short)" << cmdid << ", &detail_" << cmdname << " });\n";
		cpp_init_code << "\t\tcmdids::cmd_name_to_detail.insert({ \"" << cmdname << "\", &detail_" << cmdname << " });\n";
	}

	std::string hpp_content(hpp_template);
	string_replace(&hpp_content, std::string_view("@DEFINES@"), hpp_defines.str());
	hppf << hpp_content;

	std::string cpp_content(cpp_template);
	string_replace(&cpp_content, std::string_view("@INCLUDES@"), cpp_includes.str());
	string_replace(&cpp_content, std::string_view("@INIT_CODE@"), cpp_init_code.str());
	cppf << cpp_content;
}
