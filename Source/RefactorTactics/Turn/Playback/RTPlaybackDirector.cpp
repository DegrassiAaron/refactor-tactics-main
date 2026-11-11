#include "Turn/Playback/RTPlaybackDirector.h"
#include "Turn/Playback/RTPlaybackLibrary.h"
#include "Unit/RTGraykitLibrary.h"      // #2880: la posa si valuta sullo stesso Alpha del movimento
#include "Turn/Playback/RTPresentationBinding.h" // #2881: l'andatura la decide l'owner di [D-278], non questo file
#include "Perception/RTTeamKnowledge.h" // ObservedPrefixLength: la regola di troncamento di [D-223], una sola volta
#include "Player/RTPlayerState.h"       // TeamIdOf: l'unica porta per sapere di che squadra e' chi guarda ([D-285])
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "UI/HUD/RTHudViewModel.h"          // #4025: `ActionShouts` decide chi grida; il testo della riga del Registro
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Unit/RTUnit.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h" // ON_SCOPE_EXIT: oggi arriva dalla PCH condivisa, ma il file non deve dipenderne

/*
 * I corpi qui sotto sono quelli che vivevano in `RTTurnManager.cpp`, spostati con le loro note (#4057). Cambia solo il
 * modo di raggiungere cio' che resta all'orchestratore:
 *
 *   - le manopole si leggono da `Tuning()`, la timeline da `Timeline()`;
 *   - i delegate si trasmettono da `Signals`;
 *   - mondo, unita', log, tick, `IsResolving` e chiusura del turno passano da `Host`.
 *
 * ⛔ Se un corpo qui dentro ha bisogno d'altro, la strada e' `FRTPlaybackHost`: e' l'unico posto in cui il confine e'
 * scritto, e la ragione di ogni sua voce sta accanto alla voce.
 */

void FRTPlaybackDirector::Bind(FRTPlaybackHost& InHost, const FRTPlaybackSignals& InSignals)
{
	Host = &InHost;
	Signals = InSignals;
}

void FRTPlaybackDirector::RefreshPresentationGeometry(float PreviousLayerHeight)
{
	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerH = 0.f;
	Host->GetHexContext(Origin, HexSize, LayerH);
	if (PreviousLayerHeight <= UE_KINDA_SMALL_NUMBER || FMath::IsNearlyEqual(PreviousLayerHeight, LayerH))
	{
		return;
	}

	// La cache del playback la leggono le animazioni e `FinishPlayback`, che a fine turno posa OGNI unita' sulla sua
	// cella: si aggiorna sempre, anche in un playback senza movimenti. Fuori dal playback la riscrive il prossimo.
	PBOrigin = Origin;
	PBCellSize = HexSize;
	PBLayerHeight = LayerH;

	// Le animazioni conservano le CELLE del percorso: le posizioni si ricalcolano da li', e il fotogramma dopo
	// `InterpolateAlongPath` le segue.
	for (FRTMoveAnim& A : MoveAnims)
	{
		const ARTUnit* Animata = A.Unit.Get();
		if (!Animata)
		{
			continue;
		}
		for (int32 I = 0; I < A.Cells.Num() && I < A.World.Num(); ++I)
		{
			A.World[I] = Animata->WorldForCell(A.Cells[I], Origin, HexSize, LayerH);
		}
	}

	// 🔴 **Ogni modello SALE o SCENDE, e basta: la pianta non si tocca.** Non si riposa su `Cell`: durante il
	// playback `Cell` e' gia' la cella FINALE, mentre il modello sta dove il playback lo ha messo — all'inizio di
	// un'animazione di una fase futura, alla fine di una passata, a meta' di quella in corso. Vale per tutti, anche
	// per i caduti ancora a schermo.
	//
	// La quota sopra il piano terra e' LINEARE nella quota dei piani (`AxialToWorld`: `Layer * LayerHeight`), e lo
	// resta lungo un'interpolazione fra due celle: scalarla e' esatto per un modello fermo, a meta' salita, in pausa
	// o in step. Arrotondarla a un piano sbagliava proprio quest'ultimo caso (seconda review di #3632).
	// `SetActorLocation` e non `PlaceOnCell`: nessuna cella cambia, e nessuna porta che la scriva va attraversata.
	const float Scala = LayerH / PreviousLayerHeight;
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(Host->GetPlaybackWorld(), ARTUnit::StaticClass(), Actors);
	for (AActor* A : Actors)
	{
		ARTUnit* U = Cast<ARTUnit>(A);
		if (!U)
		{
			continue;
		}
		const FVector Qui = U->GetActorLocation();
		const double Terra = Origin.Z + U->VisualZOffset;
		U->SetActorLocation(FVector(Qui.X, Qui.Y, Terra + (Qui.Z - Terra) * Scala));
	}
}

bool FRTPlaybackDirector::AnimatedCellFor(const ARTUnit* Unit, FRTCellId& OutCell) const
{
	if (!Unit || !PlaybackPhases.IsValidIndex(PlaybackPhaseIdx))
	{
		return false;
	}
	const ERTMatchPhase Ph = PlaybackPhases[PlaybackPhaseIdx];

	for (int32 A = 0; A < MoveAnims.Num(); ++A)
	{
		const FRTMoveAnim& Anim = MoveAnims[A];
		if (Anim.Phase != Ph || Anim.Unit.Get() != Unit || Anim.Cells.Num() == 0)
		{
			continue;
		}
		const int32 Idx = PlaybackAnimCellIndex.IsValidIndex(A) ? PlaybackAnimCellIndex[A] : 0;
		OutCell = Anim.Cells[FMath::Clamp(Idx, 0, Anim.Cells.Num() - 1)];
		return true;
	}
	return false;
}

