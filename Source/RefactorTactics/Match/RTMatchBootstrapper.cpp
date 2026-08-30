#include "Match/RTMatchBootstrapper.h"

#include "Ability/RTCatalogLibrary.h" // DefaultLoadoutFor: l'equipaggiamento con cui un eroe entra in partita
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/World.h"
#include "Frontend/RTStartupReport.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "RefactorTactics.h" // LogRT
#include "Turn/RTMatchFormatData.h"
#include "Turn/RTMatchFormatLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

/**
 * I passi dell'allestimento.
 *
 * Namespace NOMINATO e non anonimo: la unity build fonde le translation unit, e due `ApplyMapSource`
 * statiche in file diversi collidono. E' lo stesso vincolo che ha costretto quarantatre' file di test a
 * rinominare la propria `MakeWorld`, e la difesa che scala e' il namespace, non il nome piu' lungo.
 */
namespace RTMatchBootstrapDetail
{
	/**
	 * Il minimo che tiene VIVO il turno in una partita non presidiata.
	 *
	 * 🔴 **`0` non significa «turni incatenati»: significa fermo per sempre.** `SetPlanningSeconds` e
	 * `StartPlanningTimer` armano il timer solo `if (PlanningSeconds > 0.f)` — con zero non lo arma nessuno,
	 * `OnPlanningTimeout` non scatta mai, `LockInAndResolve` non viene chiamato, e in una partita non
	 * presidiata **non c'e' nessuno che possa premere il lock-in**. La partita resta al turno 1 mentre la
	 * banda dichiara che si sta giocando da sola.
	 *
	 * ⚠️ Zero e' legittimo altrove, ed e' da li' che veniva la convinzione sbagliata: `RTScenarioSession`
	 * chiama `SetPlanningSeconds(0.f)` per le run headless — ma li' il turno lo pompa l'harness. La
	 * differenza non e' il valore, e' chi fa avanzare il turno.
	 *
	 * ∴ l'intento «il piu' veloce possibile» resta onorato e non viene riportato al ripiego: viene alzato al
	 * minimo che l'orologio del motore sa ancora far scattare, e la correzione e' dichiarata nel log invece
	 * che applicata in silenzio.
	 */
	static constexpr float MinUnattendedPlanningSeconds = 0.1f;

