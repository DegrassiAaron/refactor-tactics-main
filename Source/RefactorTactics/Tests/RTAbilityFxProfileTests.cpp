// IL PROFILO FX PER ABILITA' (#3578, spec «il profilo FX per abilita'» §2.1-§2.4, §5.1).
//
// 🔑 **Funzioni pure**: nessun mondo, nessun Actor. La tabella vera si legge solo dove il test giudica la tabella
// vera (`DeclaredOverridesMatchTheProposal`); il ripiego si prova su una tabella INIETTATA (`FxProfileForIn`), come
// R11 della spec della clip: un asserto che regge su una riga giudicata cade quando l'autore cambia idea.
// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

#include "Misc/AutomationTest.h"
#include "Map/RTPlaybackTracer.h"
#include "Turn/RTPresentationBinding.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTResolvedEvent.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Perception/RTTeamKnowledge.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	using FxA = ERTActivationFxStyle;
	using FxT = ERTTracerStyle;
	using FxI = ERTImpactFxStyle;
	using FxF = ERTFootprintFxStyle;

	FRTAbilityFxProfile FxP(FxA A, FxT T, FxI I, FxF F)
	{
		return URTPresentationBindingLibrary::MakeFxProfile(A, T, I, F);
	}

	/** Il profilo come testo: un `TestEqual` su due stringhe dice QUALE campo e' diverso. */
	FString FxTesto(const FRTAbilityFxProfile& P)
	{
		return FString::Printf(TEXT("%s · %s · %s · %s"),
			*UEnum::GetValueAsString(P.Activation), *UEnum::GetValueAsString(P.Tracer),
			*UEnum::GetValueAsString(P.Impact), *UEnum::GetValueAsString(P.Footprint));
	}

	FRTAbilityFxProfile FxNessuno() { return FxP(FxA::None, FxT::None, FxI::None, FxF::None); }
}

