/* Thread reading Oracle Redo Logs using online with ASM
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

#include <unistd.h>

#include "ConfigurationException.h"
#include "DatabaseConnection.h"
#include "OracleAnalyzerOnlineASM.h"
#include "ReaderASM.h"
#include "RuntimeException.h"

namespace OpenLogReplicator {
    OracleAnalyzerOnlineASM::OracleAnalyzerOnlineASM(OutputBuffer* outputBuffer, uint64_t dumpRedoLog, uint64_t dumpRawData,
            const char* dumpPath, const char* alias, const char* database, uint64_t memoryMinMb, uint64_t memoryMaxMb,
            uint64_t readBufferMax, uint64_t disableChecks, const char* user, const char* password, const char* connectString,
            const char* userASM, const char* passwordASM, const char* connectStringASM) :
        OracleAnalyzerOnline(outputBuffer, dumpRedoLog, dumpRawData, dumpPath, alias, database, memoryMinMb, memoryMaxMb,
                readBufferMax, disableChecks, user, password, connectString) {

        connASM = new DatabaseConnection(env, userASM, passwordASM, connectStringASM, true);
        if (connASM == nullptr) {
            RUNTIME_FAIL("couldn't allocate " << std::dec << sizeof(class DatabaseConnection) << " bytes memory (for: ASM connection)");
        }
    }

    OracleAnalyzerOnlineASM::~OracleAnalyzerOnlineASM() {
        if (connASM != nullptr) {
            delete connASM;
            connASM = nullptr;
        }
    }

    bool OracleAnalyzerOnlineASM::checkConnection(void) {
        if (!OracleAnalyzerOnline::checkConnection())
            return false;

        if (!connASM->connected) {
            INFO("connecting to ASM instance of " << database << " to " << connASM->connectString);
        }

        while (!shutdown) {
            if (!connASM->connected) {
                try {
                    connASM->connect();
                } catch (RuntimeException& ex) {
                    //
                }
            }

            if (connASM->connected)
                return true;

            DEBUG("cannot connect to ASM, retry in 5 sec.");
            sleep(5);
        }

        return false;
    }

    Reader* OracleAnalyzerOnlineASM::readerCreate(int64_t group) {
        for (Reader* reader : readers)
            if (reader->group == group)
                return reader;

        ReaderASM* readerASM = new ReaderASM(alias.c_str(), this, group);
        if (readerASM == nullptr) {
            RUNTIME_FAIL("couldn't allocate " << std::dec << sizeof(ReaderASM) << " bytes memory (for: asm reader creation)");
        }
        readers.insert(readerASM);
        readerASM->initialize();

        if (pthread_create(&readerASM->pthread, nullptr, &Reader::runStatic, (void*)readerASM)) {
            CONFIG_FAIL("spawning thread");
        }
        return readerASM;
    }

    const char* OracleAnalyzerOnlineASM::getModeName(void) const {
        return "ASM";
    }
}
