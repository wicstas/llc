#include <llc/misc.h>

#include <sstream>

namespace llc {

std::vector<std::string> separate_lines(const std::string& source) {
    std::vector<std::string> lines;
    std::istringstream stream(source);

    std::string line;
    while (std::getline(stream, line))
        lines.push_back(line);

    return lines;
}

}  // namespace llc
