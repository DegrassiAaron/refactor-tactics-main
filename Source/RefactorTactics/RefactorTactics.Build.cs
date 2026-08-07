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

		// Json/JsonUtilities: scenari di test e report machine-readable dello Scenario Harness
		// (Test/RTScenarioLoader, Test/RTTestReportWriter). Sono moduli engine standard, disponibili anche
		// in build packaged: il harness deve poter girare headless da riga di comando, non solo in Editor.
		PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });

		// Dipendenza SOLO in build Editor (FScopedTransaction/Undo per il generatore mappa hex, H2). Il runtime
		// packaged NON la include: l'Editor non e' richiesto a runtime (ADR-0002 / documento hex §1). Il modulo
		// Editor dedicato (documento §3) e' rimandato a H5.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