void FRTPlaybackDirector::BeginPlayback(bool bPreserveClock)
{
	// 🔑 **`bPreserveClock` distingue ESTENDERE da RICOMINCIARE** (`#2679` fetta 3, [D-355]). Quando la
	// resolution si sospende su una finestra il playback parte su una timeline parziale; alla ripresa la
	// timeline si completa e questa funzione va rieseguita — ma azzerare l'orologio farebbe ripartire il
	// movimento da capo sotto gli occhi di chi ha appena deciso.
	// Estendendo, cio' che teneva fermo il playback non c'e' piu': la timeline e' cresciuta.
	bPlaybackHeldByWindow = false;

	// `#2858`: la politica *start paused*, applicata dove il playback comincia a scorrere.
	//
	// 🔑 **Solo quando RICOMINCIA, mai quando ESTENDE.** Con `bPreserveClock` questa funzione riprende un
	// playback gia' in corso dopo una finestra di reazione: rimetterlo in pausa li' fermerebbe l'immagine
	// **subito dopo** che chi guardava ha deciso, cioe' nel punto in cui si aspetta di vedere l'esito della
	// propria scelta. La politica riguarda l'INIZIO di un playback, e quello e' l'altro ramo.
	//
	// ⛔ La subordinazione ai controlli e' gia' in `SetStartPlaybackPaused`, che e' l'unico modo di
	// accendere questo flag: qui non si ricontrolla, o sarebbero due guardie da tenere d'accordo.
	if (!bPreserveClock && bStartPlaybackPaused)
	{
		bPlaybackPaused = true;
		PlaybackStepTargetElapsed = -1.f;
	}

	const float ClockElapsedTotal = PlaybackElapsedTotal;
	const int32 ClockPhaseIdx = PlaybackPhaseIdx;
	const float ClockPhaseElapsed = PlaybackPhaseElapsed;

	// 🔑 **`#3666`: estendendo si riprende la fase che si stava MOSTRANDO, cercata per nome.** Al trattenimento
	// (`FinishPlayback`) l'indice e' gia' oltre l'ultima fase parziale e l'orologio di fase vale ancora la sua
	// fine: «la fase dell'orologio» e' allora l'ultima mostrata. ⛔ Il clamp sull'indice, che c'era prima, la
	// sbagliava quando la timeline completa ha una fase IN PIU' — il Move dopo il Blast di un `Brace`: l'indice
	// vecchio puntava a quella fase nuova, che partiva con l'orologio del Blast e senza `EnterPlaybackPhase`.
	// Da `#3666` si risponde sempre a playback trattenuto, quindi non e' piu' un caso raro.
	const bool bFaseDelClockNota = bPreserveClock && PlaybackPhases.Num() > 0;
	const ERTMatchPhase FaseDelClock = PlaybackPhases.IsValidIndex(ClockPhaseIdx)
		? PlaybackPhases[ClockPhaseIdx]
		: (PlaybackPhases.Num() > 0 ? PlaybackPhases.Last() : ERTMatchPhase::Planning);
	// Chi aveva gia' un'anim nella fase dell'orologio la CONTINUA (la rotta del mover dell'Overwatch si allunga);
	// chi non l'aveva ne riceve una NUOVA, che comincia adesso (lo scarto del `Brace`).
	TSet<const ARTUnit*> AnimatiNellaFaseDelClock;
	// ⚠️ E chi la continua conserva il proprio inizio, se un'estensione PRECEDENTE gliel'aveva dato (review di #3675):
	// la ricostruzione qui sotto parte da zero, e senza portarlo avanti l'anim tornerebbe sull'orologio della fase e
	// salterebbe. Succede a un'anim nata in un'estensione e ancora in corsa alla successiva.
	// 🛡️ **Difensiva, e nessun test la fissa.** Nel Blast gli scarti si applicano dopo il ciclo sui bersagli
	// (`KTargets`), quindi nascono tutti nell'ultima estensione; nel Move servirebbe una rotta che diventa visibile fra
	// due finestre. Nessun turno costruibile oggi nei test ci arriva.
	TMap<const ARTUnit*, float> InizioNellaFaseDelClock;
	if (bFaseDelClockNota)
	{
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.Phase == FaseDelClock && A.Unit.IsValid())
			{
				AnimatiNellaFaseDelClock.Add(A.Unit.Get());
				if (A.StartElapsed > 0.f) { InizioNellaFaseDelClock.Add(A.Unit.Get(), A.StartElapsed); }
			}
		}
	}

	// Cache della trasformazione della mappa per convertire celle -> mondo durante il playback.
	// Stessa fonte usata da ResolveMovement: risoluzione e playback non possono divergere di scala.
	Host->GetHexContext(PBOrigin, PBCellSize, PBLayerHeight);

	// Deriva le animazioni di movimento e la lista attacchi dagli eventi risolti.
	MoveAnims.Reset();
	// Gli indici di cella seguono le anim a cui si riferiscono (`#2876`): sopravvivergli farebbe
	// leggere il confine di un'altra unita'.
	PlaybackAnimCellIndex.Reset();
	PlaybackAnimEntryYaw.Reset();
	PlaybackAnimArrived.Reset();
	PlaybackDefeated.Reset();
	PlaybackActivationsPrep.Reset();
	PlaybackActivationsDash.Reset();
	// `PlaybackBlastSequence` NON si azzera qui: estendendo (D-355) e' il `Previous` della ricostruzione.
	// L'annuncio e' per playback: il marcatore non sopravvive al round. ➕ **Ma estendere non e' un playback
	// nuovo** (`#3666`): la fase ripresa si richiude, e senza il marcatore una morte gia' annunciata alla sua
	// prima chiusura — prima del trattenimento — ripartirebbe col suo `Death` e un secondo `OnUnitDefeated`.
	if (!bPreserveClock)
	{
		PlaybackDefeatShown.Reset();
	}
	PlaybackDefeatBeatRemaining = 0.f; // e nemmeno la coda: `SkipPlayback` passa di qui e la scavalca
	// 🔴 **La squadra di chi GUARDA, e il playback si tronca su di essa** (`#1525`, [D-223]). Stessa porta
	// di `ARTHUD::DrawHUD` e del velo: una sola risposta a «di che squadra e' questo giocatore» ([D-285]).
	//
	// ⚠️ **Limite dichiarato, ed e' lo stesso del velo**: qui il viewer e' il giocatore LOCALE, perche' in
	// v0.1 il TurnManager e il client sono lo stesso processo. Questo troncamento e' presentazione, non un
	// confine di sicurezza: quando la partita sara' in rete, il dato non autorizzato non dovra' essere
	// replicato — non filtrato all'arrivo. Vedi `conoscenza-parziale-visibile-spec.md` §1.3.
	const int32 ViewerTeamId = ARTPlayerState::TeamIdOf(UGameplayStatics::GetPlayerController(Host->GetPlaybackWorld(), 0));
	PlaybackViewerTeamId = ViewerTeamId;

	TSet<ARTUnit*> StartPositioned; // per posizionare il cilindro all'inizio della sua PRIMA fase (Dash prima di Move)
	for (const FRTResolvedEvent& Ev : Timeline())
	{
		// L'UNICO punto in cui l'id torna a essere un Actor, ed e' qui perche' qui si comincia ad animare
		// (#1800). `nullptr` significa «non c'e' piu' nessuno da muovere» e cade nei rami che gia'
		// esistevano per `TWeakObjectPtr` scaduto.
		ARTUnit* const Src = Host->UnitByStableId(Ev.SourceStableUnitId);

		if (Ev.Type == ERTResolvedEventType::Move && Src && Ev.Path.Num() >= 2)
		{
			// Quante celle iniziali l'osservatore ha il diritto di vedere percorrere. La regola e' quella
			// della traccia, e la funzione e' LA STESSA: `VisibleTrailFor` chiama questa.
			const int32 Visible = URTTeamKnowledgeLibrary::ObservedPrefixLength(
				Ev.Path, Ev.CellVerdicts, ViewerTeamId);

			// 🔴 **Meno di due celle osservate: nessuna animazione, e soprattutto NESSUN posizionamento
			// sullo start.** Il `SetVisualLocation` qui sotto e' esso stesso un canale: piazzare il modello
			// sulla cella di PARTENZA di un movimento che l'osservatore non ha visto cominciare rivelerebbe
			// da dove il nemico e' uscito — e lo farebbe proprio nel caso in cui la sua destinazione e'
			// visibile, cioe' quando `bKnownToObserver` lo lascia acceso. Saltando, il modello resta dove
			// il frame precedente lo aveva lasciato e `FinishPlayback` lo porta alla sua cella logica.
			if (Visible < 2)
			{
				continue;
			}

			FRTMoveAnim Anim;
			Anim.Unit = Src;
			Anim.Phase = Ev.Phase; // Dash o Move
			Anim.SourceStatusNames = Ev.SourceStatusNames; // `#3117`: copiati, non riletti dall'unita'
			// `#3263`: il confine fra i passi VOLUTI e quelli che il terreno ha imposto, troncato al
			// prefisso osservato come `World` e `Cells`. ⛔ Copiarlo intero direbbe a chi disegna che il
			// piano arrivava oltre cio' che si vede — il tratto che [D-223] nasconde.
			Anim.PlannedLength = FMath::Clamp(Ev.PlannedLength, 0, Visible);
			Anim.World.Reserve(Visible);
			Anim.Cells.Reserve(Visible);
			for (int32 i = 0; i < Visible; ++i)
			{
				Anim.World.Add(Src->WorldForCell(Ev.Path[i], PBOrigin, PBCellSize, PBLayerHeight));
				// La stessa cella, dallo STESSO indice troncato: chi legge le pose durante il playback
				// (`#2876`) guarda cio' che l'anim disegna, mai la rotta piena.
				Anim.Cells.Add(Ev.Path[i]);
			}
			// Metti il cilindro all'inizio della sua PRIMA anim (Dash precede Move nella timeline):
			// niente flash sulla cella finale. Un'anim successiva della stessa unita' non ne sposta lo start.
			// ⛔ **Non si riposiziona quando si estende**: il cilindro e' gia' dove il playback lo ha
			// portato, e riportarlo all'inizio della propria anim lo farebbe saltare indietro.
			if (!bPreserveClock && !StartPositioned.Contains(Src))
			{
				Src->SetVisualLocation(Anim.World[0]);
				StartPositioned.Add(Src);
			}
			// `#3666`: estendendo, chi non e' animato nella fase dell'orologio si posa sulla partenza della sua prima
			// anim ancora da mostrare — quella NUOVA nella fase dell'orologio, che comincia adesso, o una di una fase
			// dopo. La resolution ha gia' portato l'Actor sulla cella logica: senza, lo si vedrebbe arrivato fino a
			// quando la sua anim comincia, e poi tornare indietro di colpo (misurato: il Move dopo il `Brace`).
			// ⛔ Chi e' animato nella fase dell'orologio no: e' gia' dove il playback lo ha portato.
			if (const float* Inizio = (bFaseDelClockNota && Anim.Phase == FaseDelClock)
				? InizioNellaFaseDelClock.Find(Src) : nullptr)
			{
				Anim.StartElapsed = *Inizio;
			}
			if (bFaseDelClockNota && !AnimatiNellaFaseDelClock.Contains(Src))
			{
				const bool bNuovaNellaFaseDelClock = (Anim.Phase == FaseDelClock);
				if (bNuovaNellaFaseDelClock)
				{
					Anim.StartElapsed = ClockPhaseElapsed;
				}
				// L'enum segue l'ordine canonico delle fasi (`Prep -> Dash -> Blast -> Move`): «dopo» e' un confronto.
				if ((bNuovaNellaFaseDelClock || Anim.Phase > FaseDelClock) && !StartPositioned.Contains(Src))
				{
					Src->SetVisualLocation(Anim.World[0]);
					StartPositioned.Add(Src);
				}
			}
			MoveAnims.Add(MoveTemp(Anim));
		}
		else if (Ev.Type == ERTResolvedEventType::Defeated)
		{
			PlaybackDefeated.Add(Ev);
		}
		else if (Ev.Type == ERTResolvedEventType::AbilityActivated)
		{
			// 🔴 D6, spec §2.5: il predicato e' quello delle RIGHE DI LOG, non quello del `Move` qui sopra. Un
			// `ObservedPrefixLength` su un'attivazione — nessuna rotta, nessuna cella — leggerebbe il vuoto come
			// fail-closed e nasconderebbe ogni attivazione, comprese quelle di chi guarda.
			// ⚠️ Il Blast non passa di qui: lo smista `BuildBlastSequence`, con lo stesso predicato.
			if (Ev.SourceVerdict.AllowsTeam(ViewerTeamId))
			{
				if (Ev.Phase == ERTMatchPhase::Prep) { PlaybackActivationsPrep.Add(Ev); }
				else if (Ev.Phase == ERTMatchPhase::Dash) { PlaybackActivationsDash.Add(Ev); }
			}
		}
		// ⏱️ *Fino a #3549 qui c'erano anche i rami `Attack`, `AttackFootprint` e `StructureHit`, uno per canale
		// parallelo del Blast.* Ora quei fatti li smista `BuildBlastSequence` qui sotto, e cio' che i loro commenti
		// avevano di ancora vero sta in testa a `AdvanceBlastSequence`.
	}

	// La sequenza per intento del Blast (D5). 🔑 **Estendendo (D-355) il prefisso gia' mostrato e' stabile**:
	// i primi `BlastElementsShown()` elementi si riproducono verbatim e non si ripetono. Il resto si ricostruisce per
	// gruppo (Ruling H): un evento nuovo si unisce al SUO gruppo `(sorgente, azione)`, e la chiave del gruppo si
	// legge dalla sua attivazione ovunque stia, prefisso compreso — quindi un evento tardivo di un atto aperto
	// presto cade fra i primi oltre il prefisso, non in fondo. Precondizione: la timeline cresce solo per
	// accodamento.
	// ⚠️ `BlastElementsShown()` e' il prefisso giusto perche' `EnterPlaybackPhase`, che ne azzera il cursore a ogni fase, non passa di
	// qui quando si estende. Vale `0` se il playback non e' ancora entrato nel Blast (prefisso vuoto); vale la
	// lunghezza della sequenza parziale se il playback parziale ha finito il Blast ed e' fermo sulla finestra —
	// tutto mostrato, ed e' un prefisso valido; vale `0` anche quando il Blast e' gia' passato e si e' entrati
	// in una fase successiva, dove la sequenza non si riproduce piu'.
	// ⚠️ Il prefisso e' `BlastElementsShown()`, derivato dal cursore dei battiti (`#2454`): un colpo LANCIATO e non
	// ancora arrivato ne fa parte, quindi resta al suo posto e il suo arrivo, ancora dovuto, cade sullo stesso indice.
	PlaybackBlastSequence = URTPlaybackLibrary::BuildBlastSequence(Timeline(),
		bPreserveClock ? PlaybackBlastSequence : TArray<FRTBlastSequenceElement>(),
		bPreserveClock ? BlastElementsShown() : 0, ViewerTeamId);

	// `#2454`: il volo di ogni elemento, deciso dall'idoneita' e non dal disegno che chi guarda vedrebbe.
	// ⚠️ **Il ritmo NON e' lo stesso per ogni squadra** (`CONTRACT CONFLICT` con la spec del tracer §2.1, che governa
	// il tracer ma non questo punto): chi guarda decide il disegno E, attraverso le attivazioni che ha il diritto di
	// vedere (`BuildBlastSequence`, D6), l'INDICE del colpo nella sequenza — quindi l'istante di lancio e di arrivo. Il
	// ritmo coincide solo fra squadre con la stessa conoscenza. Un'attivazione nascosta non lascia un buco, o ne
	// rivelerebbe l'esistenza col tempo (spec del momento §2.4; `Playback.BlastSequenceIndexDependsOnTheViewer`).
	// `IsTracerEligible` e' falso per ogni tipo che non sia `Attack`: attivazioni, impronte e muri hanno volo zero.
	// Si ricalcola anche estendendo: e' funzione pura degli eventi, e il prefisso congelato porta gli stessi eventi,
	// quindi gli stessi voli.
	// #3578: l'idoneita' e' la forma di DEFAULT di un'azione con un id (R12, R13, `IsTracerEligible`): un override che
	// toglie il disegno (`Ram`, `PassingBlade`) non toglie il volo, e un colpo non base con forma `Single`/`Line` ora vola.
	int32 NumColpi = 0, NumImpronte = 0, NumMuri = 0, NumAttivazioniBlast = 0, NumCure = 0;
	PlaybackBlastFlights.Reset(PlaybackBlastSequence.Num());
	for (const FRTBlastSequenceElement& E : PlaybackBlastSequence)
	{
		const FRTResolvedEvent& Ev = Timeline()[E.TimelineIndex];
		switch (Ev.Type)
		{
		case ERTResolvedEventType::Attack:           ++NumColpi; break;
		case ERTResolvedEventType::AttackFootprint:  ++NumImpronte; break;
		case ERTResolvedEventType::StructureHit:     ++NumMuri; break;
		case ERTResolvedEventType::AbilityActivated: ++NumAttivazioniBlast; break;
		case ERTResolvedEventType::Healed:           ++NumCure; break;
		default: break;
		}
		PlaybackBlastFlights.Add(URTPlaybackLibrary::TracerFlightFor(
			URTPlaybackLibrary::IsTracerEligible(Ev), Tuning().TracerFlightSeconds, Tuning().AttackShowSeconds));
	}
	// #3578 (spec «il profilo FX» §2.4): quale impronta porta ogni colpo. ⚠️ Si ricostruisce sull'INTERA sequenza
	// anche estendendo: un'impronta consumata nel prefisso congelato resta consumata.
	PlaybackBlastFootprintFx = URTPlaybackLibrary::FootprintFxForSequence(Timeline(), PlaybackBlastSequence);

	// Fasi attive, in ordine canonico (Prep -> Dash -> Blast -> Move). Cleanup: gia' applicato, nessun beat.
	bool bHasDash = false, bHasMove = false, bHasBlastMove = false;
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Phase == ERTMatchPhase::Dash) { bHasDash = true; }
		else if (A.Phase == ERTMatchPhase::Blast) { bHasBlastMove = true; } // spinta (knockback)
		else { bHasMove = true; }
	}
	PlaybackPhases.Reset();
	// #3549, C3: una fase NASCE anche dalle sole attivazioni visibili — un Blast di cure, una Prep nemica nota,
	// una carica su un bersaglio gia' adiacente (nessuna cella da percorrere, ma il gesto c'e').
	if (Host->IsPrepActiveThisTurn() || PlaybackActivationsPrep.Num() > 0) { PlaybackPhases.Add(ERTMatchPhase::Prep); }
	if (bHasDash || PlaybackActivationsDash.Num() > 0) { PlaybackPhases.Add(ERTMatchPhase::Dash); }
	// 🔴 **Le impronte contano quanto i colpi**, ed e' la riga che apre il caso «area su sole celle
	// vuote»: zero vittime -> zero `Attack` -> senza questo termine la fase non nasceva, e non esisteva
	// un istante in cui disegnare (`#2454`). Lo stesso vale per i muri (`#2828`) e, da #3549, per le
	// attivazioni. La decisione sta in una funzione pura perche' cambia la DURATA di un turno, ed e' cio'
	// che i test di pacing sorvegliano. I conteggi sono quelli della SEQUENZA, cioe' di cio' che chi guarda vedra'.
	if (URTPlaybackLibrary::BlastPhaseIsActive(NumColpi, bHasBlastMove, NumImpronte, NumMuri, NumAttivazioniBlast, NumCure))
	{
		PlaybackPhases.Add(ERTMatchPhase::Blast);
	}
	if (bHasMove) { PlaybackPhases.Add(ERTMatchPhase::Move); }

	if (PlaybackPhases.Num() == 0)
	{
		Host->ConcludeTurn(); // niente da mostrare
		return;
	}

	// I due termini del round, separati: quanto tempo si MOSTRA qualcosa e quanto si aspetta. La
	// distinzione nasce in `URTPlaybackLibrary::PhaseTime` e serve qui, dove il budget decide cosa
	// comprimere — il budget puo' togliere attese e non puo' toccare cio' che si vede (#1878).
	float ShownTotal = 0.f;
	float SlackTotal = 0.f;
	for (const ERTMatchPhase Ph : PlaybackPhases)
	{
		const FRTPhaseTime T = PhaseTimeForPlaybackPhase(Ph);
		ShownTotal += T.Shown;
		SlackTotal += T.Slack;
	}

	// La coda della morte entra nella STIMA, e in `Shown` e non in `Slack` (#2452 con #1878): mostra
	// qualcosa, quindi il budget non puo' toglierla. Si aggiunge SOLO se un'eliminazione cade nell'ultima
	// fase riprodotta — per tutte le altre la finestra del montaggio sono le fasi che seguono, gia' contate.
	//
	// ⚠️ Senza questa riga la barra di avanzamento direbbe una durata piu' corta di quella che si vede.
	if (Tuning().DefeatBeatSeconds > 0.f && PlaybackPhases.Num() > 0)
	{
		const ERTMatchPhase UltimaFase = PlaybackPhases.Last();
		const bool bMorteNellUltimaFase = PlaybackDefeated.ContainsByPredicate(
			[UltimaFase](const FRTResolvedEvent& D) { return D.Phase == UltimaFase; });
		if (bMorteNellUltimaFase)
		{
			ShownTotal += Tuning().DefeatBeatSeconds;
		}
	}
	// ⚠️ Da qui in poi `DurationForPlaybackPhase` restituisce la durata GIA' compressa: va calcolato prima
	// di qualunque lettura, o il primo tick userebbe la scala del round precedente.
	PlaybackSlackScale = URTPlaybackLibrary::SlackScaleForBudget(ShownTotal, SlackTotal, Tuning().MaxPlaybackSeconds);

	// Il totale che si mostrera': locomozione intatta piu' le attese sopravvissute al budget. ⚠️ Puo'
	// SFORARE `MaxPlaybackSeconds`, ed e' voluto: quando non resta comprimibile, la risoluzione dura di
	// piu' invece di far correre i personaggi.
	const float BudgetedTotal = ShownTotal + SlackTotal * PlaybackSlackScale;

	// La stima mostrata tiene conto anche della velocita' scelta da chi guarda. ⚠️ E' una stima ALL'AVVIO:
	// se la velocita' cambia a risoluzione in corso, la barra resta tarata su quella di partenza. La
	// riproduzione invece segue subito (TickPlayback rilegge a ogni tick) — l'unica cosa che diverge e'
	// il numero mostrato, non il ritmo, e non c'e' nulla di logico che vi dipenda.
	// 🔴 **Nelle unita' dell'orologio del playback, non in secondi di parete** — corretto il 2026-09-02.
	// `PlaybackElapsedTotal` accumula `Dt`, che e' gia' moltiplicato per la velocita' scelta; dividere il
	// totale per la stessa velocita' rendeva `GetPlaybackProgress01()` il rapporto fra una grandezza
	// scalata e una no, e la barra saturava a `1` dopo `1/ViewerPlaybackSpeed` della risoluzione — a x4,
	// col 75% ancora da mostrare. Il difetto precede #1878; questa riga era gia' da riscrivere e lasciarlo
	// dentro sarebbe stato peggio che allargare di poco lo scope.
	PlaybackTotalSeconds = BudgetedTotal;
	PlaybackElapsedTotal = bPreserveClock ? ClockElapsedTotal : 0.f;
	// #4025: l'orologio del fumetto riparte solo con il playback di un turno NUOVO. Estendendo dopo una finestra non si
	// tocca: e' il punto della sua esistenza (vedi `SkillCalloutClock`), perche' `ClockElapsedTotal` qui sopra puo' gia'
	// essere stato azzerato da `ConcludeResolution`.
	if (!bPreserveClock)
	{
		SkillCalloutClock = 0.f;
	}

	// Estendendo si resta nella fase in corso; il clamp copre il caso in cui la timeline completa
	// abbia meno fasi di quella parziale — non dovrebbe accadere, e un indice fuori range in
	// `TickPlayback` sarebbe un crash invece di un difetto visibile.
	const int32 IdxDellaFaseDelClock = bFaseDelClockNota ? PlaybackPhases.IndexOfByKey(FaseDelClock) : INDEX_NONE;
	PlaybackPhaseIdx = !bPreserveClock ? 0
		: (IdxDellaFaseDelClock != INDEX_NONE)
			? IdxDellaFaseDelClock
			: FMath::Clamp(ClockPhaseIdx, 0, FMath::Max(0, PlaybackPhases.Num() - 1));
	Host->SetPlaybackResolving(true);
	Host->SetPlaybackTickEnabled(true);
	// I secondi di parete si compongono qui, dove servono a chi legge, invece di essere conservati in un
	// campo che ha un'altra unita' di misura.
	const float StartSpeed = URTPlaybackLibrary::EffectivePlaybackSpeed(Tuning().ViewerPlaybackSpeed);
	// ⚠️ **`slack x%.2f` non e' decorazione: e' il segnale che rende il budget osservabile dai log.** La
	// diagnosi di #1878 e' stata possibile perche' questa riga portava il moltiplicatore del tetto, e su
	// 125.780 risoluzioni si e' potuto dire che non era mai intervenuto. Quel moltiplicatore non esiste
	// piu'; senza il suo sostituto, il giorno in cui il budget comincera' a mordere — e comincera', appena
	// la velocita' base scendera' — nessuno potrebbe accorgersene misurando.
	Host->AddLogEvent(FString::Printf(TEXT("Risoluzione: %d fasi, ~%.1fs (x%.2f, slack x%.2f)"),
		PlaybackPhases.Num(), (StartSpeed > 0.f) ? (BudgetedTotal / StartSpeed) : BudgetedTotal,
		StartSpeed, PlaybackSlackScale), FRTLogSubject::World());

	if (bPreserveClock)
	{
		// `EnterPlaybackPhase` azzera l'orologio di fase e annuncia l'ingresso: estendendo non si entra in
		// nessuna fase nuova, si continua quella in corso. Se non ha piu' niente da mostrare, il prossimo tick la
		// chiude e la fase dopo entra da `EnterPlaybackPhase`, come in un playback senza finestre (`#3666`).
		PlaybackPhaseElapsed = ClockPhaseElapsed;
		// `#3666`: chi riceve un'anim NUOVA nella fase in corso parte adesso, e lo si annuncia come farebbe
		// `EnterPlaybackPhase` all'ingresso della fase.
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.StartElapsed > 0.f && A.Unit.IsValid() && PlaybackPhases.IsValidIndex(PlaybackPhaseIdx)
				&& A.Phase == PlaybackPhases[PlaybackPhaseIdx])
			{
				A.Unit->bIsMovingVisually = true;
				Signals.OnUnitMoveStarted->Broadcast(A.Unit.Get());
			}
		}
		return;
	}

	EnterPlaybackPhase();
}

void FRTPlaybackDirector::PausePlaybackAtActBoundary()
{
	PlaybackStopAt = ERTPlaybackStopAt::None;
	PlaybackStopFromAction = NAME_None;
	PlaybackStopFromSource = 0;
	bPlaybackPaused = true;
	PlaybackStepTargetElapsed = -1.f;
}

