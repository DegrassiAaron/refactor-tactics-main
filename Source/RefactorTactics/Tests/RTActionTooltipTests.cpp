// Il tooltip delle skill (`#3499`): la frase d'autore nel catalogo, i numeri composti dal gioco, il perche' di uno
// slot spento, e lo slot che lo porta.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDescriptions.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTScreenHudWidgets.h"
#include "Unit/RTUnit.h"
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	ARTUnit* SpawnTooltipUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
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

	/** Le azioni del roster v0.1, come le fabbriche le consegnano: le generiche e il kit di ogni eroe. */
	TArray<const URTActionData*> TooltipRosterActions()
	{
		TArray<const URTActionData*> Out;
		for (const URTActionData* A : URTCatalogLibrary::MakeGenericActions(GetTransientPackage()))
		{
			Out.Add(A);
		}
		for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			for (const URTActionData* A : Hero ? Hero->Actions : TArray<TObjectPtr<URTActionData>>())
			{
				Out.Add(A);
			}
		}
		return Out;
	}

	/** Il valore della riga con quell'etichetta, o vuoto se la riga non c'e'. */
	FString TooltipValue(const FRTActionTooltipView& T, const TCHAR* Label)
	{
		for (const FRTActionTooltipLine& L : T.Lines)
		{
			if (L.Label.ToString() == Label)
			{
				return L.Value.ToString();
			}
		}
		return FString();
	}

	/** Una riga di slot costruita a mano: i campi che il tooltip legge, e nient'altro. */
	FRTAbilityCooldownView TooltipRow(const TCHAR* Id, ERTActionSlot Slot, int32 Range, int32 Cooldown, int32 Damage)
	{
		FRTAbilityCooldownView V;
		V.ActionId = Id;
		V.DisplayName = FText::FromString(Id);
		V.Description = FText::FromString(TEXT("Una frase."));
		V.Slot = Slot;
		V.PhaseMark = ERTActionPhaseMark::Blast;
		V.PhaseLabel = FText::FromString(TEXT("BLAST"));
		V.RangeCells = Range;
		V.CooldownTurns = Cooldown;
		V.Damage = Damage;
		V.bUsableNow = true;
		return V;
	}
}

