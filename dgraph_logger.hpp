#pragma once
#include <fstream>
#include <string>
#include <unordered_set>

class DotGraphLogger {
private:
    std::ofstream dotFile;
    std::unordered_set<std::string> nodes;

    std::string formatNode(uint32_t address, const std::string& type) const;

public:
    DotGraphLogger(const std::string& filename);
    ~DotGraphLogger();

    void addNode(uint32_t address, const std::string& type);
    void addEdge(uint32_t from, const std::string& fromType,
                 uint32_t to, const std::string& toType,
                 uint32_t sizeBits, uint32_t times);
    void finalize();
};