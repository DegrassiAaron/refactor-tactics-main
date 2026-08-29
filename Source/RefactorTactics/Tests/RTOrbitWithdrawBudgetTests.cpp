// #1550 — la riga aperta della tabella di mutazione, e dove la pista dei 2 MP porta davvero.
//
// `RTBotStalemateProbeTests.cpp` dichiara scoperto il caso a **2 MP**: li' il filtro di `#1287` arretra anche
// sull'arena generata (48 coppie su MakeTestArena, 19 a 3 MP, **zero da 4 in su**), e la nota chiude cosi':
//
//   «quel budget non si sceglie — "lo impone l'Overwatch" (D-070) — e un'orbita sostenuta chiede lo stesso
//    budget a ogni turno. Chi misurera' un'orbita sotto Overwatch la porti qui.»
//
// Questo file misura se quell'orbita e' costruibile. La risposta e' in due pezzi, e il secondo non era
// nell'ipotesi di partenza.
//
// ## 1. La via dell'Overwatch NON esiste nel runtime
//
// `D-070` dice che armare l'`Overwatch` riserva lo slot movimento a `Withdraw`, 2 MP. Misurato il
// 2026-08-29 su `HEAD bbf0d780`:
//
//   · `Withdraw` compare in **un solo posto** in tutto `Source/`: un commento di `RTActionDef.h`. Nessuna
//     `Action.Withdraw`, nessuna voce di catalogo runtime, nessun codice che riservi lo slot movimento.
//   · Nessun punto del runtime porta `MoveBudget` a 2: viene da `MovePoints` dell'eroe
//     (`RTScenarioDraft.cpp`), e le uniche riscritture sono `0` per lo `StaySnapshot`, il budget del Dash e
//     `EffectiveRange` della carica.
//   · Il roster spedito non ha nessuno sotto **4**: `Gadget` 5, `Phase` 5, `Riktor` **4**, `Wraith` 6 — e a
//     4 MP quella board arretra in **zero** coppie.
//
// ∴ **Un Overwatch sostenuto non tiene il budget a 2 MP perche' non lo tocca affatto.** La pista, come era
// formulata, non porta — e non perche' sia difficile da costruire, ma perche' il meccanismo che dovrebbe
// imporre il budget non e' implementato.
//
// ## 2. Ma un budget EFFICACE di 2 MP esiste, per un'altra via
//
// 🔴 `Status.Slow` **non dimezza il budget**: `RTTurnManager.cpp` fa
// `SimUnit.MoveCostModifier = HasStatus(Slow) ? 1 : 0`, cioe' **+1 al costo di ogni cella** (catalogo v0.1
// §5). Un'unita' rallentata conserva il proprio budget e paga di piu' ogni passo — e su terreno a costo
// unitario questo produce lo stesso insieme raggiungibile di un budget dimezzato.
//
// E' il pezzo che l'ipotesi di partenza non conteneva: se l'insieme raggiungibile di **Riktor rallentato**
// (4 MP, +1 per cella) coincide con quello di un'unita' a **2 MP**, allora le 48 coppie che arretrano a 2 MP
// sono raggiungibili in partita — e la pista si riapre con un meccanismo diverso, `Slow` invece di
// `Overwatch`, che a differenza di `Withdraw` **e' implementato e il bot lo puo' subire**.
//
// ⚠️ Non e' vero per costruzione: `MakeTestArena` porta terreno costoso, quindi su celle a costo 2 le due
// letture divergono. Va misurato, ed e' cio' che il primo test fa.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Pathfinding/RTHexPathLibrary.h" // GraphNeighbors: gli archi del gioco, non una loro copia
#include "RTAuthoredArenaForTest.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTWithdrawBudget
{
	/**
	 * Celle raggiungibili da `Origin` con `Budget`, pagando `CostoExtra` in piu' per ogni cella entrata.
	 *
	 * ⚠️ `CostoExtra` e' il modello dello `Slow`, e non e' una scelta di comodo: e' letteralmente
	 * `FRTHexSimUnit::MoveCostModifier`, che `RTTurnManager` mette a 1 quando l'unita' ha `Status.Slow`.
	 * Dimezzare il budget sarebbe stato il modello SBAGLIATO — quella era la semantica pre-CP4.7, e
	 * `RTCombatLibrary.h` dichiara per iscritto che non passa piu' di li'.
	 */
	void CelleEntro(const URTHexMapAsset* Map, const FRTCellId& Origin, int32 Budget, int32 CostoExtra,
		TArray<FRTCellId>& Out)
	{
		Out.Reset();
		if (Map == nullptr) { return; }

		TMap<FRTCellId, int32> Best;
		Best.Add(Origin, 0);
		TArray<FRTCellId> Frontier;
		Frontier.Add(Origin);
		while (Frontier.Num() > 0)
		{
			const FRTCellId Cur = Frontier.Pop(EAllowShrinking::No);
			const int32 CurCost = Best.FindChecked(Cur);
			for (const TPair<FRTCellId, int32>& Arc : URTHexPathLibrary::GraphNeighbors(Map, Cur))
			{
				const int32 NextCost = CurCost + Arc.Value + CostoExtra;
				if (NextCost > Budget) { continue; }
				const int32* Known = Best.Find(Arc.Key);
				if (Known == nullptr || NextCost < *Known)
				{
					Best.Add(Arc.Key, NextCost);
					Frontier.Add(Arc.Key);
				}
			}
		}
		Best.GenerateKeyArray(Out);
		Out.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
	}

	TArray<FRTCellId> CellePercorribili(const URTHexMapAsset* Map)
	{
		TArray<FRTCellId> Celle;
		if (Map == nullptr) { return Celle; }
		for (const FRTHexCellData& Cell : Map->Cells)
		{
			if (!Cell.bBlocksMovement) { Celle.Add(Cell.Id); }
		}
		Celle.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
		return Celle;
	}

	struct FConfronto
	{
		int32 Origini = 0;
		int32 Identici = 0;
		int32 RallentatoPiuLargo = 0;
		int32 RallentatoPiuStretto = 0;
		int32 Diversi = 0;   // stessa cardinalita', celle diverse
	};

	/** Confronta «budget 2, costo normale» contro «budget 4, +1 per cella» su ogni cella percorribile. */
	FConfronto ConfrontaRallentato(const URTHexMapAsset* Map, int32 BudgetPieno, int32 BudgetRidotto)
	{
		FConfronto C;
		TArray<FRTCellId> Ridotto;
		TArray<FRTCellId> Rallentato;
		for (const FRTCellId& Origin : CellePercorribili(Map))
		{
			++C.Origini;
			CelleEntro(Map, Origin, BudgetRidotto, /*CostoExtra=*/ 0, Ridotto);
			CelleEntro(Map, Origin, BudgetPieno, /*CostoExtra=*/ 1, Rallentato);

			const TSet<FRTCellId> A(Ridotto);
			const TSet<FRTCellId> B(Rallentato);
			// `TSet` non ha `Includes` in UE 5.8: l'inclusione si chiede a mano, nei due versi.
			auto Contiene = [](const TSet<FRTCellId>& Grande, const TSet<FRTCellId>& Piccolo)
			{
				for (const FRTCellId& Cella : Piccolo)
				{
					if (!Grande.Contains(Cella)) { return false; }
				}
				return true;
			};
			const bool bRidottoDentroRallentato = Contiene(B, A);
			const bool bRallentatoDentroRidotto = Contiene(A, B);
			if (bRidottoDentroRallentato && bRallentatoDentroRidotto) { ++C.Identici; }
			else if (bRidottoDentroRallentato) { ++C.RallentatoPiuLargo; }
			else if (bRallentatoDentroRidotto) { ++C.RallentatoPiuStretto; }
			else { ++C.Diversi; }
		}
		return C;
	}

	void Riporta(FAutomationTestBase& Test, const TCHAR* Board, const FConfronto& C)
	{
		Test.AddInfo(FString::Printf(
			TEXT("%s — %d origini: %d identici, %d rallentato piu' largo, %d piu' stretto, %d incomparabili"),
			Board, C.Origini, C.Identici, C.RallentatoPiuLargo, C.RallentatoPiuStretto, C.Diversi));
	}
}

