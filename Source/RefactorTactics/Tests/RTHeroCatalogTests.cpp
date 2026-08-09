#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Un'abilita' minimale ma valida: portata e danno qualsiasi, nessun ActionId (non serve a questi test). */
	URTActionData* MakeHeroAbility(int32 RangeCells = 3, int32 Power = 10)
	{
		URTActionData* Ability = NewObject<URTActionData>();
		Ability->RangeCells = RangeCells;
		Ability->Power = Power;
		return Ability;
	}

	/**
	 * Un eroe STRUTTURALMENTE completo: attacco base (indice 0, dalla fascia di portata come ogni eroe reale)
	 * + quattro abilita' fondamentali, di cui la terza (`Actions[3]`) dichiara una variante — cosi' ogni test
	 * parte da uno stato che il validator accetta, e le mutazioni lo rompono in un punto solo per volta.
	 */
	URTHeroData* MakeValidHero(const TCHAR* Id, int32 WeaponRangeCells = 4)
	{
		URTHeroData* Hero = NewObject<URTHeroData>();
		Hero->HeroId = FName(Id);
		Hero->MaxHealth = 90;
		Hero->MovePoints = 5;
		Hero->VisionRange = 6;
		Hero->PushResistance = 0;
		Hero->Affinity = TEXT("test-affinity");
		Hero->Weakness = TEXT("test-weakness");

		const FRTActionDef BasicAttackDef = URTCatalogLibrary::MakeBasicAttack(WeaponRangeCells);
		URTActionData* BasicAttack = MakeHeroAbility(BasicAttackDef.RangeCells,
			BasicAttackDef.Effects.Num() > 0 ? BasicAttackDef.Effects[0].Amount : 0);
		Hero->Actions.Add(BasicAttack);

		for (int32 i = 0; i < 4; ++i)
		{
			Hero->Actions.Add(MakeHeroAbility());
		}

		FRTAbilityVariant Variant;
		Variant.VariantId = FName(TEXT("Variant.Alt"));
		Variant.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 5));
		Hero->Actions[3]->Variants.Add(Variant);

		return Hero;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroStatsFromDataTest,
	"RefactorTactics.Heroes.StatsFromData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroStatsFromDataTest::RunTest(const FString&)
{
	// Nome vincolante della DoD (CP 6.1): un'unita' configurata da `URTHeroData` porta ESATTAMENTE i valori
	// dell'asset, non un numero scritto in ARTUnit — a differenza di `ConfigureAsArchetype`, che resta il
	// percorso dei due archetipi legacy.
	URTHeroData* Hero = MakeValidHero(TEXT("Hero.Test"));
	Hero->MaxHealth = 123;
	Hero->MovePoints = 7;
	Hero->VisionRange = 9;
	Hero->PushResistance = 2;
	Hero->Affinity = TEXT("acqua");
	Hero->Weakness = TEXT("elettricita");

	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->ConfigureFromHeroData(Hero);

	TestEqual(TEXT("HeroId"), Unit->HeroId, Hero->HeroId);
	TestEqual(TEXT("salute"), Unit->MaxHealth, 123);
	TestEqual(TEXT("salute corrente riportata al massimo"), Unit->Health, 123);
	TestEqual(TEXT("movimento"), Unit->MoveRange, 7);
	TestEqual(TEXT("vista"), Unit->VisionRange, 9);
	TestEqual(TEXT("resistenza push"), Unit->PushResistance, 2);
	TestEqual(TEXT("affinita'"), Unit->Affinity, FName(TEXT("acqua")));
	TestEqual(TEXT("debolezza"), Unit->Weakness, FName(TEXT("elettricita")));
	// Le cinque dell'eroe PIU' le generiche di D-025, accodate da `ConfigureFromHeroData`. Il numero si
	// compone, non si scrive: aggiungere una generica non deve costringere a inseguire un `8` qui dentro.
	TestEqual(TEXT("le azioni dell'eroe piu' le generiche"),
		Unit->NumAbilities(), 5 + URTCatalogLibrary::GetGenericActionIds().Num());
	// L'indice 0 resta l'attacco base: e' un contratto del catalogo, ed e' la ragione per cui le generiche
	// vanno IN CODA. Se un giorno finissero in testa, questo assert cade prima che lo faccia una partita.
	if (const URTActionData* First = Unit->GetAbility(0))
	{
		TestEqual(TEXT("l'indice 0 e' ancora l'attacco base dell'eroe"),
			First->Def.ActionId, Hero->Actions[0]->Def.ActionId);
	}

	// Cambiare i NUMERI sull'asset e riconfigurare cambia l'unita' di conseguenza: la fonte e' l'asset, non un
	// valore congelato alla prima chiamata.
	Hero->MaxHealth = 55;
	Unit->ConfigureFromHeroData(Hero);
	TestEqual(TEXT("riconfigurare aggiorna la salute"), Unit->MaxHealth, 55);

	// FAIL-CLOSED: senza dati non si scrive nulla, non un numero a caso.
	ARTUnit* Untouched = NewObject<ARTUnit>();
	const int32 BeforeHealth = Untouched->MaxHealth;
	Untouched->ConfigureFromHeroData(nullptr);
	TestEqual(TEXT("hero nullo: la salute non cambia"), Untouched->MaxHealth, BeforeHealth);
	TestTrue(TEXT("hero nullo: nessun HeroId"), Untouched->HeroId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroBasicAttackByRangeBandTest,
	"RefactorTactics.Heroes.BasicAttackByRangeBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroBasicAttackByRangeBandTest::RunTest(const FString&)
{
	// Il contratto che ogni eroe futuro deve rispettare (CP 6.2+): l'attacco base (indice 0) si costruisce
	// con `MakeBasicAttack`, mai con un danno scelto a mano — e' la stessa tabella a fasce del catalogo
	// azioni v0.1 §1, non una seconda copia per gli eroi.
	struct FBand { int32 Range; int32 ExpectedDamage; };
	const FBand Bands[] = {
		{ 1, 28 }, // corpo a corpo
		{ 3, 25 }, // corto
		{ 4, 22 }, // medio
		{ 6, 20 }, // lungo
	};

	for (const FBand& Band : Bands)
	{
		URTHeroData* Hero = MakeValidHero(TEXT("Hero.Band"), Band.Range);
		ARTUnit* Unit = NewObject<ARTUnit>();
		if (!TestNotNull(TEXT("unita' di prova"), Unit)) { continue; }
		Unit->ConfigureFromHeroData(Hero);

		TestEqual(FString::Printf(TEXT("range %d: portata dell'attacco base"), Band.Range),
			Unit->AttackRange, Band.Range);
		TestEqual(FString::Printf(TEXT("range %d: danno dalla fascia"), Band.Range),
			Unit->AttackPower, Band.ExpectedDamage);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroBasicAttackIsIndexZeroTest,
	"RefactorTactics.Heroes.BasicAttackIsIndexZeroForEveryHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroBasicAttackIsIndexZeroTest::RunTest(const FString&)
{
	// ADR-0007 punto 6: «attacco base» e' una CONVENZIONE POSIZIONALE — `URTHeroData::Actions[0]` — e non un
	// campo del dato. La convenzione e' dichiarata nel commento di `RTHeroData.h`, ma un commento non fallisce:
	// finche' nessuno la fa valere, il primo eroe che mette una firma all'indice 0 la rompe in silenzio, e con
	// essa ogni cosa che «l'attacco base» dovrebbe poter interrogare (§14 usage profile, §29 pick rate).
	//
	// Questo test e' il prezzo di NON aver aggiunto un campo di ruolo. Il giorno in cui un consumer runtime
	// esiste davvero, il campo entra e questo test cambia forma — ma fino ad allora e' l'unica cosa che tiene
	// la convenzione.
	//
	// Asserisce il CONTRATTO, non i numeri: danno e portata sono dell'eroe (li verificano le sue
	// `*.MatchesCatalog`), qui conta che l'indice 0 sia un attacco base **utilizzabile e senza costo**.
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestEqual(TEXT("il roster v0.1 ha quattro eroi"), Roster.Num(), 4)) { return false; }

	for (int32 i = 0; i < Roster.Num(); ++i)
	{
		const URTHeroData* Hero = Roster[i];
		if (!TestNotNull(FString::Printf(TEXT("eroe #%d non nullo"), i), Hero)) { continue; }

		const FString Who = Hero->HeroId.ToString();
		if (!TestEqual(*FString::Printf(TEXT("%s: cinque azioni"), *Who), Hero->Actions.Num(), 5)) { continue; }

		const URTActionData* Basic = Hero->Actions[0];
		if (!TestNotNull(*FString::Printf(TEXT("%s: indice 0 presente"), *Who), Basic)) { continue; }

		TestFalse(*FString::Printf(TEXT("%s: l'attacco base ha un ActionId stabile"), *Who),
			Basic->Def.ActionId.IsNone());
		TestEqual(*FString::Printf(TEXT("%s: risolve nella fase Attack"), *Who),
			Basic->Def.ResolutionPhase, ERTResolutionPhase::Attack);
		// Costo zero e nessuna ricarica: e' cio' che distingue l'attacco base dalle quattro fondamentali, ed
		// e' anche la ragione per cui puo' essere «la scelta corretta» quando le firme sono in cooldown.
		TestEqual(*FString::Printf(TEXT("%s: nessuna ricarica"), *Who), Basic->Def.CooldownTurns, 0);
		TestEqual(*FString::Printf(TEXT("%s: occupa lo slot principale"), *Who),
			Basic->Def.Slot, ERTActionSlot::Main);

		// Almeno un effetto di danno: un attacco base che non fa male non e' un attacco. NON asserisce
		// QUANTO — 8 (Bastion) e 22 (Flux) sono entrambi legittimi, ed e' il punto di ADR-0007.
		bool bDealsDamage = false;
		for (const FRTActionEffectSpec& Spec : Basic->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage && Spec.Amount > 0) { bDealsDamage = true; break; }
		}
		TestTrue(*FString::Printf(TEXT("%s: l'attacco base infligge danno"), *Who), bDealsDamage);

		// L'indice 0 non deve portare varianti: quelle appartengono a UNA fondamentale (indici 1-4), e
		// `ValidateHeroes` lo conta gia' escludendo lo 0. Se qualcuno ne mettesse una qui, il conteggio
		// resterebbe verde e la regola sarebbe aggirata senza che nulla diventi rosso.
		TestEqual(*FString::Printf(TEXT("%s: l'attacco base non dichiara varianti"), *Who),
			Basic->Variants.Num(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroBasicAttackDeclaresBaseTest,
	"RefactorTactics.Heroes.BasicAttackDeclaresItsBaseAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroBasicAttackDeclaresBaseTest::RunTest(const FString&)
{
	// D-033 chiede che un'azione generica con profilo sia spiegabile nel TurnLog come *azione base + profilo*.
	// Perche' lo sia, il DATO deve dichiarare la relazione: `Bastion.ImpactShot` e' un'azione d'eroe, e chi
	// legge una traccia non la risolve col catalogo core.
	//
	// Senza questo test, dimenticare `BaseActionId` su un eroe nuovo non romperebbe NIENTE — l'azione
	// funziona, e solo la spiegabilita' si degrada in silenzio. E' il difetto tipico dei campi di sola
	// documentazione: nessuno se ne accorge finche' non serve leggere una traccia.
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestEqual(TEXT("il roster v0.1 ha quattro eroi"), Roster.Num(), 4)) { return false; }

	for (const URTHeroData* Hero : Roster)
	{
		if (!TestNotNull(TEXT("eroe non nullo"), Hero) || Hero->Actions.Num() < 5) { continue; }
		const FString Who = Hero->HeroId.ToString();

		TestEqual(*FString::Printf(TEXT("%s: l'attacco base dichiara Action.BasicAttack"), *Who),
			Hero->Actions[0]->Def.BaseActionId, FName(TEXT("Action.BasicAttack")));

		// Le quattro fondamentali NON sono profili di un'azione generica: sono abilita' proprie. Dichiararlo
		// e' importante quanto il contrario — un `BaseActionId` messo dappertutto direbbe che tutto e' un
		// profilo di qualcosa, cioe' non direbbe piu' niente.
		for (int32 a = 1; a < Hero->Actions.Num(); ++a)
		{
			if (!Hero->Actions[a]) { continue; }
			TestTrue(*FString::Printf(TEXT("%s: l'abilita' #%d non e' profilo di una generica"), *Who, a),
				Hero->Actions[a]->Def.BaseActionId.IsNone());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroExactlyOneVariantTest,
	"RefactorTactics.Heroes.ExactlyOneVariantPerHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroExactlyOneVariantTest::RunTest(const FString&)
{
	// Nome vincolante della DoD (CP 6.1): il catalogo v0.1 permette UNA sola abilita' con variante per eroe.
	// Zero varianti o due sono lo stesso errore concettuale (identita' incompleta o scelta non orizzontale
	// ma cumulativa), e il validator li rifiuta entrambi.
	{
		URTHeroData* Valid = MakeValidHero(TEXT("Hero.Valid"));
		const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ Valid });
		TestEqual(TEXT("un eroe con esattamente una variante e' valido"), Errors.Num(), 0);
	}
	{
		URTHeroData* NoVariant = MakeValidHero(TEXT("Hero.NoVariant"));
		NoVariant->Actions[3]->Variants.Reset();
		const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ NoVariant });
		TestEqual(TEXT("zero varianti: un errore"), Errors.Num(), 1);
		if (Errors.Num() == 1) { TestTrue(TEXT("nomina l'eroe"), Errors[0].Contains(TEXT("Hero.NoVariant"))); }
	}
	{
		URTHeroData* TwoVariants = MakeValidHero(TEXT("Hero.TwoVariants"));
		FRTAbilityVariant Extra;
		Extra.VariantId = FName(TEXT("Variant.Second"));
		TwoVariants->Actions[1]->Variants.Add(Extra);
		const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ TwoVariants });
		TestEqual(TEXT("due abilita' con variante: un errore"), Errors.Num(), 1);
	}
	{
		// La variante sull'ATTACCO BASE (indice 0) non conta: quella e' la variante d'arma dell'equipaggiamento
		// (E7), non la variante di un'abilita' fondamentale del catalogo eroi.
		URTHeroData* OnBasicAttack = MakeValidHero(TEXT("Hero.OnBasicAttack"));
		OnBasicAttack->Actions[3]->Variants.Reset(); // toglie l'unica variante valida...
		FRTAbilityVariant OnAttack;
		OnAttack.VariantId = FName(TEXT("Variant.OnBasicAttack"));
		OnBasicAttack->Actions[0]->Variants.Add(OnAttack); // ...e la mette sull'attacco base
		const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ OnBasicAttack });
		TestEqual(TEXT("variante sull'attacco base: conta come zero varianti fondamentali"), Errors.Num(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroValidateStructureTest,
	"RefactorTactics.Heroes.ValidateHeroesStructure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroValidateStructureTest::RunTest(const FString&)
{
	// Ogni riga della DoD ha un errore dedicato, e ognuno nomina l'eroe: il validator non riduce mai un roster
	// rotto a un singolo "qualcosa non va" indistinguibile.
	{
		URTHeroData* NoId = MakeValidHero(TEXT("Hero.NoId"));
		NoId->HeroId = NAME_None;
		TestEqual(TEXT("HeroId mancante"), URTHeroCatalogLibrary::ValidateHeroes({ NoId }).Num(), 1);
	}
	{
		URTHeroData* A = MakeValidHero(TEXT("Hero.Dup"));
		URTHeroData* B = MakeValidHero(TEXT("Hero.Dup"));
		TestEqual(TEXT("HeroId duplicato"), URTHeroCatalogLibrary::ValidateHeroes({ A, B }).Num(), 1);
	}
	{
		URTHeroData* ZeroHealth = MakeValidHero(TEXT("Hero.ZeroHealth"));
		ZeroHealth->MaxHealth = 0;
		TestEqual(TEXT("salute non positiva"), URTHeroCatalogLibrary::ValidateHeroes({ ZeroHealth }).Num(), 1);
	}
	{
		URTHeroData* ZeroMove = MakeValidHero(TEXT("Hero.ZeroMove"));
		ZeroMove->MovePoints = 0;
		TestEqual(TEXT("movimento non positivo"), URTHeroCatalogLibrary::ValidateHeroes({ ZeroMove }).Num(), 1);
	}
	{
		URTHeroData* NoAffinity = MakeValidHero(TEXT("Hero.NoAffinity"));
		NoAffinity->Affinity = NAME_None;
		TestEqual(TEXT("affinita' mancante"), URTHeroCatalogLibrary::ValidateHeroes({ NoAffinity }).Num(), 1);
	}
	{
		// La debolezza non si inventa (catalogo v0.1 §5): un eroe senza debolezza dichiarata e' incompleto,
		// non "opzionale". E' la regola che tiene la porta aperta finche' non decido il valore per ogni eroe.
		URTHeroData* NoWeakness = MakeValidHero(TEXT("Hero.NoWeakness"));
		NoWeakness->Weakness = NAME_None;
		TestEqual(TEXT("debolezza mancante"), URTHeroCatalogLibrary::ValidateHeroes({ NoWeakness }).Num(), 1);
	}
	{
		URTHeroData* WrongCount = MakeValidHero(TEXT("Hero.WrongCount"));
		WrongCount->Actions.Add(MakeHeroAbility());
		TestEqual(TEXT("sei azioni invece di cinque"), URTHeroCatalogLibrary::ValidateHeroes({ WrongCount }).Num(), 1);
	}
	{
		TestEqual(TEXT("riferimento nullo nel roster"), URTHeroCatalogLibrary::ValidateHeroes({ nullptr }).Num(), 1);
	}
	return true;
}

// =====================================================================================================
// D-028 — una mobilita' che non fa danno NON puo' occupare l'azione principale.
//
// La discriminante non e' il nome dell'azione ma i suoi effetti: `Charge` sta sulla principale perche' e'
// un attacco che ti porta addosso al bersaglio; uno scatto che non colpisce e' movimento e basta.
//
// Verificato sul ROSTER, non su un'azione costruita nel test: `MakeHeroAction` assegna `Main` per DEFAULT,
// quindi ogni prossima mobilita' d'eroe nascera' sullo slot sbagliato se nessuno la dichiara. E' esattamente
// il caso che questo test deve intercettare — non l'errore di oggi, quello di domani.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroMobilitySlotTest,
	"RefactorTactics.Heroes.MobilityWithoutDamageIsNotMain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroMobilitySlotTest::RunTest(const FString&)
{
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(TEXT("il roster non e' vuoto"), Roster.Num() > 0)) { return false; }

	int32 MobilityChecked = 0;
	for (const URTHeroData* Hero : Roster)
	{
		if (!Hero) { continue; }
		for (const URTActionData* Action : Hero->Actions)
		{
			if (!Action || Action->Def.ResolutionPhase != ERTResolutionPhase::FastMovement) { continue; }

			bool bDealsDamage = false;
			for (const FRTActionEffectSpec& Effect : Action->Def.Effects)
			{
				if (Effect.Effect == ERTActionEffect::Damage) { bDealsDamage = true; break; }
			}
			if (bDealsDamage) { continue; }   // e' una carica: la principale le spetta

			++MobilityChecked;
			TestEqual(
				*FString::Printf(TEXT("%s e' mobilita' senza danno: slot movimento"), *Action->Def.ActionId.ToString()),
				static_cast<int32>(Action->Def.Slot), static_cast<int32>(ERTActionSlot::Movement));
		}
	}

	// Un ciclo che non itera non fallisce mai: senza una riga qui sotto, questo test resterebbe verde anche se
	// il roster perdesse OGNI mobilita'. Prima la riga chiedeva `> 0`; dal 2026-08-09 chiede ESATTAMENTE zero,
	// e va spiegato.
	//
	// D-046 (#282) ha cablato `Riva.FluidTrail` su `Action.CreateWater`: era l'unica mobilita' SENZA danno del
	// roster, perche' `Vektor.PassingBlade` e' FastMovement ma fa 20 danni, cioe' una carica. La regola D-028
	// e' quindi rimasta senza soggetto: vera, e oggi senza nessuno a cui applicarsi.
	//
	// Asserire lo stato attuale invece di rilassare la guardia: quando il roster v0.2 (Steel, Aurora, Murdock,
	// Kwang) introdurra' una mobilita' pura, questa riga CADRA'. E' un promemoria che si fa sentire, non un
	// vincolo — a quel punto si torna a `> 0` e il ciclo ricomincia a verificare la regola davvero.
	TestEqual(TEXT("oggi nessuna mobilita' senza danno nel roster (D-046; la v0.2 la riportera')"),
		MobilityChecked, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
