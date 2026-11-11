#pragma once

#include "CoreMinimal.h"
#include "Turn/Playback/RTPlaybackHost.h"    // FRTPlaybackHost, FRTPlaybackTuning: cio' che il playback chiede all'orchestratore
#include "Turn/Playback/RTPlaybackSignals.h" // FRTPlaybackSignals: i delegate che trasmette
#include "Turn/Resolution/RTResolvedEvent.h"
#include "Turn/Playback/RTPlaybackLibrary.h"          // FRTPhaseTime, FRTBlastSequenceElement, ERTPlaybackStopAt
#include "Turn/Resolution/RTTurnRules.h"                // ERTMatchPhase
#include "Map/RTCellId.h"
#include "UI/Feed/RTActivationLine.h"             // FRTActivationLine: la riga del Registro per un'attivazione mostrata (#4033)

class ARTUnit;

/**
 * Il playback di presentazione: la riproduzione VISIVA di un turno che la simulazione ha gia' risolto (#4057, #1818).
 *
 * 🔑 **Perche' esce da `ARTTurnManager`.** E' il primo punto in scope di E50 (#1816): *il playback dentro l'autorita'
 * competitiva*. #1817 ne aveva gia' portato fuori la matematica (`URTPlaybackLibrary`); restavano lo stato, l'orologio,
 * i cursori e le uscite verso gli Actor, che sono questa classe. Il piano di struttura del 2026-11-05 la colloca qui:
 * `Turn/Playback/` riproduce, non decide.
 *
 * ⛔ **Non decide niente, e la forma lo rende verificabile.** Qui non si scrive stato canonico: la timeline arriva in
 * sola lettura (`Timeline()`), cella e facing delle unita' si LEGGONO, e l'unico atto di flusso — chiudere il turno —
 * si chiede all'orchestratore (`FRTPlaybackHost::ConcludeTurn`). Tutto cio' di cui il director ha bisogno sta in
 * `FRTPlaybackHost`: se un giorno servisse un membro del TurnManager che li' non c'e', e' li' che va aggiunto e
 * motivato, non raggiunto di nascosto.
 *
 * ⚠️ **L'API pubblica ha due livelli.** I metodi qui sotto hanno gli stessi nomi delle facciate di `ARTTurnManager`,
 * che restano il contratto per HUD, PlayerController, harness, Blueprint e test: la documentazione di ciascuno sta li',
 * e qui non si ripete — due copie divergerebbero. Le note nella sezione `private` sono quelle che documentavano i metodi
 * e lo stato dentro il TurnManager, spostate con loro.
 *
 * Senza riflessione, come `FRTPacingRecorder` e `FRTReplayRecording`: nessun membro era una `UPROPERTY`, e i puntatori
 * ad Actor sono `TWeakObjectPtr`, che il garbage collector non ha bisogno di vedere.
 */
class REFACTORTACTICS_API FRTPlaybackDirector
{
public:
	/** Lega il director al suo orchestratore: una volta, nel costruttore del TurnManager. */
	void Bind(FRTPlaybackHost& InHost, const FRTPlaybackSignals& InSignals);

	// --- Le facciate di ARTTurnManager: stesse firme, documentate li' --------------------------------------------
	void RefreshPresentationGeometry(float PreviousLayerHeight);
	bool AnimatedCellFor(const ARTUnit* Unit, FRTCellId& OutCell) const;
	int32 MicroStepsInCurrentPlaybackPhase() const;
	void SetPlaybackControlsEnabled(bool bEnabled);
	void SetStartPlaybackPaused(bool bStartPaused);
	void PausePlayback();
	void ResumePlayback();
	void StepMicroStep();
	void RequestPlaybackStopAt(ERTPlaybackStopAt Boundary);
	void SkipPlayback();
	int32 PlaybackActivationsQueuedForTest() const;
	int32 PlaybackAnimCellIndexForTest(const ARTUnit* Unit) const;
	ERTMatchPhase GetPlaybackPhase() const;
	FString GetPlaybackPhaseName() const;
	float GetPlaybackProgress01() const;

	// --- Gli ingressi dell'orchestratore -------------------------------------------------------------------------
	/** Avvia il playback della risoluzione (movimento in parallelo, fasi a beat). */
	/** Avvia il playback. Con `bPreserveClock` ESTENDE quello in corso invece di ricominciarlo (#2679). */
	void BeginPlayback(bool bPreserveClock = false);

	void TickPlayback(float DeltaSeconds);

	/** Fa girare i pivot in corso. Gira a ogni tick, anche a playback finito: il pivot dura oltre la fase. */
	void TickPresentationPivots(float DeltaSeconds);

