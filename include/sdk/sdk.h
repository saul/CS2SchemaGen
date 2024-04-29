#pragma once

#include <Include.h>

#include <sdk/interfaces/tier0/IMemAlloc.h>

#include <sdk/interfaces/common/CBufferString.h>
#include <sdk/interfaces/common/CInterlockedInt.h>
#include <sdk/interfaces/common/CTSList.h>
#include <sdk/interfaces/common/CThreadMutex.h>
#include <sdk/interfaces/common/CThreadSpinMutex.h>
#include <sdk/interfaces/common/CThreadSpinRWLock.h>
#include <sdk/interfaces/common/CUtlMap.h>
#include <sdk/interfaces/common/CUtlMemory.h>
#include <sdk/interfaces/common/CUtlMemoryPoolBase.h>
#include <sdk/interfaces/common/CUtlRBTree.h>
#include <sdk/interfaces/common/CUtlString.h>
#include <sdk/interfaces/common/CUtlTSHash.h>
#include <sdk/interfaces/common/CUtlVector.h>

#include <sdk/interfaceregs.h>
#include <sdk/interfaces/schemasystem/Schema.h>

namespace sdk {
    inline CSchemaSystem* g_schema = nullptr;

    void GenerateTypeScopeSdk(CSchemaSystemTypeScope* current, const char* outDirName);
} // namespace sdk
