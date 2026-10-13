#include "Content/RTBuildHexVolumeMaterialCommandlet.h"
#include "Content/RTHexVolumeMaterial.h"

URTBuildHexVolumeMaterialCommandlet::URTBuildHexVolumeMaterialCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URTBuildHexVolumeMaterialCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Options;
	ParseCommandLine(*Params, Tokens, Switches, Options);

	return RTHexVolumeMaterial::Build(Switches.Contains(TEXT("DryRun")));
}
