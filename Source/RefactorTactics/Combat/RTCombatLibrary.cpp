#include "Combat/RTCombatLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

namespace
{
	/**
	 * LA TABELLA `ERTHexTargetReason` → `ERTTargetRefusal`, e **niente** velo — `#3064`.
	 *
	 * ⛔ **Privata al `.cpp`, e la privatezza E' il punto.** Le due porte pubbliche che ci passano portano
	 * ciascuna la propria domanda di conoscenza: `RefusalForObserver` la pone, perche' il suo bersaglio e'
	 * un'unita' che il velo puo' nascondere; `DescribeCellTargetRefusal` non ha nulla da porre, perche' il
	 * suo bersaglio e' una cella. Esporre questa tabella aggiungerebbe una TERZA porta, senza domanda, e un
	 * sito a unita' che la usasse sembrerebbe corretto pur non essendolo.
	 */
	ERTTargetRefusal RefusalForReason(ERTHexTargetReason Reason)
	{
		switch (Reason)
		{
		case ERTHexTargetReason::Ok:
			return ERTTargetRefusal::None;

		case ERTHexTargetReason::OutOfRange:
			return ERTTargetRefusal::Range;

		case ERTHexTargetReason::NoLineOfSight:
			return ERTTargetRefusal::Cover;

		case ERTHexTargetReason::TooClose:
			// Il gesto e' l'OPPOSTO di `Range`, che porta scritto «avvicinati»: qui si indietreggia.
			return ERTTargetRefusal::TooClose;

		case ERTHexTargetReason::OtherLayer:
			// Nessuno dei gesti NEL piano aiuta: ne' avvicinarsi ne' indietreggiare cambia il `Layer`, e
			// «spostati di lato» prometterebbe una traiettoria che non e' mai stata costruita.
			return ERTTargetRefusal::OtherLayer;

		case ERTHexTargetReason::NoMap:
			// ⚠️ Fail-closed, e per la stessa ragione di `ClassifyHexTargeting`: senza mappa autorevole la
			// linea non e' verificabile, quindi non si afferma niente su di essa. `Nothing` qui non dice
			// «non c'e' nessuno»: dice «non ho nulla da mostrarti», che e' l'unica cosa vera.
			return ERTTargetRefusal::Nothing;
		}

		// ⌫ **QUI C'ERA LA PROMESSA CHE `-Wswitch` ROMPESSE LA BUILD, E QUESTO REPOSITORY L'HA MISURATA
		// FALSA** (`#3080`, la stessa correzione e' scritta in `ARTHUD::RefusalText`). Uno `switch` senza
		// `default:` produce un **warning** su MSVC (C4061/C4062), non un errore, e la build non lo alza:
		// `TooClose` e `OtherLayer` sono entrati a due issue di distanza e nessuno si e' fermato.
		//
		// 🔴 **E da `#3064` la posta e' piu' alta, non piu' bassa**: questa tabella gira ora a OGNI click a
		// cella — esito `Ok` compreso — e non piu' solo sul ramo di rifiuto a unita'. Un valore nuovo
		// dell'enum arriverebbe qui nel percorso ordinario del giocatore.
		//
		// 🔑 **Cio' che regge davvero e' un banco**: `RefactorTactics.Combat.RefusalCoversEveryReason` legge
		// i valori dalla **reflection** e li confronta PRIMA di chiamare, quindi un motivo scoperto lo rende
		// rosso nominandolo invece di arrivare fin qui.
		//
		// ⚠️ `checkNoEntry()` resta, e resta l'ULTIMA difesa: se ci si arriva comunque, morire
		// rumorosamente e' preferibile a collassare in silenzio su `Nothing`, che e' il valore riservato al
		// velo — un motivo nuovo che tacesse per sbaglio sembrerebbe privacy e sarebbe un difetto.
		checkNoEntry();
		return ERTTargetRefusal::Nothing;
	}
}

