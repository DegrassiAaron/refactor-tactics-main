#include "UI/RTReactionWindowViewModel.h"

#include "Turn/RTTurnManager.h"

void URTReactionWindowViewModel::Hook(ARTTurnManager* InTurnManager)
{
	if (!InTurnManager)
	{
		return;
	}

	TurnManager = InTurnManager;

	// 🔑 **QUESTA RIGA E' LA ISSUE.** Prima di lei `OnReactionWindowOpened.IsBound()` era falso in partita, e
	// le quattro condizioni che aprono la finestra non passavano mai la prima: `#2679`, `#2692` e `#2717`
	// erano costruite fino al confine della UI e irraggiungibili.
	//
	// ⚠️ **`BindUObject` e non `BindLambda`**: `IsBound()` interroga `IsSafeToExecute()`, che per un binding
	// UObject verifica il weak pointer. Cosi' un view model distrutto spegne il ramo interattivo da solo,
	// invece di lasciare una finestra che si apre per un ascoltatore che non c'e' piu'.
	InTurnManager->OnReactionWindowOpened.BindUObject(
		this, &URTReactionWindowViewModel::HandleWindowOpened);
}

void URTReactionWindowViewModel::HandleWindowOpened(const FRTReactionWindowView& View,
	int32 /*OwnerUnitId*/)
{
	Window = View;

	// ⚠️ **L'id si CHIEDE al manager, non si deriva dalla chiave.** Entrambi i siti scrivono
	// `OpenWindowOpportunityId` **prima** di eseguire il delegate — `RTTurnManager_Movement.cpp` e
	// `RTTurnManager_Blast.cpp`, dove il Blast alza anche `bSuspended` prima dell'`Execute` — quindi qui
	// l'accessore risponde gia' con l'identita' di questa finestra.
	//
	// 🔴 **E ricomporla sarebbe il difetto che `FireResponseTarget` esiste per non far nascere**:
	// `DeriveOpportunityId` e' un formato con un solo produttore, e chi lo riscrivesse qui ne sarebbe il
	// secondo — fuori dai test del core, e libero di divergere.
	if (const ARTTurnManager* TM = TurnManager.Get())
	{
		WindowOpportunityId = TM->GetOpenReactionWindowId();
	}

	// ⛔ Nessuna risposta, nessuna soglia, nessun giudizio: registrare e' tutto cio' che accade qui. Chi
	// disegna interroga; chi decide e' il giocatore, e la sua risposta entra da `SubmitResponse`.
}

bool URTReactionWindowViewModel::IsWindowOpen() const
{
	if (WindowOpportunityId.IsEmpty())
	{
		return false;
	}

	const ARTTurnManager* TM = TurnManager.Get();
	if (!TM)
	{
		return false;
	}

	// 🔑 **L'uguaglianza, e non la sola non-vuotezza.** Una finestra scaduta e' seguita spesso da un'altra —
	// `ExpireReactionWindow` riprende la resolution, e la ripresa puo' riaprire subito: chiedere «c'e' una
	// finestra?» direbbe vero anche per quella nuova, e il widget mostrerebbe le opzioni di una domanda
	// diversa da quella a cui il giocatore crede di rispondere.
	return TM->GetOpenReactionWindowId() == WindowOpportunityId;
}

FRTReactionWindowView URTReactionWindowViewModel::GetWindow() const
{
	// Vista ai default quando la finestra non e' piu' quella consegnata: `bOpen` falso e nient'altro, la
	// stessa forma che `FilterWindowForTeam` da' a chi non deve sapere che una finestra esiste. Restituire
	// la vista stale sarebbe l'unico modo di far disegnare una finestra chiusa.
	return IsWindowOpen() ? Window : FRTReactionWindowView();
}

float URTReactionWindowViewModel::GetRemainingSeconds() const
{
	if (!IsWindowOpen())
	{
		return -1.f;
	}

	const ARTTurnManager* TM = TurnManager.Get();
	return TM ? TM->GetOpenReactionWindowRemainingSeconds() : -1.f;
}

void URTReactionWindowViewModel::SubmitResponse(const FString& Response)
{
	ARTTurnManager* TM = TurnManager.Get();
	if (!TM || WindowOpportunityId.IsEmpty())
	{
		return; // nessuna finestra da chiudere: una risposta senza domanda non e' un errore, e' un ritardo
	}

	// ⚠️ **Si nomina la finestra REGISTRATA, non quella aperta adesso.** E' cio' che porta il gate
	// dell'identita' del manager fin dentro il canale umano: se questa risposta arriva dopo che la sua
	// finestra e' scaduta, `SubmitReactionResponse` la ignora invece di chiudere quella successiva.
	TM->SubmitReactionResponse(WindowOpportunityId, Response);
}
