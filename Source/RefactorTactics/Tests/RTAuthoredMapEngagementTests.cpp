// #1287 — sulla mappa d'autore i bot si fermano a due celle e non sparano.
//
// Il test di ingaggio esistente, `Match.Autobattle.EngagesOnTheShippedMapSource`, imposta
// `MapSource = GeneratedTestArena`: misura l'arena generata, non la mappa che la partita carica. Dopo
// #1069 il `MapSource` spedito e' `LevelAsset`, quindi quel test guarda una configurazione che non e'
// piu' quella spedita — ed e' il buco da cui questo difetto e' passato.
//
// Misurato prima del fix, 12 turni su `L_HexArena`: 42 voci `Stayed` su 48, zero `Combat`, unita' ferme
// nove e dieci turni sulla stessa cella. La mappa ha un ostacolo al centro che blocca vista e passo, e il
// punteggio del bot misura la distanza in LINEA D'ARIA: a due celle era gia' al minimo, e aggirare
// l'ostacolo avrebbe solo peggiorato il punteggio.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "RTGameMode.h"
#include "RTWorldFixtures.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTAuthoredEngagement
{
	TArray<ARTUnit*> LiveUnits(UWorld* World)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Found);
		TArray<ARTUnit*> Units;
		for (AActor* A : Found)
		{
			ARTUnit* U = Cast<ARTUnit>(A);
			if (U && U->IsAlive()) { Units.Add(U); }
		}
		return Units;
	}

	/**
	 * L'unita' ha COLPITO QUALCUN ALTRO da questa cella in questo turno?
	 *
	 * 🔴 **E' la meta' che all'oracolo del parcheggio mancava.** `#1088` definisce il difetto come «dodici
	 * round di **soli spostamenti**, zero combattimento»: cio' che non e' legittimo e' stare fermi **senza
	 * produrre nulla**. Contando solo la cella, un kiter che presidia la propria distanza di tiro e spara
	 * risulta identico a un'unita' in stallo — e sulla mappa d'autore Gadget fa esattamente questo:
	 * dieci turni su `(-1,1,L0)`, sei voci `Combat` partite di li' e un'eliminazione al turno 11.
	 *
	 * 🔴 **`ERTLogCategory::Combat` NON vuol dire «attacco», e la prima stesura di questa funzione lo
	 * assumeva.** La categoria porta anche il danno da terreno (`RTTurnManager.cpp:133`, `SrcCell` e
	 * `TgtCell` entrambe la cella di chi subisce), il tick di `Status.Burning` (`:1167`, idem — «le due
	 * celle sono DOVE SI TROVA chi brucia») e la cura (`:1430`, `Outcome = Healed`). Con il solo confronto
	 * su `SrcCell`, un'unita' ferma **dentro il fuoco** risultava aver attaccato, e l'oracolo di #1088
	 * diventava cieco proprio sullo stallo che deve vedere. Trovato in code review.
	 *
	 * Un colpo si riconosce da due cose insieme: l'esito e' uno dei quattro **danni inflitti**, e le due
	 * celle sono **diverse** — perche' hazard, burning e autocura hanno `SrcCell == TgtCell`.
	 *
	 * ⚠️ `NoLineOfSight` resta FUORI dalla lista, ed e' deliberato: un'unita' che ogni turno pianifica un
	 * colpo che la copertura ferma, e non si sposta, e' il difetto di #1287 — non un'unita' che agisce.
	 *
	 * ⚠️ Si legge il `TurnLog`, che e' l'autorita' (CP 11.3): dedurre l'attacco dagli HP del bersaglio
	 * sarebbe una seconda verita', e non distinguerebbe il colpo assorbito dal colpo non sferrato.
	 */
	bool HaColpitoDa(const ARTTurnManager* TM, const FRTCellId& Cella)
	{
		if (!TM) { return false; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category != ERTLogCategory::Combat || E.SrcCell != Cella || E.TgtCell == Cella)
			{
				continue;
			}
			switch (static_cast<ERTCombatOutcome>(E.Outcome))
			{
			case ERTCombatOutcome::Hit:
			case ERTCombatOutcome::ShieldAbsorbed:
			case ERTCombatOutcome::Lethal:
			case ERTCombatOutcome::TerrainBonus:
				return true;
			default:
				break;
			}
		}
		return false;
	}
}