	/** Applica la sorgente mappa all'actor: sostituisce l'asset quando la scelta lo richiede, e lo dichiara. */
	static void ApplyMapSource(ARTHexMapActor* HexMap, const FRTMatchBootstrapConfig& Config,
		FRTStartupReport& Report)
	{
		if (!HexMap)
		{
			return;
		}

		// La FIXTURE vince su tutto il resto, ed e' il piu' specifico dei tre livelli: proprieta' del
		// GameMode, `rt.Map.Source`, e questa. Serve perche' le due arene generabili non portano superfici —
		// `MakeDemoArena` le lascia tutte `Default`, `MakeTestArena` tutte `Rough` — quindi nessuna delle due
		// permette di guardare a schermo se le nove tinte della tavolozza si distinguono (`#1290`).
		if (!Config.MapFixtureId.IsEmpty())
		{
			if (URTHexMapAsset* Fixture = URTMatchSetupLibrary::MakeFixtureArena(HexMap, Config.MapFixtureId))
			{
				HexMap->MapAsset = Fixture;
				HexMap->RebuildInstances();
				UE_LOG(LogRT, Warning, TEXT("[RT] rt.Map.Fixture='%s': uso quella fixture (%d celle). "
					"La mappa del livello e rt.Map.Source sono ignorate."),
					*Config.MapFixtureId, HexMap->MapAsset->NumCells());
				return;
			}

			// Fail-closed sul VALORE, non sulla partita: stessa cura di `rt.Map.Source`. Un nome sbagliato che
			// ripiegasse in silenzio farebbe attribuire un playtest a una board che non era in vigore.
			// ⚠️ I nomi si CHIEDONO, non si riscrivono (`#1459`). Questa riga era il QUARTO elenco a mano
			// delle stesse fixture, e come gli altri tre nominava `DemoArena` — che non ha un ramo nel
			// dispatcher. `rt.Map.Fixture=DemoArena` produceva quindi un warning che elencava fra i nomi validi
			// esattamente quello appena rifiutato.
			UE_LOG(LogRT, Warning,
				TEXT("[RT] rt.Map.Fixture='%s' non e' una fixture nota: ignorata, si prosegue con la sorgente "
					 "configurata. Nomi validi: %s."),
				*Config.MapFixtureId, *FString::Join(URTMatchSetupLibrary::KnownFixtureIds(), TEXT(", ")));
		}

		switch (Config.MapSource)
		{
		case ERTMapSource::GeneratedTestArena:
			// Scelta esplicita: prevale anche su una mappa d'autore presente nel livello. Va dichiarato, non subito.
			HexMap->MapAsset = URTMatchSetupLibrary::MakeTestArena(HexMap);
			HexMap->RebuildInstances();
			UE_LOG(LogRT, Warning, TEXT("[RT] MapSource=GeneratedTestArena: uso la mappa di PROVA generata "
				"(%d celle, con ostacoli, muri, terreno costoso e piattaforma). La mappa del livello e' ignorata."),
				HexMap->MapAsset ? HexMap->MapAsset->NumCells() : 0);
			// CP 46.2: la stessa condizione, in una forma che un widget puo' leggere. E' la **prima riserva
			// di `G13`**, e finora esisteva solo in questa riga di log.
			Report.Add(ERTStartupOutcome::UsingTestArena,
				FString::Printf(TEXT("%d celle"), HexMap->MapAsset ? HexMap->MapAsset->NumCells() : 0));
			return;

		case ERTMapSource::GeneratedDemoArena:
			HexMap->MapAsset = URTMatchSetupLibrary::MakeDemoArena(HexMap, Config.DemoArenaRadius);
			HexMap->RebuildInstances();
			UE_LOG(LogRT, Warning, TEXT("[RT] MapSource=GeneratedDemoArena: uso l'arena di ripiego "
				"(esagono r=%d, %d celle). La mappa del livello e' ignorata."),
				Config.DemoArenaRadius, HexMap->MapAsset ? HexMap->MapAsset->NumCells() : 0);
			Report.Add(ERTStartupOutcome::UsingDemoArena,
				FString::Printf(TEXT("esagono r=%d"), Config.DemoArenaRadius));
			return;

		case ERTMapSource::LevelAsset:
		default:
			break;
		}

		// Mappa del livello. Quello che conta non e' avere un asset, ma avere delle CELLE: un asset assegnato ma
		// VUOTO non allestisce nulla e premere Play mostra una schermata nera senza spiegazione (osservato in PIE su
		// L_DevSandbox, il cui asset si e' ritrovato a 0 celle). Senza una mappa d'autore utilizzabile si ripiega
		// sull'arena demo: meglio un fondo di scena giocabile che il vuoto.
		// Attenzione: qui si tratta solo il caso "nessuna cella". Una mappa d'autore con POCHE celle non viene
		// rimpiazzata: e' un errore dell'autore e glielo si dice, invece di nascondergli la mappa sotto i piedi.
		// COPIA di lavoro della mappa d'autore (CP 8.4): dal terreno dinamico in poi la partita **modifica** le
		// celle (fuoco che si accende e si spegne, acqua che arriva), e modificare l'asset su disco sporcherebbe
		// il contenuto del progetto — in PIE le modifiche resterebbero dopo lo Stop, e due partite di fila non
		// partirebbero dallo stesso campo, cioe' addio determinismo.
		//
		// Le due arene generate non hanno questo problema: `MakeTestArena`/`MakeDemoArena` costruiscono gia' un
		// oggetto nuovo a ogni partita. Qui si allinea il terzo caso agli altri due, invece di aggiungere un
		// secondo modello ("a volte la mappa si puo' modificare, a volte no") che qualcuno prima o poi sbaglierebbe.
		if (HexMap->MapAsset && HexMap->MapAsset->NumCells() > 0)
		{
			HexMap->MapAsset = DuplicateObject<URTHexMapAsset>(HexMap->MapAsset, HexMap);
		}

		if ((!HexMap->MapAsset || HexMap->MapAsset->NumCells() == 0) && Config.DemoArenaRadius > 0)
		{
			HexMap->MapAsset = URTMatchSetupLibrary::MakeDemoArena(HexMap, Config.DemoArenaRadius);
			HexMap->RebuildInstances();
			UE_LOG(LogRT, Warning,
				TEXT("[RT] Mappa esagonale del livello assente o senza celle: uso un'arena di ripiego "
					 "(esagono r=%d, %d celle). Posa un ARTHexMapActor con un MapAsset popolato per giocare su una "
					 "mappa d'autore."),
				Config.DemoArenaRadius, HexMap->MapAsset ? HexMap->MapAsset->NumCells() : 0);
			Report.Add(ERTStartupOutcome::LevelMapMissing,
				FString::Printf(TEXT("arena di ripiego r=%d"), Config.DemoArenaRadius));
		}
	}