/**
 * Il default di ogni forma e' la riga di spec §2.2 (D3, grammatica di #2454).
 * ✅ Validato per mutazione (1): la riga `Line` con `Projectile` fa cadere «Line → Jet».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxDefaultProfileFollowsShapeTest,
	"RefactorTactics.Fx.DefaultProfileFollowsShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxDefaultProfileFollowsShapeTest::RunTest(const FString&)
{
	const UEnum* Forme = StaticEnum<ERTAbilityShape>();
	if (!TestNotNull(TEXT("premessa: reflection di ERTAbilityShape"), Forme)) { return false; }
	// ⛔ Una quinta forma vuole una riga in `DefaultFxProfileFor` e una qui: questo asserto lo ricorda.
	TestEqual(TEXT("⛔ premessa: le forme sono quelle della tabella di §2.2"), Forme->NumEnums() - 1, 4);

	TestEqual(TEXT("Single"), FxTesto(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Single)),
		FxTesto(FxP(FxA::Ring, FxT::Projectile, FxI::Marker, FxF::None)));
	TestEqual(TEXT("🔴 Line → Jet"), FxTesto(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Line)),
		FxTesto(FxP(FxA::Ring, FxT::Jet, FxI::Marker, FxF::None)));
	TestEqual(TEXT("Area: nessun tracer, AreaPulse"), FxTesto(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Area)),
		FxTesto(FxP(FxA::Ring, FxT::None, FxI::Marker, FxF::AreaPulse)));
	TestEqual(TEXT("Cone: nessun tracer, ConeSweep"), FxTesto(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Cone)),
		FxTesto(FxP(FxA::Ring, FxT::None, FxI::Marker, FxF::ConeSweep)));
	return true;
}

/**
 * Il ripiego `ActionId` → `BaseActionId` → forma, su una tabella INIETTATA (spec §2.2, R4).
 * ✅ Validato per mutazioni (2) — la forma letta prima — e (3) — il livello `BaseActionId` saltato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxProfileFallsBackTest,
	"RefactorTactics.Fx.ProfileFallsBackActionThenBaseThenShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxProfileFallsBackTest::RunTest(const FString&)
{
	TMap<FName, FRTAbilityFxProfile> Prova;
	Prova.Add(FName(TEXT("Hero.Prova.Colpo")), FxP(FxA::Flash, FxT::None, FxI::None, FxF::None));
	Prova.Add(FName(TEXT("Action.Prova")), FxP(FxA::Pulse, FxT::None, FxI::None, FxF::None));
	const FRTAbilityFxProfile Forma = URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Single);

	TestEqual(TEXT("🔴 ActionId vince"), FxTesto(URTPresentationBindingLibrary::FxProfileForIn(Prova,
		TEXT("Hero.Prova.Colpo"), TEXT("Action.Prova"), ERTAbilityShape::Single)), FxTesto(Prova[TEXT("Hero.Prova.Colpo")]));
	TestEqual(TEXT("🔴 senza ActionId in tabella, vince BaseActionId"), FxTesto(URTPresentationBindingLibrary::FxProfileForIn(Prova,
		TEXT("Hero.Prova.Altro"), TEXT("Action.Prova"), ERTAbilityShape::Single)), FxTesto(Prova[TEXT("Action.Prova")]));
	TestEqual(TEXT("ActionId vuoto: il livello si salta, vince BaseActionId"), FxTesto(URTPresentationBindingLibrary::FxProfileForIn(Prova,
		NAME_None, TEXT("Action.Prova"), ERTAbilityShape::Single)), FxTesto(Prova[TEXT("Action.Prova")]));
	TestEqual(TEXT("nessuno dei due in tabella: la forma"), FxTesto(URTPresentationBindingLibrary::FxProfileForIn(Prova,
		TEXT("Hero.Prova.Altro"), TEXT("Action.Altra"), ERTAbilityShape::Single)), FxTesto(Forma));
	// 🔑 `BaseActionId` si legge dall'evento e non si indovina (`RTResolvedEvent.h:393-396`): `Action.Prova` e' in
	// tabella, ma l'evento non la dichiara.
	TestEqual(TEXT("BaseActionId = NAME_None non indovina la generica"), FxTesto(URTPresentationBindingLibrary::FxProfileForIn(Prova,
		TEXT("Hero.Prova.Altro"), NAME_None, ERTAbilityShape::Single)), FxTesto(Forma));
	// R4: il profilo e' intero per livello, nessuna fusione per campo con la forma.
	TestTrue(TEXT("nessuna fusione: la riga senza tracer resta senza tracer anche su Line"),
		URTPresentationBindingLibrary::FxProfileForIn(Prova, TEXT("Hero.Prova.Colpo"), NAME_None, ERTAbilityShape::Line).Tracer
			== ERTTracerStyle::None);
	return true;
}

/**
 * Review Focus (e): `ActionId` senza riga, `BaseActionId` con riga → il profilo della generica, sulla tabella VERA.
 * Il caso reale e' `Action.Charge` (spec §2.2, riga fuori roster). ✅ Validato per mutazione (3).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxBaseActionOverrideWinsTest,
	"RefactorTactics.Fx.BaseActionOverrideWinsOverShapeDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxBaseActionOverrideWinsTest::RunTest(const FString&)
{
	const FRTAbilityFxProfile Forma = URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Single);
	if (!TestTrue(TEXT("⛔ premessa: il default di Single ha un proiettile, quindi la differenza e' misurabile"),
			Forma.Tracer == ERTTracerStyle::Projectile))
	{
		return false;
	}
	TestEqual(TEXT("🔴 una carica senza riga propria, con BaseActionId = Action.Charge: il profilo della generica"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(TEXT("Hero.Prova.Carica"), TEXT("Action.Charge"), ERTAbilityShape::Single)),
		FxTesto(FxP(FxA::Ring, FxT::None, FxI::Marker, FxF::None)));
	TestEqual(TEXT("controllo: senza BaseActionId, il default della forma"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(TEXT("Hero.Prova.Carica"), NAME_None, ERTAbilityShape::Single)),
		FxTesto(Forma));
	return true;
}

/**
 * R12 — nessuna azione, nessun profilo (spec §2.2, F8). Gli attacchi legacy di `ARTUnit::MakeAbility`
 * (`Unit/RTUnit.cpp:1606-1615`) non scrivono `ActionId`.
 * ✅ Validato per mutazione (18): la guardia tolta da `FxProfileForIn` fa cadere «profilo tutto None».
 * ➕ piano. Il Task 2 aggiunge a QUESTO test l'asserto su `IsTracerEligible` (mutazione (22)): quella funzione ha
 * la propria clausola R12 e cambia nel Task 2.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxNoActionNoProfileTest,
	"RefactorTactics.Fx.NoActionNoProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxNoActionNoProfileTest::RunTest(const FString&)
{
	TestEqual(TEXT("🔴 legacy Single: profilo tutto None"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(NAME_None, NAME_None, ERTAbilityShape::Single)), FxTesto(FxNessuno()));
	TestEqual(TEXT("🔴 legacy Area: profilo tutto None"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(NAME_None, NAME_None, ERTAbilityShape::Area)), FxTesto(FxNessuno()));
	TestEqual(TEXT("controllo: con un solo id, la forma torna"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(NAME_None, TEXT("Action.BasicAttack"), ERTAbilityShape::Single)),
		FxTesto(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Single)));

	// 🔴 La clausola R12 PROPRIA di `IsTracerEligible`: legge `DefaultFxProfileFor`, non `FxProfileFor`, quindi la
	// guardia di `FxProfileForIn` non la copre (mutazione (22)).
	FRTResolvedEvent Legacy;
	Legacy.Type = ERTResolvedEventType::Attack;
	Legacy.Shape = ERTAbilityShape::Single;
	Legacy.HitGeometry.bResolved = true;
	TestFalse(TEXT("🔴 IsTracerEligible falso anche con geometria risolta"), URTPlaybackLibrary::IsTracerEligible(Legacy));
	TestEqual(TEXT("quindi nessun volo: il ritmo dei test con unita' legacy non cambia"),
		URTPlaybackLibrary::TracerFlightFor(URTPlaybackLibrary::IsTracerEligible(Legacy), 0.25f, 0.5f), 0.f);
	FRTResolvedEvent ConId = Legacy;
	ConId.ActionId = TEXT("Action.BasicAttack");
	TestTrue(TEXT("controllo: con un id la stessa geometria e' idonea"), URTPlaybackLibrary::IsTracerEligible(ConId));
	return true;
}

/**
 * La mappa degli override APPROVATA dall'autore (spec §2.2, D5), ricopiata qui come seconda copia DICHIARATA.
 * La lista delle abilita' e' una FUNZIONE del catalogo (`GetHeroRoster`): un'abilita' nuova senza riga prende il
 * default della sua forma, e questo test lo verifica senza essere toccato.
 *
 * 🔴 Ogni riga si cambia con un commit di DUE righe: quella di `DeclaredFxOverrideRows` e la sua gemella qui.
 * ✅ Validato per mutazione (4): la riga `TideGuard` tolta dalla tabella vera fa cadere la sua abilita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxDeclaredOverridesMatchTheProposalTest,
	"RefactorTactics.Fx.DeclaredOverridesMatchTheProposal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxDeclaredOverridesMatchTheProposalTest::RunTest(const FString&)
{
	TMap<FName, FRTAbilityFxProfile> Attesi;
	Attesi.Add(TEXT("Hero.Aevik.LinearDischarge"), FxP(FxA::Ring,  FxT::Zigzag, FxI::Marker, FxF::None));
	Attesi.Add(TEXT("Hero.Aevik.Overload"),        FxP(FxA::Flash, FxT::None,   FxI::Marker, FxF::AreaPulse));
	Attesi.Add(TEXT("Hero.Muiren.CircularTide"),   FxP(FxA::Pulse, FxT::None,   FxI::Marker, FxF::AreaPulse));
	Attesi.Add(TEXT("Hero.Muiren.TideGuard"),      FxP(FxA::Pulse, FxT::None,   FxI::None,   FxF::None));
	Attesi.Add(TEXT("Hero.Branth.Ram"),            FxP(FxA::Ring,  FxT::None,   FxI::Marker, FxF::None));
	Attesi.Add(TEXT("Hero.Ivrin.InterceptShot"),   FxP(FxA::Flash, FxT::None,   FxI::None,   FxF::None));
	Attesi.Add(TEXT("Hero.Ivrin.PassingBlade"),    FxP(FxA::Ring,  FxT::None,   FxI::Marker, FxF::None));
	Attesi.Add(TEXT("Hero.Ivrin.Feint"),           FxP(FxA::Flash, FxT::None,   FxI::None,   FxF::None));
	Attesi.Add(TEXT("Hero.Ivrin.PhaseGuard"),      FxP(FxA::Pulse, FxT::None,   FxI::None,   FxF::None));
	Attesi.Add(TEXT("Action.Charge"),              FxP(FxA::Ring,  FxT::None,   FxI::Marker, FxF::None));
	Attesi.Add(TEXT("Action.Push"),                FxP(FxA::Ring,  FxT::None,   FxI::Marker, FxF::None));
	Attesi.Add(TEXT("Action.Root"),                FxP(FxA::Ring,  FxT::None,   FxI::Marker, FxF::None));
	Attesi.Add(TEXT("Action.Slow"),                FxP(FxA::Ring,  FxT::None,   FxI::Marker, FxF::None));
	Attesi.Add(TEXT("Action.Interrupt"),           FxP(FxA::Ring,  FxT::None,   FxI::Marker, FxF::None));

	TSet<FName> NelRoster;
	int32 Viste = 0;
	for (const URTHeroData* Eroe : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Eroe) { continue; }
		for (const URTActionData* Azione : Eroe->Actions)
		{
			if (!Azione || Azione->Def.ActionId.IsNone()) { continue; }
			const FName Id = Azione->Def.ActionId;
			NelRoster.Add(Id);
			++Viste;
			const FRTAbilityFxProfile* Riga = Attesi.Find(Id);
			const FRTAbilityFxProfile Atteso = Riga ? *Riga : URTPresentationBindingLibrary::DefaultFxProfileFor(Azione->Shape);
			TestEqual(FString::Printf(TEXT("%s: il profilo e' quello approvato"), *Id.ToString()),
				FxTesto(URTPresentationBindingLibrary::FxProfileFor(Id, Azione->Def.BaseActionId, Azione->Shape)), FxTesto(Atteso));
			if (Riga)
			{
				// R13: un override non AGGIUNGE un tracer a una forma che non ne ha (non avrebbe volo).
				TestTrue(FString::Printf(TEXT("⛔ %s: nessun tracer su una forma senza volo"), *Id.ToString()),
					Riga->Tracer == ERTTracerStyle::None
					|| URTPresentationBindingLibrary::DefaultFxProfileFor(Azione->Shape).Tracer != ERTTracerStyle::None);
			}
		}
	}
	TestTrue(TEXT("⛔ premessa: il roster dichiara azioni"), Viste > 0);

	// #3578 (review del Task 2, Ruling R15): anche le azioni CORE che contano come attacco (`bCountsAsAttack`) — lista dal
	// catalogo, non letterale. `FRTActionDef` non porta una forma: l'azione core vive in un `URTActionData` di forma `Single`.
	// 🔴 L'ATTESO del tracer e' una FUNZIONE del catalogo: a contatto (`RangeCells == 1`) nessun proiettile; altrimenti il
	// default della forma, salvo una riga approvata dall'autore (`Action.Charge`, D5). `RangeCells <= 0` e' la portata del
	// portatore, non il contatto (`FRTActionDef::RangeCells`, punto 2).
	int32 ColpiCore = 0, Contatto = 0;
	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		if (!Def.bCountsAsAttack || Def.ActionId.IsNone()) { continue; }
		++ColpiCore;
		const bool bAContatto = Def.RangeCells == 1;
		Contatto += bAContatto ? 1 : 0;
		AddInfo(FString::Printf(TEXT("colpo core %s: RangeCells=%d%s"), *Def.ActionId.ToString(), Def.RangeCells,
			bAContatto ? TEXT(" (a contatto)") : TEXT("")));
		const FRTAbilityFxProfile Vero = URTPresentationBindingLibrary::FxProfileFor(Def.ActionId, Def.BaseActionId, ERTAbilityShape::Single);
		const FRTAbilityFxProfile* Riga = Attesi.Find(Def.ActionId);
		if (bAContatto)
		{
			TestTrue(FString::Printf(TEXT("🔴 %s (core, a contatto): ha la riga gemella"), *Def.ActionId.ToString()), Riga != nullptr);
			TestTrue(FString::Printf(TEXT("🔴 %s (core, a contatto): nessun proiettile"), *Def.ActionId.ToString()),
				Vero.Tracer == ERTTracerStyle::None);
		}
		else if (!Riga)
		{
			TestEqual(FString::Printf(TEXT("%s (core, a distanza): il tracer e' il default della forma"), *Def.ActionId.ToString()),
				static_cast<int32>(Vero.Tracer),
				static_cast<int32>(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Single).Tracer));
		}
		const FRTAbilityFxProfile Atteso = Riga ? *Riga : URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Single);
		TestEqual(FString::Printf(TEXT("%s (core): il profilo e' quello approvato"), *Def.ActionId.ToString()),
			FxTesto(Vero), FxTesto(Atteso));
	}
	TestTrue(TEXT("⛔ premessa: il catalogo core dichiara colpi"), ColpiCore > 0);
	TestTrue(TEXT("⛔ premessa: il catalogo core dichiara colpi a contatto"), Contatto > 0);

	// Il verso opposto: ogni riga VERA ha la gemella qui, e ogni riga `Hero.*` e' un'abilita' del roster.
	for (const TPair<FName, FRTAbilityFxProfile>& Riga : URTPresentationBindingLibrary::DeclaredFxOverrideRows())
	{
		TestTrue(FString::Printf(TEXT("%s: la riga vera ha la gemella nel test"), *Riga.Key.ToString()), Attesi.Contains(Riga.Key));
		if (Riga.Key.ToString().StartsWith(TEXT("Hero.")))
		{
			TestTrue(FString::Printf(TEXT("%s: e' un'abilita' del roster"), *Riga.Key.ToString()), NelRoster.Contains(Riga.Key));
		}
		else
		{
			TestTrue(FString::Printf(TEXT("%s: fuori roster, nessun tracer (la forma non si conosce qui)"), *Riga.Key.ToString()),
				Riga.Value.Tracer == ERTTracerStyle::None);
		}
	}
	TestEqual(TEXT("Action.Charge, fuori roster"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(TEXT("Action.Charge"), NAME_None, ERTAbilityShape::Single)),
		FxTesto(Attesi[TEXT("Action.Charge")]));
	return true;
}

/**
 * Nessuna chiave duplicata fra le righe (spec §5.1, F17): in una mappa costruita dalle righe una duplicata vincerebbe
 * in silenzio. ✅ Validato per mutazione (P1): una riga ripetuta fa cadere il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxDeclaredOverridesHaveNoDuplicateKeysTest,
	"RefactorTactics.Fx.DeclaredOverridesHaveNoDuplicateKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxDeclaredOverridesHaveNoDuplicateKeysTest::RunTest(const FString&)
{
	TSet<FName> Viste;
	for (const TPair<FName, FRTAbilityFxProfile>& Riga : URTPresentationBindingLibrary::DeclaredFxOverrideRows())
	{
		TestFalse(FString::Printf(TEXT("🔴 %s compare una volta sola"), *Riga.Key.ToString()), Viste.Contains(Riga.Key));
		Viste.Add(Riga.Key);
	}
	TestEqual(TEXT("la mappa ha una chiave per riga"),
		URTPresentationBindingLibrary::DeclaredFxOverrides().Num(), URTPresentationBindingLibrary::DeclaredFxOverrideRows().Num());
	return true;
}


// --- La cue d'attivazione (#3578, spec §2.3) -------------------------------------------------------------------

namespace
{
	/** Un'attivazione visibile alla squadra 0, con la sorgente in `Origine`. */
	FRTResolvedEvent FxAttivazione(const TCHAR* Azione, ERTAbilityShape Forma, const FRTCellId& Origine)
	{
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Prep;
		Ev.Type = ERTResolvedEventType::AbilityActivated;
		Ev.SourceStableUnitId = 1;
		Ev.ActionId = Azione;
		Ev.Shape = Forma;
		Ev.Origin = Origine;
		Ev.SourceVerdict.AllowTeam(0);
		return Ev;
	}
}

