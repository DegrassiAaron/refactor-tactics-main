// Etichetta dell'unita' a schermo: la parte PURA (da quale ID nasce il nome mostrato).
// Che l'etichetta si VEDA sopra la testa resta al PIE (`PIE-HEXPLAY-9`); qui si verifica che la
// presentazione non si inventi il nome ma lo derivi dall'ID stabile dell'eroe.

#include "Misc/AutomationTest.h"

#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitShortHeroNameTest,
	"RefactorTactics.Unit.ShortHeroNameFromStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitShortHeroNameTest::RunTest(const FString&)
{
	const FString Fallback = TEXT("RTUnit_0");

	// Il caso reale: gli HeroId del catalogo sono namespaced.
	TestEqual(TEXT("Hero.Flux -> Flux"), ARTUnit::ShortHeroName(TEXT("Hero.Flux"), Fallback), TEXT("Flux"));
	TestEqual(TEXT("Hero.Riva -> Riva"), ARTUnit::ShortHeroName(TEXT("Hero.Riva"), Fallback), TEXT("Riva"));
	TestEqual(TEXT("Hero.Bastion -> Bastion"), ARTUnit::ShortHeroName(TEXT("Hero.Bastion"), Fallback), TEXT("Bastion"));
	TestEqual(TEXT("Hero.Vektor -> Vektor"), ARTUnit::ShortHeroName(TEXT("Hero.Vektor"), Fallback), TEXT("Vektor"));

	// Unita' legacy (archetipo, nessun eroe): l'etichetta non deve sparire.
	TestEqual(TEXT("NAME_None -> fallback"), ARTUnit::ShortHeroName(NAME_None, Fallback), Fallback);

	// Robustezza: un ID senza punto resta se stesso, non diventa vuoto.
	TestEqual(TEXT("ID senza punto -> se stesso"), ARTUnit::ShortHeroName(TEXT("Flux"), Fallback), TEXT("Flux"));

	// Namespace annidato: conta l'ULTIMO segmento.
	TestEqual(TEXT("A.B.C -> C"), ARTUnit::ShortHeroName(TEXT("Hero.Elite.Flux"), Fallback), TEXT("Flux"));

	// Punto finale senza segmento: non deve produrre una stringa vuota a schermo.
	TestEqual(TEXT("punto finale -> stringa intera, mai vuota"),
		ARTUnit::ShortHeroName(TEXT("Hero."), Fallback), TEXT("Hero."));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitDisplayLabelTest,
	"RefactorTactics.Unit.DisplayLabelPrefersTheCanonicalName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitDisplayLabelTest::RunTest(const FString&)
{
	const FString Fallback = TEXT("RTUnit_0");

	// 1. Nome canonico dichiarato: vince sull'ID stabile. E' il punto di D-120 — `Hero.Flux` si legge
	//    `Gadget`, e i due piani non convergono.
	TestEqual(TEXT("Hero.Flux + \"Gadget\" -> Gadget"),
		ARTUnit::DisplayLabel(FText::FromString(TEXT("Gadget")), TEXT("Hero.Flux"), Fallback), TEXT("Gadget"));

	// 2. Nome assente: ripiego sull'ultimo segmento dell'ID. Un FText vuoto e' un valore LEGALE, quindi
	//    senza questo ramo l'etichetta sparirebbe a schermo — il difetto che ShortHeroName impediva.
	TestEqual(TEXT("FText vuoto -> ultimo segmento dell'ID, mai stringa vuota"),
		ARTUnit::DisplayLabel(FText::GetEmpty(), TEXT("Hero.Flux"), Fallback), TEXT("Flux"));

	// Soli spazi: a schermo e' indistinguibile da assente, quindi vale come assente.
	TestEqual(TEXT("soli spazi -> ripiego, non un'etichetta invisibile"),
		ARTUnit::DisplayLabel(FText::FromString(TEXT("   ")), TEXT("Hero.Riva"), Fallback), TEXT("Riva"));

	// 3. Ne' nome ne' eroe: resta il nome dell'attore. L'etichetta non sparisce mai.
	TestEqual(TEXT("nessun nome e nessun eroe -> fallback"),
		ARTUnit::DisplayLabel(FText::GetEmpty(), NAME_None, Fallback), Fallback);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRosterCanonicalNamesTest,
	"RefactorTactics.Heroes.CanonicalNamesReachTheLabel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRosterCanonicalNamesTest::RunTest(const FString&)
{
	// I quattro nomi di D-120, pinnati sul percorso REALE: catalogo -> ConfigureFromHeroData -> etichetta.
	// Verificarli sul solo catalogo direbbe meno: e' il trasporto che si era rotto, non la dichiarazione.
	//
	// ⚠️ **E' questa mappa a difendere il vincolo di D-120 «nessuno Stable ID si rinomina»**: le chiavi sono
	// i quattro ID LEGACY scritti a mano, quindi rinominare `Hero.Flux` in catalogo fa fallire il `Find` qui
	// sotto. Il test non copre il percorso di disegno: `ARTHUD::DrawHUD` non e' esercitato da nessun test
	// automatico, e che l'etichetta si veda davvero resta la voce `PIE-NAME`.
	const TMap<FName, FString> Attesi = {
		{ TEXT("Hero.Flux"),    TEXT("Gadget") },
		{ TEXT("Hero.Riva"),    TEXT("Phase")  },
		{ TEXT("Hero.Bastion"), TEXT("Riktor") },
		{ TEXT("Hero.Vektor"),  TEXT("Wraith") },
	};

	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	TestEqual(TEXT("il roster v0.1 ha quattro eroi"), Roster.Num(), 4);

	for (const URTHeroData* Hero : Roster)
	{
		if (!Hero)
		{
			AddError(TEXT("il roster contiene un eroe nullo"));
			continue;
		}

		const FString* Atteso = Attesi.Find(Hero->HeroId);
		if (!Atteso)
		{
			AddError(FString::Printf(TEXT("eroe inatteso nel roster: %s"), *Hero->HeroId.ToString()));
			continue;
		}

		ARTUnit* Unit = NewObject<ARTUnit>();
		Unit->ConfigureFromHeroData(Hero);

		TestEqual(*FString::Printf(TEXT("%s mostra il nome canonico"), *Hero->HeroId.ToString()),
			ARTUnit::DisplayLabel(Unit->HeroDisplayName, Unit->HeroId, Unit->GetName()), *Atteso);

		// Fedelta' della copia: l'ID che l'unita' porta e' quello del catalogo. ⚠️ Questa riga da sola NON
		// difende il vincolo di D-120 — passerebbe anche se il catalogo avesse rinominato l'ID. A difenderlo
		// sono le chiavi letterali della mappa qui sopra.
		TestEqual(TEXT("l'unita' porta lo Stable ID del catalogo"), Unit->HeroId, Hero->HeroId);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
