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

// --- I test dei Task 3, 4 e 5 si aggiungono QUI, prima di `#endif` ----------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
