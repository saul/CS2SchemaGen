#include "tier0/interface.h"
#include "appframework/IAppSystem.h"
#include "icvar.h"
#include <stdexcept>
#include <format>

ICvar* g_pCVar = NULL;
ISchemaSystem* g_pSchemaSystem = NULL;
CreateInterfaceFn g_pfnServerCreateInterface = NULL;

extern void SchemaDumpAll(const char* outDirName);

typedef bool (*AppSystemConnectFn)(IAppSystem* appSystem, CreateInterfaceFn factory);
static AppSystemConnectFn g_pfnServerConfigConnect = NULL;

bool Connect(IAppSystem* appSystem, CreateInterfaceFn factory)
{
	auto result = g_pfnServerConfigConnect(appSystem, factory);

	g_pCVar = (ICvar*)factory(CVAR_INTERFACE_VERSION, NULL);
	ConVar_Register();

	g_pSchemaSystem = (ISchemaSystem*)factory(SCHEMASYSTEM_INTERFACE_VERSION, NULL);

	return result;
}

DLL_EXPORT void* CreateInterface(const char* pName, int* pReturnCode)
{
	if (g_pfnServerCreateInterface == NULL)
	{
		// Engine should stop joining VAC-secured servers with a modified gameinfo,
		// this is to be extra cautious.
		auto insecure = CommandLine()->HasParm("-insecure");
		if (!insecure)
		{
			//Plat_FatalErrorFunc("Refusing to load source2gen in secure mode.\n\nAdd -insecure to Counter-Strike's launch options and restart the game.");
		}

		// Generate the path to the real server.dll
		CUtlString realServerPath(Plat_GetGameDirectory());
		realServerPath.Append("\\citadel\\bin\\win64\\server.dll");
		realServerPath.FixSlashes();

		HMODULE serverModule = LoadLibrary(realServerPath.GetForModify());
		g_pfnServerCreateInterface = (CreateInterfaceFn)GetProcAddress(serverModule, "CreateInterface");

		if (g_pfnServerCreateInterface == NULL)
		{
			Plat_FatalErrorFunc("Could not find CreateInterface entrypoint in server.dll: %d", GetLastError());
		}
	}

	auto original = g_pfnServerCreateInterface(pName, pReturnCode);

	// Intercept the first interface requested by the engine
	if (strcmp(pName, "Source2ServerConfig001") == 0)
	{
		auto vtable = *(void***)original;

		DWORD oldProtect = 0;
		if (!VirtualProtect(vtable, sizeof(void**), PAGE_EXECUTE_READWRITE, &oldProtect))
		{
			Plat_FatalErrorFunc("VirtualProtect PAGE_EXECUTE_READWRITE failed: %d", GetLastError());
		}

		// Intercept the Connect virtual method
		g_pfnServerConfigConnect = (AppSystemConnectFn)vtable[0];
		vtable[0] = &Connect;

		DWORD ignore = 0;
		if (!VirtualProtect(vtable, sizeof(void**), oldProtect, &ignore))
		{
			Plat_FatalErrorFunc("VirtualProtect restore failed: %d", GetLastError());
		}
	}

	return original;
}

CON_COMMAND(schema_dump_all, "")
{
	if (args.ArgC() != 2)
	{
        Warning("Format: <output path>\n");
        return;
	}

    try {
        Msg(__FUNCTION__ ": Dumping schemas...\n");
        SchemaDumpAll(args.Arg(1));
        Msg(__FUNCTION__ ": Dumped all schemas\n");
    } catch (std::runtime_error& err) {
        Warning(std::format("{}: Error: {}\n", __FUNCTION__, err.what()).c_str());
    }
}