// Blind Fire offensivo — la prima azione che fa DANNO senza linea di tiro, e la regola che le da' un
// feedback (`#2890`, [D-379]).
//
// `#2870` aveva reso il requisito della linea un dato dell'azione e lo aveva dichiarato su `MistVeil`, che
// non colpisce. Questa suite misura le due cose che quella issue aveva rinviato: che un'azione OFFENSIVA
// possa dichiarare il tiro indiretto, e che un colpo al buio a segno produca un feedback — senza che il
// canale si allarghi oltre cio' che [D-379] ha deciso di aprire.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTTeamKnowledge.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPointerInteraction.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto compila in unity build. */
	UWorld* MakeMortarWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyMortarWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTUnit* SpawnMortarUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * L'azione si cerca per la sua POLICY e per il fatto che COLPISCA, mai per il nome: se domani il
	 * mortaio cambiasse identita' o un'altra azione ereditasse la licenza, questi test seguono il dato.
	 * E' la stessa disciplina di `FindAbilityWithPolicy` nella suite di `#2870`.
	 */
	int32 FindOffensiveBlindFire(const ARTUnit* U)
	{
		for (int32 i = 0; i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && !A->bSelfTarget && A->Def.bCountsAsAttack
				&& A->Def.LineOfSightPolicy == ERTLineOfSightPolicy::NotRequired)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	struct FMortarBench
	{
		UWorld* World = nullptr;
		URTHexMapAsset* Map = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTUnit* Mine = nullptr;
	};

	/** Stessa arena della suite `#2870`: muro alla vista lungo `q = 0`, tiratore in `(-1,0)`. */
	bool SetUpMortarBench(FMortarBench& B)
	{
		B.World = MakeMortarWorld();
		if (!B.World) { return false; }
		B.Map = URTMatchSetupLibrary::MakeTestArena(B.World);
		ARTHexMapActor* MapActor = B.World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = B.Map;
		B.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		B.Mine = SpawnMortarUnit(B.World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-1, 0, 0));
		B.PC = B.World->SpawnActor<ARTPlayerController>();
		return B.PC != nullptr && B.Mine != nullptr;
	}

	/** Una conoscenza vuota ma LEGGIBILE: versione corrente, nessun contatto, nessuna cella. */
	FRTTeamKnowledge EmptyKnowledge(int32 TeamId, int32 TurnNumber)
	{
		FRTTeamKnowledge K;
		K.Version = FRTTeamKnowledge::CurrentVersion;
		K.TeamId = TeamId;
		K.TurnNumber = TurnNumber;
		return K;
	}
}

// ======================================================================================================
// 1-3 — l'azione esiste, paga un prezzo, ed e' raggiungibile dal percorso del GIOCATORE
// ======================================================================================================

