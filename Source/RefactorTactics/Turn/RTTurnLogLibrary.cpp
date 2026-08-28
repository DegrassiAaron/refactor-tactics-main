#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo del fallback, leggibile nel log
#include "Turn/RTReactionLibrary.h" // ERTReactionOutcome: l'esito di una reazione, leggibile nel log
#include "Core/RTGameplayTags.h" // TAG_Status_Burning: la causa ambientale si CHIEDE al tag, non si riscrive
#include "Misc/FileHelper.h"
#include "Containers/ArrayView.h" // i campi discriminanti viaggiano come una vista, non come copie
#include "Templates/Function.h"   // TFunctionRef: il visitor dei campi non alloca
#include "Core/RTEnumName.h" // RTReflection::EnumName: i nomi degli enum si CHIEDONO, non si ricopiano

bool URTTurnLogLibrary::EntryLess(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B)
{
	// Ordine totale: confronta ogni campo in sequenza, cosi' due voci diverse hanno sempre un ordine definito
	// (permutare l'input e riordinare -> stessa sequenza). Enum confrontati per valore intero (invariante #4).
	if (A.Phase != B.Phase)                 { return static_cast<uint8>(A.Phase) < static_cast<uint8>(B.Phase); }
	if (A.Category != B.Category)           { return static_cast<uint8>(A.Category) < static_cast<uint8>(B.Category); }
	if (A.SrcCell.X != B.SrcCell.X)         { return A.SrcCell.X < B.SrcCell.X; }
	if (A.SrcCell.Y != B.SrcCell.Y)         { return A.SrcCell.Y < B.SrcCell.Y; }
	if (A.SrcCell.Layer != B.SrcCell.Layer) { return A.SrcCell.Layer < B.SrcCell.Layer; }
	if (A.TgtCell.X != B.TgtCell.X)         { return A.TgtCell.X < B.TgtCell.X; }
	if (A.TgtCell.Y != B.TgtCell.Y)         { return A.TgtCell.Y < B.TgtCell.Y; }
	if (A.TgtCell.Layer != B.TgtCell.Layer) { return A.TgtCell.Layer < B.TgtCell.Layer; }
	if (A.Outcome != B.Outcome)             { return A.Outcome < B.Outcome; }
	if (A.Amount != B.Amount)               { return A.Amount < B.Amount; }
	// Confronto LESSICOGRAFICO (`FName::Compare`), mai `FastLess`: quello ordina per indice nella name table,
	// che dipende dall'ordine in cui i nomi sono stati creati nel processo — due esecuzioni della stessa
	// partita darebbero due ordini diversi, cioe' due hash diversi (#4).
	if (A.ActionId != B.ActionId) { return A.ActionId.Compare(B.ActionId) < 0; }

	// I campi della v6 chiudono l'ordine. NON e' un dettaglio estetico: `SerializeTurnLog` li SCRIVE, e un
	// campo scritto che il confronto non guarda lascia due voci a pari merito — dove a decidere l'ordine
	// resta `TArray::Sort`, che non e' stabile. Due inserimenti diversi produrrebbero due file diversi con
	// lo stesso contenuto, cioe' esattamente cio' che `D-SR-1` promette non accada.
	//
	// `BaseActionId` resta fuori e non e' un'omissione: e' una FUNZIONE di `ActionId`, quindi due voci che
	// pareggiano su `ActionId` pareggiano anche su di lui e non c'e' niente da spareggiare.
	if (A.TurnNumber != B.TurnNumber)     { return A.TurnNumber < B.TurnNumber; }
	if (A.GraphRevision != B.GraphRevision) { return A.GraphRevision < B.GraphRevision; }
	if (A.UnitId != B.UnitId)             { return A.UnitId < B.UnitId; }
	// `Priority` (v7) chiude l'ordine per la stessa ragione dei tre campi qui sopra: e' SCRITTO da
	// `SerializeTurnLog`, e un campo scritto che il confronto non guarda lascia due voci a pari merito.
	if (A.Priority != B.Priority) { return A.Priority < B.Priority; }

	// I tre campi della v8, per la stessa ragione ancora: sono scritti, quindi devono spareggiare. Due
	// decisioni della **stessa** unita' nello stesso micro-step — due Overwatch armate — pareggiano su tutto
	// il resto e si distinguono solo qui.
	if (A.OpportunityId != B.OpportunityId) { return A.OpportunityId < B.OpportunityId; }
	if (A.ReactionInstanceId != B.ReactionInstanceId) { return A.ReactionInstanceId < B.ReactionInstanceId; }
	if (A.SelectedTargetUnitId != B.SelectedTargetUnitId) { return A.SelectedTargetUnitId < B.SelectedTargetUnitId; }

	// 🔴 **Il campo della v9 spareggia come tutti gli altri**, e la prima stesura lo aveva dimenticato — cioe'
	// aveva rifatto il difetto che il commento in cima a questa funzione descrive per `UnitId` in v6. Oggi il
	// caso non e' raggiungibile (con un solo produttore, `SrcCell` — la cella del protetto — rompe sempre la
	// parita' prima di arrivare qui), ma l'assertion nuova dichiara di valere per «qualunque redirect che un
	// giorno venisse aggiunto»: il primo che ne emettesse due pareggianti farebbe dipendere i **byte** del
	// file, e il suo checksum, dall'ordine d'inserimento. `TArray::Sort` non e' stabile, e `D-SR-1` cadrebbe.
	// Trovato da una code review; `TurnLog.CanonicalOrderCoversSerializedFields` esiste per questo.
	if (A.OriginalTargetUnitId != B.OriginalTargetUnitId)
	{
		return A.OriginalTargetUnitId < B.OriginalTargetUnitId;
	}

	// 🔴 **E il campo della v10 spareggia come tutti gli altri.** Questa riga e' stata scritta **subito**, e
	// non e' merito: e' che il commento qui sopra racconta la stessa svista fatta con la v9 — un campo
	// SCRITTO da `SerializeTurnLog` che il confronto non guarda lascia due voci a pari merito, e `TArray::Sort`
	// non e' stabile. Il file lo aveva gia' pagato una volta, e questa sarebbe stata la seconda.
	//
	// 🔴 **`Compare(..., CaseSensitive)` e NON `operator<`**, ed e' una correzione di una prima stesura che
	// aveva rifatto il difetto di `FName::FastLess` descritto in cima a questa funzione — in un'altra forma.
	// Misurato nel sorgente dell'engine: `FString::UEOpLessThan` e' `FPlatformString::Stricmp(...) < 0`, con
	// un `@note case insensitive` esplicito (`UnrealString.h.inl:873`). Quindi `operator<` **non e' un ordine
	// totale** sui byte: due token che differiscono solo per il caso pareggerebbero in entrambi i versi,
	// resterebbero a pari merito, e `TArray::Sort` — che non e' stabile — deciderebbe secondo l'ordine
	// d'inserimento. I byte del file e il suo checksum ne dipenderebbero: esattamente il buco di `D-SR-1` che
	// questa riga esiste per chiudere.
	//
	// ⚠️ E il resto della pipeline confronta le risposte **case-sensitive** — `IsResponseAllowed` e
	// `ClassifyChosenResponse` passano entrambe `ESearchCase::CaseSensitive`: con `operator<` due token che
	// il resolver tratta come risposte DIVERSE sarebbero uguali per l'ordine canonico, cioe' due verita' sullo
	// stesso dato.
	//
	// ⚠️ **Oggi il caso non e' raggiungibile, e la ragione va detta bene perche' la prima stesura la aveva
	// scritta al contrario**: due voci che pareggiassero fin qui condividono l'`OpportunityId`, che porta
	// l'`OwnerId` e il micro-step — quindi sono della stessa unita' nello stesso istante, e la `SrcCell` e'
	// **identica**, non diversa. A separarle e' `ReactionInstanceId` per l'Overwatch; il produttore del
	// `Brace` lo lascia a `INDEX_NONE`, quindi per lui nemmeno quello. Cio' che rende il caso irraggiungibile
	// e' un'altra cosa, gia' scritta in `ResolveReactionBoundary`: un'unita' pianifica **una sola** abilita'
	// per turno. La riga esiste per il giorno in cui quella premessa cadesse.
	return A.ReactionResponse.Compare(B.ReactionResponse, ESearchCase::CaseSensitive) < 0;
}

void URTTurnLogLibrary::SortTurnLog(TArray<FRTTurnLogEntry>& Entries)
{
	Entries.Sort([](const FRTTurnLogEntry& A, const FRTTurnLogEntry& B) { return EntryLess(A, B); });
}

bool URTTurnLogLibrary::IsEnvironmentalDamage(const FRTTurnLogEntry& Entry)
{
	if (Entry.Category != ERTLogCategory::Combat || Entry.UnitId == 0)
	{
		return false;
	}

	// 1) La CAUSA dichiarata. `Status.Burning` arriva dal tag e non da un letterale: il produttore scrive
	//    `TAG_Status_Burning.GetTag().GetTagName()`, e due stringhe uguali per abitudine divergono al primo
	//    rename (`D-098`). La prima stesura di questa funzione aveva tolto il letterale del prefisso e
	//    lasciato questo — meta' del difetto corretta. Trovato in code review.
	//
	//    ⚠️ Il confronto e' CASE-INSENSITIVE in entrambi i rami. `FName::ToString()` non e' stabile nel caso
	//    fuori dall'editor (`WITH_CASE_PRESERVING_NAME`): restituisce il caso della PRIMA registrazione di
	//    quel comparison name, che una traccia deserializzata o uno scenario possono aver fissato altrove.
	//    Un `CaseSensitive` qui sarebbe verde in automation e falso nel pacchettizzato.
	const FString Causa = Entry.ActionId.ToString();
	if (Causa.StartsWith(TerrainCausePrefix(), ESearchCase::IgnoreCase)
		|| Entry.ActionId == TAG_Status_Burning.GetTag().GetTagName())
	{
		return true;
	}

	// 2) La RETE, e serve perche' l'elenco qui sopra fallisce APERTO. Una causa ambientale nuova — `#1077`
	//    sta portando gli stati nel TurnLog — che nessuno aggiungesse a (1) verrebbe classificata come danno
	//    INFLITTO, cioe' accreditata a chi la subisce: il verso pericoloso, e senza che niente lo segnali.
	//    Trovato in code review.
	//
	//    La forma di una voce ambientale e' che le due celle COINCIDONO: chi subisce e' anche il «soggetto».
	//    Un attacco non puo' averle uguali — `SrcCell` e' la cella di chi colpisce, `TgtCell` quella di chi
	//    e' colpito.
	//
	//    ⛔ **NON e' il discriminante primario, ed e' importante che resti secondo.** `AppendLogEntry`
	//    dichiara che `SrcCell` non identifica l'unita', con tre controesempi; costruirci sopra la regola
	//    sarebbe l'inferenza che il formato ha smesso di sostenere quando `UnitId` e' nato (`D-063`). Qui
	//    serve solo a far fallire CHIUSO cio' che l'elenco non conosce.
	//
	//    ⚠️ **Un falso positivo noto**: un'area con fuoco amico che investa la cella di chi la lancia
	//    produce `SrcCell == TgtCell` su un danno che l'attore ha davvero inflitto. Contarlo come «subito»
	//    sottostima il danno inflitto invece di gonfiarlo — la direzione innocua fra le due — e la
	//    alternativa (fallire aperto su ogni status nuovo) e' peggiore.
	return Entry.SrcCell == Entry.TgtCell;
}