void FRTPlaybackDirector::ShowActivation(const FRTResolvedEvent& Ev)
{
	ARTUnit* const Src = Host->UnitByStableId(Ev.SourceStableUnitId);
	Host->AddLogEvent(FString::Printf(TEXT("Attiva: %s -> %s"), Src ? *Src->GetName() : TEXT("?"), *Ev.ActionId.ToString()),
		FRTLogSubject::Frozen(Ev.SourceStableUnitId, Ev.SourceVerdict));
	// #4033 (R1, R2): la RIGA del Registro nasce qui, per OGNI attivazione mostrata e prima del ramo di `ActionShouts`, che
	// decide il solo fumetto. Chi puo' vederla l'hanno gia' deciso `BeginPlayback` e `BuildBlastSequence`, come per il fumetto.
	AddActivationLine(Ev, Src);
	if (Src)
	{
		// L'azione arriva al beat (#3563, D3): la clip del profilo, poi della generica, poi del ruolo.
		Src->PlayPresentationRole(ERTPresentationRole::Cast, Ev.ActionId, Ev.BaseActionId);

		// #4025: il FUMETTO della skill di firma. Chi grida lo decide `ActionShouts` (base vuota, fuori dal catalogo
		// core e non concessa da un pezzo d'equipaggiamento, R1b/A7); se mostrarlo a chi guarda NON si decide qui — il
		// filtro di privacy sta a monte, in `BeginPlayback` e in `BuildBlastSequence`, che non fanno arrivare qui una
		// sorgente non osservata (R9), e il velo lo spegne in overlay (`ARTHUD::UpdateObserverVeil`).
		//
		// 🔎 **R11, misurato per lettura al commit `99d6a2580`; ancorato ai SIMBOLI, non alle righe**, che i merge di
		// `main` spostano. Il fumetto eredita la risposta dell'EVENTO `AbilityActivated` e non ne scrive una sua: nessuna
		// riga di codice qui dipende da R11.
		//
		// - **ATTACCHI**: un intento INTERROTTO non emette `AbilityActivated`, quindi non grida. `ResolveCombatPasses`
		//   chiama `ApplyInterrupts` (in `RTTurnManager_Blast.cpp`), che mette l'intento cancellato in
		//   `Ctx.InterruptedIntents`; il ciclo di emissione della stessa `ResolveCombatPasses` viene DOPO, e
		//   `EmitAttackIntentActivation` esce su `Ctx.InterruptedIntents.Contains(k)`.
		//   ⚠️ Il DEGRADATO (`SuppressSecondary`) va in `Ctx.DegradedIntents`, non li': la sua azione e' avvenuta, monca,
		//   quindi si attiva e grida.
		// - **CURE** (`Ctx.DeferredActivations`, accodate da `CollectHealActions` e dal ramo `ModifyArc` di
		//   `CollectAttackIntents`; emesse dal ciclo di `ResolveCombatPasses` con `EmitAbilityActivated`): NON passano da
		//   `InterruptedIntents`, e l'emissione non lo legge. Un curatore STORDITO non si attiva: in `CollectHealActions` il
		//   rifiuto (`RefuseMainActionIfStunned` e `continue`) sta PRIMA dell'accodamento, nel ramo `ModifyArc` uguale, e un
		//   curatore morto esce prima ancora. Un curatore INTERROTTO invece SI attiva ancora: `ApplyInterrupts` sceglie le
		//   vittime fra `Intents[k]` con `AttackerId == Hit.TargetId`, e una cura non e' mai in `Intents` —
		//   `CollectHealActions` azzera il piano e `continue`, e `ResolveCombatPasses` la chiama prima di
		//   `CollectAttackIntents` — quindi un `Action.Interrupt` su chi cura non ha niente da cancellare e la cura avviene.
		//
		// ⛔ `Ev.ActionId` non e' mai vuoto in questo punto: `EmitAbilityActivated` non emette senza, quindi
		// `BuildSkillCallout` non vedra' mai un `Label` ricavato da `None`.
		//
		// 🔴 **L'equipaggiamento non grida** (A7: solo le skill del KIT; R1b: «la generica con un nome» non grida).
		// `MakeEquipmentAction` scrive `ActionId = EquipmentId` (`Gadget.*`, `Reaction.*`) e copia dal core un
		// `BaseActionId` vuoto: `FindCoreAction` non lo trova e la base non lo esclude, quindi senza la seconda domanda il
		// loadout di default faceva gridare «SPRINKLER» a Muiren e «COPERTURA PORTATILE» a Branth. La seconda domanda si fa
		// solo se la prima risponde no: un'attivazione core non costruisce i cataloghi d'equipaggiamento.
		const bool bIsGenericOrEquipment = !URTCatalogLibrary::FindCoreAction(Ev.ActionId).ActionId.IsNone()
			|| URTCatalogLibrary::FindEquipment(Ev.ActionId) != nullptr;
		if (URTHudViewModel::ActionShouts(Ev.ActionId, Ev.BaseActionId, bIsGenericOrEquipment))
		{
			Src->ShowSkillCallout(Ev.ActionId, SkillCalloutClock);
			// L'orologio ARRIVA con la comparsa, non al tick dopo: il registro dell'unita' tiene l'ultimo valore ricevuto,
			// e fino al prossimo `SyncSkillCalloutClocks` conserverebbe quello di un fumetto (o di un turno) precedente.
			Src->SetSkillCalloutClock(SkillCalloutClock);
			SkillCalloutUnits.AddUnique(Src);
		}
	}
}

void FRTPlaybackDirector::AddActivationLine(const FRTResolvedEvent& Ev, const ARTUnit* Src)
{
	// R5: il testo si compone ADESSO, perche' qui c'e' l'unita'; dopo, la riga non legge piu' il mondo. Senza unita' non c'e'
	// nessuno da nominare, e nessuna riga.
	if (Src == nullptr)
	{
		return;
	}

	// 🔑 **Una riga per attivazione, e la chiave `(fase, sorgente, azione)` basta senza ordinale.** Misurato per lettura su
	// questo commit: l'unico produttore di `AbilityActivated` e' `EmitAbilityActivated`, e ogni suo sito emette per il piano
	// di UN'unita' in UNA fase — `PlannedAbilityIndex`, azzerato dal primo sito che lo consuma (Prep, Cleanse, cure,
	// `CollectAttackIntents`), o `DashAbilityIdx` — mentre l'impatto di una carica non si riattiva nel Blast
	// (`IntentAbilityIndex == INDEX_NONE`). Quindi un'unita' non attiva due volte la stessa azione nella stessa fase di un
	// turno. Se un giorno potra' farlo, la chiave vorra' l'ordinale fra le attivazioni con la stessa terna.
	// ⛔ Serve perche' la stessa attivazione ha due ingressi: `ShowActivation` e il recupero di fine playback (R8, che passa
	// anche per le gia' mostrate), piu' un'eventuale `ShowActivation` ripetuta dopo una finestra di reazione.
	const bool bGiaScritta = ActivationLines.ContainsByPredicate([&Ev](const FRTActivationLine& Riga)
	{
		return Riga.Phase == Ev.Phase && Riga.SourceStableUnitId == Ev.SourceStableUnitId && Riga.ActionId == Ev.ActionId;
	});
	if (bGiaScritta)
	{
		return;
	}

	FRTActivationLine& Riga = ActivationLines.AddDefaulted_GetRef();
	Riga.SourceStableUnitId = Ev.SourceStableUnitId;
	Riga.ActionId = Ev.ActionId;
	Riga.Phase = Ev.Phase;
	Riga.ViewerTeamId = PlaybackViewerTeamId; // R7b: la chiave del pubblico, non un verdetto di conoscenza
	Riga.TurnNumber = Host->GetPlaybackTurnNumber(); // lo stesso delle voci di TurnLog del turno: `ConcludeTurn` lo avanza DOPO il playback
	Riga.Text = URTHudViewModel::ComposeActivationLineText(
		ARTUnit::DisplayLabel(Src->HeroDisplayName, Src->HeroId, Src->GetName()), Src->KitDisplayNameFor(Ev.ActionId),
		Ev.ActionId);
}

void FRTPlaybackDirector::SyncSkillCalloutClocks()
{
	// Il tempo del fumetto e' `SkillCalloutClock` e basta (R4, A8): nessun `DeltaTime` passa di qui. In pausa
	// `TickPlayback` esce prima di arrivarci, quindi l'orologio non viene portato ed e' la regola, non un caso.
	// ⛔ Non `PlaybackElapsedTotal`: `ConcludeResolution` lo azzera alla ripresa dopo una finestra di reazione, e i
	// fumetti accesi prima tornerebbero pieni e ingranditi (vedi `SkillCalloutClock`).
	for (int32 I = SkillCalloutUnits.Num() - 1; I >= 0; --I)
	{
		if (ARTUnit* const Unit = SkillCalloutUnits[I].Get())
		{
			Unit->SetSkillCalloutClock(SkillCalloutClock);
		}
		else
		{
			SkillCalloutUnits.RemoveAt(I); // distrutta a meta' turno: niente da portare e niente da spegnere
		}
	}
}

bool FRTPlaybackDirector::NotePlaybackActShown(const FRTResolvedEvent& Ev)
{
	// `#2855`, `#3292`, `#3549`: il confine d'AZIONE, valutato su OGNI fatto che il playback rivela — attivazioni di
	// Prep e Dash, elementi della sequenza di Blast. ⏱️ *Fino a #3549 lo valutavano tre siti, uno per canale
	// parallelo del Blast; Prep e Dash non ne avevano nessuno, e `Next Action` le attraversava.*
	//
	// 🔑 **Si ferma DOPO aver mostrato il fatto, non prima.** `Next Action` vuol dire *«portami al prossimo
	// atto»*: fermarsi un istante prima lo lascerebbe fuori dallo schermo, cioe' porterebbe dove l'atto sta per
	// cominciare invece che dove comincia.
	//
	// ✅ **La regola e' quella di `URTPlaybackLibrary::IsActBoundary`**, l'unico posto in cui esiste (`#3292`), e
	// il criterio e' la coppia `(sorgente, azione)` (#3549).
	//
	// ⚠️ L'atto in corso si aggiorna **solo** su un'azione vera: un `NAME_None` non lo azzera, come nella
	// scansione all'indietro di `NextActionBoundary`. Su uno `StructureHit` il vuoto E' un confine ([D-437]) e
	// non aggiorna l'atto: l'aggregato ha piu' autori, quindi non ce n'e' uno a cui l'atto possa passare.
	// 🔴 **E la sorgente si aggiorna solo da un evento che ne porta una** (review della PR #3561): `0` e' «sorgente
	// sconosciuta» ([D-063]) e non cancella quella nota dell'atto, che `IsActBoundary` confronta. Quando l'AZIONE
	// cambia, invece, la sorgente dell'atto precedente non vale piu': si riparte da quella del nuovo, `0` compreso.
	// E' la regola della scansione all'indietro di `URTPlaybackLibrary::NextActionBoundary`, scritta in avanti.
	if (!Ev.ActionId.IsNone())
	{
		if (Ev.ActionId != PlaybackLastShownAction)
		{
			PlaybackLastShownSource = 0;
		}
		PlaybackLastShownAction = Ev.ActionId;
		if (Ev.SourceStableUnitId != 0)
		{
			PlaybackLastShownSource = Ev.SourceStableUnitId;
		}
	}
	if (PlaybackStopAt == ERTPlaybackStopAt::NextAction
		&& URTPlaybackLibrary::IsActBoundary(Ev, PlaybackStopFromAction, PlaybackStopFromSource))
	{
		PausePlaybackAtActBoundary();
		return true; // cio' che questo tick avrebbe ancora rivelato resta per la ripresa
	}
	return false;
}

bool FRTPlaybackDirector::RevealPlaybackActivations(const TArray<FRTResolvedEvent>& Activations, int32 UpTo)
{
	const int32 Target = FMath::Min(UpTo, Activations.Num());
	while (ActivationsShown < Target)
	{
		const FRTResolvedEvent& Ev = Activations[ActivationsShown];
		ShowActivation(Ev);
		++ActivationsShown;
		// #3549: i rami Prep e Dash aggiornano l'atto in corso come fa il Blast, o `Next Action` salterebbe le
		// attivazioni (spec §2.4, confini d'atto).
		if (NotePlaybackActShown(Ev))
		{
			return true;
		}
	}
	return false;
}

// La sequenza di Blast (#3549, D5). I commenti dei tre rami di `BeginPlayback` che questa funzione sostituisce,
// in cio' che hanno di ancora vero:
//
// ⛔ **Nessun filtro qui, e non e' una dimenticanza**: i filtri sono a monte, in `BuildBlastSequence` (vedi
// sotto). ⏱️ *Fino a #3681 questa riga giustificava l'assenza di filtro sul muro con «il soggetto e' un BORDO,
// e un muro che cade non rivela chi ci stava dietro». Era vero per chi stava DIETRO il muro, e falso per chi
// aveva sparato: il bordo dice la fila da cui e' arrivato il colpo ([D-502]).* ⚠️ Se un giorno l'impronta
// portasse un dato dipendente da CHI e' stato colpito, diventerebbe un canale e andrebbe filtrata come il `Move`.
// ⚠️ **Nessun `Src` richiesto per impronte e muri**: il fatto riguarda la mappa e resta vero anche se chi ha
// sparato e' morto nello stesso Blast. Pretendere l'Actor perderebbe proprio i colpi dei caduti.
// 🔴 **Il filtro di FASE ora c'e'.** ⏱️ *Fino a #3549 il ramo `StructureHit` di `BeginPlayback` non filtrava
// `Ev.Phase`, e un secondo produttore in Cleanup avrebbe inventato una fase `Blast`.* `BuildBlastSequence`
// prende solo `Phase == Blast`: un muro colpito in Cleanup non entra in questa sequenza e non apre la fase.
// 🔑 **I filtri di privacy di questa sequenza sono a monte, in `BuildBlastSequence`**: le ATTIVAZIONI (D6); da
// `#3551`, le tracce di un `Line`/`Cone` il cui attaccante chi guarda non vedeva ([D-500]); da `#3681`, i muri
// colpiti che chi guarda non osservava ([D-502]). Qui si mostra tutto cio' che e' in sequenza.
bool FRTPlaybackDirector::AdvanceBlastSequence(int32 BeatsTarget)
{
	const int32 Target = FMath::Min(BeatsTarget, 2 * PlaybackBlastSequence.Num());
	if (BlastBeatsDone >= Target)
	{
		return false;
	}

	// ⚠️ L'actor si cerca UNA volta per chiamata, non per elemento: `FindInWorld` itera gli attori, e farlo dentro
	// il ciclo lo renderebbe quadratico in un ramo che gira a ogni tick del Blast.
	ARTHexMapActor* const MapActor = ARTHexMapActor::FindInWorld(Host->GetPlaybackWorld());

	while (BlastBeatsDone < Target)
	{
		const int32 Elemento = BlastBeatsDone / 2;
		const bool bArrivo = (BlastBeatsDone % 2) == 1;
		// ⚠️ Avanza anche con un indice fuori range o senza `MapActor`: un battito consumato non ripassa.
		++BlastBeatsDone;
		const int32 Indice = PlaybackBlastSequence[Elemento].TimelineIndex;
		if (!Timeline().IsValidIndex(Indice))
		{
			continue;
		}
		const FRTResolvedEvent& Ev = Timeline()[Indice];

		if (bArrivo)
		{
			// 🔑 **L'arrivo esiste solo per un colpo** (`#2454`): per ogni altro tipo il volo e' zero e il battito
			// non fa niente — l'elemento si e' gia' mostrato tutto alla rivelazione.
			if (Ev.Type != ERTResolvedEventType::Attack)
			{
				continue;
			}
			// L'ordinale serve solo alla traccia dei test: in produzione non si scorre la sequenza a ogni arrivo.
			ArrivePlaybackAttack(Ev, Tuning().bRecordAttackBeatsForTest ? BlastAttackOrdinal(Elemento) : 0);
			// ⛔ **L'arrivo ha appena trasmesso `OnAttackResolved`**: un ascoltatore (Blueprint-assegnabile) puo'
			// chiudere il playback — `SkipPlayback` → `FinishPlayback` svuota la sequenza e azzera il cursore — e
			// da qui in poi nulla del playback va riletto senza saperlo (`#2454`, guardia di rientranza).
			if (!Host->IsPlaybackResolving() || !PlaybackBlastSequence.IsValidIndex(Elemento) || !Timeline().IsValidIndex(Indice))
			{
				return true;
			}
			// `#2855`: il confine d'atto di un colpo cade al suo ARRIVO, perche' il colpo e' mostrato quando arriva.
			if (NotePlaybackActShown(Timeline()[Indice]))
			{
				return true; // i battiti che questo tick avrebbe ancora eseguito restano per la ripresa
			}
			continue;
		}

		// 🔑 **Un `switch`, e i rami sono le cue di prima** (#3549, D5): la cue per elemento non cambia, cambia
		// chi decide QUANDO — la sequenza, non tre contatori. Per un colpo la rivelazione e' il LANCIO (`#2454`).
		switch (Ev.Type)
		{
		case ERTResolvedEventType::AbilityActivated:
			ShowActivation(Ev);
			break;
		case ERTResolvedEventType::AttackFootprint:
			// ⛔ Le celle COSI' COME ARRIVANO ([D-301]): `HexHitCells` le ha gia' prodotte nell'ordine stabile di
			// `URTHexLibrary::StableLess`, e riordinarle o filtrarle qui sarebbe la seconda risposta a una domanda che
			// il resolver ha gia' chiuso.
			if (MapActor) { MapActor->AddPlaybackFootprint(Ev.HitCells); }
			break;
		case ERTResolvedEventType::StructureHit:
			// ⛔ **Il bordo che l'evento PORTA, e nient'altro** (#2828): non si chiama `FirstCoveredEdge`, non si
			// chiede alla mappa cosa ci fosse su quel lato, non si converte con `EdgeDirection`. Ognuna sarebbe una
			// seconda risposta a una domanda che la simulazione ha gia' chiuso.
			if (MapActor)
			{
				MapActor->AddPlaybackStructureHit(Ev.StructureCell, Ev.StructureToward,
					Ev.EnvironmentOutcome == ERTEnvironmentOutcome::CoverDestroyed);
			}
			break;
		case ERTResolvedEventType::Healed:
			// [D-505], `#3600`: il «+N» e la barra che pulsa sul CURATO, alla rivelazione — una cura non vola.
			// ⛔ Nessun controllo di velo qui: chi vede il curato lo decide `ARTHUD::UpdateObserverVeil`
			// sull'overlay, come per il token del colpo (`ARTUnit::ShowDamageToken`).
			if (ARTUnit* const Curato = Host->UnitByStableId(Ev.TargetStableUnitId))
			{
				Curato->ShowHealToken(Ev.Amount);
			}
			break;
		case ERTResolvedEventType::Attack:
			// L'ordinale serve solo alla traccia dei test: in produzione non si scorre la sequenza a ogni lancio.
			LaunchPlaybackAttack(Ev, Tuning().bRecordAttackBeatsForTest ? BlastAttackOrdinal(Elemento) : 0);
			// ⚠️ Nessun confine qui: quello di un colpo cade all'arrivo, ed e' il battito che segue.
			continue;
		default:
			break;
		}

		if (NotePlaybackActShown(Ev))
		{
			return true;
		}
	}
	return false;
}

