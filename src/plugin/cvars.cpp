#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "sdk/sdk.h"

void SchemaDumpAll(const char* outDirName)
{
    sdk::g_schema = CSchemaSystem::GetInstance();
    if (!sdk::g_schema)
        throw std::runtime_error(std::format("Unable to obtain Schema interface"));

    const auto type_scopes = sdk::g_schema->GetTypeScopes();
    for (auto i = 0; i < type_scopes.Count(); ++i) {
        sdk::GenerateTypeScopeSdk(type_scopes.m_pElements[i], outDirName);
    }

    sdk::GenerateTypeScopeSdk(sdk::g_schema->GlobalTypeScope(), outDirName);
}
