#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ScenarioHarness/RTScenarioRunner.h"

/**
 * IL MONDO DI PROVA DEI TEST, dichiarato una volta invece che quarantatre'.
 *
 * ## Il duplicato che questo header chiude, misurato
 *
 * Estratto il corpo di ogni funzione di `Tests/*.cpp` che chiama `UWorld::CreateWorld` o
 * `DestroyWorldContext`, tolti commenti e whitespace, raggruppato per corpo:
 *
 *     Make*World      43 file    1 forma
 *     Destroy*World   43 file    2 forme  (41 + 2, la differenza e' un early-return)
 *
 * Quarantatre' file dichiarano la STESSA funzione con quarantatre' nomi diversi. Non e' distrazione: la
 * causa e' scritta nel codice che la subisce —
 *
 *     «Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.»
 *     RTSimulationDeterminismTests.cpp
 *
 * Due namespace anonimi con la stessa funzione collidono quando la unity build li fonde in un'unita' di
 * traduzione sola, e la difesa e' stata rinominare per file. E' corretta e locale; un header `inline` in un
 * namespace NOMINATO scioglie lo stesso vincolo una volta per tutte, perche' e' la stessa entita' e non due
 * omonime.
 *
 * ## Perche' un file nuovo e non `RTOccupancyFixtures.h`
 *
 * Quello e' l'altro — e finora unico — header condiviso di `Tests/`, ma e' geometria di occupancy:
 * `FRTOccupancyPolyline`, i dodici settori, i gemelli di controllo di #620/#621. Il mondo di test non e'
 * quel dominio, e ospitarlo li' renderebbe falso il nome del file.
 *
 * ## Cosa NON sta qui, e perche' (misurato, non dimenticato)
 *
 * · **il caricamento di uno scenario per Id** (`URTScenarioIndex::ResolvePath` + `LoadFromFile`) ricorre in
 *   **7 file di test**, ma i siti di chiamata intrecciano il caricamento con la propria assertion — chi passa
 *   il `FAutomationTestBase&` per fare `AddError`, chi avvolge tutto in un `TestTrue` col proprio messaggio.
 *   Estrarlo cambierebbe *cosa* riporta ciascun test, non solo dove sta scritto: e' un secondo lavoro, con
 *   una decisione da prendere, non una riga da spostare.
 * · **lo spawn di un'unita'** (`Spawn*(UWorld*, ...)` che ritorna `ARTUnit*`) ricorre in **31 file**, con
 *   firme che divergono davvero — alcune prendono l'eroe dal catalogo, altre un `FName`, altre nulla. Un
 *   minimo comune qui sarebbe una funzione con cinque parametri opzionali, cioe' il costo di leggere
 *   l'astrazione al posto del codice.
 *
 * ⚠️ I due numeri qui sopra sono misurati **prima** di questa estrazione, come tutti gli altri di questo
 * commento: dopo, questo header stesso compare nei grep e li sposta di uno. Chi li rilegge li rimisura.
 *
 * Il criterio e' quello del mandato QA Terminal A §2: si estrae il **minimo riutilizzabile**, e si migrano
 * pochi consumatori — non tutti. I 40 file non migrati restano corretti come sono; chi li tocca per un'altra
 * ragione trova questo header gia' pronto.
 */
namespace RTWorldFixtures
{
	/**
	 * Un mondo `Game` vuoto, registrato nel motore.
	 *
	 * `bInformEngineOfWorld = false` di proposito: il mondo di un test non deve comparire fra quelli che il
	 * motore considera vivi, altrimenti un tick globale o un iteratore di mondi lo raccoglierebbe insieme a
	 * quello della sessione. Il `FWorldContext` invece serve — senza, `GetWorld()` non risolve e gli actor
	 * spawnati non trovano il proprio mondo.
	 *
	 * Ritorna `nullptr` se la creazione fallisce: il chiamante lo verifica, perche' un test che prosegue su un
	 * mondo nullo fallisce dove il difetto non e'.
	 */
	inline UWorld* MakeWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	/**
	 * Distrugge il mondo e il suo contesto. Da chiamare SEMPRE: un mondo lasciato in piedi tiene vivi gli
	 * actor della prova precedente, e due esecuzioni identiche lo sarebbero per il motivo sbagliato.
	 *
	 * ⚠️ **E' l'unione delle due forme misurate, non una terza.** Quarantuno file condizionano tutto a
	 * `GEngine`; due distruggono il mondo anche senza. La differenza si vede solo con `GEngine == nullptr`,
	 * che in una run di automation non accade — ma fra le due si tiene quella che non perde il mondo, perche'
	 * un `UWorld` orfano e' una perdita silenziosa e il contesto mancante no.
	 */
	inline void DestroyWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
		World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	}

	/**
	 * Esegue lo scenario in un mondo NUOVO e lo distrugge: nessuno stato sopravvive fra una prova e l'altra.
	 *
	 * E' il percorso reale del gioco — `URTScenarioRunner::Run` entra dagli stessi ingressi del giocatore e
	 * non conosce il resolver — quindi qui non nasce nessun resolver parallelo per i test.
	 *
	 * Misurato prima di questa estrazione: **6 file** ripetevano le stesse tre righe (crea, esegui, distruggi)
	 * prima di divergere sul proprio controllo dell'esito. Il controllo resta dei chiamanti, perche' e' li'
	 * che dice qualcosa; la danza del mondo no.
	 *
	 * Con un mondo non creabile ritorna un `FRTTestResult` di default — `Outcome` vale `Error` — invece di
	 * lanciare: un harness che crasha non sa dire perche'.
	 */
	inline FRTTestResult RunScenarioIsolated(const FRTTestScenario& Scenario)
	{
		UWorld* World = MakeWorld();
		if (!World)
		{
			return FRTTestResult();
		}
		const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
		DestroyWorld(World);
		return Result;
	}
}
