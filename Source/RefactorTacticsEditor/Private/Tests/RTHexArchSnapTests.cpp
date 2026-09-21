#include "Misc/AutomationTest.h"

#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * IL RI-SNAP DEL GIZMO DELL'ARCH È STABILE: agganciare al centro non cambia la cella agganciata (#931).
 *
 * 🔑 **Che cosa di #931 è verificabile senza schermo, e che cosa no — detto prima, perché la differenza è
 * tutto il valore di questo file.** La issue riporta due sintomi: il gizmo che si riaggancia *al gesto dopo*
 * invece che al rilascio, e il gizmo che sparisce toccando la Transform dell'actor. **Nessuno dei due si
 * osserva headless**: il primo è *quando* qualcosa si muove a schermo, il secondo vive in un altro modulo ed
 * è [#996](https://github.com/DegrassiAaron/refactor-tactics-main/issues/996). Il gizmo stesso non è
 * istanziabile in un test — `CreateCustomTransformGizmo` apre con `check(bDefaultGizmosRegistered)` e il
 * builder con `check(GizmoViewContext && …)`.
 *
 * Ciò che invece **si** verifica è la proprietà su cui il ri-snap poggia, e che nessun test copriva.
 * `URTHexArchTool::OnGizmoMoved` fa tre cose in fila: legge la cella sotto il punto trascinato
 * (`WorldToCellId`), scrive `To`, e riporta il gizmo sul **centro** di quella cella (`AxialToWorld`). ∴ il
 * criterio *«`To` nel pannello e la posizione del gizmo concordano sempre»* è vero **se e solo se** il
 * centro di una cella ricade nella cella stessa: se lo snap depositasse il gizmo appena oltre un bordo,
 * `OnGizmoMoved` calcolerebbe al gesto successivo una cella **diversa** da quella che il pannello mostra —
 * cioè lo stesso disallineamento che #931 descrive, prodotto dalla correzione invece che dal difetto.
 *
 * ⚠️ **Non è il round-trip che `RefactorTactics.Hex.WorldToCellIdRoundTripAcrossLayers` già copre.** Quello
 * parte da celle e verifica `WorldToCellId(AxialToWorld(C)) == C`. Qui si parte da punti **arbitrari** —
 * dove un trascinamento finisce davvero, che non è mai un centro — e si verifica che lo snap sia un punto
 * fisso: `WorldToCellId(AxialToWorld(WorldToCellId(W))) == WorldToCellId(W)`. È il caso reale del gesto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexArchSnapIsStableTest,
	"RefactorTactics.HexEditor.ArchSnapToCellCentreIsStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexArchSnapIsStableTest::RunTest(const FString&)
{
	// Origine non banale e layer alti: un'origine a zero nasconde gli errori di traslazione, e un solo layer
	// nasconde quelli di quota.
	const FVector Origin(1000.0, -500.0, 200.0);
	const float HexSize = 100.f;
	const float LayerHeight = 250.f;

	// Punti presi FRA i centri, non sui centri: è dove un trascinamento finisce davvero. Gli offset sono
	// frazioni di `HexSize` scelte per cadere in punti diversi del tassello — vicino al centro, verso un
	// vertice, oltre metà lato — e su layer diversi.
	const double Offsets[] = { 0.0, 0.31, -0.47, 0.62, -0.68, 0.49 };
	const int32 Layers[] = { 0, 1, 2, -1 };

	int32 Esaminati = 0;

	for (int32 L : Layers)
	{
		for (double DX : Offsets)
		{
			for (double DY : Offsets)
			{
				// Si parte da un centro noto e ci si sposta di una frazione di cella: il punto risultante è
				// arbitrario, ma sappiamo che è nei dintorni di una cella reale invece che all'infinito.
				const FRTCellId Base(2, -1, L);
				const FVector Centro = URTHexLibrary::AxialToWorld(Base, Origin, HexSize, LayerHeight);
				const FVector Trascinato = Centro + FVector(DX * HexSize, DY * HexSize, 0.0);

				// Ciò che `OnGizmoMoved` calcola e scrive nel pannello.
				const FRTCellId Agganciata =
					URTHexLibrary::WorldToCellId(Trascinato, Origin, HexSize, LayerHeight);

				// Ciò su cui `OnGizmoMoved` riporta il gizmo.
				const FVector Snap = URTHexLibrary::AxialToWorld(Agganciata, Origin, HexSize, LayerHeight);

				// 🔴 E ciò che il gesto SUCCESSIVO leggerebbe da quella posizione. Se differisse, il pannello
				// e il gizmo mostrerebbero due celle diverse — il difetto di #931 riprodotto dalla fix.
				const FRTCellId Rileggendo =
					URTHexLibrary::WorldToCellId(Snap, Origin, HexSize, LayerHeight);

				++Esaminati;
				TestTrue(
					*FString::Printf(
						TEXT("snap stabile da (%.0f, %.0f, layer %d): agganciata %s, rileggendo %s"),
						Trascinato.X, Trascinato.Y, L,
						*Agganciata.ToString(), *Rileggendo.ToString()),
					Rileggendo == Agganciata);

				// E il layer non si perde per strada: il ri-snap non deve far scendere di piano.
				TestEqual(
					*FString::Printf(TEXT("il layer sopravvive allo snap (atteso %d)"), L),
					Rileggendo.Layer, L);
			}
		}
	}

	// Se i cicli girassero a vuoto — un array svuotato da un refactor — il test passerebbe senza aver
	// guardato niente. Il numero non è un totale da mantenere: è una soglia contro lo zero.
	TestTrue(TEXT("il campione non è vuoto"), Esaminati >= 100);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
