using UnrealBuildTool;

public class RefactorTactics : ModuleRules
{
	public RefactorTactics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Permette include con prefisso cartella dentro il modulo (es. "Core/RTTypes.h", "Grid/RTGridLibrary.h").
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Dipendenza SOLO in build Editor (FScopedTransaction/Undo per il generatore mappa hex, H2). Il runtime
		// packaged NON la include: l'Editor non e' richiesto a runtime (ADR-0002 / documento hex §1). Il modulo
		// Editor dedicato (documento §3) e' rimandato a H5.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