/**
 * R7: la funzione pura della cue d'attivazione RICONTROLLA `SourceVerdict` (le code sono gia' filtrate a monte, e
 * questa e' la seconda porta). Lo stile e' quello del profilo, la cella e' `Ev.Origin`.
 * ✅ Validato per mutazione (8): il controllo di `SourceVerdict` tolto fa cadere «chi non vede la sorgente».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyActivationCueNeedsTheSourceTest,
	"RefactorTactics.Privacy.ActivationCueNeedsTheSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyActivationCueNeedsTheSourceTest::RunTest(const FString&)
{
	const FRTResolvedEvent Scudo = FxAttivazione(TEXT("Hero.Muiren.TideGuard"), ERTAbilityShape::Single, FRTCellId(2, 1));
	FRTPlaybackCue Cue;
	TestFalse(TEXT("🔴 chi non vede la sorgente non riceve la cue"), URTPlaybackLibrary::ActivationCueFor(Scudo, 1, 0.5f, Cue));
	TestFalse(TEXT("un osservatore fuori intervallo non legge"), URTPlaybackLibrary::ActivationCueFor(Scudo, -1, 0.5f, Cue));
	if (TestTrue(TEXT("chi la vede riceve la cue"), URTPlaybackLibrary::ActivationCueFor(Scudo, 0, 0.5f, Cue)))
	{
		TestTrue(TEXT("TideGuard: Pulse (override)"), Cue.Kind == ERTPlaybackCueKind::Pulse);
		TestTrue(TEXT("sulla cella dell'evento, non dell'attore"), Cue.At == FRTCellId(2, 1));
		TestEqual(TEXT("con l'Alpha data"), Cue.Alpha, 0.5f);
	}
	FRTPlaybackCue Altra;
	TestTrue(TEXT("Overload: Flash"), URTPlaybackLibrary::ActivationCueFor(
		FxAttivazione(TEXT("Hero.Aevik.Overload"), ERTAbilityShape::Area, FRTCellId(0, 0)), 0, 0.f, Altra)
		&& Altra.Kind == ERTPlaybackCueKind::Flash);
	TestTrue(TEXT("un attacco base: Ring (default)"), URTPlaybackLibrary::ActivationCueFor(
		FxAttivazione(TEXT("Hero.Branth.ImpactShot"), ERTAbilityShape::Single, FRTCellId(0, 0)), 0, 0.f, Altra)
		&& Altra.Kind == ERTPlaybackCueKind::Ring);
	FRTResolvedEvent Colpo = Scudo;
	Colpo.Type = ERTResolvedEventType::Attack;
	TestFalse(TEXT("solo un AbilityActivated ha una cue d'attivazione"), URTPlaybackLibrary::ActivationCueFor(Colpo, 0, 0.f, Altra));
	return true;
}

// --- Le cue di colpo (#3578, spec §2.4) ------------------------------------------------------------------------

namespace
{
	FRTResolvedEvent FxImpronta(int32 Sorgente, const TCHAR* Azione, ERTAbilityShape Forma, const FRTCellId& Origine,
		const FRTCellId& Mira)
	{
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = ERTResolvedEventType::AttackFootprint;
		Ev.SourceStableUnitId = Sorgente;
		Ev.ActionId = Azione;
		Ev.Shape = Forma;
		Ev.Origin = Origine;
		Ev.AimCell = Mira;
		return Ev;
	}

	/** Un colpo risolto, visibile alle squadre 0 e 1 su entrambi gli estremi. */
	FRTResolvedEvent FxColpo(int32 Sorgente, const TCHAR* Azione, ERTAbilityShape Forma, const FRTCellId& Da,
		const FRTCellId& Vittima)
	{
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = ERTResolvedEventType::Attack;
		Ev.SourceStableUnitId = Sorgente;
		Ev.ActionId = Azione;
		Ev.Shape = Forma;
		Ev.HitGeometry.bResolved = true;
		Ev.HitGeometry.From = Da;
		Ev.HitGeometry.Impact = Vittima;
		Ev.HitGeometry.FromVerdict.AllowTeam(0);
		Ev.HitGeometry.FromVerdict.AllowTeam(1);
		Ev.HitGeometry.ImpactVerdict.AllowTeam(0);
		Ev.HitGeometry.ImpactVerdict.AllowTeam(1);
		return Ev;
	}

	/** La sequenza «come viene»: un elemento per evento, nell'ordine dato (il test costruisce gia' l'ordine di §2.4). */
	TArray<FRTBlastSequenceElement> FxSequenza(const TArray<FRTResolvedEvent>& T)
	{
		TArray<FRTBlastSequenceElement> S;
		for (int32 I = 0; I < T.Num(); ++I)
		{
			FRTBlastSequenceElement E;
			E.TimelineIndex = I;
			E.SourceStableUnitId = T[I].SourceStableUnitId;
			E.ActionId = T[I].ActionId;
			S.Add(E);
		}
		return S;
	}

	/** I voli come li calcola `BeginPlayback` (`RTTurnManager.cpp`, il ciclo su `PlaybackBlastSequence`). */
	TArray<float> FxVoli(const TArray<FRTResolvedEvent>& T, float A)
	{
		TArray<float> V;
		for (const FRTResolvedEvent& Ev : T)
		{
			V.Add(URTPlaybackLibrary::TracerFlightFor(URTPlaybackLibrary::IsTracerEligible(Ev), 0.25f, A));
		}
		return V;
	}

	/** Le cue del Blast all'istante `t`, per chi guarda: la stessa composizione di `PushPlaybackCues`. */
	TArray<FRTPlaybackCue> FxCueAl(const TArray<FRTResolvedEvent>& T, float t, float A, int32 Viewer = 0)
	{
		const TArray<FRTBlastSequenceElement> S = FxSequenza(T);
		const TArray<float> V = FxVoli(T, A);
		const TArray<int32> Impronte = URTPlaybackLibrary::FootprintFxForSequence(T, S);
		const int32 Battiti = URTPlaybackLibrary::AttackBeatsDue(t, A, V);
		TArray<FRTPlaybackCue> Out;
		URTPlaybackLibrary::BlastActivationCuesAt(T, S, Battiti, t, A, 0.35f, Viewer, Out);
		URTPlaybackLibrary::BlastHitCuesAt(T, S, V, Impronte, Battiti, t, A, 0.20f, Viewer, Out);
		return Out;
	}

	bool FxHa(const TArray<FRTPlaybackCue>& C, ERTPlaybackCueKind Tipo, const FRTCellId& Cella)
	{
		return C.ContainsByPredicate([&](const FRTPlaybackCue& X) { return X.Kind == Tipo && X.At == Cella; });
	}

	/** Quante volte una cue del tipo compare, contando le CORSE di presenza su tutto il Blast (passo 5 ms). */
	int32 FxCorse(const TArray<FRTResolvedEvent>& T, float A, ERTPlaybackCueKind Tipo)
	{
		int32 Corse = 0;
		bool bPrima = false;
		for (float t = 0.f; t <= T.Num() * A + 0.01f; t += 0.005f)
		{
			const bool bOra = FxCueAl(T, t, A).ContainsByPredicate([&](const FRTPlaybackCue& X) { return X.Kind == Tipo; });
			Corse += (bOra && !bPrima) ? 1 : 0;
			bPrima = bOra;
		}
		return Corse;
	}

	/** L'atto `Area` di prova: impronta di Branth centrata su (3,0), due vittime NON al centro. */
	TArray<FRTResolvedEvent> FxAttoArea(bool bConImpronta = true)
	{
		TArray<FRTResolvedEvent> T;
		if (bConImpronta)
		{
			T.Add(FxImpronta(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(0, 0), FRTCellId(3, 0)));
		}
		T.Add(FxColpo(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(3, 0), FRTCellId(4, 0)));
		T.Add(FxColpo(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(3, 0), FRTCellId(2, 1)));
		return T;
	}
}

