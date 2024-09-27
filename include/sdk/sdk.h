#pragma once

#include <Include.h>


#include "tier0/memalloc.h"

#include "tier1/bufferstring.h"
#include "tier1/utlmap.h"
#include "tier1/utlmemory.h"
#include "tier1/utlrbtree.h"
#include "tier1/utlstring.h"
#include "tier1/utltshash.h"
#include "tier1/utlvector.h"

#include <sdk/interfaceregs.h>
#include "schemasystem/schemasystem.h"

namespace sdk {
    void GenerateTypeScopeSdk(CSchemaSystemTypeScope* current, const char* outDirName);
} // namespace sdk
