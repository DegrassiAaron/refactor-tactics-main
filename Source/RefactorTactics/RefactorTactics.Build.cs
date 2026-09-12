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
			"GameplayTags",
			// CP 11.7: le classi BASE dei widget dello Screen HUD (§4.1). Il layer §4.2 — path, AoE, facing,
			// barre ancorate alle unita' — resta in Canvas dentro `ARTHUD`, dove la spec lo vuole: aggiungere
			// UMG non autorizza a migrarlo, e `progettazione-hud.md` dice testualmente che quel layer «non
			// deve essere realizzato come grandi widget HUD statici».
			//
			// `Slate`/`SlateCore` accanto a `UMG` perche' `FSlateBrush` e i tipi di stile vivono li': un
			// widget che espone un brush senza di essi non compila, e il messaggio d'errore non lo dice.
			"UMG",
			"Slate",
			"SlateCore"
		});

		// #712 / seduta `U22`: `ARTHexMapActor::GetCellPrismMesh` costruisce in codice il prisma esagonale
		// della cella, invece di istanziare `/Engine/BasicShapes/Cylinder` — che si vedeva come un disco.
		// ⚠️ Servono da questo modulo anche se `Engine.Build.cs` li elenca gia' fra le proprie
		// `PublicDependencyModuleNames`: quell'elenco propaga gli include path, non risolve i simboli a
		// valle. Toglierli non rompe la compilazione, rompe il LINK — nove `LNK2019` su `FMeshDescription`,
		// `FStaticMeshAttributes::Register` e `MeshAttribute::Vertex::Position`.
		PrivateDependencyModuleNames.AddRange(new string[] { "MeshDescription", "StaticMeshDescription" });

		// CP E21.2 (`#288`): `FAnimNode_TwoWayBlend` e `FAnimNode_Slot` vivono in `AnimGraphRuntime`, non in
		// `Engine` — solo `FAnimInstanceProxy` e `FAnimNode_SequencePlayer` stanno li'. Senza questa riga
		// `RTUnitAnimInstance` non linka, e l'errore parla di simboli, non di moduli.
		PrivateDependencyModuleNames.Add("AnimGraphRuntime");

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

		// #950 / #923: `Tests/` E' COMPILATA IN OGNI TARGET, E NON E' UNA DIMENTICANZA.
		//
		// Qui non c'e' nessuna condizione su `Target.Configuration` che escluda `Tests/`, e la ragione va
		// scritta qui perche' e' qui che verrebbe voglia di aggiungerla. E' successo in `RTHexSimTests.cpp`
		// (2026-08-09), `RTNoisePropagationTests.cpp` (2026-08-11), `RTFrontendNavigationTests.cpp`
		// (2026-08-23, #1292), `RTScenarioRunnerTests.cpp` (2026-08-24, #1312) e di nuovo in
		// `RTHexSimTests.cpp` (2026-09-10, #2959): ogni volta codice di test lasciato fuori da
		// `#if WITH_DEV_AUTOMATION_TESTS` ha rotto la sola build Shipping, e ogni volta la domanda
		// «perche' non escludiamo i test dal target gioco?» e' tornata senza trovare una risposta scritta.
		// ⛔ L'elenco e' per NOME e non per conteggio di proposito: un numero in prosa invecchia da solo, e
		// questo e' cresciuto due volte da quando #923 lo dava per «due».
		//
		// ⛔ **Non si escludono con una riga qui, e questo e' il primo fatto**: `ModuleRules` non espone
		// nessuna API di esclusione dei sorgenti —
		// `grep -ic exclud Engine/Source/Programs/UnrealBuildTool/Configuration/Rules/ModuleRules.cs` risponde
		// **0** su UE 5.8. UBT compila **tutti** i `.cpp` sotto `ModuleDirectory`. L'alternativa vera non e'
		// una condizione: e' un **modulo separato**, con i suoi `.Build.cs`, il suo posto nel `.uproject` e
		// l'export dei simboli che oggi i test raggiungono perche' stanno dentro lo stesso modulo. E' un
		// lavoro, non una riga.
		// ⚠️ La prima stesura di questo commento diceva che quel grep «trova un commento su unity build»: era
		// falso, ed e' stato corretto in code review. Il match veniva da una ricerca piu' larga, su
		// `SourceFiles`. Un'evidenza che non riproduce scredita il blocco che dovrebbe sostenere.
		//
		// 🔑 **E il target Game Development ha i test ACCESI**: `WITH_DEV_AUTOMATION_TESTS` vale 1 in ogni
		// configurazione tranne `Test` e `Shipping` (`UnrealBuildTool/Configuration/UEBuildTarget.cs:6311`,
		// UE 5.8). Il pacchetto che il runbook costruisce e' `-clientconfig=Development`
		// (`docs/technical/runbooks/test-e-diagnosi.md` §1): li' i test sono nel binario del gioco **e
		// istanziati**, cioe' interrogabili fuori dall'editor. Escluderli toglierebbe quella possibilita'.
		// ⚠️ **Quella riga e' il DEFAULT, e tre flag di `TargetRules` lo scavalcano nelle due direzioni**
		// (`UEBuildTarget.cs:6313-6324`): `bForceCompileDevelopmentAutomationTests` lo forza a 1 **anche in
		// Shipping**, `bForceDisableAutomationTests` lo forza a 0 **anche in Development**. Misurato il
		// 2026-09-12: `grep -r` su `Source/` non ne trova nessuno, quindi qui il default vale — ed e' questo
		// il fatto, non un invariante del motore. Chi ne impostasse uno in un `*.Target.cs` inverte in
		// silenzio tutto il ragionamento di questo commento.
		//
		// ⚠️ **In Shipping invece NON sono nel binario, e la deduzione contraria e' gia' stata falsificata
		// una volta**: `WITH_AUTOMATION_WORKER` vale 0 (`Core/Public/Misc/Build.h:127`), la macro prende il
		// ramo che definisce la classe senza istanziarla e `/OPT:REF` la scarta. Misurato su #923: togliere
		// 89 test dal binario Shipping ha cambiato **1024 byte su 166 MB**. Chi volesse riaprire questa
		// scelta argomentando «i test finiscono nella build distribuita» sta ripetendo una misura gia'
		// smentita.
		//
		// ∴ il prezzo della scelta e' che la guardia `#if WITH_DEV_AUTOMATION_TESTS` e' **obbligatoria** in
		// ogni `.cpp` di `Tests/`, e che dimenticarla si vede **solo** compilando la Shipping. Il prezzo e'
		// pagato da un oracolo, non dalla memoria di chi scrive: `RefactorTactics.Meta.TestGuardClosesAtEndOfFile`
		// (`Tests/RTTestGuardTests.cpp`) fallisce nella suite Editor invece che alla prossima build di
		// release. L'unita' di misura di quell'oracolo — i `.cpp`, non gli header — e' dichiarata nel suo
		// docstring insieme alla ragione.
	}
}
