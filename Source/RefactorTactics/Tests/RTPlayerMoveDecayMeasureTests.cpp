#include "Misc/AutomationTest.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Radice del corpus golden. Stessa di `RTGoldenCorpusTests`: sta nel SORGENTE, accanto ai test. */
	FString DecayGoldenTurnPath(const FString& ScenarioId, int32 TurnNumber)
	{
		return FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/RefactorTactics/Tests/Golden"),
			ScenarioId, FString::Printf(TEXT("turn-%02d.rttl"), TurnNumber));
	}
}

/**
 * **Quante volte, sullo showcase, un Move pianificato puo' decadere per uno spostamento che lo precede.**
 *
 * 🔑 **Il numero che [D-045] chiedeva e che nessuno aveva preso da questa parte del campo** (`#2747`). Quella
 * decisione e' una baseline dichiaratamente rivedibile, con un criterio di uscita quantificato —
 *
 * > *«se in playtest un Move viene annullato **piu' di una volta ogni due round**, si prova `C`»*
 *
 * — e quel criterio **e' scattato**, ma sul **bot**: sull'arena generata la sequenza ferma piu' lunga passa da
 * `4` a `11` con `Model A` (`#2556`). Il bot pero' pianifica DESTINAZIONI e sta in mischia quasi ogni turno
 * (`RTTurnManager.cpp:837-841`, che azzera `PlannedPath` e `PlannedWaypoints`): il suo numero non e'
 * automaticamente quello di chi gioca.
 *
 * 🔑 **Il dato era gia' su disco, e non serve strumentare niente.** `AppendDisplacementEntry` marca lo
 * spostamento forzato con `Phase = Blast` e `Category = Move`, e dichiara il perche' accanto:
 * *«e' quello che rende leggibile "sono stato spostato PRIMA di potermi muovere"»* (`RTTurnManager.cpp:2517`).
 * La fase `Move` viene dopo il `Blast`, quindi ogni voce cosi' marcata e' un'occasione in cui un piano di
 * movimento, se c'era, e' decaduto.
 *
 * ⚠️ **E' un LIMITE SUPERIORE, non il numero esatto, ed e' scritto anche nel referto**: un'unita' spostata che
 * non aveva pianificato nulla non perde niente. Contare gli spostamenti pre-Move sovrastima i Move annullati,
 * mai il contrario — quindi se il limite superiore sta sotto la soglia, il numero vero ci sta a maggior
 * ragione. E' la direzione utile per la domanda che `D-045` pone.
 *
 * 🔑 **Il campione e' lo showcase, ed e' una scelta**: e' l'unico corpus in cui i piani sono scritti da un
 * umano turno per turno — tutti e quattro, non solo quelli di una squadra. L'autobattle e' escluso di
 * proposito: li' i piani li scrive il bot, ed e' la meta' gia' misurata.
 *
 * ⛔ **Questo test misura, non giudica**, come `RTStallDefinitionMeasureTests` per lo stallo. Il suo valore sta
 * nel REFERTO, non nell'esito: asserisce solo che il corpus sia leggibile, perche' un test che fallisse su un
 * numero di bilanciamento introdurrebbe una soglia che nessuna decisione ha posto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerMoveDecayMeasureTest,
	"RefactorTactics.Meta.PlayerMoveDecayOnTheShowcase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerMoveDecayMeasureTest::RunTest(const FString&)
{
	const FString ScenarioId = TEXT("RT_Showcase_Relay_v01");
	const int32 TurniAttesi = 8;

	int32 TurniLetti = 0;
	int32 SpostamentiPreMove = 0;      // `Move` in fase `Blast`: il limite superiore dei Move annullati
	int32 MovimentiVolontari = 0;      // `Move` in fase `Move` con esito `Moved`: chi si e' mosso davvero
	// 🔴 **La guardia contro il campione DEGENERE**: se nello showcase non spinge mai nessuno, uno zero
	// non direbbe *«il Move sopravvive»* ma *«la domanda non e' stata posta»*. Si contano quindi anche gli
	// spostamenti forzati in QUALUNQUE fase — `Push`, `Pull`, la caduta, lo slide — perche' il referto
	// possa distinguere i due casi invece di lasciarli uguali.
	int32 SpostamentiForzatiTotali = 0;
	TSet<int32> UnitaOsservate;
	TArray<int32> PerTurno;
	PerTurno.Init(0, TurniAttesi + 1);

	for (int32 Turno = 1; Turno <= TurniAttesi; ++Turno)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *DecayGoldenTurnPath(ScenarioId, Turno)))
		{
			AddError(FString::Printf(TEXT("golden mancante: turno %d di %s"), Turno, *ScenarioId));
			continue;
		}
		TArray<FRTTurnLogEntry> Voci;
		if (!URTTurnLogLibrary::DeserializeTurnLog(Bytes, Voci))
		{
			AddError(FString::Printf(TEXT("golden illeggibile: turno %d"), Turno));
			continue;
		}
		++TurniLetti;

		for (const FRTTurnLogEntry& E : Voci)
		{
			if (E.Category != ERTLogCategory::Move) { continue; }
			UnitaOsservate.Add(E.UnitId);

			// ⚠️ **`Blast` e non `Move`**: uno spostamento forzato risolve dove risolve il colpo che lo
			// produce, e la fase `Move` viene dopo. E' esattamente la condizione di [D-045] — *«l'origine
			// effettiva e' diversa da quella su cui il percorso era stato pianificato»*.
			// ⚠️ `DisplacementResisted` **non** conta: chi resiste non si sposta, quindi la sua origine non
			// cambia e nessun Move decade. Contano gli spostamenti AVVENUTI, in qualunque forma li produca
			// il gioco — spinta, scivolata sul ghiaccio, caduta.
			const ERTMoveOutcome Esito = static_cast<ERTMoveOutcome>(E.Outcome);
			const bool bForzato = Esito == ERTMoveOutcome::Displaced
				|| Esito == ERTMoveOutcome::Slid
				|| Esito == ERTMoveOutcome::Fell
				|| Esito == ERTMoveOutcome::FellToAlternative
				|| Esito == ERTMoveOutcome::FellToLastStable
				|| Esito == ERTMoveOutcome::FellWithoutLanding;
			if (bForzato) { ++SpostamentiForzatiTotali; }

			if (E.Phase == ERTMatchPhase::Blast)
			{
				++SpostamentiPreMove;
				++PerTurno[Turno];
			}
			else if (E.Phase == ERTMatchPhase::Move
				&& static_cast<ERTMoveOutcome>(E.Outcome) == ERTMoveOutcome::Moved)
			{
				++MovimentiVolontari;
			}
		}
	}

	if (!TestEqual(TEXT("premessa: il corpus golden dello showcase e' leggibile per intero"),
			TurniLetti, TurniAttesi))
	{
		return false;
	}

	// --- REFERTO ---------------------------------------------------------------------------------------
	AddInfo(FString::Printf(
		TEXT("campione: %s — %d turni, %d unita' osservate, TUTTI i piani scritti a mano"),
		*ScenarioId, TurniLetti, UnitaOsservate.Num()));
	AddInfo(FString::Printf(
		TEXT("spostamenti forzati PRIMA della fase Move: %d in %d round"), SpostamentiPreMove, TurniLetti));
	AddInfo(FString::Printf(
		TEXT("movimenti volontari riusciti (fase Move, esito Moved): %d"), MovimentiVolontari));

	FString Ripartizione;
	for (int32 Turno = 1; Turno <= TurniAttesi; ++Turno)
	{
		Ripartizione += FString::Printf(TEXT("T%d=%d "), Turno, PerTurno[Turno]);
	}
	AddInfo(FString::Printf(TEXT("per turno: %s"), *Ripartizione));

	// Il criterio di [D-045] in numeri: «piu' di una volta ogni due round» = piu' di `TurniLetti / 2`.
	const int32 SogliaD045 = TurniLetti / 2;
	AddInfo(FString::Printf(
		TEXT("criterio D-045 su questo campione: la soglia e' %d annullamenti in %d round; ")
		TEXT("il LIMITE SUPERIORE misurato e' %d — %s"),
		SogliaD045, TurniLetti, SpostamentiPreMove,
		SpostamentiPreMove > SogliaD045 ? TEXT("SOPRA la soglia") : TEXT("sotto la soglia")));

	// ⚠️ I due limiti, scritti qui e non lasciati a chi legge.
	// 🔴 **La riga che impedisce di leggere uno zero per quello che non e'.**
	if (SpostamentiPreMove == 0 && SpostamentiForzatiTotali == 0)
	{
		AddInfo(TEXT("⚠ CAMPIONE DEGENERE per questa domanda: nel campione non c'e' NESSUNO spostamento ")
			TEXT("forzato, in nessuna fase. Lo zero qui sopra dice che la condizione di D-045 non si e' mai ")
			TEXT("presentata, NON che il Move vi sopravviva. Serve un campione in cui qualcuno venga spinto."));
	}
	else
	{
		AddInfo(FString::Printf(
			TEXT("il campione NON e' degenere: %d spostamenti non volontari (Displaced/Slid/Fell) sono avvenuti, ")
			TEXT("quindi lo zero pre-Move e' un esito e non un'assenza di occasioni"), SpostamentiForzatiTotali));
	}

	// 🔑 **La variabile che spiega la differenza col bot, e vale piu' dello zero.**
	AddInfo(FString::Printf(
		TEXT("occasioni: %d spostamenti non volontari in %d round — e' la FREQUENZA dell'occasione, non la ")
		TEXT("sopravvivenza del Move, cio' che separa questo campione dal bot. Il bot viene spostato in ")
		TEXT("mischia quasi ogni turno; qui la condizione di D-045 si presenta di rado."),
		SpostamentiForzatiTotali, TurniLetti));
	if (SpostamentiForzatiTotali > 0 && SpostamentiForzatiTotali <= 2)
	{
		AddInfo(TEXT("⚠ con COSI' POCHE occasioni lo zero dice che la condizione e' RARA su questo ")
			TEXT("campione, NON che il Move vi sopravviva: per quest'ultima serve un campione che spinga di piu'"));
	}

	AddInfo(TEXT("limite 1: e' un limite SUPERIORE — un'unita' spostata senza un piano di movimento non ")
		TEXT("perde nulla, quindi il numero vero e' <= a questo"));
	AddInfo(TEXT("limite 2: otto turni non sono un playtest. Questo numero descrive lo showcase, e non si ")
		TEXT("estrapola a una partita giocata"));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