	// --- Letture e azzeramenti che l'orchestratore chiede --------------------------------------------------------
	bool ArePlaybackControlsEnabled() const { return bPlaybackControlsEnabled; }
	bool DoesPlaybackStartPaused() const { return bStartPlaybackPaused; }
	ERTPlaybackStopAt GetArmedPlaybackStop() const { return PlaybackStopAt; }
	bool IsPlaybackPaused() const { return bPlaybackPaused; }
	bool IsPlaybackHeldByWindow() const { return bPlaybackHeldByWindow; }
	bool IsPresentationPivotRunning() const { return PivotAnims.Num() > 0; }
	float GetPlaybackSlackScale() const { return PlaybackSlackScale; }
	const TArray<FRTActivationLine>& GetActivationLines() const { return ActivationLines; }

	/** L'orologio TOTALE del playback: la barra di avanzamento e `Pacing.Close`. */
	float GetElapsedTotal() const { return PlaybackElapsedTotal; }

	/**
	 * ⚠️ **Li chiama la risoluzione, non il playback**: le righe del Registro si azzerano con il TurnLog in
	 * `LockInAndResolve`, l'orologio e la scala del budget in `ConcludeResolution`. Le ragioni sono scritte accanto alle
	 * chiamate, in `RTTurnManager.cpp`.
	 */
	void ResetActivationLines() { ActivationLines.Reset(); }
	void ResetElapsedTotal() { PlaybackElapsedTotal = 0.f; }
	void ResetSlackScale() { PlaybackSlackScale = 1.f; }

	// --- Solo test: le letture che i ganci `*ForTest` del TurnManager inoltrano ----------------------------------
	const TArray<FString>& AttackBeatTraceForTest() const { return AttackBeatTrace; }
	int32 PlaybackBlastShownForTest() const { return BlastElementsShown(); }
	float PlaybackPhaseElapsedForTest() const { return PlaybackPhaseElapsed; }
	float PlaybackSkillCalloutClockForTest() const { return SkillCalloutClock; }
	int32 SkillCalloutUnitsForTest() const { return SkillCalloutUnits.Num(); }
	TArray<int32> PlaybackBlastSequenceIndicesForTest() const
	{
		TArray<int32> Out;
		for (const FRTBlastSequenceElement& E : PlaybackBlastSequence) { Out.Add(E.TimelineIndex); }
		return Out;
	}

private:
	/** Le manopole, chieste a ogni uso: vedi `FRTPlaybackTuning`. */
	FRTPlaybackTuning Tuning() const { return Host->GetPlaybackTuning(); }

	/** La timeline del turno, in sola lettura. */
	const TArray<FRTResolvedEvent>& Timeline() const { return Host->GetPlaybackTimeline(); }

	FRTPlaybackHost* Host = nullptr;
	FRTPlaybackSignals Signals;

	/**
	 * Rivela le attivazioni fino a `UpTo`, in ordine di timeline; `true` se un confine d'atto ha messo in
	 * pausa (#3549). ⚠️ Si ferma **dopo** aver mostrato il fatto, come `#2855` prescrive: quelle che il tick
	 * avrebbe ancora rivelato restano per la ripresa. ⛔ Ogni chiamante esce dal tick su `true` — il ramo per tick
	 * e la rete di fine fase — o il passaggio alla fase dopo perderebbe le attivazioni in coda.
	 */
	bool RevealPlaybackActivations(const TArray<FRTResolvedEvent>& Activations, int32 UpTo);

	/**
	 * Percorre la sequenza di Blast sul cursore dei battiti fino a `BeatsTarget` (#3549 D5 con `#2454`); `true` se il
	 * tick deve uscire: un confine d'atto ha messo in pausa, o un ascoltatore di `OnAttackResolved` ha chiuso il
	 * playback.
	 *
	 * 🔑 **Un cursore solo, `BlastBeatsDone`, sugli elementi della sequenza**: il battito `2k` RIVELA l'elemento `k`
	 * — un ramo per tipo; per un `Attack` e' il LANCIO — e il `2k+1` e' il suo ARRIVO, che fa qualcosa solo per un
	 * `Attack` (`Hit`, numero, `Colpo:`, `OnAttackResolved`). Il volo e' `PlaybackBlastFlights[k]`, zero per ogni
	 * altro tipo, quindi l'elemento `k` esce a `k·A` come con `AttacksToShow` e l'arrivo cade prima di `k+1`.
	 * ⛔ Due cursori — la sequenza da una parte, i colpi dall'altra — in un tick lungo rivelerebbero l'elemento dopo
	 * prima dell'arrivo del colpo: e' la stessa ragione per cui `#2454` ne ha uno.
	 *
	 * ⛔ **Non riordina, non aggrega e non ricalcola**: l'ordine e' quello che `BuildBlastSequence` ha deciso, le
	 * celle dell'impronta e il bordo del muro si LEGGONO dall'evento (`#2454`, `#2828`).
	 * ⚠️ Il confine `Next Action` di un colpo cade al suo ARRIVO (`#2454`), quello di ogni altro elemento alla
	 * rivelazione: un colpo e' mostrato quando arriva.
	 */
	bool AdvanceBlastSequence(int32 BeatsTarget);

