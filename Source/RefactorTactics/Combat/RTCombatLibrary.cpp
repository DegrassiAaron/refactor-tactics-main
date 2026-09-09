#include "Combat/RTCombatLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

// Le due provenienze dei pool d'assorbimento (`#2213`). La ragione per cui la seconda non nomina un
// `ActionId` sta sulla dichiarazione, in `RTCombatLibrary.h`.
const FName URTCombatLibrary::GuardPoolSource = FName(TEXT("D-292 · Status.Guarded"));
const FName URTCombatLibrary::ReactionReductionPoolSource = FName(TEXT("D-309 · Reactions.DamageReduction"));

FRTDamageResult URTCombatLibrary::ApplyDamage(int32 Damage, ERTDamageSource Source, int32 Shield,
	int32 TemporaryShield, int32 Health)
{
	const int32 SafeDamage = FMath::Max(0, Damage);
	const int32 SafeShield = FMath::Max(0, Shield);
	// `Clamp` e non `Max`: un temporaneo maggiore del totale renderebbe la base NEGATIVA, e da li' in poi
	// l'aritmetica direbbe cose false in silenzio invece di rompersi.
	const int32 SafeTemp = FMath::Clamp(TemporaryShield, 0, SafeShield);
	const int32 SafeBase = SafeShield - SafeTemp;

	// 1 — il temporaneo assorbe per primo, qualunque sia la sorgente: sta per scadere comunque, e consumare
	// prima la base significherebbe buttare via protezione destinata a sparire.
	const int32 AbsorbedByTemp = FMath::Min(SafeDamage, SafeTemp);
	int32 Remaining = SafeDamage - AbsorbedByTemp;
	const int32 NewTemp = SafeTemp - AbsorbedByTemp;

	// 2 — la base assorbe SOLO il danno diretto ([D-224]): `Burning`, danno da terreno e propagazione
	// elettrica la attraversano interi.
	const int32 AbsorbedByBase = (Source == ERTDamageSource::Direct)
		? FMath::Min(Remaining, SafeBase)
		: 0;
	Remaining -= AbsorbedByBase;
	const int32 NewBase = SafeBase - AbsorbedByBase;

	const int32 NewHealth = FMath::Max(0, Health - Remaining);

	return FRTDamageResult(NewHealth, NewBase + NewTemp, NewTemp);
}

ERTCombatOutcome URTCombatLibrary::ClassifyCombatOutcome(int32 HealthBefore, int32 HealthAfter, int32 AttackerDmgBonus)
{
	if (HealthBefore > 0 && HealthAfter <= 0) { return ERTCombatOutcome::Lethal; }
	if (HealthAfter == HealthBefore)          { return ERTCombatOutcome::ShieldAbsorbed; }
	if (AttackerDmgBonus > 0)                 { return ERTCombatOutcome::TerrainBonus; }
	return ERTCombatOutcome::Hit;
}

int32 URTCombatLibrary::EffectiveMoveRange(int32 BaseRange, bool bRooted)
{
	return bRooted ? 0 : BaseRange;
}

bool URTCombatLibrary::IsAbilityUsable(int32 CooldownRemaining)
{
	return CooldownRemaining <= 0;
}

bool URTCombatLibrary::IsIntentVisibleTo(int32 ObserverTeamId, int32 OwnerTeamId, bool bOwnerRevealed)
{
	// Alleati: sempre. Avversari: solo se il proprietario e' rivelato.
	return ObserverTeamId == OwnerTeamId || bOwnerRevealed;
}

bool URTCombatLibrary::CanPlayerControlUnit(int32 UnitTeamId, int32 PlayerTeamId, bool bUnitIsBotControlled)
{
	// Nessuna eccezione: si comanda solo la propria squadra (niente "possessione" di unita' avversarie).
	// E nemmeno un compagno che il bot sta gia' pianificando: sarebbe un piano scritto a due mani, e la mano
	// che vince e' sempre quella di `PlanBots()`, che gira a inizio turno e azzera i campi del piano.
	return UnitTeamId == PlayerTeamId && !bUnitIsBotControlled;
}

int32 URTCombatLibrary::ControlGroupForUnit(int32 IndexInTeam, int32 UnitsPerPlayer)
{
	// Fail-closed, e la scelta e' la stessa di `AssignSeats` un livello sopra: un formato che non dichiara
	// quante unita' comanda una persona non produce un gruppo valido per inerzia. `INDEX_NONE` non coincide
	// con nessun gruppo di giocatore, quindi il controllo si perde invece di essere regalato.
	if (UnitsPerPlayer <= 0 || IndexInTeam < 0)
	{
		return INDEX_NONE;
	}
	return IndexInTeam / UnitsPerPlayer;
}

bool URTCombatLibrary::CanPlayerControlUnitInGroup(int32 UnitTeamId, int32 UnitControlGroup,
	int32 PlayerTeamId, int32 PlayerControlGroup, bool bUnitIsBotControlled)
{
	// La regola di SQUADRA resta quella di sempre e non si riscrive: si aggiunge una condizione.
	if (!CanPlayerControlUnit(UnitTeamId, PlayerTeamId, bUnitIsBotControlled))
	{
		return false;
	}

	// ⛔ `INDEX_NONE` da una delle due parti non comanda niente: e' il gruppo che `ControlGroupForUnit`
	// restituisce quando il formato non dichiara `UnitsPerPlayer`, e due `INDEX_NONE` che si riconoscessero
	// fra loro rimetterebbero in piedi proprio il caso che il fail-closed toglie.
	if (UnitControlGroup == INDEX_NONE || PlayerControlGroup == INDEX_NONE)
	{
		return false;
	}
	return UnitControlGroup == PlayerControlGroup;
}

