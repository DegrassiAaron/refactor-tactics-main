#include "Misc/AutomationTest.h"
#include "UObject/UObjectIterator.h"

#include "ContextObjectStore.h"
#include "InteractiveTool.h"          // UInteractiveToolPropertySet
#include "RTHexEditorClick.h"         // RTHexEditor::ShouldShowSurfaceOverlay
#include "RTHexEditorMode.h"
#include "RTHexEditorModeSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * L'overlay delle superfici e' uno stato del MODE, non di un tool (#921).
 *
 * 🔴 **L'ordine di questi test non e' estetico, ed e' la correzione piu' importante della issue.** La prima
 * stesura di #921 aveva **un solo** criterio automatico — *«nessun property set dichiara `bShowOverlay`»* —
 * ed era soddisfacibile dal commit sbagliato: cancellare le due dichiarazioni e non fare altro lo rendeva
 * verde, con l'overlay **spento ovunque** e il prodotto peggiore di prima. Un divieto senza un obbligo
 * speculare e' un test che premia la rimozione. Percio' l'obbligo positivo — il flag esiste come stato del
 * mode e i tool lo raggiungono — sta **prima**, e il divieto arriva dopo a chiudere la porta.
 *
 * ⚠️ **E il divieto e' sulla FORMA, non sul nome.** Cercare la stringa `bShowOverlay` avrebbe lasciato
 * passare un tool che chiamasse il proprio flag `bDrawOverlay`: si itera la reflection, come fa gia'
 * `RTHexToolPropertiesTests.cpp` per #871, invece di trascrivere l'elenco degli strumenti.
 */