	/** Quanti elementi della sequenza di Blast sono gia' stati RIVELATI: il prefisso congelato di D-355. */
	int32 BlastElementsShown() const { return (BlastBeatsDone + 1) / 2; }

	/** Il numero d'ordine del colpo fra i soli `Attack` della sequenza, per la traccia dei battiti dei test. */
	int32 BlastAttackOrdinal(int32 SequenceIndex) const;

	/** Aggiorna l'atto in corso e consuma un `Next Action` armato: la regola e' `IsActBoundary` (#3292, #3549). */
	bool NotePlaybackActShown(const FRTResolvedEvent& Ev);

	/** Il LANCIO di un colpo (`#2454`): il ruolo `Attack` sull'attaccante. `AttackOrdinal` serve solo alla traccia. */
	void LaunchPlaybackAttack(const FRTResolvedEvent& Atk, int32 AttackOrdinal);

	/**
	 * L'ARRIVO di un colpo (`#2454`): la riga `Colpo:`, `Hit`, numero e `OnAttackResolved`.
	 * ⚠️ **Scrive la riga anche dalla rete di fine fase** (#3549, spec «il momento» §6): la rete percorre lo
	 * stesso cursore del ciclo, e un recupero che tacesse il feed sarebbe una seconda versione dello stesso fatto.
	 * ⛔ Trasmette `OnAttackResolved`: un ascoltatore puo' chiudere il playback, e chi chiama lo verifica prima di
	 * rileggere qualunque stato del playback.
	 */
	void ArrivePlaybackAttack(const FRTResolvedEvent& Atk, int32 AttackOrdinal);

	/**
	 * Consegna alla mappa il tracer in volo (`#2454`). Col cursore unico ce n'e' AL PIU' UNO: l'elemento dopo
	 * segue l'arrivo del colpo. Il disegno dipende da chi guarda (`TracerStyleFor`), il ritmo no.
	 * ⚠️ Non tocca la mappa se non c'e' nulla in volo e l'ultima consegna era gia' vuota (`bPlaybackTracerChannelFull`).
	 */
	void PushPlaybackTracers();

	/**
	 * Consegna alla mappa le cue del profilo FX della fase (#3578, spec §2.4): Prep e Dash dalle code di attivazione,
	 * il Blast dalla sequenza e dal cursore dei battiti. Gemella di `PushPlaybackTracers`, con un flag gemello: non tocca
	 * la mappa se non c'e' nulla da dire e l'ultima consegna era gia' vuota (`bPlaybackCueChannelFull`).
	 */
	void PushPlaybackCues(ERTMatchPhase InPhase);

	/**
	 * Mette in pausa il playback su un confine d'atto e disarma il predicato — `#3292`.
	 *
	 * 🔑 **Esiste perche' i siti che la chiamano sono piu' d'uno** — `NotePlaybackActShown`, per ogni fatto che
	 * il playback rivela, e il cambio di fase — e quattro righe ripetute sono altrettante occasioni di
	 * dimenticarne una. ⏱️ *Fino a #3549 erano i tre canali del `Blast`.* ⚠️ `PlaybackStepTargetElapsed = -1`
	 * e' quella che si dimentica: senza, un passo pendente riprenderebbe da solo subito dopo la pausa.
	 */
	void PausePlaybackAtActBoundary();

	/**
	 * Il beat di un `AbilityActivated`: la clip del ruolo `Cast` sulla sorgente e la riga `Attiva:` del feed
	 * (#3549, spec §2.3), e la riga del Registro (#4033, `AddActivationLine`). ⛔ Non decide se mostrarlo: il filtro di
	 * privacy sta a monte, in `BeginPlayback` e in `BuildBlastSequence`. La riga `Attiva:` porta il verdetto CONGELATO
	 * nell'evento, non uno ricalcolato.
	 */
	void ShowActivation(const FRTResolvedEvent& Ev);

	/**
	 * La riga del Registro per un'attivazione mostrata (#4033, R1/R5/R7b): una sola per attivazione, con il testo composto
	 * adesso dai campi di `Src`, la squadra di chi guarda (`PlaybackViewerTeamId`) e il turno del playback. Nessuna riga se
	 * `Src` e' nullo o se l'attivazione ne ha gia' una (chiave `(fase, sorgente, azione)`: vedi la definizione).
	 * ⛔ Non giudica la conoscenza: la chiamano solo `ShowActivation` e il recupero di fine playback (R8), che leggono le
	 * liste gia' filtrate per chi guarda.
	 */
	void AddActivationLine(const FRTResolvedEvent& Ev, const ARTUnit* Src);