bool URTTurnLogLibrary::IsDamageInflictedByActor(const FRTTurnLogEntry& Entry)
{
	// «Nessuna unita' dichiarata» non e' un attore: `AppendLogEntry` scrive `0` quando l'attore e' `nullptr`,
	// e diversi siti di combattimento possono passarlo. Senza questa guardia un'aggregazione per unita'
	// produce un'unita' fantasma `0` che regge danno vero. Trovato in code review.
	if (Entry.UnitId == 0 || IsEnvironmentalDamage(Entry))
	{
		return false;
	}

	// 🔴 **Il danno inflitto NON vive solo in `Combat`, e la prima stesura lo assumeva.** Overwatch lo scrive
	// come `ReactionDecision` (`Entry.Amount = Armed.Damage`, attore `WatchOwner`) e la previsione come
	// `Predictive` (attore `Shooter`): entrambi danno vero, inflitto dall'unita' in `UnitId`. Filtrando la
	// sola `Combat`, chi aggrega otteneva **zero** per un `InterceptShot` andato a segno — lo stesso «numero
	// plausibile e sbagliato» che `#1150` esiste per impedire, nel verso opposto. Trovato in code review.
	//
	// ⚠️ **L'esito si legge per categoria, e `Amount` da solo non basta.** In `Fallback` quel campo porta un
	// `ERTActionInvalidReason`, non un danno: un predicato «`Amount > 0`» sommerebbe codici di errore.
	switch (Entry.Category)
	{
	case ERTLogCategory::Combat:
		switch (static_cast<ERTCombatOutcome>(Entry.Outcome))
		{
		// I quattro esiti che portano danno inflitto. `Healed` e `NoLineOfSight` restano fuori, e non e'
		// ovvio in nessuno dei due: la cura ha un agente vero ma non e' danno; un attacco fermato dalla
		// copertura ha agente e categoria giusti, e zero danno. Contarli sbaglierebbe in versi opposti.
		case ERTCombatOutcome::Hit:
		case ERTCombatOutcome::ShieldAbsorbed:
		case ERTCombatOutcome::Lethal:
		case ERTCombatOutcome::TerrainBonus:
			return true;
		default:
			return false;
		}

	case ERTLogCategory::Predictive:
		// Solo la previsione AZZECCATA porta danno: sul whiff la voce esiste — ed e' giusto, dice dove si e'
		// scommesso — ma `Amount` non e' un danno inflitto.
		return static_cast<ERTPredictiveOutcome>(Entry.Outcome) == ERTPredictiveOutcome::TriggerMatched;

	case ERTLogCategory::ReactionDecision:
		// L'Overwatch che SPARA. Le altre decisioni della finestra — `HOLD`, il timeout — non portano danno.
		return static_cast<ERTReactionDecisionOutcome>(Entry.Outcome) == ERTReactionDecisionOutcome::FireChosen;

	default:
		return false;
	}
}

FString URTTurnLogLibrary::DescribeActionIdentity(const FRTTurnLogEntry& Entry)
{
	// «azione base + profilo» quando la voce sa dirlo (D-033), altrimenti il solo ActionId. La forma con la
	// barretta si legge in un colpo — `Action.BasicAttack · Riktor.ImpactShot` — e non richiede di sapere a
	// memoria che ImpactShot e' un attacco base.
	//
	// Il caso `BaseActionId == ActionId` non produce «X · X»: un'azione generica usata direttamente e' il
	// profilo di se stessa, e ripeterla due volte sarebbe rumore.
	// ⚠️ Solo il profilo, senza l'azione: una traccia deserializzata puo' portarlo, e un produttore che
	// riempisse prima l'azione base pure. Senza questo ramo la riga direbbe `Action.BasicAttack · None`,
	// cioe' spaccerebbe per id d'azione il `None` di un `FName` non impostato.
	if (Entry.ActionId.IsNone())
	{
		return Entry.BaseActionId.ToString();
	}
	if (Entry.BaseActionId.IsNone() || Entry.BaseActionId == Entry.ActionId)
	{
		return Entry.ActionId.ToString();
	}
	return FString::Printf(TEXT("%s · %s"), *Entry.BaseActionId.ToString(), *Entry.ActionId.ToString());
}

namespace
{
	/**
	 * La voce dichiara un'azione?
	 *
	 * ⚠️ Si chiede a ENTRAMBI i nomi: `ActionId` da solo lascerebbe cadere una voce che porta il profilo e
	 * non l'azione, e la riga direbbe «non dichiarata» su un log che l'azione base ce l'ha scritta.
	 */
	bool HasDeclaredAction(const FRTTurnLogEntry& Entry)
	{
		return !Entry.ActionId.IsNone() || !Entry.BaseActionId.IsNone();
	}

	/**
	 * L'identita' dell'azione, o cio' che si dice quando non c'e'.
	 *
	 * In un posto solo perche' l'idioma era ricopiato in cinque rami di `DescribeEntry`, ognuno con la sua
	 * guardia — ed e' esattamente per questo che il sesto (`Fallback`) se l'era dimenticato: non c'era
	 * niente di centrale da scordarsi di chiamare (`#1412`). Una categoria nuova eredita il comportamento
	 * invece di doverlo ricordare.
	 */
	FString ActionIdentityOr(const FRTTurnLogEntry& Entry, const TCHAR* Missing)
	{
		return HasDeclaredAction(Entry)
			? URTTurnLogLibrary::DescribeActionIdentity(Entry) : FString(Missing);
	}

	/** Il suffisso ` (azione, pN)` delle categorie che lo appendono in coda, o niente se non c'e' azione. */
	FString ActionIdentitySuffix(const FRTTurnLogEntry& Entry)
	{
		if (!HasDeclaredAction(Entry))
		{
			return FString();
		}
		// `p0` non si stampa: su tracce scritte prima della v7 lo zero significa «priorita' non dichiarata».
		return Entry.Priority != 0
			? FString::Printf(TEXT(" (%s, p%d)"), *URTTurnLogLibrary::DescribeActionIdentity(Entry), Entry.Priority)
			: FString::Printf(TEXT(" (%s)"), *URTTurnLogLibrary::DescribeActionIdentity(Entry));
	}
}

TArray<FRTDescribedLine> URTTurnLogLibrary::DescribeTurnLogWithSubjects(TArray<FRTTurnLogEntry> Entries)
{
	// Per VALORE e ordinato qui dentro: la sequenza leggibile non deve dipendere dall'ordine in cui le voci
	// sono arrivate, e ordinare la copia evita di riordinare il TurnLog del chiamante come effetto collaterale.
	SortTurnLog(Entries);

	TArray<FRTDescribedLine> Lines;
	Lines.Reserve(Entries.Num());
	for (const FRTTurnLogEntry& Entry : Entries)
	{
		// ⚠️ Due sentinelle diverse per la stessa assenza, e la traduzione sta QUI. Nel TurnLog «nessuna
		// unita' dichiarata» e' `0` (D-063, e gli `StableUnitId` partono da 1); nel combat log e'
		// `INDEX_NONE`. Passare lo zero cosi' com'e' farebbe cercare l'unita' 0 nella vista di conoscenza,
		// che non esiste: ogni riga di mondo sparirebbe fail-closed.
		FRTDescribedLine& Line = Lines.AddDefaulted_GetRef();
		Line.Text = DescribeEntry(Entry);
		Line.SubjectStableUnitId = Entry.UnitId == 0 ? INDEX_NONE : Entry.UnitId;
		// 🔴 Il verdetto si TRASPORTA. Ricalcolarlo qui sarebbe calcolarlo a fine turno, cioe' sulla
		// conoscenza sbagliata: e' il difetto che [D-223] esiste per chiudere.
		Line.Verdict = Entry.Verdict;
	}
	return Lines;
}

TArray<FString> URTTurnLogLibrary::DescribeTurnLog(TArray<FRTTurnLogEntry> Entries)
{
	// Adattatore, non un secondo produttore: testo e ordine nascono in un posto solo, quindi le due forme
	// non possono divergere il giorno in cui una delle due cambia.
	TArray<FRTDescribedLine> WithSubjects = DescribeTurnLogWithSubjects(MoveTemp(Entries));

	TArray<FString> Lines;
	Lines.Reserve(WithSubjects.Num());
	for (FRTDescribedLine& Line : WithSubjects)
	{
		Lines.Add(MoveTemp(Line.Text));
	}
	return Lines;
}

FString URTTurnLogLibrary::DescribeInvalidReason(ERTActionInvalidReason Reason)
{
	switch (Reason)
	{
	case ERTActionInvalidReason::TargetGone:     return TEXT("bersaglio assente");
	case ERTActionInvalidReason::TargetDead:     return TEXT("bersaglio eliminato");
	case ERTActionInvalidReason::TargetFriendly: return TEXT("bersaglio alleato");
	case ERTActionInvalidReason::OutOfRange:     return TEXT("fuori portata");
	case ERTActionInvalidReason::NoLineOfSight:  return TEXT("nessuna linea di tiro");
	case ERTActionInvalidReason::NoMap:          return TEXT("nessuna mappa autorevole");
	case ERTActionInvalidReason::SlotOccupied:   return TEXT("lo slot e' gia' occupato");
	case ERTActionInvalidReason::OnCooldown:     return TEXT("l'abilita' e' in ricarica");
	// CP 13.2: la squadra non sa dove sia, e non ne ha un ricordo su cui ripiegare. Senza questo caso
	// cadeva nel generico «non eseguibile», che e' la forma di riga che questo ramo esiste per non produrre.
	//
	// ⚠️ Il testo dice «alla squadra» e non e' pleonastico: la conoscenza e' di SQUADRA (E13), quindi un
	// bersaglio puo' essere ignoto a chi agisce e noto a un compagno. «Bersaglio ignoto» lascerebbe
	// intendere che nessuno lo veda, che e' una frase piu' forte di quella che il dato autorizza.
	// (Entrambi i rami del merge del 2026-08-28 avevano aggiunto questo case, in punti diversi dello
	// switch: git non ha visto un conflitto e ne ha prodotti DUE. Il compilatore l'ha fermato — C2196.)
	case ERTActionInvalidReason::TargetUnknown:  return TEXT("bersaglio ignoto alla squadra");
	case ERTActionInvalidReason::Interrupted:    return TEXT("interrotta");
	case ERTActionInvalidReason::NoEffect:       return TEXT("nessun effetto da applicare");
	// ⚠️ Diverso da «interrotta»: quella e' stata CANCELLATA, questa e' avvenuta senza ottenere niente.
	case ERTActionInvalidReason::Neutralised:    return TEXT("neutralizzata da un'interruzione reciproca");
	default:                                     return TEXT("non eseguibile");
	}
}