/**
 * OGNI AZIONE DEL ROSTER HA UNA FRASE, E OGNI FRASE HA UN'AZIONE - `#3499`, DoD 1.
 *
 * 🔑 **I due versi.** Senza il primo un'azione nuova arriva al giocatore con i soli numeri; senza il secondo una
 * frase resta nella tabella dopo che la sua azione e' stata rinominata, e il documento la ripete come se valesse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEveryRosterActionHasADescriptionTest,
	"RefactorTactics.Catalog.EveryRosterActionHasADescription",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEveryRosterActionHasADescriptionTest::RunTest(const FString&)
{
	const TArray<const URTActionData*> Roster = TooltipRosterActions();
	if (!TestTrue(TEXT("premessa: il roster ha delle azioni"), Roster.Num() > 0))
	{
		return false;
	}

	TSet<FName> Ids;
	for (const URTActionData* A : Roster)
	{
		if (!TestNotNull(TEXT("un'azione del roster esiste"), A)) { continue; }
		Ids.Add(A->Def.ActionId);
		TestFalse(*FString::Printf(TEXT("%s ha una frase"), *A->Def.ActionId.ToString()),
			A->Description.IsEmptyOrWhitespace());
		TestEqual(*FString::Printf(TEXT("%s porta la frase della tabella"), *A->Def.ActionId.ToString()),
			A->Description.ToString(), RTActionDescriptions::For(A->Def.ActionId).ToString());
	}

	TSet<FName> Viste;
	for (const TPair<FName, FString>& Voce : RTActionDescriptions::All())
	{
		TestTrue(*FString::Printf(TEXT("la frase di %s appartiene a un'azione del roster"), *Voce.Key.ToString()),
			Ids.Contains(Voce.Key));
		TestFalse(*FString::Printf(TEXT("%s compare una volta sola"), *Voce.Key.ToString()), Viste.Contains(Voce.Key));
		Viste.Add(Voce.Key);
	}
	return true;
}

/**
 * LE RIGHE DEL TOOLTIP: FASE, SLOT, PORTATA, RICARICA, DANNO - `#3499`, decisione d'autore del 2026-10-05.
 *
 * ⚠️ Una riga senza valore non si scrive: un'azione senza danno non dice «Danno 0».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTooltipRowsTest,
	"RefactorTactics.HudViewModel.TooltipComposesTheCatalogRows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTooltipRowsTest::RunTest(const FString&)
{
	const FRTActionTooltipView Colpo = URTHudViewModel::BuildActionTooltip(
		TooltipRow(TEXT("Prova.Colpo"), ERTActionSlot::Main, 4, 2, 22), /*bArmed=*/ false);
	TestEqual(TEXT("il titolo e' il nome"), Colpo.Title.ToString(), FString(TEXT("Prova.Colpo")));
	TestEqual(TEXT("la frase"), Colpo.Description.ToString(), FString(TEXT("Una frase.")));
	TestEqual(TEXT("fase"), TooltipValue(Colpo, TEXT("Fase")), FString(TEXT("BLAST")));
	TestEqual(TEXT("slot principale"), TooltipValue(Colpo, TEXT("Slot")), FString(TEXT("Principale")));
	TestEqual(TEXT("portata in celle"), TooltipValue(Colpo, TEXT("Portata")), FString(TEXT("4 celle")));
	TestEqual(TEXT("ricarica in turni"), TooltipValue(Colpo, TEXT("Ricarica")), FString(TEXT("2 turni")));
	TestEqual(TEXT("danno"), TooltipValue(Colpo, TEXT("Danno")), FString(TEXT("22")));
	TestTrue(TEXT("niente da spiegare"), Colpo.Reason.IsEmpty());

	FRTAbilityCooldownView SuDiSe = TooltipRow(TEXT("Prova.Guardia"), ERTActionSlot::Main, 0, 0, 0);
	SuDiSe.bSelfTarget = true;
	const FRTActionTooltipView Guardia = URTHudViewModel::BuildActionTooltip(SuDiSe, false);
	TestEqual(TEXT("su di se'"), TooltipValue(Guardia, TEXT("Portata")), FString(TEXT("Su di te")));
	TestEqual(TEXT("nessuna ricarica"), TooltipValue(Guardia, TEXT("Ricarica")), FString(TEXT("Nessuna")));
	TestEqual(TEXT("senza danno, la riga non c'e'"), TooltipValue(Guardia, TEXT("Danno")), FString());

	const FRTActionTooltipView Uno = URTHudViewModel::BuildActionTooltip(
		TooltipRow(TEXT("Prova.Uno"), ERTActionSlot::Reaction, 1, 1, 0), false);
	TestEqual(TEXT("una cella"), TooltipValue(Uno, TEXT("Portata")), FString(TEXT("1 cella")));
	TestEqual(TEXT("un turno"), TooltipValue(Uno, TEXT("Ricarica")), FString(TEXT("1 turno")));
	TestEqual(TEXT("slot reazione"), TooltipValue(Uno, TEXT("Slot")), FString(TEXT("Reazione")));

	FRTAbilityCooldownView Ferma = TooltipRow(TEXT("Prova.Attesa"), ERTActionSlot::None, 0, 0, 0);
	Ferma.PhaseMark = ERTActionPhaseMark::None;
	Ferma.PhaseLabel = FText::FromString(TEXT("—"));
	Ferma.Description = FText::GetEmpty();
	const FRTActionTooltipView Attesa = URTHudViewModel::BuildActionTooltip(Ferma, false);
	TestEqual(TEXT("senza fase, la riga non c'e'"), TooltipValue(Attesa, TEXT("Fase")), FString());
	TestEqual(TEXT("senza slot, la riga non c'e'"), TooltipValue(Attesa, TEXT("Slot")), FString());
	TestEqual(TEXT("senza portata, la riga non c'e'"), TooltipValue(Attesa, TEXT("Portata")), FString());
	TestTrue(TEXT("un'azione senza frase ha i soli numeri"), Attesa.Description.IsEmpty() && Attesa.IsValid());

	TestFalse(TEXT("una posizione vuota non ha tooltip"),
		URTHudViewModel::BuildActionTooltip(FRTAbilityCooldownView(), false).IsValid());
	return true;
}