	/**
	 * Porta ai fumetti di skill accesi il tempo del playback (`SkillCalloutClock`) e toglie dalla lista le unita'
	 * che non esistono piu' (#4025, spec del fumetto R4/A8). ⚠️ **Non e' l'unico punto in cui il fumetto riceve
	 * l'orologio**: alla comparsa lo riceve `ShowActivation`, e questo lo ripete a ogni tick in cui il tempo avanza.
	 */
	void SyncSkillCalloutClocks();

	void EnterPlaybackPhase();

	void FinishPlayback();

	/** Avvia, o riavvia dallo yaw attuale, il pivot finale di `Unit` verso `ToYaw` (`#2167`). */
	void StartPresentationPivot(ARTUnit* Unit, float ToYaw);

	/** Lo yaw del facing LOGICO dell'unita' sulla sua cella: dove la mesh deve finire. */
	float FinalFacingYaw(const ARTUnit* Unit) const;

	/**
	 * I due termini della fase — movimento e attesa — prima che il budget tocchi il secondo.
	 * Raccoglie gli ingressi che solo il TurnManager possiede e delega la formula a
	 * `URTPlaybackLibrary::PhaseTime`. ⚠️ Porta anche l'anticipo delle attivazioni (`Lead`), che le rotte del Dash, la
	 * corsa rinviata e `StepMicroStep` leggono da qui (review della PR #3561).
	 */
	FRTPhaseTime PhaseTimeForPlaybackPhase(ERTMatchPhase InPhase) const;

	/** La durata della fase come sara' riprodotta: locomozione intatta, attese scalate dal budget. */
	float DurationForPlaybackPhase(ERTMatchPhase InPhase) const;

	/**
	 * L'indice di cella su cui ogni anim si trovava all'ultimo fotogramma, parallelo a `MoveAnims`.
	 *
	 * E' cio' che rende osservabile il **confine** di micro-step: senza, «l'unita' e' a meta' fra due celle»
	 * e «l'unita' e' appena entrata in una cella nuova» sarebbero lo stesso frame.
	 */
	TArray<int32> PlaybackAnimCellIndex;

	/** Lo yaw con cui ogni anim entra in scena, preso al suo PRIMO tick (`#2167`); `TNumericLimits<float>::Max()` = non ancora. */
	TArray<float> PlaybackAnimEntryYaw;

	/** Vero quando l'anim e' arrivata e ha passato la posa al pivot finale (`#2167`). */
	TArray<bool> PlaybackAnimArrived;

	/** Un pivot finale di presentazione: l'unita' gira sul posto da `From` a `To` (`#2167`). */
	struct FRTPivotAnim
	{
		TWeakObjectPtr<ARTUnit> Unit;
		float From = 0.f;
		float To = 0.f;
		float Elapsed = 0.f;
	};

	TArray<FRTPivotAnim> PivotAnims;

	/** Vero mentre `FinishPlayback` arriva da `SkipPlayback`: saltare scatta, non anima. */
	bool bFinishingBySkip = false;

	/** Animazione di movimento di una singola unita': waypoint gia' convertiti in mondo + fase (Dash/Move). */
	struct FRTMoveAnim
	{
		TWeakObjectPtr<ARTUnit> Unit;
		TArray<FVector> World; // start + celle attraversate, in coordinate mondo

		/**
		 * Le stesse pose di `World`, in celle (`#2876`). Parallelo e della **stessa lunghezza**: e' la
		 * copia troncata dal prefisso osservabile, non la rotta reale.
		 *
		 * 🔴 **Il troncamento e' il punto.** `World` nasce gia' tagliato da
		 * `URTTeamKnowledgeLibrary::ObservedPrefixLength`, e chi ricava la visibilita' durante il playback
		 * deve guardare **cio' che l'anim disegna**, non dove l'unita' e' passata davvero. Un secondo array
		 * costruito dalla rotta piena rivelerebbe esattamente il tratto che [D-223] tronca.
		 */
		TArray<FRTCellId> Cells;

		ERTMatchPhase Phase = ERTMatchPhase::Move; // fase in cui va riprodotta (Dash o Move)

		// `#3117`: gli stati che l'unita' portava QUANDO si e' mossa, copiati dall'evento risolto.
		// ⛔ **Non si rilegge `Unit` per averli**, benche' sia proprio li' sopra: darebbe lo stato al
		// momento del playback, e la posa smetterebbe di essere una funzione del solo tempo normalizzato.
		TArray<FName> SourceStatusNames;

		/**
		 * Quante celle di `Cells` appartengono al PIANO di chi si muove — `#3263`. Oltre c'e' l'estensione
		 * che il terreno ha imposto: uno scivolamento su ghiaccio.
		 *
		 * 🔴 **Troncato come `World` e `Cells`, e per la stessa ragione.** Se il prefisso osservato taglia
		 * la rotta, il numero di celle pianificate **visibili** puo' essere minore di quello reale: un
		 * valore non troncato direbbe a chi disegna che l'unita' aveva pianificato oltre cio' che si vede
		 * — che e' esattamente il tratto che [D-223] nasconde.
		 *
		 * ⚠️ `0` significa «tutto pianificato», la stessa convenzione dell'evento da cui viene.
		 */
		int32 PlannedLength = 0;