/**
 * Il `Marker` solo per chi conosceva la vittima nella sua cella (`ImpactVerdict`, spec §2.3), e solo se il profilo lo
 * dice. ✅ Validato per mutazione (10): `ImpactVerdict` ignorato fa cadere «chi non conosceva la vittima».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyImpactMarkerNeedsTheVictimTest,
	"RefactorTactics.Privacy.ImpactMarkerNeedsTheVictim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyImpactMarkerNeedsTheVictimTest::RunTest(const FString&)
{
	FRTResolvedEvent Colpo = FxColpo(1, TEXT("Hero.Branth.ImpactShot"), ERTAbilityShape::Single, FRTCellId(0, 0), FRTCellId(2, 0));
	Colpo.HitGeometry.ImpactVerdict = FRTKnowledgeVerdict::NoOne();
	Colpo.HitGeometry.ImpactVerdict.AllowTeam(0);
	FRTPlaybackCue C;
	TestFalse(TEXT("🔴 chi non conosceva la vittima non vede il Marker"), URTPlaybackLibrary::ImpactCueFor(Colpo, 1, 0.5f, C));
	if (TestTrue(TEXT("chi la conosceva lo vede"), URTPlaybackLibrary::ImpactCueFor(Colpo, 0, 0.5f, C)))
	{
		TestTrue(TEXT("e' un Marker"), C.Kind == ERTPlaybackCueKind::Marker);
		TestTrue(TEXT("sulla cella d'impatto"), C.At == FRTCellId(2, 0));
	}
	const FRTResolvedEvent Finta = FxColpo(1, TEXT("Hero.Ivrin.Feint"), ERTAbilityShape::Single, FRTCellId(0, 0), FRTCellId(2, 0));
	TestFalse(TEXT("un profilo con Impact = None non ha Marker"), URTPlaybackLibrary::ImpactCueFor(Finta, 0, 0.5f, C));
	return true;
}

/**
 * L'`AreaPulse` sta sull'`AimCell` dell'IMPRONTA, non su un `Impact` (spec §2.4, F5).
 * ➕ rev2. **Premessa asserita prima**: l'`AimCell` e' diversa da OGNI `Impact` dell'atto, o la mutante (11) sarebbe
 * indistinguibile. ✅ Validato per mutazione (11).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxAreaPulseIsOnTheFootprintAimTest,
	"RefactorTactics.Fx.AreaPulseIsOnTheFootprintAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxAreaPulseIsOnTheFootprintAimTest::RunTest(const FString&)
{
	const TArray<FRTResolvedEvent> T = FxAttoArea();
	const FRTCellId Mira = T[0].AimCell;
	for (int32 I = 1; I < T.Num(); ++I)
	{
		if (!TestFalse(TEXT("⛔ premessa: l'AimCell non e' l'Impact di nessun colpo"), T[I].HitGeometry.Impact == Mira)) { return false; }
	}
	const float A = 0.5f;
	const float Arrivo = URTPlaybackLibrary::AttackBeatSeconds(3, A, FxVoli(T, A)); // il battito 2·1+1: arrivo del primo colpo
	const TArray<FRTPlaybackCue> C = FxCueAl(T, Arrivo + 0.01f, A);
	TestTrue(TEXT("🔴 l'AreaPulse e' sull'AimCell dell'impronta"), FxHa(C, ERTPlaybackCueKind::AreaPulse, Mira));
	TestFalse(TEXT("e non sull'Impact del colpo che lo porta"), FxHa(C, ERTPlaybackCueKind::AreaPulse, T[1].HitGeometry.Impact));
	return true;
}

/**
 * OGNI `Attack` ha il suo `Marker`, il primo dell'atto compreso (spec §2.4, F6: la R8 e' caduta), su un'`Area` con
 * due vittime e su una `Line` con due vittime.
 * ✅ Validato per mutazione (12).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackEveryAttackGetsItsProfileMarkerTest,
	"RefactorTactics.Playback.EveryAttackGetsItsProfileMarker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackEveryAttackGetsItsProfileMarkerTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T = FxAttoArea();
	T.Add(FxImpronta(2, TEXT("Hero.Aevik.LinearDischarge"), ERTAbilityShape::Line, FRTCellId(0, 2), FRTCellId(3, 2)));
	T.Add(FxColpo(2, TEXT("Hero.Aevik.LinearDischarge"), ERTAbilityShape::Line, FRTCellId(0, 2), FRTCellId(1, 2)));
	T.Add(FxColpo(2, TEXT("Hero.Aevik.LinearDischarge"), ERTAbilityShape::Line, FRTCellId(0, 2), FRTCellId(2, 2)));
	const float A = 0.5f;
	const TArray<float> V = FxVoli(T, A);
	TestTrue(TEXT("premessa: la scarica vola (Line con un id), il mortaio no"), V[4] > 0.f && V[1] == 0.f);
	for (int32 K = 0; K < T.Num(); ++K)
	{
		if (T[K].Type != ERTResolvedEventType::Attack) { continue; }
		const float Arrivo = URTPlaybackLibrary::AttackBeatSeconds(2 * K + 1, A, V);
		TestTrue(FString::Printf(TEXT("🔴 elemento %d: Marker sulla sua vittima all'arrivo"), K),
			FxHa(FxCueAl(T, Arrivo + 0.01f, A), ERTPlaybackCueKind::Marker, T[K].HitGeometry.Impact));
	}
	return true;
}

/**
 * UNA cue d'impronta per impronta, portata dal primo colpo, contata su TUTTI gli istanti del Blast (➕ rev2.); con
 * l'impronta tolta nessun pulse, nessun errore, e i `Marker` restano.
 * ✅ Validato per mutazione (21): l'impronta non consumata fa portare il pulse a ogni colpo — due corse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackFootprintCueOncePerFootprintTest,
	"RefactorTactics.Playback.FootprintCueOncePerFootprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackFootprintCueOncePerFootprintTest::RunTest(const FString&)
{
	TestEqual(TEXT("🔴 un solo AreaPulse in tutto il Blast"), FxCorse(FxAttoArea(), 0.5f, ERTPlaybackCueKind::AreaPulse), 1);
	TestEqual(TEXT("e due Marker, uno per colpo"), FxCorse(FxAttoArea(), 0.5f, ERTPlaybackCueKind::Marker), 2);
	TestEqual(TEXT("impronta tolta: nessun pulse"), FxCorse(FxAttoArea(false), 0.5f, ERTPlaybackCueKind::AreaPulse), 0);
	TestEqual(TEXT("impronta tolta: i Marker restano"), FxCorse(FxAttoArea(false), 0.5f, ERTPlaybackCueKind::Marker), 2);
	return true;
}

/**
 * R14 (➕ rev2.): due `AttackFootprint` con la stessa chiave prima del colpo → la cue usa le celle della SECONDA.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackSecondFootprintReplacesTheFirstTest,
	"RefactorTactics.Playback.SecondFootprintReplacesTheFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackSecondFootprintReplacesTheFirstTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T;
	T.Add(FxImpronta(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(0, 0), FRTCellId(3, 0)));
	T.Add(FxImpronta(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(0, 0), FRTCellId(5, 0)));
	T.Add(FxColpo(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(5, 0), FRTCellId(6, 0)));
	const TArray<int32> Impronte = URTPlaybackLibrary::FootprintFxForSequence(T, FxSequenza(T));
	TestEqual(TEXT("🔴 il colpo consuma la SECONDA impronta"), Impronte.IsValidIndex(2) ? Impronte[2] : INDEX_NONE, 1);
	const float Arrivo = URTPlaybackLibrary::AttackBeatSeconds(5, 0.5f, FxVoli(T, 0.5f));
	const TArray<FRTPlaybackCue> C = FxCueAl(T, Arrivo + 0.01f, 0.5f);
	TestTrue(TEXT("il pulse e' sul centro della seconda"), FxHa(C, ERTPlaybackCueKind::AreaPulse, FRTCellId(5, 0)));
	TestFalse(TEXT("e non su quello della prima"), FxHa(C, ERTPlaybackCueKind::AreaPulse, FRTCellId(3, 0)));
	return true;
}

/**
 * Review Focus (d): il centro di un'`Area` non visto da chi guarda non si rivela col pulse. `FromVerdict` di un colpo
 * `Area` e' congelato sul CENTRO (`RTTurnManager.cpp`, il produttore dell'`Attack`; spec §2.4); il `Marker` segue
 * invece la vittima.
 * ✅ Validato per mutazione (P3): `FromVerdict` ignorato in `FootprintCueFor`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyAreaPulseNeedsTheCenterTest,
	"RefactorTactics.Privacy.AreaPulseNeedsTheCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyAreaPulseNeedsTheCenterTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T = FxAttoArea();
	T[1].HitGeometry.FromVerdict = FRTKnowledgeVerdict::NoOne();
	T[1].HitGeometry.FromVerdict.AllowTeam(0);
	FRTPlaybackCue C;
	TestFalse(TEXT("🔴 chi non vede il centro non riceve l'AreaPulse"), URTPlaybackLibrary::FootprintCueFor(T[0], T[1], 1, 0.5f, C));
	TestTrue(TEXT("ma vede il Marker della vittima che conosce"), URTPlaybackLibrary::ImpactCueFor(T[1], 1, 0.5f, C));
	TestTrue(TEXT("controllo: chi vede il centro riceve il pulse"), URTPlaybackLibrary::FootprintCueFor(T[0], T[1], 0, 0.5f, C)
		&& C.Kind == ERTPlaybackCueKind::AreaPulse && C.At == FRTCellId(3, 0));
	return true;
}

/**
 * Le finestre delle cue di elementi diversi non si sovrappongono e finiscono entro `N·A` (spec §2.1, «conseguenza dei
 * tetti»), sulla griglia dichiarata: `A ∈ {0.1, 0.5, 1.0}`, `F ∈ {0, A/4, A/2, A}`, durate `∈ {0, A/2, 2A}`.
 * Sequenza mista: attivazione, colpo con volo, colpo senza volo, attivazione, colpo con volo.
 * ✅ Validato per mutazione (13): i `Min` tolti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackFxCuesNeverOverlapInTheBlastTest,
	"RefactorTactics.Playback.FxCuesNeverOverlapInTheBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackFxCuesNeverOverlapInTheBlastTest::RunTest(const FString&)
{
	for (const float A : { 0.1f, 0.5f, 1.0f })
	{
		for (const float Fq : { 0.f, 0.25f * A, 0.5f * A, A })
		{
			for (const float Act : { 0.f, 0.5f * A, 2.f * A })
			{
				for (const float Imp : { 0.f, 0.5f * A, 2.f * A })
				{
					const float F = URTPlaybackLibrary::TracerFlightFor(true, Fq, A);
					// (tipo, volo): true = attivazione, false = colpo
					const TArray<TPair<bool, float>> Elementi = { { true, 0.f }, { false, F }, { false, 0.f }, { true, 0.f }, { false, F } };
					TArray<FVector2D> Finestre; // [inizio, fine)
					for (int32 K = 0; K < Elementi.Num(); ++K)
					{
						const float Lancio = URTPlaybackLibrary::AttackLaunchSeconds(K, A);
						const float Inizio = Elementi[K].Key ? Lancio : Lancio + Elementi[K].Value;
						const float Durata = Elementi[K].Key ? URTPlaybackLibrary::ActivationCueDuration(Act, A)
							: URTPlaybackLibrary::ImpactCueDuration(Imp, A, Elementi[K].Value);
						Finestre.Add(FVector2D(Inizio, Inizio + Durata));
					}
					for (int32 I = 0; I < Finestre.Num(); ++I)
					{
						TestTrue(FString::Printf(TEXT("A=%.2f F=%.2f act=%.2f imp=%.2f: elemento %d entro N·A"), A, F, Act, Imp, I),
							Finestre[I].Y <= Elementi.Num() * A + 1e-4f);
						for (int32 J = I + 1; J < Finestre.Num(); ++J)
						{
							TestTrue(FString::Printf(TEXT("🔴 A=%.2f F=%.2f act=%.2f imp=%.2f: %d finisce prima che %d cominci"),
								A, F, Act, Imp, I, J), Finestre[I].Y <= Finestre[J].X + 1e-4f);
						}
					}
				}
			}
		}
	}
	return true;
}

// --- La geometria delle cue (#3578, spec §2.1, F7) -------------------------------------------------------------

namespace
{
	struct FFxFirma { int32 Segmenti = 0; float Max = 0.f; float Min = TNumericLimits<float>::Max(); float Verticale = 0.f; };

	FFxFirma FxFirmaDi(ERTPlaybackCueKind Tipo)
	{
		const FVector Ancora(0.f, 0.f, 0.f);
		TArray<FVector> Da, A;
		URTPlaybackLibrary::CueSegments(Tipo, Ancora, FVector(300.f, 0.f, 0.f), 100.f, 0.5f, Da, A);
		FFxFirma F;
		F.Segmenti = Da.Num();
		float ZMin = TNumericLimits<float>::Max(), ZMax = -TNumericLimits<float>::Max();
		for (int32 I = 0; I < Da.Num(); ++I)
		{
			for (const FVector& P : { Da[I], A[I] })
			{
				const float D = FVector::Dist(P, Ancora);
				F.Max = FMath::Max(F.Max, D);
				F.Min = FMath::Min(F.Min, D);
				ZMin = FMath::Min(ZMin, P.Z);
				ZMax = FMath::Max(ZMax, P.Z);
			}
		}
		F.Verticale = Da.Num() > 0 ? ZMax - ZMin : 0.f;
		return F;
	}
}

/**
 * Le cue elencate qui sotto sono DIVERSE in geometria, a coppie (spec §2.1, F7, #2453: il canale e' la geometria, mai il solo
 * colore): per ogni coppia, numero di segmenti diverso, oppure ≥ 0.1 s di differenza nella distanza massima o minima
 * dall'ancora, oppure nell'estensione verticale. A `α = 0.5`, `s = 100`.
 * ✅ Validato per mutazione (20): `AreaPulse` disegnato come `Ring` fa cadere la coppia `Ring`/`AreaPulse`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxCueStylesDifferByGeometryTest,
	"RefactorTactics.Fx.CueStylesDifferByGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxCueStylesDifferByGeometryTest::RunTest(const FString&)
{
	const TArray<ERTPlaybackCueKind> Tipi = { ERTPlaybackCueKind::Ring, ERTPlaybackCueKind::Pulse, ERTPlaybackCueKind::Flash,
		ERTPlaybackCueKind::Marker, ERTPlaybackCueKind::AreaPulse, ERTPlaybackCueKind::ConeSweep };
	const float Soglia = 0.1f * 100.f;
	for (int32 I = 0; I < Tipi.Num(); ++I)
	{
		const FFxFirma Fi = FxFirmaDi(Tipi[I]);
		TestTrue(FString::Printf(TEXT("%s: almeno un segmento"), *UEnum::GetValueAsString(Tipi[I])), Fi.Segmenti > 0);
		for (int32 J = I + 1; J < Tipi.Num(); ++J)
		{
			const FFxFirma Fj = FxFirmaDi(Tipi[J]);
			const bool bDiversi = Fi.Segmenti != Fj.Segmenti || FMath::Abs(Fi.Max - Fj.Max) >= Soglia
				|| FMath::Abs(Fi.Min - Fj.Min) >= Soglia || FMath::Abs(Fi.Verticale - Fj.Verticale) >= Soglia;
			TestTrue(FString::Printf(TEXT("🔴 %s e %s si distinguono per geometria"),
				*UEnum::GetValueAsString(Tipi[I]), *UEnum::GetValueAsString(Tipi[J])), bDiversi);
		}
	}
	return true;
}

/**
 * Il `ConeSweep` (spec §2.4, F4): da un atto `Cone` costruito dal test — nessuna azione del catalogo dichiara `Cone`
 * (spec §1) — la cue e' ancorata all'`Origin` dell'impronta e punta all'`AimCell`; la geometria ha i due bordi
 * simmetrici attorno all'asse e il braccio lungo `|Origin → AimCell|`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxConeSweepAxisIsTheAimTest,
	"RefactorTactics.Fx.ConeSweepAxisIsTheAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxConeSweepAxisIsTheAimTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T;
	T.Add(FxImpronta(3, TEXT("Hero.Prova.Ventaglio"), ERTAbilityShape::Cone, FRTCellId(0, 0), FRTCellId(2, 0)));
	T.Add(FxColpo(3, TEXT("Hero.Prova.Ventaglio"), ERTAbilityShape::Cone, FRTCellId(0, 0), FRTCellId(1, 0)));
	const float Arrivo = URTPlaybackLibrary::AttackBeatSeconds(3, 0.5f, FxVoli(T, 0.5f));
	const TArray<FRTPlaybackCue> C = FxCueAl(T, Arrivo + 0.01f, 0.5f);
	const FRTPlaybackCue* Sweep = C.FindByPredicate([](const FRTPlaybackCue& X) { return X.Kind == ERTPlaybackCueKind::ConeSweep; });
	if (!TestNotNull(TEXT("🔴 l'atto Cone ha il suo ConeSweep"), Sweep)) { return false; }
	TestTrue(TEXT("ancorato all'Origin dell'impronta"), Sweep->At == FRTCellId(0, 0));
	TestTrue(TEXT("verso l'AimCell dell'impronta, non verso la vittima"), Sweep->Toward == FRTCellId(2, 0));

	const FVector Da(0.f, 0.f, 0.f), Verso(200.f, 0.f, 0.f);
	TArray<FVector> S, E;
	URTPlaybackLibrary::CueSegments(ERTPlaybackCueKind::ConeSweep, Da, Verso, 100.f, 0.5f, S, E);
	if (!TestEqual(TEXT("tre segmenti"), S.Num(), 3)) { return false; }
	for (const FVector& P : S) { TestTrue(TEXT("ogni segmento parte dall'origine"), P.Equals(Da, 0.01f)); }
	// ⚠️ Letterali `double`: `FVector` e' in doppia precisione (LWC), e `TestEqual(double, float, float)` e' ambiguo.
	TestEqual(TEXT("bordo sinistro lungo 0.3 L"), FVector::Dist(Da, E[0]), 0.3 * 200.0, 0.5);
	TestEqual(TEXT("bordo destro lungo 0.3 L"), FVector::Dist(Da, E[1]), 0.3 * 200.0, 0.5);
	TestEqual(TEXT("i bordi sono simmetrici attorno all'asse"), E[0].Y, -E[1].Y, 0.5);
	TestEqual(TEXT("🔴 il braccio e' lungo |Origin → AimCell|"), FVector::Dist(Da, E[2]), 200.0, 0.5);
	TestEqual(TEXT("a α = 0.5 il braccio sta sull'asse"), E[2].Y, 0.0, 0.5);
	return true;
}

// --- I test dei Task 3, 4 e 5 si aggiungono QUI, prima di `#endif` ----------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
