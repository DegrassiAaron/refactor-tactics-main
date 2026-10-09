#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTHexCellVisibility.generated.h"

struct FRTHexCellData;

/**
 * Se un campo della cella si puo' comporre senza sapere PER CHI si compone.
 *
 * 🔑 **Due valori, e il secondo ne ha un solo abitante — che e' il reperto, non una sciatteria.**
 * Misurato il 2026-09-23 su `FRTHexCellData`: ogni suo campo e' geometria di mappa **redatta a mano**
 * nell'asset (`URTHexMapAsset::Cells`), e una mappa non e' segreta: i due giocatori la guardano insieme.
 * Cio' che e' segreto e' **chi ci sta sopra**, e quello non e' un campo di questa struct — vive nello
 * snapshot, che nasce gia' per un osservatore.
 */
UENUM(BlueprintType)
enum class ERTCellFieldVisibility : uint8
{
	/** Componibile per chiunque veda la mappa: e' la mappa. */
	Public,

	/**
	 * Componibile solo da uno snapshot costruito PER quell'osservatore.
	 *
	 * ⚠️ Non significa «filtralo prima di stamparlo»: significa che la vista non si costruisce affatto
	 * se la fonte non e' gia' quella giusta — [#1805](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1805),
	 * *«la privacy non e' non disegnare, e' non costruire la vista»*.
	 */
	ObserverDependent
};

/**
 * La classificazione di visibilita' dei campi di cella, in **un solo posto**, sul modello gia' provato di
 * `URTReplayPrivacyLibrary::FieldVisibility()` per `FRTTurnLogEntry`.
 *
 * ⛔ **Non sta in `Debug/`, e la collocazione e' il punto.** Questa e' una regola di visibilita', e una
 * regola di visibilita' non si possiede da dentro uno strumento di ispezione: sarebbe esattamente cio' che
 * [#2485](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2485) esiste per impedire — *«una
 * terza regola di visibilita' scritta per omissione»*. Vive accanto alla struct che classifica.
 *
 * 🔴 **A che serve una tabella i cui valori sono tutti `Public`.** Non a filtrare: a **obbligare a
 * classificare**. Chi aggiunge una `UPROPERTY` a `FRTHexCellData` e non dice se sia pubblica trova rosso
 * `RefactorTactics.Debug.EveryCellFieldIsClassified`, e la scelta diventa una decisione presa invece di un
 * default subito. E' il difetto che `#1805` chiede per nome sul proprio modello, qui sul modello della mappa.
 *
 * **Sola lettura, nessuno stato.** Le funzioni qui non guardano il mondo: si provano senza livello.
 */
UCLASS()
class REFACTORTACTICS_API URTHexCellVisibilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Nome del campo -> visibilita'. Copre **tutti** i campi riflessi di `FRTHexCellData`, e solo quelli:
	 * totalita' e assenza di fantasmi sono misurate da
	 * `RefactorTactics.Debug.EveryCellFieldIsClassified`, che le legge per reflection e non da un elenco
	 * scritto una seconda volta.
	 */
	static const TMap<FName, ERTCellFieldVisibility>& FieldVisibility();

	/**
	 * L'occupante: l'unico dato del contesto-cella che sia `ObserverDependent`, e **non e' un campo di
	 * `FRTHexCellData`**. Sta qui perche' la classificazione deve essere leggibile in un posto solo, e
	 * perche' era proprio la sua assenza dalla struct a farlo sfuggire a ogni gate.
	 *
	 * 🔴 **Il motivo per cui e' segreto sta scritto altrove e non si riscrive qui**:
	 * `ERTKnowledgeVisibility::Remembered` dichiara *«cella dell'ULTIMO CONTATTO, sagoma. Mai la posizione
	 * vera»*. Comporre l'occupante da uno snapshot onnisciente per una vista di squadra rivelerebbe
	 * esattamente la posizione che quella riga vieta.
	 */
	static FName OccupantField();

	/**
	 * Uno snapshot costruito per `SnapshotObserverTeamId` puo' comporre una vista per `ViewObserverTeamId`?
	 *
	 * 🔑 **Solo se coincidono, e la regola e' volutamente senza scorciatoie.** In particolare uno
	 * snapshot **onnisciente** non compone una vista di squadra: contiene tutto, e filtrarlo qui vorrebbe
	 * dire riscrivere la regola di conoscenza dentro un compositore — che e' la tentazione che
	 * `DescribeIntents` evita passando da `URTIntentPrivacyLibrary::FilterForTeam`. Si ricostruisce lo
	 * snapshot per l'osservatore giusto, oppure non si compone.
	 *
	 * ⚠️ Il verso opposto — snapshot di squadra, vista onnisciente — e' **anch'esso** rifiutato: non
	 * perde nulla, ma prometterebbe un audit completo su una fonte che completa non e'.
	 */
	static bool SnapshotEntitles(int32 SnapshotObserverTeamId, int32 ViewObserverTeamId);
};
