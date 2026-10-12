#include "Content/RTBuildUnitOutlineMaterialCommandlet.h"
#include "Content/RTUnitOutlineMaterial.h"

URTBuildUnitOutlineMaterialCommandlet::URTBuildUnitOutlineMaterialCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URTBuildUnitOutlineMaterialCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Options;
	ParseCommandLine(*Params, Tokens, Switches, Options);

	return RTUnitOutlineMaterial::Build(Switches.Contains(TEXT("DryRun")));
}