/**
 * IL PERCHE' DI UNO SLOT SPENTO, UNO SOLO E IN ORDINE - `#3499`, D005.
 *
 * ⚠️ **Il rifiuto sotto il puntatore non e' un motivo del tooltip**: mentre il cursore sta sulla barra non punta il
 * campo. ⛔ E un rifiuto `Nothing` — il bersaglio ignoto — non ha testo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTooltipReasonTest,
	"RefactorTactics.HudViewModel.TooltipNamesWhyTheSlotIsOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTooltipReasonTest::RunTest(const FString&)
{
	FRTAbilityCooldownView Ricarica = TooltipRow(TEXT("Prova.Ricarica"), ERTActionSlot::Main, 4, 3, 10);
	Ricarica.TurnsRemaining = 2;
	TestEqual(TEXT("in ricarica, i turni che restano"),
		URTHudViewModel::BuildActionTooltip(Ricarica, false).Reason.ToString(), FString(TEXT("In ricarica: ancora 2 turni")));
	Ricarica.TurnsRemaining = 1;
	TestEqual(TEXT("un turno"),
		URTHudViewModel::BuildActionTooltip(Ricarica, false).Reason.ToString(), FString(TEXT("In ricarica: ancora 1 turno")));

	FRTAbilityCooldownView Illegale = Ricarica;
	Illegale.bPlanInvalid = true;
	Illegale.PlanInvalidReason = ERTActionInvalidReason::SlotOccupied;
	const FRTActionTooltipView Colpevole = URTHudViewModel::BuildActionTooltip(Illegale, false);
	TestEqual(TEXT("il piano illegale vince sulla ricarica, col motivo del validatore"), Colpevole.Reason.ToString(),
		FString(TEXT("Piano illegale: lo slot e' gia' occupato")));
	TestEqual(TEXT("e lo stato e' Invalid"), Colpevole.State, ERTActionSlotState::Invalid);
	Illegale.PlanInvalidReason = ERTActionInvalidReason::None;
	TestEqual(TEXT("senza motivo, la sola parola"),
		URTHudViewModel::BuildActionTooltip(Illegale, false).Reason.ToString(), FString(TEXT("Piano illegale")));

	FRTAbilityCooldownView Degradata = TooltipRow(TEXT("Prova.Degradata"), ERTActionSlot::Main, 4, 0, 10);
	Degradata.bPlanned = true;
	Degradata.bPlanDegraded = true;
	Degradata.PlanDegradedRefusal = ERTTargetRefusal::Range;
	Degradata.PlanDegradedRange = 3;
	const FString Lontano = URTHudViewModel::BuildActionTooltip(Degradata, false).Reason.ToString();
	TestTrue(TEXT("il bersaglio degradato dice il rifiuto, con la portata applicata"),
		Lontano.Contains(TEXT("portata 3")) && Lontano.Contains(TEXT("ripiego")));
	Degradata.PlanDegradedRefusal = ERTTargetRefusal::Cover;
	TestTrue(TEXT("e la copertura"),
		URTHudViewModel::BuildActionTooltip(Degradata, false).Reason.ToString().StartsWith(TEXT("Coperto")));
	Degradata.PlanDegradedRefusal = ERTTargetRefusal::Nothing;
	TestTrue(TEXT("⛔ un bersaglio ignoto non ha testo"),
		URTHudViewModel::BuildActionTooltip(Degradata, false).Reason.IsEmpty());

	FRTAbilityCooldownView Puntata = TooltipRow(TEXT("Prova.Puntata"), ERTActionSlot::Main, 4, 0, 10);
	Puntata.bTargetRefused = true;
	const FRTActionTooltipView Armata = URTHudViewModel::BuildActionTooltip(Puntata, /*bArmed=*/ true);
	TestEqual(TEXT("premessa: armata e rifiutata sotto il puntatore, lo slot e' Invalid"), Armata.State,
		ERTActionSlotState::Invalid);
	TestTrue(TEXT("ma il tooltip non ne dice il motivo: il cursore sta sulla barra"), Armata.Reason.IsEmpty());
	return true;
}

