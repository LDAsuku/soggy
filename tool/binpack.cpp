// binpack.cpp

#include <cassert>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

const char *const cpp_template = R"(// generated file

#include <unordered_map>
#include <string>
#include <cstdint>
#include <cstddef>

struct Entry {
	uint8_t *data;
	size_t len;
};

@DATA_CODE@

std::unordered_map<std::string, Entry> pack = {
@INIT_CODE@
};

const Entry *binpack_get_file(const std::string &key) {
	return &pack.at(key);
}
)";

void string_replace(std::string *s, const std::string_view search, const std::string_view replace) {
	s->replace(s->find(search), search.length(), replace);
}

int main(int argc, char *argv[]) {
	assert(argc >= 3);

	std::ofstream cppf(argv[1]);
	std::string_view root_dir(argv[2]);

	std::ostringstream cpp_data_code;
	std::ostringstream cpp_init_code;

	int file_num = 0;
	for (int i = 3; i < argc; i++) {
		std::ifstream ifs(argv[i], std::ios::binary);

		ifs.unsetf(std::ios::skipws);
		ifs.seekg(0, std::ios::end);
		int file_size = ifs.tellg();
		ifs.seekg(0, std::ios::beg);

		std::vector<uint8_t> file_content;
		file_content.reserve(file_size);
		file_content.insert(file_content.begin(), std::istream_iterator<uint8_t>(ifs), std::istream_iterator<uint8_t>());

		std::string_view path = std::string_view(argv[i]).substr(root_dir.length() + 1);

		cpp_data_code << "// " << std::quoted(path) << "\n";
		cpp_data_code << "uint8_t file" << file_num << "_data[] = {";
		for (int i = 0; i < file_content.size(); i++) {
			if ((i % 32) == 0) {
				cpp_data_code << "\n\t";
			}
			cpp_data_code << (unsigned int)file_content[i] << ',';
		}
		cpp_data_code << "\n};\n";

		cpp_init_code << "\t{ " << std::quoted(path) << ", Entry{ file" << file_num << "_data, " << file_size << " } },\n";

		file_num++;
	}

	std::string cpp_content(cpp_template);
	string_replace(&cpp_content, std::string_view("@DATA_CODE@"), cpp_data_code.str());
	string_replace(&cpp_content, std::string_view("@INIT_CODE@"), cpp_init_code.str());
	cppf << cpp_content;
}
