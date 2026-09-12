// L'allestimento che rende raggiungibile il rifiuto per COPERTURA — `#3085`.
//
// 🔑 **La domanda che questo file risponde, e perche' esiste.** La seduta PIE del 2026-09-12 ha provato
// quattro volte a produrre un rifiuto `Cover` in partita libera e non ci e' riuscita, ogni volta per una
// ragione diversa: i tasti, il click sulla cella, il velo, e infine la geometria che si dissolve mentre i
// bot avanzano. Il caso NON si lascia raggiungere a mano.
//
// ⛔ **E non si lascia raggiungere nemmeno con uno scenario dell'harness**: quello e' auto-run e scrive i
// campi `Planned*` direttamente, mentre un rifiuto nasce da un CLICK. Cio' che serve e' un allestimento —
// una mappa le cui posizioni di partenza producano il caso al turno 1 — e questo file misura quali delle
// fixture esistenti lo facciano, prima che se ne scriva una nuova (`SEARCH -> REUSE -> CREATE`).
//
// 🔴 **Il caso e' piu' stretto di quanto sembri, ed e' la ragione per cui la seduta manuale falliva.**
// `ARTHUD::ShouldDrawUnitOverlay` rende noto un avversario solo se `Visibility == Live`, cioe' visto ORA:
// un nemico dietro un muro di solito non e' nemmeno NOTO, e `RefusalForObserver` restituisce allora
// `Nothing`, non `Cover`. Il rifiuto per copertura vive quindi in una fessura precisa:
//
//     la conoscenza e' di SQUADRA   ->  basta che UN compagno veda il nemico
//     la linea di tiro e' per UNITA' ->  il tiratore puo' non averla
//
// ∴ serve **un compagno che veda** e **un altro che non abbia la linea**, nello stesso istante.

#include "Misc/AutomationTest.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTRefusalSetup
{
	/** Una fixture che produce il caso, e le tre celle che lo compongono. */
	struct FEsito
	{
		bool bTrovato = false;
		FRTCellId Vede;      // il compagno che VEDE il nemico
		FRTCellId Coperto;   // il compagno che NON ha la linea di tiro
		FRTCellId Nemico;    // il bersaglio
	};

	/**
	 * Cerca nella mappa una terna che produce il rifiuto per copertura, con le posizioni che
	 * `PickStartCells` assegna davvero — non con celle scelte a mano, che proverebbero una partita che
	 * nessuno gioca.
	 *
	 * ⚠️ `VisionRange` non e' un parametro: la vista si approssima con la sola LOS piu' la distanza, che e'
	 * la condizione NECESSARIA. Una terna trovata qui va comunque guardata in PIE — questo test dice dove
	 * cercare, non che si veda.
	 */
	FEsito CercaTerna(const URTHexMapAsset* Map, int32 UnitsPerTeam, int32 RaggioVisivo)
	{
		FEsito Out;
		if (!Map)
		{
			return Out;
		}

		const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, UnitsPerTeam, 0);
		if (Start.Num() != UnitsPerTeam * 2)
		{
			return Out;
		}

		// Contratto di `PickStartCells`: le prime `UnitsPerTeam` al team 0, le successive al team 1.
		for (int32 INemico = UnitsPerTeam; INemico < Start.Num(); ++INemico)
		{
			for (int32 IVede = 0; IVede < UnitsPerTeam; ++IVede)
			{
				const int32 DistVede = URTHexLibrary::HexDistance(Start[IVede], Start[INemico]);
				const bool bVede = DistVede <= RaggioVisivo
					&& URTHexVisionLibrary::HasLineOfSight(Map, Start[IVede], Start[INemico]);
				if (!bVede)
				{
					continue;
				}

				for (int32 ICoperto = 0; ICoperto < UnitsPerTeam; ++ICoperto)
				{
					if (ICoperto == IVede)
					{
						continue;
					}
					// 🔑 L'altra meta': il secondo compagno NON ha la linea sullo stesso nemico.
					if (!URTHexVisionLibrary::HasLineOfSight(Map, Start[ICoperto], Start[INemico]))
					{
						Out.bTrovato = true;
						Out.Vede = Start[IVede];
						Out.Coperto = Start[ICoperto];
						Out.Nemico = Start[INemico];
						return Out;
					}
				}
			}
		}
		return Out;
	}
}

/**
 * Quali fixture producono il caso al turno 1 — la misura che decide se serve scriverne una nuova.
 *
 * ⛔ **Non asserisce che una fixture lo produca**: e' una SONDA, e il suo valore e' l'elenco che stampa.
 * Un'asserzione qui fisserebbe come contratto una proprieta' che nessuna fixture dichiara di avere, e
 * cadrebbe al primo ritocco di geometria fatto per un'altra ragione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRefusalSetupProbeTest,
	"RefactorTactics.Setup.WhichFixturesAllowACoverRefusal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRefusalSetupProbeTest::RunTest(const FString&)
{
	// Il raggio visivo piu' lungo del roster: e' la condizione piu' PERMISSIVA, quindi una fixture che non
	// produce il caso nemmeno qui non lo produce con nessun eroe.
	constexpr int32 RaggioPiuLungo = 7;
	constexpr int32 UnitaPerSquadra = 2;

	int32 Trovate = 0;
	for (const FString& Nome : URTMatchSetupLibrary::KnownFixtureIds())
	{
		const URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), Nome);
		if (!Map)
		{
			AddInfo(FString::Printf(TEXT("  %-14s  (non costruita)"), *Nome));
			continue;
		}

		const RTRefusalSetup::FEsito E =
			RTRefusalSetup::CercaTerna(Map, UnitaPerSquadra, RaggioPiuLungo);

		if (E.bTrovato)
		{
			++Trovate;
			AddInfo(FString::Printf(
				TEXT("  %-14s  SI — vede %s · coperto %s · nemico %s"),
				*Nome, *E.Vede.ToString(), *E.Coperto.ToString(), *E.Nemico.ToString()));
		}
		else
		{
			AddInfo(FString::Printf(TEXT("  %-14s  no"), *Nome));
		}
	}

	AddInfo(FString::Printf(TEXT("fixture che producono il caso: %d"), Trovate));

	// L'unica asserzione: la sonda ha davvero guardato qualcosa. Senza, un elenco vuoto — nessuna fixture
	// costruita, o `KnownFixtureIds` svuotato da un refactor — sarebbe indistinguibile da «nessuna lo
	// produce», che e' una risposta diversa e porterebbe a scrivere una fixture che forse gia' esiste.
	TestTrue(TEXT("la sonda ha esaminato almeno una fixture"),
		URTMatchSetupLibrary::KnownFixtureIds().Num() > 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