	/**
	 * Risolve il formato di partita e lo consegna al `TurnManager`. Ritorna **false** se il formato e'
	 * presente ma invalido, oppure se non e' compatibile con la classe della mappa (CP 19.1) — in quel caso
	 * la partita non va allestita.
	 *
	 * E' qui che vive la politica di ripiego: la libreria pura rifiuta e basta, l'allestimento decide che
	 * farne e lo dichiara in Warning. Un ripiego silenzioso non produce una partita rotta, produce numeri di
	 * playtest attribuiti a un formato che non era in vigore.
	 *
	 * `OutRules` esce valorizzato solo quando il ritorno e' `true`: l'allestimento legge da li' la
	 * composizione (`UnitsPerTeam`, CP 19.2) invece di dichiararla per conto proprio.
	 */
	static bool ApplyMatchFormat(ARTTurnManager* TurnManager, const URTHexMapAsset* Map,
		const FRTMatchBootstrapConfig& Config, FRTStartupReport& Report, FRTMatchRules& OutRules)
	{
		FRTMatchRules Rules;
		FString Reason;

		if (Config.MatchFormat)
		{
			if (!URTMatchFormatLibrary::ResolveRules(Config.MatchFormat, Rules, Reason))
			{
				// Contenuto sbagliato: si rifiuta, non si ripiega. Un formato invalido silenziosamente sostituito
				// dal ripiego farebbe girare la partita con regole diverse da quelle che il designer ha scritto.
				UE_LOG(LogRT, Error,
					TEXT("[RT] Formato di partita '%s' NON valido: %s. Partita non allestita: correggi l'asset "
						 "oppure lascia MatchFormat vuoto per giocare con il formato di ripiego."),
					*GetNameSafe(Config.MatchFormat), *Reason);
				// CP 46.2: fatale. `Reason` e' gia' prodotto dal validator — si **trasporta**, non si ricompone.
				Report.Add(ERTStartupOutcome::FormatAssetInvalid, Reason);
				return false;
			}
		}
		else if (const URTMatchFormatData* Shipped = URTMatchFormatLibrary::FindShippedFormat(Config.ShippedFormatId))
		{
			// Nessun asset, ma un formato SPEDITO con quell'identita': si gioca quello. E' la stessa strada con
			// cui eroi e azioni arrivano in partita senza che nessuno debba creare un `.uasset` in editor, ed e'
			// cio' che separa una build pacchettizzata «che gira» da una «che gioca il formato della release».
			if (!URTMatchFormatLibrary::ResolveRules(Shipped, Rules, Reason))
			{
				// Un formato spedito che non passa il proprio validator e' un difetto di CODICE, non di dato:
				// rifiutare e' l'unica risposta onesta, perche' ripiegare lo nasconderebbe fino al playtest.
				UE_LOG(LogRT, Error,
					TEXT("[RT] Il formato spedito '%s' non e' valido: %s. Partita non allestita."),
					*Config.ShippedFormatId.ToString(), *Reason);
				Report.Add(ERTStartupOutcome::ShippedFormatInvalid, Reason);
				return false;
			}
			UE_LOG(LogRT, Log,
				TEXT("[RT] Nessun MatchFormat assegnato: uso il formato SPEDITO '%s'. Assegna un "
					 "URTMatchFormatData al GameMode per sovrascriverlo."),
				*Rules.FormatId.ToString());
		}
		else
		{
			Rules = URTMatchFormatLibrary::MakeFallbackRules();
			UE_LOG(LogRT, Warning,
				TEXT("[RT] Nessun MatchFormat assegnato e nessun formato spedito per '%s': uso il RIPIEGO '%s' "
					 "(RoundLimit %d, soglia obiettivo %d). Assegna un URTMatchFormatData al GameMode per giocare "
					 "un formato dichiarato: le misure di playtest vanno attribuite al formato giusto."),
				*Config.ShippedFormatId.ToString(), *Rules.FormatId.ToString(), Rules.RoundLimit, Rules.ScoreToWin);
			// CP 46.2. ⚠️ **Ramo raro, e vale la pena dire perche'**: ci si arriva solo se non esiste nemmeno un
			// formato **spedito** — e `Format.Skirmish2v2` e' spedito da C++ dal commit `9f44570d`. In una build
			// normale questa riga non scatta, ed e' giusto cosi'.
			// 🔴 Una stesura precedente la chiamava «seconda riserva di `G13`»: **falso**. Le due riserve sono
			// l'arena di test e il fatto che *«la via a punti non e' mai stata esercitata, perche' la soglia
			// obiettivo e' 0»* — che e' un valore del formato, non il formato di ripiego. Connessione plausibile
			// e sbagliata, trovata da un test rosso.
			Report.Add(ERTStartupOutcome::UsingFallbackFormat, Rules.FormatId.ToString());
		}

		// CP 19.1: l'accoppiata formato/mappa si verifica QUI, prima di schierare. Un 3v3 Standard su una mappa
		// disegnata per il 2v2 non e' una partita piu' stretta, e' una partita sbagliata — e scoprirlo al terzo
		// turno costa un playtest. Vale anche per il ripiego: se il livello porta una mappa Operations, il 2v2 di
		// ripiego non e' la partita giusta da avviarci sopra.
		const TArray<FString> Mismatch = URTMatchFormatLibrary::ValidateAgainstMap(Rules, Map);
		if (Mismatch.Num() > 0)
		{
			UE_LOG(LogRT, Error,
				TEXT("[RT] Formato e mappa non combaciano: %s. Partita non allestita: assegna una mappa della "
					 "classe richiesta, oppure un formato disegnato per questa mappa."),
				*FString::Join(Mismatch, TEXT("; ")));
			Report.Add(ERTStartupOutcome::FormatMapMismatch, FString::Join(Mismatch, TEXT("; ")));
			return false;
		}

		OutRules = Rules;

		if (!TurnManager)
		{
			// Le regole non hanno destinatario: la partita girerebbe senza limite di round e nessuno lo saprebbe.
			UE_LOG(LogRT, Warning,
				TEXT("[RT] Nessun ARTTurnManager nel livello: il formato '%s' non e' stato applicato."),
				*Rules.FormatId.ToString());
			// Degradato e non fatale: `return true` — la partita prosegue, ma senza limite di round e nessuno
			// lo saprebbe. E' esattamente il caso che il banner esiste per rendere visibile.
			Report.Add(ERTStartupOutcome::NoTurnManager, Rules.FormatId.ToString());
			return true;
		}

		TurnManager->SetMatchRules(Rules);
		UE_LOG(LogRT, Log,
			TEXT("[RT] Formato di partita in vigore: '%s' (RoundLimit %d, soglia obiettivo %d, %d unita' per squadra)"),
			*Rules.FormatId.ToString(), Rules.RoundLimit, Rules.ScoreToWin, Rules.UnitsPerTeam);
		return true;
	}

