// Modulo EDITOR-ONLY del bridge MCP. Il `.uplugin` lo dichiara `Type: "Editor"` con
// `TargetAllowList: ["Editor"]`: non entra nel packaged, e il modulo gameplay runtime non lo conosce.
// La dipendenza va in una direzione sola — RTDeveloperTools -> RefactorTactics — e mai al contrario.

using UnrealBuildTool;

public class RTDeveloperTools : ModuleRules
{
	public RTDeveloperTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			// UToolsetDefinition (la classe base dei tool) e UToolsetRegistry (registrazione allo startup).
			// Pubblica perche' `RTDevToolset.h` eredita da UToolsetDefinition: chi include l'header ne ha bisogno.
			"ToolsetRegistry"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			// UKismetSystemLibrary::RaiseScriptError (il canale d'errore dei toolset, vedi RTDevToolset.cpp),
			// UWorld, StaticEnum<>.
			"Engine",
			// GEditor: il world dell'Editor da cui si pesca ARTHexMapActor. E' cio' che rende il modulo Editor-only.
			"UnrealEd",
			// Il gameplay AUTOREVOLE: URTHexMapAsset, URTHexPathLibrary, ARTHexMapActor. Il bridge non ne
			// duplica una riga — se un dato non e' qui, il tool non lo espone.
			"RefactorTactics"
		});
	}
}