FString URTTurnLogLibrary::DescribeEntry(const FRTTurnLogEntry& Entry)
{
	auto CellText = [](const FRTCellId& Cell)
	{
		return FString::Printf(TEXT("(q=%d,r=%d,L=%d)"), Cell.X, Cell.Y, Cell.Layer);
	};

	if (Entry.Category == ERTLogCategory::Move)
	{
		const TCHAR* Reason = TEXT("");
		switch (static_cast<ERTMoveOutcome>(Entry.Outcome))
		{
		case ERTMoveOutcome::Moved:             Reason = TEXT("si muove"); break;
		case ERTMoveOutcome::BlockedContested:  Reason = TEXT("fermo: cella contesa"); break;
		case ERTMoveOutcome::BlockedByUnit:     Reason = TEXT("fermo: cella occupata"); break;
		case ERTMoveOutcome::BlockedByPriority: Reason = TEXT("fermo: precedenza avversa"); break;
		case ERTMoveOutcome::BlockedByImpact:   Reason = TEXT("fermo: scontro frontale"); break;
		// La cella e' LIBERA: a fermarla e' stato un colpo deciso un turno prima (E18). Dirlo «occupata»
		// manderebbe il giocatore a cercare un'unita' che non c'e'.
		case ERTMoveOutcome::StoppedByPrediction: Reason = TEXT("fermo: colto da una previsione"); break;
		// Come la previsione, la cella e' libera — ma il giocatore deve poter distinguere le due cose, perche'
		// insegnano lezioni diverse: la previsione era una scommessa fatta al buio un turno prima, l'Overwatch
		// e' qualcuno che ti ha visto arrivare e ha scelto TE. «Colto da una previsione» qui manderebbe a
		// cercare un errore di lettura dove c'e' stata una lettura riuscita (CP 14.5).
		case ERTMoveOutcome::StoppedByOverwatch: Reason = TEXT("fermo: colpito in overwatch"); break;
		case ERTMoveOutcome::SupersededByDash:  Reason = TEXT("movimento non speso: lo scatto aveva gia' preso lo slot"); break;
		// Subito, non scelto (#307): «si muove» direbbe una cosa falsa di un'unita' che e' stata spinta.
		case ERTMoveOutcome::Displaced:         Reason = TEXT("spostata"); break;
		// Spinta annullata (#420). Il testo NON e' costante: il valore di questa voce sta tutto nel dire
		// QUALE dei sei modi di non muoversi si e' verificato, e «fermo: spinta annullata» ne direbbe zero.
		case ERTMoveOutcome::DisplacementResisted:
			switch (static_cast<ERTDisplacementBlockReason>(Entry.Amount))
			{
			case ERTDisplacementBlockReason::Guarded:        Reason = TEXT("spinta retta: in guardia"); break;
			case ERTDisplacementBlockReason::Braced:         Reason = TEXT("spinta retta: irrigidito"); break;
			case ERTDisplacementBlockReason::OpposingForces: Reason = TEXT("spinta annullata: forze opposte"); break;
			case ERTDisplacementBlockReason::NoDestination:  Reason = TEXT("spinta annullata: nessuna uscita"); break;
			case ERTDisplacementBlockReason::ContestedDestination:
				Reason = TEXT("spinta annullata: destinazione contesa"); break;
			// «retta» come `Guarded`/`Braced` e non «annullata»: qualcuno ha fatto qualcosa. La differenza con
			// quelle due — che l'ancora e' una REAZIONE, e si consuma — la dice gia' la voce di categoria
			// `Reaction` che il pass scrive nello stesso turno.
			case ERTDisplacementBlockReason::Anchored:
				Reason = TEXT("spinta retta: ancorato"); break;
			// Un valore aggiunto in coda e non ancora tradotto qui: si legge lo stesso, senza mentire su quale
			// sia. Il `default` di sopra direbbe «resta», che e' la parola dell'esito sbagliato.
			default:                                         Reason = TEXT("spinta annullata"); break;
			}
			break;
		// L'esito piu' FREQUENTE di tutti, e finche' e' stato un `default` era indistinguibile da un valore
		// che nessuno ha tradotto: dodici turni di autobattle producevano solo righe «resta», e non si poteva
		// sapere se fosse una scelta o un esito ignoto (#79, misurato il 2026-08-23).
		case ERTMoveOutcome::Stayed:            Reason = TEXT("resta"); break;
		// Un valore aggiunto in coda all'enum e non tradotto qui: si legge lo stesso e DICE di non essere
		// tradotto, invece di travestirsi da «resta». Chi lo incontra sa dove guardare.
		default:
			return FString::Printf(TEXT("esito di movimento non tradotto (%d) %s"),
				Entry.Outcome, *CellText(Entry.SrcCell));
		}

		// PERCHE' si e' mossa (#307): l'azione che ha causato lo spostamento, quando la voce la dichiara.
		// `Action.Move` compreso — un movimento volontario e uno scatto vanno distinti, e sono la stessa
		// categoria di voce con `ActionId` diverso.
		const FString Cause = ActionIdentitySuffix(Entry);

		// `SupersededByDash` sta QUI e non nel ramo breve: la sua ragione d'essere e' la destinazione mai
		// raggiunta, e un rendering che stampa solo `SrcCell` la nasconde. La coppia descrive la rotta
		// scartata, ed e' la stessa forma di `Moved` — cambia il motivo, non la geometria.
		if (static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::Moved
			|| static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::Displaced
			|| static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::SupersededByDash)
		{
			return FString::Printf(TEXT("%s %s -> %s (%d celle)%s"),
				Reason, *CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Cause);
		}
		return FString::Printf(TEXT("%s %s%s"), Reason, *CellText(Entry.SrcCell), *Cause);
	}

	// Fallback: cosa e' successo all'azione che non era piu' eseguibile. Il motivo per cui non lo era viaggia
	// in `Amount` (ERTActionInvalidReason): una riga che dice «annullata» senza dire da cosa non insegna nulla.
	// 🔴 **Senza questo ramo una voce `Status` cadeva nell'interpretazione di combattimento** (#1077,
	// trovato in code review): `Outcome` e' un `uint8` il cui significato lo decide la CATEGORIA, quindi
	// `AppliedWhileOnCell` (2) veniva letto come `ERTCombatOutcome::Lethal` e un'unita' che si era solo
	// bagnata compariva nel referto come *«0 danni, eliminata (Status.Wet)»*. Un rapporto di divergenza del
	// corpus golden che mente e' peggio di uno che tace.
	if (Entry.Category == ERTLogCategory::Status)
	{
		const FString Dove = CellText(Entry.SrcCell);
		const FString Quale = Entry.ActionId.IsNone() ? TEXT("(stato senza tag)") : *Entry.ActionId.ToString();
		switch (static_cast<ERTStatusOutcome>(Entry.Outcome))
		{
		case ERTStatusOutcome::AppliedByAction:
			return FString::Printf(TEXT("%s: %s per %d turno/i, da un'azione"), *Dove, *Quale, Entry.Amount);
		case ERTStatusOutcome::AppliedByTerrain:
			return FString::Printf(TEXT("%s: %s per %d turno/i, dal terreno"), *Dove, *Quale, Entry.Amount);
		case ERTStatusOutcome::AppliedWhileOnCell:
			return FString::Printf(TEXT("%s: %s finche' resta sulla cella"), *Dove, *Quale);
		case ERTStatusOutcome::Revoked:
			return FString::Printf(TEXT("%s: %s revocato, ha lasciato la cella"), *Dove, *Quale);
		case ERTStatusOutcome::Expired:
			return FString::Printf(TEXT("%s: %s scaduto"), *Dove, *Quale);

		// Le tre rimozioni causate da un altro effetto (`#1314`). Ognuna dice CHI l'ha tolto, che e' la
		// domanda a cui il replay non sapeva rispondere.
		case ERTStatusOutcome::Extinguished:
			return FString::Printf(TEXT("%s: %s spento dall'acqua"), *Dove, *Quale);
		case ERTStatusOutcome::Cleansed:
			return FString::Printf(TEXT("%s: %s purificato"), *Dove, *Quale);
		case ERTStatusOutcome::Spent:
			return FString::Printf(TEXT("%s: %s incassato"), *Dove, *Quale);
		default:
			return FString::Printf(TEXT("%s: esito di stato non tradotto (%d) su %s"),
				*Dove, Entry.Outcome, *Quale);
		}
	}

	if (Entry.Category == ERTLogCategory::Fallback)
	{
		const TCHAR* What = TEXT("");
		switch (static_cast<ERTFallbackOutcome>(Entry.Outcome))
		{
		case ERTFallbackOutcome::Stopped:      What = TEXT("fermata"); break;
		case ERTFallbackOutcome::Waited:       What = TEXT("sostituita con l'attesa"); break;
		case ERTFallbackOutcome::AttackedCell: What = TEXT("colpisce la cella pianificata"); break;
		default:                               What = TEXT("annullata"); break;
		}

		const FString Why = DescribeInvalidReason(static_cast<ERTActionInvalidReason>(Entry.Amount));

		// 🔴 **QUALE azione** (`#1412`). Era l'unico ramo con un'azione da nominare che non la nominava:
		// rendeva celle, esito e motivo — pura geometria. Due azioni annullate dalla stessa unita' nello
		// stesso turno producevano righe identiche byte a byte, e [D-063] vieta di dedurre l'unita' da
		// `SrcCell`, quindi non c'era modo di dire quale delle due fosse.
		//
		// ⚠️ Il suffisso e' CONDIZIONALE e in tondo, come `Move` e `Combat`: una riga che finisse sempre con
		// un «non dichiarata» direbbe al giocatore una lacuna interna a ogni annullamento, e nessuna delle
		// due cose gli serve. Dei tre produttori di questa categoria due l'azione la scrivono gia'
		// (`RTTurnManager_Blast.cpp:294`, `RTTurnManager.cpp:3457`) e da oggi si leggono; il terzo
		// (`FallbackEntry`, `Blast.cpp:455`) no, e riempirlo cambia l'hash delle tracce — `ActionId` entra
		// in `VisitDiscriminatingFields` — quindi e' un cambio d'identita' da dichiarare a parte.
		return FString::Printf(TEXT("%s -> %s: azione %s (%s)%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), What, *Why,
			*ActionIdentitySuffix(Entry));
	}

	// Reazione: attivata o no, e perche' — mai in silenzio (CP 5.1).
	if (Entry.Category == ERTLogCategory::Reaction)
	{
		const TCHAR* What = TEXT("");
		switch (static_cast<ERTReactionOutcome>(Entry.Outcome))
		{
		case ERTReactionOutcome::Activated:    What = TEXT("reazione attivata"); break;
		case ERTReactionOutcome::NotTriggered: What = TEXT("reazione pronta, nessun trigger"); break;
		default:                               What = TEXT("reazione non disponibile"); break;
		}
		// QUALE reazione, quando l'identita' c'e': fra `Riktor.Interposition` e `Action.Intercept` cambia
		// l'abilita' spesa e il cooldown, non solo l'esito (CP 5.5).
		if (HasDeclaredAction(Entry))
		{
			return FString::Printf(TEXT("%s: %s (%s)"),
				*CellText(Entry.SrcCell), What, *DescribeActionIdentity(Entry));
		}
		return FString::Printf(TEXT("%s: %s"), *CellText(Entry.SrcCell), What);
	}

	// Previsione: dove si e' scommesso, e se la scommessa ha pagato (E18 CP 18.1). Il whiff ha una riga
	// propria perche' e' il caso da leggere: senza, un turno in cui non succede niente sarebbe indistinguibile
	// da un turno in cui l'azione non e' mai stata dichiarata.
	if (Entry.Category == ERTLogCategory::Predictive)
	{
		const FString Who = ActionIdentityOr(Entry, TEXT("previsione"));

		if (static_cast<ERTPredictiveOutcome>(Entry.Outcome) == ERTPredictiveOutcome::TriggerMatched)
		{
			return FString::Printf(TEXT("%s -> %s: previsione azzeccata, %d danni e movimento troncato (%s)"),
				*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Who);
		}
		return FString::Printf(TEXT("%s -> %s: previsione a vuoto, nessuno e' entrato (%s)"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), *Who);
	}

	// La DECISIONE di una finestra di reazione (CP 14.5). Il testo dice sempre e per prima cosa il MOTIVO,
	// perche' e' li' che sta l'informazione: `HOLD` scelto e `HOLD` per scadenza hanno lo stesso effetto e
	// raccontano due turni diversi, e un combat log che scrivesse «tiene il colpo» per entrambi cancellerebbe
	// proprio la distinzione per cui il motivo esiste.
	if (Entry.Category == ERTLogCategory::ReactionDecision)
	{
		const FString Who = ActionIdentityOr(Entry, TEXT("overwatch"));

		// 🔴 **«la reazione» e non «overwatch», dal 2026-08-20.** Ogni arma di questo `switch` diceva
		// *overwatch* alla lettera, ed era vero finche' quello era l'unico produttore di decisioni. Con
		// [D-047] le finestre le apre anche `Action.Brace`, e il testo avrebbe raccontato un Overwatch che non
		// c'e' — «overwatch tiene il colpo» per un'unita' che si e' irrigidita. Il nome vero e' gia' in `Who`,
		// che porta l'`ActionId`: il soggetto qui torna neutro e lascia parlare quello.
		const TCHAR* const Soggetto = TEXT("la reazione");

		switch (static_cast<ERTReactionDecisionOutcome>(Entry.Outcome))
		{
		case ERTReactionDecisionOutcome::FireChosen:
			return FString::Printf(TEXT("%s -> %s: %s spara sull'unita' %d, %d danni (%s)"),
				*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Soggetto, Entry.SelectedTargetUnitId,
				Entry.Amount, *Who);
		// La risposta di un profilo (E14.7): il TOKEN e' l'informazione, e senza di lui due decisioni diverse
		// — tenere la cella o scartare — si leggerebbero identiche. E' lo stesso argomento con cui i cinque
		// `Hold` qui sotto NON condividono un testo.
		case ERTReactionDecisionOutcome::ResponseChosen:
			return FString::Printf(TEXT("%s: %s risponde «%s» (%s)"),
				*CellText(Entry.SrcCell), Soggetto,
				Entry.ReactionResponse.IsEmpty() ? TEXT("?") : *Entry.ReactionResponse, *Who);
		// I cinque `Hold` NON condividono un testo, ed e' il punto della voce: applicano tutti lo stesso
		// effetto — nessuno — e rispondono in modo diverso all'unica domanda che il giocatore pone davvero,
		// *perche' non ha sparato?*. Un «tiene il colpo» per tutti cancellerebbe proprio quella differenza.
		case ERTReactionDecisionOutcome::HoldChosen:
			return FString::Printf(TEXT("%s: %s tiene il colpo, resta armata (%s)"),
				*CellText(Entry.SrcCell), Soggetto, *Who);
		case ERTReactionDecisionOutcome::HoldTimeout:
			return FString::Printf(TEXT("%s: %s non risponde in tempo, resta armata (%s)"),
				*CellText(Entry.SrcCell), Soggetto, *Who);
		case ERTReactionDecisionOutcome::HoldNoDecider:
			return FString::Printf(TEXT("%s: %s senza decisore, resta armata (%s)"),
				*CellText(Entry.SrcCell), Soggetto, *Who);
		case ERTReactionDecisionOutcome::HoldRejected:
			return FString::Printf(TEXT("%s: risposta non ammessa, %s resta armata (%s)"),
				*CellText(Entry.SrcCell), Soggetto, *Who);
		case ERTReactionDecisionOutcome::HoldImmediate:
			return FString::Printf(TEXT("%s: nessuna scelta possibile, %s resta armata (%s)"),
				*CellText(Entry.SrcCell), Soggetto, *Who);
		// ⚠️ Distinto da `HoldImmediate` perche' dice la CAUSA e non solo la constatazione (#583, [D-109]):
		// la' non c'era scelta, qui non c'e' perche' la condizione dichiarata ha escluso i bersagli.
		// Il testo vecchio di `HoldImmediate` — «nessun bersaglio ammesso» — descriveva in realta' QUESTO caso,
		// ed e' passato a «nessuna scelta possibile», che e' cio' che quell'esito significa davvero.
		case ERTReactionDecisionOutcome::HoldCollapsedByCondition:
			return FString::Printf(TEXT("%s: nessun bersaglio soddisfa la condizione, %s resta armata (%s)"),
				*CellText(Entry.SrcCell), Soggetto, *Who);
		}
		// Un valore aggiunto in coda e non ancora tradotto: si dice cosi', invece di mentire su quale sia.
		// Stessa disciplina di `ERTDisplacementBlockReason` piu' sopra.
		return FString::Printf(TEXT("%s: %s, esito non tradotto (%s)"), *CellText(Entry.SrcCell), Soggetto, *Who);
	}

	// Orientamento: quando cambia, e chi lo ha letto (CP 16.1). Senza queste righe il combat log direbbe che
	// un colpo e' arrivato alle spalle senza mai dire quando l'unita' si e' girata.
	if (Entry.Category == ERTLogCategory::Facing)
	{
		static const TCHAR* DirectionNames[6] = { TEXT("E"), TEXT("NE"), TEXT("NW"), TEXT("W"), TEXT("SW"), TEXT("SE") };
		const int32 DirIndex = FMath::Clamp(Entry.Amount, 0, 5);
		const TCHAR* Dir = DirectionNames[DirIndex];

		switch (static_cast<ERTFacingOutcome>(Entry.Outcome))
		{
		case ERTFacingOutcome::DerivedFromMove:
			return FString::Printf(TEXT("%s: si orienta a %s (movimento)"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::DerivedFromDash:
			return FString::Printf(TEXT("%s: si orienta a %s (scatto)"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::DeclaredInPlanning:
			return FString::Printf(TEXT("%s: si gira a %s (dichiarata)"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::DeclarationRejected:
			return FString::Printf(TEXT("%s: rotazione illegale rifiutata, resta a %s"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::TargetingReoriented:
			return FString::Printf(TEXT("%s: si orienta a %s verso il bersaglio"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::TurnedToDisplacementSource:
			return FString::Printf(TEXT("%s: spinta, si gira a %s verso la sorgente"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::KeptOnEnvironmentalDisplacement:
			return FString::Printf(TEXT("%s: trascinata, resta a %s"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::UsedByBlast:
			return FString::Printf(TEXT("%s: il colpo usa l'orientamento %s"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::UsedByOverwatch:
			return FString::Printf(TEXT("%s: l'overwatch usa l'orientamento %s"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::RearHitBypassedGuard:
			return FString::Printf(TEXT("%s -> %s: colpo fuori dall'arco frontale (guardava %s), la Guard non vale"),
				*CellText(Entry.TgtCell), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::RearHitBypassedCover:
			// ⚠️ Qui `Amount` NON e' una direzione: sono i punti di riduzione scavalcati, quindi `Dir` non si
			// usa. E' la ragione per cui i due esiti sono separati (`#1430`, [D-199]).
			return FString::Printf(TEXT("%s -> %s: colpo alle spalle, la copertura non vale (%d punti scavalcati)"),
				*CellText(Entry.TgtCell), *CellText(Entry.SrcCell), Entry.Amount);
		default:
			return FString::Printf(TEXT("%s: orientamento %s"), *CellText(Entry.SrcCell), Dir);
		}
	}

	// Combat: chi colpisce chi, con quale esito e quanto danno.
	//
	// CON CHE COSA (CP 11.3, #79): il nome dell'azione si accoda fra parentesi quando la voce lo porta, con la
	// stessa forma «base · profilo» delle reazioni. Va in coda e non in testa perche' cio' che il giocatore
	// cerca per primo e' l'esito — «22 danni, eliminata» — e il nome e' la risposta alla domanda successiva.
	// Le voci senza identita' restano ESATTAMENTE la stringa di prima: le righe gia' verificate non cambiano.
	// Con che cosa, e **con quale precedenza** (CP 11.3, formato v7). La priorita' compare solo quando la
	// voce la dichiara: `p0` su ogni riga sarebbe rumore su tracce scritte prima della v7, dove lo zero
	// significa «non dichiarata» e non «priorita' zero».
	const FString Tail = ActionIdentitySuffix(Entry);

	switch (static_cast<ERTCombatOutcome>(Entry.Outcome))
	{
	case ERTCombatOutcome::NoLineOfSight:
		return FString::Printf(TEXT("%s -> %s: nessuna linea di tiro%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), *Tail);

	case ERTCombatOutcome::ShieldAbsorbed:
		return FString::Printf(TEXT("%s -> %s: %d assorbiti dallo scudo%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Tail);

	case ERTCombatOutcome::Lethal:
		return FString::Printf(TEXT("%s -> %s: %d danni, eliminata%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Tail);

	case ERTCombatOutcome::TerrainBonus:
		return FString::Printf(TEXT("%s -> %s: %d danni (bonus posizione)%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Tail);

	default:
		return FString::Printf(TEXT("%s -> %s: %d danni%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Tail);
	}
}

namespace
{
	constexpr uint32 RT_FNV_OFFSET_BASIS = 2166136261u;
	constexpr uint32 RT_FNV_PRIME        = 16777619u;

	/**
	 * Il visitor dei campi discriminanti.
	 *
	 * Un campo e' NUMERICO (`Nums`) oppure TESTUALE (`Chars`, che si mescola char per char). La distinzione
	 * non e' estetica: questo percorso e' anche quello dell'hash, e copiare una stringa in un buffer di
	 * interi solo per darla al mixer sarebbe un costo che prima non c'era.
	 *
	 * `Display` e' **pigro**: l'hash non lo chiama mai, quindi non paga nessuna formattazione. Cio' che
	 * l'hash paga comunque e' `FName::ToString()` per `ActionId`, esattamente come prima di questo elenco.
	 */
	using FDiscriminatingFieldVisitor = TFunctionRef<void(const TCHAR* Name, TArrayView<const uint32> Nums,
		const FString* Chars, TFunctionRef<FString()> Display)>;

	/**
	 * L'elenco UNICO dei campi che DISCRIMINANO una voce di TurnLog.
	 *
	 * Lo percorrono in due: `MixEntryFields`, che li mescola nell'hash, e `DescribeFirstDivergence`, che
	 * nomina quello cambiato quando due tracce divergono. Erano due elenchi, ed e' la ragione per cui la
	 * diagnosi ha taciuto il campo che diverge **tre volte** — `ActionId` (trovato in mutazione), `TgtCell`,
	 * e da ultimo `GraphRevision` (`#1423`), che produceva «atteso [X], trovato [X]» su ogni campo stampato.
	 *
	 * ⚠️ **L'ORDINE E' L'HASH.** FNV-1a e' sensibile alla sequenza: spostare una riga qui sotto cambia
	 * l'hash di ogni voce del progetto, quindi il corpus golden, gli `OrderedHashPerTurn` degli archivi di
	 * replay e il checksum di fine partita. Un campo nuovo si aggiunge **in coda**, e resta comunque un
	 * cambio dell'identita' delle tracce: si dichiara, non si scopre. A pinnarlo e' `TurnLog.HashMixesThe
	 * DeclaredFieldsInOrder`, che rifa' il conto a mano — il corpus golden NON lo pinna, perche'
	 * `CompareSerializedTraces` ricalcola l'hash su entrambi i lati e un cambio qui li muove insieme.
	 *
	 * Cosa NON entra, e perche', sta scritto voce per voce in `FRTTurnLogEntry` — che resta l'inventario
	 * autoritativo, non questo commento: `UnitId` e `TurnNumber` (D-063: rendono la traccia spiegabile, non
	 * la discriminano), `Priority`, `ReactionInstanceId` (numero d'ordine dell'armamento: due tracce che
	 * differissero solo per lui differirebbero gia' per l'`OpportunityId`), `OriginalTargetUnitId` (gia'
	 * discriminato da `SrcCell`) e `ReactionResponse` (gia' discriminata da `Outcome` e
	 * `SelectedTargetUnitId`). Un campo che non entra qui non fa divergere due tracce, quindi non ha niente
	 * da nominare in una diagnosi: sono lo stesso elenco letto da due lati.
	 *
	 * ⚠️ `BaseActionId` sta fuori per una proprieta' che puo' SMETTERE di valere: e' una FUNZIONE di
	 * `ActionId`, che qui c'e' gia', quindi due tracce non possono differire solo per quel campo e
	 * mescolarlo aggiungerebbe zero potere discriminante — stesso ragionamento di `FormatId` (CP 10.3). Se
	 * un giorno smettesse di essere derivabile da `ActionId`, questa riga diventa falsa e il campo deve
	 * entrare: e' la condizione da ricontrollare, non una proprieta' per sempre.
	 */
	void VisitDiscriminatingFields(const FRTTurnLogEntry& E, FDiscriminatingFieldVisitor Visit)
	{
		auto Number = [&Visit](const TCHAR* Name, int32 Value)
		{
			const uint32 Nums[] = { static_cast<uint32>(Value) };
			auto Display = [Value] { return FString::FromInt(Value); };
			Visit(Name, MakeArrayView(Nums, UE_ARRAY_COUNT(Nums)), nullptr, Display);
		};
		auto Named = [&Visit](const TCHAR* Name, int32 Value, const FString& Text)
		{
			const uint32 Nums[] = { static_cast<uint32>(Value) };
			auto Display = [&Text] { return Text; };
			Visit(Name, MakeArrayView(Nums, UE_ARRAY_COUNT(Nums)), nullptr, Display);
		};
		auto Cell = [&Visit](const TCHAR* Name, const FRTCellId& C)
		{
			const uint32 Nums[] = { static_cast<uint32>(C.X), static_cast<uint32>(C.Y),
				static_cast<uint32>(C.Layer) };
			// `FRTCellId::ToString()`, non un formato nuovo: la prima meta' del messaggio di divergenza
			// rende le celle con `DescribeEntry`, che usa quella. Due notazioni nella stessa riga
			// obbligherebbero chi legge a indovinare se il secondo triplo sia (q,r,L) o (x,y,z).
			auto Display = [&C] { return C.ToString(); };
			Visit(Name, MakeArrayView(Nums, UE_ARRAY_COUNT(Nums)), nullptr, Display);
		};
		auto Text = [&Visit](const TCHAR* Name, const FString& S)
		{
			// Char per char, ed e' la semantica dell'hash da CP 5.5: una stringa vuota non mescola nulla,
			// perche' il ciclo non gira.
			//
			// ⚠️ Un `FName` NON impostato NON e' una stringa vuota: `FName().ToString()` rende `None`, e
			// quei quattro caratteri li mescola. Il serializzatore lo sa e mette la guardia
			// (`E.ActionId.IsNone() ? FString() : ...`); qui la guardia NON c'e', ed e' cosi' da prima di
			// questo elenco. Toglierla o metterla cambia l'hash di ogni voce senza azione, quindi e' un
			// cambio dell'identita' delle tracce: va deciso, non corretto di passaggio.
			auto Display = [&S] { return FString::Printf(TEXT("'%s'"), *S); };
			Visit(Name, TArrayView<const uint32>(), &S, Display);
		};

		Named(TEXT("phase"), static_cast<int32>(E.Phase), RTReflection::EnumName(E.Phase));
		Named(TEXT("category"), static_cast<int32>(E.Category), RTReflection::EnumName(E.Category));
		// ⚠️ `outcome` resta un numero: il suo significato dipende dalla categoria, e a risolverlo c'e' gia'
		// `URTScenarioLoader::OutcomeEnumForCategory` — che pero' vive in `ScenarioHarness`, il quale dipende
		// da `Turn` e non viceversa. Portarla qui e' il lavoro giusto e non e' questo (`#1427`).
		// ⚠️ `outcome` e' l'unico campo il cui significato dipende dalla CATEGORIA, ed e' per questo che si
		// rende per nome (`#1427`): «atteso 2, trovato 5» mandava chi legge a cercare in `RTTurnLog.h` se
		// `2` fosse `Lethal`, `AppliedWhileOnCell` o `BlockedByUnit` — il viaggio di ritorno che questo
		// report esiste per togliere. Il valore mescolato resta il numero: cambia la resa, non l'identita'.
		Named(TEXT("outcome"), E.Outcome, URTTurnLogLibrary::DescribeOutcome(E.Category, E.Outcome));
		Cell(TEXT("src"), E.SrcCell);
		Cell(TEXT("tgt"), E.TgtCell);
		Number(TEXT("amount"), E.Amount);
		// L'identita' dell'azione entra byte per byte: due reazioni con la stessa geometria e lo stesso esito,
		// ma abilita' diverse, devono produrre hash diversi — altrimenti il replay di CP 12.6 non
		// distinguerebbe `Riktor.Interposition` da `Action.Intercept`.
		const FString ActionIdText = E.ActionId.ToString();
		Text(TEXT("actionId"), ActionIdText);
		// `GraphRevision` ENTRA (D-067): due tracce possono differire SOLO per lei — stessi eventi, ma grafo
		// modificato in un turno precedente — e sono due partite diverse. Un movimento validato su un grafo e
		// uno validato su un altro non sono lo stesso evento, anche quando le celle coincidono.
		Number(TEXT("graphRevision"), E.GraphRevision);
		Text(TEXT("opportunityId"), E.OpportunityId);
		if (!E.OpportunityId.IsEmpty())
		{
			// La DECISIONE di una finestra ENTRA (v8, CP 14.5): due partite con gli stessi movimenti in cui un
			// giocatore ha sparato e l'altro ha tenuto sono due partite diverse. Solo DENTRO il ramo: mescolare
			// `INDEX_NONE` incondizionatamente cambierebbe l'hash di **ogni** voce del progetto.
			Number(TEXT("selectedTarget"), E.SelectedTargetUnitId);
		}
	}

	/**
	 * Mescola i CAMPI di una voce in un FNV-1a a 32 bit.
	 *
	 * Estratto perche' i due hash del TurnLog — `HashTurnLog` (canonico) e `HashTurnLogOrdered` — devono
	 * mescolare **esattamente gli stessi campi**: l'unica differenza fra loro e' il sort davanti. Se i due
	 * elenchi di campi divergessero, i due hash risponderebbero a domande diverse da quelle documentate e
	 * nessun test se ne accorgerebbe.
	 *
	 * L'elenco non e' piu' qui: sta in `VisitDiscriminatingFields`, che lo condivide con la diagnosi.
	 */
	void MixEntryFields(uint32& Hash, const FRTTurnLogEntry& E)
	{
		VisitDiscriminatingFields(E,
			[&Hash](const TCHAR*, TArrayView<const uint32> Nums, const FString* Chars, TFunctionRef<FString()>)
			{
				auto Mix = [&Hash](uint32 Value)
				{
					Hash ^= Value;
					Hash *= RT_FNV_PRIME;
				};
				if (Chars)
				{
					for (const TCHAR Ch : *Chars)
					{
						Mix(static_cast<uint32>(Ch));
					}
					return;
				}
				for (const uint32 Value : Nums)
				{
					Mix(Value);
				}
			});
	}

	/** Un campo raccolto per il confronto: la SEQUENZA che l'hash mescolerebbe, piu' come si scrive. */
	struct FCollectedField
	{
		FString Name;
		TArray<uint32> Mixed;
		FString Display;
	};

	TArray<FCollectedField> CollectDiscriminatingFields(const FRTTurnLogEntry& E)
	{
		TArray<FCollectedField> Fields;
		VisitDiscriminatingFields(E,
			[&Fields](const TCHAR* Name, TArrayView<const uint32> Nums, const FString* Chars,
				TFunctionRef<FString()> Display)
			{
				FCollectedField& Field = Fields.AddDefaulted_GetRef();
				Field.Name = Name;
				Field.Display = Display();
				if (Chars)
				{
					Field.Mixed.Reserve(Chars->Len());
					for (const TCHAR Ch : *Chars)
					{
						Field.Mixed.Add(static_cast<uint32>(Ch));
					}
					return;
				}
				Field.Mixed.Append(Nums.GetData(), Nums.Num());
			});
		return Fields;
	}

	/**
	 * I campi in cui due voci differiscono, nominati coi due valori.
	 *
	 * 🔴 Il confronto e' sulla **sequenza mescolata**, non sul testo reso, ed e' la parte che conta: due
	 * valori diversi possono scriversi uguali — un `Outcome` fuori dall'enum, una categoria che questa build
	 * non conosce — e confrontare il display li dichiarerebbe identici. Cioe' esattamente il difetto che
	 * questa funzione esiste per chiudere, riaperto un livello piu' sotto. «Uguale» qui significa «uguale
	 * per l'hash», che e' la stessa regola con cui `GoldenEntriesMatch` ha dichiarato la divergenza.
	 */
	FString DescribeDivergingFields(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B)
	{
		const TArray<FCollectedField> Expected = CollectDiscriminatingFields(A);
		const TArray<FCollectedField> Found = CollectDiscriminatingFields(B);

		auto ByName = [](const TArray<FCollectedField>& Fields, const FString& Name)
		{
			return Fields.FindByPredicate([&Name](const FCollectedField& Field) { return Field.Name == Name; });
		};
		const FString Absent = TEXT("<assente>");

		TArray<FString> Diverging;
		for (const FCollectedField& Field : Expected)
		{
			// Un campo puo' esserci da una parte sola: `selectedTarget` entra solo dentro una finestra,
			// quindi due voci di cui una senza `OpportunityId` non hanno lo stesso elenco. Dirlo assente e'
			// piu' onesto che stampare un valore che quella voce non porta.
			const FCollectedField* Other = ByName(Found, Field.Name);
			if (!Other)
			{
				Diverging.Add(FString::Printf(TEXT("%s atteso %s, trovato %s"),
					*Field.Name, *Field.Display, *Absent));
			}
			else if (Other->Mixed != Field.Mixed)
			{
				Diverging.Add(FString::Printf(TEXT("%s atteso %s, trovato %s"),
					*Field.Name, *Field.Display, *Other->Display));
			}
		}
		for (const FCollectedField& Field : Found)
		{
			if (!ByName(Expected, Field.Name))
			{
				Diverging.Add(FString::Printf(TEXT("%s atteso %s, trovato %s"),
					*Field.Name, *Absent, *Field.Display));
			}
		}

		return Diverging.Num() > 0
			? FString::Printf(TEXT(" — campi: %s"), *FString::Join(Diverging, TEXT("; ")))
			: FString();
	}
}

uint32 URTTurnLogLibrary::HashTurnLog(const TArray<FRTTurnLogEntry>& Entries)
{
	// Ordina prima di mescolare: stesso insieme di voci -> stessa sequenza -> stesso hash (permutazione-invariante).
	TArray<FRTTurnLogEntry> Sorted = Entries;
	SortTurnLog(Sorted);

	uint32 Hash = RT_FNV_OFFSET_BASIS;
	for (const FRTTurnLogEntry& E : Sorted)
	{
		MixEntryFields(Hash, E);
	}
	return Hash;
}

uint32 URTTurnLogLibrary::HashTurnLogOrdered(const TArray<FRTTurnLogEntry>& Entries)
{
	// NESSUN sort: le voci si mescolano nell'ordine in cui il resolver le ha emesse. E' l'UNICA differenza
	// con `HashTurnLog` — stessi campi, stesso FNV — ed e' cio' che rende visibile un riordino delle
	// emissioni, che all'hash canonico e' invisibile per costruzione.
	uint32 Hash = RT_FNV_OFFSET_BASIS;
	for (const FRTTurnLogEntry& E : Entries)
	{
		MixEntryFields(Hash, E);
	}
	return Hash;
}

namespace
{
	// Magic 'RTTL' e helper little-endian espliciti: il formato non dipende dall'endianness della
	// piattaforma (determinismo/portabilita', invariante #4). Solo interi.
	constexpr uint32 RT_TURNLOG_MAGIC = 0x4C545452u; // byte su disco: 'R','T','T','L'

	void AppendU8(TArray<uint8>& B, uint8 V) { B.Add(V); }

	void AppendU16LE(TArray<uint8>& B, uint16 V)
	{
		B.Add(static_cast<uint8>(V & 0xFF));
		B.Add(static_cast<uint8>((V >> 8) & 0xFF));
	}

	void AppendU32LE(TArray<uint8>& B, uint32 V)
	{
		B.Add(static_cast<uint8>(V & 0xFF));
		B.Add(static_cast<uint8>((V >> 8) & 0xFF));
		B.Add(static_cast<uint8>((V >> 16) & 0xFF));
		B.Add(static_cast<uint8>((V >> 24) & 0xFF));
	}

	void AppendI32LE(TArray<uint8>& B, int32 V) { AppendU32LE(B, static_cast<uint32>(V)); }

	/**
	 * Stringa a lunghezza variabile: uint16 di lunghezza in byte + payload UTF-8. Primo campo non a
	 * dimensione fissa del formato — l'ActionId e' un nome, e troncarlo a lunghezza fissa renderebbe due
	 * azioni dal prefisso comune indistinguibili.
	 *
	 * Oltre 65535 byte la stringa viene troncata: e' il limite del campo di lunghezza. Nessun ActionId del
	 * catalogo si avvicina a quella soglia (sono nomi come `Riktor.Interposition`), quindi il caso non e'
	 * raggiungibile da dati validi — se lo diventasse, il posto dove rifiutarlo e' il validator del catalogo,
	 * non il serializzatore.
	 */
	void AppendStringUtf8(TArray<uint8>& B, const FString& S)
	{
		const FTCHARToUTF8 Utf8(*S);
		const int32 Len = FMath::Min(Utf8.Length(), static_cast<int32>(MAX_uint16));
		AppendU16LE(B, static_cast<uint16>(Len));
		B.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Len);
	}

	// Letture con bounds-check: ritornano false invece di leggere fuori dal buffer (parser sicuro).
	bool ReadU8(const TArray<uint8>& B, int32& Pos, uint8& Out)
	{
		if (Pos + 1 > B.Num()) { return false; }
		Out = B[Pos];
		Pos += 1;
		return true;
	}

	bool ReadU16LE(const TArray<uint8>& B, int32& Pos, uint16& Out)
	{
		if (Pos + 2 > B.Num()) { return false; }
		Out = static_cast<uint16>(static_cast<uint16>(B[Pos]) | (static_cast<uint16>(B[Pos + 1]) << 8));
		Pos += 2;
		return true;
	}

	bool ReadU32LE(const TArray<uint8>& B, int32& Pos, uint32& Out)
	{
		if (Pos + 4 > B.Num()) { return false; }
		Out = static_cast<uint32>(B[Pos])
			| (static_cast<uint32>(B[Pos + 1]) << 8)
			| (static_cast<uint32>(B[Pos + 2]) << 16)
			| (static_cast<uint32>(B[Pos + 3]) << 24);
		Pos += 4;
		return true;
	}

	bool ReadI32LE(const TArray<uint8>& B, int32& Pos, int32& Out)
	{
		uint32 U = 0;
		if (!ReadU32LE(B, Pos, U)) { return false; }
		Out = static_cast<int32>(U);
		return true;
	}

	bool ReadStringUtf8(const TArray<uint8>& B, int32& Pos, FString& Out)
	{
		uint16 Len = 0;
		if (!ReadU16LE(B, Pos, Len)) { return false; }
		if (Pos + Len > B.Num()) { return false; } // bounds-check come per gli interi: nessuna lettura fuori
		Out = FString(FUTF8ToTCHAR(reinterpret_cast<const ANSICHAR*>(B.GetData() + Pos), Len));
		Pos += Len;
		return true;
	}

	// Checksum FNV-1a 32-bit sui byte grezzi (stesso mescolamento di HashTurnLog, ma sul buffer):
	// rileva la corruzione del contenuto che magic/versione da soli non catturano.
	uint32 FnvBytes(const uint8* Data, int32 Len)
	{
		uint32 H = 2166136261u; // offset basis
		for (int32 i = 0; i < Len; ++i)
		{
			H ^= Data[i];
			H *= 16777619u; // prime
		}
		return H;
	}
}

TArray<uint8> URTTurnLogLibrary::SerializeTurnLog(const TArray<FRTTurnLogEntry>& Entries, ERTLogTopology Topology,
	FName FormatId)
{
	// Forma CANONICA: ordina con EntryLess prima di scrivere -> byte permutazione-invarianti (come l'hash).
	TArray<FRTTurnLogEntry> Canonical = Entries;
	SortTurnLog(Canonical);

	TArray<uint8> Out;
	// 31 byte fissi + 2+2 di lunghezza per ActionId e BaseActionId + 12 per i tre interi della v6.
	Out.Reserve(14 + Canonical.Num() * 47);

	// Header: magic + versione + flags(topologia) + identita' del formato + conteggio (little-endian).
	// Il FormatId sta DOPO i flags e prima del conteggio: le posizioni dei campi precedenti non si spostano,
	// cosi' un lettore che ispeziona magic/versione/flags continua a trovarli dove sono sempre stati.
	AppendU32LE(Out, RT_TURNLOG_MAGIC);
	AppendU16LE(Out, static_cast<uint16>(ERTTurnLogFormatVersion::WithReactionResponse));
	AppendU16LE(Out, static_cast<uint16>(Topology));
	AppendStringUtf8(Out, FormatId.IsNone() ? FString() : FormatId.ToString());
	AppendU32LE(Out, static_cast<uint32>(Canonical.Num()));

	for (const FRTTurnLogEntry& E : Canonical)
	{
		AppendU8(Out, static_cast<uint8>(E.Phase));
		AppendU8(Out, static_cast<uint8>(E.Category));
		AppendU8(Out, E.Outcome);
		AppendI32LE(Out, E.SrcCell.X);
		AppendI32LE(Out, E.SrcCell.Y);
		AppendI32LE(Out, E.SrcCell.Layer);
		AppendI32LE(Out, E.TgtCell.X);
		AppendI32LE(Out, E.TgtCell.Y);
		AppendI32LE(Out, E.TgtCell.Layer);
		AppendI32LE(Out, E.Amount);
		AppendStringUtf8(Out, E.ActionId.IsNone() ? FString() : E.ActionId.ToString());
		// Subito dopo l'ActionId, con lo stesso schema: e' il suo complemento, e tenerli adiacenti
		// significa che un lettore che sa saltare uno sa saltare anche l'altro.
		AppendStringUtf8(Out, E.BaseActionId.IsNone() ? FString() : E.BaseActionId.ToString());
		// In coda alla voce (v6): i campi precedenti non si spostano. Interi, non stringhe — l'identita' di
		// un'unita' e' un numero, e passare da `FName` costerebbe una tabella dei nomi in un formato che
		// esiste per essere confrontabile byte-per-byte.
		AppendI32LE(Out, E.UnitId);
		AppendI32LE(Out, E.TurnNumber);
		AppendI32LE(Out, E.GraphRevision);
		AppendI32LE(Out, E.Priority); // v7: in CODA, i campi precedenti non si spostano
		// v8: la decisione di una finestra di reazione. La stringa per prima, con lo schema di `ActionId`,
		// poi i due interi — stesso ordine in cui il lettore li ripesca.
		AppendStringUtf8(Out, E.OpportunityId);
		AppendI32LE(Out, E.ReactionInstanceId);
		AppendI32LE(Out, E.SelectedTargetUnitId);
		// v9: chi era il bersaglio PRIMA di un redirect (#1060). In coda, i campi precedenti non si spostano.
		AppendI32LE(Out, E.OriginalTargetUnitId);
		// v10: il token della risposta, quando non e' derivabile dall'esito (E14.7, [D-047]). Stringa con lo
		// schema di `ActionId`, in coda: i campi precedenti non si spostano. **Vuota** per ogni finestra
		// dell'Overwatch — la sua risposta si deduce, e una traccia scritta prima della v10 significa la
		// stessa cosa di una scritta dopo.
		AppendStringUtf8(Out, E.ReactionResponse);
	}

	// Checksum FNV di tutto cio' che precede (header + voci), in coda: rileva la corruzione del contenuto.
	const uint32 Checksum = FnvBytes(Out.GetData(), Out.Num());
	AppendU32LE(Out, Checksum);
	return Out;
}

bool URTTurnLogLibrary::DeserializeTurnLog(const TArray<uint8>& Bytes, TArray<FRTTurnLogEntry>& OutEntries,
	ERTLogTopology* OutTopology, FName* OutFormatId)
{
	OutEntries.Reset();
	if (OutFormatId)
	{
		*OutFormatId = NAME_None;
	}

	int32 Pos = 0;
	uint32 Magic = 0;
	if (!ReadU32LE(Bytes, Pos, Magic) || Magic != RT_TURNLOG_MAGIC) { return false; }

	// Versioni LEGGIBILI: la corrente e le quattro precedenti. La 2 non porta l'ActionId, la 3 non porta il
	// FormatId, la 4 non porta il BaseActionId, la 5 non porta UnitId/TurnNumber, e in ogni caso il posto
	// resta vuoto — leggerle e' onesto (quei byte non contenevano quell'informazione), inventarla no. Ogni
	// altro valore e' rifiutato: interpretare byte di un formato ignoto produce un replay sbagliato in silenzio.
	uint16 Version = 0;
	if (!ReadU16LE(Bytes, Pos, Version)) { return false; }
	// v10 (E14.7, [D-047]): il token della risposta. Come per ogni estensione precedente, la versione nuova
	// implica tutte quelle sotto — le versioni sono cumulative, non alternative.
	const bool bHasReactionResponse =
		(Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithReactionResponse));
	const bool bHasRedirectOrigin = bHasReactionResponse
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithRedirectOrigin));
	const bool bHasReactionDecision = bHasRedirectOrigin
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithReactionDecision));
	const bool bHasPriority = bHasReactionDecision
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithPriority));
	const bool bHasUnitId = bHasPriority
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithUnitId));
	const bool bHasBaseActionId = bHasUnitId
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithBaseActionId));
	const bool bHasFormatId = bHasBaseActionId
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithFormatId));
	const bool bHasActionId = bHasFormatId
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithActionId));
	if (!bHasActionId && Version != static_cast<uint16>(ERTTurnLogFormatVersion::WithChecksum)) { return false; }

	// Flags = topologia delle celle. Fail-closed sui valori sconosciuti (come per la versione): interpretare
	// coordinate di una topologia ignota produrrebbe un replay sbagliato in silenzio.
	uint16 Flags = 0;
	if (!ReadU16LE(Bytes, Pos, Flags)) { return false; }
	if (Flags != static_cast<uint16>(ERTLogTopology::Square) && Flags != static_cast<uint16>(ERTLogTopology::Hex))
	{
		return false;
	}
	if (OutTopology)
	{
		*OutTopology = static_cast<ERTLogTopology>(Flags);
	}

	if (bHasFormatId)
	{
		FString FormatId;
		if (!ReadStringUtf8(Bytes, Pos, FormatId)) { return false; }
		if (OutFormatId)
		{
			*OutFormatId = FormatId.IsEmpty() ? NAME_None : FName(*FormatId);
		}
	}

	uint32 Count = 0;
	if (!ReadU32LE(Bytes, Pos, Count)) { return false; }

	// Fail-closed sul CONTEGGIO, prima di riservare memoria. Un file corrotto puo' dichiarare miliardi di
	// voci, e `Reserve` su quel numero termina il processo (`OnInvalidArrayNum`) prima ancora che il checksum
	// in coda possa smentirlo: il parser deve rifiutare, non morire. Il limite superiore vero e' il buffer
	// che resta — ogni voce occupa almeno i suoi campi a dimensione fissa.
	constexpr int32 FixedEntryBytes = 31;         // 3 uint8 + 7 int32
	// + 2 byte di lunghezza per ogni stringa presente nel formato: ActionId da v3, BaseActionId da v5.
	// + 12 byte fissi per UnitId, TurnNumber e GraphRevision da v6, + 4 per Priority da v7.
	// + 2 di lunghezza per `OpportunityId` e 8 per i due interi della decisione, da v8.
	// + 4 per `OriginalTargetUnitId` da v9, + 2 di lunghezza per `ReactionResponse` da v10. ⚠️ Va aggiornato
	// a OGNI versione che allunga la voce, o il guard sottostima e lascia passare un `Count` piu' grande di
	// quanto il buffer regga — che e' esattamente cio' che questo calcolo esiste per impedire. La prima
	// stesura della v9 l'aveva dimenticato: 61 byte dichiarati contro 65 reali, il 6% di margine in meno su un
	// controllo fail-closed. Trovato da una code review, ed e' il motivo per cui la v10 lo aggiorna **nello
	// stesso commit** che allunga la voce, invece di lasciarlo a un giro successivo.
	const int32 MinEntryBytes = FixedEntryBytes + (bHasActionId ? 2 : 0) + (bHasBaseActionId ? 2 : 0)
		+ (bHasUnitId ? 12 : 0) + (bHasPriority ? 4 : 0) + (bHasReactionDecision ? 10 : 0)
		+ (bHasRedirectOrigin ? 4 : 0) + (bHasReactionResponse ? 2 : 0);
	const int32 Remaining = Bytes.Num() - Pos;
	if (Remaining < 0 || Count > static_cast<uint32>(Remaining / MinEntryBytes))
	{
		return false;
	}

	OutEntries.Reserve(static_cast<int32>(Count));
	for (uint32 i = 0; i < Count; ++i)
	{
		FRTTurnLogEntry E;
		uint8 Phase = 0;
		uint8 Category = 0;
		uint8 Outcome = 0;
		if (!ReadU8(Bytes, Pos, Phase) || !ReadU8(Bytes, Pos, Category) || !ReadU8(Bytes, Pos, Outcome))
		{
			OutEntries.Reset();
			return false;
		}
		E.Phase = static_cast<ERTMatchPhase>(Phase);
		E.Category = static_cast<ERTLogCategory>(Category);
		E.Outcome = Outcome;

		if (!ReadI32LE(Bytes, Pos, E.SrcCell.X) || !ReadI32LE(Bytes, Pos, E.SrcCell.Y) || !ReadI32LE(Bytes, Pos, E.SrcCell.Layer)
			|| !ReadI32LE(Bytes, Pos, E.TgtCell.X) || !ReadI32LE(Bytes, Pos, E.TgtCell.Y) || !ReadI32LE(Bytes, Pos, E.TgtCell.Layer)
			|| !ReadI32LE(Bytes, Pos, E.Amount))
		{
			OutEntries.Reset();
			return false;
		}

		if (bHasActionId)
		{
			FString ActionId;
			if (!ReadStringUtf8(Bytes, Pos, ActionId))
			{
				OutEntries.Reset();
				return false;
			}
			E.ActionId = ActionId.IsEmpty() ? NAME_None : FName(*ActionId);
		}
		if (bHasBaseActionId)
		{
			FString BaseActionId;
			if (!ReadStringUtf8(Bytes, Pos, BaseActionId))
			{
				OutEntries.Reset();
				return false;
			}
			E.BaseActionId = BaseActionId.IsEmpty() ? NAME_None : FName(*BaseActionId);
		}
		if (bHasUnitId)
		{
			if (!ReadI32LE(Bytes, Pos, E.UnitId) || !ReadI32LE(Bytes, Pos, E.TurnNumber)
				|| !ReadI32LE(Bytes, Pos, E.GraphRevision))
			{
				OutEntries.Reset();
				return false;
			}
		}
		// Sotto la v6 i tre campi restano a 0: nessuna unita' dedotta dalla cella, nessun turno inventato,
		// nessuna revisione di grafo attribuita a una traccia che non la dichiarava.
		if (bHasPriority)
		{
			if (!ReadI32LE(Bytes, Pos, E.Priority))
			{
				OutEntries.Reset();
				return false;
			}
		}
		// Sotto la v7 `Priority` resta 0, e NON si deduce dall'`ActionId` consultando il catalogo — che pure
		// sarebbe possibile. Sarebbe la stessa inferenza che D-063 ha dichiarato non valida per l'unita': il
		// catalogo di oggi puo' non essere quello con cui la traccia fu scritta, e una priorita' inventata
		// racconterebbe un ordine di risoluzione che quel turno non ha avuto.
		if (bHasReactionDecision)
		{
			if (!ReadStringUtf8(Bytes, Pos, E.OpportunityId)
				|| !ReadI32LE(Bytes, Pos, E.ReactionInstanceId)
				|| !ReadI32LE(Bytes, Pos, E.SelectedTargetUnitId))
			{
				OutEntries.Reset();
				return false;
			}
		}
		// Sotto la v8 i tre campi restano ai loro default — id vuoto, `INDEX_NONE` sui due interi — e qui non
		// c'e' nemmeno la tentazione di dedurli: in quelle versioni nessuna finestra si apriva in partita, e
		// una traccia senza decisioni e' completa cosi' com'e', non monca.
		if (bHasRedirectOrigin)
		{
			if (!ReadI32LE(Bytes, Pos, E.OriginalTargetUnitId))
			{
				OutEntries.Reset();
				return false;
			}
		}
		// Sotto la v9 resta `INDEX_NONE`, e **non** si deduce dalla `SrcCell` risolvendo l'occupante — che pure
		// sarebbe possibile. E' la stessa inferenza che D-063 vieta, e su una traccia storica sarebbe peggio:
		// la cella dice dove il protetto stava al Blast, l'occupante di fine turno puo' essere un altro.
		if (bHasReactionResponse)
		{
			if (!ReadStringUtf8(Bytes, Pos, E.ReactionResponse))
			{
				OutEntries.Reset();
				return false;
			}
		}
		// Sotto la v10 resta **vuoto**, ed e' la lettura giusta e non una perdita: in quelle versioni l'unico
		// produttore di finestre era l'Overwatch, la cui risposta si **deduce** dall'esito. Un campo vuoto
		// dice a `ArmRecordedReactionDecisions` «deducila come sempre», che e' esattamente cio' che quei byte
		// significavano. ⚠️ Inventare qui un token — ricostruendolo dall'`Outcome` — sarebbe peggio che
		// lasciarlo vuoto: farebbe sembrare esplicita una deduzione, e il lettore perderebbe la sola
		// informazione che distingue le due epoche del formato.
		OutEntries.Add(E);
	}

	// Verifica il checksum in coda: ricalcola FNV su header+voci e confronta (rileva corruzione del contenuto).
	const int32 PayloadEnd = Pos;
	uint32 StoredChecksum = 0;
	if (!ReadU32LE(Bytes, Pos, StoredChecksum))
	{
		OutEntries.Reset();
		return false;
	}
	if (FnvBytes(Bytes.GetData(), PayloadEnd) != StoredChecksum)
	{
		OutEntries.Reset();
		return false;
	}
	return true;
}

bool URTTurnLogLibrary::SaveTurnLogToFile(const FString& Path, const TArray<FRTTurnLogEntry>& Entries,
	ERTLogTopology Topology, FName FormatId)
{
	const TArray<uint8> Bytes = SerializeTurnLog(Entries, Topology, FormatId);
	return FFileHelper::SaveArrayToFile(Bytes, *Path);
}

bool URTTurnLogLibrary::LoadTurnLogFromFile(const FString& Path, TArray<FRTTurnLogEntry>& OutEntries,
	ERTLogTopology* OutTopology, FName* OutFormatId)
{
	OutEntries.Reset();
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path))
	{
		return false; // file mancante o illeggibile
	}
	return DeserializeTurnLog(Bytes, OutEntries, OutTopology, OutFormatId);
}

ERTTraceComparison URTTurnLogLibrary::CompareSerializedTraces(const TArray<uint8>& A, const TArray<uint8>& B)
{
	TArray<FRTTurnLogEntry> EntriesA;
	TArray<FRTTurnLogEntry> EntriesB;
	ERTLogTopology TopologyA = ERTLogTopology::Square;
	ERTLogTopology TopologyB = ERTLogTopology::Square;
	FName FormatA = NAME_None;
	FName FormatB = NAME_None;

	if (!DeserializeTurnLog(A, EntriesA, &TopologyA, &FormatA)
		|| !DeserializeTurnLog(B, EntriesB, &TopologyB, &FormatB))
	{
		return ERTTraceComparison::Unreadable;
	}

	// Il CONTESTO prima del contenuto: due tracce prodotte con formati (o topologie) diversi non sono
	// confrontabili, e dire "divergenza" manderebbe a cercare un difetto nel codice dove c'e' una
	// configurazione diversa. E' la stessa ragione per cui l'header porta questi due campi.
	if (FormatA != FormatB)
	{
		return ERTTraceComparison::FormatMismatch;
	}
	if (TopologyA != TopologyB)
	{
		return ERTTraceComparison::TopologyMismatch;
	}

	return HashTurnLog(EntriesA) == HashTurnLog(EntriesB)
		? ERTTraceComparison::Identical
		: ERTTraceComparison::Divergence;
}

/**
 * Uguaglianza secondo i campi che entrano nell'HASH, non campo per campo a mano.
 *
 * Cosi' la diagnosi considera divergenza esattamente cio' che `HashTurnLog` considera, che e' l'unica
 * regola con cui ha senso confrontarsi: `DescribeFirstDivergence` viene chiamata *dopo* che l'hash ha
 * dichiarato una divergenza, e deve indicare **dove** quell'hash e' cambiato.
 *
 * ⚠️ Confrontava con `EntryLess`, ed era corretto finche' i campi dell'ordinamento coincidevano con
 * quelli dell'hash. Non e' piu' vero: `UnitId` e `TurnNumber` sono entrati in `EntryLess` per chiudere
 * la forma canonica della serializzazione (D-067) e restano fuori dall'hash (D-063). Con `EntryLess`
 * questa funzione discriminerebbe **piu'** dell'hash e si fermerebbe su una voce identica per l'hash —
 * per giunta mostrando due descrizioni uguali, perche' quei campi nessuno li stampa.
 *
 * Passa dalla `HashTurnLogOrdered` di una voce sola invece di elencare i campi a mano: cosi' l'elenco
 * resta uno solo (`MixEntryFields`) e non puo' divergere in silenzio da quello vero.
 */
const UEnum* URTTurnLogLibrary::OutcomeEnumForCategory(ERTLogCategory Category)
{
	switch (Category)
	{
	case ERTLogCategory::Move:             return StaticEnum<ERTMoveOutcome>();
	case ERTLogCategory::Combat:           return StaticEnum<ERTCombatOutcome>();
	case ERTLogCategory::Fallback:         return StaticEnum<ERTFallbackOutcome>();
	case ERTLogCategory::Reaction:         return StaticEnum<ERTReactionOutcome>();
	case ERTLogCategory::Environment:      return StaticEnum<ERTEnvironmentOutcome>();
	case ERTLogCategory::Facing:           return StaticEnum<ERTFacingOutcome>();
	case ERTLogCategory::Status:           return StaticEnum<ERTStatusOutcome>();
	case ERTLogCategory::Predictive:       return StaticEnum<ERTPredictiveOutcome>();
	case ERTLogCategory::ReactionDecision: return StaticEnum<ERTReactionDecisionOutcome>();
	case ERTLogCategory::ReactionClash:    return StaticEnum<ERTClashLogEvent>();
	default:                               return nullptr;
	}
}

FString URTTurnLogLibrary::DescribeOutcome(ERTLogCategory Category, uint8 Outcome)
{
	const UEnum* Enum = OutcomeEnumForCategory(Category);
	const FString Nome = Enum ? Enum->GetNameStringByValue(static_cast<int64>(Outcome)) : FString();
	// Un esito fuori dall'enum non e' impossibile — il campo e' un `uint8` e una traccia vecchia puo'
	// portarne uno che questa build non conosce piu': mostrarlo GREZZO e' l'unica risposta onesta.
	return Nome.IsEmpty() ? FString::FromInt(static_cast<int32>(Outcome)) : Nome;
}

bool URTTurnLogLibrary::IsSubjectTheSufferer(const FRTTurnLogEntry& Entry)
{
	if (Entry.UnitId == 0)
	{
		return false; // nessuna unita' dichiarata: non c'e' nessun soggetto di cui dire il ruolo
	}
	// Il danno ambientale: la domanda ce l'ha gia' una funzione sua, e passarci evita di duplicarne il
	// riconoscimento della causa — che ha una rete di sicurezza e un motivo per averla.
	if (IsEnvironmentalDamage(Entry))
	{
		return true;
	}
	// La guardia (o la copertura) scavalcata da un colpo alle spalle: la voce descrive l'orientamento del
	// DIFENSORE, quindi il soggetto e' chi ha subito il colpo. L'attaccante e' in `SrcCell` (`#1418`).
	if (Entry.Category == ERTLogCategory::Facing
		&& (Entry.Outcome == static_cast<uint8>(ERTFacingOutcome::RearHitBypassedCover)
			|| Entry.Outcome == static_cast<uint8>(ERTFacingOutcome::RearHitBypassedGuard)))
	{
		return true;
	}
	// L'azione cancellata da un `Action.Interrupt`: `UnitId` porta chi l'ha SUBITA, non chi ha interrotto
	// — chi ha interrotto non e' nella voce affatto (`#1437`, [D-196]). Senza questo caso, chi conta le
	// azioni compiute da un'unita' le accrediterebbe una cancellazione che ha solo subito.
	return Entry.Category == ERTLogCategory::Fallback
		&& Entry.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted);
}

bool URTTurnLogLibrary::GoldenEntriesMatch(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B)
{
	return URTTurnLogLibrary::HashTurnLogOrdered({ A }) == URTTurnLogLibrary::HashTurnLogOrdered({ B });
}

FString URTTurnLogLibrary::DescribeFirstDivergence(int32 TurnNumber, const TArray<FRTTurnLogEntry>& Golden,
	const TArray<FRTTurnLogEntry>& Actual)
{
	const int32 Common = FMath::Min(Golden.Num(), Actual.Num());
	for (int32 i = 0; i < Common; ++i)
	{
		if (GoldenEntriesMatch(Golden[i], Actual[i]))
		{
			continue;
		}

		// Turno, fase e ActionId sono cio' che il DoD di CP 12.6 chiede per nome; la descrizione delle due
		// voci evita il viaggio di ritorno al codice per capire cosa sia cambiato.
		//
		// L'ActionId si nomina DA ENTRAMBE le parti quando differisce, e non e' un dettaglio estetico:
		// `DescribeEntry` non lo stampa per le voci `Move`, quindi una regressione che cambia SOLO l'azione
		// produceva «atteso [X], trovato [X]» — due stringhe identiche accanto alla parola «diverge». Trovato
		// con la verifica di mutazione, che e' esattamente il caso per cui serve.
		// L'azione si nomina come il DoD di CP 12.6 chiede per nome. Forma BREVE anche quando diverge: se e'
		// lei a cambiare lo dice l'elenco dei campi qui sotto, che la tratta come ogni altro campo — dirlo
		// due volte nella stessa riga e' la riga piu' letta del report che ripete se stessa.
		const FString ActionText = FString::Printf(TEXT("azione '%s'"), *Golden[i].ActionId.ToString());

		const FString GoldenText = DescribeEntry(Golden[i]);
		const FString ActualText = DescribeEntry(Actual[i]);

		// **Quale campo** e' cambiato, coi due valori. La prosa non basta e non puo' bastare: `DescribeEntry`
		// rende cio' che serve a un umano che rilegge una partita, non l'insieme di cio' che distingue due
		// tracce — e i due insiemi non coincidono per nessuna categoria.
		//
		// L'elenco da cui questo nasce e' lo STESSO che alimenta l'hash (`VisitDiscriminatingFields`), e non
		// e' un dettaglio di implementazione: `GoldenEntriesMatch` confronta gli hash, quindi l'insieme dei
		// campi che possono far divergere due voci **e' per costruzione** quello. Finche' erano due elenchi
		// separati la diagnosi ha taciuto tre volte il campo che divergeva — `ActionId`, `TgtCell`,
		// `GraphRevision` — e ogni volta si e' chiuso il caso singolo. Qui si chiude la classe.
		//
		// Si nominano SOLO i campi diversi: elencarli tutti rimetterebbe chi legge a cercare.
		const FString RawDetail = DescribeDivergingFields(Golden[i], Actual[i]);

		return FString::Printf(
			TEXT("turno %d, voce %d: fase %s, %s — atteso [%s], trovato [%s]%s"),
			TurnNumber, i, *RTReflection::EnumName(Golden[i].Phase), *ActionText,
			*GoldenText, *ActualText, *RawDetail);
	}

	// Stesse voci fin dove entrambe arrivano, ma una delle due finisce prima: e' una divergenza, e va detta
	// invece di leggere fuori dall'array. La prima voce in piu' (o in meno) e' la piu' informativa.
	if (Golden.Num() != Actual.Num())
	{
		const bool bMissing = Actual.Num() < Golden.Num();
		const FRTTurnLogEntry& Odd = bMissing ? Golden[Common] : Actual[Common];
		return FString::Printf(
			TEXT("turno %d: %d voci attese, %d trovate — la prima %s e' in fase %s, azione '%s' [%s]"),
			TurnNumber, Golden.Num(), Actual.Num(),
			bMissing ? TEXT("MANCANTE") : TEXT("IN PIU'"),
			*RTReflection::EnumName(Odd.Phase), *Odd.ActionId.ToString(), *DescribeEntry(Odd));
	}

	return FString();
}