/**
 * **Test 1** — il catalogo dichiara un'azione che COLPISCE senza chiedere la linea, e paga un prezzo.
 *
 * 🔴 **Il prezzo si misura contro il gemello, non contro numeri scritti qui.** Un mortaio che costasse
 * quanto `Action.CircularAoE` sarebbe la stessa azione con un vincolo in meno, e un test che asserisse
 * `Damage == 12` resterebbe verde anche il giorno in cui il gemello scendesse a 12: misurerebbe una
 * costante, non una relazione.
 *
 * ⛔ **Verifica di mutazione**: portare il danno del mortaio a 18, o la sua ricarica a 2, deve rendere
 * ROSSO questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMortarCatalogPaysAPriceTest,
	"RefactorTactics.BlindFireOffensive.MortarPaysAPriceAgainstItsLineOfSightTwin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMortarCatalogPaysAPriceTest::RunTest(const FString&)
{
	const FRTActionDef Mortar = URTCatalogLibrary::FindCoreAction(TEXT("Action.Mortar"));
	const FRTActionDef Twin   = URTCatalogLibrary::FindCoreAction(TEXT("Action.CircularAoE"));

	if (!TestFalse(TEXT("`Action.Mortar` e' nel catalogo core"), Mortar.ActionId.IsNone())) { return false; }
	if (!TestFalse(TEXT("premessa: `Action.CircularAoE` e' il riferimento"), Twin.ActionId.IsNone())) { return false; }

	TestEqual(TEXT("il mortaio NON chiede la linea di tiro"),
		Mortar.LineOfSightPolicy, ERTLineOfSightPolicy::NotRequired);
	TestTrue(TEXT("e COLPISCE: e' un'aggressione dichiarata"), Mortar.bCountsAsAttack);

	// Il gemello resta il caso normale: se aprisse anche lui, il confronto non misurerebbe piu' niente.
	TestEqual(TEXT("il gemello a vista continua a chiederla"),
		Twin.LineOfSightPolicy, ERTLineOfSightPolicy::Required);

	// ⚠️ Il danno si chiede a `URTCatalogLibrary::DeclaredDamage`, che e' la funzione che il progetto usa
	// gia' per questa domanda: un helper locale sarebbe una seconda definizione di «quanto fa male», e
	// `Meta.AnonymousHelpersDoNotCollideUnderUnity` prende proprio questo.
	TestTrue(TEXT("il mortaio colpisce MENO del gemello a vista"),
		URTCatalogLibrary::DeclaredDamage(Mortar) < URTCatalogLibrary::DeclaredDamage(Twin));
	TestTrue(TEXT("e si ricarica piu' lentamente"), Mortar.CooldownTurns > Twin.CooldownTurns);

	// ⚠️ La portata NON e' un asse del prezzo, ed e' dichiarato: un mortaio e' un'arma di distanza, e
	// accorciarlo per punirlo contraddirebbe cio' che l'azione E'.
	TestEqual(TEXT("la portata resta quella del gemello"), Mortar.RangeCells, Twin.RangeCells);
	return true;
}

/**
 * **Test 2** — il mortaio arriva dal percorso di INPUT, non dall'harness.
 *
 * 🔑 E' il vincolo che `#2870` aveva posto e che questa issue eredita: *«non considerare concluso il
 * lavoro se funziona solo attraverso Scenario Harness»*. Il test clicca la cella come farebbe una persona,
 * su un'unita' selezionata dalla stessa porta del giocatore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMortarReachableFromPlayerInputTest,
	"RefactorTactics.BlindFireOffensive.MortarIsTargetableThroughABlockerFromPlayerInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMortarReachableFromPlayerInputTest::RunTest(const FString&)
{
	FMortarBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpMortarBench(B))) { DestroyMortarWorld(B.World); return false; }

	const int32 Idx = FindOffensiveBlindFire(B.Mine);
	if (!TestTrue(TEXT("il kit contiene un'offensiva a tiro indiretto"), Idx != INDEX_NONE))
	{
		DestroyMortarWorld(B.World); return false;
	}

	B.PC->SelectActorForTest(B.Mine);
	B.Mine->SelectAbility(Idx);
	TestEqual(TEXT("il mortaio chiede una CELLA"),
		B.PC->GetPointerTargetKind(), ERTPointerTargetKind::Cell);

	const FRTCellId Oltre(1, 0, 0);
	// Premessa MISURATA: senza il muro il test proverebbe solo che si puo' mirare a una cella libera,
	// che era gia' vero prima di `#2870`.
	TestTrue(TEXT("premessa: quella cella NON e' in linea di tiro"),
		URTCombatLibrary::ClassifyHexTargeting(B.Map, B.Mine->Cell, Oltre, /*RangeCells=*/ 4,
			ERTLineOfSightPolicy::Required) == ERTHexTargetReason::NoLineOfSight);

	TestTrue(TEXT("il mortaio accetta la cella non visibile"), B.PC->HandleTargetCell(Oltre));
	TestTrue(TEXT("il piano e' a cella"), B.Mine->bAttackTargetsCell);
	TestTrue(TEXT("e la cella e' quella scelta"), B.Mine->PlannedAttackCell == Oltre);
	TestEqual(TEXT("l'azione pianificata e' quella armata"), B.Mine->PlannedAbilityIndex, Idx);

	DestroyMortarWorld(B.World);
	return true;
}