int32 FRTPlaybackDirector::BlastAttackOrdinal(int32 SequenceIndex) const
{
	int32 Ordinale = 0;
	for (int32 K = 0; K < SequenceIndex && PlaybackBlastSequence.IsValidIndex(K); ++K)
	{
		const int32 Indice = PlaybackBlastSequence[K].TimelineIndex;
		if (Timeline().IsValidIndex(Indice) && Timeline()[Indice].Type == ERTResolvedEventType::Attack)
		{
			++Ordinale;
		}
	}
	return Ordinale;
}

void FRTPlaybackDirector::LaunchPlaybackAttack(const FRTResolvedEvent& Atk, int32 AttackOrdinal)
{
	if (ARTUnit* const AtkSrc = Host->UnitByStableId(Atk.SourceStableUnitId))
	{
		// ⚠️ Per l'impatto di una carica `ActionId` e' lo SCATTO (`Impact.Def = Dash->Def`): e' voluto (#3563).
		AtkSrc->PlayPresentationRole(ERTPresentationRole::Attack, Atk.ActionId, Atk.BaseActionId);
	}
	if (Tuning().bRecordAttackBeatsForTest) { AttackBeatTrace.Add(FString::Printf(TEXT("L%d"), AttackOrdinal)); }
}

void FRTPlaybackDirector::ArrivePlaybackAttack(const FRTResolvedEvent& Atk, int32 AttackOrdinal)
{
	ARTUnit* const AtkSrc = Host->UnitByStableId(Atk.SourceStableUnitId);
	ARTUnit* const AtkTgt = Host->UnitByStableId(Atk.TargetStableUnitId);
	const int32 Amount = Atk.Amount; // ⛔ `Atk` vive nella timeline: dopo il broadcast non si rilegge
	Host->AddLogEvent(FString::Printf(TEXT("Colpo: %s -> %s (%d)"),
		AtkSrc ? *AtkSrc->GetName() : TEXT("?"),
		AtkTgt ? *AtkTgt->GetName() : TEXT("(eliminato)"),
		// `FRTLogSubject::Unit` vuole l'Actor e non l'id, e lo dichiara: da un id soltanto il
		// verdetto di [D-223] non si calcola — servono anche squadra e cella.
		Amount), FRTLogSubject::Unit(AtkSrc));
	if (AtkTgt)
	{
		AtkTgt->PlayPresentationRole(ERTPresentationRole::Hit);
		// #2455 — il NUMERO del colpo, dallo stesso evento e nello stesso istante della cue.
		//
		// 🔑 **Il simulatore passa un intero, non una vista.** `Atk.Amount` e' lo stesso valore che
		// il log scrive e che `OnAttackResolved` gia' trasporta: la composizione avviene in `ARTUnit`,
		// e questo file continua a non includere **nessun** header di `UI/`.
		//
		// ⛔ Sul BERSAGLIO e mai sull'attaccante: e' chi subisce a portare il numero, la stessa
		// convenzione della cue di impatto e della categoria `Combat` del TurnLog (`#1150`).
		AtkTgt->ShowDamageToken(Amount);
	}
	Signals.OnAttackResolved->Broadcast(AtkSrc, AtkTgt, Amount);
	if (Tuning().bRecordAttackBeatsForTest) { AttackBeatTrace.Add(FString::Printf(TEXT("A%d"), AttackOrdinal)); }
}

void FRTPlaybackDirector::PushPlaybackTracers()
{
	TArray<FRTPlaybackTracer> InVolo;
	if (BlastBeatsDone % 2 == 1) // rivelato, non ancora arrivato: in volo solo se e' un colpo con un tracer
	{
		const int32 Elemento = BlastBeatsDone / 2;
		if (PlaybackBlastSequence.IsValidIndex(Elemento) && PlaybackBlastFlights.IsValidIndex(Elemento)
			&& Timeline().IsValidIndex(PlaybackBlastSequence[Elemento].TimelineIndex))
		{
			const FRTResolvedEvent& Atk = Timeline()[PlaybackBlastSequence[Elemento].TimelineIndex];
			// `TracerStyleFor` risponde `None` per ogni tipo che non sia un `Attack` idoneo (`IsTracerEligible`).
			const ERTTracerStyle Style = URTPlaybackLibrary::TracerStyleFor(Atk, PlaybackViewerTeamId);
			if (Style != ERTTracerStyle::None)
			{
				FRTPlaybackTracer T;
				T.From = Atk.HitGeometry.From;
				T.To = Atk.HitGeometry.Impact;
				T.Style = Style;
				// ⚠️ L'indice e' quello dell'ELEMENTO nella sequenza: il lancio cade a `Elemento·A`, lo stesso istante
				// di `AttacksToShow` e di `AttackBeatsDue` sui voli della sequenza.
				T.Alpha = URTPlaybackLibrary::TracerAlpha(
					Elemento, PlaybackPhaseElapsed, Tuning().AttackShowSeconds, PlaybackBlastFlights[Elemento]);
				InVolo.Add(T);
			}
		}
	}
	// ⚠️ **Si consegna solo se c'e' qualcosa da dire alla mappa.** Questa funzione gira a OGNI tick del `Blast` e
	// `FindInWorld` scorre il mondo: e' la stessa preoccupazione per cui `AdvanceBlastSequence` cerca l'actor
	// UNA volta per chiamata e non per elemento. Niente in volo adesso E canale gia' vuoto = niente da consegnare.
	// ⛔ Il canale si spegne anche altrove (finalizzazione del `Blast`, `FinishPlayback`), e il flag si azzera
	// ESATTAMENTE li': un canale svuotato non va creduto pieno, e uno pieno non va creduto vuoto — il tracer
	// resterebbe disegnato.
	const bool bInVolo = !InVolo.IsEmpty();
	if (!bInVolo && !bPlaybackTracerChannelFull)
	{
		return;
	}
	ARTHexMapActor* const MapActor = ARTHexMapActor::FindInWorld(Host->GetPlaybackWorld());
	if (!MapActor) { return; }

	MapActor->SetPlaybackTracers(InVolo);
	bPlaybackTracerChannelFull = bInVolo;
}

void FRTPlaybackDirector::PushPlaybackCues(ERTMatchPhase InPhase)
{
	// ⚠️ `InPhase` e non `Phase`: `Phase` e' gia' un membro (la fase della partita), e il parametro lo nasconderebbe
	// (`C4458`, che qui e' un errore). E' la fase del PLAYBACK, quella che `TickPlayback` chiama `Ph`.
	TArray<FRTPlaybackCue> Cues;
	if (InPhase == ERTMatchPhase::Prep || InPhase == ERTMatchPhase::Dash)
	{
		const TArray<FRTResolvedEvent>& Coda = (InPhase == ERTMatchPhase::Prep) ? PlaybackActivationsPrep : PlaybackActivationsDash;
		URTPlaybackLibrary::ActivationCuesAt(Coda, ActivationsShown, PlaybackPhaseElapsed, Tuning().AttackShowSeconds,
			Tuning().ActivationCueSeconds, PlaybackViewerTeamId, Cues);
	}
	else if (InPhase == ERTMatchPhase::Blast)
	{
		URTPlaybackLibrary::BlastActivationCuesAt(Timeline(), PlaybackBlastSequence, BlastBeatsDone,
			PlaybackPhaseElapsed, Tuning().AttackShowSeconds, Tuning().ActivationCueSeconds, PlaybackViewerTeamId, Cues);
		URTPlaybackLibrary::BlastHitCuesAt(Timeline(), PlaybackBlastSequence, PlaybackBlastFlights,
			PlaybackBlastFootprintFx, BlastBeatsDone, PlaybackPhaseElapsed, Tuning().AttackShowSeconds, Tuning().ImpactCueSeconds,
			PlaybackViewerTeamId, Cues);
	}
	// ⚠️ Stessa economia di `PushPlaybackTracers`: niente da dire E canale gia' vuoto = la mappa non si tocca.
	const bool bPiene = !Cues.IsEmpty();
	if (!bPiene && !bPlaybackCueChannelFull)
	{
		return;
	}
	ARTHexMapActor* const MapActor = ARTHexMapActor::FindInWorld(Host->GetPlaybackWorld());
	if (!MapActor) { return; }
	MapActor->SetPlaybackCues(Cues);
	bPlaybackCueChannelFull = bPiene;
}

void FRTPlaybackDirector::EnterPlaybackPhase()
{
	PlaybackPhaseElapsed = 0.f;
	ActivationsShown = 0;
	AttackBeatTrace.Reset();
	// ⚠️ Il cursore dei battiti si azzera a ogni FASE come i contatori che sostituisce, e il ramo `bPreserveClock`
	// salta questa funzione: e' cio' che rende `BlastElementsShown()` il prefisso congelato di D-355, e cio' che
	// impedisce all'estensione di rigiocare lanci e arrivi (`#2454`, `Reactions.Brace.ExtendedBlastDoesNotReplayHits`).
	// ⚠️ **Ma `ARTHexMapActor` svuota impronte e muri solo in `FinishPlayback`.** Oggi non si vede perche'
	// `Blast` compare al massimo una volta in `PlaybackPhases`. ⛔ Ma l'invariante non e' difeso da niente: un
	// passo-indietro che rientrasse nel `Blast`, o un `PlaybackPhases` che lo contenesse due volte,
	// ri-aggiungerebbe ogni impronta e ogni muro. Chi introduce uno dei due svuoti anche i canali dell'actor.
	BlastBeatsDone = 0;
	// `#3292`: l'atto in corso e' un contatore di rivelazione come gli altri, e si azzera con loro.
	// ⚠️ Entrando in una fase nessun atto e' ancora passato, quindi il primo fatto che esce e' gia' un
	// confine — la stessa semantica che `NextActionBoundary` da' a un indice negativo.
	PlaybackLastShownAction = NAME_None;
	PlaybackLastShownSource = 0;
	const ERTMatchPhase Ph = PlaybackPhases[PlaybackPhaseIdx];
	Host->AddLogEvent(FString::Printf(TEXT("Playback fase: %s"), *GetPlaybackPhaseName()), FRTLogSubject::World());
	Signals.OnPhasePlaybackStarted->Broadcast(Ph);
	// In corsa deve risultare SOLO chi si muove in QUESTA fase. Senza l'azzeramento, un'unita' che ha scattato
	// nel Dash resta con il flag alzato per tutto il Blast (e il Move) e l'AnimBP la tiene in corsa sul posto:
	// il flag lo spegneva solo FinishPlayback, a risoluzione conclusa.
	for (const FRTMoveAnim& A : MoveAnims)
	{
		// 🔴 La posa torna a riposo INSIEME al flag, e non dopo: un `Lean` concluso TIENE il proprio valore
		// finale (e' il patto di `WindowAlpha`), quindi senza questa riga il corpo resterebbe inclinato per
		// tutto il resto del turno — un difetto che si vedrebbe solo dal secondo turno in poi.
		if (A.Unit.IsValid()) { A.Unit->bIsMovingVisually = false; A.Unit->ResetGraykitPose(); }
	}

	if (Ph == ERTMatchPhase::Dash || Ph == ERTMatchPhase::Move || Ph == ERTMatchPhase::Blast)
	{
		// #3549: con attivazioni nel Dash la corsa NON parte con la fase — la accende `TickPlayback` quando le
		// rotte cominciano, dopo l'anticipo. Accenderla qui farebbe correre sul posto lo scattatore mentre suona
		// il cast: la classe di difetti di #3519. L'annuncio `OnUnitMoveStarted` resta all'ingresso.
		const bool bCorsaRinviata = PhaseTimeForPlaybackPhase(Ph).Lead > 0.f;
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.Phase == Ph && A.Unit.IsValid())
			{
				if (!bCorsaRinviata) { A.Unit->bIsMovingVisually = true; }
				Signals.OnUnitMoveStarted->Broadcast(A.Unit.Get());
			}
		}
	}
}

int32 FRTPlaybackDirector::MicroStepsInCurrentPlaybackPhase() const
{
	if (!PlaybackPhases.IsValidIndex(PlaybackPhaseIdx))
	{
		return 0;
	}
	const ERTMatchPhase Ph = PlaybackPhases[PlaybackPhaseIdx];

	int32 Massimo = 0;
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Phase == Ph)
		{
			Massimo = FMath::Max(Massimo, URTPlaybackLibrary::MicroStepsInPath(A.World));
		}
	}
	return Massimo;
}

void FRTPlaybackDirector::SetPlaybackControlsEnabled(bool bEnabled)
{
	bPlaybackControlsEnabled = bEnabled;
	if (!bEnabled)
	{
		// ⚠️ Togliere i controlli mentre il playback e' fermo lo fa RIPARTIRE. Lasciarlo in pausa senza il
		// comando per riprenderlo sarebbe una partita bloccata da un flag, e il modo piu' facile di
		// arrivarci e' proprio disabilitare i controlli in una sessione in pausa.
		bPlaybackPaused = false;
		PlaybackStepTargetElapsed = -1.f;
		// `#2855`: e con essi cade il predicato armato. Un `Next Phase` che sopravvivesse alla revoca dei
		// controlli fermerebbe il playback in una sessione che non ha piu' il comando per riprenderlo — la
		// stessa partita bloccata da un flag che le due righe qui sopra esistono per evitare.
		PlaybackStopAt = ERTPlaybackStopAt::None;
		PlaybackStopFromAction = NAME_None;
		PlaybackStopFromSource = 0;
		// `#2858`: e cade anche la politica di partire in pausa. Sopravvivendo, il prossimo playback
		// nascerebbe fermo in una sessione che non ha piu' il comando per riprenderlo — la stessa partita
		// bloccata da un flag, per una terza via.
		bStartPlaybackPaused = false;
	}
}

void FRTPlaybackDirector::SetStartPlaybackPaused(bool bStartPaused)
{
	// ⛔ **Subordinato ai controlli, e non e' una comodita'.** Partire in pausa senza `ResumePlayback`
	// raggiungibile e' una partita che non riparte. Chiederlo a controlli spenti non fa nulla **e non lo
	// ricorda per dopo**: un'intenzione messa in coda si applicherebbe a un'accensione successiva che
	// nessuno ha collegato a questa richiesta.
	if (!bPlaybackControlsEnabled)
	{
		return;
	}
	bStartPlaybackPaused = bStartPaused;
}

void FRTPlaybackDirector::PausePlayback()
{
	if (!bPlaybackControlsEnabled)
	{
		return; // fail-closed: vedi `bPlaybackControlsEnabled`
	}
	bPlaybackPaused = true;
	// Uno `Step` in volo viene abbandonato: la pausa e' esplicita e vince su un avanzamento gia' chiesto.
	PlaybackStepTargetElapsed = -1.f;
}

void FRTPlaybackDirector::ResumePlayback()
{
	if (!bPlaybackControlsEnabled)
	{
		return;
	}
	bPlaybackPaused = false;
	PlaybackStepTargetElapsed = -1.f;
}