ERTHexTargetReason URTCombatLibrary::ClassifyHexTargeting(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To, int32 RangeCells)
{
	if (!Map)
	{
		return ERTHexTargetReason::NoMap; // FAIL-CLOSED: senza mappa la linea di tiro non e' verificabile
	}
	// La portata la CAPPA il terreno (Fumo): si valuta quella effettiva, non quella dichiarata dall'abilita'.
	// Senza, la preview accetta un bersaglio che CollectHexAttacks scarta poi in silenzio: slot speso, nessun
	// effetto, nessuna riga di log che lo spieghi.
	const int32 EffectiveRange = URTTerrainLibrary::EffectiveTargetingRange(Map, From, To, RangeCells);
	if (URTHexLibrary::HexDistance(From, To) > FMath::Max(0, EffectiveRange))
	{
		return ERTHexTargetReason::OutOfRange; // la portata si valuta PRIMA: e' un difetto diverso da "coperto"
	}
	return URTHexVisionLibrary::HasLineOfSight(Map, From, To)
		? ERTHexTargetReason::Ok : ERTHexTargetReason::NoLineOfSight;
}

ERTTargetRefusal URTCombatLibrary::RefusalForObserver(ERTHexTargetReason Reason, bool bTargetKnownToObserver)
{
	// 🔴 **La conoscenza si valuta PRIMA della geometria, e l'ordine e' il requisito.**
	//
	// Invertirlo produrrebbe un difetto che nessun test di forma vedrebbe: su un bersaglio ignoto
	// classificato `NoLineOfSight` uscirebbe «copertura», e il giocatore avrebbe appreso che li' c'e'
	// qualcuno da un messaggio che parla d'altro. Il velo non e' un filtro applicato all'esito: e' la
	// domanda che viene per prima.
	if (!bTargetKnownToObserver)
	{
		return ERTTargetRefusal::Nothing;
	}

	switch (Reason)
	{
	case ERTHexTargetReason::Ok:
		return ERTTargetRefusal::None;

	case ERTHexTargetReason::OutOfRange:
		return ERTTargetRefusal::Range;

	case ERTHexTargetReason::NoLineOfSight:
		return ERTTargetRefusal::Cover;

	case ERTHexTargetReason::NoMap:
		// ⚠️ Fail-closed, e per la stessa ragione di `ClassifyHexTargeting`: senza mappa autorevole la
		// linea non e' verificabile, quindi non si afferma niente su di essa. `Nothing` qui non dice
		// «non c'e' nessuno»: dice «non ho nulla da mostrarti», che e' l'unica cosa vera.
		return ERTTargetRefusal::Nothing;
	}

	// ⛔ **Nessun `default:` nello switch, ed e' una scelta.** Con un `default` un enumerato nuovo
	// scivolerebbe in silenzio su «non dire niente», e la feature perderebbe un caso con la suite verde.
	// Senza, `-Wswitch` lo rende un errore di compilazione qui e ora — che e' cio' che il fail-closed di
	// questa funzione dichiara di volere.
	checkNoEntry();
	return ERTTargetRefusal::Nothing;
}

FString URTCombatLibrary::OutOfRangeDiagnostic(int32 DeclaredRange, int32 EffectiveRange)
{
	// Il caso ordinario: nessun terreno ha ridotto niente, e il numero dichiarato E' il limite applicato.
	// Si stampa **una** portata, perche' due numeri uguali affiancati insegnano a ignorarli.
	if (EffectiveRange >= DeclaredRange)
	{
		return FString::Printf(TEXT("fuori portata (max %d)"), DeclaredRange);
	}

	// 🔴 Il caso che apriva la issue. Si dice per PRIMO il limite applicato — e' quello che spiega il
	// rifiuto — e poi la dichiarata, che senza contesto sembrerebbe smentirlo.
	//
	// ⚠️ Il terreno non viene NOMINATO: questa funzione riceve due interi e non sa quale superficie ha
	// cappato. Nominarlo richiederebbe che `EffectiveTargetingRange` restituisse anche la cella
	// responsabile, cioe' un secondo valore di ritorno per un log. Il numero basta a togliere l'inganno:
	// chi legge `max 2` non conclude piu' che il classificatore sia rotto.
	return FString::Printf(TEXT("fuori portata (max %d applicato, %d dichiarata: il terreno sulla linea la riduce)"),
		EffectiveRange, DeclaredRange);
}

bool URTCombatLibrary::CanTargetHexCell(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
	int32 RangeCells)
{
	return ClassifyHexTargeting(Map, From, To, RangeCells) == ERTHexTargetReason::Ok;
}

int32 URTCombatLibrary::EffectiveAttackPower(int32 BasePower, int32 OccupantDamageBonus)
{
	return FMath::Max(0, BasePower + OccupantDamageBonus);
}

TArray<int32> URTCombatLibrary::NewlyDefeated(const TArray<int32>& HealthBefore, const TArray<int32>& HealthAfter)
{
	TArray<int32> Result;
	const int32 Count = FMath::Min(HealthBefore.Num(), HealthAfter.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		if (HealthBefore[i] > 0 && HealthAfter[i] <= 0)
		{
			Result.Add(i); // viva prima, morta ora
		}
	}
	return Result;
}
