// L'ANDATURA dello slot movimento, e cio' che il pannello UMG ne legge (D-425).
//
// Qui stava anche la composizione della terna movimento / principale / reazione per il Canvas
// (`ComposeSlotLines`), ritirata con D-538 (#2764): il Canvas era uscito con #1936 e il pannello UMG si riempie
// da solo (`URTSelectedUnitPanelWidget::RefreshLook`), con i propri test (`ScreenHud.PanelChipsAndWarningsFollowThePlan`).
// Restano i test della regola che il pannello USA: l'andatura (`DescribeMovementSlot`, `DescribeMovementProfile`).

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "UI/RTHudViewModel.h"
#include "Ability/RTMovementProfileLibrary.h" // gli id dei profili, mai scritti a mano

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	FRTPlannedSlotView MakeSlotLineFixture(bool bOccupied, const TCHAR* DisplayName = nullptr)
	{
		FRTPlannedSlotView Slot;
		Slot.bOccupied = bOccupied;
		if (DisplayName)
		{
			Slot.ActionId = FName(DisplayName);
			Slot.DisplayName = FText::FromString(DisplayName);
		}
		return Slot;
	}
}

/**
 * `#1410` `AC-1`: l'ANDATURA si legge nel contenuto dello slot movimento, e **solo quando devia dal cammino**.
 *
 * La regola di [D-425] resa in testo, con la sua asimmetria: `Move` e `Still` NON si nominano perche' sono
 * il caso normale; `Sprint` e `Sneak` aggettivano il percorso; `Withdraw` lo SOSTITUISCE, perche' il
 * ripiegamento non e' un cammino svolto in un certo modo.
 *
 * Gli id non si scrivono a mano: vengono dalla libreria, o il test si romperebbe in silenzio il giorno in
 * cui un profilo cambia nome — e resterebbe verde asserendo su una stringa che non esiste piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDMovementProfileReadsOnTheLineTest,
	"RefactorTactics.HUD.MovementProfileReadsOnTheMovementLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDMovementProfileReadsOnTheLineTest::RunTest(const FString&)
{
	using Lib = URTMovementProfileLibrary;

	auto RigaMovimento = [](FName Profilo)
	{
		FRTUnitSlotsView Slots;
		Slots.Movement = MakeSlotLineFixture(true);
		Slots.MovementProfileId = Profilo;
		return ARTHUD::DescribeMovementSlot(Slots).ToString();
	};

	// --- il cammino non si nomina, ed e' la meta' che conta -------------------------------------------
	const FString Cammino = RigaMovimento(Lib::ProfileMove);
	TestTrue(TEXT("il Move resta «percorso»"), Cammino.Contains(TEXT("percorso")));
	TestFalse(TEXT("e non guadagna nessun aggettivo"), Cammino.Contains(TEXT(",")));
	TestEqual(TEXT("un piano senza profilo dichiarato legge come il Move"),
		RigaMovimento(NAME_None), Cammino);
	TestEqual(TEXT("e il fermo pure: non e' un'andatura diversa"),
		RigaMovimento(Lib::ProfileStill), Cammino);

	// --- le due andature che deviano ------------------------------------------------------------------
	const FString Corsa = RigaMovimento(Lib::ProfileSprint);
	TestTrue(TEXT("lo Sprint si legge «di corsa»"), Corsa.Contains(TEXT("di corsa")));
	TestTrue(TEXT("e resta un percorso"), Corsa.Contains(TEXT("percorso")));

	const FString Furtivo = RigaMovimento(Lib::ProfileSneak);
	TestTrue(TEXT("lo Sneak si legge «furtivo»"), Furtivo.Contains(TEXT("furtivo")));
	TestTrue(TEXT("e resta un percorso"), Furtivo.Contains(TEXT("percorso")));

	// ⛔ **Il ripiegamento SOSTITUISCE il percorso, non lo aggettiva.** Senza questa riga negativa la
	// funzione potrebbe concatenare come le altre due e il test resterebbe verde: «percorso, ripiegamento»
	// direbbe che l'`Overwatch` impone un modo di camminare invece di un'altra cosa.
	const FString Ripiego = RigaMovimento(Lib::ProfileWithdraw);
	TestTrue(TEXT("il Withdraw si legge «ripiegamento»"), Ripiego.Contains(TEXT("ripiegamento")));
	TestFalse(TEXT("e NON e' un percorso"), Ripiego.Contains(TEXT("percorso")));

	// --- il NOME dell'azione vince sull'andatura -------------------------------------------------------
	// Una mobilita' che occupa lo slot si chiama col proprio nome: l'andatura parla del movimento SENZA
	// nome, che e' il caso di cui D-425 si occupa. Senza questa riga, un'andatura che scavalcasse il nome
	// non si vedrebbe da nessun altro assert.
	FRTUnitSlotsView ConNome;
	ConNome.Movement = MakeSlotLineFixture(true, TEXT("Scatto"));
	ConNome.MovementProfileId = Lib::ProfileSprint;
	const FString RigaConNome = ARTHUD::DescribeMovementSlot(ConNome).ToString();
	TestTrue(TEXT("l'azione che occupa lo slot si chiama col proprio nome"),
		RigaConNome.Contains(TEXT("Scatto")));
	TestFalse(TEXT("e l'andatura non lo scavalca"), RigaConNome.Contains(TEXT("di corsa")));

	// --- un id sconosciuto non finisce a schermo -------------------------------------------------------
	// ⛔ Un `FName` grezzo davanti al giocatore sarebbe un difetto di catalogo trasformato in testo. Ricade
	// sul cammino, e il posto dove quel difetto deve farsi vedere e' il test del catalogo.
	const FString Ignoto = RigaMovimento(FName(TEXT("MovementProfile.NonEsiste")));
	TestEqual(TEXT("un profilo sconosciuto legge come il cammino"), Ignoto, Cammino);
	TestFalse(TEXT("e non stampa il proprio id"), Ignoto.Contains(TEXT("NonEsiste")));

	return true;
}

/**
 * IL CANCELLO CHE MANCAVA: ogni profilo del catalogo ha una resa, o e' dichiarato liscio.
 *
 * `MovementProfileReadsOnTheMovementLine` enumera i cinque profili **a mano**, quindi un sesto sarebbe
 * entrato a catalogo e si sarebbe letto «percorso» liscio senza che niente fallisse. Il commento di
 * `DescribeMovementProfile` dichiarava che il caso lo copriva *«il test del catalogo»*:
 * `MovementProfile.CatalogDeclaresTheProfiles` verifica `Sprint` e `Withdraw` **per nome** e non itera,
 * quindi quella frase era falsa. Trovata da una review esterna il 2026-09-16.
 *
 * 🔑 **Elencare cio' che e' VOLUTAMENTE liscio, invece di cio' che deve avere una resa.** Un profilo nuovo
 * nasce cosi' nella meta' che fallisce, e chi lo vuole silenzioso deve passare da qui e dirlo. La lista
 * opposta — «questi devono avere un testo» — lascerebbe il nuovo fuori da entrambe le meta', cioe' nel
 * silenzio che questo test esiste per togliere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDEveryCatalogProfileHasAReadingTest,
	"RefactorTactics.HUD.EveryCatalogProfileHasAReading",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDEveryCatalogProfileHasAReadingTest::RunTest(const FString&)
{
	using Lib = URTMovementProfileLibrary;

	// I due che non si nominano, e il PERCHE' sta accanto al nome: il cammino e' il caso normale, il fermo
	// lo dice gia' «Movimento: libero». Chi aggiunge un terzo qui deve poter scrivere la stessa riga.
	const TArray<FName> Lisci = { Lib::ProfileMove, Lib::ProfileStill };

	const TArray<FRTMovementProfile> Catalogo = Lib::GetCoreMovementProfileCatalog();
	if (!TestTrue(TEXT("premessa: il catalogo dei profili non e' vuoto"), Catalogo.Num() > 0))
	{
		return false;
	}

	int32 ConResa = 0;
	for (const FRTMovementProfile& Profilo : Catalogo)
	{
		const FText Resa = ARTHUD::DescribeMovementProfile(Profilo.Id);
		if (Lisci.Contains(Profilo.Id))
		{
			TestTrue(*FString::Printf(TEXT("%s e' dichiarato liscio: la resa resta vuota"),
					*Profilo.Id.ToString()),
				Resa.IsEmpty());
		}
		else
		{
			// 🔴 Il cuore: un profilo a catalogo che nessuno ha dichiarato liscio DEVE avere una resa.
			//
			// ⚠️ **Il messaggio nomina la CONDIZIONE, non l'esito voluto**, e su un `TestFalse` la
			// differenza si vede solo nel log: la macro stampa *«Expected '<msg>' to be false»*, quindi un
			// messaggio scritto *«X ha una resa»* produce *«Expected 'X ha una resa' to be false»* — cioe'
			// l'opposto del punto, davanti a chi sta cercando di capire cosa ha rotto.
			//
			// ⏱️ *Era scritto cosi' fino al 2026-09-16*, e l'ha visto una review esterna leggendo il log
			// della mutazione invece del sorgente. 🔑 Qui pesa piu' del solito: il valore di questo test e'
			// per intero la frase che legge il prossimo — la stessa ragione per cui la riga di
			// `DescribeMovementProfile` era dannosa invece che soltanto inutile.
			TestFalse(*FString::Printf(
					TEXT("la resa di %s e' vuota: se e' nuovo, aggiungila in DescribeMovementProfile o "
						 "dichiaralo liscio qui"), *Profilo.Id.ToString()),
				Resa.IsEmpty());
			++ConResa;
		}
	}

	// ⛔ **ANTI-VACUITA'.** Senza questa riga il test resterebbe verde su un catalogo fatto di soli profili
	// lisci — cioe' nel mondo in cui il ramo che conta non viene mai eseguito. E' lo stesso difetto che una
	// verifica di mutazione ha trovato oggi su `SprintRefusedWhileUnbalanced`, in un altro file.
	TestTrue(TEXT("e il ramo che conta e' stato davvero percorso"), ConResa > 0);

	// ⛔ **E l'elenco dei lisci non nomina profili morti.** Un `Id` rinominato lascerebbe qui una voce che
	// non esclude piu' niente, e il profilo rinominato cadrebbe nel ramo severo senza che si capisca
	// perche' — oppure, peggio, qualcuno la ri-aggiungerebbe allargando il silenzio.
	for (const FName& Liscio : Lisci)
	{
		TestTrue(*FString::Printf(TEXT("%s, dichiarato liscio, esiste ancora a catalogo"),
				*Liscio.ToString()),
			Catalogo.ContainsByPredicate(
				[&Liscio](const FRTMovementProfile& P) { return P.Id == Liscio; }));
	}

	return true;
}

/**
 * LA SEDE E' UNA: `DescribeMovementSlot` rende il contenuto dello slot movimento per OGNI andatura.
 *
 * Il difetto che questo test chiudeva esisteva prima di [D-425]: il pannello `WBP_RT_SelectedUnitPanel` legava
 * `GetSlots()` e mostrava il `DisplayName` grezzo mentre il Canvas componeva la riga, due rese dello stesso
 * fatto gia' divergenti. Il Canvas e' uscito (#1936) e con lui `ComposeSlotLines` (D-538): la sede unica ha
 * adesso un solo lettore, il pannello UMG (`RefreshLook`), e il suo test lo confronta con questa funzione
 * (`ScreenHud.PanelChipsAndWarningsFollowThePlan`, caso 2b). Qui si pinna la sede in se': mai vuota, il
 * NOME dell'azione che vince sull'andatura, e rese DIVERSE per casi diversi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDMovementSlotSingleSeatTest,
	"RefactorTactics.HUD.MovementSlotRendersFromTheSingleSeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDMovementSlotSingleSeatTest::RunTest(const FString&)
{
	using Lib = URTMovementProfileLibrary;

	auto Verifica = [this](const TCHAR* Caso, const FRTUnitSlotsView& Slots)
	{
		const FString Sede = ARTHUD::DescribeMovementSlot(Slots).ToString();
		TestFalse(*FString::Printf(TEXT("%s: la sede unica non e' vuota"), Caso), Sede.IsEmpty());
	};

	// Slot libero.
	FRTUnitSlotsView Libero;
	Verifica(TEXT("libero"), Libero);

	// Occupato SENZA nome: e' il caso di cui [D-425] si occupa, e va provato su ogni andatura.
	for (const FName& Profilo : { Lib::ProfileMove, Lib::ProfileSprint, Lib::ProfileSneak,
			Lib::ProfileWithdraw, Lib::ProfileStill })
	{
		FRTUnitSlotsView Slots;
		Slots.Movement = MakeSlotLineFixture(true);
		Slots.MovementProfileId = Profilo;
		Verifica(*FString::Printf(TEXT("percorso, %s"), *Profilo.ToString()), Slots);
	}

	// Occupato CON nome: l'azione vince, e la sede deve dirlo — non l'andatura.
	FRTUnitSlotsView ConNome;
	ConNome.Movement = MakeSlotLineFixture(true, TEXT("Scatto"));
	ConNome.MovementProfileId = Lib::ProfileSprint;
	Verifica(TEXT("con nome"), ConNome);
	TestEqual(TEXT("e la sede rende il NOME dell'azione, non l'andatura"),
		ARTHUD::DescribeMovementSlot(ConNome).ToString(), FString(TEXT("Scatto")));

	// ⛔ **ANTI-VACUITA'**: senza, un `DescribeMovementSlot` che rendesse sempre la stessa stringa
	// passerebbe ogni contenimento qui sopra. I casi devono produrre rese DIVERSE fra loro.
	FRTUnitSlotsView Corsa;  Corsa.Movement  = MakeSlotLineFixture(true); Corsa.MovementProfileId  = Lib::ProfileSprint;
	FRTUnitSlotsView Ripiego; Ripiego.Movement = MakeSlotLineFixture(true); Ripiego.MovementProfileId = Lib::ProfileWithdraw;
	TestNotEqual(TEXT("corsa e ripiegamento non rendono la stessa cosa"),
		ARTHUD::DescribeMovementSlot(Corsa).ToString(),
		ARTHUD::DescribeMovementSlot(Ripiego).ToString());
	TestNotEqual(TEXT("ne' il libero e il percorso"),
		ARTHUD::DescribeMovementSlot(Libero).ToString(),
		ARTHUD::DescribeMovementSlot(Corsa).ToString());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
