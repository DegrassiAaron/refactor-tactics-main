// Il pannello del Lab: un'ability canonica si sceglie, si esegue, e si legge cosa e' successo.
//
// 🔑 **Questo widget non decide niente.** Filtro, selezione, costruzione della fixture ed esecuzione
// passano tutti da `FRTLabViewModel`, che vive separato proprio perche' resti qualcosa da misurare: Slate
// su un editor vivo non lo vede nessun automation test — `RTDevSandboxLauncherTests.cpp` lo dichiara in
// testa per il proprio pannello, e vale identico qui.
//
// ⛔ **Nessuna formula di gameplay.** Nessun danno, nessuna portata, nessun costo calcolati qui: il
// readout viene da `URTActionReadoutLibrary` e l'esito dal resolver, attraverso il modello.
//
// ## Un pannello, due issue
//
// ```text
// filtro eroe vuoto      -> il catalogo canonico intero   -> #2599 Ability Lab
// filtro eroe impostato  -> il kit di quell'eroe          -> #2600 Hero Lab
// ```
//
// ⚠️ Il **giudizio visivo** su questo pannello — che si legga, che i controlli si trovino, che il TurnLog
// basti a capire cosa e' successo — appartiene a **#2601**, l'unico proprietario Editor/PIE del giro
// integrato. Questa fetta non lo rivendica.

#pragma once

#include "CoreMinimal.h"
#include "RTLabViewModel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class SEditableTextBox;

class SRTLabPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRTLabPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Il nome del tab, per la registrazione nel modulo. */
	static const FName TabId;

private:
	using FVoce = TSharedPtr<FRTAbilityLabEntry>;

	/** Rilegge l'elenco visibile dal modello. Non filtra qui: chiede. */
	void RiapplicaElenco();

	TSharedRef<SWidget> CostruisciFiltro();
	TSharedRef<SWidget> CostruisciEsecuzione();

	TSharedRef<ITableRow> GeneraRiga(FVoce Voce, const TSharedRef<STableViewBase>& Owner);
	void OnSelezione(FVoce Voce, ESelectInfo::Type);

	/**
	 * Esegue la fixture su un mondo **transitorio**, creato e distrutto qui.
	 *
	 * ⛔ Non usa il mondo dell'editor: il Lab esegue una fixture, non il livello aperto, e posare unita'
	 * nel livello di qualcuno sarebbe un effetto collaterale che nessuno ha chiesto.
	 */
	FReply OnEsegui();

	FText TestoIdentita() const;
	FText TestoParametri() const;
	FText TestoEsito() const;
	FText TestoTurnLog() const;

	FRTLabViewModel Modello;

	TArray<FVoce> Voci;
	TSharedPtr<SListView<FVoce>> Lista;
	TSharedPtr<SEditableTextBox> CampoFiltro;

	/** L'ultimo errore di costruzione o esecuzione, per mostrarlo invece di lasciare il pannello muto. */
	FString UltimoErrore;
};