		/**
		 * Da quando, sull'orologio della fase, l'anim comincia — `#3666`. `0` per tutte, tranne quelle che
		 * un'estensione (`BeginPlayback(true)`) aggiunge alla fase che si stava mostrando: per loro e' l'istante
		 * dell'estensione, e corrono alla velocita' di rotta da li'.
		 *
		 * 🔴 **Senza, un'anim nuova si animava sull'orologio di una fase gia' mostrata.** Da `#3666` si risponde
		 * a una finestra a playback trattenuto, cioe' a fase finita: lo scarto `SIDESTEP` del `Brace` nasceva con
		 * l'alpha di fase gia' a `1` e scattava a destinazione in un fotogramma (misurato: zero fotogrammi
		 * intermedi). Prima di `#3666` la risposta arrivava a Blast in corso, e saltava a meta' strada.
		 */
		float StartElapsed = 0.f;
	};

	/**
	 * 🔴 **Pause/Step sono NEGATI finche' qualcuno non li abilita, e il default e' la regola** (`#1879`).
	 *
	 * La matrice delle modalita' di `#1879` vieta Pause e Step nel **PvP live competitivo**: l'autorita' non
	 * puo' essere fermata localmente da un client, e la deducibilita' del ritmo ha gia' un proprietario
	 * (`#759`).
	 *
	 * ⚠️ **Quella modalita' non esiste ancora nel codice** — `ERTMatchMode`, `bCompetitive`, `bIsPvP`:
	 * zero occorrenze su `origin/main`, coerente con l'assenza di replica di rete misurata su `#1805`.
	 * ⛔ Inventarla per poterla negare sarebbe fabbricare una dipendenza. Il divieto si ottiene invece
	 * **per costruzione**: chi non abilita esplicitamente questi controlli non li ha. Il giorno in cui il
	 * PvP live esistera' sara' gia' fuori, senza che nessuno debba ricordarsi di escluderlo — che e' la
	 * differenza fra un fail-closed e una convenzione.
	 */
	bool bPlaybackControlsEnabled = false;

	/**
	 * Ogni playback di questa sessione comincia fermo (`#2858`).
	 *
	 * ⚠️ **Separato da `bPlaybackPaused`, che e' lo stato corrente.** Questo e' una **politica di sessione**
	 * — vale per ogni turno finche' non la si spegne — mentre quello dice se l'immagine e' ferma **adesso**.
	 * Fonderli renderebbe `ResumePlayback` una revoca della politica: si riprenderebbe una volta e il turno
	 * dopo ripartirebbe da solo, che e' l'opposto di cio' che chiede chi sta ispezionando.
	 *
	 * ⛔ Nasce `false` come `bPlaybackControlsEnabled`, e come quello non si accende da se'.
	 */
	bool bStartPlaybackPaused = false;

	/**
	 * Il playback e' fermo.
	 *
	 * ⚠️ **Ferma la PRESENTAZIONE.** Questa riga aggiungeva *«la risoluzione e' gia' avvenuta per intero»*, e
	 * dal 2026-09-08 non e' piu' vero (`#2679` fetta 3, [D-355]): con una finestra di reazione aperta il
	 * playback mostra un turno **risolto a meta'**, e cio' che lo tiene fermo in quel caso non e' questo flag
	 * ma `bPlaybackHeldByWindow`. I due sono separati apposta — vedi la sua dichiarazione.
	 */
	bool bPlaybackPaused = false;

	/**
	 * Il playback ha mostrato tutto cio' che la resolution ha risolto, e ATTENDE una finestra (#2679).
	 *
	 * ⛔ **Non e' `bPlaybackPaused`, e tenerli separati e' la scelta**: quella e' la pausa del giocatore, ha
	 * i suoi comandi e un suo significato. Con un flag solo, un `ResumePlayback` durante una finestra
	 * farebbe ripartire un playback che non ha nulla da mostrare.
	 */
	bool bPlaybackHeldByWindow = false;

	/**
	 * Se `>= 0`, il playback avanza fino a questo `PlaybackPhaseElapsed` e poi si ferma: e' lo `Step`.
	 *
	 * 🔑 **Un tempo e non un contatore di frame**: il confine si calcola una volta, alla pressione, e il
	 * tick ci arriva senza sapere quanti frame servono. Un «avanza per N tick» dipenderebbe dal frame rate,
	 * e la stessa pressione fermerebbe il playback in punti diversi su macchine diverse.
	 */
	float PlaybackStepTargetElapsed = -1.f;

	TArray<FRTMoveAnim> MoveAnims;          // derivati dagli eventi Move
	TArray<FRTResolvedEvent> PlaybackDefeated; // eventi Defeated, mostrati a fine della loro fase

