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
			// Il master e le sei istanze del kit graybox (#1714, commandlet RTBuildGrayboxMeshes):
			// `UMaterialEditingLibrary` vive qui, ed e' l'unico modo di costruire un material graph in C++
			// invece di autorarlo a mano — cioe' di farlo sopravvivere alla rigenerazione delle mesh.
			"MaterialEditor",
			"AssetRegistry",
			// Il launcher (#1680, slice L1): la categoria del menu Window in cui il tab si registra,
			// `UEditorSubsystem` come classe base, e `UGameMapsSettings` per leggere la EditorStartupMap nel test.
			// ⚠️ Nessuno dei tre arriva per transitivita': misurato, senza dichiararli il link non risolve.
			"WorkspaceMenuStructure",
			"EditorSubsystem",
			"EngineSettings",
			"RefactorTactics" // modulo runtime: URTHexMapAsset / URTHexLibrary / ARTHexMapActor (usati da H5b)
		});
	}
}