void FRTPlaybackDirector::StepMicroStep()
{
	if (!bPlaybackControlsEnabled)
	{
		return;
	}

	const int32 Passi = MicroStepsInCurrentPlaybackPhase();
	const float Durata = PlaybackPhases.IsValidIndex(PlaybackPhaseIdx)
		? DurationForPlaybackPhase(PlaybackPhases[PlaybackPhaseIdx])
		: 0.f;

	// Una fase senza segmenti o senza durata non ha barriere da attraversare: si resta fermi invece di
	// inventare un avanzamento. ⚠️ Senza questo, `Alpha` sarebbe una divisione per zero.
	if (Passi <= 0 || Durata <= 0.f)
	{
		bPlaybackPaused = true;
		PlaybackStepTargetElapsed = -1.f;
		return;
	}

	// #3549: nel Dash i micro-step cominciano DOPO le attivazioni — i confini si contano sul solo tratto delle
	// rotte, o `Step` si fermerebbe a meta' di un segmento. Fuori dal Dash l'anticipo e' zero e la formula e'
	// quella di prima.
	const float Anticipo = FMath::Clamp(PhaseTimeForPlaybackPhase(PlaybackPhases[PlaybackPhaseIdx]).Lead, 0.f, Durata);
	const float DurataRotte = Durata - Anticipo;
	if (DurataRotte <= 0.f)
	{
		bPlaybackPaused = true;
		PlaybackStepTargetElapsed = -1.f;
		return;
	}
	const float AlphaCorrente = FMath::Clamp((PlaybackPhaseElapsed - Anticipo) / DurataRotte, 0.f, 1.f);
	const float AlphaTarget = URTPlaybackLibrary::NextMicroStepBoundary(AlphaCorrente, Passi);

	// Il confine in SECONDI, calcolato ora: il tick ci arriva senza sapere quanti frame servono.
	PlaybackStepTargetElapsed = Anticipo + AlphaTarget * DurataRotte;
	bPlaybackPaused = false; // si riparte, ma solo fino al confine
}

void FRTPlaybackDirector::RequestPlaybackStopAt(ERTPlaybackStopAt Boundary)
{
	if (!bPlaybackControlsEnabled)
	{
		return; // fail-closed, come `#1879`: vedi `bPlaybackControlsEnabled`
	}

	PlaybackStopAt = Boundary;

	if (Boundary == ERTPlaybackStopAt::None)
	{
		PlaybackStopFromAction = NAME_None;
		PlaybackStopFromSource = 0;
		return;
	}

	// 🔑 **L'atto in corso si congela ADESSO**, ed e' il termine di paragone del confine. Se non ne e'
	// ancora passato nessuno resta `NAME_None`, cioe' «nessun atto in corso» — e il primo che passa e' gia'
	// un confine, la stessa semantica che `NextActionBoundary` da' a un indice negativo.
	//
	// 🔴 **Si legge da TUTTI i canali, e prima da uno solo** (`#3292`). Questa riga diceva
	// `PlaybackAttacks[AttacksShown - 1]`, cioe' l'ultimo **colpo**: ∴ dopo essersi fermati su
	// un'impronta o su un muro, il paragone tornava a un'azione **precedente**, e il fatto successivo
	// dello stesso intento sembrava aprire un atto nuovo. Due fermate dentro un intento solo.
	// ⚠️ `PlaybackLastShownAction` lo tiene aggiornato `NotePlaybackActShown`, per ogni fatto che il playback
	// rivela in qualunque fase (#3549). ⏱️ *Fino a #3549 lo aggiornavano i tre canali paralleli del Blast, che
	// avevano contatori indipendenti e nessun indice comune da cui leggerlo all'indietro.* Anche con la sequenza
	// unica la risposta resta questa: Prep e Dash hanno code proprie, e la sequenza non e' la timeline.
	//
	// 🔑 La COPPIA (#3549): azione e sorgente si congelano insieme. ⚠️ `0` e' la sentinella dello stato — nessuna
	// sorgente, nessun atto in corso — ed e' diversa dal default `-1` di `IsActBoundary`, che vuol dire «sorgente
	// non dichiarata» e riporterebbe il criterio storico sul solo `ActionId`.
	PlaybackStopFromAction = PlaybackLastShownAction;
	PlaybackStopFromSource = PlaybackLastShownSource;

	// 🔑 **Si riparte, ma solo fino al confine — la stessa forma di `StepMicroStep`.** E' lo stesso gesto a
	// una granularita' diversa: *«portami al prossimo X e fermati li'»*. Chi lo preme lo preme quasi sempre
	// **da fermo**, e un predicato che si limitasse ad armarsi chiederebbe un `Resume` per fare qualcosa —
	// due comandi per un'intenzione sola, e un `Next Phase` che a schermo non fa niente.
	//
	// ⚠️ Uno `Step` in volo viene abbandonato: il comando piu' recente vince, come in `PausePlayback`.
	bPlaybackPaused = false;
	PlaybackStepTargetElapsed = -1.f;
}