	/**
	 * Le attivazioni VISIBILI di Prep e di Dash, in ordine di timeline (#3549, spec §2.4). Una sorgente che
	 * chi guarda non ha il diritto di vedere non entra: tutto o niente (D6).
	 */
	TArray<FRTResolvedEvent> PlaybackActivationsPrep;
	TArray<FRTResolvedEvent> PlaybackActivationsDash;

	/**
	 * Il Blast come UNA sequenza per intento (#3549, D5): attivazione, impronte, muri, colpi, un atto dopo
	 * l'altro. 🔴 Sostituisce i canali paralleli di prima (colpi, impronte da #2454, muri da #2828), che si
	 * svelavano con contatori indipendenti: «l'attivazione precede il colpo dello stesso intento» non
	 * discendeva dall'ordine delle code. La costruisce `URTPlaybackLibrary::BuildBlastSequence`.
	 *
	 * ⚠️ **Un elemento e' un indice di timeline, e il tipo lo DICHIARA l'evento.** `AdvanceBlastSequence` sceglie
	 * la cue per `Type` — un `Attack` per VITTIMA, un'impronta per INTENTO ([D-301]: l'area su sole celle vuote
	 * ha un'impronta e zero colpi), un muro per BORDO — e non chiede mai a un colpo se sia un muro: e' la logica
	 * nella presentazione che [D-278] vieta, e la ragione per cui i muri avevano un array proprio.
	 * ⚠️ **Non si azzera in `BeginPlayback`**: estendendo (D-355) e' il `Previous` della ricostruzione. La
	 * svuota `FinishPlayback`.
	 */
	TArray<FRTBlastSequenceElement> PlaybackBlastSequence;

	/**
	 * Chi ha gia' ricevuto l'annuncio di morte in questo playback, per `StableUnitId`.
	 *
	 * 🔴 **Esiste perche' `IsHidden()` non puo' piu' fare da guardia** (#2452). Fino al 2026-09-05
	 * l'idempotenza dell'annuncio veniva dall'hide stesso: si nascondeva l'unita' e la si riconosceva
	 * «gia' mostrata» perche' era nascosta. Ma nascondere PRIMA di `PlayDefeatMontage()` rendeva il
	 * montaggio `Death` invisibile, quindi l'hide si e' spostato a `FinishPlayback` — e senza un
	 * marcatore proprio il catch-all rifarebbe partire il montaggio e ribroadcasterebbe
	 * `OnUnitDefeated` sulla stessa unita'.
	 *
	 * ⚠️ **Non se ne itera mai l'ordine** (solo `Contains`/`Add`): l'invariante «niente dipendenza
	 * dall'ordine di un container hash» resta intatta.
	 */
	TSet<int32> PlaybackDefeatShown;

	/**
	 * Secondi che restano alla coda finale della morte, `0` quando non e' in corso (#2452).
	 *
	 * Quando l'ultima fase si chiude su un'eliminazione il playback NON finisce: entra qui, e
	 * `FinishPlayback` arriva allo scadere. ⚠️ `SkipPlayback` la scavalca — chiama `FinishPlayback`
	 * direttamente — ed e' voluto: chi salta la risoluzione non vuole aspettare una coda.
	 */
	float PlaybackDefeatBeatRemaining = 0.f;

