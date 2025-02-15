/* Base class for state in fileson disk
   Copyright (C) 2018-2022 Adam Leszczynski (aleszczynski@bersler.com)

This file is part of OpenLogReplicator.

OpenLogReplicator is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 3, or (at your option)
any later version.

OpenLogReplicator is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenLogReplicator; see the file LICENSE;  If not see
<http://www.gnu.org/licenses/>.  */

#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include "RuntimeException.h"
#include "StateDisk.h"

namespace OpenLogReplicator {
    StateDisk::StateDisk(const char* path) :
        State(),
        path(path) {
    }

    StateDisk::~StateDisk() {
    }

    void StateDisk::list(std::set<std::string>& namesList) {
        DIR* dir;
        if ((dir = opendir(path.c_str())) == nullptr) {
            RUNTIME_FAIL("can't access directory: " << path);
        }

        struct dirent* ent;
        typeSCN fileScnMax = 0;
        while ((ent = readdir(dir)) != nullptr) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;

            struct stat fileStat;
            std::string fileName(ent->d_name);

            std::string fullName(path + "/" + ent->d_name);
            if (stat(fullName.c_str(), &fileStat)) {
                WARNING("reading information for file: " << fullName << " - " << strerror(errno));
                continue;
            }

            if (S_ISDIR(fileStat.st_mode))
                continue;

            std::string suffix(".json");
            if (fileName.length() < suffix.length() || fileName.substr(fileName.length() - suffix.length(), fileName.length()).compare(suffix) != 0)
                continue;

            std::string fileBase(fileName.substr(0, fileName.length() - suffix.length()));
            namesList.insert(fileBase);
        }
        closedir(dir);
    }

    bool StateDisk::read(std::string& name, uint64_t maxSize, std::string& in, bool noFail) {
        std::string fileName(path + "/" + name + ".json");
        struct stat fileStat;
        int ret = stat(fileName.c_str(), &fileStat);
        TRACE(TRACE2_FILE, "FILE: stat for file: " << fileName << " - " << strerror(errno));
        if (ret != 0) {
            if (noFail)
                return false;

            RUNTIME_FAIL("reading information for file: " << fileName << " - " << strerror(errno));
        }
        if (fileStat.st_size > maxSize || fileStat.st_size == 0) {
            RUNTIME_FAIL("checkpoint file: " << fileName << " wrong size: " << std::dec << fileStat.st_size);
        }

        std::ifstream inputStream;
        inputStream.open(fileName.c_str(), std::ios::in);

        if (!inputStream.is_open()) {
            RUNTIME_FAIL("read error for: " << fileName);
        }

        in.assign((std::istreambuf_iterator<char>(inputStream)), std::istreambuf_iterator<char>());
        inputStream.close();
        return true;
    }

    void StateDisk::write(std::string& name, std::stringstream& out) {
        std::string fileName(path + "/" + name + ".json");
        std::ofstream outputStream;
        outputStream.open(fileName.c_str(), std::ios::out | std::ios::trunc);

        if (!outputStream.is_open()) {
            RUNTIME_FAIL("writing checkpoint data to " << fileName);
        }
        outputStream << out.rdbuf();
        outputStream.close();
    }

    void StateDisk::drop(std::string& name) {
        std::string fileName(path + "/" + name + ".json");
        if (unlink(fileName.c_str()) != 0) {
            WARNING("can't remove file: " << fileName << " - " << strerror(errno));
        }
    }
}
