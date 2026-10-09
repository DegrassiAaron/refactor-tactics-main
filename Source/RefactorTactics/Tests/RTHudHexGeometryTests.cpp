// La geometria da cui nasce ogni conversione cella → schermo della HUD: i tre ripieghi, e i tre stati.
//
// Perché esiste (#2184): `ARTHUD::DrawHUD` apriva con tre letterali — `FVector::ZeroVector`, `150.f`,
// `250.f` — che decidono **dove finisce ogni conversione** quando l'attore mappa non c'è. Non erano
// protetti da nessun condizionale, quindi nessun referto li aveva catalogati: la verifica di chiusura
// guardava i rami, e questi non sono rami.
//
// 🔴 **`Map == nullptr` significa DUE cose, e il codice le confondeva.**
// `ARTHexMapActor::GetHexContext` rende l'asset, ma scrive i tre valori **anche** quando l'asset manca:
// li prende dai campi dell'attore, che in graybox sono la fonte giusta. Gli stati sono tre:
//
//     nessun attore        →  ripieghi `(0, 150, 250)`  ·  geometria INVENTATA
//     attore senza asset   →  dai campi dell'attore      ·  geometria VERA (graybox)
//     attore con asset     →  dall'asset autorevole      ·  geometria VERA
//
// ⛔ **Ed è la ragione per cui la guardia non può essere `if (Map)`**: spegnerebbe anche il graybox,
// dove la geometria è valida. `bFromWorld` separa il solo caso rotto.
//
// ⚠️ **Il difetto che questo ha permesso di correggere non è di stile.** Il blocco della traccia
// post-lock leggeva i tre valori **senza controllare niente**: senza attore mappa la traccia non spariva
// — si disegnava alla scala di ripiego, attorno all'origine del mondo. Muto, perché `DrawHUD` non ha
// copertura headless e nessun test poteva vederlo.
//
// ── TABELLA DELLE ATTESE DI MUTAZIONE ────────────────────────────────────────────────────────────────
// Scritta prima di lanciare ed eseguibile: ogni riga nomina un test che esiste e una mutazione che
// compila, a partire dal codice spedito.
//
//   # | mutazione                                                 | rosso atteso              | esito
//  ---|------------------------------------------------------------|---------------------------|---------
//   2 | `Out.bFromWorld = true;` → `= false;`                       | GeometryFromTheActor…     | 2 rossi
//   4 | default della struct: `HexSize = 150.f` → `151.f`           | HexGeometryFallsBack…     | 1, esatto
//
// ⚠️ **La 4 colpisce la STRUCT, non la funzione**, ed è la ragione per cui esiste: i ripieghi sono
// default, e prima di questa fetta erano tre letterali dentro `DrawHUD` che nessun test pinnava. Senza
// quel caso, la scala di ogni conversione nel caso degradato poteva cambiare in silenzio.
//
// ⛔ La 2 ne uccide due — `GeometryFromTheActor…` e `GeometryComesFromTheAsset…` — perché entrambe le
// vie che passano dall'attore asseriscono il flag. L'attesa era un minimo.
//
// ⌫ La tabella ne prometteva quattro. La 1 (`if (!HexMap)` → `if (false)`) **non si esegue**: senza la
// guardia la funzione dereferenzia un puntatore nullo e il processo cade, e un crash non è un rosso —
// è una run che non misura niente. La 3 (rimuovere la riga) è la 2 scritta in un altro modo: stesso
// stato, stesso rosso. Tolte entrambe invece di lasciarle scritte e non eseguite, perché una tabella
// che promette più di quanto esegue è la stessa promessa vuota della fetta 5.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * 🔴 **Senza attore mappa si torna ai ripieghi, e lo si SA.**
 *
 * I tre valori sono quelli che `DrawHUD` portava scritti a mano; il flag è ciò che prima non esisteva,
 * e senza il quale la HUD non poteva distinguere «geometria vera» da «geometria inventata».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudHexGeometryFallbackTest,
	"RefactorTactics.HUD.HexGeometryFallsBackWithoutAnActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudHexGeometryFallbackTest::RunTest(const FString&)
{
	const FRTHudHexGeometry Geo = ARTHUD::ComposeHexGeometry(nullptr);

	TestFalse(TEXT("⛔ la geometria NON viene dal mondo, e il flag lo dice"), Geo.bFromWorld);

	// ⛔ I tre ripieghi si pinnano: erano letterali dentro `DrawHUD` e nessuno li misurava.
	TestEqual(TEXT("origine di ripiego"), Geo.Origin, FVector::ZeroVector);
	TestEqual(TEXT("dimensione di cella di ripiego"), Geo.HexSize, 150.f);
	TestEqual(TEXT("altezza di layer di ripiego"), Geo.LayerH, 250.f);
	TestNull(TEXT("e nessun asset"), Geo.Map);

	return true;
}

