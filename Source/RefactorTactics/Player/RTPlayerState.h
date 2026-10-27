#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "RTPlayerState.generated.h"

class APlayerController;

/**
 * L'IDENTITA' DI SQUADRA DEL GIOCATORE, e la sua unica porta di lettura.
 *
 * 🔑 **Il viewer e' del giocatore, non della partita.** Fino a questa fetta viveva su
 * `ARTPlayerController::PlayerTeamId`, un `EditDefaultsOnly` che valeva `0` e che nessuno assegnava a
 * runtime: con due client varrebbe `0` per entrambi, e si romperebbe **in silenzio** perche' `0` e' una
 * risposta plausibile.
 *
 * ⛔ **`TeamId` non e' editabile**: e' stato di runtime, scritto da `AssignTeam`. Quando la replicazione
 * arrivera' il campo diventa `Replicated` con condizione owner-only e il seam e' gia' al posto giusto.
 */
UCLASS()
class REFACTORTACTICS_API ARTPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/** La squadra assegnata. Ripiego `0` finche' nessuno ha assegnato: vedi `TeamIdOf`. */
	int32 GetTeamId() const { return TeamId; }

	/**
	 * Assegna la squadra. **Il nome dichiara l'autorita'**: il chiamante di produzione e'
	 * `ARTGameMode::AssignSeats`, che deriva i posti dal formato e li assegna a chi e' seduto; lo chiamano
	 * anche `RTWorldFixtures::MakePlayerOnTeam` e i test che allestiscono uno stato di prova. Quando
	 * arrivera' la replicazione sara' il server a chiamarlo — la stessa autorita' che il nome gia' dichiara.
	 */
	void AssignTeam(int32 InTeamId) { TeamId = InTeamId; }

	/**
	 * Il GRUPPO DI CONTROLLO dentro la squadra — `CP 19.3`, `#1124`. Quale delle persone sedute in questa
	 * squadra comanda quali unita'.
	 *
	 * Ripiego `0` come `TeamId`: con `UnitsPerPlayer == UnitsPerTeam` — la v0.1 — c'e' un posto solo per
	 * squadra e il gruppo `0` e' l'unico che esista, quindi il default e' anche la risposta giusta.
	 */
	int32 GetControlGroup() const { return ControlGroup; }
	void AssignControlGroup(int32 InGroup) { ControlGroup = InGroup; }

	/**
	 * Il POSTO: squadra e gruppo insieme, assegnati dal GameMode (`ARTGameMode::AssignSeats`) — `#3895`. E' l'unica porta che segna
	 * il giocatore come SEDUTO.
	 *
	 * 🔑 **Esiste perche' `TeamId` e `ControlGroup` valgono `0` di default e non c'e' un «non seduto»**: `0` e' una risposta plausibile,
	 * e senza un marcatore il GameMode non puo' distinguere chi ha ricevuto un posto da chi ha il valore di partenza. Il marcatore e'
	 * PRIVATO e non cambia nessun ripiego pubblico: `TeamIdOf` risponde ancora `0` per un controller senza stato.
	 *
	 * ⚠️ `AssignTeam` e `AssignControlGroup` NON segnano il giocatore come seduto, di proposito: sono le porte che usano le fixture e i
	 * test per allestire uno stato, e un giocatore cosi' allestito deve ricevere il suo posto da `AssignSeats`.
	 */
	void AssignSeat(int32 InTeamId, int32 InControlGroup)
	{
		TeamId = InTeamId;
		ControlGroup = InControlGroup;
		bSeated = true;
	}

	/** Libera il posto: il giocatore non e' piu' seduto. Squadra e gruppo restano com'erano finche' non ne riceve uno nuovo. */
	void ReleaseSeat() { bSeated = false; }

	/** Vero se `AssignSeat` ha dato un posto a questo giocatore e nessuno lo ha liberato. */
	bool HasSeat() const { return bSeated; }

	/** Il gruppo di chi ha in mano quel controller, `0` se non c'e' uno stato. Gemella di `TeamIdOf`. */
	static int32 ControlGroupOf(const APlayerController* Controller);

	/**
	 * 🔑 **L'UNICA porta.** Risale dal controller al proprio `ARTPlayerState` e ripiega su `0`.
	 *
	 * 🔴 **Perche' una porta e non un letterale ad ogni chiamata**: un `= 0` sparso non ha un modo di
	 * fallire chiuso, ed e' il debito che [D-242] punto (5) lasciava scritto — prima di questa porta, sia
	 * `ARTHUD::DrawHUD` che gli altri lettori di squadra avevano ciascuno la propria copia della stessa
	 * domanda («di chi e' la vista?»), e uno di questi era proprio un letterale `PlayerTeamId = 0` che
	 * alimentava sei consumatori a valle, quattro dei quali filtri di privacy. Centralizzare la domanda
	 * qui rende il ripiego una decisione sola, verificabile in un posto solo, invece di N copie che
	 * potrebbero divergere.
	 *
	 * ⚠️ **Il ripiego ha TRE cause e una sola risposta**: controller nullo, nessun PlayerState, PlayerState
	 * della classe sbagliata — quest'ultimo e' cio' che `InitializeActorsForPlay` produce nei mondi di
	 * prova, misurato il 2026-08-30. Tutte e tre valgono `0`.
	 *
	 * ⛔ **`0` e non `INDEX_NONE`**, e la ragione non e' pigrizia: `URTIntentPrivacyLibrary::FilterForTeam`
	 * decide con `Intent.TeamId == ObserverTeamId`, quindi un osservatore invalido non nasconde **di
	 * meno** — rovescia la simmetria, e gli intenti non rivelati dell'avversario diventano «alleati».
	 * Nessuno dei consumatori ha una risposta per «nessuna squadra».
	 */
	static int32 TeamIdOf(const APlayerController* Controller);

private:
	int32 TeamId = 0;

	int32 ControlGroup = 0;

	/** Vedi `AssignSeat`. Non replicato: e' stato di autorita', come la squadra. */
	bool bSeated = false;
};
