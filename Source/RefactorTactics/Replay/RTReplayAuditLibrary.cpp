#include "Replay/RTReplayAuditLibrary.h"

#include "Dom/JsonObject.h"
#include "Perception/RTKnowledgeView.h" // FRTKnowledgeSubject: solo per ricostruire l'ingresso di FreezeVerdict
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	// ⚠️ **Nomi distinti per file, e non e' pedanteria: e' un rosso gia' preso.** Con la unity build questo
	// `.cpp` finisce nello stesso blob di `RTReplayRecorderLibrary.cpp`, che ha le proprie `K_VERSION` e
	// `K_MATCH_ID` in un namespace anonimo: due nomi generici uguali sono una **ridefinizione**, e il build
	// cade per chi arriva dopo. Il difetto non si vede finche' la build adattiva tiene i due file separati —
	// qui e' comparso solo dopo un merge, quando il working set e' cambiato.
	// Le chiavi in un posto solo, come `RTReplayRecorderLibrary`: una stringa ripetuta a mano fra scrittura
	// e lettura e' il modo piu' silenzioso di rompere un round-trip.
	const TCHAR* KAudit_Version   = TEXT("Version");
	const TCHAR* KAudit_MatchId  = TEXT("MatchId");
	const TCHAR* KAudit_Turn      = TEXT("TurnNumber");
	const TCHAR* KAudit_Hash      = TEXT("OrderedHash");
	const TCHAR* KAudit_Planning  = TEXT("PlanningKnowledge");
	const TCHAR* KAudit_Blast     = TEXT("BlastKnowledge");
	const TCHAR* KAudit_Verdicts  = TEXT("Verdicts");
	const TCHAR* KAudit_Decisions = TEXT("BotDecisions");
	const TCHAR* KAudit_Target    = TEXT("TargetUnitId");
	const TCHAR* KAudit_TargetTeam = TEXT("TargetTeamId");
	const TCHAR* KAudit_TargetCell = TEXT("TargetCell");

	const TCHAR* KAudit_Team      = TEXT("TeamId");
	const TCHAR* KAudit_Visible   = TEXT("VisibleCells");
	const TCHAR* KAudit_Contacts  = TEXT("Contacts");
	const TCHAR* KAudit_Unit      = TEXT("StableUnitId");
	const TCHAR* KAudit_Cell      = TEXT("Cell");
	const TCHAR* KAudit_Mask      = TEXT("Mask");
	const TCHAR* KAudit_Phase     = TEXT("Phase");

	/** Una cella come tre numeri: `[q, r, layer]`. Compatta, e si legge a occhio. */
	TSharedPtr<FJsonValue> AuditCellToJson(const FRTCellId& Cell)
	{
		TArray<TSharedPtr<FJsonValue>> Triple;
		Triple.Add(MakeShared<FJsonValueNumber>(Cell.X));
		Triple.Add(MakeShared<FJsonValueNumber>(Cell.Y));
		Triple.Add(MakeShared<FJsonValueNumber>(Cell.Layer));
		return MakeShared<FJsonValueArray>(Triple);
	}

	bool AuditCellFromJson(const TSharedPtr<FJsonValue>& Value, FRTCellId& OutCell)
	{
		const TArray<TSharedPtr<FJsonValue>>* Triple = nullptr;
		if (!Value.IsValid() || !Value->TryGetArray(Triple) || Triple->Num() != 3)
		{
			return false;
		}
		OutCell.X = static_cast<int32>((*Triple)[0]->AsNumber());
		OutCell.Y = static_cast<int32>((*Triple)[1]->AsNumber());
		OutCell.Layer = static_cast<int32>((*Triple)[2]->AsNumber());
		return true;
	}

	TSharedPtr<FJsonValue> AuditCellsToJson(const TArray<FRTCellId>& Cells)
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		Out.Reserve(Cells.Num());
		for (const FRTCellId& C : Cells) { Out.Add(AuditCellToJson(C)); }
		return MakeShared<FJsonValueArray>(Out);
	}

	bool AuditCellsFromJson(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Key, TArray<FRTCellId>& OutCells)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Obj->TryGetArrayField(Key, Values))
		{
			return true; // campo assente: lista vuota, non un errore — un turno puo' non vedere niente
		}
		for (const TSharedPtr<FJsonValue>& V : *Values)
		{
			FRTCellId Cell;
			if (!AuditCellFromJson(V, Cell)) { return false; }
			OutCells.Add(Cell);
		}
		return true;
	}

	TSharedPtr<FJsonValue> AuditKnowledgeToJson(const FRTTeamKnowledge& K)
	{
		const TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(KAudit_Version, K.Version);
		Obj->SetNumberField(KAudit_Team, K.TeamId);
		Obj->SetNumberField(KAudit_Turn, K.TurnNumber);
		Obj->SetField(KAudit_Visible, AuditCellsToJson(K.VisibleCells));

		TArray<TSharedPtr<FJsonValue>> Contacts;
		Contacts.Reserve(K.Contacts.Num());
		for (const FRTLastKnownContact& C : K.Contacts)
		{
			const TSharedRef<FJsonObject> CO = MakeShared<FJsonObject>();
			CO->SetNumberField(KAudit_Unit, C.StableUnitId);
			CO->SetField(KAudit_Cell, AuditCellToJson(C.Cell));
			CO->SetNumberField(KAudit_Turn, C.TurnNumber);
			Contacts.Add(MakeShared<FJsonValueObject>(CO));
		}
		Obj->SetArrayField(KAudit_Contacts, Contacts);
		return MakeShared<FJsonValueObject>(Obj);
	}

	bool AuditKnowledgeFromJson(const TSharedPtr<FJsonValue>& Value, FRTTeamKnowledge& OutK)
	{
		const TSharedPtr<FJsonObject>* Obj = nullptr;
		if (!Value.IsValid() || !Value->TryGetObject(Obj)) { return false; }

		// 🔴 **La versione della CONOSCENZA si RIFIUTA, non si riproduce.** `AwarenessOfUnit` e `FindContact`
		// sono fail-closed su una versione diversa da `CurrentVersion`: riprodurre quella del file farebbe
		// collassare a `Rejected` ogni ricalcolo il giorno in cui `FRTTeamKnowledge::CurrentVersion` sale, e
		// un artefatto vecchio produrrebbe un muro di finte divergenze invece di un rifiuto pulito.
		double Version = 0.0;
		if (!(*Obj)->TryGetNumberField(KAudit_Version, Version)
			|| static_cast<int32>(Version) != FRTTeamKnowledge::CurrentVersion)
		{
			return false;
		}
		OutK.Version = static_cast<int32>(Version);

		double Team = 0.0, Turn = 0.0;
		(*Obj)->TryGetNumberField(KAudit_Team, Team);
		(*Obj)->TryGetNumberField(KAudit_Turn, Turn);
		OutK.TeamId = static_cast<int32>(Team);
		OutK.TurnNumber = static_cast<int32>(Turn);

		if (!AuditCellsFromJson(*Obj, KAudit_Visible, OutK.VisibleCells)) { return false; }

		const TArray<TSharedPtr<FJsonValue>>* Contacts = nullptr;
		if ((*Obj)->TryGetArrayField(KAudit_Contacts, Contacts))
		{
			for (const TSharedPtr<FJsonValue>& CV : *Contacts)
			{
				const TSharedPtr<FJsonObject>* CO = nullptr;
				if (!CV.IsValid() || !CV->TryGetObject(CO)) { return false; }

				FRTLastKnownContact C;
				double Unit = 0.0, CTurn = 0.0;
				(*CO)->TryGetNumberField(KAudit_Unit, Unit);
				(*CO)->TryGetNumberField(KAudit_Turn, CTurn);
				C.StableUnitId = static_cast<int32>(Unit);
				C.TurnNumber = static_cast<int32>(CTurn);
				if (!AuditCellFromJson((*CO)->TryGetField(KAudit_Cell), C.Cell)) { return false; }
				OutK.Contacts.Add(C);
			}
		}
		return true;
	}
}

