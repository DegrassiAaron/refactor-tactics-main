#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "RTBuildPlaygroundPanelCommandlet.generated.h"

/**
 * Genera `WBP_RT_GrayKitPlayground`, l'`EditorUtilityWidget` del Playground Panel (#1993, `D-304`).
 *
 * 🔑 **Perche' un commandlet**, con le parole che `RTSetObjectiveCell` usa gia': *«un `.uasset` non e'
 * diffabile, quindi il diff di una PR non puo' mostrare cosa e' cambiato dentro. Qui il cambiamento e' un
 * comando scritto — si rilegge, si ripete, e se qualcuno lo disfa per sbaglio si riapplica identico»*.
 *
 * ## ⚠️ Cosa genera, e cosa NO — dichiarato invece che scoperto a meta'
 *
 * **Genera**: la gerarchia dei widget, con **nomi stabili** (`Txt_MapState`, `Cmb_Facing`, `Btn_Focus`…)
 * e le tre righe di `DIAGNOSTICS` come testo letterale — quelle stesse che
 * `RefactorTactics.Playground.PanelDiagnosticsLinesAreExact` asserisce, prese da
 * `URTPlaygroundPanelLibrary::DiagnosticsLines()` invece che riscritte.
 *
 * ⛔ **Non genera il grafo**: gli eventi che chiamano il modello — `Focus`, `Select Fixture`, il dropdown
 * del `Facing` — restano authoring. Un `UEdGraph` costruito da codice sarebbe illeggibile in UMG e
 * impossibile da mantenere, e il precedente del progetto (`WBP_RT_ScenarioComposer`) e' autorato a mano.
 *
 * 🔑 **Cio' che rende il cablaggio banale e' che il modello esiste gia'**: ogni pulsante ha una funzione
 * `BlueprintCallable` che fa una cosa sola, e le tre parti provabili headless — station, sei direzioni,
 * `Ready`/`Error` — sono gia' verificate. Il grafo non contiene decisioni: solo chiamate.
 *
 * ## I tre modi, e solo uno e' sicuro su un asset che esiste gia'
 *
 * ```
 * UnrealEditor-Cmd.exe <progetto> -run=RTBuildPlaygroundPanel -RefreshOptions   # <- l'uso normale
 * UnrealEditor-Cmd.exe <progetto> -run=RTBuildPlaygroundPanel                   # rifiuta se esiste
 * UnrealEditor-Cmd.exe <progetto> -run=RTBuildPlaygroundPanel -Force            # rigenera, PERDE il grafo
 * ```
 *
 * 🔑 **`-RefreshOptions` non e' piu' solo «le voci delle combo».** Riconcilia anche la presentazione e
 * **crea i controlli mancanti** (`EnsureFixtureAndViewControls`), restando non distruttivo: il grafo
 * autorato in `RTPlaygroundPanelGraph.dsl` resta dov'e'. E' la via con cui i quattro `USpinBox` e
 * `Chk_Labels` sono entrati nell'asset il 2026-09-05 senza rigenerarlo.
 *
 * ⛔ **`-Force` cancella il grafo**, e il messaggio lo dice prima di farlo: dopo un `-Force` va
 * riapplicato il `.dsl` con `write_graph_dsl`, piu' i tredici `BindToEventProperty` che
 * `write_graph_dsl` **non** crea. Non e' l'uso quotidiano.
 */
UCLASS()
class URTBuildPlaygroundPanelCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