void FRTPlaybackDirector::TickPlayback(float DeltaSeconds)
{
	// RILETTA a ogni tick, non congelata in BeginPlayback: e' cio' che rende la velocita' scelta
	// applicabile DURANTE la risoluzione, e non solo dal turno successivo (CP 47.2, #955).
	//
	// 🔑 **Il budget NON entra qui, e non ci entra piu' dal 2026-09-02** (#1878). Questo e' l'unico
	// orologio del playback: quello che divide `PlaybackPhaseElapsed` per la durata di fase e produce
	// l'`Alpha` con cui i cilindri si interpolano. Moltiplicarlo per un fattore derivato dal tetto era
	// **il** modo in cui la durata target finiva per decidere la velocita' visuale della locomozione. Il
	// budget agisce ora sulla DURATA della fase (`DurationForPlaybackPhase`, attese comprimibili), non
	// sulla velocita' con cui la si attraversa.
	// 🔴 **In pausa l'orologio non avanza, e il tick esce prima di toccare qualunque cosa** (`#1879`).
	// Non `Dt = 0`: un tick che prosegue con passo nullo rivelerebbe comunque i colpi del Blast, perche'
	// `AttacksToShow` legge `PlaybackPhaseElapsed` e non il passo. Fermarsi significa non guardare.
	if (bPlaybackPaused && PlaybackStepTargetElapsed < 0.f)
	{
		return;
	}

	// 🔑 **Trattenuto da una finestra**: il playback parziale ha mostrato tutto cio' che era risolto e
	// aspetta la risposta. Non e' la pausa del giocatore — quella e' `bPlaybackPaused`, ha i suoi comandi
	// e un suo significato — ed e' tenuta separata di proposito: due pause con lo stesso flag si
	// annullerebbero a vicenda al primo `ResumePlayback`.
	if (bPlaybackHeldByWindow)
	{
		return;
	}

	const float Dt = DeltaSeconds * URTPlaybackLibrary::EffectivePlaybackSpeed(Tuning().ViewerPlaybackSpeed);

	// 🔴 **La coda della morte, e sta PRIMA di leggere la fase corrente** (#2452): quando l'ultima fase si e'
	// chiusa su un'eliminazione, `PlaybackPhaseIdx` e' gia' oltre l'ultimo indice — `PlaybackPhases[Idx]`
	// qui sotto sarebbe fuori range. Il tick si esaurisce qui finche' la coda non scade.
	//
	// Il tempo scorre con lo stesso `Dt` di tutto il resto, quindi la coda scala con la velocita' scelta da
	// chi guarda; e `PlaybackElapsedTotal` la conta, perche' e' tempo che si vede.
	//
	// ⛔ Non aspetta il montaggio: aspetta una DURATA. Un `Death` assente non blocca nulla, e la risoluzione
	// resta deterministica — la presentazione non decide (invariante #1).
	if (PlaybackDefeatBeatRemaining > 0.f)
	{
		PlaybackDefeatBeatRemaining -= Dt;
		PlaybackElapsedTotal += Dt;
		// #4025: la coda e' tempo che si vede, quindi il fumetto ancora acceso lo vive fino alla fine (pop, tenuta,
		// dissolvenza) invece di restare fermo sull'ultimo valore finche' `FinishPlayback` non lo spegne. Il suo orologio
		// avanza dello STESSO `Dt` del totale: e' un altro accumulatore dello stesso tempo, non un altro tempo.
		SkillCalloutClock += Dt;
		SyncSkillCalloutClocks();
		if (PlaybackDefeatBeatRemaining <= 0.f)
		{
			PlaybackDefeatBeatRemaining = 0.f;
			FinishPlayback();
		}
		return;
	}

	PlaybackPhaseElapsed += Dt;
	PlaybackElapsedTotal += Dt;
	SkillCalloutClock += Dt; // #4025: stesso passo del totale, ma sopravvive alla finestra di reazione (`SkillCalloutClock`)

	// ⛔ **Lo `Step` si ferma AL confine, mai oltre.** Il tick puo' superarlo — il passo dipende dal frame
	// rate — quindi si riporta indietro l'orologio della fase al confine esatto: senza, la stessa
	// pressione fermerebbe il playback in punti diversi su macchine diverse, e due `Step` di fila
	// finirebbero per saltare una barriera.
	if (PlaybackStepTargetElapsed >= 0.f && PlaybackPhaseElapsed >= PlaybackStepTargetElapsed)
	{
		// `PlaybackElapsedTotal` si corregge dello stesso scarto: e' il tempo mostrato a chi guarda, e
		// lasciarlo avanti direbbe che la riproduzione e' durata piu' di quanto si e' visto. L'orologio del fumetto
		// (#4025) dello stesso scarto, per la stessa ragione: uno `Step` lo fa avanzare di cio' che si e' mostrato (R4).
		const float ScartoOltreIlConfine = PlaybackPhaseElapsed - PlaybackStepTargetElapsed;
		PlaybackElapsedTotal -= ScartoOltreIlConfine;
		SkillCalloutClock -= ScartoOltreIlConfine;
		PlaybackPhaseElapsed = PlaybackStepTargetElapsed;
		PlaybackStepTargetElapsed = -1.f;
		bPlaybackPaused = true;
	}

	// #4025: il fumetto della skill vive del tempo del playback, e questo e' il punto in cui `SkillCalloutClock` e'
	// definitivo per il tick — DOPO la correzione dello `Step`, che puo' riportarlo indietro. Il tick in pausa e'
	// uscito all'inizio, quindi in pausa l'orologio del fumetto non viene portato: e' la regola, non un caso da gestire.
	SyncSkillCalloutClocks();

	const ERTMatchPhase Ph = PlaybackPhases[PlaybackPhaseIdx];
	const float PhaseDur = DurationForPlaybackPhase(Ph);

	if (Ph == ERTMatchPhase::Dash || Ph == ERTMatchPhase::Move || Ph == ERTMatchPhase::Blast)
	{
		// Movimento in PARALLELO: i cilindri di QUESTA fase partono insieme.
		//
		// 🔑 **In parallelo significa partire insieme, non finire insieme** (`#2370`). Fino al 2026-09-05
		// qui c'era UN solo `Alpha` — `PlaybackPhaseElapsed / PhaseDur` — condiviso da tutte le anim. Poiche'
		// `PhaseDur` vale il percorso PIU' LUNGO e `InterpolateAlongPath` distribuisce `Alpha` sull'INTERO
		// percorso, 2 celle e 10 celle arrivavano nello stesso istante: chi ne aveva `S` in una fase da `M`
		// si muoveva a `CellsPerSecond * S/M`, cioe' cinque volte piu' lento del rate base dichiarato.
		// Era la durata target a decidere la velocita' visuale — l'invariante che `#1878` nega — su un
		// canale che quella issue non aveva guardato.
		//
		// ⚠️ **Il `Blast` resta sull'`Alpha` di fase, e resta di proposito.** Li' la durata vale
		// `Max(Max(1, N) x AttackShowSeconds, spinta)` con `N` la lunghezza della SEQUENZA per intento (#3549, D5),
		// e NON `MaxSeg / rate`: la spinta del knockback si distende sulla finestra della sequenza, ed e'
		// documentato come deliberato in `URTPlaybackLibrary.h`.
		// ⏱️ *Fino a #3549 questa riga parlava di tre canali paralleli (colpi, muri, impronte) e del `Max` fra
		// loro.* ⚠️ Ne segue cio' che la spec dichiara (§6): la sequenza e' piu' lunga del piu' lungo dei canali,
		// quindi la spinta rallenta ancora, e la puo' allungare un intento che con l'unita' spinta non ha rapporto.
		// Cambiarlo e' una decisione separata con la sua evidenza, non un effetto collaterale di questa.
		// 🔑 Nel Dash, invece, le rotte partono DOPO l'anticipo delle attivazioni (`FRTPhaseTime::Lead`).
		const bool bAlphaPerPercorso = (Ph != ERTMatchPhase::Blast);
		const float AlphaFase = (PhaseDur > 0.f) ? FMath::Clamp(PlaybackPhaseElapsed / PhaseDur, 0.f, 1.f) : 1.f;

		// Il confine di micro-step (`#2876`): l'indice di cella su cui ogni anim si trovava al fotogramma
		// scorso, per accorgersi di quando ne attraversa uno nuovo.
		if (PlaybackAnimCellIndex.Num() != MoveAnims.Num())
		{
			PlaybackAnimCellIndex.Init(0, MoveAnims.Num());
		}
		if (PlaybackAnimEntryYaw.Num() != MoveAnims.Num())
		{
			PlaybackAnimEntryYaw.Init(TNumericLimits<float>::Max(), MoveAnims.Num());
			PlaybackAnimArrived.Init(false, MoveAnims.Num());
		}
		bool bAttraversatoUnConfine = false;

		// 🔑 **Un alpha per anim, calcolato UNA volta per fotogramma** (`#3519`): lo leggono la posizione, la posa
		// e la corsa di ciascuna, e la corsa di un'unita' chiede anche quelli delle sue ALTRE anim della fase.
		// Calcolarlo due volte darebbe due orologi sulla stessa animazione.
		//
		// I segmenti sono quelli che l'anim DISEGNA, non quelli del percorso reale: `World` e' gia' troncato al
		// prefisso osservabile (`ObservedPrefixLength`), e leggerlo qui tiene la presentazione dalla parte giusta
		// del confine di privacy.
		//
		// #3549: nel Dash le rotte partono DOPO le attivazioni (spec §2.4). `RouteAlpha` clampa un tempo negativo
		// a zero: durante le attivazioni il cilindro resta sulla cella di partenza.
		// L'anticipo lo possiede `PhaseTime` (`FRTPhaseTime::Lead`): la stessa formula che dimensiona la fase.
		const float AnticipoAttivazioni = PhaseTimeForPlaybackPhase(Ph).Lead;
		TArray<float, TInlineAllocator<16>> AlphaAnim;
		AlphaAnim.SetNumUninitialized(MoveAnims.Num());
		for (int32 AnimIdx = 0; AnimIdx < MoveAnims.Num(); ++AnimIdx)
		{
			// `#3666`: un'anim che un'estensione ha aggiunto alla fase in corso corre alla velocita' di rotta dal
			// proprio inizio, in ogni fase — Blast compreso, dove l'alpha di fase e' gia' consumato.
			AlphaAnim[AnimIdx] = (MoveAnims[AnimIdx].StartElapsed > 0.f)
				? URTPlaybackLibrary::RouteAlpha(MoveAnims[AnimIdx].World.Num() - 1,
					PlaybackPhaseElapsed - MoveAnims[AnimIdx].StartElapsed, Tuning().PlaybackCellsPerSecond)
				: bAlphaPerPercorso
				? URTPlaybackLibrary::RouteAlpha(MoveAnims[AnimIdx].World.Num() - 1,
					PlaybackPhaseElapsed - AnticipoAttivazioni, Tuning().PlaybackCellsPerSecond)
				: AlphaFase;
		}

		for (int32 AnimIdx = 0; AnimIdx < MoveAnims.Num(); ++AnimIdx)
		{
			const FRTMoveAnim& A = MoveAnims[AnimIdx];
			if (A.Phase == Ph && A.Unit.IsValid())
			{
				const float Alpha = AlphaAnim[AnimIdx];
				A.Unit->SetVisualLocation(URTPlaybackLibrary::InterpolateAlongPath(A.World, Alpha));

				// 🔑 **Arrivata, smette di correre** (`#3519`). `EnterPlaybackPhase` accende la corsa a tutte le
				// unita' della fase e la spegne alla fase dopo: finche' tutte arrivavano insieme, a fine fase, era
				// giusto. Da `#2370` ogni rotta procede alla propria velocita', e chi ha la piu' corta arriva prima
				// — e correva sul posto finche' non arrivava la piu' lunga, col corpo fermo sull'ultimo `Lean`.
				// ⛔ **Per livello, a ogni fotogramma, e non con un evento all'arrivo**: copre anche l'estensione a
				// meta' turno (`bPreserveClock`, [D-355]), che salta `EnterPlaybackPhase`. Il `Blast` resta com'e':
				// li' l'alpha e' di fase, e la spinta si distende sull'intera finestra.
				if (bAlphaPerPercorso)
				{
					bool bInCorsa = false;
					// #3549: durante l'anticipo delle attivazioni lo scattatore e' fermo sul punto di partenza mentre
					// suona il cast — `RouteAlpha` lo tiene a zero — e NON corre: la corsa parte con la rotta.
					const bool bInAnticipo = PlaybackPhaseElapsed < AnticipoAttivazioni;
					for (int32 Altra = 0; Altra < MoveAnims.Num() && !bInCorsa; ++Altra)
					{
						bInCorsa = !bInAnticipo && MoveAnims[Altra].Phase == Ph && MoveAnims[Altra].Unit == A.Unit
							&& AlphaAnim[Altra] < 1.f;
					}
					if (A.Unit->bIsMovingVisually && !bInCorsa)
					{
						// La posa torna a riposo INSIEME al flag, come in `EnterPlaybackPhase`.
						A.Unit->ResetGraykitPose();
					}
					A.Unit->bIsMovingVisually = bInCorsa;
				}

				// 🔑 **La posa segue il passo** ([D-462] punto 2, `#2167`): a ogni confine guarda l'ultimo passo
				// compiuto, come `FacingAtMicroStep`, e si gira verso il nuovo all'inizio di ciascuno. ⛔ Solo per chi
				// cammina o scatta: una spinta del Blast non orienta chi la subisce. ⏱️ *Fino a `#2167` la mesh restava
				// girata com'era per tutto il percorso, e a fine playback scattava sul facing logico.*
				if (bAlphaPerPercorso && A.World.Num() >= 2 && PlaybackAnimEntryYaw.IsValidIndex(AnimIdx))
				{
					if (PlaybackAnimEntryYaw[AnimIdx] == TNumericLimits<float>::Max())
					{
						PlaybackAnimEntryYaw[AnimIdx] = A.Unit->GetActorRotation().Yaw;
					}
					if (!PlaybackAnimArrived[AnimIdx])
					{
						A.Unit->SetActorRotation(FRotator(0.f, URTPlaybackLibrary::StepYawAtAlpha(
							A.World, Alpha, PlaybackAnimEntryYaw[AnimIdx], Tuning().StepTurnFraction), 0.f));
						if (Alpha >= 1.f)
						{
							PlaybackAnimArrived[AnimIdx] = true;
							// [D-462] punto 3: arrivata, si gira SUL POSTO verso il verso finale. Solo dopo la sua
							// ULTIMA anim: dopo uno scatto seguito da un Move il pivot appartiene al Move.
							bool bUltima = true;
							for (int32 Dopo = AnimIdx + 1; Dopo < MoveAnims.Num(); ++Dopo)
							{
								if (MoveAnims[Dopo].Unit == A.Unit) { bUltima = false; break; }
							}
							if (bUltima)
							{
								StartPresentationPivot(A.Unit.Get(), FinalFacingYaw(A.Unit.Get()));
							}
						}
					}
				}

				// 🔑 **Su quale cella e' ADESSO** (`#2876`). `Alpha` copre l'intero percorso disegnato,
				// quindi l'indice e' la sua frazione sui segmenti — la stessa aritmetica che
				// `InterpolateAlongPath` usa per scegliere il segmento, letta come intero.
				//
				// ⚠️ `A.Cells` e non la rotta reale: e' gia' troncato al prefisso osservabile, e leggerlo
				// qui tiene il velo dalla parte giusta del confine di privacy — come `A.World` sopra.
				//
				// 🔴 **Si chiede a `MicroStepAtAlpha`, e fino a `#3458` era un floor nudo.** `Step` ferma
				// l'orologio sul confine, dove `Alpha * Segmenti` dovrebbe valere un intero esatto; in virgola
				// mobile puo' valerne uno appena SOTTO, e il floor assegnava la cella PRECEDENTE. L'unita' era
				// disegnata al posto giusto — `InterpolateAlongPath` e' continua, e a un confine `k - ε` e `k`
				// danno lo stesso punto — mentre velo e rivelazione delle unita' leggevano la cella di un passo
				// indietro. Misurato da `Veil.RevealsAtTheMicroStepAtAnyPlaybackRate`: su 5 segmenti cadevano
				// 1.15, 1.95, 2.30 e 2.95 celle/s, e al primo passo B1 non compariva. Al default `1.44` ci cade
				// il confine 7 dei percorsi da 7 a 11 segmenti (`Playback.MicroStepAtAlphaLandsOnTheBoundary…`
				// li stampa); su 5 segmenti no.
				if (A.Cells.Num() > 0)
				{
					const int32 Segmenti = FMath::Max(A.Cells.Num() - 1, 1);
					const int32 Idx = FMath::Clamp(
						URTPlaybackLibrary::MicroStepAtAlpha(Alpha, Segmenti), 0, A.Cells.Num() - 1);
					if (PlaybackAnimCellIndex.IsValidIndex(AnimIdx) && PlaybackAnimCellIndex[AnimIdx] != Idx)
					{
						PlaybackAnimCellIndex[AnimIdx] = Idx;
						bAttraversatoUnConfine = true;
					}
				}

				// Posa graykit (#2880): il corpo si deforma attorno al punto dove `SetVisualLocation` lo ha
				// appena messo.
				//
				// 🔑 **Si riusa `Alpha`, non se ne calcola un altro.** E' il tempo normalizzato di QUESTA
				// unita' su QUESTO percorso — per percorso nel Move/Dash (#2370), di fase nel Blast — e
				// ricalcolarlo qui produrrebbe due orologi sulla stessa animazione, sfasati al primo cambio
				// di velocita' di playback.
				//
				// ⚠️ **L'andatura la decide `URTPresentationBindingLibrary`, non questo ciclo** (#2881). Fino
				// al 2026-09-10 la scelta era scritta qui inline, dichiarata temporanea: era il TurnManager a
				// rispondere a «cosa significa questa fase», che e' una domanda di [D-278].
				//
				// 🔑 Si passa `A.Phase` e non `Ph`: il ramo li rende uguali (`A.Phase == Ph` e' la condizione
				// del filtro), ma il primo e' il dato che appartiene all'ANIM — cioe' all'azione — mentre il
				// secondo e' lo stato del riproduttore. Se un giorno il ciclo smettesse di filtrare per fase,
				// questa riga resterebbe giusta.
				// `#2881`: la fase dice COSA sta facendo, gli stati dicono in che condizione lo fa. Entrambi
				// vengono dall'ANIM — cioe' dall'evento risolto — e ⛔ **nessuno dei due da `A.Unit`**, che e'
				// qui accanto e al playback direbbe lo stato di ADESSO invece di quello dell'azione.
				// ⛔ **Non dopo l'arrivo** (`#3519`): l'alpha resta fermo a `1`, e riapplicare la posa conclusa
				// rimetterebbe il `Lean` che il reset qui sopra ha appena tolto.
				// #3549: e nemmeno la posa di corsa durante l'anticipo — l'alpha e' zero, ma la posa a zero e' gia'
				// un passo accennato.
				const bool bPosaInAnticipo = bAlphaPerPercorso && PlaybackPhaseElapsed < AnticipoAttivazioni;
				if ((!bAlphaPerPercorso || Alpha < 1.f) && !bPosaInAnticipo)
				{
					const ERTGraykitLocomotionStyle Style =
						URTPresentationBindingLibrary::StyleForMovement(A.Phase, A.SourceStatusNames);
					A.Unit->ApplyGraykitPose(URTGraykitLibrary::Evaluate(
						URTGraykitLibrary::DescriptorForStyle(Style), Alpha));
				}
			}
		}

		// 🔑 **Il velo puo' seguire il movimento** (`#2876`). Solo sui confini: fra due di essi le pose sono
		// le stesse celle, e ricalcolare darebbe lo stesso insieme.
		//
		// ⛔ **Non e' un terzo punto di refresh della conoscenza CANONICA**: `AdvancePlaybackKnowledge`
		// scrive `PlaybackKnowledgeState`, che e' presentazione e non entra ne' nello snapshot ne' nel
		// `TurnLog` ne' nello `StateHash`. `OnTeamKnowledgeRefreshed` non emette, quindi
		// `Veil.FollowsRefreshPoints` resta verde per costruzione.
		if (bAttraversatoUnConfine && Host->AdvancePlaybackKnowledge())
		{
			Signals.OnPlaybackStepAdvanced->Broadcast();
		}
	}
	// #3549: Prep e Dash svelano le proprie attivazioni una per volta, con lo stesso ritmo dei colpi.
	// ⚠️ **Il `return` e' lo stesso del Blast, e per la stessa ragione** (`#3292`): dopo questo blocco il tick
	// prosegue con la finalizzazione della fase, quindi uscire senza uscire dal tick farebbe durare la fermata zero.
	// 🧪 `bRevealActivationsOnlyAtPhaseEndForTest` salta questo ramo: simula una fase accorciata, l'unico caso in cui la
	// rete di fine fase qui sotto recupera qualcosa (`Playback.PhaseEndNetStopsWithThePause`).
	if ((Ph == ERTMatchPhase::Prep || Ph == ERTMatchPhase::Dash) && !Tuning().bRevealActivationsOnlyAtPhaseEndForTest)
	{
		// #3578: la cue d'attivazione si consegna a OGNI uscita di questo ramo — il `return` delle fermate compreso —
		// come il tracer nel Blast. ⚠️ E' il primo ramo di Prep/Dash che consegna qualcosa alla mappa.
		ON_SCOPE_EXIT{ PushPlaybackCues(Ph); };
		const TArray<FRTResolvedEvent>& Attivazioni =
			(Ph == ERTMatchPhase::Prep) ? PlaybackActivationsPrep : PlaybackActivationsDash;
		if (RevealPlaybackActivations(Attivazioni,
			URTPlaybackLibrary::AttacksToShow(Attivazioni.Num(), PlaybackPhaseElapsed, Tuning().AttackShowSeconds)))
		{
			return;
		}
	}
	// ⚠️ `if`, NON `else if`: il Blast fa DUE cose insieme — scivolare (knockback, sopra) e rivelare. Con l'else
	// il secondo ramo era irraggiungibile, perche' il primo cattura gia' Blast, e AttackShowSeconds non aveva
	// alcun effetto: i colpi uscivano tutti nello stesso frame dal blocco di finalizzazione (#911).
	if (Ph == ERTMatchPhase::Blast)
	{
		// `#2454`: il tracer si consegna a OGNI uscita di questo ramo — il `return` delle fermate compreso — o a
		// una fermata resterebbe disegnato nella posizione del tick prima. In pausa il tick non arriva qui
		// (`bPlaybackPaused`, in testa a `TickPlayback`), e il tracer resta fermo dove l'ultima consegna lo ha
		// lasciato.
		// ⚠️ Lo scope e' questo `if`, non la funzione: la finalizzazione della fase, piu' sotto, gira DOPO la
		// consegna — ed e' li' che il canale si spegne.
		ON_SCOPE_EXIT{ PushPlaybackTracers(); PushPlaybackCues(Ph); }; // #3578: due consegne per tick (R10)

		// 🔴 **Una sequenza, un cursore, una cadenza** (#3549 D5, `#2454`): attivazione, impronta, muri, colpi di un
		// intento, poi il successivo — un elemento ogni `AttackShowSeconds`, per leggibilita' del danno. L'impronta
		// PRECEDE i suoi colpi e l'attivazione precede tutto il suo atto: lo decide la sequenza. Un colpo PARTE
		// quando il suo elemento esce e ARRIVA dopo il volo, prima dell'elemento dopo: lancio e arrivo sono due
		// battiti dello stesso cursore, monotoni perche' `TracerFlightFor` taglia il volo a `A/2`.
		// ⛔ Due cicli separati — la sequenza da una parte, gli arrivi dall'altra — in un tick lungo rivelerebbero
		// l'elemento dopo prima che l'arrivo del colpo fermi `Next Action`: `Playback.AttackBeatsStayOrderedInOneTick`.
		// ⚠️ **Stessa funzione di ritmo, non un tempo suo**: con voli nulli `AttackBeatsDue` rivela l'elemento `k` a
		// `k·A` come `AttacksToShow`, il contro-termine di `PhaseTime` che dimensiona il Blast sulla stessa sequenza.
		// ⏱️ *Fino a #3549 tre canali paralleli con contatori propri, e il `Max` fra loro; fino al merge di `#2454`
		// due cursori, la sequenza su `AttacksToShow` e i colpi su `AttackBeatsDue`.*
		// 🔴 **`return` e non `break`, e la differenza e' un difetto vero** (`#2855`): uscendo solo dal ciclo, un
		// fatto che cade nell'ultimo tick della fase metterebbe in pausa e la finalizzazione qui sotto avanzerebbe
		// lo stesso. Uscire dal tick lascia la fase dov'e'; la finalizzazione la fara' il primo tick dopo la ripresa.
		if (AdvanceBlastSequence(URTPlaybackLibrary::AttackBeatsDue(
			PlaybackPhaseElapsed, Tuning().AttackShowSeconds, PlaybackBlastFlights)))
		{
			return;
		}
	}

	if (PlaybackPhaseElapsed >= PhaseDur)
	{
		// Finalizza la fase corrente.
		if (Ph == ERTMatchPhase::Dash || Ph == ERTMatchPhase::Move || Ph == ERTMatchPhase::Blast)
		{
			for (const FRTMoveAnim& A : MoveAnims)
			{
				if (A.Phase == Ph && A.Unit.IsValid() && A.World.Num() > 0)
				{
					A.Unit->SetVisualLocation(A.World.Last());
				}
			}
		}
		// Chi non ha fatto in tempo a comparire compare adesso: una fase non deve PERDERE un fatto, deve solo
		// mostrarlo piu' in fretta.
		//
		// 🔴 **E' una RETE SENZA CASI** (`#3277`), e non si toglie. `PhaseTime` dimensiona il Blast su
		// `Max(1, N) x AttackShowSeconds` con `N` la lunghezza della SEQUENZA (#3549), e Prep e Dash su
		// `N x AttackShowSeconds` piu' le loro altre voci: a fine fase `AttacksToShow` vale gia' `N`, e le righe qui
		// sotto non recuperano nulla. ⛔ Il recupero e' silenzioso per costruzione: se qualcuno accorciasse una fase,
		// la rete tornerebbe necessaria e nessun rosso lo direbbe. Per il Blast l'invariante che la rende inutile e'
		// pinnato da `Playback.EveryChannelIsFullyRevealedByPhaseEnd` (le rivelazioni) e da
		// `Playback.EveryAttackArrivesByPhaseEnd` (gli arrivi, `#2454`), che cadono **prima**; per Prep e Dash vale la
		// stessa aritmetica (`Shown >= N x AttackShowSeconds`), e nessun gate la pinna.
		//
		// ⚠️ **Dichiarato (spec §6): il recupero del Blast ora fa due cose che prima non faceva.** Scrive anche le
		// righe `Colpo:` — ⏱️ *il vecchio ciclo di recupero suonava ruoli, token e broadcast SENZA la riga del feed* —
		// e passa dal predicato di confine (`NotePlaybackActShown`), quindi un `Next Action` armato puo' fermarsi
		// anche qui.
		// 🔴 **Prep e Dash escono dal tick se la rete mette in pausa, come il Blast** (review della PR #3561):
		// `RevealPlaybackActivations` dice `true` quando si ferma su un confine, e proseguendo il passaggio alla fase
		// dopo — `EnterPlaybackPhase` azzera `ActivationsShown` — avrebbe perso le attivazioni ancora in coda. Uscire
		// lascia la fase dov'e': al primo tick dopo la ripresa la rete riprende dal contatore. ⏱️ *La stesura di #3549
		// ignorava il ritorno e dichiarava saltati gli elementi in coda.* Su un turno normale non accade, perche' la
		// rete e' senza casi; lo forza `bRevealActivationsOnlyAtPhaseEndForTest`.
		if (Ph == ERTMatchPhase::Prep || Ph == ERTMatchPhase::Dash)
		{
			const TArray<FRTResolvedEvent>& Attivazioni =
				(Ph == ERTMatchPhase::Prep) ? PlaybackActivationsPrep : PlaybackActivationsDash;
			if (RevealPlaybackActivations(Attivazioni, Attivazioni.Num()))
			{
				return;
			}
			// #3578: nessuna cue sopravvive alla fase (spec §2.4). ⚠️ Coi tetti di R2 qui il canale e' gia' vuoto: e' la
			// rete del caso in cui una fase venga accorciata.
			if (ARTHexMapActor* const CueMap = ARTHexMapActor::FindInWorld(Host->GetPlaybackWorld()))
			{
				CueMap->ClearPlaybackCues();
			}
			bPlaybackCueChannelFull = false;
		}
		if (Ph == ERTMatchPhase::Blast)
		{
			// 🔑 **Un punto di uscita, in quest'ordine: mostra il resto → consegna gli arrivi → spegni il canale**
			// (merge di `#2454` in #3549). Rivelazioni e arrivi sono battiti dello STESSO cursore, quindi la rete li
			// percorre insieme nell'ordine `L, A` del ciclo: un elemento mai rivelato riceve rivelazione (per un colpo
			// il lancio) e arrivo; un colpo lanciato e non arrivato riceve SOLO l'arrivo — ⛔ mai un secondo ruolo
			// `Attack` sull'attaccante.
			// ⚠️ Coi battiti resta una rete senza casi: l'ultimo arrivo cade entro `(N - ½)·A`, prima della fine della
			// fase — lo pinna `Playback.EveryAttackArrivesByPhaseEnd`, sulla durata REALE di `PhaseTime` e sulla
			// sequenza; `EveryChannelIsFullyRevealedByPhaseEnd` pinna solo le rivelazioni (`(N - 1)·A`).
			// 🔴 **Si esce se la rete si ferma**, ed e' un cambio rispetto alle due stesure fuse: un ascoltatore di
			// `OnAttackResolved` puo' aver chiuso il playback (la guardia di `#2454`), oppure un `Next Action` armato
			// si e' fermato su un confine — e allora la fase resta dov'e', e la rete riprende dal cursore al primo
			// tick dopo la ripresa. ⏱️ *La stesura di #3549 proseguiva alla fase dopo e dichiarava saltati gli
			// elementi in coda; con gli arrivi in coda avrebbe saltato un `Hit` e il suo numero.*
			if (AdvanceBlastSequence(2 * PlaybackBlastSequence.Num()))
			{
				return;
			}

			// `#2454`: nessun tracer sopravvive alla fase. ⚠️ Qui e non solo in `FinishPlayback`, che esce presto
			// quando e' trattenuto da una finestra di reazione.
			if (ARTHexMapActor* const TracerMap = ARTHexMapActor::FindInWorld(Host->GetPlaybackWorld()))
			{
				TracerMap->ClearPlaybackTracers();
				TracerMap->ClearPlaybackCues(); // #3578: nessuna cue sopravvive al Blast
			}
			bPlaybackTracerChannelFull = false; // vedi `PushPlaybackTracers`: il flag segue il canale
			bPlaybackCueChannelFull = false;
		}

		// Morte visiva differita: l'eliminazione si ANNUNCIA qui, a fine della fase in cui e' avvenuta, dopo
		// che il colpo (Blast) o l'attraversamento (Move) e' stato mostrato.
		//
		// 🔴 **La sparizione non avviene piu' qui, e la ragione e' misurata** (#2452). `HideForDefeat()`
		// chiama `SetActorHiddenInGame(true)`, che propaga a TUTTI i componenti — skeletal compresa.
		// Chiamarlo la riga prima di `PlayDefeatMontage()` faceva partire il montaggio su un attore gia'
		// nascosto: `Death` non veniva **mai** disegnato, e la terza clausola di `PIE-AS4b` non era
		// raggiungibile a nessuna qualita' degli asset. Ora nasconde un solo punto — `FinishPlayback` — e il
		// montaggio ha per finestra le fasi che restano.
		//
		// ⚠️ **Il limite residuo e' dichiarato, non nascosto**: chi cade nell'ULTIMA fase riprodotta ha una
		// finestra di durata zero, perche' `FinishPlayback` segue immediatamente. `PlaybackPhases` contiene
		// `Prep -> Dash -> Blast -> Move` e mai `Cleanup`, quindi il caso comune (morte nel Blast con un Move
		// dopo) e' coperto e quello di coda no. Chiuderlo richiede un'attesa a fine playback: e' la parte di
		// #2452 che resta aperta.
		//
		// ⚠️ **La guardia non puo' piu' essere `IsHidden()`** — era l'hide stesso a renderla idempotente.
		// Il marcatore e' `PlaybackDefeatShown`, altrimenti il catch-all di `FinishPlayback` rifarebbe
		// partire il montaggio e ribroadcasterebbe `OnUnitDefeated` sulla stessa unita'.
		//
		// ⛔ **Nulla di logico si muove**: `HP=0` lo ha gia' deciso il resolver (`ApplyCombatState`) e la
		// distruzione resta in `ConcludeTurn`. La presentazione non decide — invariante #1.
		bool bMorteAnnunciataInQuestaFase = false;
		for (const FRTResolvedEvent& D : PlaybackDefeated)
		{
			ARTUnit* const DefU = Host->UnitByStableId(D.SourceStableUnitId);
			if (D.Phase == Ph && DefU && !PlaybackDefeatShown.Contains(D.SourceStableUnitId))
			{
				PlaybackDefeatShown.Add(D.SourceStableUnitId);
				bMorteAnnunciataInQuestaFase = true;
				Host->AddLogEvent(FString::Printf(TEXT("Morte mostrata: %s"), *DefU->GetName()), FRTLogSubject::World());
				DefU->PlayPresentationRole(ERTPresentationRole::Death);
				Signals.OnUnitDefeated->Broadcast(DefU);
			}
		}

		++PlaybackPhaseIdx;
		if (PlaybackPhaseIdx >= PlaybackPhases.Num())
		{
			// 🔴 **La coda della morte** (#2452). Se l'ULTIMA fase si e' appena chiusa su un'eliminazione, il
			// montaggio `Death` avrebbe finestra ZERO: non c'e' nessuna fase dopo in cui giocarlo, e
			// `PlaybackPhases` non contiene mai `Cleanup`. Il playback tiene ancora `DefeatBeatSeconds`.
			//
			// ⛔ Aspetta una DURATA, non il montaggio: nessuna callback di animazione, nessun
			// `IsPlaying()`. Un `Death` assente non allunga e non accorcia niente, e `DefeatBeatSeconds = 0`
			// riporta al comportamento precedente.
			if (bMorteAnnunciataInQuestaFase && Tuning().DefeatBeatSeconds > 0.f)
			{
				PlaybackDefeatBeatRemaining = Tuning().DefeatBeatSeconds;
				return;
			}
			FinishPlayback();
			return;
		}
		EnterPlaybackPhase();

		// `#2855`: il predicato armato consuma QUESTO confine.
		//
		// 🔑 **Dopo `EnterPlaybackPhase` e non prima**, ed e' il criterio d'accettazione alla lettera:
		// fermandosi qui `GetPlaybackPhaseName()` nomina gia' la fase NUOVA, che e' cio' che chi ha chiesto
		// *«portami alla prossima fase»* si aspetta di leggere. Fermarsi una riga sopra lo lascerebbe in
		// quella vecchia, a guardare un'immagine che dice il contrario del comando che ha premuto.
		//
		// 🔑 **Vale anche per `NextAction`, e non e' un ripiego.** Il tempo del playback scorre per fase, e
		// un cambio di fase e' sempre anche un cambio d'atto: la fase `Move` contiene il solo `Action.Move`,
		// e nessun atto attraversa due fasi. Un `Next Action` armato durante il `Move` non ha colpi da
		// aspettare — il suo prossimo atto E' la fase seguente.
		//
		// ⛔ **Senza un predicato armato non si ferma niente**: e' la riga che tiene questo comando dalla
		// parte giusta di [D-355], che vieta a una fase di acquisire una ragione di fermarsi *per
		// simmetria*.
		if (PlaybackStopAt != ERTPlaybackStopAt::None)
		{
			// ⚠️ Le stesse quattro righe di ogni confine d'atto (`#3292`, `NotePlaybackActShown`), e passano
			// dallo stesso helper: il nome
			// regge perche' il commento qui sopra lo argomenta — un cambio di fase **e'** un cambio d'atto.
			PausePlaybackAtActBoundary();
		}
	}
}

