// Etichetta dell'unita' a schermo: la parte PURA — da dove nasce il nome mostrato.
//
// Dal 2026-08-13 (#715) la fonte primaria e' il nome CANONICO dichiarato dal catalogo (D-120), e lo Stable
// ID e' il ripiego: `ShortHeroName` non e' piu' la regola ma il fallback, e `DisplayLabel` sceglie fra i due.
//
// ⚠️ Che l'etichetta si VEDA sopra la testa resta al PIE (`PIE-NAME`, `PIE-HEXPLAY-9`): `ARTHUD::DrawHUD`
// non e' esercitato da nessun test automatico, quindi rimettere la vecchia chiamata in `RTHUD.cpp`
// lascerebbe VERDE tutto questo file. E' una lacuna dichiarata, non coperta.

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
	TestEqual(TEXT("Hero.Gadget -> Gadget"), ARTUnit::ShortHeroName(TEXT("Hero.Gadget"), Fallback), TEXT("Gadget"));
	TestEqual(TEXT("Hero.Phase -> Phase"), ARTUnit::ShortHeroName(TEXT("Hero.Phase"), Fallback), TEXT("Phase"));
	TestEqual(TEXT("Hero.Riktor -> Riktor"), ARTUnit::ShortHeroName(TEXT("Hero.Riktor"), Fallback), TEXT("Riktor"));
	TestEqual(TEXT("Hero.Wraith -> Wraith"), ARTUnit::ShortHeroName(TEXT("Hero.Wraith"), Fallback), TEXT("Wraith"));

	// Unita' legacy (archetipo, nessun eroe): l'etichetta non deve sparire.
	TestEqual(TEXT("NAME_None -> fallback"), ARTUnit::ShortHeroName(NAME_None, Fallback), Fallback);

	// Robustezza: un ID senza punto resta se stesso, non diventa vuoto.
	TestEqual(TEXT("ID senza punto -> se stesso"), ARTUnit::ShortHeroName(TEXT("Gadget"), Fallback), TEXT("Gadget"));

	// Namespace annidato: conta l'ULTIMO segmento.
	TestEqual(TEXT("A.B.C -> C"), ARTUnit::ShortHeroName(TEXT("Hero.Elite.Gadget"), Fallback), TEXT("Gadget"));

	// Punto finale senza segmento: non deve produrre una stringa vuota a schermo.
	TestEqual(TEXT("punto finale -> stringa intera, mai vuota"),
		ARTUnit::ShortHeroName(TEXT("Hero."), Fallback), TEXT("Hero."));

	return true;
}