	TArray<ERTMatchPhase> PlaybackPhases;   // fasi attive, in ordine
	int32 PlaybackPhaseIdx = 0;
	float PlaybackPhaseElapsed = 0.f;
	float PlaybackSlackScale = 1.f;         // quanto il budget comprime le ATTESE (1 = nessuna, 0 = tutto)
	float PlaybackTotalSeconds = 0.f;       // durata stimata (per la progress bar)
	float PlaybackElapsedTotal = 0.f;
	/**
	 * L'orologio del FUMETTO della skill (#4025, spec del fumetto R4/R7): tempo del playback, lo stesso `Dt` e la stessa
	 * correzione dello `Step` di `PlaybackElapsedTotal` in `TickPlayback`, quindi la pausa e il trattenimento di una
	 * finestra lo fermano come fermano l'altro.
	 *
	 * 🔴 **Esiste perche' `PlaybackElapsedTotal` non sopravvive a una finestra di reazione.** `ConcludeResolution` lo
	 * azzera anche quando ci si arriva dalla RIPRESA dopo una finestra (`bIsResolving` vero), e `BeginPlayback(true)`
	 * «conserva» un orologio gia' azzerato. I fumetti accesi prima della finestra hanno uno `StartAt` su quel tempo:
	 * con il totale ripartito da zero `Now - StartAt` diventava negativo, e ogni fumetto del turno — anche quelli gia'
	 * dissolti — tornava pieno e ingrandito. Cambiare l'azzeramento condiviso toccherebbe la barra e `Pacing.Close`
	 * (TurnLog): fuori dalla fetta, quindi il fumetto ha un orologio suo.
	 *
	 * ⚠️ **Si azzera in due punti soltanto**: all'inizio del playback di un turno nuovo (`BeginPlayback(false)`) e alla
	 * fine VERA del playback (`FinishPlayback`, dopo il ramo sospeso), dove si spengono i fumetti. Mai in
	 * `ConcludeResolution`. Non e' una `UPROPERTY`: solo presentazione, fuori da snapshot, TurnLog e hash (R10).
	 */
	float SkillCalloutClock = 0.f;
	int32 ActivationsShown = 0;             // attivazioni gia' rivelate nella fase corrente (Prep o Dash), #3549
	/**
	 * Le unita' che hanno un fumetto di skill acceso in questo turno (#4025): chi lo ha mostrato (`ShowActivation`),
	 * a cui `TickPlayback` porta il tempo e che `FinishPlayback` spegne. Puntatori DEBOLI: un'unita' distrutta a meta'
	 * turno non deve tenere il manager legato a un oggetto morto, e `SyncSkillCalloutClocks` la scarta.
	 * ⚠️ **Sopravvive a `BeginPlayback` con `bPreserveClock`** (D-355: il playback riparte dopo una finestra di
	 * reazione senza che il turno sia finito), quindi non lo si azzera li': solo `FinishPlayback`, sul ramo che
	 * chiude il turno. Solo presentazione: fuori da snapshot, TurnLog e hash.
	 */
	TArray<TWeakObjectPtr<ARTUnit>> SkillCalloutUnits;
	/**
	 * Le righe del Registro delle attivazioni mostrate in questo turno (#4033): vedi `GetActivationLines`. Le scrive soltanto
	 * `AddActivationLine` — da `ShowActivation` e dal recupero di fine playback (R8) — e le azzera soltanto `LockInAndResolve`,
	 * accanto a `TurnLog.Reset()` (R7). ⛔ Non `BeginPlayback`, che riparte anche dopo una finestra di reazione, non
	 * `ConcludeResolution` e non `FinishPlayback`. Non e' una `UPROPERTY`: solo presentazione, fuori da snapshot e hash.
	 */
	TArray<FRTActivationLine> ActivationLines;
	/**
	 * Battiti gia' eseguiti nel Blast corrente, sulla SEQUENZA per intento (#3549 D5 con `#2454`): il `2k` RIVELA
	 * l'elemento `k` — per un colpo e' il LANCIO — e il `2k+1` e' il suo ARRIVO, che conta solo per un colpo.
	 * ⛔ Un cursore solo: due contatori separati, in un tick lungo, rivelerebbero l'elemento `k+1` prima dell'arrivo
	 * di `k`. `BlastElementsShown()` ne deriva il prefisso congelato di D-355. Si azzera in `EnterPlaybackPhase` e
	 * in `FinishPlayback`, MAI in `BeginPlayback` (l'estensione con `bPreserveClock` salta `EnterPlaybackPhase`).
	 * ⏱️ *Fino al merge di #2454 in #3549 erano due: `BlastShown` sulla sequenza e `AttackBeatsDone` sui soli colpi.*
	 */
	int32 BlastBeatsDone = 0;
	/**
	 * `true` se l'ultima consegna a `ARTHexMapActor::SetPlaybackTracers` non era vuota (`#2454`): permette a
	 * `PushPlaybackTracers` di non toccare la mappa a ogni tick quando non c'e' nulla in volo e nulla da spegnere.
	 * ⛔ Si azzera ESATTAMENTE dove il canale si spegne (finalizzazione del `Blast` e `FinishPlayback`).
	 */
	bool bPlaybackTracerChannelFull = false;
	/** #3578: come `bPlaybackTracerChannelFull`, per `SetPlaybackCues`. Si azzera dove il canale si spegne. */
	bool bPlaybackCueChannelFull = false;
	/**
	 * Il volo di ogni elemento, parallelo a `PlaybackBlastSequence` (`URTPlaybackLibrary::TracerFlightFor`): zero per
	 * ogni elemento che non e' un colpo idoneo, quindi il suo arrivo coincide con la rivelazione.
	 */
	TArray<float> PlaybackBlastFlights;
	/**
	 * #3578: per ogni elemento di `PlaybackBlastSequence`, l'indice di timeline dell'impronta che consuma (o
	 * `INDEX_NONE`), da `URTPlaybackLibrary::FootprintFxForSequence`. Parallelo alla sequenza come i voli, e come loro si
	 * ricalcola anche estendendo: funzione pura degli eventi.
	 */
	TArray<int32> PlaybackBlastFootprintFx;
	/**
	 * La squadra di chi guarda, fissata in `BeginPlayback`: decide il DISEGNO del tracer E, tramite le attivazioni che
	 * ha il diritto di vedere, l'indice di un colpo nella sequenza di Blast — quindi il suo istante. Il ritmo e' lo
	 * stesso solo fra squadre con la stessa conoscenza (`CONTRACT CONFLICT` risolto dalla spec del momento §2.4).
	 */
	int32 PlaybackViewerTeamId = 0;
	/** Traccia dei battiti per i test (`bRecordAttackBeatsForTest`): `L<i>`/`A<i>`, `i` = ordine fra i soli colpi. */
	TArray<FString> AttackBeatTrace;

