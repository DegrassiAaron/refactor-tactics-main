#pragma once

#include "CoreMinimal.h"
#include "RTAnimBrowserModel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class SEditableTextBox;
class SRTAnimPreviewViewport;

/**
 * Il browser delle animazioni: si guarda una clip, e si decide.
 *
 * 🔑 **Questo widget non decide niente.** Ogni comando passa da `FRTAnimBrowserModel`, che vive
 * separato proprio perche' resti qualcosa da misurare: Slate su un editor vivo non lo vede nessun
 * automation test — `RTDevSandboxLauncherTests.cpp` lo dichiara in testa per il proprio pannello, e
 * vale identico qui. Le regole (chi puo' scrivere `Status`, che una variante entra inattiva, che
 * `Make Active` e' atomico) sono provate sul modello; qui c'e' il cablaggio.
 *
 * ⛔ **Nessuna euristica.** Non esiste un pulsante «promuovi tutte quelle che sembrano buone»: `Promoted`
 * significa che una persona ha guardato la clip, e l'unico modo di scriverlo e' che quella persona prema
 * il pulsante su quella riga.
 *
 * ⚠️ Il giudizio visivo su questo pannello — che si legga, che i controlli si trovino, che l'anteprima
 * basti a decidere — e' di **#2444**, l'unico proprietario Editor/PIE. Questa issue non lo rivendica.
 */
class SRTAnimBrowserPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRTAnimBrowserPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Il nome del tab, per la registrazione nel modulo. */
	static const FName TabId;

private:
	using FRowPtr = TSharedPtr<FRTAnimBrowserRow>;

	// ── Dati ────────────────────────────────────────────────────────────────────────────────────────
	void Ricarica();
	void RiapplicaFiltri();
	void Salva();

	// ── Widget ──────────────────────────────────────────────────────────────────────────────────────
	TSharedRef<ITableRow> GeneraRiga(FRowPtr Riga, const TSharedRef<STableViewBase>& Owner);
	void OnSelezione(FRowPtr Riga, ESelectInfo::Type);

	TSharedRef<SWidget> CostruisciBarraFiltri();
	TSharedRef<SWidget> CostruisciAzioni();
	TSharedRef<SWidget> CostruisciTrasporto();

	/** L'unico ingresso ai comandi di stato: un click su un pulsante, e nient'altro. */
	FReply OnComandoStato(ERTAnimClipStatus Nuovo);

	FText TestoStatoSelezione() const;

	FRTAnimBrowserModel Modello;
	FString PercorsoCatalogo;
	FString UltimoErrore;

	TArray<FRowPtr> Righe;
	TSharedPtr<SListView<FRowPtr>> Lista;
	TSharedPtr<SRTAnimPreviewViewport> Anteprima;
	FRowPtr Selezione;
};
