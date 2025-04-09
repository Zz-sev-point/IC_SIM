#include "dgraph_logger.h"
#include <sstream>
#include <iomanip>

DotGraphLogger::DotGraphLogger(const std::string& filename) {
    dotFile.open(filename);
    dotFile << "digraph InterconnectGraph {\n";
}

DotGraphLogger::~DotGraphLogger() {
    finalize();
}

std::string DotGraphLogger::formatNode(uint32_t address, const std::string& type) const {
    std::stringstream ss;
    ss << "0x" << std::hex << address << " " << type;
    return ss.str();
}

void DotGraphLogger::addNode(uint32_t address, const std::string& type) {
    std::string nodeLabel = formatNode(address, type);
    if (nodes.find(nodeLabel) == nodes.end()) {
        dotFile << "  \"" << nodeLabel << "\";\n";
        nodes.insert(nodeLabel);
    }
}

void DotGraphLogger::addEdge(uint32_t from, const std::string& fromType,
                             uint32_t to, const std::string& toType,
                             uint32_t sizeBits, uint32_t times) {
    std::string fromNode = formatNode(from, fromType);
    std::string toNode = formatNode(to, toType);

    // addNode(from, fromType);
    // addNode(to, toType);

    dotFile << "  \"" << fromNode << "\" -> \"" << toNode
            << "\" [label=\"" << std::dec << times << "x " << std::dec << sizeBits << " bits\"];\n";
}

void DotGraphLogger::finalize() {
    dotFile << "}\n";
    dotFile.close();
}