void FRTPlaybackDirector::FinishPlayback()
{
	// 🔴 **Il playback puo' finire mentre il turno NON e' finito** (`#2679` fetta 3, [D-355]). Il playback
	// parziale mostra il tratto percorso fino al boundary — pochi secondi — mentre la finestra ne dura
	// `FastReactionDuration`: senza questo controllo `ConcludeTurn` chiuderebbe il turno sotto chi sta
	// ancora decidendo, e la sua risposta arriverebbe a partita gia' avanzata.
	//
	// ✅ **Fermarsi qui e' anche la lettura giusta**: ADR-0004 §5 dice che chi guarda *«vede il mondo
	// fermarsi su un momento di tensione»*. Il mondo si ferma sull'ultimo fotogramma mostrato, che e'
	// esattamente l'istante in cui il nemico e' entrato nella zona.
	if (Host->IsPlaybackResolutionSuspended())
	{
		// `#3666` (review di #3675): uno Skip prima del trattenimento salta l'AVVICINAMENTO, e la finestra si presenta
		// adesso. Chi la riceve deve trovare lo schermo sull'ingresso, non a meta' strada: ogni anim va sul suo ultimo
		// punto, che e' il confine del tratto parziale. ⛔ `World` e' gia' troncato al prefisso osservato, quindi
		// l'ultimo punto non rivela niente che il playback non avrebbe mostrato.
		if (bFinishingBySkip)
		{
			for (const FRTMoveAnim& A : MoveAnims)
			{
				if (A.Unit.IsValid() && A.World.Num() > 0)
				{
					A.Unit->SetVisualLocation(A.World.Last());
					A.Unit->bIsMovingVisually = false;
				}
			}
			// 🔴 **E l'orologio dove il trattenimento naturale lo lascia**: oltre l'ultima fase parziale, alla fine di
			// quella fase. Senza, alla risposta `BeginPlayback(true)` ripartiva dall'orologio di meta' avvicinamento, e
			// il mover tornava indietro a rifare il tratto saltato (seconda review di #3675).
			if (PlaybackPhases.Num() > 0)
			{
				PlaybackPhaseElapsed = DurationForPlaybackPhase(PlaybackPhases.Last());
				PlaybackPhaseIdx = PlaybackPhases.Num();
			}
			// La barra di avanzamento come al trattenimento naturale: in fondo al tratto parziale.
			PlaybackElapsedTotal = FMath::Max(PlaybackElapsedTotal, PlaybackTotalSeconds);
			// #4025: l'orologio del fumetto NO. Il tratto saltato non e' tempo che si e' visto (R4), quindi un fumetto
			// acceso resta dov'era e la finestra lo trova fermo (R7), come al trattenimento naturale.
			// ⚠️ **Limite dichiarato**: le rivelazioni del tratto saltato — attivazioni, colpi, impronte del Blast — non
			// escono, come in ogni Skip. Chi salta ha scelto di non guardare, e la finestra resta l'unico punto in cui il
			// gioco si ferma per lui. Con un `Brace`, quindi, la finestra si presenta senza aver mostrato il colpo che
			// spinge (seconda review di #3675).
		}
		bPlaybackHeldByWindow = true;
		// `#3666`: e' QUESTO il fotogramma in cui la finestra si presenta, e il suo countdown comincia.
		Host->PresentPendingReactionWindow();
		return;
	}

	Host->SetPlaybackResolving(false);
	Host->SetPlaybackTickEnabled(false);

	// `#2855`: **il predicato non sopravvive al turno che l'ha visto.** Armato durante l'ULTIMA fase
	// riprodotta, `Next Phase` non trova un confine da consumare — `PlaybackPhases` finisce — e arrivare
	// qui e' il suo esito legittimo. Ma lasciarlo armato fermerebbe il playback del turno SEGUENTE al suo
	// primo cambio di fase, senza che nessuno l'abbia chiesto: un predicato armato e mai soddisfatto blocca
	// l'osservazione senza dirlo, ed e' il rischio che `#2855` registra e chiude proprio qui.
	//
	// ⚠️ **Qui e non in `BeginPlayback`**, che ricomincia anche a META' turno (`bPreserveClock`, [D-355])
	// quando una finestra di reazione si e' chiusa: disarmare li' butterebbe un `Next Phase` chiesto prima
	// della finestra e mai ancora servito.
	PlaybackStopAt = ERTPlaybackStopAt::None;
	PlaybackStopFromAction = NAME_None;
	PlaybackStopFromSource = 0;
	PlaybackLastShownAction = NAME_None; // `#3292`: nemmeno l'atto in corso sopravvive al turno
	PlaybackLastShownSource = 0; // #3549: la coppia si azzera insieme

	// #4025 (spec del fumetto R7): **i fumetti delle skill si spengono QUI e non prima.** La posizione e' il punto:
	// il ramo `IsResolutionSuspended()` qui sopra esce con `return`, perche' una finestra di reazione e' la fine di un
	// PREFISSO del turno e non la fine del turno. Chi decide nella finestra deve ritrovare il fumetto dove l'ha
	// lasciato — acceso e FERMO, perche' `TickPlayback` non gira finche' il playback e' trattenuto — e spegnerlo li'
	// lo farebbe sparire proprio mentre il mondo si ferma su un momento di tensione. Si spegne alla fine VERA del turno
	// e passa di qui anche `SkipPlayback`, che chiama questa funzione direttamente.
	// ⚠️ `ClearSkillCallout` chiude la vita e lascia il registro (`LastSkillCalloutForTest`): e' lo scopo del campo.
	for (const TWeakObjectPtr<ARTUnit>& CalloutUnit : SkillCalloutUnits)
	{
		if (ARTUnit* const Unit = CalloutUnit.Get())
		{
			Unit->ClearSkillCallout();
		}
	}
	SkillCalloutUnits.Reset();
	// E il loro orologio, che e' arrivato alla fine del turno insieme a loro. ⛔ Solo qui e in `BeginPlayback(false)`:
	// il ramo sospeso qui sopra lo deve conservare, e `ConcludeResolution` non lo tocca (vedi `SkillCalloutClock`).
	SkillCalloutClock = 0.f;

	// #4033 (R8): **alla fine VERA ogni attivazione che il playback avrebbe mostrato a chi guarda ha la sua riga**, anche se
	// si e' saltato. Solo la riga: nessuna clip, nessun fumetto, nessuna riga `Attiva:` — chi salta ha scelto di non guardare,
	// e il Registro gli dice soltanto cosa e' partito. L'ordine e' quello del playback: Prep, Dash, poi la sequenza del Blast.
	// ⛔ Le fonti sono le liste GIA' filtrate per chi guarda (`BeginPlayback`, `BuildBlastSequence`), non la timeline: nessun
	// secondo giudizio di conoscenza (R2). Le gia' mostrate passano anche loro, e la chiave di `AddActivationLine` le scarta.
	// ⚠️ Qui e non nel ramo sospeso qui sopra: una finestra di reazione non e' la fine del turno, e alla ripresa il playback
	// mostrera' ancora il resto. Sta prima dell'azzeramento delle liste, piu' sotto.
	for (const TArray<FRTResolvedEvent>* Coda : { &PlaybackActivationsPrep, &PlaybackActivationsDash })
	{
		for (const FRTResolvedEvent& Ev : *Coda)
		{
			AddActivationLine(Ev, Host->UnitByStableId(Ev.SourceStableUnitId));
		}
	}
	for (const FRTBlastSequenceElement& E : PlaybackBlastSequence)
	{
		if (Timeline().IsValidIndex(E.TimelineIndex)
			&& Timeline()[E.TimelineIndex].Type == ERTResolvedEventType::AbilityActivated)
		{
			const FRTResolvedEvent& Ev = Timeline()[E.TimelineIndex];
			AddActivationLine(Ev, Host->UnitByStableId(Ev.SourceStableUnitId));
		}
	}
	// 🔴 **E la LISTA intera torna nell'ordine delle fasi del playback** (review finale di #4033, R8). Il recupero qui sopra e'
	// in ordine, ma accoda: un salto durante Prep/Dash di un playback PARZIALE (finestra di reazione nel Blast) passa dal ramo
	// sospeso, che non mostra niente; alla ripresa il playback mostra il Blast, e le righe di Prep e Dash nascevano qui DOPO le
	// sue. L'ordinamento e' STABILE: dentro una fase resta l'ordine di nascita, cioe' quello in cui il playback le ha mostrate o
	// il recupero le ha date. ⚠️ Si ordina per il valore di `ERTMatchPhase` perche' coincide con l'ordine del playback, misurato
	// per lettura: l'enum dichiara `Planning, Prep, Dash, Blast, Move, Cleanup`, `URTTurnRules::NextPhase` (il passo di
	// `RunPhaseLoop`) percorre le fasi in quell'ordine, e `BeginPlayback` costruisce `PlaybackPhases` Prep -> Dash -> Blast ->
	// Move. Una riga ha la fase della sua attivazione, che e' Prep, Dash o Blast (le tre code qui sopra).
	ActivationLines.StableSort([](const FRTActivationLine& A, const FRTActivationLine& B) { return A.Phase < B.Phase; });

	// 🔴 **E la PAUSA non sopravvive al turno, per la stessa ragione e per una via che `#2858` apre**
	// (rischio dichiarato in quella issue). `TickPlayback` esce prima di toccare qualunque cosa quando e'
	// in pausa, quindi un playback in pausa non puo' finire da solo: l'unica strada per arrivare qui con
	// il flag acceso e' `SkipPlayback()`, che salta il resto e chiama direttamente questa funzione.
	//
	// 🔎 Misurato su `00b751ce`: ne' questa funzione ne' `BeginPlayback` azzeravano `bPlaybackPaused`
	// (`grep -n "bPlaybackPaused" RTTurnManager.cpp` → nessuna scrittura in nessuna delle due). Il turno
	// SEGUENTE nasceva quindi in pausa, e il suo `TickPlayback` usciva al primo tick: l'osservazione si
	// fermava senza che nessuno l'avesse chiesto, e senza dirlo.
	//
	// ⚠️ **Era latente finche' `#1879` non aveva chiamanti di produzione** — nessuno poteva mettere in
	// pausa, quindi nessuno poteva saltare da fermo. `#2858` accende i comandi: la via si apre, e va
	// chiusa nello stesso commit che la apre.
	//
	// ⛔ **Qui e non in `BeginPlayback`**: quella ricomincia anche a META' turno (`bPreserveClock`,
	// [D-355]), e azzerare li' farebbe ripartire l'immagine sotto chi l'aveva messa in pausa prima di una
	// finestra di reazione.
	bPlaybackPaused = false;
	PlaybackStepTargetElapsed = -1.f;

	// Snap di sicurezza alle posizioni finali (la cella logica e' gia' quella finale).
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Unit.IsValid())
		{
			A.Unit->SetVisualLocation(A.Unit->WorldForCell(A.Unit->Cell, PBOrigin, PBCellSize, PBLayerHeight));
		}
	}
	// La mesh ATTERRA sul facing logico (CP 16.1). Durante il playback lo yaw interpola verso la direzione di
	// marcia, che e' presentazione; a fine risoluzione deve coincidere con il valore che le REGOLE useranno nel
	// turno dopo — altrimenti il giocatore legge un orientamento e ne subisce un altro quando arriva la difesa
	// direzionale (CP 16.2).
	//
	// Qui e non alla fine dell'animazione: `SkipPlayback` passa da questa funzione, quindi saltare la
	// risoluzione non deve lasciare la figura girata dalla parte sbagliata.
	{
		TArray<AActor*> AllUnitsForFacing;
		UGameplayStatics::GetAllActorsOfClass(Host->GetPlaybackWorld(), ARTUnit::StaticClass(), AllUnitsForFacing);
		for (AActor* UnitActor : AllUnitsForFacing)
		{
			ARTUnit* U = Cast<ARTUnit>(UnitActor);
			if (!U || !U->IsAlive()) { continue; }

			// [D-462] punto 3 (`#2167`): alla fine naturale la mesh ci arriva con un pivot animato, anche chi non si e'
			// mosso — una rotazione dichiarata da fermo, o un facing che il Blast ha cambiato. ⛔ Saltando, scatta:
			// chi salta chiede lo stato finale, non un'altra animazione.
			const float Verso = FinalFacingYaw(U);
			if (bFinishingBySkip)
			{
				PivotAnims.RemoveAll([U](const FRTPivotAnim& P) { return P.Unit.Get() == U; });
				U->SetActorRotation(FRotator(0.f, Verso, 0.f));
			}
			else
			{
				StartPresentationPivot(U, Verso);
			}
		}
	}

	// Fine della presentazione di morte. Da #2452 questo e' l'UNICO punto che nasconde, e le due cose che fa
	// sono distinte:
	//
	//   1. il **catch-all dell'annuncio** — chi e' caduto in una fase che non e' stata riprodotta (`Cleanup`
	//      non e' mai una fase di playback) o quando la risoluzione e' stata saltata. Qui il montaggio ha
	//      finestra ZERO: e' il limite residuo dichiarato in #2452;
	//   2. la **sparizione**, che vale per tutti — annunciati qui o in una fase precedente.
	//
	// ⚠️ L'ordine conta: annunciare prima di nascondere e' precisamente cio' che il difetto sbagliava.
	for (const FRTResolvedEvent& D : PlaybackDefeated)
	{
		ARTUnit* const DefU = Host->UnitByStableId(D.SourceStableUnitId);
		if (!DefU)
		{
			continue;
		}
		if (!PlaybackDefeatShown.Contains(D.SourceStableUnitId))
		{
			PlaybackDefeatShown.Add(D.SourceStableUnitId);
			Host->AddLogEvent(FString::Printf(TEXT("Morte mostrata: %s"), *DefU->GetName()), FRTLogSubject::World());
			DefU->PlayPresentationRole(ERTPresentationRole::Death);
			Signals.OnUnitDefeated->Broadcast(DefU);
		}
		if (!DefU->IsHidden())
		{
			DefU->HideForDefeat();
		}
	}

	MoveAnims.Reset();
	// Gli indici di cella seguono le anim a cui si riferiscono (`#2876`): sopravvivergli farebbe
	// leggere il confine di un'altra unita'.
	PlaybackAnimCellIndex.Reset();
	PlaybackAnimEntryYaw.Reset();
	PlaybackAnimArrived.Reset();
	PlaybackDefeated.Reset();
	PlaybackActivationsPrep.Reset();
	PlaybackActivationsDash.Reset();
	ActivationsShown = 0;
	// La sequenza si svuota QUI e non in `BeginPlayback`, che estendendo (D-355) la usa come `Previous`; con lei
	// il cursore dei battiti e i voli che le sono paralleli (`#2454`).
	PlaybackBlastSequence.Reset();
	BlastBeatsDone = 0;
	PlaybackBlastFlights.Reset();
	PlaybackBlastFootprintFx.Reset(); // #3578: parallelo alla sequenza, si svuota con lei
	// ⛔ **Il canale si spegne qui, e passa di qui anche `SkipPlayback`**: un'impronta che
	// sopravvivesse al turno sarebbe un'anteprima di qualcosa che non accadra' (`#2454`).
	if (ARTHexMapActor* const FootprintMap = ARTHexMapActor::FindInWorld(Host->GetPlaybackWorld()))
	{
		FootprintMap->ClearPlaybackFootprint();
		FootprintMap->ClearPlaybackTracers(); // `#2454`: e passa di qui anche `SkipPlayback`
		FootprintMap->ClearPlaybackCues(); // #3578: e passa di qui anche `SkipPlayback` — nessuna cue rigiocata (§2.6)
		// ⛔ **Anche i muri caduti si spengono qui, e passa di qui pure `SkipPlayback`** (`#2828`): il
		// segno e' il CAMBIAMENTO, e un cambiamento che sopravvive al turno torna a essere «lo stato dopo»
		// — cioe' quel che si vedeva prima che questo evento esistesse.
		FootprintMap->ClearPlaybackStructureHits();
	}
	bPlaybackTracerChannelFull = false; // `#2454`: il flag di `PushPlaybackTracers` segue il canale che si spegne qui
	bPlaybackCueChannelFull = false; // #3578: il flag segue il canale che si spegne qui
	PlaybackDefeatShown.Reset(); // l'annuncio e' per playback: il marcatore non sopravvive al round
	PlaybackDefeatBeatRemaining = 0.f; // e nemmeno la coda: `SkipPlayback` passa di qui e la scavalca
	PlaybackPhases.Reset();

	Host->AddLogEvent(FString::Printf(TEXT("Risoluzione completata (%.1fs)"), PlaybackElapsedTotal), FRTLogSubject::World());
	// Fine playback (anche via Skip, che passa di qui): nessuna unita' e' piu' in movimento -> l'AnimBP torna idle.
	{
		TArray<AActor*> AllUnits;
		UGameplayStatics::GetAllActorsOfClass(Host->GetPlaybackWorld(), ARTUnit::StaticClass(), AllUnits);
		for (AActor* UnitActor : AllUnits)
		{
			// Stessa ragione della fase: la posa non deve sopravvivere alla risoluzione che l'ha prodotta.
			if (ARTUnit* U = Cast<ARTUnit>(UnitActor)) { U->bIsMovingVisually = false; U->ResetGraykitPose(); }
		}
	}
	Signals.OnResolvePlaybackFinished->Broadcast();
	Host->ConcludeTurn();
}

