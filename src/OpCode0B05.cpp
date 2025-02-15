/* Oracle Redo OpCode: 11.5
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

#include "OpCode0B05.h"
#include "OracleAnalyzer.h"
#include "RedoLogRecord.h"

namespace OpenLogReplicator {
    void OpCode0B05::process(OracleAnalyzer* oracleAnalyzer, RedoLogRecord* redoLogRecord) {
        OpCode::process(oracleAnalyzer, redoLogRecord);
        uint64_t fieldPos = 0;
        typeFIELD fieldNum = 0;
        uint16_t fieldLength = 0;

        oracleAnalyzer->nextField(redoLogRecord, fieldNum, fieldPos, fieldLength, 0x0B0501);
        //field: 1
        ktbRedo(oracleAnalyzer, redoLogRecord, fieldPos, fieldLength);

        if (!oracleAnalyzer->nextFieldOpt(redoLogRecord, fieldNum, fieldPos, fieldLength, 0x0B0502))
            return;
        //field: 2
        kdoOpCode(oracleAnalyzer, redoLogRecord, fieldPos, fieldLength);
        uint8_t* nulls = redoLogRecord->data + redoLogRecord->nullsDelta;

        if (!oracleAnalyzer->nextFieldOpt(redoLogRecord, fieldNum, fieldPos, fieldLength, 0x0B0503))
            return;
        //field: 3
        uint8_t* colNums = nullptr;
        if (fieldLength > 0 && redoLogRecord->cc > 0) {
            redoLogRecord->colNumsDelta = fieldPos;
            colNums = redoLogRecord->data + redoLogRecord->colNumsDelta;
        }

        if ((redoLogRecord->flags & FLAGS_KDO_KDOM2) != 0) {
            oracleAnalyzer->nextField(redoLogRecord, fieldNum, fieldPos, fieldLength, 0x0B0504);
            //field: 4
            redoLogRecord->rowData = fieldNum;
            if (oracleAnalyzer->dumpRedoLog >= 1)
                dumpColsVector(oracleAnalyzer, redoLogRecord, redoLogRecord->data + fieldPos, oracleAnalyzer->read16(colNums), fieldLength);
        } else {
            redoLogRecord->rowData = fieldNum + 1;
            uint8_t bits = 1;

            //fields: 4 + cc .. 4 + cc - 1
            for (uint64_t i = 0; i < redoLogRecord->cc; ++i) {
                if (fieldNum >= redoLogRecord->fieldCnt)
                    break;
                if (i < redoLogRecord->ccData)
                    oracleAnalyzer->nextField(redoLogRecord, fieldNum, fieldPos, fieldLength, 0x0B0506);

                if (fieldLength > 0 && (*nulls & bits) != 0 && i < redoLogRecord->ccData) {
                    WARNING("length: " << std::dec << fieldLength << " for NULL column offset: " << redoLogRecord->dataOffset);
                }

                if (oracleAnalyzer->dumpRedoLog >= 1)
                    dumpCols(oracleAnalyzer, redoLogRecord, redoLogRecord->data + fieldPos, oracleAnalyzer->read16(colNums), fieldLength, *nulls & bits);
                bits <<= 1;
                colNums += 2;
                if (bits == 0) {
                    bits = 1;
                    ++nulls;
                }
            }
        }
    }
}
