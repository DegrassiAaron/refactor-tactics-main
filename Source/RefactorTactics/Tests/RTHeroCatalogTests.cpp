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
	// dell'asset, non un numero scritto in ARTUnit. Dal 2026-08-10 e' anche l'unico modo di configurarla:
	// `ConfigureAsArchetype`, che i numeri li aveva in C++, e' stata rimossa.
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
		// [D-226] Un eroe ne ha cinque o sei: la sesta e' lo scudo proattivo di Phase e Wraith. Qui basta
		// che il kit NON sia vuoto — la proprieta' in esame e' «l'indice 0 e' l'attacco base», e legarla a
		// un conteggio esatto la faceva cadere ogni volta che il roster cambiava forma.
		if (!TestTrue(*FString::Printf(TEXT("%s: il kit non e' vuoto"), *Who), Hero->Actions.Num() > 0))
		{
			continue;
		}

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
		// QUANTO — 8 (Riktor) e 22 (Gadget) sono entrambi legittimi, ed e' il punto di ADR-0007.
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
	// Perche' lo sia, il DATO deve dichiarare la relazione: `Riktor.ImpactShot` e' un'azione d'eroe, e chi
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
		// [D-226] Il confine si e' spostato, e questo test lo insegue da ENTRAMBI i lati: sei azioni sono
		// legali — e' cio' che permette a Phase e Wraith di portare lo scudo proattivo — sette no.
		//
		// ⚠️ **Il verso positivo non e' ridondante.** Con il solo caso a sette, un validatore che avesse
		// smesso del tutto di contare resterebbe rosso qui e verde ovunque; con il solo caso a sei, uno che
		// non contasse piu' passerebbe. Servono i due lati per pinnare un INTERVALLO.
		URTHeroData* SixActions = MakeValidHero(TEXT("Hero.SixActions"));
		SixActions->Actions.Add(MakeHeroAbility());
		TestEqual(TEXT("sei azioni: legale da D-226"),
			URTHeroCatalogLibrary::ValidateHeroes({ SixActions }).Num(), 0);

		URTHeroData* SevenActions = MakeValidHero(TEXT("Hero.SevenActions"));
		SevenActions->Actions.Add(MakeHeroAbility());
		SevenActions->Actions.Add(MakeHeroAbility());
		TestEqual(TEXT("sette azioni: oltre il tetto, il kit non sarebbe piu' premibile"),
			URTHeroCatalogLibrary::ValidateHeroes({ SevenActions }).Num(), 1);

		// E il lato basso resta chiuso: quattro azioni sono un eroe incompleto.
		URTHeroData* FourActions = MakeValidHero(TEXT("Hero.FourActions"));
		FourActions->Actions.Pop();
		TestEqual(TEXT("quattro azioni: sotto il minimo"),
			URTHeroCatalogLibrary::ValidateHeroes({ FourActions }).Num(), 1);
	}
	{
		TestEqual(TEXT("riferimento nullo nel roster"), URTHeroCatalogLibrary::ValidateHeroes({ nullptr }).Num(), 1);
	}
	return true;
}