/**
 * **Test 3** — la licenza toglie la LINEA, non la portata.
 *
 * ⛔ **Verifica di mutazione**: spostare il controllo di portata DOPO il ramo `NotRequired` in
 * `ClassifyHexTargeting` deve rendere ROSSO questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMortarStillObeysRangeTest,
	"RefactorTactics.BlindFireOffensive.MortarStillObeysRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMortarStillObeysRangeTest::RunTest(const FString&)
{
	const FRTActionDef Mortar = URTCatalogLibrary::FindCoreAction(TEXT("Action.Mortar"));
	if (!TestFalse(TEXT("premessa: il mortaio esiste"), Mortar.ActionId.IsNone())) { return false; }

	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 8);
	const FRTCellId From(0, 0, 0);

	TestEqual(TEXT("una cella entro portata e' bersagliabile al buio"),
		URTCombatLibrary::ClassifyHexTargeting(Map, From, FRTCellId(Mortar.RangeCells, 0, 0),
			Mortar.RangeCells, Mortar.LineOfSightPolicy), ERTHexTargetReason::Ok);

	// Tiro indiretto non significa portata infinita, e il rifiuto lo NOMINA: `OutOfRange`, non un generico
	// «no». Un'asserzione sul solo booleano non distinguerebbe questo caso da una linea bloccata.
	TestEqual(TEXT("una cella oltre la portata resta rifiutata, e per portata"),
		URTCombatLibrary::ClassifyHexTargeting(Map, From, FRTCellId(Mortar.RangeCells + 1, 0, 0),
			Mortar.RangeCells, Mortar.LineOfSightPolicy), ERTHexTargetReason::OutOfRange);
	return true;
}

// ======================================================================================================
// 4-8 — «chi hai colpito, lo hai trovato»: la regola nuova, e i suoi confini
// ======================================================================================================

/**
 * **Test 4** — il colpo rivela la VITTIMA, e soltanto lei.
 *
 * 🔴 **E' la riga che impedisce all'area di diventare un radar.** La tentazione naturale sarebbe rivelare
 * le celle investite: un'AoE di raggio 1 ne copre sette, e sette celle rivelate per un colpo a segno
 * direbbero al giocatore molto piu' di quanto abbia pagato. Si rivela chi il colpo ha TROVATO, uno.
 *
 * ⛔ **Verifica di mutazione**: far rivelare a `RevealByHit` anche i non colpiti — o passarle le celle
 * dell'area invece delle vittime — deve rendere ROSSO questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRevealNamesOnlyTheVictimTest,
	"RefactorTactics.BlindFireOffensive.HitRevealsOnlyTheVictim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRevealNamesOnlyTheVictimTest::RunTest(const FString&)
{
	const int32 Turno = 5;
	const int32 ColpitoId = 42;
	const int32 PresenteMaNonColpitoId = 43;

	TArray<FRTLastKnownContact> Vittime;
	Vittime.Add(FRTLastKnownContact(ColpitoId, FRTCellId(1, 0, 0), /*ignorato*/ 0));

	const FRTTeamKnowledge Dopo = URTTeamKnowledgeLibrary::RevealByHit(
		EmptyKnowledge(/*TeamId=*/ 0, Turno), Vittime, Turno);

	TestEqual(TEXT("un solo contatto nuovo"), Dopo.Contacts.Num(), 1);

	const FRTLastKnownContact* Trovato = URTTeamKnowledgeLibrary::FindContact(Dopo, ColpitoId);
	if (!TestNotNull(TEXT("il colpito e' diventato un contatto"), Trovato)) { return false; }
	TestTrue(TEXT("nella cella in cui il colpo lo ha trovato"), Trovato->Cell == FRTCellId(1, 0, 0));
	TestEqual(TEXT("e col turno del colpo, da cui parte la scadenza"), Trovato->TurnNumber, Turno);

	TestNull(TEXT("chi NON e' stato colpito resta ignoto"),
		URTTeamKnowledgeLibrary::FindContact(Dopo, PresenteMaNonColpitoId));
	return true;
}