/**
 * 🔑 **Con l'attore la geometria è quella del mondo — anche SENZA asset.**
 *
 * È il caso graybox, e l'asserzione che conta è `bFromWorld` **vero** con `Map` **nullo**: se il codice
 * usasse `Map` come discriminante, spegnerebbe la HUD proprio qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudHexGeometryGrayboxTest,
	"RefactorTactics.HUD.GeometryFromTheActorIsMarkedEvenWithoutAnAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudHexGeometryGrayboxTest::RunTest(const FString&)
{
	// ⚠️ `NewObject`, non uno spawn: questa funzione non tocca il mondo, e il file resta headless —
	// è la stessa forma che #2182 ha già adottato per l'anteprima di pianificazione.
	ARTHexMapActor* Attore = NewObject<ARTHexMapActor>();
	if (!TestNotNull(TEXT("attore di prova"), Attore)) { return false; }

	// Nessun `MapAsset`: è il graybox, dove la geometria vive sui campi dell'attore.
	Attore->HexSize = 111.f;
	Attore->LayerHeight = 222.f;

	const FRTHudHexGeometry Geo = ARTHUD::ComposeHexGeometry(Attore);

	TestTrue(TEXT("🔑 la geometria VIENE dal mondo, benche' l'asset manchi"), Geo.bFromWorld);
	TestNull(TEXT("e l'asset e' davvero nullo: e' il caso graybox"), Geo.Map);

	// ⛔ **L'asserzione che distingue i tre stati**: i valori sono quelli dell'attore, non i ripieghi.
	// Con `Map` come discriminante questo caso sarebbe indistinguibile dal primo test.
	TestEqual(TEXT("⛔ la dimensione e' quella dell'ATTORE, non il ripiego"), Geo.HexSize, 111.f);
	TestEqual(TEXT("⛔ e cosi' l'altezza"), Geo.LayerH, 222.f);

	return true;
}

/**
 * Con l'asset la geometria è quella autorevole, e l'asset torna al chiamante.
 *
 * ⚠️ I valori sono presi dall'asset, non dai campi dell'attore: qui i due differiscono apposta, così
 * l'asserzione distingue le due sorgenti invece di essere soddisfatta da entrambe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudHexGeometryAssetTest,
	"RefactorTactics.HUD.GeometryComesFromTheAssetWhenThereIsOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudHexGeometryAssetTest::RunTest(const FString&)
{
	ARTHexMapActor* Attore = NewObject<ARTHexMapActor>();
	if (!TestNotNull(TEXT("attore di prova"), Attore)) { return false; }

	URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 2);
	if (!TestNotNull(TEXT("asset di prova"), Asset)) { return false; }

	// I campi dell'attore dicono una cosa, l'asset un'altra: è ciò che rende l'asserzione discriminante.
	Attore->HexSize = 111.f;
	Attore->LayerHeight = 222.f;
	Asset->HexSize = 333.f;
	Asset->LayerHeight = 444.f;
	Attore->MapAsset = Asset;

	const FRTHudHexGeometry Geo = ARTHUD::ComposeHexGeometry(Attore);

	TestTrue(TEXT("la geometria viene dal mondo"), Geo.bFromWorld);
	TestEqual(TEXT("e l'asset torna al chiamante"), Geo.Map, static_cast<const URTHexMapAsset*>(Asset));
	TestEqual(TEXT("⛔ la dimensione e' quella dell'ASSET, non dell'attore"), Geo.HexSize, 333.f);
	TestEqual(TEXT("⛔ e cosi' l'altezza"), Geo.LayerH, 444.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