/**
 * **Lo `Slow` produce il budget efficace che l'`Overwatch` avrebbe dovuto imporre?** (`#1550`)
 *
 * Confronta, cella per cella, l'insieme raggiungibile di un'unita' a **2 MP** con quello di un'unita' a
 * **4 MP rallentata** (`+1` per cella). Se coincidono, le 48 coppie che arretrano a 2 MP su `MakeTestArena`
 * sono raggiungibili in partita da **Riktor rallentato** — il piu' lento del roster spedito — e la pista
 * dell'orbita si riapre con un meccanismo implementato.
 *
 * ⛔ **Non e' un oracolo del bot e non tocca nessuna soglia**: e' una proprieta' della board e del modello
 * dello `Slow`. Non dice che il bot arrivi in quelle celle, ne' che ci resti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSlowReachesWithdrawBudgetTest,
	"RefactorTactics.Bot.SlowReachesTheWithdrawBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSlowReachesWithdrawBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("MakeTestArena si costruisce"), Arena)) { return false; }
	URTHexMapAsset* Authored = RTAuthoredArena::Load();
	if (!TestNotNull(TEXT("la mappa d'autore si carica"), Authored)) { return false; }

	// 4 MP e' il minimo del roster spedito (Riktor); 2 MP e' il budget del `Withdraw` di `D-070`.
	const RTWithdrawBudget::FConfronto CA = RTWithdrawBudget::ConfrontaRallentato(Arena, 4, 2);
	const RTWithdrawBudget::FConfronto CM = RTWithdrawBudget::ConfrontaRallentato(Authored, 4, 2);
	RTWithdrawBudget::Riporta(*this, TEXT("MakeTestArena"), CA);
	RTWithdrawBudget::Riporta(*this, TEXT("DA_HexMap_Arena"), CM);

	// La premessa: senza origini il confronto sarebbe verde sul vuoto.
	TestTrue(FString::Printf(TEXT("premessa: le due board hanno celle percorribili (%d, %d)"),
		CA.Origini, CM.Origini), CA.Origini > 0 && CM.Origini > 0);

	// 🔴 **LA PROPRIETA' SU CUI POGGIA LA CONCLUSIONE: il rallentato non e' MAI piu' stretto.**
	//
	// Misurato il 2026-08-29: sulla mappa d'autore i due insiemi sono identici su **tutte** le 55 origini;
	// su `MakeTestArena` sono identici su 49 di 62 e altrove il rallentato raggiunge di PIU', mai di meno.
	// ∴ ogni cella che un'unita' a 2 MP puo' toccare, un `Riktor` rallentato la tocca — e con lei le 48
	// coppie che arretrano a 2 MP su quella board. E' cio' che rende la pista dell'orbita raggiungibile in
	// partita, per una via che non e' quella dell'`Overwatch`.
	//
	// ⚠️ Verificato per mutazione: portando `CostoExtra` da 1 a 2 — cioe' modellando lo `Slow` come «+2 per
	// cella» invece del `+1` che `RTTurnManager` scrive — il rallentato diventa piu' stretto e questa
	// asserzione cade.
	//
	// ⚠️ **LIMITE DICHIARATO**: qui si misura il DOMINIO raggiungibile, non l'arretramento. Un dominio piu'
	// largo da' al filtro di `#1287` piu' opzioni, quindi «contiene le 48 coppie» non significa «produce gli
	// stessi 48 arretramenti». Chi vorra' quel numero rifaccia la spazzata degli arretramenti col modello
	// dello `Slow`, non col budget ridotto.
	TestEqual(FString::Printf(
		TEXT("MakeTestArena: il rallentato non e' mai piu' stretto del budget ridotto (%d origini su %d)"),
		CA.RallentatoPiuStretto, CA.Origini), CA.RallentatoPiuStretto, 0);
	TestEqual(FString::Printf(
		TEXT("mappa d'autore: il rallentato non e' mai piu' stretto (%d origini su %d)"),
		CM.RallentatoPiuStretto, CM.Origini), CM.RallentatoPiuStretto, 0);
	TestEqual(FString::Printf(
		TEXT("e nessuna origine e' incomparabile (%d + %d)"), CA.Diversi, CM.Diversi),
		CA.Diversi + CM.Diversi, 0);

	return true;
}

/**
 * **Il roster spedito resta SOPRA il budget in cui questa board arretra** — ed e' il grilletto che riapre
 * la pista (`#1550`).
 *
 * `RTBotStalemateProbeTests.cpp` misura che su `MakeTestArena` il filtro di `#1287` arretra a **2** e **3**
 * MP e **non arretra da 4 in su**. Il roster v0.1 ha minimo **4** (`Riktor`). Finche' e' cosi', nessuna
 * unita' puo' trovarsi, per solo budget, nella regione dove l'orbita si chiude.
 *
 * 🔴 **Questo test esiste per diventare rosso.** Se `#149` ritara un eroe a 3 MP, o se `D-070` atterra
 * portando `Withdraw` a 2, la regione diventa raggiungibile e la riga aperta della tabella di
 * `RTMatchAutobattleTests.cpp` torna viva. Senza questo controllo quel cambio passerebbe in silenzio, ed e'
 * la stessa forma di difetto che `GoldenCorpusCoversItsCategories` sorveglia sul corpus.
 *
 * ⚠️ **Non e' un numero di bilanciamento e non ne propone uno**: non dice quanto debba valere `MovePoints`,
 * dice che sotto 4 cambia una proprieta' misurata della board. Chi abbassa quel valore legge qui il perche'
 * e decide con la misura in mano — che e' esattamente cio' che la riga aperta chiede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRosterAboveBackstepBudgetTest,
	"RefactorTactics.Bot.ShippedRosterStaysAboveTheBackstepBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRosterAboveBackstepBudgetTest::RunTest(const FString&)
{
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(FString::Printf(TEXT("premessa: il roster non e' vuoto (%d)"), Roster.Num()),
		Roster.Num() > 0))
	{
		return false;
	}

	int32 Minimo = TNumericLimits<int32>::Max();
	FString Chi;
	for (const URTHeroData* Hero : Roster)
	{
		if (Hero == nullptr) { continue; }
		// `HeroId` e non `GetName()`: il secondo da' il nome dell'OGGETTO (`RTHeroData_12`), che non dice
		// di chi si sta parlando — misurato leggendo il primo referto.
		AddInfo(FString::Printf(TEXT("%s: %d MP"), *Hero->HeroId.ToString(), Hero->MovePoints));
		if (Hero->MovePoints < Minimo) { Minimo = Hero->MovePoints; Chi = Hero->HeroId.ToString(); }
	}

	// 🔴 **Il numero viene dalla misura pubblicata, non da un'intuizione**: su `MakeTestArena` il filtro
	// arretra a 2 MP (48 coppie) e a 3 MP (19), e a 4 MP in **zero**. `4` e' quindi la prima soglia sicura.
	const int32 PrimoBudgetSenzaArretramenti = 4;
	TestTrue(FString::Printf(
		TEXT("il piu' lento del roster (%s, %d MP) resta a %d o piu': sotto, MakeTestArena arretra e ")
		TEXT("l'orbita di #1287 torna possibile"), *Chi, Minimo, PrimoBudgetSenzaArretramenti),
		Minimo >= PrimoBudgetSenzaArretramenti);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