/**
 * **Test 5** — la rivelazione SCADE, come ogni contatto.
 *
 * 🔑 E' cio' che rende il sondaggio pagato **una volta sola**: un contatto che non scadesse
 * trasformerebbe un colpo fortunato in una posizione nota per sempre. La scadenza non e' scritta da
 * questa feature — e' quella di `Observe` — e il test misura che il contatto nuovo la EREDITI invece di
 * sfuggirle, il che e' esattamente cio' che un `TurnNumber` sbagliato produrrebbe.
 *
 * ⛔ **Verifica di mutazione**: scrivere in `RevealByHit` un `TurnNumber` piu' alto di quello del colpo
 * deve rendere ROSSO questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRevealExpiresTest,
	"RefactorTactics.BlindFireOffensive.RevealFromHitExpiresLikeAnySighting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRevealExpiresTest::RunTest(const FString&)
{
	const int32 TurnoDelColpo = 5;
	const int32 VittimaId = 42;

	TArray<FRTLastKnownContact> Vittime;
	Vittime.Add(FRTLastKnownContact(VittimaId, FRTCellId(1, 0, 0), /*ignorato*/ 0));
	const FRTTeamKnowledge Rivelata = URTTeamKnowledgeLibrary::RevealByHit(
		EmptyKnowledge(/*TeamId=*/ 0, TurnoDelColpo), Vittime, TurnoDelColpo);

	if (!TestNotNull(TEXT("premessa: il contatto esiste"),
		URTTeamKnowledgeLibrary::FindContact(Rivelata, VittimaId))) { return false; }

	// La scadenza la applica `Observe`, che e' la porta da cui la conoscenza passa a ogni turno. Si simula
	// il turno seguente SENZA osservatori: nessuno vede niente, quindi sopravvive solo cio' che non e'
	// scaduto. E' la stessa via del ricordo di un avvistamento, non una regola propria del colpo.
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
	const TArray<FRTPerceiver> NessunOsservatore;
	const TArray<FRTLastKnownContact> NessunNemicoVisto;

	const int32 EntroLaVita = TurnoDelColpo + URTTeamKnowledgeLibrary::ContactLifetimeTurns;
	const FRTTeamKnowledge Ancora = URTTeamKnowledgeLibrary::Observe(Map, /*TeamId=*/ 0, EntroLaVita,
		NessunOsservatore, NessunNemicoVisto, Rivelata);
	TestNotNull(TEXT("entro la vita del contatto il ricordo c'e' ancora"),
		URTTeamKnowledgeLibrary::FindContact(Ancora, VittimaId));

	const FRTTeamKnowledge Scaduta = URTTeamKnowledgeLibrary::Observe(Map, /*TeamId=*/ 0, EntroLaVita + 1,
		NessunOsservatore, NessunNemicoVisto, Rivelata);
	TestNull(TEXT("oltre, la rivelazione e' scaduta come un avvistamento qualunque"),
		URTTeamKnowledgeLibrary::FindContact(Scaduta, VittimaId));
	return true;
}

/**
 * **Test 6 — CANARY** — la rivelazione concede un CONTATTO, non la VISTA.
 *
 * 🔴 **E' il confine fra questa decisione e quella che l'ha preceduta, e la sua misura.** [D-378] ha
 * chiuso il tiro indiretto come rilevatore nel planning; [D-379] apre un canale nella risoluzione. Il
 * modo in cui il secondo puo' riaprire il primo non e' teorico: basterebbe che `RevealByHit`, per
 * comodita', aggiungesse la cella colpita a `VisibleCells` o a `ExploredCells`. Sarebbe **una riga**, e
 * regalerebbe la geometria di un posto in cui nessuno ha guardato — cioe' esattamente cio' che [D-225]
 * nega e che [D-373] ha dichiarato pubblico soltanto per la **topologia**.
 *
 * ⚠️ **Il targeting non e' fra i consumatori da controllare qui, ed e' un fatto strutturale**:
 * `ClassifyHexTargeting` non ha un ingresso di conoscenza — non puo' imparare niente perche' non ha da
 * dove. Asserirlo confrontando la funzione con se stessa sarebbe una tautologia travestita da canary; la
 * proprieta' falsificabile e' questa, e vive dove il dato viene scritto.
 *
 * ⛔ **Verifica di mutazione**: aggiungere in `RevealByHit` la cella della vittima a `VisibleCells` o a
 * `ExploredCells` deve rendere ROSSO questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRevealGrantsContactNotVisionTest,
	"RefactorTactics.BlindFireOffensive.RevealGrantsAContactNotVision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRevealGrantsContactNotVisionTest::RunTest(const FString&)
{
	const int32 Turno = 5;
	const FRTCellId DoveEStato(1, 0, 0);

	const FRTTeamKnowledge Prima = EmptyKnowledge(/*TeamId=*/ 0, Turno);
	// Premessa misurata: la squadra non vede e non ha mai visto quella cella. Senza, il test non
	// distinguerebbe «la rivelazione non concede vista» da «la vista c'era gia'».
	if (!TestFalse(TEXT("premessa: la cella non e' visibile"), Prima.VisibleCells.Contains(DoveEStato)))
	{
		return false;
	}
	if (!TestFalse(TEXT("premessa: ne' mai esplorata"), Prima.ExploredCells.Contains(DoveEStato)))
	{
		return false;
	}

	TArray<FRTLastKnownContact> Vittime;
	Vittime.Add(FRTLastKnownContact(/*StableUnitId=*/ 42, DoveEStato, /*ignorato*/ 0));
	const FRTTeamKnowledge Dopo = URTTeamKnowledgeLibrary::RevealByHit(Prima, Vittime, Turno);

	// Cio' che la rivelazione concede: il contatto.
	TestNotNull(TEXT("il colpito e' un contatto"), URTTeamKnowledgeLibrary::FindContact(Dopo, 42));

	// Cio' che NON concede, ed e' il punto del canary.
	TestFalse(TEXT("la cella colpita NON diventa visibile"), Dopo.VisibleCells.Contains(DoveEStato));
	TestFalse(TEXT("e NON diventa esplorata"), Dopo.ExploredCells.Contains(DoveEStato));
	TestEqual(TEXT("nessuna cella visibile in piu'"), Dopo.VisibleCells.Num(), Prima.VisibleCells.Num());
	TestEqual(TEXT("nessuna cella esplorata in piu'"), Dopo.ExploredCells.Num(), Prima.ExploredCells.Num());
	return true;
}

