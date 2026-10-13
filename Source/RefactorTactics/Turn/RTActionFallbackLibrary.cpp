#include "Turn/RTActionFallbackLibrary.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

ERTActionInvalidReason URTActionFallbackLibrary::ValidateInstance(const FRTActionInstance& Instance,
	const TArray<FRTHexCombatUnit>& Units, const URTHexMapAsset* Map)
{
	// Un'azione senza bersaglio-unita' non ha nulla da validare qui: il movimento lo giudica il resolver dei
	// percorsi, il supporto su se stessi non puo' "perdere il bersaglio".
	if (Instance.TargetUnitId == INDEX_NONE)
	{
		return ERTActionInvalidReason::None;
	}

	if (!Units.IsValidIndex(Instance.SourceUnitId) || !Units.IsValidIndex(Instance.TargetUnitId))
	{
		return ERTActionInvalidReason::TargetGone;
	}

	const FRTHexCombatUnit& Source = Units[Instance.SourceUnitId];
	const FRTHexCombatUnit& Target = Units[Instance.TargetUnitId];

	// Su se stessi (scudi, stance) non si valuta ne' squadra ne' portata: si e' sempre a portata di se'.
	if (Instance.SourceUnitId == Instance.TargetUnitId)
	{
		return ERTActionInvalidReason::None;
	}

	if (!Target.bAlive)
	{
		return ERTActionInvalidReason::TargetDead;
	}
	if (Target.TeamId == Source.TeamId)
	{
		return ERTActionInvalidReason::TargetFriendly;
	}

	// Portata PRIMA della linea di tiro: sono due difetti diversi da correggere per chi gioca, e attribuire
	// ogni rifiuto alla copertura renderebbe il log una bugia (stessa disciplina di ClassifyHexTargeting).
	// La portata e' quella EFFETTIVA: il terreno (Fumo) la cappa. Il controllo NON passa da ClassifyHexTargeting
	// (qui serve distinguere anche NoMap da OutOfRange con quest'ordine), quindi la regola si chiama diretta —
	// stessa funzione condivisa, nessuna seconda copia della logica. Con `Map == nullptr` il cap e' un no-op e
	// l'ordine dei motivi resta quello di prima.
	//
	// [D-415]/[D-419] — PORTATA E LINEA DI TIRO SI MISURANO SULLA MIRA CONGELATA, `Instance.TargetCell`, non
	// sulla cella VIVA del bersaglio. 🔴 Fino al 2026-10-09 questa funzione leggeva `Target.Cell` mentre il
	// Blast sparava su `Intent.TargetCell`: due celle decidevano due meta' dello stesso colpo, e il golden
	// `Visual.Combat.FallbackTargetMoved` aveva archiviato `OutOfRange` su una cella in portata — trovato
	// dalla code review di #3230. Quando l'azione dichiara l'aggancio (`ERTActionFallback::AttackTarget`)
	// `TargetCell` e' gia' la cella viva, scritta da `CollectAttackIntents`: qui non si sceglie, si legge.
	const FRTCellId& Aim = Instance.TargetCell;
	const int32 EffectiveRange = URTTerrainLibrary::EffectiveTargetingRange(Map, Source.Cell, Aim,
		Instance.Def.RangeCells);
	if (URTHexLibrary::HexDistance(Source.Cell, Aim) > EffectiveRange)
	{
		return ERTActionInvalidReason::OutOfRange;
	}

	// FAIL-CLOSED: senza mappa autorevole non si valida la traiettoria, quindi non si colpisce. Il motivo resta
	// distinto da una copertura: e' un difetto del livello, non un esito di gioco.
	if (Map == nullptr)
	{
		return ERTActionInvalidReason::NoMap;
	}
	// La policy viene dal `Def` dell'istanza, cioe' dalla stessa riga di catalogo che il planning ha letto
	// (`#2870`, [D-378]). ⚠️ **Qui morde solo per i bersagli-UNITA'**: un bersaglio a cella e' gia' uscito
	// con `None` in cima a questa funzione, e la sua linea di tiro la giudica `CollectHexAttacks`, che e'
	// l'owner della geometria dei colpi a cella. Passarla comunque non e' ridondanza: e' cio' che tiene il
	// dato unico se domani un'azione mirata a un'unita' dichiarera' il tiro indiretto.
	//
	// D-490 (#3608): qui si giudica un colpo che parte, quindi conta la TRAIETTORIA e non la licenza di mira —
	// `BlindAimDirect` ha mirato al buio, ma il primo ostacolo lo ferma come `Required`.
	//
	// 🔴 **OGNI VERDETTO HA UN RAMO, E CIO' CHE NON NE HA UNO NON E' UN VIA LIBERA** ([D-509], `#2951`).
	// Fino al 2026-10-13 questa riga era `Classify(...) == NoLineOfSight`, seguita da `return None`: ogni
	// altro verdetto del classificatore cadeva nel ramo «valido». Quando `#2951` gli ha aggiunto
	// `OtherLayer`, la casella *«i chiamanti leggono l'esito e non ricalcolano il predicato»* era vera alla
	// lettera — e in risoluzione la regola del piano non esisteva: una mira fra piani era un'azione valida.
	//
	// ⛔ **Lo `switch` senza `default` NON ferma la build**, e la prima stesura di questo commento lo
	// prometteva: su questo stesso enum il repository l'ha gia' misurato falso (`#3080`, in
	// `URTCombatLibrary::RefusalForReason`) — MSVC emette C4061/C4062, un warning che la build non alza.
	// Cio' che regge e' un banco: `Actions.Fallback.ValidationCoversEveryTargetingVerdict` legge i verdetti
	// dalla reflection, e uno senza un esito dichiarato lo rende rosso nominandolo.
	switch (URTCombatLibrary::ClassifyHexTargeting(Map, Source.Cell, Aim, Instance.Def.RangeCells,
		URTCombatLibrary::TrajectoryPolicy(Instance.Def.LineOfSightPolicy)))
	{
	case ERTHexTargetReason::Ok:            return ERTActionInvalidReason::None;
	// I due qui sotto sono gia' usciti piu' in alto, con lo stesso criterio: restano scritti perche' il
	// verdetto del classificatore non diventi «valido» il giorno in cui quei controlli si spostano.
	case ERTHexTargetReason::NoMap:         return ERTActionInvalidReason::NoMap;
	case ERTHexTargetReason::OutOfRange:    return ERTActionInvalidReason::OutOfRange;
	// Come `NoLineOfSight`: chi chiama non applica un fallback, lascia il motivo alla raccolta dei colpi.
	case ERTHexTargetReason::OtherLayer:    return ERTActionInvalidReason::OtherLayer;
	case ERTHexTargetReason::NoLineOfSight: return ERTActionInvalidReason::NoLineOfSight;
	// La distanza MINIMA non si rivaluta in risoluzione: il parametro non viene passato, quindi il
	// classificatore non puo' rispondere cosi'. Non e' una regola decisa qui — e' lo stato di `#2950`.
	case ERTHexTargetReason::TooClose:      return ERTActionInvalidReason::None;
	}

	// Qui si arriva solo con un verdetto che lo `switch` non conosce. FAIL-CLOSED: e' un rifiuto, e in
	// sviluppo e' rumoroso. ⚠️ Il motivo e' `OutOfRange` per una sola ragione, e va detta: fra quelli che
	// esistono e' un rifiuto di GEOMETRIA che il Blast passa al fallback dichiarato — e con `Cancel`,
	// `AttackCell`, `AttackTarget`, `BasicAttack` e `Wait` nessun colpo parte. `NoMap` e `NoLineOfSight` il
	// Blast li lascia alla raccolta, che su un verdetto ignoto non saprebbe cosa fermare. Il testo nel log
	// sarebbe sbagliato; un colpo partito per distrazione sarebbe peggio.
	//
	// ⛔ **Non e' chiuso per `Stop`**, che lascia l'istanza intatta: e' la regola del
	// MOVIMENTO, e nessuna azione del Blast la dichiara — il validatore di catalogo la impone ai movimenti e
	// non la vieta agli altri. Trovato dalla quarta review; il giorno in cui un attacco dichiarasse `Stop`,
	// questo ramo non lo fermerebbe.
	checkNoEntry();
	return ERTActionInvalidReason::OutOfRange;
}