// =====================================================================================================
// [D-191] — una mobilita' rapida occupa il MOVIMENTO. Il danno non conta, lo stile non conta.
//
// Chi risolve nella macro-fase `Dash` si e' mosso, e ha speso per questo lo slot movimento: che poi faccia
// danno a chi trapassa (`LinearPass`), a chi raggiunge (`LinearCharge`) o a nessuno (`LinearDash`,
// `LinearLeap`, `Budget`) non cambia CHE COSA ha speso. Chi vuole che una mobilita' costi anche la
// principale lo dichiara, e il modo esiste: `ERTActionSlot::MovementAndMain`.
//
// Verificato sul ROSTER, non su un'azione costruita nel test: `MakeHeroAction` assegna `Main` per DEFAULT,
// quindi ogni prossima mobilita' d'eroe nascera' sullo slot sbagliato se nessuno la dichiara. E' esattamente
// il caso che questo test deve intercettare — non l'errore di oggi, quello di domani.
//
// 🔴 **E fino al 2026-08-25 non lo intercettava.** Il criterio era «fa danno o no», e il ciclo faceva
// `continue` su ogni mobilita' che colpisce: `Hero.Wraith.PassingBlade` — che fa 20 danni e attraversa —
// passava indenne col `Main` di default, e da li' due azioni principali in un turno. Il test che diceva di
// prendere «l'errore di domani» non prendeva nemmeno quello di ieri, perche' la clausola sul danno lo
// escludeva per costruzione.
//
// ⚠️ **La prima correzione, lo stesso giorno, ne teneva ancora una di clausole**: distingueva per STILE e
// si aspettava `Main` dalle cariche. Bastava a rendere il test non-vacuo, non a renderlo semplice — e la
// misura su `Hero.Riktor.Ram` ha mostrato che quel caso speciale era esso stesso il difetto successivo.
// Ora non c'e' nessun caso speciale: chi risolve nel Dash dichiara il movimento, e basta.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroMobilitySlotTest,
	"RefactorTactics.Heroes.EveryFastMovementTakesTheMovementSlot",
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

			// Nessun `continue` e nessun caso speciale: ogni mobilita' del roster viene esaminata e deve
			// dichiarare lo slot movimento. Saltarne una era il difetto di prima — un'azione non esaminata e'
			// un'azione non difesa — e distinguere per stile era il difetto di poche ore dopo.
			++MobilityChecked;
			TestEqual(
				*FString::Printf(TEXT("%s risolve nel Dash: slot movimento"), *Action->Def.ActionId.ToString()),
				static_cast<int32>(Action->Def.Slot), static_cast<int32>(ERTActionSlot::Movement));
		}
	}

	// Un ciclo che non itera non fallisce mai: senza una riga qui sotto, questo test resterebbe verde anche se
	// il roster perdesse OGNI mobilita'.
	//
	// 🔵 **La riga e' tornata a `> 0` il 2026-08-16, ed e' il test stesso ad averlo prescritto.** Dal
	// 2026-08-09 chiedeva ESATTAMENTE zero, perche' D-046 (#282) aveva cablato `Phase.FluidTrail` su
	// `Action.CreateWater` togliendo al roster l'unica mobilita' SENZA danno — `Wraith.PassingBlade` e'
	// FastMovement ma fa 20 danni, cioe' una carica — e la regola D-028 era rimasta vera e senza soggetto.
	// Accanto c'era scritto: *«quando il roster v0.2 introdurra' una mobilita' pura, questa riga CADRA' […]
	// a quel punto si torna a `> 0` e il ciclo ricomincia a verificare la regola davvero»*.
	//
	// E' caduta, ma non per la v0.2: per **#1006**. `Phase.FluidTrail` e' tornata uno scatto perche' #995 ha
	// deciso che Phase e' **abilitata** a Water e non padrona — grado `Access`, una sola capability
	// elementale — e quella era la terza. Il soggetto di D-028 esiste di nuovo, quindi la guardia torna a
	// verificare la regola invece di consuntivare un'assenza.
	//
	// ⚠️ Non e' un rilassamento: `> 0` significa che il ciclo sopra ha ESEGUITO le sue assert almeno una
	// volta. Se domani il roster tornasse senza mobilita' pura, questa riga cadrebbe di nuovo e andrebbe
	// riletta — non rilassata a `>= 0`, che e' il modo in cui una guardia smette di guardare.
	TestTrue(TEXT("il roster ha almeno una mobilita' senza danno, quindi D-028 ha un soggetto (#1006)"),
		MobilityChecked > 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// `GetHeroIds()` e' una seconda copia dell'elenco: questo test le impedisce di divergere in silenzio
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroIdsMatchRosterTest,
	"RefactorTactics.Heroes.HeroIdsMatchRoster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroIdsMatchRosterTest::RunTest(const FString&)
{
	// `GetHeroIds()` esiste per non istanziare quattro `URTHeroData` completi quando servono solo i nomi
	// (CP 20.2 deriva `UI.Icon.Identity.*` dal roster). Il prezzo di quella scorciatoia e' una lista
	// scritta due volte, e il prezzo si paga QUI: se le due divergono, cade questo test invece di
	// mancare un'icona che nessuno nota finche' non apre il gioco.
	const TArray<FName> Ids = URTHeroCatalogLibrary::GetHeroIds();
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();

	if (!TestEqual(TEXT("le due liste hanno la stessa lunghezza"), Ids.Num(), Roster.Num()))
	{
		return false;
	}

	// Confronto posizione per posizione: l'ordine e' parte del contratto, perche' `RequiredIconIds()`
	// produce una lista deterministica e un riordino cambierebbe l'ordine delle chiavi Identity.
	for (int32 i = 0; i < Roster.Num(); ++i)
	{
		if (!TestNotNull(*FString::Printf(TEXT("eroe #%d non nullo"), i), Roster[i])) { continue; }

		TestEqual(*FString::Printf(TEXT("HeroId #%d coincide"), i),
			Ids[i].ToString(), Roster[i]->HeroId.ToString());
	}

	// Nessun id vuoto: un `NAME_None` qui produrrebbe la chiave `UI.Icon.Identity.` — sintatticamente una
	// chiave, semanticamente niente.
	for (const FName& Id : Ids)
	{
		TestFalse(TEXT("nessun HeroId e' vuoto"), Id.IsNone());
	}

	return true;
}

/**
 * Ogni azione del catalogo eroi ha un nome mostrabile (issue #892).
 *
 * `DisplayName` e' dichiarato «nome mostrato (UI/log)» su `URTActionData`, e in partita e' l'unico canale:
 * l'HUD non e' a schermo (#613), quindi il log e' cio' che il giocatore legge. Vuoto, produce
 * `[RT] Piano: RTUnit_0 usa  su RTUnit_3` — due spazi al posto dell'abilita' — e
 * `[RT] X: abilita' attiva -> `, osservati nel playtest di M6.8 il 2026-08-15.
 *
 * ⚠️ L'elenco si **genera** da `GetHeroRoster()`, non si trascrive: un'azione aggiunta domani senza nome
 * deve far fallire QUESTO test, non passare inosservata perche' la lista attesa era scritta a mano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroActionDisplayNameTest,
	"RefactorTactics.Heroes.EveryActionHasADisplayName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroActionDisplayNameTest::RunTest(const FString&)
{
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(TEXT("il roster non e' vuoto"), Roster.Num() > 0))
	{
		return false;
	}

	// Contato e verificato: senza questo, un roster che smettesse di esporre azioni renderebbe il test
	// verde per assenza di soggetti — la forma di falso verde che #892 esiste per evitare.
	int32 Checked = 0;
	for (const URTHeroData* Hero : Roster)
	{
		if (!Hero)
		{
			continue;
		}
		for (int32 i = 0; i < Hero->Actions.Num(); ++i)
		{
			const URTActionData* Action = Hero->Actions[i];
			if (!Action)
			{
				continue;
			}
			++Checked;
			TestFalse(
				*FString::Printf(TEXT("%s azione #%d (%s) ha un DisplayName"),
					*Hero->HeroId.ToString(), i, *Action->Def.ActionId.ToString()),
				Action->DisplayName.IsEmpty());
		}
	}

	TestTrue(TEXT("almeno un'azione controllata"), Checked > 0);
	return true;
}

/**
 * Ogni `ActionId` d'eroe e' `Hero.<Nome>.<Abilita>`, e nessuno porta un nome ritirato (#754, D-130).
 *
 * ⚠️ **Prende il posto di una spunta del DoD che non e' eseguibile.** `#754` chiede che
 * `Catalog.ValidatorRejectsRetiredHeroAbilityId` *«diventi verde»*, ma quel test non esiste e non puo':
 * il suo omologo `Catalog.ValidatorRejectsRetiredStableId` e' stato **rimosso** da **D-134** insieme al
 * redirect, «perche' verificava un divieto che non esiste piu'». Dove non c'e' redirect non c'e' nulla da
 * rifiutare: un ID ritirato semplicemente non risolve. La guardia che serviva davvero e' questa.
 *
 * Due asserzioni, e la seconda e' quella che il grep non sa fare. Il DoD motiva il **terzo segmento**
 * dicendo che un prefisso piatto metterebbe `Gadget.ArcPulse` accanto a `Gadget.Medkit` (un oggetto) e a
 * `ERTEquipmentSlot::Gadget` (serializzato). Ma «comincia per `Hero.`» non basterebbe: legare l'azione
 * al `HeroId` del **suo** eroe fa cadere anche un'azione di Gadget che finisse sotto `Hero.Phase.` —
 * un errore che un rename massivo produce esattamente come quello che deve correggere.
 *
 * Il prefisso si legge dal roster, non si scrive qui: un quinto eroe non richiede di toccare il test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroAbilityIdNamespaceTest,
	"RefactorTactics.Heroes.AbilityIdsAreNamespacedUnderTheirHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroAbilityIdNamespaceTest::RunTest(const FString&)
{
	// 🔴 **NON RINOMINARE QUESTA RIGA.** Sono i quattro nomi che D-130 ritira, e devono comparire qui
	// **scritti per esteso**: e' l'unico posto del progetto in cui nominarli e' il punto, non un residuo.
	//
	// Il rename di #754 li ha portati via DUE volte, e le due ragioni sono diverse.
	//   1. Un passaggio finale cercava i nomi ritirati *senza confini di parola*, per stanare le forme
	//      concatenate — `<ritirato>EffectAmount`, `<ritirato>Data` — che nessun pattern con `\b` o col
	//      punto poteva vedere. Ha riscritto anche questa lista, e il test e' diventato l'opposto di se
	//      stesso: asseriva che nessun ID contenesse i nomi **nuovi**, quindi sarebbe caduto su tutti e
	//      venti. Una guardia rovesciata non e' una guardia rotta: e' una guardia che accusa il codice sano.
	//   2. Poi lo stesso script e' stato rilanciato **per misurare quanti residui restassero**, e ha
	//      disfatto la riparazione appena scritta. Uno script che sostituisce non e' una misura, per quanto
	//      il suo ultimo `print` somigli a una.
	const TArray<FString> Ritirati = { TEXT("Flux"), TEXT("Riva"), TEXT("Bastion"), TEXT("Vektor") };

	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(TEXT("il roster non e' vuoto"), Roster.Num() > 0)) { return false; }

	int32 Checked = 0;
	for (const URTHeroData* Hero : Roster)
	{
		if (!Hero) { continue; }
		const FString Prefisso = Hero->HeroId.ToString() + TEXT(".");

		for (const URTActionData* Action : Hero->Actions)
		{
			if (!Action) { continue; }
			++Checked;
			const FString Id = Action->Def.ActionId.ToString();

			// 1. Nessun nome ritirato, in nessuna posizione dell'ID.
			for (const FString& Vecchio : Ritirati)
			{
				TestFalse(*FString::Printf(TEXT("%s non contiene il nome ritirato '%s'"), *Id, *Vecchio),
					Id.Contains(Vecchio));
			}

			// 2. E l'azione sta sotto il SUO eroe, non sotto un eroe qualsiasi.
			TestTrue(*FString::Printf(TEXT("%s sta sotto il proprio eroe (%s)"), *Id, *Prefisso),
				Id.StartsWith(Prefisso));
		}
	}

	// Senza questo, un roster che smettesse di esporre azioni renderebbe il test verde per assenza di
	// soggetti — la stessa forma di falso verde contro cui `EveryActionHasADisplayName` si difende.
	TestTrue(TEXT("almeno un'azione controllata"), Checked > 0);
	// Anti-vacuita': se il ciclo non avesse esaminato nulla, i controlli sopra sarebbero verdi su zero.
	// **22** da [D-226]: cinque per Gadget e Riktor, sei per Phase e Wraith che portano lo scudo proattivo.
	TestEqual(TEXT("il roster v0.1 dichiara ventidue abilita'"), Checked, 22);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroDerivedActionsDeclareOriginTest,
	"RefactorTactics.Heroes.DerivedActionsDeclareTheirOrigin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroDerivedActionsDeclareOriginTest::RunTest(const FString&)
{
	// Le derivazioni del roster v0.1 — otto fino al 2026-08-27, **dieci** da [D-226]. Chi aggiunge un eroe che
	// deriva da un'azione core aggiunge una riga qui: e' l'elenco che rende la relazione verificabile,
	// invece di lasciarla vivere nel solo sorgente dove nessun test la vede.
	//
	// ⛔ Gli attacchi base NON sono qui: dichiarano `BaseActionId` — profilo di una generica, D-033 — e non
	// una derivazione di parametri. Tre dei quattro hanno i numeri scritti a mano, non presi dal core:
	// chiamarli «derivati» trasformerebbe «i parametri vengono da li'» in «gli somiglia».
	const TMap<FName, FName> Atteso = {
		{ TEXT("Hero.Gadget.ConductiveNode"),     TEXT("Action.Electrify")    },
		{ TEXT("Hero.Phase.FluidTrail"),          TEXT("Action.Dash")         },
		{ TEXT("Hero.Phase.MistVeil"),            TEXT("Action.Ignite")       },
		{ TEXT("Hero.Riktor.KineticPanel"),       TEXT("Action.CreateCover")  },
		{ TEXT("Hero.Riktor.Ram"),                TEXT("Action.Charge")       },
		{ TEXT("Hero.Gadget.ReactiveCapacitor"),  TEXT("Action.Counter")      },
		{ TEXT("Hero.Riktor.Interposition"),      TEXT("Action.Intercept")    },
		{ TEXT("Hero.Wraith.Deflection"),         TEXT("Action.Deflect")      },
		// [D-226]: le due che chiudono la meta' `Shield` di `#1403`, uno scudo proattivo per squadra.
		{ TEXT("Hero.Phase.TideGuard"),           TEXT("Action.Shield")       },
		{ TEXT("Hero.Wraith.PhaseGuard"),         TEXT("Action.Shield")       },
	};

	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestEqual(TEXT("il roster v0.1 ha quattro eroi"), Roster.Num(), 4)) { return false; }

	int32 Dichiarate = 0;
	for (const URTHeroData* Hero : Roster)
	{
		if (!Hero) { continue; }
		for (const URTActionData* A : Hero->Actions)
		{
			if (!A) { continue; }
			const FName* Origine = Atteso.Find(A->Def.ActionId);
			if (Origine)
			{
				TestEqual(*FString::Printf(TEXT("%s dichiara la sua origine"), *A->Def.ActionId.ToString()),
					A->Def.DerivedFromActionId, *Origine);
				++Dichiarate;
			}
			else
			{
				// Il verso opposto conta quanto il primo: un campo messo dappertutto non direbbe piu'
				// niente. Non ereditano da nessuna azione core, e restano vuote, **otto** abilita':
				// `LinearDischarge`, `Overload`, `CircularTide`, `Reconfigure`, `FlowReaction`,
				// `InterceptShot`, `PassingBlade`, `Feint`. Piu' i quattro attacchi base, che dichiarano il
				// profilo (`BaseActionId`) e non questo.
				//
				// Dieci derivate + otto proprie + quattro base = **22**: cinque abilita' per Gadget e
				// Riktor, **sei** per Phase e Wraith, che da [D-226] portano anche lo scudo proattivo.
				TestTrue(*FString::Printf(TEXT("%s non deriva da nulla e non lo dichiara"),
					*A->Def.ActionId.ToString()), A->Def.DerivedFromActionId.IsNone());
			}
		}
	}

	// Anti-vacuita': se il roster tornasse vuoto o gli ID cambiassero, i cicli sopra non asserirebbero
	// niente e il test resterebbe verde raccontando che va tutto bene.
	TestEqual(TEXT("tutte le derivazioni attese sono state trovate"), Dichiarate, Atteso.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroDerivedFromUnknownIsNullTest,
	"RefactorTactics.Heroes.DerivedFromUnknownCoreActionIsNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroDerivedFromUnknownIsNullTest::RunTest(const FString&)
{
	// Fail-closed: un'azione core che il catalogo non conosce non produce un'abilita' coi default di
	// `FRTActionDef` e una derivazione falsa — quella funzionerebbe in partita e mentirebbe al gate.
	//
	// ⚠️ Si prova `MakeHeroActionFromCore` **direttamente**. La prima stesura provava il fratello
	// `MakeHeroReactionFromCoreAction` dicendo che aveva «la stessa guardia»: non e' vero — quella ha una
	// clausola in piu' sullo slot — e proprio quella differenza teneva nascosto il caso `NAME_None`.
	TestNull(TEXT("un ID che il catalogo non conosce non produce un'abilita'"),
		URTHeroCatalogLibrary::MakeHeroActionFromCore(
			TEXT("Hero.Test.Inesistente"), TEXT("Action.NonEsiste"), 1));

	// 🔴 Il caso che il confronto da solo NON prende: `Core.ActionId != CoreActionId` con entrambi vuoti e'
	// FALSO, quindi senza `IsNone()` la guardia lascia passare un'abilita' con priorita' 50, portata 0,
	// nessun effetto e derivazione vuota — invisibile anche al gate della raggiungibilita'.
	TestNull(TEXT("un ID VUOTO non produce un'abilita'"),
		URTHeroCatalogLibrary::MakeHeroActionFromCore(TEXT("Hero.Test.Vuoto"), NAME_None, 1));

	// E il verso positivo, senza il quale i due TestNull sarebbero veri anche per una funzione che
	// restituisce sempre `nullptr`.
	const URTActionData* Buona = URTHeroCatalogLibrary::MakeHeroActionFromCore(
		TEXT("Hero.Test.Buona"), TEXT("Action.Charge"), 2);
	if (TestNotNull(TEXT("un ID valido invece produce l'abilita'"), Buona))
	{
		TestEqual(TEXT("e dichiara la sua origine"), Buona->Def.DerivedFromActionId,
			FName(TEXT("Action.Charge")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
