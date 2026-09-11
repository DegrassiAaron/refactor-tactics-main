// Copyright RefactorTactics. All Rights Reserved.

#include "Bot/RTBotPlanningLibrary.h"

#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Perception/RTTeamKnowledge.h"
#include "Turn/RTHexSimLibrary.h"

FRTBotPlanningOutcome URTBotPlanningLibrary::PlanTurn(
	const FRTHexSnapshot& BaseSnapshot,
	const TArray<FRTBotUnitFacts>& Facts,
	const FRTBotWeights& Pesi,
	const TMap<int32, FRTTeamKnowledge>& KnowledgeByTeam,
	TMap<int32, int32>& IdleTurns,
	TMap<int32, int32>& IdleRound,
	int32 TurnNumber,
	bool bRecordAudit)
{
	FRTBotPlanningOutcome Esito;


	// Gli snapshot di pianificazione: uno per squadra che ha almeno un bot.
	//
	// 🔑 **Condiviso dentro la squadra, e non e' un dettaglio**: i compagni prenotano la rotta scelta sullo
	// stesso snapshot, quindi chi decide prima vincola chi segue. E' cio' che impedisce a due alleati di
	// pianificare la stessa cella (`Bot.PlanBotsGivesTeammatesDistinctCells`).
	TMap<int32, FRTHexSnapshot> PlanningSnapshots;
	for (const FRTBotUnitFacts& U : Facts)
	{
		if (U.bIsBotControlled && !PlanningSnapshots.Contains(U.TeamId))
		{
			PlanningSnapshots.Add(U.TeamId, BaseSnapshot);
		}
	}

	// Prenota sullo snapshot di squadra la cella che questo bot ha scelto, cosi' il compagno che decide
	// dopo non la considera libera.
	//
	// ⚠️ **Riceve il PIANO invece di leggerlo dall'unita'** (#3013): prima prendeva un `ARTUnit*` e ne
	// rileggeva `PlannedDashCell` / `PlannedCell` — l'unica ragione per cui aveva bisogno dell'Actor.
	auto ReserveNormalMove = [](FRTHexSnapshot& TeamSnapshot, const FRTBotPlanDecision& PianoBot,
		const FRTCellId& CellaAttuale, int32 PlannedIdx)
	{
		if (PianoBot.PlannedDashAbility != INDEX_NONE)
		{
			if (!(PianoBot.PlannedDashCell == CellaAttuale)
				&& !TeamSnapshot.Occupancy.Contains(PianoBot.PlannedDashCell))
			{
				TeamSnapshot.Occupancy.Add(PianoBot.PlannedDashCell, PlannedIdx);
			}
			return;
		}
		URTHexBotLibrary::ReservePlannedRoute(TeamSnapshot, PlannedIdx, PianoBot.PlannedCell);
	};


	// Gli id di TUTTO l'equipaggiamento spedito, per distinguere un'abilita' concessa dal LOADOUT da una del
	// KIT (`#1403`, [D-220]). Si chiede al catalogo, non all'indice.
	//
	// ⚠️ **Tutto l'equipaggiamento, non i soli moduli reazione**: `EquipLoadout` passa da
	// `MakeEquipmentAction` per ogni pezzo non-arma, **gadget compresi**, e quella funzione scrive
	// `Def.ActionId = Item->EquipmentId` per tutti. Un gadget costruito su un'azione di slot reazione
	// finirebbe archiviato fra le abilita' di kit — e sarebbe la stessa «origine per accidente» che questa
	// riga esiste per togliere, un livello piu' sotto.
	//
	// ⚠️ **`static`, quindi una volta per processo**: `FindEquipment` ricostruisce i tre cataloghi a ogni
	// chiamata — **diciassette** `NewObject` piu' le `FText` — e `PlanBots` gira a ogni turno. E' l'idioma
	// che `DefaultReactionModuleFor` e `DefaultGadgetFor` gia' usano due funzioni piu' su.
	static const TSet<FName> IdEquipaggiamento = []()
	{
		TSet<FName> Ids;
		for (const TArray<URTEquipmentData*>& Catalogo :
			{ URTCatalogLibrary::MakeWeaponVariants(), URTCatalogLibrary::MakeGadgets(),
			  URTCatalogLibrary::MakeReactionModules() })
		{
			for (const URTEquipmentData* Pezzo : Catalogo)
			{
				if (Pezzo) { Ids.Add(Pezzo->EquipmentId); }
			}
		}
		return Ids;
	}();


	for (int32 BotIdx = 0; BotIdx < Facts.Num(); ++BotIdx)
	{
		const FRTBotUnitFacts& Bot = Facts[BotIdx];
		if (!Bot.bIsBotControlled)
		{
			continue; // il giocatore umano mira dove vuole: il suo filtro e' altrove, e non e' questo
		}

		// Il piano di QUESTO bot, aggiunto subito: i `continue` piu' sotto escono con un piano parziale, e
		// un piano parziale e' il piano — «fermo, senza attacco» e' una decisione come le altre.
		FRTBotPlanDecision& Piano = Esito.Decisions.AddDefaulted_GetRef();
		Piano.UnitIndex = Bot.Index;

		// Il record si APRE adesso e si chiude dopo la scelta: un bot che esce dal ciclo senza bersaglio ne
		// lascia comunque uno, con `TargetUnitId` a `INDEX_NONE`. Cosi' «nessuna scelta» resta distinguibile
		// da «nessuna cattura», che e' la differenza fra un dato e un buco.
		const int32 IdxScelta = bRecordAudit ? Esito.AuditDecisions.Num() : INDEX_NONE;
		if (bRecordAudit)
		{
			FRTAuditBotDecision Apertura;
			Apertura.UnitId = Bot.StableUnitId;
			Apertura.TeamId = Bot.TeamId;
			Esito.AuditDecisions.Add(Apertura);
		}

		Piano.PlannedCell = Bot.Cell;   // default: fermo
		Piano.PlannedAttackTargetIndex = INDEX_NONE;
		Piano.PlannedAbilityIndex = INDEX_NONE;
		Piano.PlannedPath.Reset();       // il bot pianifica destinazioni, non percorsi a waypoint
		Piano.PlannedWaypoints.Reset();
		// 🔴 Anche lo SCATTO, che fino al 2026-08-25 restava fuori da questo azzeramento: quattro campi
		// su cinque ripartivano da zero e il quinto no. Un dash che il resolver non ha consumato — perche'
		// la sua destinazione non era piu' raggiungibile, o perche' l'azione era in ricarica — sopravviveva
		// alla ripianificazione e si sommava al movimento deciso in QUESTO turno.
		//
		// ⚠️ Il difetto era invisibile finche' la carica occupava la principale: `[Ram(Main), Move(Movement)]`
		// e' un piano legale, e nessuno guardava. Con [D-191] la carica e' mobilita', quindi le due voci si
		// contendono lo slot e `ValidatePlan` lo dichiara `SlotOccupied` — misurato dal bot, non dedotto.
		Piano.PlannedDashAbility = INDEX_NONE;
		// ⚠️ **E lo slot REAZIONE, che era il sesto campo su sei a non ripartire da zero** (`#1403`,
		// [D-220]): fino a oggi dipendeva solo da `ClearReactionPlan()` nel Cleanup, che non gira sul
		// passaggio di `BeginPlay` ne' quando `PlanBotsForTest()` precede `LockInAndResolve()`. Da [D-220]
		// «nessuna reazione utilizzabile» e' un esito raggiungibile da due categorie indipendenti — kit e
		// loadout — quindi un indice stantio sopravviverebbe piu' spesso di prima. Si passa dalla porta di
		// [D-109]: azzera lo slot **e la sua condizione**, che sono una cosa sola.
		Piano.PlannedReactionAbility = INDEX_NONE;

		// Lo snapshot su cui QUESTO bot pianifica: quello della sua squadra, che porta le prenotazioni delle
		// compagne gia' passate di qui. Esistono tutti da prima del ciclo, quindi qui non si inserisce nulla
		// e il riferimento non puo' essere invalidato da una riallocazione.
		FRTHexSnapshot* TeamSnapshotPtr = PlanningSnapshots.Find(Bot.TeamId);
		if (!TeamSnapshotPtr)
		{
			continue; // non puo' accadere: la mappa e' costruita sugli stessi bot che questo ciclo visita
		}
		// UN SOLO nome, e non due: un alias `const` accanto a uno scrivibile dichiarerebbe un'immutabilita'
		// che non c'e' — la prenotazione a fine iterazione scrive proprio qui dentro.
		FRTHexSnapshot& Snapshot = *TeamSnapshotPtr;

		// Difesa: se ferito (sotto meta' HP) e ha un'abilita' che lo RIMETTE IN PIEDI, la usa e salta il turno.
		//
		// «Supporto» qui significa curare o schermare, non genericamente «agire su di se'»: il filtro era
		// `bSelfTarget` e basta, e finche' nessuna azione dichiarava quel flag la differenza non si vedeva.
		// Appena `Action.Guard` e `Action.Brace` l'hanno dichiarato — sono generiche, quindi le ha OGNI eroe —
		// un bot sotto meta' HP entrava qui ogni turno: Guard ha cooldown 0, quindi e' sempre pronta, e il
		// `continue` gli fa saltare l'attacco. Risultato: il bot ferito si mette in guardia per sempre e la
		// partita non finisce (`HexMatch.PlaysToCompletion`).
		//
		// Il ramo era scritto per `Guardian.Barrier`, che di cooldown ne aveva 3 e dava 40 di scudo. Chiedere
		// un effetto curativo lo riporta a quel significato senza dipendere dai cooldown, che sono
		// bilanciamento e cambiano.
		//
		// 🔴 2026-09-04 (`#2283`): **e lo SCUDO non basta, serve la CURA.** Lo stesso loop e' tornato
		// appena `Action.Shield` ha dichiarato `bSelfTarget` e i suoi due portatori d'eroe hanno dato al ramo
		// il suo primo consumatore reale del roster. La ragione sta nella condizione d'ingresso, non nei
		// cooldown: `Health * 2 < MaxHealth` la scioglie **solo** un effetto che alza gli HP. Lo scudo di
		// `Action.Shield` e' TEMPORANEO — `AddTemporaryShield`, scade nel Cleanup — quindi non tocca
		// `Health`, la condizione resta vera per sempre e il bot rientra qui a ogni ricarica. Misurato: Ivrin
		// ferma 5 turni contro un limite di 4, «di cui 2 inerti e 3 armati», con `Bot.StallDefinitions...`,
		// `Match.Autobattle...`, `Replay.Producer...` e il playback dell'Editor rossi a cascata.
		//
		// 🔑 Il criterio e' quindi **l'effetto che scioglie la guardia**, non «supporto» in generale: e'
		// cio' che rende il ramo non ripetibile a vuoto senza aggiungere stato all'unita' — stato che il
		// replay dovrebbe serializzare, e sarebbe determinismo speso per un ripiego.
		//
		// Con questo il ramo torna NON ATTRAVERSATO nel roster v0.1, che e' lo stato documentato da `#464` e
		// scelto dal progetto: rendere un'azione curativa lanciabile su di se' *«e' una scelta di
		// bilanciamento — un eroe che si cura da solo cambia il ritmo dello scontro — non un refactoring»*,
		// ed e' rinviata alla v0.2. Chi la prendera' trovera' qui la prova che «schermi» non equivale a
		// «curi», e che il ramo va ripensato prima di aprirlo allo scudo.
		bool bUsedSupport = false;
		for (int32 A = 0; A < Bot.NumAbilities(); ++A)
		{
			const URTActionData* Ab = Bot.GetAbility(A);
			bool bRestores = false;
			if (Ab)
			{
				for (const FRTActionEffectSpec& Spec : Ab->Def.Effects)
				{
					// Solo `Heal`: vedi sopra — uno scudo non alza `Health`, quindi non scioglie la guardia
					// che ha fatto entrare qui, e il ramo si ripeterebbe a ogni ricarica (#2283).
					if (Spec.Effect == ERTActionEffect::Heal)
					{
						bRestores = true;
						break;
					}
				}
			}
			if (Ab && Ab->bSelfTarget && bRestores && Bot.CanUseAbility(A) && Bot.Health * 2 < Bot.MaxHealth)
			{
				Piano.PlannedAbilityIndex = A;
				bUsedSupport = true;
				break;
			}
		}
		// REAZIONE (`#601`): il bot arma la reazione che ha, se ne ha una pronta. Lo slot e' indipendente da
		// Movimento e Principale, quindi non compete con nient'altro e si dichiara PRIMA di ogni `continue`
		// del resto della pianificazione — altrimenti un bot che cura o che scatta uscirebbe dal ciclo senza
		// armarla.
		//
		// Nessuna euristica su QUANDO conviene: il trigger e' dichiarato dall'abilita' e valutato dal
		// resolver, e una reazione non armata non costa nulla a nessuno.
		//
		// Senza questa riga meta' delle unita' della v0.1 non reagirebbe mai, e il playtest misurerebbe un
		// gioco diverso da quello progettato: i sette moduli di CP 7.5 sarebbero verdi nei test e assenti in
		// partita.
		//
		// Contesto di valutazione: i nemici che la SQUADRA DEL BOT conosce (celle, gittata effettiva,
		// HP+scudo) e pesi dal tuning.
		// L'ordine dei nemici viene da Units (ordine stabile dello snapshot): il punteggio non dipende
		// dall'ordine di enumerazione degli Actor.
		//
		// CP 13.5 — IL BOT PIANIFICA SULLA CONOSCENZA DELLA SUA SQUADRA (#160, RT-FEAT-BOT-FAIRNESS).
		// Fino a qui `Ctx.Enemies` conteneva *tutte* le unita' nemiche vive, senza filtro di percezione: il
		// bot vedeva ogni posizione avversaria mentre il giocatore no. Non era una svista nascosta — la spec
		// lo dichiarava (`docs/gameplay/spec-bot-hex.md` §6) — ma rendeva falsa la promessa che la Wiki fa al
		// giocatore, «il bot non vede piu' di te», e invalidava per costruzione ogni playtest contro di lui.
		//
		// La regola e' la STESSA del targeting umano (`ClassifyTarget`, piu' sotto in questo file): non un
		// secondo modello di conoscenza per il bot, che divergerebbe dal primo alla prima modifica.
		// 🔴 **L'identita' della squadra si RIMETTE, e non e' pedanteria.** `FindRef` su una chiave assente
		// restituisce un `FRTTeamKnowledge` default-costruito, il cui `TeamId` vale **0**; e
		// `URTTeamKnowledgeLibrary::ClassifyTarget` corto-circuita su `TargetTeamId == Knowledge.TeamId`
		// restituendo `Allowed`. Un bot di una squadra diversa da 0, con la conoscenza incompleta,
		// classificherebbe quindi OGNI nemico della squadra 0 come pienamente visibile — cella vera, salute
		// vera, `Unbalanced` vero: esattamente la fuga di informazione che CP 13.5 esiste per chiudere.
		//
		// ⚠️ `ARTTurnManager::KnowledgeForTeam` questo lo faceva gia' — timbra `Empty.TeamId = TeamId` sul
		// proprio ripiego — e la traduzione lo aveva perso. Qui la porta d'ingresso e' **pubblica**, quindi
		// il caso degradato e' raggiungibile da chiunque costruisca una mappa parziale. Trovato in code review.
		FRTTeamKnowledge BotKnowledge = KnowledgeByTeam.FindRef(Bot.TeamId);
		BotKnowledge.TeamId = Bot.TeamId;
		FRTHexBotContext Ctx;
		Ctx.Origin = Bot.Cell;
		// Da dove il bot guarda ORA: e' il punto di partenza della stima di come sara' orientato a fine turno
		// (CP 13.5). Chi resta fermo e non attacca conserva questo.
		Ctx.SelfFacing = Bot.Facing;
		// Il kiting lo DERIVA il bot dalla portata dell'attacco base: e' un comportamento dell'IA, non una
		// caratteristica dell'unita' (che quando la muove il giocatore non lo consulta mai).
		Ctx.KiteStandoff = URTHexBotLibrary::DeriveKiteStandoff(Bot.AttackRange);


		Ctx.WKill = Pesi.WKill;
		Ctx.WDamage = Pesi.WDamage;
		Ctx.WThreat = Pesi.WThreat;
		Ctx.WKiteViolation = Pesi.WKiteViolation;
		Ctx.WApproach = Pesi.WApproach;
		Ctx.WElevation = Pesi.WElevation;
		Ctx.WEngage = Pesi.WEngage;
		Ctx.WEngageDecay = Pesi.WEngageDecay;
		Ctx.WObjective = Pesi.WObjective;
		Ctx.WObjectiveFalloff = Pesi.WObjectiveFalloff;

		// Le celle OBIETTIVO, lette dai dati di mappa (`#2269`).
		//
		// ⚠️ **Geometria pubblica, e per questo NON passa dal filtro di percezione** che le righe qui sotto
		// applicano ai nemici. Dov'e' l'obiettivo lo vedono entrambe le squadre — il giocatore umano ce l'ha
		// sullo schermo dal primo fotogramma — quindi nasconderlo al bot non sarebbe fairness, sarebbe
		// renderlo cieco a un'informazione che non e' mai stata segreta. Cio' che CP 13.5 protegge sono le
		// UNITA' avversarie e i loro intenti, non il terreno.
		//
		// ⚠️ **Si legge da `Map->Cells`, che e' ordinato** (`SortCells`): l'ordine dell'array e' stabile, e
		// nessuna decisione del bot dipende dall'ordine di enumerazione (invariante #4).
		//
		// ⚠️ Su una mappa senza obiettivi l'array resta vuoto e il termine vale zero riga per riga: e' la
		// ragione per cui nessuna arena generata — che un obiettivo non lo posa — cambia comportamento.
		if (Snapshot.Map)
		{
			for (const FRTHexCellData& Cell : Snapshot.Map->Cells)
			{
				if (Cell.bIsObjective)
				{
					Ctx.ObjectiveCells.Add(Cell.Id);
				}
			}
		}

		// La memoria per unita' del termine di ingaggio: quanti turni consecutivi questa unita' non
		// pianifica un attacco (#1300, D-185). Si aggiorna piu' sotto, a piano scelto.
		Ctx.IdleTurns = IdleTurns.FindRef(Bot.StableUnitId);

		TArray<int32> EnemyUnitIndex; // parallelo a Ctx.Enemies: indice dell'unita' in Units
		const FRTBotUnitFacts* Nearest = nullptr;
		// La cella da cui il kiter fugge, come la CONOSCE la squadra: su un contatto incerto e' il ricordo,
		// non la posizione vera. Senza questo campo la fuga userebbe `Nearest->Cell` — cioe' il bot
		// scapperebbe da dove il nemico e' davvero, che e' l'onniscienza rientrata dalla finestra.
		FRTCellId NearestKnownCell;
		int32 NearestDistance = MAX_int32;
		for (int32 j = 0; j < Facts.Num(); ++j)
		{
			const FRTBotUnitFacts& Other = Facts[j];
			if (Other.TeamId == Bot.TeamId)
			{
				// Alleati: servono a pesare il collaterale di un'area (#213). Il bot NON si conta fra loro
				// perche' `CollectHexAttacks` salta sempre l'attaccante.
				if (j != BotIdx)
				{
					Ctx.Allies.Add(Other.Cell);
					Ctx.AllyHealth.Add(Other.Health + Other.Shield);
				}
				continue;
			}
			int32 EnemyReach = Other.AttackRange;
			for (int32 a = 0; a < Other.NumAbilities(); ++a)
			{
				const URTActionData* EAb = Other.GetAbility(a);
				// La minaccia e' cio' che il nemico puo' COLPIRE: una mobilita' rapida sposta, non fa danno a
				// distanza, e contarla gonfierebbe la portata percepita di ogni eroe che ne ha una.
				if (EAb && !URTCatalogLibrary::IsFastMovement(EAb->Def))
				{
					EnemyReach = FMath::Max(EnemyReach, EAb->RangeCells);
				}
			}
			// Cosa la squadra sa di questo nemico. `EnemyReach` NON passa di qui: gittate e forme sono
			// catalogo, cioe' dato pubblico — sapere che Phase ha portata 5 non e' sapere dov'e' Phase.
			FRTCellId KnownCell = Other.Cell;
			int32 KnownHealth = Other.Health + Other.Shield;
			// La CONDIZIONE segue la stessa disciplina degli HP: su un contatto incerto non si sa, e non si
			// indovina ([D-319], `#2253`). Vedi il ramo `CellOnly` sotto.
			bool bKnownUnbalanced = Other.bUnbalanced;
			switch (URTTeamKnowledgeLibrary::ClassifyTarget(BotKnowledge, Other.StableUnitId,
				Other.TeamId, Other.Cell))
			{
			case ERTTargetKnowledge::Allowed:
				break; // la squadra lo vede: cella e condizione attuali, come sempre

			case ERTTargetKnowledge::CellOnly:
			{
				// Contatto INCERTO: vale la cella dell'ULTIMO contatto, mai quella attuale — altrimenti il
				// ricordo inseguirebbe il bersaglio, che e' il modo silenzioso di continuare a vederlo.
				if (!URTTeamKnowledgeLibrary::LastKnownCell(BotKnowledge, Other.StableUnitId, KnownCell))
				{
					continue; // incerto senza ricordo: non e' ne' bersaglio ne' minaccia contabilizzabile
				}
				// ⚠️ **Gli HP correnti sarebbero la fuga esatta che il canary deve prendere**: direbbero al
				// bot che un'unita' che NON vede e' quasi morta, e lo manderebbe a finirla. Cio' che la
				// squadra conosce di un ricordo e' l'IDENTITA' (`StableUnitId` -> eroe -> catalogo), non la
				// condizione: si assume quindi integro, con un valore pubblico.
				//
				// ⚠️ Limite dichiarato: cosi' si PERDE anche informazione legittima — se la squadra l'ha
				// visto a 10 HP un turno fa, quel dato c'era. Il modello corretto e' un HP nel contatto, cioe'
				// un campo in `FRTLastKnownContact` e un incremento di `FRTTeamKnowledge::CurrentVersion`:
				// una decisione di formato, non un dettaglio di questo checkpoint. L'errore va nella
				// direzione sicura — il bot sottostima le occasioni, non ne inventa.
				KnownHealth = Other.MaxHealth;
				// Stesso argomento, stesso verso sicuro: «sbilanciato» e' CONDIZIONE, non identita'. Dirlo
				// su un ricordo manderebbe il bot a capitalizzare su un'unita' che non vede, ed e' la fuga
				// di conoscenza che il filtro esiste per chiudere. Il bot perde occasioni, non ne inventa.
				bKnownUnbalanced = false;
				break;
			}

			default:
				continue; // ignoto alla squadra: per il bot quella cella e' vuota
			}

			Ctx.Enemies.Add(KnownCell);
			Ctx.EnemyRanges.Add(EnemyReach);
			Ctx.EnemyHealth.Add(KnownHealth);
			Ctx.EnemyUnbalanced.Add(bKnownUnbalanced);
			// CP 13.5 — l'ORIENTAMENTO del nemico, che decide se la sua copertura vale (ADR-0005 §4a).
			//
			// Si prende quello corrente e non si filtra, ed e' corretto: il facing e' cio' che la mesh mostra,
			// quindi il giocatore umano lo legge allo stesso modo. A restare privato e' l'INTENTO di rotazione
			// (`Facing.IntentIsTeamFiltered`), che qui non passa.
			//
			// ⚠️ Su un contatto `CellOnly` la cella e' quella del RICORDO ma il facing e' quello ATTUALE: e' una
			// piccola incoerenza voluta, perche' l'alternativa — ricordare anche l'orientamento — vorrebbe un
			// campo in `FRTLastKnownContact` e un incremento di `FRTTeamKnowledge::CurrentVersion`, cioe' una
			// decisione di formato. L'errore va nella direzione sicura: la copertura si calcola fra la cella
			// ricordata e la mia, e un facing piu' aggiornato del ricordo non rivela DOVE sia l'unita'.
			Ctx.EnemyFacings.Add(Other.Facing);
			EnemyUnitIndex.Add(j);

			// La distanza si misura da cio' che si CONOSCE: su un contatto incerto e' la cella del ricordo.
			const int32 Distance = URTHexLibrary::HexDistance(Bot.Cell, KnownCell);
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				Nearest = &Other;
				NearestKnownCell = KnownCell;
			}
		}

		// 🔴 **LA REGOLA E' CAMBIATA: punteggio tattico, e il kit RETROCEDE a tie-break** ([D-268], `#1802`).
		//
		// [D-220] aveva DICHIARATO la regola che c'era gia' — prima il kit, il modulo come riserva — invece di
		// sceglierne una. E' deterministica ma non tattica: la reazione d'identita' vince sempre, quale che sia
		// il suo valore in quella situazione, e il valore del loadout si perde. Il caso concreto: un Branth con
		// `Interposition` nel kit e un modulo di contrattacco, in un turno in cui nessun alleato e' minacciato
		// e un nemico conosciuto puo' colpirlo, armava l'interposizione — cioe' una reazione che non sarebbe
		// scattata.
		//
		// ⚠️ **E per questo il blocco vive QUI e non piu' prima del contesto.** Un punteggio tattico si misura
		// su cio' che la squadra CONOSCE, e `Ctx.Enemies`/`Ctx.Allies` nascono dalla raccolta qui sopra. Le due
		// alternative erano peggiori: ricalcolare il filtro di conoscenza nel punto vecchio avrebbe creato il
		// secondo modello di conoscenza che il commento della raccolta vieta per nome, e lasciare il punteggio
		// cieco avrebbe reso **vacua** l'AC di equita' di [D-268] — un punteggio costante la soddisfa.
		//
		// ⚠️ **`if (bUsedSupport)` si e' spostato INSIEME al blocco, e non e' un dettaglio**: prima usciva
		// dal ciclo PRIMA della raccolta, quindi un bot che si cura armava comunque la reazione. Lasciandolo
		// dov'era, questo spostamento gliela avrebbe tolta — un cambio di comportamento che `#1802` non chiede.
		//
		// 🔑 **La proprieta' che rende il cambio atterrabile**: dove la conoscenza non separa i candidati tutti
		// i punteggi valgono zero, decide il tie-break, e il bot arma esattamente cio' che armava prima.
		TArray<FRTReactionCandidate> ReactionCandidates;
		for (int32 R = 0; R < Bot.NumAbilities(); ++R)
		{
			const URTActionData* Reaction = Bot.GetAbility(R);
			if (!Reaction || Reaction->Def.Slot != ERTActionSlot::Reaction || !Bot.CanUseAbility(R))
			{
				continue;
			}
			// L'origine si chiede al CATALOGO: `MakeEquipmentAction` scrive `Def.ActionId = EquipmentId`.
			// Dedurla dalla posizione — «i moduli stanno in fondo perche' `Add` accoda» — e' l'accidente che
			// [D-220] aveva gia' smesso di usare, e che qui serve come TIE-BREAK invece che come regola.
			FRTReactionCandidate& Candidate = ReactionCandidates.AddDefaulted_GetRef();
			Candidate.AbilityIndex = R;
			Candidate.bFromKit = !IdEquipaggiamento.Contains(Reaction->Def.ActionId);
			Candidate.Score = URTHexBotLibrary::ScoreReaction(Snapshot.Map, Reaction->Def, Ctx);
		}

		// La scelta porta con se' la RAGIONE, che [D-245] chiede sia un dato e non una deduzione di chi
		// legge: «ha vinto perche' valeva di piu'», «ha vinto lo spareggio di kit» e «ha vinto l'indice» sono
		// tre spiegazioni diverse della stessa riga, e un'etichetta sola le confonderebbe.
		const FRTReactionChoice Choice = URTHexBotLibrary::SelectReaction(ReactionCandidates);
		if (const URTActionData* Armed = Bot.GetAbility(Choice.AbilityIndex)) // `nullptr` per `INDEX_NONE`
		{
			Piano.PlannedReactionAbility = Choice.AbilityIndex;

			const TCHAR* Reason = TEXT("");
			switch (Choice.DecidedBy)
			{
			case ERTReactionTieBreak::Kit:   Reason = TEXT(", spareggio: kit");   break;
			case ERTReactionTieBreak::Index: Reason = TEXT(", spareggio: indice"); break;
			case ERTReactionTieBreak::Utility:
			case ERTReactionTieBreak::None:  break;
			}
			Esito.LogLines.Add(FRTBotLogLine{FString::Printf(TEXT("%s: arma %s (reazione, punteggio %d%s)"),
				*Bot.DisplayName, *Armed->Def.ActionId.ToString(), Choice.Score, Reason), Bot.Index});
		}

		if (bUsedSupport)
		{
			continue;
		}

		// CP 13.5 — NESSUN CONTATTO: si cerca, non ci si ferma.
		//
		// Prima del filtro di percezione questo caso non esisteva: `Ctx.Enemies` conteneva sempre tutti i
		// nemici vivi, quindi c'era sempre qualcuno verso cui avvicinarsi. Con la conoscenza parziale una
		// squadra puo' non sapere dove sia nessuno — ed e' la condizione NORMALE del primo turno, perche' su
		// una mappa di raggio 5 gli schieramenti opposti distano piu' della vista di chiunque.
		//
		// ⚠️ **Senza questo ramo la partita non finisce.** Con `Ctx.Enemies` vuoto lo scoring perde i termini
		// di minaccia e di avvicinamento, ogni cella vale uguale, e il bot resta fermo per sempre: due squadre
		// cieche che si aspettano. Non e' «il bot perde il contatto e sbaglia» (che il DoD ammette): e' un bot
		// che smette di giocare, e l'ha misurato `HexMatch.PlaysToCompletion` diventando rosso.
		//
		// La condotta e' la piu' povera che ristabilisce il contatto: avvicinarsi al CENTRO della mappa, che
		// e' geometria pubblica — zero informazione nascosta. Non e' una ricerca intelligente e non pretende
		// di esserlo: i goal veri (`SecureObjective`, `GatherInformation`) sono E26, e questo ramo e' il posto
		// in cui atterreranno. Deterministica: distanza minima dal centro, poi `StableLess`.
		// **Livello 3 di #1287: la condizione si estende da «non so dove sia nessuno» a «non ho nessuno che
		// posso ingaggiare».**
		//
		// Il caso che mancava: contatto NOTO ma non raggiungibile in modo utile. Misurato sulla mappa
		// d'autore — le due squadre si fermano ai lati dell'ostacolo centrale, che blocca vista e passo, a due
		// e tre celle di distanza in linea d'aria. `Ctx.Enemies` non e' vuoto, quindi questo ramo non entrava;
		// e il punteggio, che misura la distanza in linea d'aria, diceva «sei vicino, resta». Dodici turni,
		// 42 voci di TurnLog su 48 con esito `Stayed`, zero `Combat`.
		bool bQualcunoDaIngaggiare = false;
		if (Ctx.Enemies.Num() > 0 && Snapshot.Map)
		{
			for (const FRTHexReachableCell& R : URTHexSimLibrary::ReachableCells(Snapshot, Bot.Index))
			{
				for (const FRTCellId& KnownEnemy : Ctx.Enemies)
				{
					if (URTHexVisionLibrary::HasLineOfSight(Snapshot.Map, R.Cell, KnownEnemy))
					{
						bQualcunoDaIngaggiare = true;
						break;
					}
				}
				if (bQualcunoDaIngaggiare) { break; }
			}
		}

		if (Ctx.Enemies.Num() == 0 || !bQualcunoDaIngaggiare)
		{
			if (Snapshot.Map && Snapshot.Map->Cells.Num() > 0)
			{
				// **Il PUNTO DI OSSERVAZIONE (#1287)**, quando un contatto noto esiste ma non e' ingaggiabile: la
				// cella percorribile piu' vicina PER CAMMINO da cui quel contatto si vedrebbe.
				//
				// ⚠️ **Per cammino e non in linea d'aria**, ed e' la differenza fra funzionare e no: con un
				// ostacolo in mezzo la meta e' geometricamente vicina e topologicamente lontana, e minimizzare la
				// distanza in linea d'aria incastra il bot contro il muro — che e' il difetto originale, ripetuto
				// un livello piu' in la'.
				//
				// ⚠️ Usa la MEMORIA del contatto (`FRTLastKnownContact`, CP 13.4), non le posizioni vere: il bot
				// va dove ha visto qualcuno, non dove qualcuno e'.
				//
				// ⛔ Non e' un pattern di ricerca: niente memoria di dove ha gia' guardato, niente settori, niente
				// coordinamento. Quelli sono E26 (#326), e chiedono stato per unita' che il bot oggi non ha.
				FRTCellId SeekCell;
				bool bHaMeta = false;
				if (Ctx.Enemies.Num() > 0)
				{
					int32 MiglioreCosto = MAX_int32;
					for (const FRTHexCellData& C : Snapshot.Map->Cells)
					{
						if (C.bBlocksMovement) { continue; }
						bool bVede = false;
						for (const FRTCellId& KnownEnemy : Ctx.Enemies)
						{
							if (URTHexVisionLibrary::HasLineOfSight(Snapshot.Map, C.Id, KnownEnemy)) { bVede = true; break; }
						}
						if (!bVede) { continue; }

						const FRTHexPathResult Verso = URTHexSimLibrary::FindPathForUnit(Snapshot, Bot.Index, C.Id);
						if (Verso.Path.Num() == 0) { continue; } // irraggiungibile: non e' una meta
						if (Verso.TotalCost < MiglioreCosto
							|| (Verso.TotalCost == MiglioreCosto && URTHexLibrary::StableLess(C.Id, SeekCell)))
						{
							MiglioreCosto = Verso.TotalCost;
							SeekCell = C.Id;
							bHaMeta = true;
						}
					}
				}

				if (!bHaMeta)
				{
					// Nessun contatto noto, o nessuna cella lo vede: il CENTRO, la condotta di CP 13.5. Geometria
					// pubblica, zero informazione nascosta.
					int64 SumX = 0, SumY = 0;
					for (const FRTHexCellData& C : Snapshot.Map->Cells) { SumX += C.Id.X; SumY += C.Id.Y; }
					const int32 N = Snapshot.Map->Cells.Num();
					const FRTCellId Barycentre(static_cast<int32>(SumX / N), static_cast<int32>(SumY / N), 0);

					SeekCell = Snapshot.Map->Cells[0].Id;
					int32 BestToBary = MAX_int32;
					for (const FRTHexCellData& C : Snapshot.Map->Cells)
					{
						// ⚠️ **Percorribile**, e l'assenza di questo filtro ha fermato l'intera partita. Su
						// `L_HexArena` il baricentro e' `(0,0)`, che blocca il passo: la meta era una cella in cui
						// non si puo' entrare, quindi nessun cammino, quindi nessun passo. Il codice precedente si
						// AVVICINAVA alla meta e sopravviveva a una meta impenetrabile; seguire un cammino no.
						if (C.bBlocksMovement) { continue; }
						const int32 D = URTHexLibrary::HexDistance(C.Id, Barycentre);
						if (D < BestToBary || (D == BestToBary && URTHexLibrary::StableLess(C.Id, SeekCell)))
						{
							BestToBary = D;
							SeekCell = C.Id;
						}
					}
				}

				// **Si SEGUE il cammino**, non si minimizza una distanza: il prefisso percorribile entro il
				// budget. Restare vince a parita' (cammino vuoto = si e' gia' a destinazione).
				FRTCellId Best = Bot.Cell;
				const FRTHexPathResult Rotta = URTHexSimLibrary::FindPathForUnit(Snapshot, Bot.Index, SeekCell);
				const TArray<FRTCellId> Passi = URTHexSimLibrary::TruncatePathToBudget(Snapshot, Bot.Index, Rotta.Path);
				if (Passi.Num() > 1)
				{
					Best = Passi.Last();
				}
				else
				{
					// Nessun cammino: ci si AVVICINA, che e' la condotta di CP 13.5 e non richiede che la meta sia
					// raggiungibile. Restare vince a parita', quindi un bot gia' al punto migliore non oscilla.
					int32 BestDistance = URTHexLibrary::HexDistance(Bot.Cell, SeekCell);
					for (const FRTHexReachableCell& R : URTHexSimLibrary::ReachableCells(Snapshot, Bot.Index))
					{
						const int32 D = URTHexLibrary::HexDistance(R.Cell, SeekCell);
						if (D < BestDistance || (D == BestDistance && URTHexLibrary::StableLess(R.Cell, Best)))
						{
							BestDistance = D;
							Best = R.Cell;
						}
					}
				}
				Piano.PlannedCell = Best;
			}
			// #1088 — anche qui, ed e' il ramo che il difetto colpiva per primo: due compagne che cercano il
			// contatto puntano ENTRAMBE la cella piu' vicina al centro, che e' una sola.
			ReserveNormalMove(Snapshot, Piano, Bot.Cell, Bot.Index);
			continue; // niente da bersagliare: nessun attacco, nessuno scatto verso un nemico che non si conosce
		}

		// Scatto disponibile per questo turno (serve sia alla fuga sia alle candidate di riposizionamento).
		const int32 DashIdx = Bot.DashAbilityIndex;
		const URTActionData* DashAb = Bot.GetAbility(DashIdx);
		const bool bDashReady = DashAb && URTCatalogLibrary::IsFastMovement(DashAb->Def) && Bot.CanUseAbility(DashIdx);

		// Portata dello scatto letta come la legge ResolveDash: dal CATALOGO se l'azione ne fa parte,
		// altrimenti dal campo legacy dell'asset. Se il bot leggesse un numero diverso da quello che il
		// resolver usera', proporrebbe scatti fuori portata (o si negherebbe quelli buoni).
		const int32 DashBudget = bDashReady ? Bot.EffectiveDashRange : 0;
		const ERTMovementStyle DashStyle = bDashReady ? DashAb->Def.MovementStyle : ERTMovementStyle::None;

		// Nemici del bot, per UnitId dello snapshot: la carica li tratta come bersagli, gli altri stili come
		// ostacoli. Sono gli stessi indici che ResolveDash passa a ResolveLinearMove.
		TSet<int32> DashHostiles;
		for (int32 j = 0; j < Facts.Num(); ++j)
		{
			if (Facts[j].bAlive && Facts[j].TeamId != Bot.TeamId) { DashHostiles.Add(Facts[j].Index); }
		}

		// 🔴 **Dalla BASE, non dallo snapshot di squadra, e la differenza e' una fase.** Le prenotazioni
		// descrivono dove le compagne andranno nel MOVE; il Dash risolve PRIMA del Move, quando quelle celle
		// sono ancora vuote. Copiandole qui, `ResolveLinearMove` e `IsLinearReachable` — che trattano ogni
		// occupante non ostile come un corpo solido — scarterebbero cariche e scatti perfettamente legali,
		// in silenzio. La prenotazione e' del Move: che sia CONSUMATA solo dal Move.
		FRTHexSnapshot DashSnapshot = BaseSnapshot;
		if (bDashReady)
		{
			// Le candidate nascono da `ReachableCells`, che spende PUNTI MOVIMENTO (Dijkstra sui costi). Ma la
			// portata di una mobilita' LINEARE si misura in CELLE — il catalogo dice che il terreno non la
			// riduce. Passare la portata direttamente come budget tronca le candidate sul terreno caro: su
			// acqua (costo 2) uno scatto da 5 celle ne vedrebbe 2, e le celle 3-5 non verrebbero mai
			// proposte benche' il resolver le raggiunga. E' la stessa divergenza celle-vs-MP di #140, un
			// gradino piu' a monte: il filtro puo' solo SCARTARE candidate, non farle nascere.
			//
			// Si allarga quindi il budget al caso peggiore (portata x costo della cella piu' cara della
			// mappa) e si lascia che `IsDashReachable` poti cio' che non e' in linea.
			int32 CandidateBudget = DashBudget;
			if (URTMovementActionLibrary::IsLinear(DashStyle) && Snapshot.Map)
			{
				int32 MaxCellCost = 1;
				for (const FRTHexCellData& Cell : Snapshot.Map->Cells)
				{
					MaxCellCost = FMath::Max(MaxCellCost, Cell.TotalMoveCost());
				}
				CandidateBudget = DashBudget * MaxCellCost;
			}
			DashSnapshot.Units[Bot.Index].MoveBudget = CandidateBudget;
		}

		// Il bot valuta la raggiungibilita' con lo STESSO codice che la fase Dash usa per eseguirla
		// (issue #140): il grafo genera le candidate, ma e' la linearita' a dire quali sopravvivono.
		//
		// L'instradamento per STILE e' quello di ResolveDash: solo le mobilita' LINEARI passano da
		// `ResolveLinearMove`. Una mobilita' a budget (`Action.Sprint`) risolve col pathfinding, lo stesso
		// grafo da cui le candidate sono nate — quindi li' non c'e' nulla da filtrare, e applicare il filtro
		// lineare scarterebbe mosse perfettamente legali.
		//
		// Anche il GATE "questa e' un'azione di scatto" e' lo stesso (#142): `URTCatalogLibrary::IsFastMovement`
		// legge la fase del catalogo, qui come in ResolveDash. Prima le due risposte divergevano e le azioni
		// degli eroi — che dichiarano la fase e nient'altro — non venivano mai pianificate come scatto.
		auto IsDashReachable = [&](const FRTHexSnapshot& Snap, const FRTCellId& Goal) -> bool
		{
			if (!URTMovementActionLibrary::IsLinear(DashStyle))
			{
				return true;
			}
			return URTMovementActionLibrary::IsLinearReachable(
				Snap.Map, Bot.Cell, Goal, DashBudget, DashStyle, Snap.Occupancy, DashHostiles);
		};

		// Priorita' ritirata: se un nemico e' molto vicino (meta' dello standoff), il kiter fugge SUBITO,
		// rinunciando al tiro. Guardia del bot quadrato, conservata: non passa dalla utility.
		const int32 Standoff = URTHexBotLibrary::DeriveKiteStandoff(Bot.AttackRange);
		const bool bKiter = Standoff > 0;
		if (bKiter && Nearest && NearestDistance <= Standoff / 2)
		{
			if (bDashReady)
			{
				// Anche la fuga del kiter passa da ReachableCells (grafo): se la cella scelta non e'
				// raggiungibile in LINEA, lo scatto verrebbe rifiutato e il panico si tradurrebbe in un turno
				// perso. Meglio non scattare e lasciare decidere al movimento normale.
				const FRTCellId Dest = URTHexBotLibrary::BestKiteCell(DashSnapshot, Bot.Index, NearestKnownCell);
				if (Dest != Bot.Cell && IsDashReachable(DashSnapshot, Dest))
				{
					Piano.PlannedDashAbility = DashIdx;
					Piano.PlannedDashCell = Dest;
					Esito.LogLines.Add(FRTBotLogLine{FString::Printf(TEXT("%s: scatto difensivo (schiva) -> (q=%d,r=%d,L%d)"),
						*Bot.DisplayName, Dest.X, Dest.Y, Dest.Layer), Bot.Index});
					continue;
				}
			}
			Piano.PlannedCell = URTHexBotLibrary::BestKiteCell(Snapshot, Bot.Index, NearestKnownCell);
			Esito.LogLines.Add(FRTBotLogLine{FString::Printf(TEXT("%s: arretra -> (q=%d,r=%d,L%d)"),
				*Bot.DisplayName, Piano.PlannedCell.X, Piano.PlannedCell.Y, Piano.PlannedCell.Layer), Bot.Index});
			ReserveNormalMove(Snapshot, Piano, Bot.Cell, Bot.Index);
			continue;
		}

		// --- Pool di candidate ---------------------------------------------------------------------
		// Un'unica utility sceglie fra: restare e sparare, riposizionarsi, scattare e sparare, scattare
		// per riposizionarsi. L'ATTACCO vale solo dalla cella in cui il bot si trovera' nel Blast: quella
		// attuale (il Move viene DOPO il Blast) o quella post-scatto (il Dash viene PRIMA).
		TArray<FRTHexBotPlan> Plans;
		TArray<int32> PlanAbility;  // abilita' d'attacco della candidata (INDEX_NONE = solo movimento)
		TArray<bool> PlanViaDash;   // la candidata si raggiunge con lo scatto
		// Vero se la candidata e' una CARICA: allora si punta la cella del NEMICO (`Ctx.Enemies[TargetIndex]`),
		// non `DestCell` — che per una carica e' dove ci si ferma, cioe' davanti al bersaglio. Serve un flag e
		// non una cella-sentinella: `FRTCellId()` vale (0,0,0), che e' una cella vera della mappa.
		TArray<bool> PlanIsCharge;

		auto AddCandidates = [&](const FRTHexSnapshot& Snap, int32 AbilityIndex, int32 Range, int32 Damage,
			bool bViaDash, bool bAttacksOnly)
		{
			FRTHexBotContext LocalCtx = Ctx;
			LocalCtx.AttackRange = Range;
			LocalCtx.AttackDamage = Damage;

			// Forma dell'azione valutata: senza, ogni attacco verrebbe pesato come un colpo singolo e un'area
			// non mostrerebbe ne' i nemici presi in piu' ne' il compagno investito (#213).
			if (const URTActionData* ShapedAbility = (AbilityIndex != INDEX_NONE) ? Bot.GetAbility(AbilityIndex) : nullptr)
			{
				LocalCtx.AttackShape = ShapedAbility->Shape;
				LocalCtx.AttackAreaRadius = ShapedAbility->AreaRadius;
				LocalCtx.bAttackFriendlyFire = ShapedAbility->Def.bFriendlyFire;
				// Chi SPOSTA, letto dagli effetti dichiarati ([D-319], `#2253`). Dal `Def` e non da una
				// lista di `ActionId`: cosi' vale anche per gli effetti che l'EQUIPAGGIAMENTO aggiunge —
				// `Weapon.Impact` accoda un `Push` all'attacco base, ed e' il loadout di default di Phase
				// (D-089). Una lista di nomi avrebbe mancato proprio il caso piu' comune.
				LocalCtx.bAttackDisplaces = false;
				for (const FRTActionEffectSpec& Effect : ShapedAbility->Def.Effects)
				{
					if (Effect.Effect == ERTActionEffect::Push || Effect.Effect == ERTActionEffect::Pull)
					{
						LocalCtx.bAttackDisplaces = true;
						break;
					}
				}
			}
			for (const FRTHexBotPlan& Candidate : URTHexBotLibrary::BuildCandidates(Snap, Bot.Index, LocalCtx))
			{
				if (bAttacksOnly && !Candidate.bHasAttack) { continue; }
				// Le candidate nascono da ReachableCells, che segue il GRAFO. Lo scatto invece e' lineare
				// (CP 4.5): senza questo filtro il bot proporrebbe scatti che ResolveDash rifiuta, sprecando
				// l'abilita' in silenzio. L'invariante "il bot non propone mosse illegali" vale anche qui.
				if (bViaDash && !IsDashReachable(Snap, Candidate.DestCell))
				{
					continue;
				}
				Plans.Add(Candidate);
				PlanAbility.Add(Candidate.bHasAttack ? AbilityIndex : INDEX_NONE);
				PlanViaDash.Add(bViaDash);
				PlanIsCharge.Add(false);
			}
		};

		// 1) Riposizionamento col movimento normale (gittata 0 -> nessun attacco: nel Blast il bot e' ancora qui).
		AddCandidates(Snapshot, INDEX_NONE, /*Range*/ 0, /*Damage*/ 0, /*bViaDash*/ false, /*bAttacksOnly*/ false);

		// 2) Attacco da FERMO, un'abilita' per volta: budget 0 -> l'unica cella candidata e' quella attuale.
		FRTHexSnapshot StaySnapshot = Snapshot;
		StaySnapshot.Units[Bot.Index].MoveBudget = 0;
		for (int32 A = 0; A < Bot.NumAbilities(); ++A)
		{
			const URTActionData* Ability = Bot.GetAbility(A);
			if (!Ability || URTCatalogLibrary::IsFastMovement(Ability->Def) || Ability->bSelfTarget
				|| !Bot.CanUseAbility(A)) { continue; }
			AddCandidates(StaySnapshot, A, Ability->RangeCells, Ability->Power, /*bViaDash*/ false, /*bAttacksOnly*/ true);
		}

		// 3) CARICA: l'unico modo di scattare E colpire nello stesso turno, perche' il danno e' dell'azione di
		// movimento stessa e non di una seconda azione principale (#145). Le candidate non possono nascere da
		// `ReachableCells`: quella cerca celle LIBERE, mentre una carica punta la cella OCCUPATA dal nemico e
		// si ferma davanti. Si generano quindi dai bersagli, chiedendo al resolver se la traiettoria li
		// raggiunge — lo stesso codice che poi la eseguira'.
		if (bDashReady && DashStyle == ERTMovementStyle::LinearCharge)
		{
			const int32 ImpactDamage = URTCatalogLibrary::FirstDamage(DashAb->Def);
			for (int32 e = 0; e < Ctx.Enemies.Num(); ++e)
			{
				// Occupazione dalla BASE, come per `DashSnapshot`: la carica risolve nella fase Dash, e una
				// cella prenotata per il Move di una compagna li' e' ancora vuota. Con `Snapshot.Occupancy`
				// la traiettoria si fermerebbe su un corpo che non c'e' e la candidata sparirebbe in silenzio.
				const FRTLinearMoveResult Linear = URTMovementActionLibrary::ResolveLinearMove(
					BaseSnapshot.Map, Bot.Cell, Ctx.Enemies[e], DashBudget, DashStyle,
					BaseSnapshot.Occupancy, DashHostiles);

				// Vale solo se l'impatto colpisce PROPRIO quel nemico: una traiettoria che ne incontra un altro
				// prima e' una candidata diversa, e la genera il suo giro di ciclo.
				if (Linear.Stop != ERTLinearStop::Impact
					|| !EnemyUnitIndex.IsValidIndex(e) || Facts[EnemyUnitIndex[e]].Index != Linear.ImpactUnitId)
				{
					continue;
				}

				FRTHexBotPlan Charge;
				Charge.DestCell = Linear.Final;   // dove il bot si ferma: adiacente al bersaglio
				Charge.bHasAttack = true;
				Charge.TargetIndex = e;
				Charge.AttackDamage = ImpactDamage;
				Charge.TargetHealth = Ctx.EnemyHealth.IsValidIndex(e) ? Ctx.EnemyHealth[e] : 0;
				Plans.Add(Charge);
				PlanAbility.Add(INDEX_NONE);      // il colpo NON e' una seconda azione: e' l'impatto della carica
				PlanViaDash.Add(true);
				PlanIsCharge.Add(true);
			}
		}

		// 4) Scatto + attacco, e scatto per riposizionarsi.
		//
		// NOTA (#145, aggiornata da D-028): scatto e attacco sono ora slot DIVERSI — movimento e principale —
		// quindi pianificarli insieme e' legale, ed e' la scelta *schivo e sparo*. Il prezzo c'e' e non e' piu'
		// implicito: chi scatta non prosegue col Move (lo applica il resolver piu' sotto), chi carica si.
		//
		// Resta il problema di bilanciamento che la nota segnalava, e resta misurato sugli ARCHETIPI: per il
		// Guardian «scatto + Sweep» fa 30 danni e spinta 2 con cooldown 0, la Charge 20 e spinta 1 con
		// cooldown 3. Sul roster eroi i numeri sono altri. Il meccanismo qui sopra e' corretto; a renderlo
		// utile e' il bilanciamento — voce `BAL-1` del backlog, che parte da una misura e non da una correzione.
		if (bDashReady)
		{
			for (int32 A = 0; A < Bot.NumAbilities(); ++A)
			{
				const URTActionData* Ability = Bot.GetAbility(A);
				if (!Ability || URTCatalogLibrary::IsFastMovement(Ability->Def) || Ability->bSelfTarget
					|| !Bot.CanUseAbility(A)) { continue; }
				AddCandidates(DashSnapshot, A, Ability->RangeCells, Ability->Power, /*bViaDash*/ true, /*bAttacksOnly*/ true);
			}
			AddCandidates(DashSnapshot, INDEX_NONE, /*Range*/ 0, /*Damage*/ 0, /*bViaDash*/ true, /*bAttacksOnly*/ false);
		}

		const FRTHexBotPlan Best = URTHexBotLibrary::ChooseBestPlan(Snapshot.Map, Plans, Ctx);

		// Da quale candidata viene il piano scelto (per sapere abilita' e se passa dallo scatto). Le candidate
		// di movimento normale sono in testa: a parita' di campi si preferisce NON consumare lo scatto.
		int32 BestIdx = INDEX_NONE;
		for (int32 p = 0; p < Plans.Num(); ++p)
		{
			if (Plans[p].DestCell == Best.DestCell && Plans[p].bHasAttack == Best.bHasAttack
				&& Plans[p].TargetIndex == Best.TargetIndex && Plans[p].AttackDamage == Best.AttackDamage)
			{
				BestIdx = p;
				break;
			}
		}

		const bool bViaDash = Plans.IsValidIndex(BestIdx) && PlanViaDash[BestIdx];
		const bool bIsCharge = Plans.IsValidIndex(BestIdx) && PlanIsCharge[BestIdx];
		const int32 BestAbility = Plans.IsValidIndex(BestIdx) ? PlanAbility[BestIdx] : INDEX_NONE;
		const FRTBotUnitFacts* Target = (Best.bHasAttack && EnemyUnitIndex.IsValidIndex(Best.TargetIndex))
			? &Facts[EnemyUnitIndex[Best.TargetIndex]] : nullptr;
		const int32 Score = URTHexBotLibrary::ScorePlan(Snapshot.Map, Best, Ctx);

		// Il TERMINE d'obiettivo accanto al totale, non dentro (`#2269`).
		//
		// 🔴 **E' la proprieta' che `spec-bot-tattico.md` §5 chiede, e il motivo per cui la chiede.** Un
		// `score=-40` senza righe e' indebuggabile: quando il bot sbaglia non si sa QUALE termine ha vinto, e
		// si finisce a ritoccare i pesi a caso. Il difetto che questa issue chiude e' stato diagnosticato
		// esattamente cosi' — leggendo `utility -> (q=0,r=-3,L0) score=-40` e non potendo dire se quella cella
		// avesse vinto per l'obiettivo (impossibile: il termine non esisteva) o per avvicinamento e quota.
		//
		// ⚠️ **Il breakdown COMPLETO e' lavoro di E26**, e questa e' una riga sola: si scrive quando pesa,
		// cosi' che ogni partita su una mappa senza obiettivi produca un log identico a prima. La differenza
		// fra «il termine vale zero» e «il termine non c'e'» qui non si vede — e per una mappa senza obiettivi
		// e' la stessa cosa.
		const int32 ObjectiveTerm = URTHexBotLibrary::ScoreObjectiveTerm(Snapshot.Map, Best.DestCell, Ctx);
		const FString ObjectiveNote = ObjectiveTerm > 0
			? FString::Printf(TEXT(" [obiettivo +%d]"), ObjectiveTerm)
			: FString();

		// La memoria si aggiorna UNA VOLTA per round: `PlanBotsForTest()` e `LockInAndResolve()`
		// pianificano entrambi lo stesso round, e senza guardia il decadimento andrebbe al doppio.
		{
			int32& UltimoRound = IdleRound.FindOrAdd(Bot.StableUnitId, -1);
			if (UltimoRound != TurnNumber)
			{
				UltimoRound = TurnNumber;
				int32& TurniInerti = IdleTurns.FindOrAdd(Bot.StableUnitId, 0);
				TurniInerti = Best.bHasAttack ? 0 : TurniInerti + 1;
			}
		}

		// Il bersaglio su cui il piano vincente AGISCE: lo valorizzano i tre rami che attaccano (carica,
		// scatto+attacco, attacco da fermo) e nessun altro. E' cio' che [D-313] chiama «la scelta».
		const FRTBotUnitFacts* Scelto = nullptr;

		if (bIsCharge && Target && Ctx.Enemies.IsValidIndex(Best.TargetIndex))
		{
			Scelto = Target;
			// CARICA: si punta la cella del bersaglio e la fase Dash si ferma addosso a lui registrando
			// l'impatto. Nessun `PlannedAbilityIndex`: il colpo e' dell'azione di movimento, e pianificare
			// anche un'azione principale significherebbe spendere due volte lo stesso slot.
			Piano.PlannedDashAbility = DashIdx;
			Piano.PlannedDashCell = Ctx.Enemies[Best.TargetIndex];
			// Il soggetto e' il BOT, non il bersaglio: e' la sua posizione e la sua intenzione che trapelano
			// qui. Il bersaglio e' gia' filtrato dalla riga che lo riguarda.
			Esito.LogLines.Add(FRTBotLogLine{FString::Printf(TEXT("%s: utility -> CARICA su %s (impatto da (q=%d,r=%d,L%d)) score=%d%s"),
				*Bot.DisplayName, *Target->DisplayName, Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, Score,
				*ObjectiveNote), Bot.Index});
		}
		else if (bViaDash && Target && BestAbility != INDEX_NONE)
		{
			// Scatta (fase Dash) e attacca dalla cella post-scatto: nel Blast, che segue il Dash, il bot e' li'.
			Piano.PlannedDashAbility = DashIdx;
			Piano.PlannedDashCell = Best.DestCell;
			Piano.PlannedAbilityIndex = BestAbility;
			// Il bot dichiara un bersaglio-UNITA', e la forma opposta si ritira con esso (`#2884`): il suo
			// piano nasce da `PlanBots`, che azzera gia' tutto, ma la simmetria col percorso del giocatore
			// vale piu' di una riga risparmiata — un secondo produttore che scriva il campo grezzo e' il modo
			// in cui l'esclusivita' torna a essere una convenzione.
			Piano.PlannedAttackTargetIndex = Target->Index;
			Scelto = Target;
			// Soggetto = il BOT (vedi nota sulla CARICA sopra).
			Esito.LogLines.Add(FRTBotLogLine{FString::Printf(TEXT("%s: utility -> scatto (q=%d,r=%d,L%d) + attacca %s score=%d%s"),
				*Bot.DisplayName, Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, *Target->DisplayName, Score,
				*ObjectiveNote), Bot.Index});
		}
		else if (Target && BestAbility != INDEX_NONE)
		{
			// Resta e attacca dalla cella attuale (Best.DestCell == cella d'origine).
			Piano.PlannedCell = Best.DestCell;
			Piano.PlannedAbilityIndex = BestAbility;
			Piano.PlannedAttackTargetIndex = Target->Index; // come sopra (`#2884`)
			Scelto = Target;
			// Soggetto = il BOT (vedi nota sulla CARICA sopra).
			Esito.LogLines.Add(FRTBotLogLine{FString::Printf(TEXT("%s: utility -> (q=%d,r=%d,L%d) attacca %s score=%d%s"),
				*Bot.DisplayName, Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, *Target->DisplayName, Score,
				*ObjectiveNote), Bot.Index});
		}
		else if (bViaDash)
		{
			// Riposizionamento rapido con lo scatto (nessun tiro disponibile da nessuna cella).
			Piano.PlannedDashAbility = DashIdx;
			Piano.PlannedDashCell = Best.DestCell;
			Esito.LogLines.Add(FRTBotLogLine{FString::Printf(TEXT("%s: scatto -> (q=%d,r=%d,L%d) score=%d%s"),
				*Bot.DisplayName, Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, Score,
				*ObjectiveNote), Bot.Index});
		}
		else
		{
			// Posizionamento con il movimento normale (o "resta", se l'utility preferisce la cella attuale).
			Piano.PlannedCell = Best.DestCell;
			Esito.LogLines.Add(FRTBotLogLine{FString::Printf(TEXT("%s: utility -> (q=%d,r=%d,L%d) score=%d%s%s"),
				*Bot.DisplayName, Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, Score,
				Best.DestCell == Bot.Cell ? TEXT(" (resta)") : TEXT(""),
				*ObjectiveNote), Bot.Index});
		}

		// [D-313] — si chiude il record con il bersaglio SCELTO, quando c'e'.
		//
		// 🔴 **`Target`, non `Piano.PlannedAttackTargetIndex`**: il ramo della CARICA non scrive quel campo, perche'
		// il colpo e' dell'azione di movimento e non di una principale. Leggere il campo avrebbe perso
		// l'intera classe di scelte piu' aggressiva del bot — proprio quella su cui la domanda d'equita'
		// morde di piu' — archiviandola come «nessun bersaglio», cioe' come niente da giudicare.
		//
		// ⚠️ Si prendono solo i tre rami che AGISCONO su di lui: `Target` e' valorizzato anche quando il piano
		// vincente non attacca, e un riposizionamento non e' una scelta di bersaglio.
		if (IdxScelta != INDEX_NONE && Scelto && Esito.AuditDecisions.IsValidIndex(IdxScelta))
		{
			FRTAuditBotDecision& Record = Esito.AuditDecisions[IdxScelta];
			Record.TargetUnitId = Scelto->StableUnitId;
			Record.TargetTeamId = Scelto->TeamId;
			// ⚠️ La cella e' quella VERA del bersaglio adesso, non quella che il bot ricordava: e' l'ingresso
			// che il cancello di produzione passa a `ClassifyTarget`, e con un'altra il ricalcolo porrebbe
			// una domanda simile invece della stessa.
			Record.TargetCell = Scelto->Cell;
		}

		// #1088 — l'ultima cosa che il bot fa: dichiarare alle compagne dove sta andando. Copre i quattro
		// rami qui sopra; i due `continue` piu' in alto prenotano per conto proprio, perche' escono prima.
		ReserveNormalMove(Snapshot, Piano, Bot.Cell, Bot.Index);
	}

	return Esito;
}
