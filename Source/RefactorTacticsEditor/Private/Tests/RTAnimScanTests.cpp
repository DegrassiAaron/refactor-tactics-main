#include "Misc/AutomationTest.h"

#include "Tools/RTAnimScan.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** La cartella animazioni di Gadget: il vertical slice di ANIM LAB parte da qui. */
	const TCHAR* GadgetAnimations =
		TEXT("/Game/FabAsset/Paragon/ParagonGadget/Characters/Heroes/Gadget/Animations");
}

/**
 * 🔴 **Lo scanner distingue «non ho trovato niente» da «non ho potuto guardare», e la distinzione e'
 * l'intera ragione per cui questo test esiste.**
 *
 * `Content/FabAsset/` e' **gitignorato** (~48 GB): su ogni clone appena creato la cartella di Gadget non
 * esiste. Un test scritto come «restituisce N clip» sarebbe rosso per chiunque cloni senza i pack, e quel
 * rosso direbbe *«il tuo checkout e' incompleto»* mentre sembra dire *«il codice e' rotto»*.
 *
 * Il pattern non e' inventato qui: e' lo stesso `bOutRan` che
 * `URTAnimCatalogLibrary::ValidateReferents` gia' usa, e per la stessa ragione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimScanDeclaresNotRunWithoutPacksTest,
	"RefactorTactics.Anim.Scan.DeclaresNotRunWithoutPacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimScanDeclaresNotRunWithoutPacksTest::RunTest(const FString&)
{
	// Una cartella che non esiste in nessun checkout, mai.
	const FRTAnimScanResult Assente =
		RTScanAnimSequencesUnder(TEXT("/Game/FabAsset/Paragon/ParagonNonEsiste/Animations"));

	TestFalse(TEXT("una cartella inesistente non e' una scansione eseguita"), Assente.bRan);
	TestTrue(TEXT("e dichiara il motivo invece di tacere"), !Assente.MotivoNotRun.IsEmpty());
	TestEqual(TEXT("e non inventa clip"), Assente.SequencePaths.Num(), 0);

	// ⛔ Il caso peggiore possibile: una stringa vuota non deve passare per «zero clip trovate».
	const FRTAnimScanResult Vuota = RTScanAnimSequencesUnder(FString());
	TestFalse(TEXT("una cartella vuota non e' una scansione eseguita"), Vuota.bRan);
	return true;
}

/**
 * 🔑 **Il controllo positivo di questo test e' `ScartatiPerClasse`, non il numero di sequenze.**
 *
 * `SequencePaths.Num() > 0` e' quasi vacuo: passerebbe anche con un filtro di classe rotto che accetta
 * qualunque asset. Sotto `Animations/` di Gadget esistono due sottocartelle — `AimOffsets/` e
 * `Blendspaces/` — quindi un filtro che funziona **deve** scartare qualcosa. Se scarta zero, non sta
 * filtrando, e il numero di «sequenze» sarebbe il numero di **file**.
 *
 * ⚠️ E' esattamente la confusione gia' vista due volte su questa famiglia: **113** (ricorsivo) e **85**
 * (non ricorsivo) sono entrambi conteggi di file, e nessuno dei due e' il numero di `UAnimSequence`.
 *
 * Su un checkout senza i pack questo test **non asserisce nulla** e lo dichiara: e' il caso `NOT RUN`
 * governato dal test qui sopra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimScanFiltersByClassNotByNameTest,
	"RefactorTactics.Anim.Scan.FiltersByClassNotByName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimScanFiltersByClassNotByNameTest::RunTest(const FString&)
{
	const FRTAnimScanResult Esito = RTScanAnimSequencesUnder(GadgetAnimations);

	if (!Esito.bRan)
	{
		AddInfo(FString::Printf(
			TEXT("NOT RUN: %s. I pack Paragon sono gitignorati; su un checkout che non li ha questa ")
			TEXT("misura non e' eseguibile, e un verde qui non significherebbe niente."),
			*Esito.MotivoNotRun));
		return true;
	}

	// (1) Anti-vacuita': senza sequenze ogni asserzione sotto guarderebbe un array vuoto.
	if (!TestTrue(TEXT("il pack di Gadget porta almeno una UAnimSequence"),
			Esito.SequencePaths.Num() > 0))
	{
		return false;
	}

	// (2) 🔑 Il controllo positivo: il filtro ha scartato qualcosa. `AimOffsets/` e `Blendspaces/`
	//     esistono sotto quella cartella, quindi uno zero qui vuol dire «filtro inerte», non «pack pulito».
	TestTrue(TEXT("il filtro di classe ha scartato almeno un asset non-sequenza"),
		Esito.ScartatiPerClasse > 0);

	// (3) Nessun path duplicato: un `AV_ID` per package path non tollera due voci per lo stesso asset.
	TSet<FString> Unici(Esito.SequencePaths);
	TestEqual(TEXT("nessun package path duplicato"), Unici.Num(), Esito.SequencePaths.Num());

	// (4) Tutti sotto la cartella chiesta: un filtro ricorsivo che sfugge di cartella e' un difetto che
	//     nessun conteggio rivela.
	for (const FString& Path : Esito.SequencePaths)
	{
		if (!Path.StartsWith(GadgetAnimations))
		{
			AddError(FString::Printf(TEXT("%s e' fuori dalla cartella chiesta"), *Path));
			break;
		}
	}

	// (5) Ordine stabile: due scansioni danno la stessa sequenza. Senza, un id assegnato in ordine di
	//     enumerazione dipenderebbe dall'ordine del registry, che non e' garantito.
	const FRTAnimScanResult Seconda = RTScanAnimSequencesUnder(GadgetAnimations);
	TestEqual(TEXT("due scansioni danno lo stesso numero"),
		Seconda.SequencePaths.Num(), Esito.SequencePaths.Num());
	if (Seconda.SequencePaths.Num() == Esito.SequencePaths.Num() && Esito.SequencePaths.Num() > 0)
	{
		TestEqual(TEXT("e lo stesso ordine"), Seconda.SequencePaths[0], Esito.SequencePaths[0]);
	}

	// I due numeri che questa issue deve registrare, entrambi e non uno solo.
	AddInfo(FString::Printf(TEXT("Gadget: %d UAnimSequence, %d asset scartati per classe"),
		Esito.SequencePaths.Num(), Esito.ScartatiPerClasse));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
