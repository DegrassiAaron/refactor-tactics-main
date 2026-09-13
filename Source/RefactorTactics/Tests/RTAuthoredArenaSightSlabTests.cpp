#include "Misc/AutomationTest.h"

#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "RTAuthoredArenaForTest.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La mappa d'autore isola la **lastra sola** in due celle, e in quali.
 *
 * 🔑 **Perche' questo test esiste, ed e' una misura che nessuno puo' rifare con `grep`.** Il criterio di
 * `PIE-VIS-SIGHTWALL` prescrive *«non `L_HexArena`»*, e la guida di `U46` ne da' la ragione: *«le celle di
 * barriera di `L_HexArena` portano entrambi i flag, quindi la lastra non si giudica da sola»*. Misurato il
 * 2026-09-13 caricando `DA_HexMap_Arena`, quell'affermazione e' **vera per la barriera centrale e falsa per
 * lo schermo meridionale**: `(2,1,L0)` e `(3,1,L0)` negano la vista e lasciano passare il movimento, esatta-
 * mente come la `(0,0,0)` del banco su `TestArena`.
 *
 * ⛔ **Senza questo test quel fatto vive nel corpo di una PR**, cioe' in un posto che nessuno rimisura. Il
 * giorno in cui qualcuno risalva l'asset spostando lo schermo, il criterio della voce e il banco
 * `Visual.Map.SightSlabOnTheArena` diventerebbero falsi **insieme e in silenzio** — e la prima cosa che
 * qualcuno vedrebbe e' un tiro che passa dove il banco dice che deve fermarsi.
 *
 * ⚠️ **Cosa questo test NON dice.** Non dice che la lastra si **veda**: quello e' `PIE-VIS-SIGHTWALL`, ed e'
 * giudizio umano alla camera del giocatore. Qui c'e' solo dove stanno le celle e cosa negano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAuthoredArenaSightSlabTest,
	"RefactorTactics.Map.AuthoredArenaIsolatesTheSightSlab",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAuthoredArenaSightSlabTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = RTAuthoredArena::Load();
	if (!TestNotNull(TEXT("la mappa d'autore si carica"), Arena)) { return false; }

	TSet<FRTCellId> LastraSola;   // nega la VISTA, lascia passare
	TSet<FRTCellId> Entrambi;     // nega vista e passo: lastra e colonna sovrapposte
	TSet<FRTCellId> ColonnaSola;  // nega il solo passo

	for (const FRTHexCellData& C : Arena->Cells)
	{
		if (C.bBlocksLineOfSight && !C.bBlocksMovement) { LastraSola.Add(C.Id); }
		else if (C.bBlocksLineOfSight && C.bBlocksMovement) { Entrambi.Add(C.Id); }
		else if (C.bBlocksMovement) { ColonnaSola.Add(C.Id); }
	}

	// --- L'insieme, non la cardinalita' -------------------------------------------------------------
	//
	// ⚠️ **Uguaglianza fra insiemi e non «ne esistono due»**: un conteggio resterebbe verde il giorno in cui
	// una lastra si sposta e un'altra compare altrove, cioe' proprio il cambiamento che rende falso il banco.
	const TSet<FRTCellId> Attese = { FRTCellId(2, 1, 0), FRTCellId(3, 1, 0) };
	TestTrue(TEXT("le lastre sole sono ESATTAMENTE (2,1,L0) e (3,1,L0)"),
		LastraSola.Num() == Attese.Num() && LastraSola.Includes(Attese));

	if (!LastraSola.Includes(Attese) || LastraSola.Num() != Attese.Num())
	{
		FString Trovate;
		for (const FRTCellId& Id : LastraSola)
		{
			Trovate += FString::Printf(TEXT("(%d,%d,L%d) "), Id.X, Id.Y, Id.Layer);
		}
		AddError(FString::Printf(TEXT("lastre sole trovate: %s"), *Trovate));
	}

	// --- Il controllo che rende il test capace di distinguere ----------------------------------------
	//
	// 🔑 Senza questo, un asset in cui NESSUNA cella blocca il movimento farebbe passare l'asserzione qui
	// sopra per la ragione sbagliata: tutte le bloccanti diventerebbero «lastre sole». La barriera centrale
	// deve continuare a portare ENTRAMBI i flag — e' cio' che rende lo schermo meridionale un caso distinto
	// invece della norma.
	TestTrue(TEXT("la barriera centrale porta entrambi i flag: la lastra sola e' l'eccezione"),
		Entrambi.Contains(FRTCellId(0, 0, 0)) && Entrambi.Contains(FRTCellId(-1, 0, 0))
		&& Entrambi.Contains(FRTCellId(1, 0, 0)));

	TestEqual(TEXT("nessuna cella nega il solo passo"), ColonnaSola.Num(), 0);

	// --- E la geometria su cui il banco poggia -------------------------------------------------------
	//
	// `Visual.Map.SightSlabOnTheArena` mette A1 in (2,0) e B1 in (2,2) perche' la linea fra i due passa per
	// (2,1). Asserirlo QUI lega il banco alla mappa: se lo schermo si sposta, questo test diventa rosso
	// insieme a lui invece di lasciarlo verde su una premessa che non vale piu'.
	const FRTLineOfSightResult R = URTHexVisionLibrary::DescribeLineOfSight(
		Arena, FRTCellId(2, 0, 0), FRTCellId(2, 2, 0));

	TestEqual(TEXT("il tiro da (2,0) a (2,2) e' fermato da una CELLA"),
		static_cast<int32>(R.Block), static_cast<int32>(ERTLineOfSightBlock::CellBlocker));
	TestTrue(TEXT("e la cella che lo ferma e' la lastra (2,1,L0)"), R.BlockedAt == FRTCellId(2, 1, 0));

	return true;
}

#endif
