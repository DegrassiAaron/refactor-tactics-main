// E53 CP 53.2b (#4013): DEF, RES di default, RES per elemento e bonus alle spalle esistono su `URTHeroData` e
// viaggiano per valore su `ARTUnit`, come `GuardReduction` ([D-589], [D-595], [D-599]). TUTTI A ZERO oggi: e'
// il gate di questo passo — i campi entrano senza che un numero del gioco cambi, e questo file lo pinna da
// tre lati: il catalogo li dichiara a zero, l'unita' li copia, il validator rifiuta cio' che D-595 non ha
// deciso (valori negativi) e cio' che D-593 non conosce (un elemento fuori dai dodici).
//
// Il resolver non li legge ancora (CP 53.2c). Quando lo fara', con questi zeri la formula
// `URTCombatLibrary::MitigateDirectDamage` e' l'identita' (`Combat.Formula.ZeroDefenseIsIdentity`), ed e' la
// ragione per cui i golden di questo commit sono byte-identici.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	TArray<URTHeroData*> RosterToday()
	{
		return { URTHeroCatalogLibrary::MakeAevik(), URTHeroCatalogLibrary::MakeMuiren(),
			URTHeroCatalogLibrary::MakeBranth(), URTHeroCatalogLibrary::MakeIvrin() };
	}
}

/** Gate di CP 53.2b: ogni eroe del catalogo dichiara DEF, RES e bonus alle spalle a ZERO, e nessuna RES per elemento. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroDefenseFieldsAreZeroToday,
	"RefactorTactics.Heroes.DefenseFieldsAreZeroToday",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroDefenseFieldsAreZeroToday::RunTest(const FString&)
{
	for (const URTHeroData* Hero : RosterToday())
	{
		const FString Who = Hero->HeroId.ToString();
		TestEqual(*FString::Printf(TEXT("%s: Defense 0"), *Who), Hero->Defense, 0);
		TestEqual(*FString::Printf(TEXT("%s: ResistanceDefault 0"), *Who), Hero->ResistanceDefault, 0);
		TestEqual(*FString::Printf(TEXT("%s: nessuna RES per elemento"), *Who), Hero->ElementResistance.Num(), 0);
		TestEqual(*FString::Printf(TEXT("%s: RearBonus 0"), *Who), Hero->RearBonus, 0);
	}
	// Il roster di oggi e' valido con i campi nuovi a zero: il validator non inventa un errore dove non c'e'.
	TArray<const URTHeroData*> Const;
	for (URTHeroData* H : RosterToday()) { Const.Add(H); }
	TestEqual(TEXT("il roster valida senza errori"), URTHeroCatalogLibrary::ValidateHeroes(Const).Num(), 0);
	return true;
}

/** L'unita' copia i quattro campi per valore, come fa con `GuardReduction` (#715: `ARTUnit` e' una copia di `URTHeroData`). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitCopiesDefenseFieldsFromHeroData,
	"RefactorTactics.Unit.CopiesDefenseFieldsFromHeroData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitCopiesDefenseFieldsFromHeroData::RunTest(const FString&)
{
	URTHeroData* Hero = URTHeroCatalogLibrary::MakeAevik();
	// Valori arbitrari e distinti fra loro: una copia che scambiasse due campi non passerebbe.
	Hero->Defense = 25;
	Hero->ResistanceDefault = 40;
	Hero->ElementResistance.Add(FName(TEXT("Element.Electric")), 55);
	Hero->RearBonus = 60;

	ARTUnit* Unit = NewObject<ARTUnit>();
	Unit->ConfigureFromHeroData(Hero);

	TestEqual(TEXT("Defense copiata"), Unit->Defense, 25);
	TestEqual(TEXT("ResistanceDefault copiata"), Unit->ResistanceDefault, 40);
	TestEqual(TEXT("RearBonus copiato"), Unit->RearBonus, 60);
	TestEqual(TEXT("RES specifica per Electric"), Unit->ResistanceAgainst(FName(TEXT("Element.Electric"))), 55);
	TestEqual(TEXT("RES di default per Fire"), Unit->ResistanceAgainst(FName(TEXT("Element.Fire"))), 40);
	return true;
}

/** D-595 ha deciso solo valori non negativi: il validator rifiuta ogni campo sotto zero, nominando l'eroe. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroNegativeDefenseIsAValidationError,
	"RefactorTactics.Heroes.NegativeDefenseIsAValidationError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroNegativeDefenseIsAValidationError::RunTest(const FString&)
{
	{
		URTHeroData* H = URTHeroCatalogLibrary::MakeAevik();
		H->Defense = -1;
		TestEqual(TEXT("difesa negativa"), URTHeroCatalogLibrary::ValidateHeroes({ H }).Num(), 1);
	}
	{
		URTHeroData* H = URTHeroCatalogLibrary::MakeAevik();
		H->ResistanceDefault = -1;
		TestEqual(TEXT("resistenza di default negativa"), URTHeroCatalogLibrary::ValidateHeroes({ H }).Num(), 1);
	}
	{
		URTHeroData* H = URTHeroCatalogLibrary::MakeAevik();
		H->RearBonus = -1;
		TestEqual(TEXT("bonus alle spalle negativo"), URTHeroCatalogLibrary::ValidateHeroes({ H }).Num(), 1);
	}
	{
		URTHeroData* H = URTHeroCatalogLibrary::MakeAevik();
		H->ElementResistance.Add(FName(TEXT("Element.Frost")), -5);
		TestEqual(TEXT("resistenza negativa per un elemento"), URTHeroCatalogLibrary::ValidateHeroes({ H }).Num(), 1);
	}
	return true;
}

/** D-593: una RES per un elemento che non e' fra i dodici e' un errore; una fra i dodici no. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroUnknownElementResistanceIsAValidationError,
	"RefactorTactics.Heroes.UnknownElementResistanceIsAValidationError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroUnknownElementResistanceIsAValidationError::RunTest(const FString&)
{
	TestEqual(TEXT("i dodici sono dodici"), URTHeroCatalogLibrary::ElementIds().Num(), 12);
	TestTrue(TEXT("Element.Warp e' noto"), URTHeroCatalogLibrary::IsKnownElementId(FName(TEXT("Element.Warp"))));
	// I tre usciti con D-588 non sono piu' elementi; `Shield` e `Ice` non lo sono mai stati come ID (D-593).
	for (const TCHAR* Gone : { TEXT("Element.Void"), TEXT("Element.Harmonic"), TEXT("Element.Acid"), TEXT("Element.Shield"), TEXT("Element.Ice"), TEXT("Element.Flux") })
	{
		TestFalse(*FString::Printf(TEXT("%s non e' un elemento"), Gone), URTHeroCatalogLibrary::IsKnownElementId(FName(Gone)));
	}
	{
		URTHeroData* H = URTHeroCatalogLibrary::MakeAevik();
		H->ElementResistance.Add(FName(TEXT("Element.Harmonic")), 10);
		TestEqual(TEXT("elemento sconosciuto"), URTHeroCatalogLibrary::ValidateHeroes({ H }).Num(), 1);
	}
	{
		URTHeroData* H = URTHeroCatalogLibrary::MakeAevik();
		H->ElementResistance.Add(FName(TEXT("Element.Decay")), 10);
		TestEqual(TEXT("elemento noto, valore valido"), URTHeroCatalogLibrary::ValidateHeroes({ H }).Num(), 0);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