/**
 * **Sulla mappa che si gioca, nessuno si parcheggia.**
 *
 * Stesso oracolo di #1088 e di `D-184` — nessuna unita' viva ferma sulla stessa cella oltre
 * `RoundLimit / 3` — applicato alla mappa d'autore invece che all'arena generata. Il pareggio allo
 * scadere resta un esito legittimo: cio' che non lo e' e' stare fermi.
 *
 * ⚠️ **«Fermi» vuol dire INERTI, e la precisazione e' del 2026-08-23.** La prima stesura contava i turni
 * sulla stessa cella e basta, quindi accusava di stallo anche un kiter che presidia la propria distanza
 * di tiro e spara — che e' il comportamento che `ScorePlan` dichiara corretto («resta a standoff e spara
 * e' esattamente il punto in cui entrambi i termini si annullano»). #1088 definisce il difetto come
 * «soli spostamenti, **zero combattimento**»: il colpo e' meta' della definizione, e mancava.
 * ⛔ **Non e' un indebolimento**, ed e' verificato per mutazione: rimettendo il difetto di #1287 —
 * la distanza in linea d'aria al posto dei passi — questo test torna rosso, perche' li' le unita'
 * stanno ferme **e** non sparano (42 voci `resta` su 48, zero `Combat`).
 *
 * 🔴 **NON e' il rilevatore di OSCILLAZIONE, benche' lo dichiarasse.** Questa riga diceva che un bot che
 * alterna fra «cerca» e «avvicinati» «tornerebbe sulle stesse celle» e che la sequenza per unita' lo
 * mostra. Il suo oracolo conta i turni **consecutivi sulla stessa cella** — `Ferma` si azzera appena la
 * cella cambia — quindi un'alternanza `A->B->A->B` lo lascia a zero e passa sempre. Misurato il
 * 2026-08-23 su `L_HexArena`: Riktor alterna fra `(1,-1,L0)` e la piattaforma `(3,-3,L1)` **otto volte in
 * dodici turni**, e questo test resta verde. L'oscillazione ha il suo oracolo in
 * `NobodyOscillatesOnTheAuthoredMap`, qui sotto: quel che questo test misura e' il **parcheggio**, e per
 * quello basta e vale.
 *
 * ⚠️ Il fatto che faccia girare una partita resta necessario — entrambi i difetti sono di sequenza, non
 * di singola decisione — ma non e' sufficiente: e' l'ORACOLO a decidere cosa una sequenza rivela.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAuthoredMapNobodyParksTest,
	"RefactorTactics.Match.Autobattle.NobodyParksOnTheAuthoredMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAuthoredMapNobodyParksTest::RunTest(const FString&)
{
	const TCHAR* AssetPath = TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.DA_HexMap_Arena");
	URTHexMapAsset* Authored = Cast<URTHexMapAsset>(StaticLoadObject(URTHexMapAsset::StaticClass(), nullptr, AssetPath));
	if (!TestNotNull(TEXT("la mappa d'autore si carica"), Authored)) { return false; }

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// La mappa D'AUTORE, non una generata: e' il punto del test.
	HexMap->MapAsset = Authored;
	GameMode->MapSource = ERTMapSource::LevelAsset;
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	TArray<ARTUnit*> Units = RTAuthoredEngagement::LiveUnits(World);
	if (!TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Sequenza di celle per unita': la memoria che rende visibile sia il parcheggio sia l'oscillazione.
	TMap<int32, FRTCellId> Ultima;
	TMap<int32, int32> Ferma;
	TMap<int32, int32> PiuLunga;

	const int32 MaxTurni = 12;
	int32 Turni = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < MaxTurni)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		// 🔴 **Il tetto di 400 tick non e' una fine turno, e prima usciva in silenzio.** Esaurito il budget
		// il ciclo terminava lo stesso, le celle venivano lette a turno mezzo applicato e il
		// `LockInAndResolve` successivo si impilava sopra: l'oracolo misurava posizioni che non sono mai
		// state uno stato di fine turno. Che il budget si esaurisca non e' teorico — lo scenario
		// `AutoBattle.ArenaV01` lo documenta da riga di comando. Trovato in code review.
		if (!TestFalse(*FString::Printf(TEXT("il turno %d ha finito di risolvere entro 400 tick"), Turni + 1),
			TM->IsResolving()))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}
		++Turni;

		for (const ARTUnit* U : RTAuthoredEngagement::LiveUnits(World))
		{
			const int32 Id = U->GetUniqueID();
			const FRTCellId* Prev = Ultima.Find(Id);
			const bool bInerte = Prev && *Prev == U->Cell
				&& !RTAuthoredEngagement::HaColpitoDa(TM, U->Cell);
			if (bInerte) { Ferma.FindOrAdd(Id) += 1; }
			else { Ferma.FindOrAdd(Id) = 0; }
			int32& Record = PiuLunga.FindOrAdd(Id);
			Record = FMath::Max(Record, Ferma[Id]);
			Ultima.FindOrAdd(Id) = U->Cell;
		}
	}

	int32 Peggiore = 0;
	for (const TPair<int32, int32>& P : PiuLunga) { Peggiore = FMath::Max(Peggiore, P.Value); }

	// `RoundLimit / 3`: la soglia di D-184, la stessa che l'oracolo di #1088 usa sull'arena di prova.
	const int32 Limite = FMath::Max(1, MaxTurni / 3);
	AddInfo(FString::Printf(TEXT("turni giocati: %d  |  piu' lunga sequenza ferma: %d (limite %d)"),
		Turni, Peggiore, Limite));

	TestTrue(*FString::Printf(
		TEXT("nessuna unita' si parcheggia sulla mappa d'autore: piu' lunga sequenza ferma %d turni (limite %d)"),
		Peggiore, Limite), Peggiore <= Limite);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Sulla mappa che si gioca, nessuno fa avanti-e-indietro.**
 *
 * Il difetto che questo test pinna e' il **gemello** del parcheggio, e il fix di #1287 lo ha prodotto
 * mentre chiudeva l'altro. Il livello 2 di quel fix (`BuildCandidates`) restringe il dominio alle celle
 * **da cui si vede** quando il bot non puo' colpire e non vede gia' nessuno. Il suo commento dichiara che
 * restringere il dominio «spezza l'oscillazione fra cerca e avvicinati senza introdurre stato», con la
 * motivazione «uscire dalla ricerca non puo' riportare su una cella cieca, perche' quelle non sono piu'
 * candidate».
 *
 * 🔴 **La motivazione non regge, ed e' il difetto.** Uscire dalla ricerca significa `bVedeGia == true`,
 * e con quella condizione il filtro **e' spento**: le celle cieche tornano candidate immediatamente. La
 * sequenza e' un ciclo di periodo due — cella cieca (filtro acceso) -> cella che vede (filtro spento) ->
 * la cieca torna la migliore per punteggio -> cella cieca — e non si ferma mai.
 *
 * Misurato su `L_HexArena` il 2026-08-23: Riktor alterna fra `(1,-1,L0)` e la piattaforma `(3,-3,L1)`
 * **otto volte in dodici turni** in partita, e **trentasette in quaranta** nello scenario
 * `AutoBattle.ArenaV01`, dove la partita non si decide affatto.
 *
 * ⚠️ **L'oracolo e' il RITORNO DI PERIODO DUE**, non i turni fermi: `Cell[t] == Cell[t-2]` con
 * `Cell[t] != Cell[t-1]`. E' la forma minima che distingue un'alternanza da un percorso che ripassa —
 * contare le celle ripetute punirebbe un bot che aggira un ostacolo, che e' esattamente il comportamento
 * che #1287 e' andato a comprare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAuthoredMapNoOscillationTest,
	"RefactorTactics.Match.Autobattle.NobodyOscillatesOnTheAuthoredMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAuthoredMapNoOscillationTest::RunTest(const FString&)
{
	const TCHAR* AssetPath = TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.DA_HexMap_Arena");
	URTHexMapAsset* Authored = Cast<URTHexMapAsset>(StaticLoadObject(URTHexMapAsset::StaticClass(), nullptr, AssetPath));
	if (!TestNotNull(TEXT("la mappa d'autore si carica"), Authored)) { return false; }

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	HexMap->MapAsset = Authored;
	GameMode->MapSource = ERTMapSource::LevelAsset;
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	if (!TestEqual(TEXT("quattro unita' in campo"), RTAuthoredEngagement::LiveUnits(World).Num(), 4))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Le ULTIME DUE celle per unita': tutto cio' che serve a vedere un ciclo di periodo due, e niente di
	// piu' — una storia intera inviterebbe a cercarci dentro cicli piu' lunghi, che sono un'altra domanda.
	TMap<int32, FRTCellId> Ultima;
	TMap<int32, FRTCellId> Penultima;
	TMap<int32, int32> Ritorni;

	const int32 MaxTurni = 12;
	int32 Turni = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < MaxTurni)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		// 🔴 **Il tetto di 400 tick non e' una fine turno, e prima usciva in silenzio.** Esaurito il budget
		// il ciclo terminava lo stesso, le celle venivano lette a turno mezzo applicato e il
		// `LockInAndResolve` successivo si impilava sopra: l'oracolo misurava posizioni che non sono mai
		// state uno stato di fine turno. Che il budget si esaurisca non e' teorico — lo scenario
		// `AutoBattle.ArenaV01` lo documenta da riga di comando. Trovato in code review.
		if (!TestFalse(*FString::Printf(TEXT("il turno %d ha finito di risolvere entro 400 tick"), Turni + 1),
			TM->IsResolving()))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}
		++Turni;

		for (const ARTUnit* U : RTAuthoredEngagement::LiveUnits(World))
		{
			const int32 Id = U->GetUniqueID();
			const FRTCellId* Prev = Ultima.Find(Id);
			const FRTCellId* Prev2 = Penultima.Find(Id);
			if (Prev2 && Prev && *Prev2 == U->Cell && *Prev != U->Cell)
			{
				Ritorni.FindOrAdd(Id) += 1;
			}
			if (Prev) { Penultima.FindOrAdd(Id) = *Prev; }
			Ultima.FindOrAdd(Id) = U->Cell;
		}
	}

	int32 Peggiore = 0;
	for (const TPair<int32, int32>& P : Ritorni) { Peggiore = FMath::Max(Peggiore, P.Value); }

	// 🔴 **La soglia si deriva dai turni GIOCATI, non dalla costante.** Con `MaxTurni / 3` il limite restava
	// 4 anche quando la partita finiva prima, e un ritorno di periodo due e' osservabile solo dal terzo
	// turno: una partita decisa al quinto ne poteva produrre al massimo tre e passava **per aritmetica**.
	// Poiche' lo scopo di questo lavoro e' proprio far decidere prima la partita, il fix avrebbe potuto
	// rendere vacuo il proprio test. Trovato in code review.
	const int32 Limite = FMath::Max(1, Turni / 3);

	// E la premessa va asserita, non sperata: senza abbastanza turni l'oracolo non puo' cadere, e un verde
	// li' non dice niente.
	const int32 TurniMinimi = 6;
	if (!TestTrue(*FString::Printf(
		TEXT("la partita e' durata abbastanza perche' l'oracolo possa cadere (%d turni, minimo %d)"),
		Turni, TurniMinimi), Turni >= TurniMinimi))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	AddInfo(FString::Printf(TEXT("turni giocati: %d  |  piu' ritorni di periodo due su una unita': %d (limite %d)"),
		Turni, Peggiore, Limite));

	TestTrue(*FString::Printf(
		TEXT("nessuna unita' oscilla fra due celle: %d ritorni di periodo due (limite %d)"),
		Peggiore, Limite), Peggiore <= Limite);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