	/**
	 * Spawna l'eroe con l'`HeroId` dato. `Hero == nullptr` non spawna nulla (fail-closed): un'unita' con
	 * statistiche di default al posto di un eroe sarebbe piu' difficile da diagnosticare di un'unita' assente.
	 */
	static ARTUnit* SpawnHero(UWorld* World, int32 TeamId, const URTHeroData* Hero,
		const FRTMatchBootstrapConfig& Config, const FRTCellId& InCell, const FVector& Origin,
		float HexSize, float LayerHeight)
	{
		if (!World || Hero == nullptr)
		{
			return nullptr; // fail-closed: senza dati non si spawna un'unita' con statistiche inventate
		}

		// Classe visiva per eroe: se assegnata (BP_Unit con skeletal) usala, altrimenti fallback al cilindro C++.
		// E' il comportamento di ripiego di sempre, ora per HeroId invece che per archetipo.
		const TSubclassOf<ARTUnit>* Configured = Config.HeroUnitClasses.Find(Hero->HeroId);
		UClass* UnitClass = (Configured && *Configured) ? Configured->Get() : ARTUnit::StaticClass();

		// Deferred: team e statistiche PRIMA di BeginPlay, cosi' colore e dati sono corretti al primo frame.
		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(UnitClass, FTransform::Identity);
		if (Unit)
		{
			Unit->TeamId = TeamId;
			// Team 1 al bot: e' il default di sempre, pinnato da `RTHeroSpawnTests`. La modalita' non presidiata
			// (#954) lo ESTENDE — mette al bot anche la squadra 0 — e non lo sostituisce: con l'autobattle spento
			// questa riga vale esattamente quanto valeva prima.
			//
			// Legge la decisione LATCHATA e non il resolver: le quattro unita' di una partita devono ricevere la
			// stessa risposta, e una console variable digitata fra uno spawn e l'altro produrrebbe una squadra
			// mista. Vale anche per il costo — questa riga gira una volta per unita'.
			Unit->bIsBotControlled = (TeamId == 1) || Config.bAutobattle;
			Unit->ConfigureFromHeroData(Hero);

			// EQUIPAGGIAMENTO (`#1054`, CP 7.4). Fino al 2026-08-16 nessuno equipaggiava: `DefaultLoadoutFor`
			// aveva chiamanti solo nei test, e la spinta di 2 di `Weapon.Impact` esisteva nei dati e in nessuna
			// partita — la ragione misurata per cui `Guard` e `Brace` non si distinguevano a video (`#403`).
			//
			// ⚠️ **Qui e non in `ConfigureFromHeroData`**, che e' la copia dei DATI dell'eroe: metterla la'
			// equipaggerebbe anche le decine di unita' che i test unitari costruiscono per avere «un'unita'
			// qualunque», cambiando sotto i piedi misure che non parlano di equipaggiamento.
			//
			// ⚠️ **E nessun ramo per il bot**: questa funzione e' l'ingresso dello spawn di partita ed e'
			// dove si decide `bIsBotControlled` due righe sopra, quindi le due squadre che passano di qui sono
			// allestite dallo stesso codice. Chi altro scrive quel campo: `ARTUnit::bIsBotControlled`.
			// Un `if (!bIsBotControlled)` sarebbe la forma in cui «il bot gioca un altro gioco» rientra.
			//
			// `DefaultLoadoutFor` risponde VUOTO per un eroe i cui pezzi non sono spediti — oggi Gadget e
			// Wraith, che §4 assegna a due gadget che v0.1 non costruisce — e un array vuoto qui non fa nulla.
			Unit->EquipLoadout(URTCatalogLibrary::DefaultLoadoutFor(Hero->HeroId));

			UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
			Unit->PlaceOnCell(InCell, Origin, HexSize, LayerHeight);
		}
		return Unit;
	}
}