FString URTReplayAuditLibrary::TurnAuditFileName(int32 TurnNumber)
{
	// Stesso zero-padding di `TurnFileName`: i due file di un turno si leggono uno accanto all'altro in un
	// elenco di cartella, che e' spesso il primo strumento di diagnosi.
	return FString::Printf(TEXT("turn-%03d.rtaudit"), TurnNumber);
}

FString URTReplayAuditLibrary::AuditToJson(const FRTTurnAudit& Audit)
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

	// La versione per PRIMA, come nel manifest: e' l'unico campo che un lettore deve saper trovare sempre,
	// perche' e' cio' che gli permette di rifiutare il resto senza interpretarlo.
	Root->SetNumberField(KAudit_Version, static_cast<double>(ERTAuditFormatVersion::Current));
	Root->SetStringField(KAudit_MatchId, Audit.MatchId.ToString(EGuidFormats::Digits));
	Root->SetNumberField(KAudit_Turn, Audit.TurnNumber);

	// L'hash come STRINGA e non come numero: e' a 64 bit, e oltre 2^53 un double perderebbe gli ultimi bit
	// in silenzio. Il manifest se lo puo' permettere perche' i suoi restano piccoli; un'ancora che mentisse
	// sugli ultimi bit smetterebbe di essere un'ancora.
	Root->SetStringField(KAudit_Hash, LexToString(Audit.OrderedHash));

	TArray<TSharedPtr<FJsonValue>> Planning;
	for (const FRTTeamKnowledge& K : Audit.PlanningKnowledge) { Planning.Add(AuditKnowledgeToJson(K)); }
	Root->SetArrayField(KAudit_Planning, Planning);

	TArray<TSharedPtr<FJsonValue>> Blast;
	for (const FRTTeamKnowledge& K : Audit.BlastKnowledge) { Blast.Add(AuditKnowledgeToJson(K)); }
	Root->SetArrayField(KAudit_Blast, Blast);

	TArray<TSharedPtr<FJsonValue>> Verdicts;
	for (const FRTAuditVerdictRecord& R : Audit.Verdicts)
	{
		const TSharedRef<FJsonObject> VO = MakeShared<FJsonObject>();
		VO->SetNumberField(KAudit_Phase, static_cast<double>(R.Phase));
		VO->SetNumberField(KAudit_Unit, R.SubjectUnitId);
		VO->SetNumberField(KAudit_Team, R.SubjectTeamId);
		VO->SetField(KAudit_Cell, AuditCellToJson(R.SubjectCell));
		VO->SetNumberField(KAudit_Mask, static_cast<double>(R.Verdict.Mask));
		Verdicts.Add(MakeShared<FJsonValueObject>(VO));
	}
	Root->SetArrayField(KAudit_Verdicts, Verdicts);

	TArray<TSharedPtr<FJsonValue>> Decisions;
	for (const FRTAuditBotDecision& D : Audit.BotDecisions)
	{
		const TSharedRef<FJsonObject> DO = MakeShared<FJsonObject>();
		DO->SetNumberField(KAudit_Unit, D.UnitId);
		DO->SetNumberField(KAudit_Team, D.TeamId);
		DO->SetNumberField(KAudit_Target, D.TargetUnitId);
		DO->SetNumberField(KAudit_TargetTeam, D.TargetTeamId);
		DO->SetField(KAudit_TargetCell, AuditCellToJson(D.TargetCell));
		Decisions.Add(MakeShared<FJsonValueObject>(DO));
	}
	Root->SetArrayField(KAudit_Decisions, Decisions);

	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

