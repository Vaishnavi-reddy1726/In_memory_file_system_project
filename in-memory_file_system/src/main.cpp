#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include "FileSystem.hpp"

namespace {

void printHelp() {
    std::cout <<
        "Commands:\n"
        "  create <path> <content...>   create a new file\n"
        "  read <path>                  read a file (uses cache when possible)\n"
        "  update <path> <content...>   overwrite a file's content\n"
        "  delete <path>                delete a file\n"
        "  list                         list all files on disk\n"
        "  stats                        show cache hit/miss stats\n"
        "  help                         show this message\n"
        "  exit                         quit\n";
}

void printStats(const FileSystem& fs) {
    auto s = fs.stats();
    std::cout << "[" << fs.policyName() << "] disk files=" << fs.diskFileCount()
               << " cached=" << fs.cacheSize()
               << " hits=" << s.hits << " misses=" << s.misses
               << " hitRate=" << std::fixed << std::setprecision(1) << s.hitRate() << "%\n";
}

CachePolicy choosePolicy() {
    std::cout << "Select cache eviction policy:\n  1) LRU (Least Recently Used)\n  2) LFU (Least Frequently Used)\n> ";
    std::string choice;
    std::getline(std::cin, choice);
    return (choice == "2") ? CachePolicy::LFU : CachePolicy::LRU;
}

} // namespace

int main() {
    std::cout << "=== In-Memory File System (LRU/LFU cached) ===\n";
    CachePolicy policy = choosePolicy();

    std::cout << "Cache capacity (number of files to keep hot): ";
    std::string capStr;
    std::getline(std::cin, capStr);
    size_t capacity = 3;
    try { capacity = capStr.empty() ? 3 : std::stoul(capStr); } catch (...) { capacity = 3; }

    FileSystem fs(policy, capacity);
    std::cout << "Using " << fs.policyName() << " cache with capacity " << capacity << ".\n";
    printHelp();

    std::string line;
    std::cout << "\n> ";
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "exit" || cmd == "quit") {
            break;
        } else if (cmd == "help") {
            printHelp();
        } else if (cmd == "create") {
            std::string path, content;
            iss >> path;
            std::getline(iss, content);
            if (!content.empty() && content[0] == ' ') content.erase(0, 1);
            if (path.empty()) {
                std::cout << "usage: create <path> <content...>\n";
            } else if (fs.createFile(path, content)) {
                std::cout << "created " << path << "\n";
            } else {
                std::cout << "error: " << path << " already exists\n";
            }
        } else if (cmd == "read") {
            std::string path;
            iss >> path;
            auto result = fs.readFile(path);
            if (result) std::cout << *result << "\n";
            else std::cout << "error: " << path << " not found\n";
        } else if (cmd == "update") {
            std::string path, content;
            iss >> path;
            std::getline(iss, content);
            if (!content.empty() && content[0] == ' ') content.erase(0, 1);
            if (fs.updateFile(path, content)) std::cout << "updated " << path << "\n";
            else std::cout << "error: " << path << " not found\n";
        } else if (cmd == "delete") {
            std::string path;
            iss >> path;
            if (fs.deleteFile(path)) std::cout << "deleted " << path << "\n";
            else std::cout << "error: " << path << " not found\n";
        } else if (cmd == "list") {
            auto files = fs.listFiles();
            if (files.empty()) std::cout << "(no files)\n";
            for (const auto& f : files) std::cout << "  " << f << "\n";
        } else if (cmd == "stats") {
            printStats(fs);
        } else if (!cmd.empty()) {
            std::cout << "unknown command '" << cmd << "' (type 'help')\n";
        }
        std::cout << "\n> ";
    }

    std::cout << "\nFinal stats: ";
    printStats(fs);
    std::cout << "Goodbye.\n";
    return 0;
}
