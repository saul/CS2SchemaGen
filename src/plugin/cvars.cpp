#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "sdk/sdk.h"

void SchemaDumpAll(const char* outDirName)
{
    const auto schemaSystem = (CSchemaSystem*)g_pSchemaSystem;

    const auto& type_scopes = schemaSystem->m_TypeScopes;
    for (auto i = 0; i < type_scopes.GetNumStrings(); ++i) {
        sdk::GenerateTypeScopeSdk(type_scopes[i], outDirName);
    }

    sdk::GenerateTypeScopeSdk(schemaSystem->GlobalTypeScope(), outDirName);
}
