// Modulo EDITOR-ONLY del pivot esagonale (H5): Editor Mode dedicato (UEdMode + Interactive Tools Framework).
// Le dipendenze editor NON entrano nel packaged (Type "Editor" nel .uproject). Il runtime resta autorevole.

using UnrealBuildTool;

public class RefactorTacticsEditor : ModuleRules
{
	public RefactorTacticsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"InputCore",
			"EditorFramework",
			"UnrealEd",
			"LevelEditor",
			"InteractiveToolsFramework",
			"EditorInteractiveToolsFramework",
			// Import delle texture e creazione del DA_IconCatalog dal commandlet RTBuildIconCatalog.
			"AssetTools",
			// Generazione e salvataggio delle mesh del kit graybox (D-229, commandlet RTBuildGrayboxMeshes):
			// la FMeshDescription si costruisce qui e diventa la SORGENTE dell'asset salvato.
			"MeshDescription",
			"StaticMeshDescription",
			"AssetRegistry",
			"RefactorTactics" // modulo runtime: URTHexMapAsset / URTHexLibrary / ARTHexMapActor (usati da H5b)
		});
	}
}
