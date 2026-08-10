#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo del fallback, leggibile nel log
#include "Turn/RTReactionLibrary.h" // ERTReactionOutcome: l'esito di una reazione, leggibile nel log
#include "Misc/FileHelper.h"

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
	// Ultimo campo, ultimo tie-break. Confronto LESSICOGRAFICO (`FName::Compare`), mai `FastLess`: quello
	// ordina per indice nella name table, che dipende dall'ordine in cui i nomi sono stati creati nel processo
	// — due esecuzioni della stessa partita darebbero due ordini diversi, cioe' due hash diversi (#4).
	return A.ActionId.Compare(B.ActionId) < 0;
}

void URTTurnLogLibrary::SortTurnLog(TArray<FRTTurnLogEntry>& Entries)
{
	Entries.Sort([](const FRTTurnLogEntry& A, const FRTTurnLogEntry& B) { return EntryLess(A, B); });
}

FString URTTurnLogLibrary::DescribeActionIdentity(const FRTTurnLogEntry& Entry)
{
	// «azione base + profilo» quando la voce sa dirlo (D-033), altrimenti il solo ActionId. La forma con la
	// barretta si legge in un colpo — `Action.BasicAttack · Bastion.ImpactShot` — e non richiede di sapere a
	// memoria che ImpactShot e' un attacco base.
	//
	// Il caso `BaseActionId == ActionId` non produce «X · X»: un'azione generica usata direttamente e' il
	// profilo di se stessa, e ripeterla due volte sarebbe rumore.
	if (Entry.BaseActionId.IsNone() || Entry.BaseActionId == Entry.ActionId)
	{
		return Entry.ActionId.ToString();
	}
	return FString::Printf(TEXT("%s · %s"), *Entry.BaseActionId.ToString(), *Entry.ActionId.ToString());
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
		default:                                Reason = TEXT("resta"); break;
		}

		if (static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::Moved)
		{
			return FString::Printf(TEXT("%s %s -> %s (%d celle)"),
				Reason, *CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount);
		}
		return FString::Printf(TEXT("%s %s"), Reason, *CellText(Entry.SrcCell));
	}

	// Fallback: cosa e' successo all'azione che non era piu' eseguibile. Il motivo per cui non lo era viaggia
	// in `Amount` (ERTActionInvalidReason): una riga che dice «annullata» senza dire da cosa non insegna nulla.
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

		const TCHAR* Why = TEXT("");
		switch (static_cast<ERTActionInvalidReason>(Entry.Amount))
		{
		case ERTActionInvalidReason::TargetGone:     Why = TEXT("bersaglio assente"); break;
		case ERTActionInvalidReason::TargetDead:     Why = TEXT("bersaglio eliminato"); break;
		case ERTActionInvalidReason::TargetFriendly: Why = TEXT("bersaglio alleato"); break;
		case ERTActionInvalidReason::OutOfRange:     Why = TEXT("fuori portata"); break;
		case ERTActionInvalidReason::NoLineOfSight:  Why = TEXT("nessuna linea di tiro"); break;
		case ERTActionInvalidReason::NoMap:          Why = TEXT("nessuna mappa autorevole"); break;
		default:                                     Why = TEXT("non eseguibile"); break;
		}

		return FString::Printf(TEXT("%s -> %s: azione %s (%s)"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), What, Why);
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
		// QUALE reazione, quando l'identita' c'e': fra `Bastion.Interposition` e `Action.Intercept` cambia
		// l'abilita' spesa e il cooldown, non solo l'esito (CP 5.5).
		if (!Entry.ActionId.IsNone())
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
		const FString Who = Entry.ActionId.IsNone()
			? FString(TEXT("previsione")) : DescribeActionIdentity(Entry);

		if (static_cast<ERTPredictiveOutcome>(Entry.Outcome) == ERTPredictiveOutcome::TriggerMatched)
		{
			return FString::Printf(TEXT("%s -> %s: previsione azzeccata, %d danni e movimento troncato (%s)"),
				*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Who);
		}
		return FString::Printf(TEXT("%s -> %s: previsione a vuoto, nessuno e' entrato (%s)"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), *Who);
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
		case ERTFacingOutcome::RearHitBypassedCover:
			return FString::Printf(TEXT("%s -> %s: colpo fuori dall'arco frontale (guardava %s), la protezione non vale"),
				*CellText(Entry.TgtCell), *CellText(Entry.SrcCell), Dir);
		default:
			return FString::Printf(TEXT("%s: orientamento %s"), *CellText(Entry.SrcCell), Dir);
		}
	}

	// Combat: chi colpisce chi, con quale esito e quanto danno.
	switch (static_cast<ERTCombatOutcome>(Entry.Outcome))
	{
	case ERTCombatOutcome::NoLineOfSight:
		return FString::Printf(TEXT("%s -> %s: nessuna linea di tiro"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell));

	case ERTCombatOutcome::ShieldAbsorbed:
		return FString::Printf(TEXT("%s -> %s: %d assorbiti dallo scudo"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount);

	case ERTCombatOutcome::Lethal:
		return FString::Printf(TEXT("%s -> %s: %d danni, eliminata"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount);

	case ERTCombatOutcome::TerrainBonus:
		return FString::Printf(TEXT("%s -> %s: %d danni (bonus posizione)"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount);

	default:
		return FString::Printf(TEXT("%s -> %s: %d danni"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount);
	}
}

namespace
{
	constexpr uint32 RT_FNV_OFFSET_BASIS = 2166136261u;
	constexpr uint32 RT_FNV_PRIME        = 16777619u;

	/**
	 * Mescola i CAMPI di una voce in un FNV-1a a 32 bit.
	 *
	 * Estratto perche' i due hash del TurnLog — `HashTurnLog` (canonico) e `HashTurnLogOrdered` — devono
	 * mescolare **esattamente gli stessi campi**: l'unica differenza fra loro e' il sort davanti. Se i due
	 * elenchi di campi divergessero, i due hash risponderebbero a domande diverse da quelle documentate e
	 * nessun test se ne accorgerebbe.
	 */
	void MixEntryFields(uint32& Hash, const FRTTurnLogEntry& E)
	{
		auto Mix = [&Hash](uint32 V)
		{
			Hash ^= V;
			Hash *= RT_FNV_PRIME;
		};
		Mix(static_cast<uint32>(E.Phase));
		Mix(static_cast<uint32>(E.Category));
		Mix(static_cast<uint32>(E.Outcome));
		Mix(static_cast<uint32>(E.SrcCell.X));
		Mix(static_cast<uint32>(E.SrcCell.Y));
		Mix(static_cast<uint32>(E.SrcCell.Layer));
		Mix(static_cast<uint32>(E.TgtCell.X));
		Mix(static_cast<uint32>(E.TgtCell.Y));
		Mix(static_cast<uint32>(E.TgtCell.Layer));
		Mix(static_cast<uint32>(E.Amount));
		// L'identita' dell'azione entra nell'hash byte per byte: due reazioni con la stessa geometria e lo
		// stesso esito, ma abilita' diverse, devono produrre hash diversi — altrimenti il replay di CP 12.6
		// non distinguerebbe `Bastion.Interposition` da `Action.Intercept`. Un nome vuoto non mescola nulla,
		// quindi le tracce senza ActionId hanno lo stesso hash di prima di CP 5.5.
		for (const TCHAR Ch : E.ActionId.ToString())
		{
			Mix(static_cast<uint32>(Ch));
		}
		// `BaseActionId` NON entra, ed e' deliberato: e' una FUNZIONE di `ActionId`, che qui c'e' gia'.
		// Due tracce non possono differire solo per quel campo, quindi mescolarlo aggiungerebbe zero potere
		// discriminante — e invaliderebbe in blocco ogni hash golden. Stesso ragionamento di `FormatId`
		// (CP 10.3). Se un giorno `BaseActionId` smettesse di essere derivabile da `ActionId`, questa riga
		// di commento diventa falsa e il campo deve entrare: e' la condizione da ricontrollare, non una
		// proprieta' per sempre.
		//
		// `UnitId` e `TurnNumber` NON entrano, per lo stesso criterio (D-063): servono a rendere la traccia
		// spiegabile — chi ha agito, in quale turno — non a discriminarla. Includerli invaliderebbe in blocco
		// ogni hash golden senza aggiungere potere discriminante.
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
	 * catalogo si avvicina a quella soglia (sono nomi come `Bastion.Interposition`), quindi il caso non e'
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
	// 31 byte fissi + 2+2 di lunghezza per ActionId e BaseActionId + 8 per UnitId/TurnNumber (v6).
	Out.Reserve(14 + Canonical.Num() * 43);

	// Header: magic + versione + flags(topologia) + identita' del formato + conteggio (little-endian).
	// Il FormatId sta DOPO i flags e prima del conteggio: le posizioni dei campi precedenti non si spostano,
	// cosi' un lettore che ispeziona magic/versione/flags continua a trovarli dove sono sempre stati.
	AppendU32LE(Out, RT_TURNLOG_MAGIC);
	AppendU16LE(Out, static_cast<uint16>(ERTTurnLogFormatVersion::WithUnitId));
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
	const bool bHasUnitId = (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithUnitId));
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
	// + 8 byte fissi per UnitId e TurnNumber da v6.
	const int32 MinEntryBytes = FixedEntryBytes + (bHasActionId ? 2 : 0) + (bHasBaseActionId ? 2 : 0)
		+ (bHasUnitId ? 8 : 0);
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
			if (!ReadI32LE(Bytes, Pos, E.UnitId) || !ReadI32LE(Bytes, Pos, E.TurnNumber))
			{
				OutEntries.Reset();
				return false;
			}
		}
		// Sotto la v6 i due campi restano a 0: nessuna unita' dedotta dalla cella, nessun turno inventato.
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