/**
 * Nessun `HeroId` del roster porta un nome legacy (#753, D-130).
 *
 * ⚠️ **Questo test esiste perché quello sopra NON è caduto, e doveva.** Il DoD di #753 prevedeva che
 * `Unit.ShortHeroNameFromStableId` cadesse col rename, perché pinnava l'ID ritirato del primo eroe sul suo
 * ultimo segmento. Non è caduto: quel test verifica un **meccanismo** — l'ultimo segmento di un ID
 * namespaced — e i suoi ID sono esempi, quindi rinominarli insieme al resto è corretto e lo lascia verde.
 *
 * Ma un meccanismo verde non è una guardia: dopo il rename **niente** impedirebbe a un eroe nuovo, o a un
 * merge che riporta indietro un file, di reintrodurre uno degli ID ritirati. Il grep del DoD lo troverebbe
 * solo se qualcuno lo eseguisse a mano. Questo test lo trova da sé.
 *
 * Interroga il **roster reale**, non una lista scritta qui: un quinto eroe col nome sbagliato deve far
 * rosso senza che nessuno aggiorni il test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCanonicalHeroIdTest,
	"RefactorTactics.Unit.CanonicalHeroIdHasNoLegacyName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCanonicalHeroIdTest::RunTest(const FString&)
{
	// I quattro nomi che D-130 ritira. Scritti qui e non altrove: se un giorno la lista cambiasse, deve
	// cambiare in un punto che il test rilegge, non in un commento.
	// 🔴 **NON RINOMINARE QUESTA RIGA** — vale la stessa avvertenza di
	// `Heroes.AbilityIdsAreNamespacedUnderTheirHero`, e per lo stesso incidente, capitato DUE volte:
	// la prima quando il passaggio finale di #754 ha cercato i nomi ritirati senza confini di parola,
	// la seconda quando lo stesso script e' stato rilanciato *per misurare i residui* — uno script che
	// sostituisce non e' una misura, e rieseguirlo ha disfatto la riparazione appena scritta.
	const TArray<FString> Legacy = { TEXT("Flux"), TEXT("Riva"), TEXT("Bastion"), TEXT("Vektor") };

	const TArray<FName> Ids = URTHeroCatalogLibrary::GetHeroIds();
	if (!TestEqual(TEXT("il roster v0.1 dichiara quattro id"), Ids.Num(), 4)) { return false; }

	for (const FName& Id : Ids)
	{
		const FString Testo = Id.ToString();
		for (const FString& Vecchio : Legacy)
		{
			TestFalse(*FString::Printf(TEXT("%s non contiene il nome ritirato '%s'"), *Testo, *Vecchio),
				Testo.Contains(Vecchio));
		}
		// E il prefisso resta quello: la rinomina cambia il nome, non lo schema dell'identificatore.
		TestTrue(*FString::Printf(TEXT("%s resta namespaced su Hero."), *Testo),
			Testo.StartsWith(TEXT("Hero.")));
	}

	// La stessa guardia sul roster COSTRUITO, non solo sulla lista degli id: sono due copie (lo dichiara
	// `Heroes.HeroIdsMatchRoster`), e un rename applicato a una sola delle due passerebbe il ciclo sopra.
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		const FString Testo = Hero->HeroId.ToString();
		for (const FString& Vecchio : Legacy)
		{
			TestFalse(*FString::Printf(TEXT("roster: %s non contiene '%s'"), *Testo, *Vecchio),
				Testo.Contains(Vecchio));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitDisplayLabelTest,
	"RefactorTactics.Unit.DisplayLabelPrefersTheCanonicalName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitDisplayLabelTest::RunTest(const FString&)
{
	const FString Fallback = TEXT("RTUnit_0");

	// 1. Nome canonico dichiarato: vince sull'ID stabile. E' il punto di D-120 — `Hero.Gadget` si legge
	//    `Gadget`, e i due piani non convergono.
	TestEqual(TEXT("Hero.Gadget + \"Gadget\" -> Gadget"),
		ARTUnit::DisplayLabel(FText::FromString(TEXT("Gadget")), TEXT("Hero.Gadget"), Fallback), TEXT("Gadget"));

	// 2. Nome assente: ripiego sull'ultimo segmento dell'ID. Un FText vuoto e' un valore LEGALE, quindi
	//    senza questo ramo l'etichetta sparirebbe a schermo — il difetto che ShortHeroName impediva.
	TestEqual(TEXT("FText vuoto -> ultimo segmento dell'ID, mai stringa vuota"),
		ARTUnit::DisplayLabel(FText::GetEmpty(), TEXT("Hero.Gadget"), Fallback), TEXT("Gadget"));

	// Soli spazi: a schermo e' indistinguibile da assente, quindi vale come assente.
	TestEqual(TEXT("soli spazi -> ripiego, non un'etichetta invisibile"),
		ARTUnit::DisplayLabel(FText::FromString(TEXT("   ")), TEXT("Hero.Phase"), Fallback), TEXT("Phase"));

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
	// ⚠️ **La ragione di questa mappa e' CAMBIATA con #753, e va detto invece di lasciare il commento
	// vecchio.** Difendeva il vincolo di D-120 «nessuno Stable ID si rinomina»: le chiavi erano i quattro ID
	// legacy scritti a mano, e rinominarli in catalogo faceva fallire il `Find`. D-130 ha superato quel
	// vincolo e il rename e' avvenuto, quindi oggi la mappa difende un'altra cosa: che catalogo ed etichetta
	// **restino d'accordo** dopo la rinomina, cioe' che nessuno dei due sia stato aggiornato da solo.
	// Il test non copre il percorso di disegno: `ARTHUD::DrawHUD` non e' esercitato da nessun test
	// automatico, e che l'etichetta si veda davvero resta la voce `PIE-NAME`.
	const TMap<FName, FString> Attesi = {
		{ TEXT("Hero.Gadget"),    TEXT("Gadget") },
		{ TEXT("Hero.Phase"),    TEXT("Phase")  },
		{ TEXT("Hero.Riktor"), TEXT("Riktor") },
		{ TEXT("Hero.Wraith"),  TEXT("Wraith") },
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