/**
 * **AC 1** — il flag esiste come stato del mode, e la classe di settings e' quella che il mode nomina.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexOverlayIsModeSettingTest,
	"RefactorTactics.HexEditor.OverlayIsModeSetting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexOverlayIsModeSettingTest::RunTest(const FString&)
{
	const URTHexEditorMode* Mode = GetDefault<URTHexEditorMode>();
	if (!TestNotNull(TEXT("il CDO del mode esiste"), Mode))
	{
		return false;
	}

	// ⚠️ **`UEdMode::SettingsClass` e' `protected`, quindi si legge per reflection e non col punto.** E' una
	// `UPROPERTY()` (`UEdMode.h:310-311`), quindi la via c'e' ed e' la stessa che `RTHexToolPropertiesTests`
	// usa gia' per i property set. ⛔ L'alternativa — aprire un accessor pubblico sul mode — sarebbe codice di
	// produzione aggiunto per compiacere un test, e nasconderebbe che il campo e' protetto apposta.
	FProperty* SettingsClassProp =
		URTHexEditorMode::StaticClass()->FindPropertyByName(TEXT("SettingsClass"));
	if (!TestNotNull(TEXT("UEdMode::SettingsClass e' raggiungibile per reflection"), SettingsClassProp))
	{
		return false;
	}

	const TSoftClassPtr<UObject>& Dichiarata =
		*SettingsClassProp->ContainerPtrToValuePtr<TSoftClassPtr<UObject>>(Mode);

	// Si confronta la classe RISOLTA, non la stringa del path: un path scritto a mano nel test
	// invecchierebbe al primo rename, e in silenzio.
	TestEqual(TEXT("URTHexEditorMode::SettingsClass nomina URTHexEditorModeSettings"),
		Dichiarata.Get(), URTHexEditorModeSettings::StaticClass());

	return true;
}

/**
 * **AC 1, seconda meta'** — il flag e' raggiungibile *per la via che un `Render` usera'*.
 *
 * 🔑 **E' l'anello che `UEdMode` non fornisce**, ed e' la ragione per cui questo test esiste separato dal
 * precedente. `UEdMode::Enter` crea il settings object e lo passa al toolkit, ma **non lo registra da
 * nessuna parte che un tool possa interrogare**: un `UInteractiveTool` vede il proprio `GetToolManager()`,
 * non il `UEdMode` che l'ha costruito. Qui si esercita il context store vero — lo stesso tipo che il tool
 * manager espone — invece di leggere il campo del mode, che proverebbe un'altra cosa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexOverlayReachableFromContextStoreTest,
	"RefactorTactics.HexEditor.OverlayReachableFromContextStore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexOverlayReachableFromContextStoreTest::RunTest(const FString&)
{
	UContextObjectStore* Store = NewObject<UContextObjectStore>(GetTransientPackage());
	if (!TestNotNull(TEXT("context store costruito"), Store))
	{
		return false;
	}

	// AC 8, ramo «assente»: uno store senza il settings non accende niente. Si misura PRIMA di aggiungerlo,
	// cosi' il valore di partenza non e' un'ipotesi.
	TestFalse(TEXT("store senza settings: overlay spento"),
		RTHexEditor::ShouldShowSurfaceOverlay(Store));

	URTHexEditorModeSettings* Settings = NewObject<URTHexEditorModeSettings>(GetTransientPackage());
	Settings->bShowSurfaceOverlay = true;
	TestTrue(TEXT("il settings entra nel context store"), Store->AddContextObject(Settings));

	TestNotNull(TEXT("FindContext restituisce il settings — la via che un Render usera'"),
		Store->FindContext<URTHexEditorModeSettings>());

	TestTrue(TEXT("acceso nel settings: l'helper risponde acceso"),
		RTHexEditor::ShouldShowSurfaceOverlay(Store));

	// ⚠️ **Spento deve dire spento**, e non e' ridondante: un helper che tornasse `true` per la sola presenza
	// del settings passerebbe l'asserzione qui sopra e romperebbe l'AC 6 (spegnendolo, nessun tool disegna).
	Settings->bShowSurfaceOverlay = false;
	TestFalse(TEXT("spento nel settings: l'helper risponde spento"),
		RTHexEditor::ShouldShowSurfaceOverlay(Store));

	return true;
}

/**
 * Chi PUBBLICA e chi LEGGE parlano dello stesso oggetto — l'accoppiamento fra le due meta' dell'AC 1.
 *
 * 🔑 **Perche' questo test esiste separato dal precedente.** Quello mette il settings nello store *con le
 * proprie mani* e verifica il lettore; questo esercita la funzione che il **mode** chiama davvero,
 * `PublishSurfaceOverlaySettings`, e verifica che cio' che pubblica sia esattamente cio' che
 * `ShouldShowSurfaceOverlay` ritrova. Sono sedi lontane — `Enter()` da una parte, sette `Render` dall'altra —
 * e nulla nel compilatore obbliga il tipo pubblicato a essere quello cercato.
 *
 * ⛔ **Cio' che questo test NON copre, dichiarato invece che lasciato scoprire**: che
 * `URTHexEditorMode::Enter()` *chiami* la pubblicazione. Cancellando quella riga l'overlay resterebbe spento
 * in tutti gli strumenti e questa suite resterebbe verde. Coprirlo richiederebbe di costruire un `UEdMode`
 * con il suo `FEditorModeTools`, che nessun test di questo modulo fa. Quel passaggio e' a carico di
 * `PIE-HEX-MODE-Q`, ed e' il limite noto della fetta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexOverlayPublishAndReadAgreeTest,
	"RefactorTactics.HexEditor.OverlayPublishAndReadAgree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexOverlayPublishAndReadAgreeTest::RunTest(const FString&)
{
	UContextObjectStore* Store = NewObject<UContextObjectStore>(GetTransientPackage());
	if (!TestNotNull(TEXT("context store costruito"), Store))
	{
		return false;
	}

	// 🔴 Il tipo SBAGLIATO viene rifiutato, e non pubblicato in silenzio. `UEdMode::SettingsObject` e' un
	// `UObject*`: se `SettingsClass` nominasse un'altra classe, senza questo rifiuto il mode pubblicherebbe
	// «con successo» un oggetto che nessun `Render` sa leggere, e l'overlay sarebbe morto senza un sintomo.
	// ⚠️ **Non `NewObject<UObject>`**: `UObject` e' `abstract`, e costruirlo produce un errore di costruzione
	// invece di un oggetto estraneo. Serve una classe concreta qualunque che non sia il settings — qui lo
	// store stesso, che e' gia' incluso e non puo' essere confuso col tipo cercato.
	UObject* Estraneo = NewObject<UContextObjectStore>(GetTransientPackage());
	TestFalse(TEXT("un oggetto del tipo sbagliato NON viene pubblicato"),
		RTHexEditor::PublishSurfaceOverlaySettings(Store, Estraneo));
	TestFalse(TEXT("e dopo quel rifiuto l'overlay resta spento"),
		RTHexEditor::ShouldShowSurfaceOverlay(Store));

	TestFalse(TEXT("store nullo: la pubblicazione fallisce invece di asserire"),
		RTHexEditor::PublishSurfaceOverlaySettings(nullptr, Estraneo));

	// La via vera: si pubblica cio' che il mode pubblica, e lo si rilegge come lo rilegge un `Render`.
	URTHexEditorModeSettings* Settings = NewObject<URTHexEditorModeSettings>(GetTransientPackage());
	Settings->bShowSurfaceOverlay = true;
	TestTrue(TEXT("il settings del mode viene pubblicato"),
		RTHexEditor::PublishSurfaceOverlaySettings(Store, Settings));
	TestTrue(TEXT("e il lettore lo ritrova: pubblicazione e lettura concordano"),
		RTHexEditor::ShouldShowSurfaceOverlay(Store));

	// E ritirandolo l'overlay si spegne: e' cio' che `Exit()` fa, e senza il ritiro uno store riusato
	// risponderebbe ancora acceso.
	RTHexEditor::WithdrawSurfaceOverlaySettings(Store, Settings);
	TestFalse(TEXT("ritirato il settings, l'overlay torna spento"),
		RTHexEditor::ShouldShowSurfaceOverlay(Store));

	return true;
}

/**
 * **AC 8** — cosa fa un tool quando il settings non c'e', dichiarato una volta invece che sette.
 *
 * ⛔ Assente significa **spento**: un overlay che comparisse da solo dove il mode non e' entrato
 * disegnerebbe sopra il lavoro di chi non l'ha chiesto. L'errore reversibile e' quello opposto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexOverlayDefaultsOffWithoutSettingsTest,
	"RefactorTactics.HexEditor.OverlayDefaultsOffWithoutSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexOverlayDefaultsOffWithoutSettingsTest::RunTest(const FString&)
{
	TestFalse(TEXT("store nullo: spento, e nessun crash"),
		RTHexEditor::ShouldShowSurfaceOverlay(static_cast<const UContextObjectStore*>(nullptr)));

	TestFalse(TEXT("tool manager nullo: spento, e nessun crash"),
		RTHexEditor::ShouldShowSurfaceOverlay(static_cast<const UInteractiveToolManager*>(nullptr)));

	// ⛔ **Qui NON si asserisce che il CDO abbia il flag spento, e la ragione e' l'AC 2 stesso.**
	// `bShowSurfaceOverlay` e' `UPROPERTY(config)`, e `UEdMode::Exit()` chiama `SettingsObject->SaveConfig()`
	// coi parametri di default: `bAllowCopyToDefaultObject` vale `true`, quindi `UObject::SaveConfig` copia il
	// valore dell'ISTANZA **dentro il CDO** e lo scrive in `EditorPerProjectUserSettings.ini`; a ogni processo
	// successivo il CDO rilegge quell'ini alla costruzione. ∴ chi esegue `PIE-HEX-MODE-Q` — che ha come
	// precondizione «overlay **acceso**» — lascerebbe `true` nel CDO di quel clone, e un'asserzione sul CDO
	// diventerebbe rossa **per una preferenza utente**, indicando il codice mentre la causa e' un `.ini`.
	// Un gate che misura la configurazione locale invece del codice e' peggio di un gate assente: manda a
	// cercare nel posto sbagliato. Il default dichiarato in C++ resta pinnato dal compilatore.

	return true;
}

/**
 * **AC 2** — il flag e' `config`, cioe' sopravvive alla chiusura dell'editor.
 *
 * 🔑 **E' l'unica meta' verificabile a macchina della persistenza.** `UEdMode::Enter` fa `LoadConfig()` e
 * `UEdMode::Exit` fa `SaveConfig()`: senza `CPF_Config` quelle due chiamate non trasportano niente, e il
 * difetto sarebbe invisibile finche' qualcuno non riapre l'editor. L'altra meta' — che il valore ricompaia
 * davvero — la vede solo un umano, ed e' `PIE-HEX-MODE-Q`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexOverlaySettingIsConfigTest,
	"RefactorTactics.HexEditor.OverlaySettingIsConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexOverlaySettingIsConfigTest::RunTest(const FString&)
{
	UClass* SettingsClass = URTHexEditorModeSettings::StaticClass();

	FProperty* Flag = SettingsClass->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(URTHexEditorModeSettings, bShowSurfaceOverlay));
	if (!TestNotNull(TEXT("la property bShowSurfaceOverlay esiste"), Flag))
	{
		return false;
	}

	TestTrue(TEXT("la property ha CPF_Config"), Flag->HasAnyPropertyFlags(CPF_Config));
	TestTrue(TEXT("la property e' editabile dal pannello del mode"), Flag->HasAnyPropertyFlags(CPF_Edit));

	// ⚠️ `UPROPERTY(config)` da solo non basta: senza `UCLASS(config = ...)` la classe non ha un file dove
	// scrivere, e `SaveConfig()` non fallisce — non fa niente. E' il modo in cui l'AC 2 potrebbe restare
	// verde mentre l'AC 7 (il flag sopravvive fra sessioni) resta rotto.
	TestTrue(TEXT("la classe dichiara un config file (UCLASS(config=...))"),
		SettingsClass->ClassConfigName != NAME_None);

	return true;
}

/**
 * **AC 3** — nessun property set del mode ridichiara l'overlay. Il divieto, **dopo** l'obbligo.
 *
 * ⚠️ La regola e' sul nome della property *e* sul suo tipo, non sulla stringa `bShowOverlay`: un tool che
 * chiamasse il proprio flag `bDrawOverlay` o `bOverlayVisible` violerebbe #921 identicamente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexNoToolRedeclaresOverlayTest,
	"RefactorTactics.HexEditor.NoToolRedeclaresOverlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexNoToolRedeclaresOverlayTest::RunTest(const FString&)
{
	int32 Esaminate = 0;

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class->IsChildOf(UInteractiveToolPropertySet::StaticClass())
			|| Class->HasAnyClassFlags(CLASS_Abstract)
			|| !Class->GetName().StartsWith(TEXT("RTHex")))
		{
			continue;
		}

		++Esaminate;

		// Solo le property DICHIARATE dalla classe: `EFieldIterationFlags::None` esclude le ereditate, che
		// altrimenti farebbero contare piu' volte una property della base.
		for (TFieldIterator<FBoolProperty> PropIt(Class, EFieldIterationFlags::None); PropIt; ++PropIt)
		{
			const FString Nome = PropIt->GetName();
			TestFalse(
				*FString::Printf(
					TEXT("%s non ridichiara l'overlay: la property '%s' contiene 'Overlay'. ")
					TEXT("Il flag e' URTHexEditorModeSettings::bShowSurfaceOverlay (#921)."),
					*Class->GetName(), *Nome),
				Nome.Contains(TEXT("Overlay")));
		}
	}

	// Se il filtro smettesse di riconoscere i property set — un rename del prefisso, una gerarchia diversa —
	// il ciclo girerebbe a vuoto e il divieto passerebbe senza aver guardato niente. I property set del mode
	// sono sette: la soglia sta sotto, perche' e' una guardia contro lo zero, non un conteggio da mantenere.
	TestTrue(TEXT("almeno due property set RTHex esaminati"), Esaminate >= 2);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