FRTMatchBootstrapOutcome FRTMatchBootstrapper::Bootstrap(ARTHexMapActor* HexMap, ARTTurnManager* TurnManager,
	const FRTMatchBootstrapConfig& Config, FRTStartupReport& Report)
{
	using namespace RTMatchBootstrapDetail;

	FRTMatchBootstrapOutcome Outcome;
	if (!HexMap)
	{
		return Outcome;
	}

	UWorld* World = HexMap->GetWorld();
	if (!World)
	{
		return Outcome;
	}

	Report.Phase = ERTLoadPhase::Map;
	ApplyMapSource(HexMap, Config, Report);

	// Le regole di formato prima delle unita': se il formato e' invalido non si allestisce nulla, e la mappa
	// resta a schermo con il motivo nel log (stesso trattamento delle celle di partenza insufficienti).
	Report.Phase = ERTLoadPhase::Scenario;
	FRTMatchRules Rules;
	if (!ApplyMatchFormat(TurnManager, HexMap->MapAsset, Config, Report, Rules))
	{
		// ⚠️ La fase resta a `Scenario`, non torna a `Idle`: **dove** ci si e' fermati e' l'informazione
		// che serve a chi guarda. Il fatale e' gia' nelle note.
		return Outcome;
	}

	Report.Phase = ERTLoadPhase::Bots;

	// LA MODALITA' SI DECIDE QUI, una volta, PRIMA che le unita' entrino in campo — vedi
	// `ARTGameMode::IsAutobattleInEffect()`. Da questo punto in poi la sessione ha una risposta sola, e la
	// banda non puo' piu' descrivere una partita diversa da quella che si sta giocando.
	//
	// 🔑 **Il valore arriva gia' risolto e questa funzione lo DICHIARA latchato**: la precedenza fra le tre
	// sorgenti l'ha decisa chi ordina; cio' che appartiene all'allestimento e' *quando* quella decisione
	// smette di poter cambiare, ed e' esattamente questa riga.
	Outcome.bModeLatched = true;
	Outcome.bAutobattleInEffect = Config.bAutobattle;

	// RITMO DEL TURNO, prima del ritorno anticipato qui sotto: e' configurazione del turno, non
	// dell'allestimento, e vale anche su un livello che porta gia' le proprie unita' — dove l'allestimento
	// non interviene ma il Planning e' comunque quello con cui si giochera'.
	if (TurnManager)
	{
		float PlanningSeconds = Config.PlanningSeconds;

		// Zero fermerebbe la partita per sempre invece di incatenare i turni: nessuno arma il timer, e qui
		// non c'e' una mano umana che possa premere il lock-in. Vedi `MinUnattendedPlanningSeconds`.
		if (Config.bAutobattle && PlanningSeconds == 0.f)
		{
			UE_LOG(LogRT, Warning,
				TEXT("[RT] AUTOBATTLE: Planning 0s bloccherebbe la partita al turno 1 — nessun timer viene "
					 "armato e non c'e' nessuno che possa chiudere il turno. Alzato a %.2fs."),
				MinUnattendedPlanningSeconds);
			PlanningSeconds = MinUnattendedPlanningSeconds;
		}

		if (PlanningSeconds >= 0.f)
		{
			TurnManager->SetPlanningSeconds(PlanningSeconds);
		}

		// #971 — stessa natura e stesso posto: e' configurazione del turno, non allestimento, e sta prima
		// del ritorno anticipato perche' vale anche su un livello che porta gia' le proprie unita'. Il
		// TurnManager non interroga il GameMode (vedi `SetUnattendedSession`): viene informato qui, dove la
		// modalita' e' appena stata latchata.
		TurnManager->SetUnattendedSession(Config.bAutobattle);
	}

	// L'ATTIVAZIONE NON E' SILENZIOSA. Chi guarda vede le proprie unita' muoversi da sole, e la spiegazione
	// non deve stare solo in una riga di log che non si ha motivo di andare a cercare: la banda a schermo la
	// dichiara (`GetScenarioBannerText`), e questa riga la mette anche nel log con la FONTE — perche' una
	// console variable impostata una volta resta attiva a ogni Play successivo, e senza saperlo si cerca il
	// difetto nella proprieta' sbagliata.
	if (Config.bAutobattle)
	{
		UE_LOG(LogRT, Warning,
			TEXT("[RT] AUTOBATTLE (da: %s): entrambe le squadre al bot, nessun input richiesto. "
				 "Planning %.2fs — questa NON e' una partita normale."),
			*Config.AutobattleSourceLabel, TurnManager ? TurnManager->GetPlanningSeconds() : -1.f);
	}

	// Il livello puo' avere unita' gia' posate a mano: in quel caso l'allestimento automatico non interviene.
	if (UGameplayStatics::GetActorOfClass(World, ARTUnit::StaticClass()))
	{
		// ...ma la MODALITA' si applica lo stesso, e questo ramo e' l'unico posto in cui puo' farlo.
		//
		// 🔴 Su un livello con unita' proprie l'allestimento ritorna prima di arrivare a `SpawnHero`, quindi
		// quelle unita' tengono il valore che si portano da sole — il default della loro dichiarazione: il
		// log dichiarava «entrambe le squadre al bot» mentre la squadra 0 non pianificava nessuno, e la
		// partita macinava turni vuoti fino al `RoundLimit` con la banda che asseriva il contrario. Trovato
		// in code review: il blocco del Planning era gia' stato spostato sopra questo ritorno *per questo
		// scenario*, l'assegnazione no.
		//
		// ⚠️ La riga che stava qui dichiarava `SpawnHero` **unico** sito di scrittura di `bIsBotControlled`,
		// e non lo era. Chi lo scrive e' elencato alla dichiarazione del campo, `ARTUnit::bIsBotControlled`:
		// qui non si duplica.
		if (Config.bAutobattle)
		{
			TArray<AActor*> Existing;
			UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Existing);
			int32 Switched = 0;
			for (AActor* Actor : Existing)
			{
				ARTUnit* Unit = Cast<ARTUnit>(Actor);
				if (Unit && !Unit->bIsBotControlled)
				{
					Unit->bIsBotControlled = true;
					++Switched;
				}
			}
			UE_LOG(LogRT, Warning,
				TEXT("[RT] AUTOBATTLE su unita' gia' presenti nel livello: %d di %d passate al bot. "
					 "L'allestimento automatico non interviene, la modalita' si'."),
				Switched, Existing.Num());
		}
		return Outcome;
	}

	// La composizione la dichiara il FORMATO (CP 19.2), non l'orchestratore: finche' il `2` viveva qui, «2v2»
	// era una proprieta' del codice di allestimento, e lo stress 4v4 di E17 doveva essere un caso speciale del
	// `GameMode` invece di un formato che dichiara 4.
	const URTHexMapAsset* Map = HexMap->MapAsset;
	const int32 CellsNeeded = Rules.UnitsPerTeam * 2;
	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, Rules.UnitsPerTeam, /*Layer=*/ 0);
	if (Start.Num() != CellsNeeded)
	{
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Mappa esagonale senza celle percorribili sufficienti: il formato '%s' ne chiede %d "
				 "(%d per squadra) e la mappa ne offre %d. Partita non allestita."),
			*Rules.FormatId.ToString(), CellsNeeded, Rules.UnitsPerTeam, Start.Num());
		return Outcome;
	}

	// Contesto geometrico dall'unica fonte (scala dall'asset autorevole, origine dall'actor).
	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	HexMap->GetHexContext(Origin, HexSize, LayerHeight);

	// Il roster del catalogo eroi (CP 6.2-6.5), non piu' i due archetipi hard-coded. Le formazioni sono un
	// dato (`Team0Heroes`/`Team1Heroes`): qui si legge chi gioca, non si decide.
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	auto FindHero = [&Roster](const FName& HeroId) -> const URTHeroData*
	{
		for (const URTHeroData* Hero : Roster)
		{
			if (Hero && Hero->HeroId == HeroId) { return Hero; }
		}
		return nullptr;
	};

	// Un eroe non puo' stare in due posti: la stessa istanza spawnata due volte condividerebbe le azioni
	// (`Abilities` e' un array di puntatori), e due unita' finirebbero per ricaricare la stessa abilita'.
	TSet<FName> Spawned;
	int32 CellIndex = 0;
	const TArray<const TArray<FName>*> Formations = { &Config.Team0Heroes, &Config.Team1Heroes };

	// La formazione deve dichiarare tanti eroi quanti il formato ne schiera (CP 19.2). Senza questo controllo
	// il formato direbbe 4 e il campo ne vedrebbe 2: la partita girerebbe, e il numero dichiarato sarebbe un
	// dato che nessuno onora — il difetto ricorrente di questo repository.
	for (int32 TeamId = 0; TeamId < Formations.Num(); ++TeamId)
	{
		if (Formations[TeamId]->Num() != Rules.UnitsPerTeam)
		{
			UE_LOG(LogRT, Error,
				TEXT("[RT] Il formato '%s' schiera %d unita' per squadra, ma la formazione della squadra %d ne "
					 "dichiara %d. Partita non allestita: allinea Team%dHeroes al formato, o il formato alla "
					 "formazione."),
				*Rules.FormatId.ToString(), Rules.UnitsPerTeam, TeamId, Formations[TeamId]->Num(), TeamId);
			return Outcome;
		}
	}

	// Le formazioni si risolvono TUTTE prima che entri in campo qualcuno (#1069). Prima la guardia stava
	// dentro il ciclo di spawn e faceva `continue`: un nome sbagliato produceva una partita allestita a
	// META', con le unita' risolte in campo e le altre no — e a schermo sembrava una partita normale.
	// E' lo stesso dato che il controllo del conteggio qui sopra protegge dall'altro lato, e riceve lo
	// stesso trattamento: `Error` e nessuna unita' spawnata.
	TArray<TArray<const URTHeroData*>> Lineups;
	for (int32 TeamId = 0; TeamId < Formations.Num(); ++TeamId)
	{
		TArray<const URTHeroData*>& Lineup = Lineups.AddDefaulted_GetRef();
		for (const FName& HeroId : *Formations[TeamId])
		{
			const URTHeroData* Hero = FindHero(HeroId);
			if (Hero == nullptr)
			{
				UE_LOG(LogRT, Error,
					TEXT("[RT] '%s' non e' nel catalogo eroi: partita non allestita. Correggi Team%dHeroes, "
						 "o aggiungi l'eroe al catalogo."),
					*HeroId.ToString(), TeamId);
				Report.Add(ERTStartupOutcome::RosterHeroMissing, HeroId.ToString());
				return Outcome;
			}
			Lineup.Add(Hero);
		}
	}

	for (int32 TeamId = 0; TeamId < Formations.Num(); ++TeamId)
	{
		for (int32 Slot = 0; Slot < Lineups[TeamId].Num(); ++Slot)
		{
			const FName& HeroId = (*Formations[TeamId])[Slot];
			if (CellIndex >= Start.Num())
			{
				UE_LOG(LogRT, Warning, TEXT("[RT] Celle di partenza insufficienti: %s non entra in campo"),
					*HeroId.ToString());
				continue;
			}
			if (Spawned.Contains(HeroId))
			{
				UE_LOG(LogRT, Warning, TEXT("[RT] %s e' schierato due volte: la seconda copia e' ignorata"),
					*HeroId.ToString());
				continue;
			}

			SpawnHero(World, TeamId, Lineups[TeamId][Slot], Config, Start[CellIndex], Origin, HexSize, LayerHeight);
			Spawned.Add(HeroId);
			++CellIndex;
		}
	}

	UE_LOG(LogRT, Log, TEXT("[RT] Board 2v2 esagonale avviata su %d celle con %d eroi"),
		Map ? Map->NumCells() : 0, Spawned.Num());

	// CP 46.2: allestimento concluso. `Ready` **non** significa «senza problemi» — le note degradate
	// restano, ed e' proprio la combinazione «pronto, ma con due ripieghi» il caso di `G13`.
	Report.Phase = ERTLoadPhase::Ready;
	Outcome.bUnitsSpawned = true;
	return Outcome;
}