bool URTReplayAuditLibrary::AuditFromJson(const FString& Json, FRTTurnAudit& OutAudit)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	// 🔴 Fail-closed, e prima di ogni altra lettura: una versione che non si conosce si RIFIUTA invece di
	// interpretarne i campi. E' la convenzione che `DeserializeTurnLog` e `ManifestFromJson` applicano gia'.
	// ⚠️ Si confronta il DOUBLE, senza restringere prima: `static_cast<uint16>(65537)` vale `1`, quindi una
	// versione futura sarebbe passata per quella corrente — e un valore negativo renderebbe la conversione
	// indefinita. Il narrowing prima del confronto e' un fail-OPEN travestito da fail-closed.
	double Version = 0.0;
	if (!Root->TryGetNumberField(KAudit_Version, Version)
		|| Version != static_cast<double>(ERTAuditFormatVersion::Current))
	{
		return false;
	}

	FRTTurnAudit Read;

	FString IdText;
	if (!Root->TryGetStringField(KAudit_MatchId, IdText) || !FGuid::Parse(IdText, Read.MatchId))
	{
		return false;
	}

	double Turn = 0.0;
	if (!Root->TryGetNumberField(KAudit_Turn, Turn)) { return false; }
	Read.TurnNumber = static_cast<int32>(Turn);

	FString HashText;
	if (!Root->TryGetStringField(KAudit_Hash, HashText) || !HashText.IsNumeric()) { return false; }
	LexFromString(Read.OrderedHash, *HashText);

	const TArray<TSharedPtr<FJsonValue>>* Planning = nullptr;
	if (Root->TryGetArrayField(KAudit_Planning, Planning))
	{
		for (const TSharedPtr<FJsonValue>& V : *Planning)
		{
			FRTTeamKnowledge K;
			if (!AuditKnowledgeFromJson(V, K)) { return false; }
			Read.PlanningKnowledge.Add(K);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Blast = nullptr;
	if (Root->TryGetArrayField(KAudit_Blast, Blast))
	{
		for (const TSharedPtr<FJsonValue>& V : *Blast)
		{
			FRTTeamKnowledge K;
			if (!AuditKnowledgeFromJson(V, K)) { return false; }
			Read.BlastKnowledge.Add(K);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Verdicts = nullptr;
	if (Root->TryGetArrayField(KAudit_Verdicts, Verdicts))
	{
		for (const TSharedPtr<FJsonValue>& V : *Verdicts)
		{
			const TSharedPtr<FJsonObject>* VO = nullptr;
			if (!V.IsValid() || !V->TryGetObject(VO)) { return false; }

			FRTAuditVerdictRecord R;
			double Unit = 0.0, Team = 0.0, Mask = 0.0, Phase = 0.0;
			(*VO)->TryGetNumberField(KAudit_Unit, Unit);
			(*VO)->TryGetNumberField(KAudit_Team, Team);
			(*VO)->TryGetNumberField(KAudit_Mask, Mask);
			(*VO)->TryGetNumberField(KAudit_Phase, Phase);
			R.Phase = static_cast<ERTMatchPhase>(static_cast<uint8>(Phase));
			R.SubjectUnitId = static_cast<int32>(Unit);
			R.SubjectTeamId = static_cast<int32>(Team);
			R.Verdict.Mask = static_cast<uint32>(Mask);
			if (!AuditCellFromJson((*VO)->TryGetField(KAudit_Cell), R.SubjectCell)) { return false; }
			Read.Verdicts.Add(R);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Decisions = nullptr;
	if (Root->TryGetArrayField(KAudit_Decisions, Decisions))
	{
		for (const TSharedPtr<FJsonValue>& V : *Decisions)
		{
			const TSharedPtr<FJsonObject>* DO = nullptr;
			if (!V.IsValid() || !V->TryGetObject(DO)) { return false; }

			// 🔴 **Ogni campo si PRETENDE, non si assume.** Con una lettura non verificata un
			// `TargetUnitId` mancante restava a zero — e zero non e' «nessun bersaglio», e' un'unita' che
			// non esiste: `EnsureMatchRoster` distribuisce identita' da 1 in su ([D-063]). Il controllo
			// avrebbe chiesto a `ClassifyTarget` conto di un'unita' inventata, non l'avrebbe trovata in
			// nessuna conoscenza, e avrebbe riportato un archivio troncato come un bot che bara.
			//
			// E' la stessa disciplina che questo file applica gia' alla versione e alle celle: un record
			// incompleto si RIFIUTA, perche' interpretarlo produce una risposta plausibile e falsa.
			FRTAuditBotDecision D;
			double Unit = 0.0, Team = 0.0, Target = 0.0, TargetTeam = 0.0;
			if (!(*DO)->TryGetNumberField(KAudit_Unit, Unit)) { return false; }
			if (!(*DO)->TryGetNumberField(KAudit_Team, Team)) { return false; }
			if (!(*DO)->TryGetNumberField(KAudit_Target, Target)) { return false; }
			if (!(*DO)->TryGetNumberField(KAudit_TargetTeam, TargetTeam)) { return false; }
			D.UnitId = static_cast<int32>(Unit);
			D.TeamId = static_cast<int32>(Team);
			D.TargetUnitId = static_cast<int32>(Target);
			D.TargetTeamId = static_cast<int32>(TargetTeam);
			if (!AuditCellFromJson((*DO)->TryGetField(KAudit_TargetCell), D.TargetCell)) { return false; }
			Read.BotDecisions.Add(D);
		}
	}

	// Si adotta solo a lettura completa: `OutAudit` non deve mai restare mezzo riempito.
	OutAudit = MoveTemp(Read);
	return true;
}

bool URTReplayAuditLibrary::RecordTurnAudit(const FString& ReplaysRoot, const FRTTurnAudit& Audit)
{
	if (!Audit.MatchId.IsValid())
	{
		return false;
	}

	const FString Path = FPaths::Combine(ReplaysRoot, Audit.MatchId.ToString(EGuidFormats::Digits),
		TurnAuditFileName(Audit.TurnNumber));

	// 🔴 **Atomica come il manifest**: si scrive un temporaneo e lo si sposta sopra. Un componente che esiste
	// per sopravvivere a un crash non puo' lasciare mezzo file: un `.rtaudit` troncato accanto a una traccia
	// valida sarebbe un'evidenza che sembra esserci.
	const FString Temp = Path + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(AuditToJson(Audit), *Temp))
	{
		return false;
	}

	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.DeleteFile(*Path); // `MoveFile` non sovrascrive
	if (!PF.MoveFile(*Path, *Temp))
	{
		PF.DeleteFile(*Temp);
		return false;
	}
	return true;
}

bool URTReplayAuditLibrary::LoadTurnAudit(const FString& ReplaysRoot, const FGuid& MatchId, int32 TurnNumber,
	FRTTurnAudit& OutAudit, int64 ExpectedOrderedHash)
{
	const FString Path = FPaths::Combine(ReplaysRoot, MatchId.ToString(EGuidFormats::Digits),
		TurnAuditFileName(TurnNumber));

	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Path))
	{
		return false;
	}

	FRTTurnAudit Read;
	if (!AuditFromJson(Json, Read))
	{
		return false;
	}

	// 🔴 **L'aggancio: il NOME del file non e' una prova.** Un artefatto copiato o rinominato dichiara
	// ancora la propria partita e il proprio turno, e sono quelli a decidere. Un'evidenza attribuita alla
	// partita sbagliata e' peggio di un'evidenza mancante, perche' sembra una risposta.
	if (Read.MatchId != MatchId || Read.TurnNumber != TurnNumber)
	{
		return false;
	}

	// E l'ANCORA, che senza questo confronto sarebbe un campo decorativo: `ExpectedOrderedHash` a zero
	// significa «non lo so», e allora non si giudica. Con un valore, una traccia rigenerata smette di
	// sembrare la coppia di questo artefatto.
	if (ExpectedOrderedHash != 0 && Read.OrderedHash != ExpectedOrderedHash)
	{
		return false;
	}

	OutAudit = MoveTemp(Read);
	return true;
}

FRTKnowledgeVerdict URTReplayAuditLibrary::RecomputeVerdict(const FRTTurnAudit& Audit,
	const FRTAuditVerdictRecord& Record)
{
	FRTKnowledgeSubject Subject;
	Subject.StableUnitId = Record.SubjectUnitId;
	Subject.TeamId = Record.SubjectTeamId;
	Subject.Cell = Record.SubjectCell;

	// 🔴 **L'istantanea giusta dipende dalla FASE**, e questa riga e' nata da un rosso su una partita vera:
	// `TeamKnowledgeState` ha due assegnazioni per turno, quindi una voce del Dash porta il verdetto della
	// conoscenza di Planning. Ricalcolarla contro quella del Blast produceva una divergenza che accusava il
	// gioco di un difetto che non aveva.
	const TArray<FRTTeamKnowledge>& Reference = (Record.Phase < ERTMatchPhase::Blast)
		? Audit.PlanningKnowledge
		: Audit.BlastKnowledge;

	// Il predicato di PRODUZIONE, non una copia: confrontare una copia con l'originale non direbbe niente
	// sul dato registrato, e le due derive si coprirebbero a vicenda.
	return URTTeamKnowledgeLibrary::FreezeVerdict(Reference, Subject);
}

TArray<FString> URTReplayAuditLibrary::FindVerdictMismatches(const FRTTurnAudit& Audit)
{
	TArray<FString> Mismatches;
	for (int32 i = 0; i < Audit.Verdicts.Num(); ++i)
	{
		const FRTAuditVerdictRecord& Record = Audit.Verdicts[i];

		// 🔴 **Un fatto di MONDO non ha un verdetto da ricalcolare: ne ha uno per REGOLA.** Una superficie che
		// scade, un ponte che crolla, una casella obiettivo: `AppendLogEntry` scrive `Everyone()` senza
		// consultare nessuna conoscenza, e senza soggetto. Ricalcolarlo darebbe al piu' la maschera delle
		// squadre registrate — mai `~0u` — e produrrebbe una divergenza a **ogni turno di ogni partita** su
		// qualunque mappa con un obiettivo. Trovato in code review; il test verde lo era perche' l'arena piatta
		// dell'harness un obiettivo non ce l'ha.
		//
		// ✅ E la regola si verifica invece di saltarla: se non c'e' soggetto, il verdetto DEVE essere
		// `Everyone()`. Il controllo diventa piu' forte, non piu' debole.
		if (Record.SubjectUnitId == INDEX_NONE)
		{
			if (!(Record.Verdict == FRTKnowledgeVerdict::Everyone()))
			{
				Mismatches.Add(FString::Printf(
					TEXT("voce %d: fatto di mondo senza soggetto, ma il verdetto registrato e' 0x%08X invece di Everyone"),
					i, Record.Verdict.Mask));
			}
			continue;
		}

		const FRTKnowledgeVerdict Recomputed = RecomputeVerdict(Audit, Record);
		if (!(Recomputed == Record.Verdict))
		{
			Mismatches.Add(FString::Printf(
				TEXT("voce %d: verdetto registrato 0x%08X, ricalcolato 0x%08X (unita' %d, squadra %d, cella %d,%d,%d)"),
				i, Record.Verdict.Mask, Recomputed.Mask, Record.SubjectUnitId, Record.SubjectTeamId,
				Record.SubjectCell.X, Record.SubjectCell.Y, Record.SubjectCell.Layer));
		}
	}
	return Mismatches;
}

TArray<FString> URTReplayAuditLibrary::FindUnauthorizedTargets(const FRTTurnAudit& Audit)
{
	TArray<FString> Violations;

	for (const FRTAuditBotDecision& Decision : Audit.BotDecisions)
	{
		if (Decision.TargetUnitId == INDEX_NONE)
		{
			continue; // nessun attacco pianificato: non c'e' una scelta da giudicare
		}

		const FRTTeamKnowledge* Known = Audit.PlanningKnowledge.FindByPredicate(
			[&Decision](const FRTTeamKnowledge& K) { return K.TeamId == Decision.TeamId; });
		if (Known == nullptr)
		{
			// Non e' «lecito», e' **non verificabile**, ed e' un difetto dell'archivio. Si dichiara invece di
			// tacere: un controllo che salta cio' che non sa leggere e' un controllo che assolve.
			Violations.Add(FString::Printf(
				TEXT("unita' %d: ha scelto un bersaglio, e per la squadra %d non c'e' conoscenza registrata"),
				Decision.UnitId, Decision.TeamId));
			continue;
		}

		// La STESSA domanda che il cancello di produzione ha posto in partita, sullo stesso predicato.
		const ERTTargetKnowledge Verdict = URTTeamKnowledgeLibrary::ClassifyTarget(
			*Known, Decision.TargetUnitId, Decision.TargetTeamId, Decision.TargetCell);
		if (Verdict == ERTTargetKnowledge::Rejected)
		{
			Violations.Add(FString::Printf(
				TEXT("unita' %d (squadra %d) ha scelto l'unita' %d in (%d,%d,%d), che la sua squadra non conosceva"),
				Decision.UnitId, Decision.TeamId, Decision.TargetUnitId,
				Decision.TargetCell.X, Decision.TargetCell.Y, Decision.TargetCell.Layer));
		}
	}

	return Violations;
}