	/**
	 * Il predicato di pausa una tantum armato da `RequestPlaybackStopAt` (`#2855`), o `None`.
	 *
	 * ⚠️ **Vive accanto a `bPlaybackPaused` e non dentro**: la pausa e' uno STATO — ci si e' fermati — e
	 * questo e' un'INTENZIONE — ci si fermera'. Fonderli renderebbe impossibile distinguere «fermo perche'
	 * qualcuno ha premuto Pause» da «in corsa verso un confine», che e' proprio cio' che chi guarda deve
	 * poter leggere sulla riga di stato.
	 */
	ERTPlaybackStopAt PlaybackStopAt = ERTPlaybackStopAt::None;

	/**
	 * L'azione in corso quando `NextAction` e' stato armato: il termine di paragone del confine.
	 *
	 * 🔑 **Congelata all'ARMAMENTO e non riletta a ogni tick.** E' la stessa regola di `StepMicroStep`, che
	 * calcola il proprio confine *«alla pressione e in secondi, non in tick»*: un paragone ricalcolato
	 * mentre il playback scorre inseguirebbe il proprio bersaglio e non si fermerebbe mai.
	 *
	 * ⚠️ `NAME_None` significa «nessun atto in corso», ed e' il valore giusto quando si arma prima che un
	 * colpo sia stato mostrato: il primo atto che passa e' allora gia' un confine — la stessa semantica
	 * che `URTPlaybackLibrary::NextActionBoundary` da' a un indice negativo.
	 */
	FName PlaybackStopFromAction;

	/**
	 * La sorgente dell'atto in corso all'armamento: con `PlaybackStopFromAction` e' la COPPIA del confine (#3549).
	 * ⚠️ `0` = nessuna sorgente: e' la sentinella dello STATO, diversa dal default `-1` di
	 * `URTPlaybackLibrary::IsActBoundary`, che significa «sorgente non dichiarata» e riporta il criterio storico.
	 */
	int32 PlaybackStopFromSource = 0;

	/**
	 * L'azione dell'ultimo fatto MOSTRATO, da qualunque fase e da qualunque tipo — `#3292`, `#3549`.
	 *
	 * 🔴 **Esiste perche' l'atto in corso si leggeva da un canale solo, ed era la terza faccia dello stesso
	 * difetto.** `RequestPlaybackStopAt` congelava `PlaybackAttacks[AttacksShown - 1]`: ∴ dopo essersi
	 * fermati su un'IMPRONTA, il paragone tornava all'ultimo colpo — un'azione **precedente** — e il colpo
	 * dello stesso intento che seguiva sembrava aprire un atto nuovo. Due fermate dentro un intento solo,
	 * che e' precisamente cio' che `#3292` esclude.
	 *
	 * ⚠️ **Lo aggiorna `NotePlaybackActShown` quando si mostra un fatto con un'azione** — attivazioni di Prep e
	 * Dash, elementi della sequenza di Blast (#3549; ⏱️ *prima i tre canali del Blast*) — e **solo** allora: un
	 * `NAME_None` non cambia l'atto in corso. E' la stessa scelta della scansione all'indietro di
	 * `NextActionBoundary`, che salta i vuoti invece di lasciarsene azzerare.
	 *
	 * ⛔ **Non e' `PlaybackStopFromAction`**, benche' si somiglino: quella e' congelata all'armamento e non
	 * si muove piu'; questa segue la riproduzione. Fonderle riporterebbe il paragone a inseguire il proprio
	 * bersaglio, che e' il difetto che il congelamento esiste per evitare.
	 */
	FName PlaybackLastShownAction;

	/**
	 * La sorgente dell'atto MOSTRATO in corso: con `PlaybackLastShownAction` e' l'atto in corso (#3549).
	 * ⚠️ `0` = nessuna sorgente, sentinella dello stato, diversa dal default `-1` della funzione (`IsActBoundary`).
	 * 🔑 Un fatto con sorgente `0` (non attribuibile, [D-063]) non la sovrascrive; un cambio d'azione la riparte
	 * (review della PR #3561, `NotePlaybackActShown`).
	 */
	int32 PlaybackLastShownSource = 0;

	// Trasformazione griglia in cache per convertire celle->mondo durante il playback.
	FVector PBOrigin = FVector::ZeroVector;
	float PBCellSize = 200.f;
	float PBLayerHeight = 0.f;
};