void FRTPlaybackDirector::SkipPlayback()
{
	if (!Host->IsPlaybackResolving())
	{
		return;
	}
	Host->MarkPlaybackSkipped();
	Host->AddLogEvent(TEXT("Risoluzione: salto"), FRTLogSubject::World());
	bFinishingBySkip = true;
	FinishPlayback();
	bFinishingBySkip = false;
}

void FRTPlaybackDirector::StartPresentationPivot(ARTUnit* Unit, float ToYaw)
{
	if (!Unit)
	{
		return;
	}
	PivotAnims.RemoveAll([Unit](const FRTPivotAnim& P) { return P.Unit.Get() == Unit; });
	const float Da = Unit->GetActorRotation().Yaw;
	// Gia' girata, o pivot spento: nessuna animazione da far girare.
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(Da, ToYaw)) < 0.5f || Tuning().FinalPivotSeconds <= 0.f)
	{
		Unit->SetActorRotation(FRotator(0.f, ToYaw, 0.f));
		return;
	}
	FRTPivotAnim Pivot;
	Pivot.Unit = Unit;
	Pivot.From = Da;
	Pivot.To = ToYaw;
	PivotAnims.Add(Pivot);
}

void FRTPlaybackDirector::TickPresentationPivots(float DeltaSeconds)
{
	for (int32 I = PivotAnims.Num() - 1; I >= 0; --I)
	{
		FRTPivotAnim& Pivot = PivotAnims[I];
		ARTUnit* U = Pivot.Unit.Get();
		if (!U)
		{
			PivotAnims.RemoveAt(I);
			continue;
		}
		Pivot.Elapsed += DeltaSeconds;
		const float Progresso = (Tuning().FinalPivotSeconds > 0.f) ? Pivot.Elapsed / Tuning().FinalPivotSeconds : 1.f;
		U->SetActorRotation(FRotator(0.f, URTPlaybackLibrary::PivotYaw(Pivot.From, Pivot.To, Progresso), 0.f));
		if (Progresso >= 1.f)
		{
			PivotAnims.RemoveAt(I);
		}
	}
}

float FRTPlaybackDirector::FinalFacingYaw(const ARTUnit* Unit) const
{
	// Dalla geometria, come l'anteprima del controller: dal centro della cella al centro del vicino nel verso.
	const FVector Qui = Unit->WorldForCell(Unit->Cell, PBOrigin, PBCellSize, PBLayerHeight);
	const FVector Avanti = Unit->WorldForCell(URTHexLibrary::Neighbor(Unit->Cell, Unit->Facing), PBOrigin, PBCellSize,
		PBLayerHeight);
	return URTPlaybackLibrary::DirectionYaw(Qui, Avanti);
}

FRTPhaseTime FRTPlaybackDirector::PhaseTimeForPlaybackPhase(ERTMatchPhase InPhase) const
{
	// Le unita' si muovono in parallelo: alla durata serve il percorso PIU' LUNGO fra quelli riprodotti in
	// questa fase, non la loro somma. E' l'unico dato che il TurnManager possiede e la library no.
	int32 MaxSeg = 0;
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Phase == InPhase) { MaxSeg = FMath::Max(MaxSeg, A.World.Num() - 1); }
	}

	int32 NumAttivazioni = 0;
	if (InPhase == ERTMatchPhase::Prep) { NumAttivazioni = PlaybackActivationsPrep.Num(); }
	else if (InPhase == ERTMatchPhase::Dash) { NumAttivazioni = PlaybackActivationsDash.Num(); }

	// La formula sta in `URTPlaybackLibrary::PhaseTime`, dove si esercita senza mondo e senza Actor
	// (#1817). Qui resta la sola raccolta degli ingressi. ⏱️ *Fino a #3549 riceveva i conteggi dei canali
	// paralleli del Blast; ora le attivazioni di Prep/Dash e la lunghezza della sequenza di Blast.*
	FRTPhaseTime T = URTPlaybackLibrary::PhaseTime(InPhase, MaxSeg, NumAttivazioni, PlaybackBlastSequence.Num(),
		Tuning().PlaybackCellsPerSecond, Tuning().AttackShowSeconds, Tuning().PhaseBeatSeconds);

	// `#3666`: un'anim che un'estensione ha aggiunto a questa fase comincia a `StartElapsed` e corre alla velocita'
	// di rotta: la fase dura almeno fino al suo arrivo. ⚠️ In `Shown` e non in `Slack`: mostra un movimento, e il
	// budget non puo' toglierlo (la stessa classificazione di `PhaseTime`).
	if (Tuning().PlaybackCellsPerSecond > 0.f)
	{
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.Phase == InPhase && A.StartElapsed > 0.f)
			{
				T.Shown = FMath::Max(T.Shown,
					A.StartElapsed + FMath::Max(0, A.World.Num() - 1) / Tuning().PlaybackCellsPerSecond);
			}
		}
	}
	return T;
}

float FRTPlaybackDirector::DurationForPlaybackPhase(ERTMatchPhase InPhase) const
{
	const FRTPhaseTime T = PhaseTimeForPlaybackPhase(InPhase);

	// 🔑 **Il budget si applica QUI, e solo allo slack.** E' la differenza fra «la fase dura meno perche'
	// le attese sono state accorciate» e «i cilindri corrono di piu'»: `T.Shown` passa intatto, quindi il
	// tempo che il movimento ha per attraversare le sue celle non scende mai sotto
	// `MaxSeg / PlaybackCellsPerSecond`.
	//
	// ⚠️ **Le fasi che mostrano qualcosa hanno `Slack == 0`**, quindi per `Dash`, `Move` e `Blast` questa
	// riga restituisce la durata piena qualunque cosa faccia il budget. Non e' un caso fortunato: e' la
	// classificazione di `PhaseTime`, e ci si e' arrivati dopo che la prima stesura — che comprimeva il
	// tempo dei colpi — faceva uscire tutti i colpi in un frame e accelerava la spinta del knockback.
	return T.Shown + T.Slack * PlaybackSlackScale;
}

int32 FRTPlaybackDirector::PlaybackActivationsQueuedForTest() const
{
	int32 N = PlaybackActivationsPrep.Num() + PlaybackActivationsDash.Num();
	for (const FRTBlastSequenceElement& E : PlaybackBlastSequence)
	{
		if (Timeline().IsValidIndex(E.TimelineIndex)
			&& Timeline()[E.TimelineIndex].Type == ERTResolvedEventType::AbilityActivated)
		{
			++N;
		}
	}
	return N;
}

int32 FRTPlaybackDirector::PlaybackAnimCellIndexForTest(const ARTUnit* Unit) const
{
	if (!PlaybackPhases.IsValidIndex(PlaybackPhaseIdx))
	{
		return INDEX_NONE;
	}
	const ERTMatchPhase Ph = PlaybackPhases[PlaybackPhaseIdx];
	for (int32 i = 0; i < MoveAnims.Num(); ++i)
	{
		if (MoveAnims[i].Phase == Ph && MoveAnims[i].Unit.Get() == Unit && PlaybackAnimCellIndex.IsValidIndex(i))
		{
			return PlaybackAnimCellIndex[i];
		}
	}
	return INDEX_NONE;
}

ERTMatchPhase FRTPlaybackDirector::GetPlaybackPhase() const
{
	return (Host->IsPlaybackResolving() && PlaybackPhases.IsValidIndex(PlaybackPhaseIdx)) ? PlaybackPhases[PlaybackPhaseIdx]
		: ERTMatchPhase::Planning;
}

FString FRTPlaybackDirector::GetPlaybackPhaseName() const
{
	switch (GetPlaybackPhase())
	{
	case ERTMatchPhase::Planning: return FString(); // fuori dal playback: `PlaybackPhases` non contiene `Planning`
	case ERTMatchPhase::Prep:    return TEXT("Prep");
	case ERTMatchPhase::Dash:    return TEXT("Dash");
	case ERTMatchPhase::Blast:   return TEXT("Blast");
	case ERTMatchPhase::Move:    return TEXT("Move");
	case ERTMatchPhase::Cleanup: return TEXT("Cleanup");
	default:                     return TEXT("Risoluzione");
	}
}

float FRTPlaybackDirector::GetPlaybackProgress01() const
{
	if (!Host->IsPlaybackResolving() || PlaybackTotalSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(PlaybackElapsedTotal / PlaybackTotalSeconds, 0.f, 1.f);
}
