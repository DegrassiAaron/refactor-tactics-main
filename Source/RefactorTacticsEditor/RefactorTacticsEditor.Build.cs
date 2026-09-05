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
			// #2326: seguire i pin di un Blueprint dal nodo evento alla chiamata di funzione. Serve alle
			// classi `UK2Node_*`, ed e' la ragione per cui questo test vive nel modulo EDITOR e non in
			// quello Runtime: `RTFrontendWidgetAssetTests.cpp` dichiara di NON poter attribuire una
			// chiamata al pulsante che la origina, perche' UMG compila ogni evento del widget in un unico
			// ubergraph e un modulo Runtime non puo' dipendere da `BlueprintGraph` senza rompere Shipping.
			"BlueprintGraph",
			// Import delle texture e creazione del DA_IconCatalog dal commandlet RTBuildIconCatalog.
			"AssetTools",
			// Il Playground Panel (#1993): l'EditorUtilityWidget e il suo albero di widget si costruiscono
			// dal commandlet `RTBuildPlaygroundPanel`, con la stessa disciplina delle mesh graybox — la
			// sorgente e' il codice, l'asset e' il suo output (`D-229`).
			"UMG",
			"UMGEditor",
			"Blutility",
			// `PanelGraphCallsTheModel` interroga i nodi dell'EventGraph del pannello per NOME DI FUNZIONE
			// (`UK2Node_CallFunction::FunctionReference`), non per titolo: il titolo e' presentazione.
			"BlueprintGraph",
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