/**
 * IL TOOLTIP LEGGE IL CATALOGO DELL'UNITA' - `#3499`: la riga che lo slot riceve porta la frase e i numeri.
 *
 * 🔑 L'oracolo e' l'azione stessa, non una costante: i numeri cambiano col bilanciamento, e un test che li
 * scrivesse a mano diventerebbe rosso a ogni ritocco senza dire niente del tooltip.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTooltipReadsTheCatalogTest,
	"RefactorTactics.HudViewModel.TooltipReadsTheUnitsCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTooltipReadsTheCatalogTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	ARTUnit* Unit = SpawnTooltipUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0, 0));
	if (!TestNotNull(TEXT("unita'"), Unit)) { RTWorldFixtures::DestroyWorld(World); return false; }

	int32 ConDanno = 0;
	for (const FRTAbilityCooldownView& Riga : URTHudViewModel::BuildAbilityCooldowns(Unit))
	{
		const URTActionData* A = Unit->GetAbility(Riga.AbilityIndex);
		if (!A) { continue; }
		const FString Id = A->Def.ActionId.ToString();
		int32 Danno = 0;
		for (const FRTActionEffectSpec& E : A->Def.Effects)
		{
			if (E.Effect == ERTActionEffect::Damage) { Danno = E.Amount; break; }
		}
		ConDanno += Danno > 0 ? 1 : 0;
		TestEqual(*FString::Printf(TEXT("%s: la frase"), *Id), Riga.Description.ToString(), A->Description.ToString());
		TestEqual(*FString::Printf(TEXT("%s: la portata che misura il click"), *Id), Riga.RangeCells, A->RangeCells);
		TestEqual(*FString::Printf(TEXT("%s: la ricarica"), *Id), Riga.CooldownTurns, A->Def.CooldownTurns);
		TestEqual(*FString::Printf(TEXT("%s: il danno"), *Id), Riga.Damage, Danno);
		TestEqual(*FString::Printf(TEXT("%s: il tooltip porta la frase"), *Id),
			URTHudViewModel::BuildActionTooltip(Riga, false).Description.ToString(), A->Description.ToString());
	}
	TestTrue(TEXT("premessa: almeno un'azione fa danno, quindi la riga del danno e' messa alla prova"), ConDanno > 0);

	// 🔑 La portata e' quella che misura il click — lo specchio — e non il `Def`. Nel roster i due coincidono, quindi
	// un'azione li separa qui: senza, leggere il campo sbagliato resterebbe verde.
	if (URTActionData* Separata = const_cast<URTActionData*>(Unit->GetAbility(0)))
	{
		Separata->Def.RangeCells = Separata->RangeCells + 2;
		const TArray<FRTAbilityCooldownView> Righe = URTHudViewModel::BuildAbilityCooldowns(Unit);
		TestEqual(TEXT("con i due campi separati, la portata e' quella del click"),
			Righe.IsValidIndex(0) ? Righe[0].RangeCells : INDEX_NONE, Separata->RangeCells);
	}

	// La barra senza unita' ([D-460]) copia i fatti di catalogo, e niente del piano.
	const TArray<FRTAbilityCooldownView> Unita = URTHudViewModel::BuildAbilityCooldowns(Unit);
	int32 Comuni = 0;
	for (const FRTAbilityCooldownView& Spenta : URTHudViewModel::BuildIdleBar({ Unit }))
	{
		const FRTAbilityCooldownView* Fonte = Unita.FindByPredicate(
			[&Spenta](const FRTAbilityCooldownView& R) { return !Spenta.ActionId.IsNone() && R.ActionId == Spenta.ActionId; });
		if (!Fonte) { continue; }
		++Comuni;
		const FString Id = Spenta.ActionId.ToString();
		TestEqual(*FString::Printf(TEXT("senza unita', %s: la frase"), *Id), Spenta.Description.ToString(),
			Fonte->Description.ToString());
		TestEqual(*FString::Printf(TEXT("senza unita', %s: la ricarica di catalogo"), *Id), Spenta.CooldownTurns,
			Fonte->CooldownTurns);
		TestEqual(*FString::Printf(TEXT("senza unita', %s: la portata"), *Id), Spenta.RangeCells, Fonte->RangeCells);
	}
	TestTrue(TEXT("premessa: la barra senza unita' ha delle comuni"), Comuni > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * LO SLOT PORTA IL SUO TOOLTIP - `#3499`: in testo semplice senza una classe, nel widget con la classe.
 *
 * 🔑 Il testo semplice e' cio' che il giocatore vede prima della seduta che disegna il tooltip: il dato arriva prima
 * del vestito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotCarriesItsTooltipTest,
	"RefactorTactics.ScreenHud.ActionSlotCarriesItsTooltip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionSlotCarriesItsTooltipTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	const FRTAbilityCooldownView Riga = TooltipRow(TEXT("Prova.Colpo"), ERTActionSlot::Main, 4, 2, 22);

	URTActionSlotWidget* Testo = NewObject<URTActionSlotWidget>(World);
	Testo->SetAction(Riga, /*bInArmed=*/ false);
	TestEqual(TEXT("senza classe, il tooltip e' il testo semplice"), Testo->GetToolTipText().ToString(),
		URTHudViewModel::ComposeTooltipText(URTHudViewModel::BuildActionTooltip(Riga, false)).ToString());
	TestTrue(TEXT("e il testo dice il nome e i numeri"),
		Testo->GetToolTipText().ToString().Contains(TEXT("Prova.Colpo"))
		&& Testo->GetToolTipText().ToString().Contains(TEXT("Portata  4 celle")));
	TestNull(TEXT("e nessun widget"), Testo->GetToolTip());

	URTActionSlotWidget* Vestito = NewObject<URTActionSlotWidget>(World);
	Vestito->TooltipClass = URTActionTooltipWidget::StaticClass();
	Vestito->SetAction(Riga, false);
	const URTActionTooltipWidget* Tooltip = Cast<URTActionTooltipWidget>(Vestito->GetToolTip());
	if (TestNotNull(TEXT("con la classe, il tooltip e' il widget"), Tooltip))
	{
		TestEqual(TEXT("e porta la vista dello slot"), Tooltip->GetView().Title.ToString(), FString(TEXT("Prova.Colpo")));
		TestEqual(TEXT("con le righe"), Tooltip->GetLinesText().ToString().Contains(TEXT("Danno  22")), true);
	}

	Vestito->SetAction(FRTAbilityCooldownView(), false);
	TestNull(TEXT("una posizione vuota toglie il tooltip"), Vestito->GetToolTip());
	TestTrue(TEXT("anche il testo"), Vestito->GetToolTipText().IsEmpty());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