// Le due provenienze dei pool d'assorbimento (`#2213`). La ragione per cui la seconda non nomina un
// `ActionId` sta sulla dichiarazione, in `RTCombatLibrary.h`.
const FName URTCombatLibrary::GuardPoolSource = FName(TEXT("D-292 · Status.Guarded"));
const FName URTCombatLibrary::ReactionReductionPoolSource = FName(TEXT("D-309 · Reactions.DamageReduction"));
const FName URTCombatLibrary::GuardPerHitSource = FName(TEXT("D-408 · Status.Guarded"));

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
	const FRTCellId& To, int32 RangeCells, ERTLineOfSightPolicy Policy, int32 MinRangeCells)
{
	if (!Map)
	{
		return ERTHexTargetReason::NoMap; // FAIL-CLOSED: senza mappa la linea di tiro non e' verificabile
	}
	// La portata la CAPPA il terreno (Fumo): si valuta quella effettiva, non quella dichiarata dall'abilita'.
	// Senza, la preview accetta un bersaglio che CollectHexAttacks scarta poi in silenzio: slot speso, nessun
	// effetto, nessuna riga di log che lo spieghi.
	//
	// 🔴 **E si valuta PRIMA della licenza, non dopo.** Il tiro indiretto toglie il requisito della LINEA e
	// nient'altro: una portata cappata dal Fumo resta cappata anche per un mortaio, altrimenti «non serve
	// vedere» diventerebbe «non serve avvicinarsi», che nessuno ha deciso.
	const int32 EffectiveRange = URTTerrainLibrary::EffectiveTargetingRange(Map, From, To, RangeCells);
	const int32 Distance = URTHexLibrary::HexDistance(From, To);
	if (Distance > FMath::Max(0, EffectiveRange))
	{
		return ERTHexTargetReason::OutOfRange; // la portata si valuta PRIMA: e' un difetto diverso da "coperto"
	}

	// ➕ **IL MINIMO, subito dopo il massimo e nello stesso blocco** (`#2950`). I due verdetti di portata
	// stanno insieme perche' rispondono alla stessa domanda — *«sono alla distanza giusta?»* — e separarli
	// avrebbe messo fra loro una regola che non c'entra.
	//
	// ⛔ **Il minimo NON passa da `EffectiveTargetingRange`, ed e' deliberato.** Il Fumo cappa quanto lontano
	// si arriva; non cambia quanto vicino un'arma smette di funzionare. Capparlo significherebbe che una
	// nube rende improvvisamente usabile in mischia un mortaio, che nessuno ha deciso.
	//
	// ⚠️ Dopo il massimo, non prima: se una distanza violasse entrambi i limiti — possibile solo con un
	// catalogo incoerente, `MinRangeCells > RangeCells` — il motivo resta quello storico, e un dato
	// incoerente non cambia in silenzio il messaggio di un caso che gia' funzionava.
	if (MinRangeCells > 0 && Distance < MinRangeCells)
	{
		return ERTHexTargetReason::TooClose;
	}

	// ➕ **IL PIANO, E NON E' UNA DOMANDA DI PORTATA** (`#2951`, [D-393]). La verticalita' non e' un
	// asse di targeting: un'azione bersaglia celle del PROPRIO `Layer`, e puntare un altro piano e' un
	// rifiuto DICHIARATO invece di un colpo che manca in silenzio.
	//
	// 🔴 **Il difetto che chiude e' un'incoerenza fra le tre forme, non un'assenza.** `HexHitCells`
	// costruisce l'impronta di `Line` con `HexLine(From, Target)`, che scrive `A.Layer` su ogni cella —
	// il piano del TIRATORE — mentre `Area` usa `HexArea(Target, R)`, che scrive `Center.Layer`.
	// ∴ prima di questa riga un'azione `Shape::Line` verso una piattaforma era ACCETTATA e non toccava
	// nessuno: la cella del bersaglio non stava nell'insieme investito, e non c'era ne' un rifiuto ne'
	// una riga di log.
	//
	// ⚠️ **Dopo la portata, non prima**, con lo stesso argomento con cui `#2950` ha messo il minimo
	// dopo il massimo: un bersaglio che violi anche la gittata conserva il motivo storico, e un caso che
	// gia' funzionava non cambia messaggio in silenzio.
	//
	// ⛔ **E PRIMA della licenza**: il tiro indiretto toglie il requisito della LINEA e nient'altro. Un
	// mortaio non scavalca un piano — [D-380] non gliel'ha concesso, e concederglielo qui sarebbe la
	// seconda sede di una decisione che nessuno ha preso.
	if (From.Layer != To.Layer)
	{
		return ERTHexTargetReason::OtherLayer;
	}

	// ➕ **LA LICENZA DELL'AZIONE** (`#2870`, [D-378]). Non e' un bypass di `HasLineOfSight`: e' la domanda
	// che viene prima — *questa azione la linea la CHIEDE?* — e solo un'azione che la chiede puo' esserne
	// rifiutata.
	//
	// ⛔ **Non si guarda cosa c'e' sulla cella, e l'assenza e' la regola.** Questa funzione riceve due celle e
	// una mappa: non ha, e non deve avere, l'elenco delle unita'. Un `NotRequired` che consultasse
	// l'occupazione per decidere restituirebbe esiti diversi fra un bersaglio vuoto e uno abitato da un
	// ignoto, e quella differenza sarebbe **essa stessa** il canale ([D-225]) — un rilevatore di presenze
	// travestito da validazione. `BlindFireIsNotAnEnemyDetector` lo pinna.
	if (Policy == ERTLineOfSightPolicy::NotRequired)
	{
		return ERTHexTargetReason::Ok;
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

	// ⌫ **Lo `switch` non e' piu' qui: e' `RefusalForReason`, statica privata in cima a questo file**
	// (`#3064`). Non e' un riordino estetico — e' cio' che permette al percorso a CELLA di riusare la
	// tabella **senza** passare da un flag di conoscenza che, non avendo un'unita' bersaglio, avrebbe
	// dovuto inventare. Questa funzione resta l'unico posto in cui il velo entra, che e' esattamente cio'
	// che il suo header dichiara: *«La conoscenza entra qui e da nessun'altra parte»*.
	return RefusalForReason(Reason);
}

FRTCellTargetRefusal URTCombatLibrary::DescribeCellTargetRefusal(const URTHexMapAsset* Map,
	const FRTCellId& From, const FRTCellId& To, int32 RangeCells, ERTLineOfSightPolicy Policy)
{
	FRTCellTargetRefusal Out;

	// ⛔ **Nessuna unita' entra da qui, e non e' disciplina: e' la firma delle tre funzioni chiamate.**
	// `ClassifyHexTargeting` riceve due celle e una mappa e dichiara l'assenza dell'elenco delle unita' come
	// REQUISITO accanto alla licenza di [D-378]; `EffectiveTargetingRange` scorre il catalogo dei terreni
	// lungo `HexLine`; `DescribeLineOfSight` ha per unica sorgente `const URTHexMapAsset*`. ∴ due mondi che
	// differiscono solo per un nemico che l'osservatore non conosce, sulla cella bersaglio, escono di qui con
	// `Out` identico campo per campo, e nessun filtro a valle deve rimediare a niente. [D-225]
	Out.Refusal = RefusalForReason(ClassifyHexTargeting(Map, From, To, RangeCells, Policy));

	// 🔑 **Calcolate SEMPRE, anche quando l'esito e' `None`.** `ARTHUD::RefusalText` legge il numero solo per
	// `Range` e `ComputeRefusedShotLine` la geometria solo per `Cover`, quindi altrove sono inerti — ed e'
	// precisamente cio' che rende sicuro calcolarle incondizionatamente invece di ricordarsi quando servono.
	// E' la disciplina che `#2800` e `#3085` hanno gia' imposto al sito a unita'.
	//
	// 🔴 **La portata la si CHIEDE alla stessa funzione che il classificatore ha applicato**, non la si
	// rideriva: riscrivere qui il cap del terreno direbbe il vero solo finche' nessuno tocca il catalogo,
	// ed e' l'inganno che `#2766` ha tolto dal log.
	//
	// ⚠️ `Map` nullo non e' un caso speciale da guardare: `EffectiveTargetingRange` restituisce `RangeCells`
	// invariato, `DescribeLineOfSight` un esito senza blocco, e `Refusal` e' gia' `Nothing` per il
	// fail-closed del classificatore. Chi non ha una mappa non riceve affermazioni su una geometria che
	// nessuno ha verificato.
	Out.EffectiveRange = URTTerrainLibrary::EffectiveTargetingRange(Map, From, To, RangeCells);

	// ⛔ **`DescribeLineOfSight` e non `AuthorizedSightLines`, ed e' una scelta [D-225].** Quella libreria
	// filtra per conoscenza nella propria FIRMA, e il filtro e' sul BERSAGLIO: per una cella non esiste un
	// `bKnownToObserver` onesto da passarle, e doverne **scegliere** uno sarebbe gia' il difetto — il suo
	// `continue` fa SPARIRE la voce, quindi «linea presente» contro «linea assente» diventerebbe un
	// rilevatore di presenze il giorno in cui quel flag smettesse di essere una costante. Qui la geometria
	// si produce nuda; il velo che conta — quello sull'OSTACOLO — sta a valle, in
	// `ARTHUD::ComputeRefusedShotLine`, che e' il punto in cui il segno diventa visibile.
	Out.Sight = URTHexVisionLibrary::DescribeLineOfSight(Map, From, To);

	return Out;
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
	int32 RangeCells, ERTLineOfSightPolicy Policy)
{
	return ClassifyHexTargeting(Map, From, To, RangeCells, Policy) == ERTHexTargetReason::Ok;
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