/**
 * **Test 7** — due colpi sullo stesso bersaglio non producono due contatti.
 *
 * ⚠️ Non e' pedanteria: un'area che investe lo stesso nemico con due intenti, o due unita' che sparano
 * nello stesso turno, sono casi ordinari. Un secondo contatto accodato renderebbe `AwarenessOfUnit` —
 * che scorre l'array e si ferma al primo — dipendente dall'ordine di inserimento invece che dal fatto
 * piu' recente, e la memoria porterebbe due verita' sulla stessa unita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRevealDoesNotDuplicateContactsTest,
	"RefactorTactics.BlindFireOffensive.RevealDoesNotDuplicateContacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRevealDoesNotDuplicateContactsTest::RunTest(const FString&)
{
	const int32 Turno = 5;
	const int32 VittimaId = 42;
	const FRTCellId Seconda(2, 0, 0);

	TArray<FRTLastKnownContact> Primo;
	Primo.Add(FRTLastKnownContact(VittimaId, FRTCellId(1, 0, 0), /*ignorato*/ 0));
	const FRTTeamKnowledge DopoUno = URTTeamKnowledgeLibrary::RevealByHit(
		EmptyKnowledge(/*TeamId=*/ 0, Turno), Primo, Turno);

	// Secondo colpo sullo stesso bersaglio, trovato altrove: il contatto si AGGIORNA, non si accoda.
	TArray<FRTLastKnownContact> Secondo;
	Secondo.Add(FRTLastKnownContact(VittimaId, Seconda, /*ignorato*/ 0));
	const FRTTeamKnowledge DopoDue = URTTeamKnowledgeLibrary::RevealByHit(DopoUno, Secondo, Turno);

	TestEqual(TEXT("resta UN solo contatto per quel bersaglio"), DopoDue.Contacts.Num(), 1);
	const FRTLastKnownContact* Trovato = URTTeamKnowledgeLibrary::FindContact(DopoDue, VittimaId);
	if (!TestNotNull(TEXT("ed e' ancora li'"), Trovato)) { return false; }
	TestTrue(TEXT("con la cella del colpo piu' recente"), Trovato->Cell == Seconda);
	return true;
}

/**
 * **Test 8** — una conoscenza ILLEGGIBILE non si arricchisce.
 *
 * ⚠️ Fail-closed, come `Observe`: aggiungere un contatto a una memoria di versione ignota produrrebbe una
 * struttura per meta' interpretabile, e nessuno se ne accorgerebbe finche' una decisione del bot non ne
 * dipendesse. E' la stessa guardia, misurata invece che assunta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRevealIsFailClosedOnVersionTest,
	"RefactorTactics.BlindFireOffensive.RevealIsFailClosedOnUnknownVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRevealIsFailClosedOnVersionTest::RunTest(const FString&)
{
	FRTTeamKnowledge Illeggibile = EmptyKnowledge(/*TeamId=*/ 0, /*TurnNumber=*/ 5);
	Illeggibile.Version = FRTTeamKnowledge::CurrentVersion + 1; // una versione che questo codice non conosce

	TArray<FRTLastKnownContact> Vittime;
	Vittime.Add(FRTLastKnownContact(/*StableUnitId=*/ 42, FRTCellId(1, 0, 0), /*ignorato*/ 0));

	const FRTTeamKnowledge Dopo = URTTeamKnowledgeLibrary::RevealByHit(Illeggibile, Vittime, 5);
	TestEqual(TEXT("nessun contatto aggiunto a una memoria che non si sa rileggere"),
		Dopo.Contacts.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