FRTFallbackResult URTActionFallbackLibrary::ApplyFallback(const FRTActionInstance& Instance,
	ERTActionInvalidReason Reason)
{
	FRTFallbackResult Result;
	Result.Instance = Instance;
	Result.Applied = Instance.Def.Fallback;
	Result.bProducesEffects = false;

	// Azione ancora valida: nessun fallback da applicare, si esegue quella pianificata.
	if (Reason == ERTActionInvalidReason::None)
	{
		Result.bProducesEffects = true;
		return Result;
	}

	switch (Instance.Def.Fallback)
	{
	case ERTActionFallback::Stop:
		// Regola del movimento: si procede fino all'ultima cella valida. L'istanza resta intatta — a fermarla
		// e' il resolver dei percorsi, che avanza a micro-step e non ricalcola nulla.
		Result.bProducesEffects = true;
		break;

	case ERTActionFallback::AttackCell:
	{
		// L'area parte comunque: si perde il bersaglio, si tiene la CELLA pianificata. Dove hai puntato, li'
		// colpisci — ma solo se il problema era il BERSAGLIO. Se era la geometria (fuori portata, traiettoria
		// bloccata, nessuna mappa) la cella ha lo stesso difetto, e colpirla vorrebbe dire sparare attraverso
		// il muro o oltre la propria portata. `TargetGone` e' escluso perche' senza il bersaglio non si sa
		// nemmeno DOVE fosse: non c'e' una cella su cui ripiegare.
		//
		// `TargetUnknown` (CP 13.2) e' escluso per una ragione piu' forte delle altre, e va detta: la cella
		// pianificata contro un bersaglio che la squadra non conosce e' informazione che il giocatore non
		// avrebbe dovuto avere. Ripiegarci sopra trasformerebbe il rifiuto in un modo per colpire l'invisibile
		// «per approssimazione», cioe' in una fuga di posizione con un altro nome. Un bersaglio soltanto
		// INCERTO non arriva mai qui: viene convertito in attacco a cella sulla sua ultima cella NOTA prima
		// della validazione, che e' un colpo legittimo e non un fallback.
		const bool bCellStillUsable = (Reason == ERTActionInvalidReason::TargetDead
			|| Reason == ERTActionInvalidReason::TargetFriendly);
		if (!bCellStillUsable)
		{
			Result.Instance.Def.Effects.Reset();
			Result.Instance.TargetUnitId = INDEX_NONE;
			Result.Applied = ERTActionFallback::Cancel;
			break;
		}
		Result.Instance.TargetUnitId = INDEX_NONE;
		Result.bProducesEffects = true;
		break;
	}

	case ERTActionFallback::AttackTarget:
		// «Segue il bersaglio, se ancora valido»: qui il bersaglio NON e' valido (siamo nel fallback perche'
		// la validazione e' fallita), quindi non c'e' nessuno da seguire e l'azione si annulla. Seguire
		// qualcun altro sarebbe targeting automatico.
		Result.Instance.Def.Effects.Reset();
		Result.Instance.TargetUnitId = INDEX_NONE;
		Result.Applied = ERTActionFallback::Cancel;
		break;

	case ERTActionFallback::Wait:
	{
		// Sostituisce l'azione con `Action.Wait`: presa dal catalogo, non inventata qui. Occupa lo slot, non
		// produce effetti, e resta osservabile nel TurnLog.
		FRTActionDef WaitDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Wait"));
		if (WaitDef.ActionId.IsNone())
		{
			WaitDef = Instance.Def;
			WaitDef.Effects.Reset(); // catalogo incompleto: meglio un'azione vuota che una che agisce a caso
		}
		Result.Instance.Def = WaitDef;
		Result.Instance.TargetUnitId = INDEX_NONE;
		break;
	}

	case ERTActionFallback::BasicAttack:
		// Non praticabile in v0.1: «il bersaglio valido piu' vicino» e' targeting automatico, che il catalogo
		// §17 elenca fra gli errori da evitare (esiti poco leggibili). Degrada a Cancel e lo DICHIARA, cosi'
		// il log dice cosa e' successo davvero invece di far sparire l'azione.
		Result.Instance.Def.Effects.Reset();
		Result.Instance.TargetUnitId = INDEX_NONE;
		Result.Applied = ERTActionFallback::Cancel;
		break;

	case ERTActionFallback::Cancel:
		// Attacchi diretti e cure: non si esegue nulla. Nessun effetto, nessun bersaglio di ripiego.
		Result.Instance.Def.Effects.Reset();
		Result.Instance.TargetUnitId = INDEX_NONE;
		break;
	}

	// Nessun ramo qui sopra guarda le altre unita': e' voluto. Un fallback che SCEGLIE un bersaglio sarebbe
	// targeting automatico, e la firma senza il contesto delle unita' rende la regola impossibile da violare
	// per distrazione — non solo sconsigliata a parole.
	return Result;
}

ERTFallbackOutcome URTActionFallbackLibrary::ToLogOutcome(ERTActionFallback Fallback)
{
	// Un caso per valore, nessun `default`: aggiungere un fallback senza dargli un esito di log non compila.
	switch (Fallback)
	{
	case ERTActionFallback::Stop:         return ERTFallbackOutcome::Stopped;
	case ERTActionFallback::Wait:         return ERTFallbackOutcome::Waited;
	case ERTActionFallback::AttackCell:   return ERTFallbackOutcome::AttackedCell;
	case ERTActionFallback::AttackTarget: return ERTFallbackOutcome::Cancelled; // nessun bersaglio da seguire
	case ERTActionFallback::BasicAttack:  return ERTFallbackOutcome::Cancelled; // niente targeting automatico
	case ERTActionFallback::Cancel:       return ERTFallbackOutcome::Cancelled;
	}
	return ERTFallbackOutcome::Cancelled;
}
