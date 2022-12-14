// binpack.hpp

#include <string>
#include <cstdint>

struct Entry {
	uint8_t *data;
	size_t len;
};

// function body defined in generated "binpack_output.cpp"
// may throw std::out_of_range
extern const Entry *binpack_get_file(const std::string &key);
