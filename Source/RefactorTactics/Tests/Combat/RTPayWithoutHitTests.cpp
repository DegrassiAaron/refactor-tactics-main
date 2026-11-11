#include "Misc/AutomationTest.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/Actions/RTActionQueueLibrary.h"    // SortUnitsForResolution: la premessa sull'ordine in cui il Blast passa le unita' (#2922)
#include "Turn/Log/RTMatchStateHash.h"        // HashMatchState: lo stato finale identico se cambia solo l'ordine di creazione degli Actor
#include "Turn/RTTurnManager.h"
#include "Turn/Log/RTTurnLog.h"
#include "Turn/Actions/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo nella voce di fallback
#include "Turn/Reaction/RTReactionLibrary.h"       // ERTReactionOutcome: la reazione armata che non scatta (RIC-3)
#include "Player/RTPlayerController.h"    // l'ORACOLO del click: DispatchUnitClickForTest (RIC-3)
#include "Unit/RTUnit.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Tests/Shared/RTAbilityFixtures.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * **«Se si usa un'abilita' si paga»** — decisione d'autore del 2026-10-31 su [D-414] (`#3134`).
 *
 * Un'azione con ricarica che l'unita' USA — un piano che il click avrebbe accettato, ricarica e slot a posto —
 * consuma la ricarica anche se la risoluzione non produce nessun colpo. Cio' che il click rifiuta (fuori portata
 * dichiarata, distanza minima) resta gratis, come [D-200] e [D-414] punto 2 dicono all'unisono.
 *
 * ⚠️ **La SEDE della scrittura non cambia**: `SpendStartedAbilities` per il Blast, `ConsumeAbility` nei siti di Prep
 * e Cleanup. Spostarla al piano definitivo e' il punto 3 di [D-414] ed e' un fatto di replay, non deciso.
 *
 * Prefisso `Paga*` negli helper: unity build, namespace anonimi fusi con gli altri file di test.
 */
namespace
{
	UWorld* PagaMakeWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void PagaDestroyWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Mappa esagonale di prova: muri alla vista, celle di Fumo e piattaforme in piu' sono dichiarati dalla riga. */
	ARTHexMapActor* PagaSpawnMap(UWorld* World, int32 Radius, const TArray<FRTCellId>& SightBlockers,
		const TArray<FRTCellId>& SmokeCells, const TArray<FRTCellId>& ExtraCells)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			FRTHexCellData Data(Id);
			Data.bBlocksLineOfSight = SightBlockers.Contains(Id);
			if (SmokeCells.Contains(Id)) { Data.Surface = ERTHexSurface::Smoke; }
			M->AddOrUpdateCell(Data);
		}
		for (const FRTCellId& Id : ExtraCells)
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* PagaSpawnUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo: il test misura il Blast, non il movimento
		U->Shield = 0;         // [D-224]: qui si misura se un colpo arriva, non quanto scudo assorbe
		return U;
	}

	void PagaRunTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/**
	 * Abbastanza da sopravvivere al decremento del Cleanup dello stesso turno e restare leggibile: con un cooldown
	 * da 1 il Cleanup lo riporta a zero prima che il test guardi, e un'asserzione «non ha pagato» sarebbe verde
	 * anche col difetto (la trappola gia' pagata da `#1445`).
	 */
	constexpr int32 PagaCooldown = 3;

	/** Il pagamento si legge sul contatore (cio' che `ConsumeAbility` scrive), mai dedotto da `CanUseAbility`. */
	bool PagaHaPagato(const ARTUnit* Unit, int32 AbilityIndex)
	{
		return Unit && Unit->GetAbilityCooldown(AbilityIndex) > 0;
	}

	// ---------------------------------------------------------------------------------------------
	// TABELLA DELLE CAUSE: una riga per ogni modo in cui un'azione d'attacco USATA non produce un colpo.
	// ---------------------------------------------------------------------------------------------

	enum class EPagaCausa : uint8
	{
		Colpisce,               //< controllo positivo: l'attacco colpisce, e paga come prima
		AreaVuota,              //< R1  area d'attacco in portata che non trova nessuno
		AreaVuotaAlBordo,       //< R1  come sopra, a distanza ESATTAMENTE pari alla portata
		BersaglioScattato,      //< R3  bersaglio-unita' scattato altrove prima del Blast
		LineaBloccata,          //< R4  linea di tiro bloccata a risoluzione (BlockedIntents)
		AltroPiano,             //< R5  mira su un altro piano (OtherLayerIntents)
		BersaglioMorto,         //< R6  bersaglio-unita' gia' morto prima del Blast (TargetDead)
		TiroCappatoDalFumo,     //< R7  tiro a cella cappato da un Fumo NON osservato (TerrainCappedIntents): il Fumo e' in (3,0,0), fuori dall'intorno di chi tira (vista 0), la mira alla portata
		TiroCappatoDalFumoOsservato, //< controllo di R7: lo STESSO cap ma con il Fumo in (1,0,0), OSSERVATO: il click lo vede e rifiuta
		TiroCappatoInterrotto,  //< R7+R8  il tiro cappato dal Fumo NON osservato, annullato da Action.Interrupt: paga come R8 (la classificazione sta PRIMA di ApplyInterrupts)
		InterrottaSenzaColpi,   //< R8  azione in portata annullata da Action.Interrupt, senza colpi
		InterruptBloccato,      //< R4  l'INTERRUPT come attore: il bersaglio-unita' sta dietro un muro (confermato dall'autore il 2026-11-01)
		NonAggressivaBloccata,  //< la clausola `bCountsAsAttack` di `IntentoPartitoSenzaColpo`: bloccata dal muro, NON paga
		NonAggressivaBersaglioMorto, //< la guardia `bCountsAsAttack` di R6: bersaglio morto, NON paga
		CellaOltrePortata,      //< R9  tiro a CELLA fuori portata dichiarata: il click lo rifiuta
		TroppoVicino,           //< R9  sotto la distanza minima (mortaio): il click lo rifiuta
		MortaioAlMinimo,        //< controllo di R9: alla distanza minima esatta lo STESSO mortaio paga
		AttaccanteGiaCaduto,    //< R10 chi attacca e' morto prima del Blast
		AttaccanteCadeNelBlast, //< R10 chi attacca muore nel Blast
	};

	struct FPagaCase
	{
		const TCHAR* Nome;
		EPagaCausa Causa;
		bool bDevePagare;
		const TCHAR* Perche;
	};

	/** Che cosa la riga dichiara di vedere nel turno: le PREMESSE, senza le quali il verde potrebbe misurare un'altra scena. */
	struct FPagaAttesi
	{
		int32 Interrotte = 0;     //< voci Fallback/Cancelled `Interrupted` di chi attacca
		int32 AltroPiano = 0;     //< voci Fallback/Cancelled `OtherLayer`
		int32 BersaglioMorto = 0; //< voci Fallback/Cancelled `TargetDead`
		int32 FuoriPortata = 0;   //< voci Fallback/Cancelled `OutOfRange`
		int32 NoLos = 0;          //< voci Combat `NoLineOfSight` (quelle non portano l'unita')
		bool bDanno = false;      //< il bersaglio incassa danno
		bool bAttaccanteVivo = true;
	};

	FPagaAttesi PagaAttesiDellaRiga(EPagaCausa Causa)
	{
		FPagaAttesi A;
		switch (Causa)
		{
		case EPagaCausa::Colpisce:              A.bDanno = true; break;
		case EPagaCausa::LineaBloccata:         A.NoLos = 1; break;
		case EPagaCausa::InterruptBloccato:     A.NoLos = 1; break;
		case EPagaCausa::NonAggressivaBloccata: A.NoLos = 1; break;
		case EPagaCausa::NonAggressivaBersaglioMorto: A.BersaglioMorto = 1; break;
		case EPagaCausa::AltroPiano:            A.AltroPiano = 1; break;
		case EPagaCausa::BersaglioMorto:        A.BersaglioMorto = 1; break;
		case EPagaCausa::TiroCappatoDalFumo:    A.FuoriPortata = 1; break;
		case EPagaCausa::TiroCappatoDalFumoOsservato: A.FuoriPortata = 1; break;
		case EPagaCausa::InterrottaSenzaColpi:  A.Interrotte = 1; break;
		case EPagaCausa::TiroCappatoInterrotto: A.Interrotte = 1; break; // e nessuna voce OutOfRange: `ApplyInterrupts` toglie il tiro dal canale
		case EPagaCausa::AttaccanteGiaCaduto:   A.bAttaccanteVivo = false; break;
		case EPagaCausa::AttaccanteCadeNelBlast: A.bAttaccanteVivo = false; break;
		default: break; // AreaVuota*, BersaglioScattato, CellaOltrePortata, TroppoVicino, MortaioAlMinimo: nessuna voce
		}
		return A;
	}

	struct FPagaEsito
	{
		bool bValido = false;
		bool bHaPagato = false;
		int32 Cooldown = 0;
		int32 Interrotte = 0;
		int32 AltroPiano = 0;
		int32 BersaglioMorto = 0;
		int32 FuoriPortata = 0;
		int32 AltreVoci = 0;   //< ogni altro Fallback/Cancelled di chi attacca: nessuna riga deve averne
		int32 NoLos = 0;
		bool bDanno = false;
		bool bAttaccanteVivo = true;
		bool bIdentitaDistinte = false; //< tutte le unita' del banco hanno uno StableUnitId non nullo e diverso
		bool bBersaglioSpostato = true; //< solo `BersaglioScattato`: il bersaglio ha davvero lasciato la cella mirata
	};

	/** Gira UN turno della riga e ne riporta cio' che le asserzioni leggono. */
	FPagaEsito PagaGiraRiga(FAutomationTestBase& Test, const FPagaCase& C)
	{
		FPagaEsito Esito;
		UWorld* World = PagaMakeWorld();
		if (!Test.TestNotNull(TEXT("world di prova"), World)) { return Esito; }

		const FRTCellId CellaAlBersaglio(3, 0, 0);
		const FRTCellId CellaDelFumo(1, 0, 0);          // OSSERVATA: nell'intorno di chi tira
		const FRTCellId CellaDelFumoNonOsservata(3, 0, 0); // a distanza 3: fuori dall'intorno di 2 celle, con la vista a 0
		const FRTCellId CellaSuAltroPiano(3, 0, 1);
		TArray<FRTCellId> Muri;
		TArray<FRTCellId> Fumo;
		TArray<FRTCellId> InPiu;
		if (C.Causa == EPagaCausa::LineaBloccata || C.Causa == EPagaCausa::InterruptBloccato
			|| C.Causa == EPagaCausa::NonAggressivaBloccata)
		{
			Muri = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0) };
		}
		if (C.Causa == EPagaCausa::TiroCappatoDalFumo || C.Causa == EPagaCausa::TiroCappatoInterrotto)
		{
			Fumo = { CellaDelFumoNonOsservata };
		}
		if (C.Causa == EPagaCausa::TiroCappatoDalFumoOsservato) { Fumo = { CellaDelFumo }; }
		if (C.Causa == EPagaCausa::AltroPiano) { InPiu = { CellaSuAltroPiano }; }
		PagaSpawnMap(World, /*Radius=*/ 9, Muri, Fumo, InPiu);

		// Il mortaio e' di Branth, tutto il resto dell'attacco base di Ivrin.
		const bool bMortaio = C.Causa == EPagaCausa::TroppoVicino || C.Causa == EPagaCausa::MortaioAlMinimo;
		ARTUnit* Attaccante = PagaSpawnUnit(World, 0,
			bMortaio ? URTHeroCatalogLibrary::MakeBranth() : URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
		// Un nemico lontano in ogni banco: la squadra avversa non e' mai vuota, e non entra nella portata di nessuno.
		ARTUnit* Spettatore = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-5, 0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: attaccante"), C.Nome), Attaccante)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: spettatore"), C.Nome), Spettatore)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: TM"), C.Nome), TM))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		TArray<TWeakObjectPtr<ARTUnit>> Tutti = { Attaccante, Spettatore };
		// R7 con il Fumo NON osservato: chi tira vede solo l'intorno di 2 celle (vista 0), quindi il Fumo in (3,0,0) e' una cella
		// che la squadra non conosce. Il tiro a (Portata,0,0) e' dentro la portata dichiarata e oltre il cap del Fumo (2).
		if (C.Causa == EPagaCausa::TiroCappatoDalFumo || C.Causa == EPagaCausa::TiroCappatoInterrotto)
		{
			Attaccante->VisionRange = 0;
		}

		// L'abilita' d'attacco e il suo cooldown, reso OSSERVABILE: l'attacco base dell'eroe non ne dichiara uno.
		int32 Idx = 0;
		if (bMortaio)
		{
			Idx = INDEX_NONE;
			for (int32 i = 0; i < Attaccante->NumAbilities(); ++i)
			{
				const URTActionData* A = Attaccante->GetAbility(i);
				if (A && (A->Def.DerivedFromActionId == FName(TEXT("Action.Mortar"))
					|| A->Def.ActionId == FName(TEXT("Action.Mortar"))))
				{
					Idx = i;
					break;
				}
			}
			if (!Test.TestTrue(*FString::Printf(TEXT("%s: il kit di Branth contiene il mortaio"), C.Nome),
				Idx != INDEX_NONE))
			{
				PagaDestroyWorld(World);
				return Esito;
			}
		}
		if (C.Causa == EPagaCausa::InterruptBloccato)
		{
			// L'Interrupt prende lo SLOT 0 (l'array dei cooldown lo copre) e la portata sale a 3: a portata 1 fra
			// attore e bersaglio non c'e' una cella dove mettere il muro. Sono DUE campi, `Def` e lo specchio legacy
			// che `ARTTurnManager` legge ancora.
			Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attaccante, TEXT("Action.Interrupt"), 0);
			if (!Test.TestEqual(*FString::Printf(TEXT("%s: l'Interrupt e' installato nello slot 0"), C.Nome), Idx, 0))
			{
				PagaDestroyWorld(World);
				return Esito;
			}
			Attaccante->Abilities[Idx]->Def.RangeCells = 3;
			Attaccante->Abilities[Idx]->RangeCells = 3;
		}
		URTActionData* Azione = Attaccante->Abilities[Idx];
		Azione->CooldownTurns = PagaCooldown;
		if (C.Causa == EPagaCausa::NonAggressivaBloccata || C.Causa == EPagaCausa::NonAggressivaBersaglioMorto)
		{
			// L'attacco base di Ivrin resta com'e', tranne la dichiarazione: non si conta come aggressione.
			Azione->Def.bCountsAsAttack = false;
		}
		const int32 Portata = Azione->RangeCells;
		if (!bMortaio && !Test.TestTrue(*FString::Printf(TEXT("%s: premessa: portata fra 3 e 7 (la mappa ha raggio 9)"), C.Nome),
			Portata >= 3 && Portata <= 7))
		{
			PagaDestroyWorld(World);
			return Esito;
		}

		Attaccante->PlannedAbilityIndex = Idx;
		ARTUnit* Bersaglio = nullptr;
		ARTUnit* Aiutante = nullptr; // l'Interrupt, o chi uccide l'attaccante nel Blast
		const FRTCellId CellaVuota(3, 0, 0);

		switch (C.Causa)
		{
		case EPagaCausa::Colpisce:
		case EPagaCausa::LineaBloccata:
		case EPagaCausa::BersaglioMorto:
		case EPagaCausa::InterruptBloccato:
		case EPagaCausa::NonAggressivaBloccata:
		case EPagaCausa::NonAggressivaBersaglioMorto:
			Bersaglio = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), CellaAlBersaglio);
			if (Bersaglio) { Attaccante->DeclareAttackOnUnit(Bersaglio); }
			if (Bersaglio && (C.Causa == EPagaCausa::BersaglioMorto || C.Causa == EPagaCausa::NonAggressivaBersaglioMorto))
			{
				Bersaglio->Health = 0;
			}
			break;

		case EPagaCausa::BersaglioScattato:
		{
			Bersaglio = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), CellaAlBersaglio);
			if (Bersaglio)
			{
				Attaccante->DeclareAttackOnUnit(Bersaglio);
				// `Action.Reposition`: uno scatto SENZA impatto ne' effetti, cosi' l'unica cosa che cambia e' dove sta
				// il bersaglio quando il Blast risolve (stessa scena di `Combat.Aim.PlannedAimDoesNotFollowTheTarget`).
				const int32 Scatto = RTAbilityFixtures::AddCoreAbility(Bersaglio, TEXT("Action.Reposition"));
				Bersaglio->PlannedDashAbility = Scatto;
				Bersaglio->PlannedDashCell = FRTCellId(3, -1, 0);
			}
			break;
		}

		case EPagaCausa::AltroPiano:
			Bersaglio = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), CellaSuAltroPiano);
			if (Bersaglio) { Attaccante->DeclareAttackOnUnit(Bersaglio); }
			break;

		case EPagaCausa::AreaVuota:
		case EPagaCausa::TiroCappatoDalFumoOsservato:
			Attaccante->DeclareAttackOnCell(CellaVuota);
			break;

		case EPagaCausa::TiroCappatoDalFumo:
			// Alla portata ESATTA: il Fumo in (3,0,0) sta sulla linea e la cappa a 2, la mira (portata >= 4) resta dentro la dichiarata.
			Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la mira (portata %d) sta oltre il Fumo in (3,0,0)"), C.Nome, Portata),
				Portata >= 4);
			Attaccante->DeclareAttackOnCell(FRTCellId(Portata, 0, 0));
			break;

		case EPagaCausa::AreaVuotaAlBordo:
			Attaccante->DeclareAttackOnCell(FRTCellId(Portata, 0, 0));
			break;

		case EPagaCausa::CellaOltrePortata:
			Attaccante->DeclareAttackOnCell(FRTCellId(Portata + 1, 0, 0));
			break;

		case EPagaCausa::TroppoVicino:
			Attaccante->DeclareAttackOnCell(FRTCellId(1, 0, 0));
			break;

		case EPagaCausa::MortaioAlMinimo:
			Attaccante->DeclareAttackOnCell(FRTCellId(Azione->Def.MinRangeCells, 0, 0));
			break;

		case EPagaCausa::InterrottaSenzaColpi:
		case EPagaCausa::TiroCappatoInterrotto:
		{
			// `Action.Interrupt` ha portata 1: l'interruttore sta accanto all'attaccante e lo coglie sulla cella dove sta.
			Aiutante = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0, 0));
			if (Aiutante)
			{
				const int32 InterruptIdx = RTAbilityFixtures::AddCoreAbility(Aiutante, TEXT("Action.Interrupt"));
				Aiutante->PlannedAbilityIndex = InterruptIdx;
				Aiutante->PlannedAttackTarget = Attaccante;
			}
			if (C.Causa == EPagaCausa::TiroCappatoInterrotto)
			{
				// `R7` + `R8`: la stessa mira di `TiroCappatoDalFumo` (Fumo non osservato in (3,0,0), alla portata esatta). La
				// classificazione del pagamento deve girare PRIMA di `ApplyInterrupts`, che legge `AzionePartita` e poi toglie il tiro
				// dal canale `TerrainCappedIntents`: dopo, l'azione annullata non risulterebbe partita e non pagherebbe.
				Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la mira (portata %d) sta oltre il Fumo in (3,0,0)"), C.Nome, Portata),
					Portata >= 4);
				Attaccante->DeclareAttackOnCell(FRTCellId(Portata, 0, 0));
			}
			else
			{
				Attaccante->DeclareAttackOnCell(FRTCellId(0, 3, 0)); // in portata, e nessuno ci sta
			}
			break;
		}

		case EPagaCausa::AttaccanteGiaCaduto:
			Attaccante->DeclareAttackOnCell(CellaVuota);
			Attaccante->Health = 0; // arriva al Blast col piano ancora addosso: cade in Prep o nel Dash
			break;

		case EPagaCausa::AttaccanteCadeNelBlast:
		{
			// L'area vuota e' lo stesso piano di `AreaVuota`; qui chi la lancia cade per mano di un nemico adiacente
			// nella stessa fase. Il carnefice sta su una cella che nessun altro montaggio usa.
			Attaccante->DeclareAttackOnCell(CellaVuota);
			Attaccante->Health = 1;
			Aiutante = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 1, 0));
			if (Aiutante)
			{
				Aiutante->PlannedAbilityIndex = 0;
				Aiutante->DeclareAttackOnUnit(Attaccante);
			}
			break;
		}
		}

		const bool bServeBersaglio = C.Causa == EPagaCausa::Colpisce || C.Causa == EPagaCausa::LineaBloccata
			|| C.Causa == EPagaCausa::BersaglioMorto || C.Causa == EPagaCausa::BersaglioScattato
			|| C.Causa == EPagaCausa::AltroPiano || C.Causa == EPagaCausa::InterruptBloccato
			|| C.Causa == EPagaCausa::NonAggressivaBloccata || C.Causa == EPagaCausa::NonAggressivaBersaglioMorto;
		const bool bServeAiutante = C.Causa == EPagaCausa::InterrottaSenzaColpi
			|| C.Causa == EPagaCausa::TiroCappatoInterrotto
			|| C.Causa == EPagaCausa::AttaccanteCadeNelBlast;
		if ((bServeBersaglio && !Bersaglio) || (bServeAiutante && !Aiutante))
		{
			Test.AddError(FString::Printf(TEXT("%s: la scena non e' stata montata"), C.Nome));
			PagaDestroyWorld(World);
			return Esito;
		}
		if (Bersaglio) { Tutti.Add(Bersaglio); }
		if (Aiutante) { Tutti.Add(Aiutante); }

		const int32 SaluteBersaglio = Bersaglio ? Bersaglio->Health : 0;
		const TWeakObjectPtr<ARTUnit> AttaccanteDebole(Attaccante);
		// La conoscenza di PIANIFICAZIONE, come a inizio turno (`PlanBots`, `RefreshTeamKnowledgeNow` in `SetupHexMatch`): il click ha giudicato su
		// di lei, e il Blast la congela al lock-in ([D-574]) invece di leggere la propria. Senza questa riga la squadra non ha nessuna
		// conoscenza al lock-in, e la risposta del click sarebbe «vedo tutto» — il Fumo NON osservato delle righe `R7` risulterebbe
		// osservato e il tiro gratis. Il banco la leggeva prima dal rinfresco del Blast; ora la dichiara. (Le altre righe non cambiano.)
		TM->RefreshTeamKnowledgeNow();
		PagaRunTurn(TM);

		// Il cadavere si legge da distrutto: `ConcludeTurn` chiama `DestroyDefeatedUnits`, quindi a turno finito
		// l'attaccante caduto non ha piu' un'abilita' da interrogare se non cosi'.
		ARTUnit* AttaccanteDopo = AttaccanteDebole.Get(/*bEvenIfPendingKill*/ true);
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: l'attaccante e' ancora leggibile"), C.Nome), AttaccanteDopo))
		{
			PagaDestroyWorld(World);
			return Esito;
		}

		Esito.Cooldown = AttaccanteDopo->GetAbilityCooldown(Idx);
		Esito.bHaPagato = PagaHaPagato(AttaccanteDopo, Idx);
		Esito.bAttaccanteVivo = AttaccanteDopo->IsAlive();
		Esito.bDanno = Bersaglio && Bersaglio->Health < SaluteBersaglio;
		Esito.bBersaglioSpostato = C.Causa != EPagaCausa::BersaglioScattato || (Bersaglio && Bersaglio->Cell != CellaAlBersaglio);

		TSet<int32> Visti;
		bool bTutteDistinte = true;
		for (const TWeakObjectPtr<ARTUnit>& U : Tutti)
		{
			const ARTUnit* Letta = U.Get(/*bEvenIfPendingKill*/ true);
			if (!Letta || Letta->StableUnitId == 0 || Visti.Contains(Letta->StableUnitId)) { bTutteDistinte = false; break; }
			Visti.Add(Letta->StableUnitId);
		}
		Esito.bIdentitaDistinte = bTutteDistinte;

		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Combat && E.Outcome == static_cast<uint8>(ERTCombatOutcome::NoLineOfSight))
			{
				++Esito.NoLos;
			}
			if (E.Category != ERTLogCategory::Fallback || E.Outcome != static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				|| E.UnitId != AttaccanteDopo->StableUnitId)
			{
				continue;
			}
			switch (static_cast<ERTActionInvalidReason>(E.Amount))
			{
			case ERTActionInvalidReason::Interrupted:  ++Esito.Interrotte; break;
			case ERTActionInvalidReason::OtherLayer:   ++Esito.AltroPiano; break;
			case ERTActionInvalidReason::TargetDead:   ++Esito.BersaglioMorto; break;
			case ERTActionInvalidReason::OutOfRange:   ++Esito.FuoriPortata; break;
			default:                                   ++Esito.AltreVoci; break;
			}
		}

		Esito.bValido = true;
		PagaDestroyWorld(World);
		return Esito;
	}
}

/**
 * **Un'azione d'attacco USATA paga la ricarica anche se non produce un colpo — tranne cio' che il click rifiuta**
 * ([D-414], decisione d'autore del 2026-10-31 «se si usa una abilita' si paga»).
 *
 * ⚠️ **A tabella, una riga per CAUSA, ciascuna col proprio mondo**: la tabella e' la regola. Le righe che pagano
 * sono le cause che si conoscono SOLO a risoluzione; quelle che non pagano sono geometria nota in pianificazione
 * (`R9`, e `R7` quando il Fumo e' OSSERVATO) e la guardia di vita di [D-209] (`R10`). Una versione ingenua —
 * «l'intento esiste, quindi paga» — romperebbe `R9`: `ValidateInstance` risponde `None` per ogni bersaglio-CELLA, e
 * la portata la giudica solo `CollectHexAttacks`, in silenzio.
 *
 * 🔴 **Il controllo di `R9` e' la riga `AreaVuotaAlBordo`**: stessa scena di `CellaOltrePortata` con la mira spostata
 * di UNA cella, dentro la portata. Una cella dopo, l'azione non paga; sul bordo, paga. Senza la coppia non si
 * distingue «l'attacco a vuoto non paga mai» (il difetto) da «l'attacco fuori portata non paga» (la regola).
 *
 * 🔴 **Il controllo di `R7` e' la coppia `TiroCappatoDalFumo` / `TiroCappatoDalFumoOsservato`** (decisione d'autore del
 * 2026-11-03): lo stesso tiro a cella cappato dal Fumo paga se la squadra non vedeva il Fumo (il click non ne vedeva il
 * cap e ha accettato) ed e' gratis se lo vedeva (il click rifiutava). Senza la coppia non si distingue «il tiro cappato
 * non paga mai» da «paga sempre» (il codice che c'era prima, piu' largo del proprio testo). La riga `TiroCappatoInterrotto`
 * pinna l'ORDINE: la classificazione (`ClassifyRangeLostPayment`) sta prima di `ApplyInterrupts`, che toglie il tiro dal
 * canale del piano subito dopo averlo letto, e un Interrupt altrui non deve rendere gratuito cio' che avrebbe pagato.
 *
 * Premesse per riga (`PagaAttesi`): ogni voce di rifiuto del turno e' asserita, ZERO comprese — cosi' la riga
 * isola la propria causa e un'altra non puo' pagare al suo posto.
 *
 * ⌫ *Fino al 2026-10-31 queste righe asserivano il contrario (D-504 punto 3, D-509 punto 7h: «un attacco fermato
 * dalla linea di tiro o dal piano non paga, il suo criterio e' il colpo prodotto»).*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartedActionPaysWithoutAHitTest,
	"RefactorTactics.Actions.Blast.StartedActionPaysWithoutAHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartedActionPaysWithoutAHitTest::RunTest(const FString&)
{
	const FPagaCase Casi[] = {
		{ TEXT("controllo: l'attacco colpisce"), EPagaCausa::Colpisce, true,
		  TEXT("chi colpisce paga, come prima: fissa il valore che le altre righe devono eguagliare") },
		{ TEXT("R1 area vuota in portata"), EPagaCausa::AreaVuota, true,
		  TEXT("l'attacco e' partito e non ha trovato nessuno: e' un ESITO, e un esito si paga") },
		{ TEXT("R1 area vuota sul bordo della portata"), EPagaCausa::AreaVuotaAlBordo, true,
		  TEXT("distanza pari alla portata: dentro, quindi parte") },
		{ TEXT("R3 bersaglio scattato altrove"), EPagaCausa::BersaglioScattato, true,
		  TEXT("la mira e' congelata al lock-in: il colpo cade sulla cella vuota, ed e' partito") },
		{ TEXT("R4 linea di tiro bloccata a risoluzione"), EPagaCausa::LineaBloccata, true,
		  TEXT("un muro comparso dopo il lock-in e' un esito: chiude l'asimmetria con la cura (D-504)") },
		{ TEXT("R5 mira su un altro piano"), EPagaCausa::AltroPiano, true,
		  TEXT("il piano si giudica a risoluzione: corregge D-509 punto 7h") },
		{ TEXT("R6 bersaglio gia' morto"), EPagaCausa::BersaglioMorto, true,
		  TEXT("come la cura su un alleato caduto: dichiarata, valida in pianificazione, e il bersaglio non c'e' piu'") },
		{ TEXT("R7 tiro cappato dal Fumo NON osservato (in (3,0,0), vista 0)"), EPagaCausa::TiroCappatoDalFumo, true,
		  TEXT("R7: il click giudica sulla mappa ottimistica, che non vede il Fumo, e accetta; il cap emerge solo a risoluzione e l'azione, partita, paga") },
		{ TEXT("controllo di R7: tiro cappato dal Fumo OSSERVATO (in (1,0,0))"), EPagaCausa::TiroCappatoDalFumoOsservato, false,
		  TEXT("decisione d'autore del 2026-11-03: la squadra vedeva il Fumo, il click vedeva il cap e rifiutava; cio' che il click rifiuta e' gratis") },
		{ TEXT("R7+R8 tiro cappato dal Fumo NON osservato, annullato da un Interrupt"), EPagaCausa::TiroCappatoInterrotto, true,
		  TEXT("[D-414] punto 1 su R7: l'Interrupt altrui non rende gratuito cio' che senza di lui avrebbe pagato (la classificazione del pagamento sta prima di ApplyInterrupts)") },
		{ TEXT("R8 annullata da un Interrupt, senza colpi"), EPagaCausa::InterrottaSenzaColpi, true,
		  TEXT("[D-414] punto 1: un controllo che impedisce l'abilita' non restituisce la ricarica") },
		// ➕ **Fix wave di `#3134`**: tre righe che difendono tre clausole che nessun test copriva.
		{ TEXT("R4 l'Interrupt come attore, bersaglio dietro un muro"), EPagaCausa::InterruptBloccato, true,
		  TEXT("confermato dall'autore il 2026-11-01: `bCountsAsAttack` e' vero per `Action.Interrupt`, quindi R4 vale anche per chi interrompe") },
		{ TEXT("azione non aggressiva bloccata dal muro"), EPagaCausa::NonAggressivaBloccata, false,
		  TEXT("D-575 (c), gratis e non decisa — la clausola `!bCountsAsAttack` di `IntentoPartitoSenzaColpo`: l'intento e' in `BlockedIntents` ma non e' un'aggressione") },
		{ TEXT("azione non aggressiva, bersaglio-unita' gia' morto"), EPagaCausa::NonAggressivaBersaglioMorto, false,
		  TEXT("D-575 (c), gratis e non decisa — la guardia `bCountsAsAttack` di R6: come su `main`, un'azione che non e' un colpo ha le proprie regole") },
		{ TEXT("R9 cella oltre la portata dichiarata"), EPagaCausa::CellaOltrePortata, false,
		  TEXT("[D-200] e [D-414] COINCIDONO: geometria nota in pianificazione, il click la rifiuta") },
		{ TEXT("R9 sotto la distanza minima"), EPagaCausa::TroppoVicino, false,
		  TEXT("stessa regola della portata: il click rifiuta `TooClose`") },
		{ TEXT("controllo di R9: mortaio alla distanza minima"), EPagaCausa::MortaioAlMinimo, true,
		  TEXT("lo STESSO mortaio, un passo piu' in la, parte e paga: la differenza e' solo la geometria") },
		{ TEXT("R10 attaccante gia' caduto"), EPagaCausa::AttaccanteGiaCaduto, false,
		  TEXT("un cadavere non paga un'azione che non ha mai eseguito") },
		{ TEXT("R10 attaccante che cade nel Blast"), EPagaCausa::AttaccanteCadeNelBlast, false,
		  TEXT("[D-209]: per chi attacca «spesa» vuol dire SOPRAVVISSUTO alla fase, anche a vuoto") },
	};

	TArray<FPagaEsito> Esiti;
	for (const FPagaCase& C : Casi)
	{
		const FPagaEsito Esito = PagaGiraRiga(*this, C);
		Esiti.Add(Esito);
		if (!Esito.bValido) { return false; }

		AddInfo(FString::Printf(TEXT("%s -> cooldown residuo %d, danno %d, voci: interrotte %d / piano %d / morto %d / portata %d / altre %d / nolos %d"),
			C.Nome, Esito.Cooldown, Esito.bDanno ? 1 : 0, Esito.Interrotte, Esito.AltroPiano, Esito.BersaglioMorto,
			Esito.FuoriPortata, Esito.AltreVoci, Esito.NoLos));

		// Le premesse: senza di esse il verde potrebbe misurare uno scenario diverso da quello dichiarato.
		const FPagaAttesi A = PagaAttesiDellaRiga(C.Causa);
		TestTrue(*FString::Printf(TEXT("%s: premessa: le unita' hanno identita' stabili distinte"), C.Nome), Esito.bIdentitaDistinte);
		TestTrue(*FString::Printf(TEXT("%s: premessa: il bersaglio ha davvero lasciato la cella"), C.Nome), Esito.bBersaglioSpostato);
		TestEqual(*FString::Printf(TEXT("%s: premessa: voci Interrupted"), C.Nome), Esito.Interrotte, A.Interrotte);
		TestEqual(*FString::Printf(TEXT("%s: premessa: voci OtherLayer"), C.Nome), Esito.AltroPiano, A.AltroPiano);
		TestEqual(*FString::Printf(TEXT("%s: premessa: voci TargetDead"), C.Nome), Esito.BersaglioMorto, A.BersaglioMorto);
		TestEqual(*FString::Printf(TEXT("%s: premessa: voci OutOfRange"), C.Nome), Esito.FuoriPortata, A.FuoriPortata);
		TestEqual(*FString::Printf(TEXT("%s: premessa: nessun altro rifiuto"), C.Nome), Esito.AltreVoci, 0);
		TestEqual(*FString::Printf(TEXT("%s: premessa: voci NoLineOfSight"), C.Nome), Esito.NoLos, A.NoLos);
		TestEqual(*FString::Printf(TEXT("%s: premessa: il bersaglio incassa?"), C.Nome), Esito.bDanno, A.bDanno);
		TestEqual(*FString::Printf(TEXT("%s: premessa: l'attaccante e' vivo a fine turno?"), C.Nome),
			Esito.bAttaccanteVivo, A.bAttaccanteVivo);

		TestEqual(*FString::Printf(TEXT("%s: %s"), C.Nome, C.Perche), Esito.bHaPagato, C.bDevePagare);
	}

	// 🔴 **UNA volta sola**: ogni riga che paga lascia lo STESSO residuo del controllo che colpisce. Un secondo
	// addebito scrive lo stesso numero, ma una sede che rifacesse il pagamento con un'altra durata si vedrebbe qui.
	const int32 Riferimento = Esiti[0].Cooldown;
	for (int32 i = 1; i < UE_ARRAY_COUNT(Casi); ++i)
	{
		if (Casi[i].bDevePagare && Esiti[i].bHaPagato && Casi[i].Causa != EPagaCausa::MortaioAlMinimo)
		{
			TestEqual(*FString::Printf(TEXT("%s: paga come chi colpisce, non di piu'"), Casi[i].Nome),
				Esiti[i].Cooldown, Riferimento);
		}
	}
	return true;
}

// =====================================================================================================
// RIC-3 (#3134, D-571): «un'azione paga se il click avrebbe accettato il piano; e' gratis solo cio' che
// il click rifiuta». Le righe sono UNA PER CAUSA, e ognuna ha il proprio controllo positivo.
// =====================================================================================================

namespace
{
	/** Che cosa il piano dichiara come bersaglio-unita'. `Assente` e' un piano che il click non puo' produrre. */
	enum class ERic3Bersaglio : uint8
	{
		Nemico,   //< un'unita' avversaria viva
		Alleato,  //< un'unita' della squadra di chi agisce
		Morto,    //< un avversario gia' caduto (salute 0) quando il turno comincia
		Assente,  //< nessun bersaglio-unita' ne' cella: il click non lo produce mai
	};

	enum class ERic3Azione : uint8
	{
		AttaccoBase,     //< l'attacco base di Ivrin (slot 0), nel Blast
		ModifyArc,       //< `Action.ModifyArc` (slot 3), intercettata nel Blast prima della raccolta degli intenti
		Ignite,          //< `Action.Ignite` (slot 3), ambientale: risolve nel Cleanup
		ReazioneCounter, //< `Action.Counter` (slot 3) armata come REAZIONE
		CreateCover,     //< `Action.CreateCover` (slot 3): una struttura di BORDO, kind `Edge` per il puntatore
		AttaccoACella,   //< l'attacco base di Ivrin (slot 0) dichiarato su una CELLA vuota: il tiro a cella di `R7`
		Interact,        //< `Action.Interact` (slot 3): una struttura di BORDO (`StructureOp = SetDoorState`), kind `Edge` per il puntatore, ricarica scritta dal banco
	};

	/** UNA scena: le righe differiscono per UNA variabile fra il controllo e il caso. */
	struct FRic3Scena
	{
		ERic3Azione Azione = ERic3Azione::AttaccoBase;
		ERic3Bersaglio Bersaglio = ERic3Bersaglio::Nemico;
		int32 Distanza = 3;                //< del bersaglio, lungo +X dall'origine dove sta chi agisce
		bool bOltrePortataDichiarata = false; //< se vero la distanza e' `portata + 1` e `Distanza` si ignora
		bool bDistanzaPariAllaPortata = false; //< se vero la distanza e' ESATTAMENTE la portata dichiarata
		int32 FumoX = INDEX_NONE;          //< se >= 0, Fumo sulla cella (FumoX,0,0), sulla linea fra chi agisce e il bersaglio
		bool bTestimone = false;           //< un compagno 2 celle dietro il bersaglio lo VEDE (vista 0, solo l'intorno), e chi agisce non vede oltre il proprio
		int32 PortataSovrascritta = INDEX_NONE; //< se >= 0 sostituisce la portata dell'azione (specchio e `Def`): serve a dare spazio alla geometria del Fumo
		int32 Visione = INDEX_NONE;        //< se >= 0 sostituisce `VisionRange` di chi agisce
		bool bScattoPrecede = false;       //< chi agisce pianifica prima uno scatto (`Action.Reposition`, 2 celle lungo +X): il click giudica dalla cella d'ARRIVO dello scatto, e il bersaglio sta alla portata da li' (D-575 (a))
		bool bOstacoloSulloScatto = false; //< solo con `bScattoPrecede`: un avversario sulla cella d'arrivo dello scatto, che lo ferma una cella prima
		bool bMiraOltreLaFineDelloScatto = false; //< solo con `bScattoPrecede`: il bersaglio sta UNA cella oltre la portata dalla cella d'arrivo dello scatto, quindi il click rifiuta
		bool bNemicoAttacca = false;       //< solo `ReazioneCounter`: un avversario adiacente colpisce chi ha la reazione armata
		bool bAttoreCaduto = false;        //< chi agisce arriva al turno gia' caduto (salute 0) col piano ancora addosso
		bool bAttoreCadeNelBlast = false;  //< un avversario adiacente lo uccide nello stesso Blast (parte con 1 di salute)
		bool bGiaInRicarica = false;       //< l'azione e' GIA' in ricarica: nessun click puo' armarla, il piano e' scritto a mano
		bool bBersaglioScattaLontano = false; //< il bersaglio scatta di due celle e finisce OLTRE la portata dichiarata
		bool bBersaglioIgnotoAlVelo = false;  //< il velo dell'HUD non conosce il bersaglio: il click non lo bersaglia (`IsKnownToObserver`)
		bool bAlleatoNonComandabile = false;  //< il bersaglio alleato e' pianificato dal bot: non e' comandabile da questo giocatore
		bool bBordoDichiarato = false;     //< solo `CreateCover` e `Interact` col piano a mano: il lato e' dichiarato (come lo scrive `HandleTargetEdge`)
		bool bPianoACellaSenzaLato = false; //< solo `CreateCover` e `Interact`: il piano a mano ha il bersaglio a CELLA (come quello dei bordi) ma NESSUN lato
		bool bPortaBloccataTraLeCelle = false; //< solo `Interact`: sul bordo fra la cella di chi agisce (0,0,0) e quella del bersaglio (1,0,0) c'e' una porta `Locked`: la commutazione e' rifiutata
		bool bCaricaPrecede = false;       //< chi agisce pianifica prima una CARICA che RIESCE (un nemico due celle dietro di lui, lungo -X): il click giudica dalla cella di PARTENZA (la carica non sposta l'origine), il Blast dalla cella d'arrivo (D-575 (1bis))
		bool bAttoreMuoreNelloScatto = false; //< solo con `bScattoPrecede` e `bOstacoloSulloScatto`: il primo passo dello scatto e' un Fuoco (10 danni all'ingresso) e chi agisce ha 5 di salute, quindi muore nel Dash, DOPO il lock-in e PRIMA del Blast
		bool bFumoEsploratoFuoriVista = false; //< la cella del Fumo e' in `ExploredCells` e NON in `VisibleCells`: la squadra non la vede, la mappa del click la LEGGE (D-534 (3), `Q4`)
		bool bCompagnoLontano = false;         //< un compagno (vista 0) a tre celle dal Fumo: in pianificazione NON lo vede, e non cambia niente se resta fermo
		bool bCompagnoScatta = false;          //< solo con `bCompagnoLontano`: il compagno scatta di due celle verso il Fumo, lo mette in vista nel Blast e non in pianificazione
		bool bCompagnaPressoLaMira = false;    //< solo `AttaccoACella`: una compagna (vista 0) a tre celle dalla mira, in una cella che ordina DOPO chi tira: la sua cella non e' quella di chi tira
	};

	/** Che cosa il click fa della scena: `ND` = non c'e' niente da cliccare, o il click non puo' nemmeno armare l'azione. */
	enum class ERic3Click : uint8 { ND, Accetta, Rifiuta };

	/** Che cosa la riga si aspetta di vedere: le PREMESSE, senza le quali il verde potrebbe misurare un'altra scena. */
	struct FRic3Atteso
	{
		bool bPaga = false;
		ERTActionInvalidReason Voce = ERTActionInvalidReason::None; //< l'UNICA voce di rifiuto del turno; `None` = nessuna
		bool bDanno = false;     //< il bersaglio incassa
		bool bArco = false;      //< `ModifyArc` ha aperto un collegamento
		bool bFuoco = false;     //< `Ignite` ha acceso la cella del bersaglio
		int32 Attivate = 0;      //< voci di reazione `Activated`
		int32 NonInnescate = 0;  //< voci di reazione `NotTriggered`
		bool bControllaCella = false; //< dove chi agisce deve trovarsi a fine turno (lo scatto fermato lo lascia una cella prima)
		FRTCellId CellaFinale;        //< dove chi agisce deve trovarsi a fine turno
		ERic3Click Click = ERic3Click::ND; //< l'ORACOLO: il click di produzione accetta il piano?
		// ⚠️ `bClickDaRiparare` e' un PIN DI DECISIONE, non una regola ([D-575] (b), registrata in #3981: il numero vale al merge): marca le
		// righe in cui il click accetta un piano che NON dovrebbe accettare (l'alleato non comandabile, la struttura di bordo senza
		// lato). L'autore ha deciso che si ripara il CLICK, e che intanto la causa e' gratis: quando il click rifiutera' (per la
		// regola di [D-571] la causa sara' gratis senza altro codice nel pagamento) la premessa «l'oracolo del click risponde come
		// la riga dichiara» diventa ROSSA — di proposito, e la riga va riscritta come «il click rifiuta, gratis», non aggirata.
		// ⌫ *Si chiamava `bConflittoNoto` e teneva verdi anche lo scatto fermato e il Fumo messo in vista dal compagno, che D-574 e
		// D-575 (a) hanno deciso: quelle due righe ora pagano e sono regole come le altre.*
		bool bClickDaRiparare = false; //< il click accetta un piano che non dovrebbe accettare, e la causa e' gratis finche' non lo si ripara
		bool bSenzaEquivalenza = false; //< chi agisce cade FRA il click e il Blast: la guardia di vita decide, non il click
		bool bAttoreNonUltimo = false;  //< solo `AttaccoACella`: un'altra unita' ordina DOPO chi tira (altrimenti chi tira e' l'ultima)
		int32 CopertureRifiutate = 0; //< voci `CoverRejected` del turno
		int32 CopertureErette = 0;    //< voci `CoverCreated` del turno
		bool bControllaBordo = false; //< solo `CreateCover` e `Interact`: il click ha scritto il lato?
		bool bBordo = false;
		bool bMuoreNelDash = false;   //< chi agisce e' MORTO a fine turno, e per un danno scritto nella fase Dash (e non nel Blast): era vivo al lock-in e cade prima del Blast
		bool bControllaBersaglio = false; //< solo `bBersaglioScattaLontano`: il bersaglio deve essere DAVVERO finito altrove
		FRTCellId CellaDelBersaglio;
		int32 ResiduoAtteso = INDEX_NONE; //< se >= 0 sostituisce il residuo calcolato da `bPaga`
	};

	struct FRic3Esito
	{
		bool bValido = false;
		bool bHaPagato = false;
		int32 Cooldown = 0;
		TArray<int32> Rifiuti;   //< i motivi di ogni voce Fallback/Cancelled di chi agisce
		bool bDanno = false;
		bool bArco = false;
		bool bFuoco = false;
		int32 Attivate = 0;
		int32 NonInnescate = 0;
		FRTCellId CellaFinale;
		ERic3Click Click = ERic3Click::ND;
		int32 CopertureRifiutate = 0;
		int32 CopertureErette = 0;
		bool bBordoDopoIlClick = false;
		bool bAttoreVivo = true;    //< chi agisce e' vivo a fine turno
		int32 DanniNelDash = 0;     //< voci `Combat` della fase Dash: il danno da terreno di uno scatto (nessun'altra riga ne produce)
		bool bAttoreUltimo = false; //< solo `AttaccoACella`: chi tira e' l'ULTIMO nell'ordine di `SortUnitsForResolution` (premessa dell'origine)
		FRTCellId CellaDelBersaglioDopo;
	};

	FRic3Esito Ric3Gira(FAutomationTestBase& Test, const TCHAR* Nome, const FRic3Scena& S)
	{
		FRic3Esito Esito;
		UWorld* World = PagaMakeWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), Nome), World)) { return Esito; }

		TArray<FRTCellId> Fumo;
		if (S.FumoX != INDEX_NONE) { Fumo.Add(FRTCellId(S.FumoX, 0, 0)); }
		ARTHexMapActor* MapActor = PagaSpawnMap(World, /*Radius=*/ 9, {}, Fumo, {});

		if (S.bAttoreMuoreNelloScatto || S.bPortaBloccataTraLeCelle)
		{
			// La cella (1,0,0) con una caratteristica in piu', scritta come fa `PagaGiraPorta`: si legge, si cambia, si riscrive e si riordina.
			// Il Fuoco e' il primo passo dello scatto (10 danni all'ingresso, `Terrain.Fire`, CP 8.1; non ferma lo scatto lineare); la porta `Locked`
			// sta sul bordo W, cioe' fra la cella di chi agisce e quella del bersaglio adiacente.
			FRTHexCellData Dati = *MapActor->MapAsset->FindCell(FRTCellId(1, 0, 0));
			if (S.bAttoreMuoreNelloScatto)
			{
				Dati.Surface = ERTHexSurface::Fire;
				Dati.MoveCost = 2; // costo del catalogo v0.1, come lo precompila il paint tool
			}
			if (S.bPortaBloccataTraLeCelle)
			{
				Dati.Doors.Add(FRTHexDoor(ERTHexDirection::W, ERTHexDoorState::Locked));
			}
			MapActor->MapAsset->AddOrUpdateCell(Dati);
			MapActor->MapAsset->SortCells();
		}

		ARTUnit* Attore = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
		// Un avversario lontano in ogni banco: la squadra avversa non e' mai vuota, e non entra nella portata di nessuno.
		ARTUnit* Spettatore = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-5, 0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: attore"), Nome), Attore)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: spettatore"), Nome), Spettatore)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: TM"), Nome), TM))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		if (S.Visione != INDEX_NONE) { Attore->VisionRange = S.Visione; }
		// Salute per cui il Fuoco in (1,0,0) uccide (10 danni, scudo gia' a zero per `PagaSpawnUnit`): muore ENTRANDO nella cella, nel Dash.
		if (S.bAttoreMuoreNelloScatto) { Attore->Health = 5; }

		// L'azione sta in uno slot che l'array dei cooldown COPRE, o il contatore non e' osservabile affatto.
		int32 Idx = 0;
		switch (S.Azione)
		{
		case ERic3Azione::ModifyArc:       Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.ModifyArc"), 3); break;
		case ERic3Azione::Ignite:          Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.Ignite"), 3); break;
		case ERic3Azione::ReazioneCounter: Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.Counter"), 3); break;
		case ERic3Azione::CreateCover:     Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.CreateCover"), 3); break;
		case ERic3Azione::Interact:        Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.Interact"), 3); break;
		default: break; // l'attacco base e' lo slot 0
		}
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: l'azione e' installata"), Nome), Idx != INDEX_NONE))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		Attore->Abilities[Idx]->CooldownTurns = PagaCooldown;
		if (S.PortataSovrascritta != INDEX_NONE)
		{
			// Ivrin tira a 4: troppo poco perche' il bersaglio sia visto da un compagno senza che lo sia anche il Fumo davanti a lui.
			Attore->Abilities[Idx]->RangeCells = S.PortataSovrascritta;
			Attore->Abilities[Idx]->Def.RangeCells = S.PortataSovrascritta;
		}
		const int32 Portata = Attore->Abilities[Idx]->RangeCells;
		if (S.Azione == ERic3Azione::AttaccoACella && !S.bScattoPrecede)
		{
			// Il confine del predicato (`>=`) e' esercitato solo se la mira sta ESATTAMENTE alla portata dichiarata: la riga lo
			// dichiara con `Distanza`, e questa premessa lo lega al catalogo. Se la portata dell'attacco base cambia, la riga
			// diventa rossa qui invece di indebolire in silenzio il confine. (Con uno scatto davanti la distanza e' calcolata dalla
			// portata stessa: `CellaScatto.X + Portata`, e non c'e' un numero da legare.)
			Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la mira a cella sta alla portata dichiarata dell'azione"), Nome),
				Portata, S.Distanza);
		}

		// Dove sta il bersaglio. Con lo scatto la distanza e' misurata dalla cella d'ARRIVO dello scatto (2,0,0): da la'
		// il bersaglio e' ESATTAMENTE alla portata, dalla cella dove lo scatto si ferma (1,0,0) e' una cella oltre.
		const FRTCellId CellaScatto(2, 0, 0);
		// Con `bBersaglioScattaLontano` il bersaglio parte UNA cella dentro la portata: due celle lungo l'asse r lo portano a
		// distanza `portata + 1` (la distanza esagonale di (x,2) e' x+2), qualunque sia la portata.
		const int32 Distanza = S.bScattoPrecede ? CellaScatto.X + Portata + (S.bMiraOltreLaFineDelloScatto ? 1 : 0)
			: S.bOltrePortataDichiarata ? Portata + 1
			: S.bDistanzaPariAllaPortata ? Portata
			: S.bBersaglioScattaLontano ? Portata - 1 : S.Distanza;
		if (S.bTestimone)
		{
			// Il compagno sta a (Distanza + 2): vede il bersaglio (intorno di 2 celle) e dista Distanza - 1 dal Fumo in (3,0,0), che
			// resta fuori dall'intorno (>= 3) solo se Distanza >= 4. Chi agisce, con vista 0, vede fino a 2 celle.
			Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la distanza (%d) regge la geometria del Fumo non osservato"), Nome, Distanza),
				Distanza >= 4 && Distanza <= Portata);
		}
		const FRTCellId CellaBersaglio(Distanza, 0, 0);
		// La vista: con il testimone, chi agisce vede solo il proprio intorno (consapevolezza ravvicinata di 2 celle) e il
		// compagno dietro il bersaglio vede il bersaglio e nient'altro, quindi una cella di Fumo a distanza 3 da tutti e
		// due e' NON OSSERVATA. Senza testimone il Fumo a (1,0,0) sta nell'intorno di chi agisce: e' OSSERVATO.
		if (S.bTestimone) { Attore->VisionRange = 0; }

		TWeakObjectPtr<ARTUnit> BersaglioDebole;
		int32 SaluteBersaglio = 0;
		if (S.Azione == ERic3Azione::ReazioneCounter)
		{
			// Qui chi agisce e' il Reactor, e chi lo colpisce (se colpisce) un avversario adiacente con l'attacco base.
			ARTUnit* Nemico = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 0, 0));
			if (!Test.TestNotNull(*FString::Printf(TEXT("%s: avversario adiacente"), Nome), Nemico))
			{
				PagaDestroyWorld(World);
				return Esito;
			}
			Attore->PlannedReactionAbility = Idx;
			Attore->PlannedAbilityIndex = INDEX_NONE; // niente azione principale: solo la reazione, pronta
			if (S.bNemicoAttacca)
			{
				Nemico->PlannedAbilityIndex = 0;
				Nemico->DeclareAttackOnUnit(Attore);
			}
		}
		else
		{
			ARTUnit* Bersaglio = nullptr;
			switch (S.Bersaglio)
			{
			case ERic3Bersaglio::Nemico:
			case ERic3Bersaglio::Morto:
				Bersaglio = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), CellaBersaglio);
				break;
			case ERic3Bersaglio::Alleato:
				Bersaglio = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), CellaBersaglio);
				break;
			case ERic3Bersaglio::Assente:
				break;
			}
			if (S.Bersaglio != ERic3Bersaglio::Assente
				&& !Test.TestNotNull(*FString::Printf(TEXT("%s: bersaglio"), Nome), Bersaglio))
			{
				PagaDestroyWorld(World);
				return Esito;
			}
			if (Bersaglio) { SaluteBersaglio = Bersaglio->Health; BersaglioDebole = Bersaglio; }
			if (Bersaglio && S.bAlleatoNonComandabile) { Bersaglio->bIsBotControlled = true; }

			// Un compagno dietro il bersaglio, che lo vede: la squadra lo conosce anche se chi agisce non vede oltre l'intorno.
			if (S.bTestimone)
			{
				ARTUnit* Testimone = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(Distanza + 2, 0, 0));
				if (!Test.TestNotNull(*FString::Printf(TEXT("%s: testimone"), Nome), Testimone))
				{
					PagaDestroyWorld(World);
					return Esito;
				}
				Testimone->VisionRange = 0;
			}

			// Un compagno a tre celle dal Fumo (FumoX,0,0), in (FumoX,3,0): fuori dall'intorno di 2 celle, quindi in pianificazione
			// la squadra NON vede il Fumo. Se scatta di due celle lungo l'asse r arriva in (FumoX,1,0), a una cella dal Fumo: nel Blast
			// (`RefreshTeamKnowledgeForBlast` osserva le posizioni POST-Dash) la cella e' in vista, e la mappa del click del Blast la
			// legge. E' l'UNICA variabile fra il controllo (compagno fermo) e il caso (compagno che scatta).
			if (S.bCompagnoLontano)
			{
				if (!Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il compagno ha un Fumo da guardare"), Nome), S.FumoX != INDEX_NONE))
				{
					PagaDestroyWorld(World);
					return Esito;
				}
				ARTUnit* Compagno = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(S.FumoX, 3, 0));
				if (!Test.TestNotNull(*FString::Printf(TEXT("%s: compagno"), Nome), Compagno))
				{
					PagaDestroyWorld(World);
					return Esito;
				}
				Compagno->VisionRange = 0;
				if (S.bCompagnoScatta)
				{
					const int32 ScattoCompagno = RTAbilityFixtures::AddCoreAbility(Compagno, TEXT("Action.Reposition"));
					Compagno->PlannedDashAbility = ScattoCompagno;
					Compagno->PlannedDashCell = FRTCellId(S.FumoX, 1, 0);
				}
			}

			// Una compagna a TRE celle dalla mira, in (X della mira, -3, 0): la sua cella ordina DOPO quella di chi tira (la X della mira
			// e' maggiore di 0) e la linea dalla sua cella alla mira (la stessa colonna) non incontra il Fumo, che sta sull'asse di chi
			// tira. Un'origine sbagliata — la cella dell'ULTIMA unita' dell'ordine invece di quella di chi tira — vedrebbe una linea
			// senza Fumo e a distanza 3: la portata efficace basterebbe e il tiro pagherebbe anche col Fumo osservato, dove
			// dall'origine vera il click rifiuta. A tre celle (e non a una) la compagna non vede il Fumo in (3,0,0), vicino alla mira,
			// quindi il controllo col Fumo NON osservato resta tale.
			if (S.bCompagnaPressoLaMira)
			{
				ARTUnit* Compagna = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(CellaBersaglio.X, -3, 0));
				if (!Test.TestNotNull(*FString::Printf(TEXT("%s: compagna presso la mira"), Nome), Compagna)
					|| !Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la compagna e' a tre celle dalla mira"), Nome),
						URTHexLibrary::HexDistance(Compagna->Cell, CellaBersaglio), 3))
				{
					PagaDestroyWorld(World);
					return Esito;
				}
				Compagna->VisionRange = 0;
			}

			if (S.bScattoPrecede)
			{
				// `Action.Reposition`: uno scatto lineare di 2 celle SENZA impatto ne' effetti. Sulla cella d'arrivo, se c'e'
				// l'ostacolo, lo scatto si ferma una cella prima: e' l'unica variabile fra controllo e caso. Lo scatto e'
				// pianificato PRIMA del click: e' da li' che il click giudica (`AimOriginFor`, [D-464]).
				const int32 Scatto = RTAbilityFixtures::AddCoreAbility(Attore, TEXT("Action.Reposition"));
				Attore->PlannedDashAbility = Scatto;
				Attore->PlannedDashCell = CellaScatto;
				if (S.bOstacoloSulloScatto)
				{
					ARTUnit* Ostacolo = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), CellaScatto);
					if (!Test.TestNotNull(*FString::Printf(TEXT("%s: ostacolo"), Nome), Ostacolo))
					{
						PagaDestroyWorld(World);
						return Esito;
					}
				}
			}
			if (S.bCaricaPrecede)
			{
				// La CARICA RIESCE ([D-575] (1bis)): un nemico in (-2,0,0), due celle dietro chi agisce, e la carica (portata 3) verso (-3,0,0) lo
				// urta e si ferma adiacente, in (-1,0,0). Il bersaglio del tiro sta sul lato opposto (+X): il click giudica dalla cella di PARTENZA
				// (`AimOriginCell` non sposta l'origine per una carica, [D-464] (5)), il Blast da quella d'arrivo, una cella piu' lontana dal
				// bersaglio. E' l'unica variabile fra il controllo (nessuna carica) e il caso. L'impatto (20 danni e una spinta di 1) e' un
				// intento a parte, con la propria ricarica: la ricarica che la riga misura e' quella del tiro (slot 0).
				ARTUnit* Caricato = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-2, 0, 0));
				if (!Test.TestNotNull(*FString::Printf(TEXT("%s: nemico caricato"), Nome), Caricato))
				{
					PagaDestroyWorld(World);
					return Esito;
				}
				const int32 Carica = RTAbilityFixtures::AddCoreAbility(Attore, TEXT("Action.Charge"));
				Attore->PlannedDashAbility = Carica;
				Attore->PlannedDashCell = FRTCellId(-3, 0, 0);
			}
			if (Bersaglio && S.bBersaglioScattaLontano)
			{
				// Il bersaglio scatta di due celle lungo l'asse r e finisce a distanza `Portata + 1` da chi agisce. La mira e'
				// congelata al lock-in sulla cella di PRIMA ([D-415]): il predicato non deve leggere dove il bersaglio e' finito.
				const int32 ScattoDelBersaglio = RTAbilityFixtures::AddCoreAbility(Bersaglio, TEXT("Action.Reposition"));
				Bersaglio->PlannedDashAbility = ScattoDelBersaglio;
				Bersaglio->PlannedDashCell = FRTCellId(Distanza, 2, 0);
			}

			if (S.bGiaInRicarica)
			{
				Attore->ConsumeAbility(Idx);
				Attore->TickCooldowns(); // PagaCooldown - 1: un secondo `ConsumeAbility` la riporterebbe a PagaCooldown
			}

			// --- L'ORACOLO: il click di produzione, sulla STESSA fixture ----------------------------------------------
			// `RefreshTeamKnowledgeNow` come all'avvio di un turno: senza, `PlayerAimMap` giudica sulla mappa autorevole e il
			// Fumo non osservato cappa anche il click. `DispatchUnitClick` e non `HandleClickOnUnit`: e' il percorso che il
			// gioco percorre, con la matrice di `ResolveOutcome` (un compagno comandabile si seleziona, non si bersaglia).
			// Non si clicca cio' che non c'e' (nessun bersaglio) ne' un'azione che il click non puo' armare (gia' in ricarica).
			TM->RefreshTeamKnowledgeNow();
			if (S.bFumoEsploratoFuoriVista)
			{
				// La cella del Fumo e' ESPLORATA ma non in vista: la squadra la conosce (era vuota quando l'ha esplorata, il Fumo e'
				// nato dopo) e non la vede adesso. La mappa del click tiene il contenuto ATTUALE delle celle esplorate (D-534 (3), `Q4`,
				// confermato dall'autore), quindi legge il Fumo e il cap. `SetTeamKnowledgeForTest` sostituisce la conoscenza della
				// squadra; il Blast la rinfresca con `Observe` a partire da questa, e le celle esplorate non si perdono.
				FRTTeamKnowledge Conoscenza = TM->KnowledgeForTeamPublic(Attore->TeamId);
				const FRTCellId CellaDelFumo(S.FumoX, 0, 0);
				Test.TestFalse(*FString::Printf(TEXT("%s: premessa: il Fumo non e' in vista alla squadra"), Nome),
					Conoscenza.VisibleCells.Contains(CellaDelFumo));
				Conoscenza.ExploredCells.AddUnique(CellaDelFumo);
				Conoscenza.ExploredCells.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
				TM->SetTeamKnowledgeForTest(Conoscenza);
				Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il Fumo e' esplorato"), Nome),
					TM->KnowledgeForTeamPublic(Attore->TeamId).ExploredCells.Contains(CellaDelFumo));
			}
			if (S.Azione == ERic3Azione::AttaccoACella)
			{
				// Il tiro a CELLA: il click di produzione e' `HandleClickOnCell` -> `HandleTargetCell`, che esige il kind `Cell`
				// (un'abilita' `Area`). Il banco dichiara `Area` SOLO per il click e rimette `Single` subito dopo, cosi' il turno
				// risolve esattamente come la riga `R7` di `StartedActionPaysWithoutAHit` (stessa azione, stessa mira a cella).
				ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
				if (!Test.TestNotNull(*FString::Printf(TEXT("%s: controller"), Nome), PC))
				{
					PagaDestroyWorld(World);
					return Esito;
				}
				Attore->Abilities[Idx]->Shape = ERTAbilityShape::Area;
				PC->SelectUnit(Attore, /*bRecordAsPlayerInput=*/ false);
				PC->ArmKitAbility(Idx);
				PC->HandleClickOnCellForTest(CellaBersaglio);
				Esito.Click = (Attore->PlannedAbilityIndex == Idx && Attore->bAttackTargetsCell
					&& Attore->PlannedAttackCell == CellaBersaglio) ? ERic3Click::Accetta : ERic3Click::Rifiuta;
				Attore->SelectAbility(INDEX_NONE);
				Attore->Abilities[Idx]->Shape = ERTAbilityShape::Single;
			}
			if (Bersaglio && !S.bGiaInRicarica)
			{
				ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
				if (!Test.TestNotNull(*FString::Printf(TEXT("%s: controller"), Nome), PC))
				{
					PagaDestroyWorld(World);
					return Esito;
				}
				PC->SelectUnit(Attore, /*bRecordAsPlayerInput=*/ false);
				PC->ArmKitAbility(Idx);
				Bersaglio->SetKnownToObserver(!S.bBersaglioIgnotoAlVelo);
				PC->DispatchUnitClickForTest(Bersaglio, Attore);
				Esito.Click = (Attore->PlannedAbilityIndex == Idx && Attore->PlannedAttackTarget.Get() == Bersaglio)
					? ERic3Click::Accetta : ERic3Click::Rifiuta;
				Esito.bBordoDopoIlClick = Attore->bHasPlannedCoverEdge;
				Attore->SelectAbility(INDEX_NONE); // il turno non porta lo stato di UI dell'oracolo
			}
			if (Esito.Click != ERic3Click::Accetta)
			{
				// Il click non ha prodotto il piano (o non c'era niente da cliccare): lo si scrive a mano, perche' la riga
				// misura anche cio' che il RESOLVER fa di un piano che il click non produce.
				Attore->PlannedAbilityIndex = Idx;
				if (S.Azione == ERic3Azione::AttaccoACella)
				{
					Attore->DeclareAttackOnCell(CellaBersaglio);
				}
				else
				{
					Attore->DeclareAttackOnUnit(Bersaglio); // `nullptr` = piano senza bersaglio-unita' ne' cella
				}
			}
			if ((S.Azione == ERic3Azione::CreateCover || S.Azione == ERic3Azione::Interact) && S.bBordoDichiarato)
			{
				// Il lato dichiarato, come lo scrive `HandleTargetEdge`: la cella del bersaglio e il bordo.
				Attore->DeclareAttackOnCell(CellaBersaglio);
				Attore->PlannedCoverEdge = ERTHexDirection::W;
				Attore->bHasPlannedCoverEdge = true;
			}
			else if ((S.Azione == ERic3Azione::CreateCover || S.Azione == ERic3Azione::Interact) && S.bPianoACellaSenzaLato)
			{
				// Stesso bersaglio a cella del lato dichiarato, ma senza lato: la sola cosa che varia rispetto al controllo e' `bHasPlannedCoverEdge`.
				Attore->DeclareAttackOnCell(CellaBersaglio);
			}

			// Dopo il click: un bersaglio che il click ha accettato da vivo e poi muore e' un fatto di risoluzione.
			if (Bersaglio && S.Bersaglio == ERic3Bersaglio::Morto) { Bersaglio->Health = 0; SaluteBersaglio = 0; }
			if (S.bAttoreCadeNelBlast)
			{
				ARTUnit* Carnefice = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 1, 0));
				if (!Test.TestNotNull(*FString::Printf(TEXT("%s: carnefice"), Nome), Carnefice))
				{
					PagaDestroyWorld(World);
					return Esito;
				}
				Attore->Health = 1;
				Carnefice->PlannedAbilityIndex = 0;
				Carnefice->DeclareAttackOnUnit(Attore);
			}
		}

		if (S.bAttoreCaduto) { Attore->Health = 0; } // arriva al Blast col piano ancora addosso

		if (S.Azione == ERic3Azione::AttaccoACella)
		{
			// 🔑 **PREMESSA SULL'ORDINE DI RISOLUZIONE** (come nel banco di `CoverTieRealOrder`: la funzione vera, `SortUnitsForResolution`,
			// sulle unita' del mondo). `Ctx.HexUnits` nasce da quest'ordine (`GatherBlastUnits`), e il tiro a cella cappato misura DA
			// `Attacker.Cell`, cioe' dalla cella di chi tira, e non da un'altra unita'. Una riga in cui chi tira e' l'ULTIMO dell'ordine
			// non distingue `Attacker.Cell` da `HexUnits.Last().Cell`: ogni riga dichiara se lo e' (`bAttoreNonUltimo`), e questa
			// misura l'ordine invece di darlo per scontato. ⚠️ L'ordine e' quello PRE-scatto: l'unico scatto dei banchi (il compagno)
			// sposta la sola Y, e la prima chiave dopo il piano e' la X.
			TArray<AActor*> Attori;
			UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Attori);
			TArray<ARTUnit*> Ordine;
			for (AActor* A : Attori)
			{
				if (ARTUnit* U = Cast<ARTUnit>(A)) { Ordine.Add(U); }
			}
			URTActionQueueLibrary::SortUnitsForResolution(Ordine);
			Esito.bAttoreUltimo = Ordine.Num() > 0 && Ordine.Last() == Attore;
		}

		const int32 ArchiPrima = MapActor->MapAsset->Transitions.Num();
		const TWeakObjectPtr<ARTUnit> AttoreDebole(Attore);
		PagaRunTurn(TM);

		// I caduti si leggono da distrutti: `ConcludeTurn` chiama `DestroyDefeatedUnits`.
		ARTUnit* AttoreDopo = AttoreDebole.Get(/*bEvenIfPendingKill*/ true);
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: l'attore e' ancora leggibile"), Nome), AttoreDopo))
		{
			PagaDestroyWorld(World);
			return Esito;
		}

		Esito.Cooldown = AttoreDopo->GetAbilityCooldown(Idx);
		Esito.bHaPagato = PagaHaPagato(AttoreDopo, Idx);
		Esito.CellaFinale = AttoreDopo->Cell;
		Esito.bAttoreVivo = AttoreDopo->IsAlive();
		const ARTUnit* BersaglioDopo = BersaglioDebole.Get(/*bEvenIfPendingKill*/ true);
		Esito.bDanno = BersaglioDopo && BersaglioDopo->Health < SaluteBersaglio;
		if (BersaglioDopo) { Esito.CellaDelBersaglioDopo = BersaglioDopo->Cell; }
		Esito.bArco = MapActor->MapAsset->Transitions.Num() > ArchiPrima;
		const FRTHexCellData* DatiCella = MapActor->MapAsset->FindCell(CellaBersaglio);
		Esito.bFuoco = DatiCella && DatiCella->Surface == ERTHexSurface::Fire;

		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Phase == ERTMatchPhase::Dash && E.Category == ERTLogCategory::Combat)
			{
				++Esito.DanniNelDash; // nessuna riga del banco colpisce nel Dash: l'impatto di una carica e i colpi risolvono nel Blast
			}
			if (E.Category == ERTLogCategory::Reaction)
			{
				if (E.Outcome == static_cast<uint8>(ERTReactionOutcome::Activated)) { ++Esito.Attivate; }
				if (E.Outcome == static_cast<uint8>(ERTReactionOutcome::NotTriggered)) { ++Esito.NonInnescate; }
			}
			if (E.Category == ERTLogCategory::Environment)
			{
				if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverRejected)) { ++Esito.CopertureRifiutate; }
				if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverCreated)) { ++Esito.CopertureErette; }
			}
			if (E.Category == ERTLogCategory::Fallback && E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.UnitId == AttoreDopo->StableUnitId)
			{
				Esito.Rifiuti.Add(E.Amount);
			}
		}

		Esito.bValido = true;
		PagaDestroyWorld(World);
		return Esito;
	}

	/** Confronta cio' che la scena ha prodotto con cio' che la riga dichiara. Ritorna falso se la scena non si e' montata. */
	bool Ric3Verifica(FAutomationTestBase& Test, const TCHAR* Nome, const TCHAR* Lato, const FRic3Scena& S,
		const FRic3Atteso& A, const TCHAR* Perche)
	{
		const FString Etichetta = FString::Printf(TEXT("%s [%s]"), Nome, Lato);
		const FRic3Esito E = Ric3Gira(Test, *Etichetta, S);
		if (!E.bValido) { return false; }

		Test.AddInfo(FString::Printf(TEXT("%s -> click %s, cooldown residuo %d, rifiuti %d, danno %d, arco %d, fuoco %d, reazioni attivate %d / non innescate %d, coperture erette %d / rifiutate %d, cella finale (%d,%d,%d)"),
			*Etichetta, E.Click == ERic3Click::Accetta ? TEXT("ACCETTA") : E.Click == ERic3Click::Rifiuta ? TEXT("RIFIUTA") : TEXT("n/d"),
			E.Cooldown, E.Rifiuti.Num(), E.bDanno ? 1 : 0, E.bArco ? 1 : 0, E.bFuoco ? 1 : 0, E.Attivate,
			E.NonInnescate, E.CopertureErette, E.CopertureRifiutate, E.CellaFinale.X, E.CellaFinale.Y, E.CellaFinale.Layer));

		// 🔑 **L'ORACOLO DEL CLICK**: la tabella «il click accetta?» non e' prosa, e' misurata sul percorso di produzione
		// (`DispatchUnitClick` -> `HandleClickOnUnit`). Ogni riga dichiara cosa il click fa; dove la regola e' «paga se e solo
		// se il click accetta», l'equivalenza e' asserita. Dove il click accetta un piano che non dovrebbe accettare e la causa
		// resta gratis (`bClickDaRiparare`, [D-575] (b)) la riga e' il pin di una decisione, non una regola: l'equivalenza non si
		// asserisce, si asserisce che il click accetta e che l'azione e' gratis, finche' il click non e' riparato.
		Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'oracolo del click risponde come la riga dichiara"), *Etichetta),
			E.Click == A.Click);
		if (A.Click != ERic3Click::ND)
		{
			const bool bAccetta = E.Click == ERic3Click::Accetta;
			if (A.bClickDaRiparare)
			{
				Test.TestTrue(*FString::Printf(TEXT("%s: D-575 (b), CLICK DA RIPARARE: il click accetta un piano che non dovrebbe e l'azione e' gratis"), *Etichetta),
					bAccetta && !E.bHaPagato);
			}
			else if (A.bSenzaEquivalenza)
			{
				// Il click accetta, poi chi agisce cade: pagare o no lo decide la guardia di vita del sito, non il click.
				Test.TestTrue(*FString::Printf(TEXT("%s: il click accetta"), *Etichetta), bAccetta);
			}
			else
			{
				Test.TestEqual(*FString::Printf(TEXT("%s: il click accetta <=> l'azione paga"), *Etichetta), bAccetta, E.bHaPagato);
			}
		}
		if (S.Azione == ERic3Azione::AttaccoACella)
		{
			// L'origine del tiro cappato e' la cella di chi tira, non quella dell'ultima unita' dell'ordine di risoluzione: la riga
			// dichiara se le due coincidono, e la misura (`SortUnitsForResolution` sulle unita' del mondo).
			Test.TestEqual(*FString::Printf(TEXT("%s: premessa: chi tira e' l'ultima unita' nell'ordine di risoluzione?"), *Etichetta),
				E.bAttoreUltimo, !A.bAttoreNonUltimo);
		}
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: coperture erette"), *Etichetta), E.CopertureErette, A.CopertureErette);
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: coperture rifiutate"), *Etichetta), E.CopertureRifiutate, A.CopertureRifiutate);
		if (A.bControllaBordo)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il click ha scritto il lato?"), *Etichetta),
				E.bBordoDopoIlClick, A.bBordo);
		}
		if (A.bMuoreNelDash)
		{
			// Senza queste due premesse la riga potrebbe misurare un attore morto PRIMA del lock-in (che `CollectLivingUnits` esclude dalle origini
			// congelate): muore, e muore per un danno della fase Dash, quindi era vivo quando il click ha giudicato e l'origine e' stata congelata.
			Test.TestFalse(*FString::Printf(TEXT("%s: premessa: chi agisce e' morto a fine turno"), *Etichetta), E.bAttoreVivo);
			Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il danno che l'ha ucciso e' della fase Dash"), *Etichetta), E.DanniNelDash, 1);
		}

		// Le premesse: la riga misura quello che dichiara. ZERO compresi, cosi' un'altra causa non puo' pagare al suo posto.
		const int32 VociAttese = A.Voce == ERTActionInvalidReason::None ? 0 : 1;
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: numero di voci di rifiuto"), *Etichetta), E.Rifiuti.Num(), VociAttese);
		if (A.Voce != ERTActionInvalidReason::None)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il motivo della voce di rifiuto"), *Etichetta),
				E.Rifiuti.Contains(static_cast<int32>(A.Voce)), true);
		}
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il bersaglio incassa?"), *Etichetta), E.bDanno, A.bDanno);
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: l'arco si apre?"), *Etichetta), E.bArco, A.bArco);
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la cella brucia?"), *Etichetta), E.bFuoco, A.bFuoco);
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: reazioni attivate"), *Etichetta), E.Attivate, A.Attivate);
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: reazioni non innescate"), *Etichetta), E.NonInnescate, A.NonInnescate);
		if (A.bControllaCella)
		{
			Test.TestTrue(*FString::Printf(TEXT("%s: premessa: dove chi agisce si trova a fine turno"), *Etichetta),
				E.CellaFinale == A.CellaFinale);
		}
		if (A.bControllaBersaglio)
		{
			Test.TestTrue(*FString::Printf(TEXT("%s: premessa: dove il bersaglio si trova a fine turno"), *Etichetta),
				E.CellaDelBersaglioDopo == A.CellaDelBersaglio);
		}

		// UNA volta sola: chi paga lascia il residuo di PagaCooldown, decrementato dal Cleanup, e non di piu'.
		if (A.ResiduoAtteso != INDEX_NONE)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: %s (residuo atteso)"), *Etichetta, Perche), E.Cooldown, A.ResiduoAtteso);
		}
		else if (A.bPaga)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: %s (residuo: pagato una volta)"), *Etichetta, Perche), E.Cooldown, PagaCooldown - 1);
		}
		else
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: %s (residuo: non pagato)"), *Etichetta, Perche), E.Cooldown, 0);
		}
		return true;
	}

	FRic3Atteso Ric3Atteso(bool bPaga, ERic3Click Click, ERTActionInvalidReason Voce = ERTActionInvalidReason::None)
	{
		FRic3Atteso A;
		A.bPaga = bPaga;
		A.Click = Click;
		A.Voce = Voce;
		return A;
	}

	/** Chi agisce cade fra il click e il Blast: il click accetta, e a decidere se si paga e' la guardia di vita del sito. */
	FRic3Atteso Ric3DopoLaCaduta(FRic3Atteso A)
	{
		A.bSenzaEquivalenza = true;
		return A;
	}
}

/**
 * **Le cause che emergono SOLO a risoluzione, su un piano che il click avrebbe accettato, PAGANO** ([D-571], RIC-3).
 *
 * Ogni riga e' una coppia: il CONTROLLO e il CASO differiscono per UNA variabile, e il controllo prova che l'azione
 * parte davvero (colpisce, apre l'arco, accende la cella). Senza il controllo un «non e' successo niente» sarebbe verde
 * anche con una fixture che non fa succedere niente.
 *
 * - `ModifyArc` con bersaglio-unita' GIA' MORTO: il click lo ha accettato da vivo e noto, e la morte e' un fatto di
 *   risoluzione — come la cura su un alleato caduto ([D-197]). Controparte gratis: nessun bersaglio dichiarato, un piano
 *   che il click non produce.
 * - azione AMBIENTALE con bersaglio-unita' morto: stessa forma, in `ResolveEnvironment` (Cleanup).
 * - bersaglio-UNITA' fuori portata SOLO per il cap del Fumo NON OSSERVATO: e' il TESTO di `R7` ([D-563] (2)). Il click
 *   ([D-465] (3)) giudica sulla mappa ottimistica, che rende neutre le celle fuori da `ExploredCells U VisibleCells`, e lo
 *   ha accettato; il costo dell'ottimismo si paga qui. Controparti gratis (in `Actions.Blast.UndecidedCausesStayFree`): Fumo
 *   su una cella OSSERVATA (il click vede il cap e rifiuta) e oltre la portata DICHIARATA.
 * - tiro a CELLA cappato dal Fumo NON OSSERVATO (`R7`, ultime righe del primo blocco): la stessa regola con la stessa mappa e la
 *   stessa funzione (`FuoriPortataMaIlClickAccettava`, sulla mappa di `ClickMapForPaymentClassification`), decisa dall'autore il
 *   2026-11-03. Controparte gratis (in `Actions.Blast.UndecidedCausesStayFree`): il Fumo su una cella OSSERVATA, dove il click
 *   rifiuta.
 * - **[D-574] il Fumo messo in vista dallo scatto di un COMPAGNO** (tiro a cella e bersaglio-unita'): la conoscenza che conta e'
 *   quella con cui il click ha giudicato, congelata all'inizio della risoluzione, non quella del Blast. Il controllo e' lo stesso
 *   compagno fermo.
 * - **[D-575] (a) lo SCATTO FERMATO** (bersaglio-unita', `ModifyArc` e, per riflesso, il tiro a cella oltre la portata dichiarata,
 *   `R9`): il click ha giudicato dalla cella d'arrivo dello scatto, il Blast misura da dove l'unita' si e' fermata. Il controllo e'
 *   lo scatto che arriva; la controparte gratis e' lo scatto fermato con un bersaglio che il click rifiutava anche dalla cella
 *   d'arrivo. (Le decisioni sono registrate in #3981: i numeri valgono al merge.)
 *
 * 🔑 **L'oracolo del click**: ogni riga dichiara cosa fa il click di produzione (`DispatchUnitClick` ->
 * `HandleClickOnUnit`, sulla STESSA fixture, con la conoscenza di squadra rinfrescata come a inizio turno) e asserisce
 * «il click accetta <=> l'azione paga». Il click giudica PRIMA di cio' che emerge (la morte del bersaglio avviene dopo il
 * click); dove l'attore cade fra il click e il Blast decide la guardia di vita del sito, e la riga lo dichiara. Non c'e'
 * oracolo dove non c'e' niente da cliccare (nessun bersaglio) o dove il click non puo' armare l'azione (gia' in ricarica).
 *
 * ⚠️ **Approssimazione del predicato, e dove sbaglia**: il resolver rifa la sola domanda di PORTATA del click, con la conoscenza e
 * l'origine con cui il click ha giudicato (congelate al lock-in). Un Fumo nato dopo l'esplorazione su una cella ora fuori vista fa
 * rifiutare il click (D-534 (3), `Q4`) e il tiro resta gratis anche se la squadra non l'ha mai visto: pinnato in
 * `UndecidedCausesStayFree`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionEmergingCausesPayTest,
	"RefactorTactics.Actions.Blast.ResolutionEmergingCausesPay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionEmergingCausesPayTest::RunTest(const FString&)
{
	// --- ModifyArc, bersaglio-unita' morto ---------------------------------------------------------
	{
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::ModifyArc;
		Controllo.Distanza = 2;
		FRic3Atteso AttesoControllo = Ric3Atteso(true, ERic3Click::Accetta);
		AttesoControllo.bArco = true;
		if (!Ric3Verifica(*this, TEXT("ModifyArc: bersaglio vivo"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("l'arco si apre e l'azione paga come prima"))) { return false; }

		FRic3Scena Morto = Controllo;
		Morto.Bersaglio = ERic3Bersaglio::Morto;
		if (!Ric3Verifica(*this, TEXT("ModifyArc: bersaglio-unita' gia' morto"), TEXT("caso"), Morto,
			Ric3Atteso(true, ERic3Click::Accetta),
			TEXT("il click lo ha accettato da vivo: la morte e' un fatto di risoluzione e la ricarica parte"))) { return false; }

		FRic3Scena Assente = Controllo;
		Assente.Bersaglio = ERic3Bersaglio::Assente;
		if (!Ric3Verifica(*this, TEXT("ModifyArc: nessun bersaglio dichiarato"), TEXT("controparte"), Assente,
			Ric3Atteso(false, ERic3Click::ND),
			TEXT("il click non produce un piano senza bersaglio: resta gratis"))) { return false; }

		FRic3Scena Caduto = Morto;
		Caduto.bAttoreCaduto = true;
		if (!Ric3Verifica(*this, TEXT("ModifyArc: bersaglio morto e attore gia' caduto"), TEXT("controparte"), Caduto,
			Ric3DopoLaCaduta(Ric3Atteso(false, ERic3Click::Accetta)),
			TEXT("D-209: chi agisce deve essere vivo quando l'azione si annota"))) { return false; }

		// 🔴 La guardia `CanUseAbility` del ramo nuovo: `ConsumeAbility` scrive un valore FISSO, quindi chiamato su
		// un'abilita' GIA' in ricarica la riporterebbe al valore pieno. Nessun click puo' armare un'azione in ricarica: il
		// piano e' scritto a mano, e il residuo dice che non e' stata ripagata.
		FRic3Scena InRicarica = Morto;
		InRicarica.bGiaInRicarica = true;
		FRic3Atteso AttesoInRicarica = Ric3Atteso(false, ERic3Click::ND);
		AttesoInRicarica.ResiduoAtteso = PagaCooldown - 2;
		if (!Ric3Verifica(*this, TEXT("ModifyArc: bersaglio morto, azione gia' in ricarica"), TEXT("controparte"), InRicarica,
			AttesoInRicarica, TEXT("la guardia CanUseAbility: un secondo addebito la riporterebbe al valore pieno"))) { return false; }

		// Chi muore nello STESSO Blast: ModifyArc annota da vivo a inizio Blast e `SpendStartedAbilities` non rilegge la vita,
		// come per il ramo normale di ModifyArc e per la cura ([D-209]). Pinnato come e' oggi.
		FRic3Scena CadeNelBlast = Morto;
		CadeNelBlast.bAttoreCadeNelBlast = true;
		// Il residuo e' `PagaCooldown` e non `PagaCooldown - 1`: la ricarica e' scritta (pagata), ma chi muore nel Blast non
		// arriva al decremento del Cleanup.
		FRic3Atteso AttesoCadeNelBlast = Ric3Atteso(true, ERic3Click::Accetta);
		AttesoCadeNelBlast.ResiduoAtteso = PagaCooldown;
		if (!Ric3Verifica(*this, TEXT("ModifyArc: bersaglio morto, l'attore cade nello stesso Blast"), TEXT("caso"), CadeNelBlast,
			AttesoCadeNelBlast,
			TEXT("annotata da vivo a inizio Blast: come il ramo normale di ModifyArc"))) { return false; }
	}

	// --- azione ambientale, bersaglio-unita' morto ------------------------------------------------
	{
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::Ignite;
		Controllo.Distanza = 3;
		FRic3Atteso AttesoControllo = Ric3Atteso(true, ERic3Click::Accetta);
		AttesoControllo.bFuoco = true;
		AttesoControllo.bDanno = true; // chi sta sulla cella in fiamme prende fuoco: l'azione ha fatto il suo lavoro
		if (!Ric3Verifica(*this, TEXT("Ignite: bersaglio vivo"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("la cella brucia e l'azione paga come prima"))) { return false; }

		FRic3Scena Morto = Controllo;
		Morto.Bersaglio = ERic3Bersaglio::Morto;
		if (!Ric3Verifica(*this, TEXT("Ignite: bersaglio-unita' gia' morto"), TEXT("caso"), Morto,
			Ric3Atteso(true, ERic3Click::Accetta),
			TEXT("il click lo ha accettato da vivo: la morte e' un fatto di risoluzione e la ricarica parte"))) { return false; }

		FRic3Scena Assente = Controllo;
		Assente.Bersaglio = ERic3Bersaglio::Assente;
		if (!Ric3Verifica(*this, TEXT("Ignite: nessun bersaglio dichiarato"), TEXT("controparte"), Assente,
			Ric3Atteso(false, ERic3Click::ND),
			TEXT("il click non produce un piano senza bersaglio: resta gratis"))) { return false; }

		FRic3Scena Caduto = Morto;
		Caduto.bAttoreCaduto = true;
		if (!Ric3Verifica(*this, TEXT("Ignite: bersaglio morto e attore gia' caduto"), TEXT("controparte"), Caduto,
			Ric3DopoLaCaduta(Ric3Atteso(false, ERic3Click::Accetta)),
			TEXT("D-209: un cadavere non paga un'azione che non ha tentato"))) { return false; }

		// Chi muore nel Blast NON paga l'azione ambientale: il Cleanup controlla `IsAlive()` DOPO il Blast. E' la differenza
		// con ModifyArc (annotata da vivo a inizio Blast), pinnata come e' oggi.
		FRic3Scena CadeNelBlast = Morto;
		CadeNelBlast.bAttoreCadeNelBlast = true;
		if (!Ric3Verifica(*this, TEXT("Ignite: bersaglio morto, l'attore cade nel Blast"), TEXT("controparte"), CadeNelBlast,
			Ric3DopoLaCaduta(Ric3Atteso(false, ERic3Click::Accetta)),
			TEXT("il Cleanup controlla che chi agisce sia vivo dopo il Blast"))) { return false; }

		// ⚠️ **Il ramo a bersaglio VIVO della STESSA funzione NON ha una guardia di vita** (preesistente, non toccato): un
		// lanciatore morto nel Blast accende comunque la cella e paga. Pinnato come e' oggi, perche' la prosa «chi muore nel
		// Blast non paga l'azione ambientale» vale SOLO per il ramo a bersaglio morto qui sopra, e perche' la ricarica di un
		// morto entra nell'hash del turno che chiude la partita (`BuildUnitDigests` include i caduti, e
		// `CaptureFinalStateHash` gira prima di `DestroyDefeatedUnits`). Il residuo e' `PagaCooldown`: nessun decremento per chi e' caduto.
		FRic3Scena VivoCadeNelBlast = Controllo;
		VivoCadeNelBlast.bAttoreCadeNelBlast = true;
		FRic3Atteso AttesoVivoCade = Ric3DopoLaCaduta(Ric3Atteso(true, ERic3Click::Accetta));
		AttesoVivoCade.bFuoco = true;
		AttesoVivoCade.bDanno = true;
		AttesoVivoCade.ResiduoAtteso = PagaCooldown;
		if (!Ric3Verifica(*this, TEXT("Ignite: bersaglio VIVO, l'attore cade nel Blast"), TEXT("controparte"), VivoCadeNelBlast,
			AttesoVivoCade, TEXT("pin: nessuna guardia di vita sul ramo a bersaglio vivo, il lanciatore morto agisce e paga"))) { return false; }
	}

	// --- bersaglio-unita' fuori portata SOLO per il cap del Fumo NON OSSERVATO --------------------
	{
		// Geometria (portata portata a 6: quella di Ivrin, 4, non lascia spazio): chi agisce in (0,0,0) vede solo l'intorno
		// (vista 0, consapevolezza ravvicinata 2); un compagno due celle dietro il bersaglio, in (7,0,0) per il bersaglio in
		// (5,0,0), vede il bersaglio e nient'altro. Il Fumo in (3,0,0) sta a distanza 3 da chi tira e 4 dal compagno: cella
		// NON OSSERVATA, quindi neutra per la mappa ottimistica del click. In (1,0,0) sta nell'intorno: OSSERVATA.
		FRic3Scena Controllo;
		Controllo.Distanza = 5;
		Controllo.PortataSovrascritta = 6;
		Controllo.bTestimone = true;
		FRic3Atteso AttesoControllo = Ric3Atteso(true, ERic3Click::Accetta);
		AttesoControllo.bDanno = true;
		if (!Ric3Verifica(*this, TEXT("bersaglio-unita' a distanza 5, visto dal compagno"), TEXT("controllo"), Controllo,
			AttesoControllo, TEXT("senza Fumo l'attacco colpisce e paga"))) { return false; }

		FRic3Scena NonOsservato = Controllo;
		NonOsservato.FumoX = 3;
		if (!Ric3Verifica(*this, TEXT("Fumo NON osservato fra chi tira e il bersaglio"), TEXT("caso"), NonOsservato,
			Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange),
			TEXT("il click giudica sulla mappa ottimistica e accetta: il cap del Fumo emerge solo a risoluzione"))) { return false; }

		// 🔑 Il confine: distanza ESATTAMENTE pari alla portata dichiarata, ancora dentro. Uccide `<=` -> `<` nel predicato.
		FRic3Scena AlConfine = NonOsservato;
		AlConfine.Distanza = 0;
		AlConfine.bDistanzaPariAllaPortata = true;
		if (!Ric3Verifica(*this, TEXT("Fumo NON osservato, bersaglio alla portata esatta"), TEXT("caso"), AlConfine,
			Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange),
			TEXT("a distanza pari alla portata il click accetta ancora"))) { return false; }

		// 🔑 La mira e' quella CONGELATA al lock-in ([D-415]): il bersaglio scatta di due celle e finisce oltre la portata, ma
		// la distanza che conta e' quella dalla cella mirata. Uccide `TargetCell` -> cella viva nel predicato.
		FRic3Scena Scatta = NonOsservato;
		Scatta.bBersaglioScattaLontano = true;
		FRic3Atteso AttesoScatta = Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange);
		AttesoScatta.bControllaBersaglio = true;
		AttesoScatta.CellaDelBersaglio = FRTCellId(5, 2, 0); // portata 6 - 1 = 5 in X, due celle lungo r: distanza 7
		if (!Ric3Verifica(*this, TEXT("Fumo NON osservato, il bersaglio scatta oltre la portata"), TEXT("caso"), Scatta,
			AttesoScatta,
			TEXT("la mira e' congelata al lock-in: dove il bersaglio e' finito non conta"))) { return false; }

		// R10: chi agisce e' caduto prima del Blast, il predicato lo annota ma il pagamento rilegge la vita.
		FRic3Scena Caduto = NonOsservato;
		Caduto.bAttoreCaduto = true;
		if (!Ric3Verifica(*this, TEXT("Fumo NON osservato, attore gia' caduto"), TEXT("controparte"), Caduto,
			Ric3DopoLaCaduta(Ric3Atteso(false, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange)),
			TEXT("R10, D-209: per chi attacca «spesa» vuol dire sopravvissuto alla fase"))) { return false; }
	}

	// --- R7, il TIRO A CELLA cappato dal Fumo NON osservato ----------------------------------------
	{
		// `R7` ([D-563] (2), confermato dall'autore) dice «cappato dal Fumo NON osservato», e dal 2026-11-03 (decisione d'autore)
		// il codice e' fedele al testo: `CollectHexAttacks` raccoglie in `TerrainCappedIntents` ogni tiro cappato, e il
		// pagamento (`ClassifyRangeLostPayment`) tiene solo quelli che il click, sulla propria mappa ottimistica
		// (`PlayerAimMap`, la stessa del click su un'unita'), avrebbe accettato. La riga con il Fumo OSSERVATO, dove il click
		// vede il cap e rifiuta, e' il controllo gratis e sta in `Actions.Blast.UndecidedCausesStayFree`. Chi tira vede solo
		// l'intorno (vista 0); l'attacco base e' dichiarato su una cella VUOTA alla portata esatta (4), il Fumo sta in (3,0,0).
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::AttaccoACella;
		Controllo.Bersaglio = ERic3Bersaglio::Assente;
		Controllo.Distanza = 4;
		Controllo.Visione = 0;
		if (!Ric3Verifica(*this, TEXT("R7 controllo: tiro a cella vuota in portata, nessun Fumo"), TEXT("controllo"), Controllo,
			Ric3Atteso(true, ERic3Click::Accetta),
			TEXT("l'area vuota in portata e' partita ed e' un esito (R1): il click la accetta e paga"))) { return false; }

		FRic3Scena NonOsservato = Controllo;
		NonOsservato.FumoX = 3; // distanza 3 da chi tira: fuori dall'intorno, cella NON osservata
		if (!Ric3Verifica(*this, TEXT("R7: tiro a CELLA, Fumo NON osservato"), TEXT("caso"), NonOsservato,
			Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange),
			TEXT("il click giudica sulla mappa ottimistica e accetta; R7 paga: click e R7 CONCORDANO"))) { return false; }
	}

	// --- D-574: il Fumo messo in vista dallo scatto di un COMPAGNO paga ----------------------------
	{
		// La conoscenza con cui il click ha giudicato si congela all'inizio della risoluzione ([D-574], registrata in #3981: il numero vale al
		// merge). Un compagno che a inizio turno non vede il Fumo (tre celle) e con uno scatto di due gli arriva a una cella lo mette in
		// vista NEL BLAST (`RefreshTeamKnowledgeForBlast` osserva le posizioni post-Dash), non in pianificazione: il click ha accettato, e
		// la conoscenza del Blast conterrebbe il Fumo (il tiro sarebbe gratis). Paga, perche' conta la conoscenza della PIANIFICAZIONE.
		// Il controllo e' lo STESSO compagno fermo: l'unica variabile fra i due e' lo scatto, e il tiro paga in entrambi i casi.
		FRic3Scena CellaFermo;
		CellaFermo.Azione = ERic3Azione::AttaccoACella;
		CellaFermo.Bersaglio = ERic3Bersaglio::Assente;
		CellaFermo.Distanza = 4;
		CellaFermo.Visione = 0;
		CellaFermo.FumoX = 3;
		CellaFermo.bCompagnoLontano = true;
		FRic3Atteso AttesoCellaFermo = Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange);
		AttesoCellaFermo.bAttoreNonUltimo = true; // il compagno ordina dopo chi tira (X = FumoX = 3)
		if (!Ric3Verifica(*this, TEXT("D-574 tiro a CELLA, Fumo NON osservato, compagno lontano e fermo"), TEXT("controllo"), CellaFermo,
			AttesoCellaFermo,
			TEXT("il compagno non vede il Fumo ne' in pianificazione ne' nel Blast: il click accetta e R7 paga"))) { return false; }

		FRic3Scena CellaScatta = CellaFermo;
		CellaScatta.bCompagnoScatta = true;
		if (!Ric3Verifica(*this, TEXT("D-574 tiro a CELLA, il compagno scatta e mette il Fumo in vista"), TEXT("caso"), CellaScatta,
			AttesoCellaFermo,
			TEXT("il click accetta (il Fumo e' ignoto in pianificazione) e conta la pianificazione, non il Blast: paga"))) { return false; }

		// Lo stesso per il bersaglio-UNITA': un testimone dietro il bersaglio lo fa vedere alla squadra, il Fumo in (3,0,0) no.
		FRic3Scena UnitaFermo;
		UnitaFermo.Distanza = 5;
		UnitaFermo.PortataSovrascritta = 6;
		UnitaFermo.bTestimone = true;
		UnitaFermo.FumoX = 3;
		UnitaFermo.bCompagnoLontano = true;
		if (!Ric3Verifica(*this, TEXT("D-574 bersaglio-unita', Fumo NON osservato, compagno lontano e fermo"), TEXT("controllo"), UnitaFermo,
			Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange),
			TEXT("il compagno non vede il Fumo: il click accetta e il tiro paga"))) { return false; }

		FRic3Scena UnitaScatta = UnitaFermo;
		UnitaScatta.bCompagnoScatta = true;
		if (!Ric3Verifica(*this, TEXT("D-574 bersaglio-unita', il compagno scatta e mette il Fumo in vista"), TEXT("caso"), UnitaScatta,
			Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange),
			TEXT("la conoscenza che conta e' quella con cui il click ha giudicato: il Fumo messo in vista dopo il lock-in non rende il tiro gratis"))) { return false; }
	}

	// --- D-575 (a), lo SCATTO FERMATO: il click ha giudicato dalla cella d'ARRIVO dello scatto ------------
	{
		// Il click giudica da `AimOriginFor(ResolutionPhase)`, la cella d'arrivo dello scatto PIANIFICATO (2,0,0); un ostacolo sulla cella
		// d'arrivo ferma lo scatto in (1,0,0), e il Blast misura da `Attacker.Cell`. Il pagamento rifa' la domanda del click con
		// l'origine CHE IL CLICK HA USATO, congelata al lock-in. I COLPI e il TurnLog non cambiano: cambia solo la ricarica.
		// Il controllo di ogni coppia e' lo stesso piano con lo scatto che arriva; la controparte gratis e' lo scatto fermato con il
		// bersaglio che il click avrebbe RIFIUTATO anche dalla cella d'arrivo (una cella oltre la portata): senza la controparte non si
		// distingue «paga la domanda del click dall'origine del click» da «paga ogni fuori portata dopo uno scatto fermato».
		FRic3Scena Controllo; // il bersaglio e' esattamente alla portata da (2,0,0)
		Controllo.bScattoPrecede = true;
		FRic3Atteso AttesoControllo = Ric3Atteso(true, ERic3Click::Accetta);
		AttesoControllo.bDanno = true;
		AttesoControllo.bControllaCella = true;
		AttesoControllo.CellaFinale = FRTCellId(2, 0, 0);
		if (!Ric3Verifica(*this, TEXT("scatto compiuto, poi attacco"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("lo scatto arriva, l'attacco colpisce e paga"))) { return false; }

		FRic3Scena Fermato = Controllo;
		Fermato.bOstacoloSulloScatto = true;
		FRic3Atteso AttesoFermato = Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange);
		AttesoFermato.bControllaCella = true;
		AttesoFermato.CellaFinale = FRTCellId(1, 0, 0);
		if (!Ric3Verifica(*this, TEXT("scatto fermato, poi attacco"), TEXT("caso"), Fermato, AttesoFermato,
			TEXT("il click ha accettato dalla cella d'arrivo dello scatto e l'esecuzione lo impedisce: paga"))) { return false; }

		FRic3Scena FermatoEOltre = Fermato;
		FermatoEOltre.bMiraOltreLaFineDelloScatto = true;
		FRic3Atteso AttesoFermatoEOltre = Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::OutOfRange);
		AttesoFermatoEOltre.bControllaCella = true;
		AttesoFermatoEOltre.CellaFinale = FRTCellId(1, 0, 0);
		if (!Ric3Verifica(*this, TEXT("scatto fermato, bersaglio oltre la portata anche dalla cella d'arrivo"), TEXT("controparte"), FermatoEOltre,
			AttesoFermatoEOltre,
			TEXT("il click rifiutava anche dalla cella d'arrivo dello scatto: geometria nota in pianificazione, gratis"))) { return false; }

		// `ModifyArc`: stessa scena, stesso sito (il bersaglio-unita' fuori portata dichiarata, ma da `Unit->Cell`).
		FRic3Scena ArcoControllo;
		ArcoControllo.Azione = ERic3Azione::ModifyArc;
		ArcoControllo.bScattoPrecede = true;
		FRic3Atteso AttesoArcoControllo = Ric3Atteso(true, ERic3Click::Accetta);
		AttesoArcoControllo.bArco = true;
		AttesoArcoControllo.bControllaCella = true;
		AttesoArcoControllo.CellaFinale = FRTCellId(2, 0, 0);
		if (!Ric3Verifica(*this, TEXT("scatto compiuto, poi ModifyArc"), TEXT("controllo"), ArcoControllo, AttesoArcoControllo,
			TEXT("lo scatto arriva, l'arco si apre e l'azione paga"))) { return false; }

		FRic3Scena ArcoFermato = ArcoControllo;
		ArcoFermato.bOstacoloSulloScatto = true;
		FRic3Atteso AttesoArcoFermato = Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange);
		AttesoArcoFermato.bControllaCella = true;
		AttesoArcoFermato.CellaFinale = FRTCellId(1, 0, 0);
		if (!Ric3Verifica(*this, TEXT("scatto fermato, poi ModifyArc"), TEXT("caso"), ArcoFermato, AttesoArcoFermato,
			TEXT("il click ha accettato dalla cella d'arrivo dello scatto e l'arco, fuori portata da dove l'unita' si e' fermata, non si apre: paga"))) { return false; }

		FRic3Scena ArcoFermatoEOltre = ArcoFermato;
		ArcoFermatoEOltre.bMiraOltreLaFineDelloScatto = true;
		FRic3Atteso AttesoArcoFermatoEOltre = Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::OutOfRange);
		AttesoArcoFermatoEOltre.bControllaCella = true;
		AttesoArcoFermatoEOltre.CellaFinale = FRTCellId(1, 0, 0);
		if (!Ric3Verifica(*this, TEXT("scatto fermato, ModifyArc oltre la portata anche dalla cella d'arrivo"), TEXT("controparte"), ArcoFermatoEOltre,
			AttesoArcoFermatoEOltre,
			TEXT("il click rifiutava anche dalla cella d'arrivo: gratis"))) { return false; }

		// 🔴 La GUARDIA DI VITA del ramo `ModifyArc` fuori portata: «vivo quando annota» ([D-209]). Chi agisce e' VIVO al lock-in (altrimenti
		// `CollectLivingUnits` lo esclude dalle origini congelate e il click rifiuterebbe comunque), il suo scatto entra in un Fuoco in
		// (1,0,0) e lo ferma l'ostacolo in (2,0,0): muore ENTRANDO, nella fase Dash, e arriva al Blast cadavere con il piano ancora addosso. Il
		// click aveva accettato dalla cella d'arrivo dello scatto (2,0,0), il Blast misurerebbe da (1,0,0): senza la guardia la riga di sopra
		// (`ArcoFermato`) pagherebbe, e un cadavere pagherebbe una ricarica che non ha tentato. E' raggiungibile nel gioco: il Fuoco fa 10 danni
		// a chi lo attraversa in scatto (`Terrain.Fire.DamagesOnDash`) e il Blast non salta chi e' caduto prima. La riga `ModifyArc: bersaglio
		// morto e attore gia' caduto` NON la raggiunge: con la salute a zero PRIMA del lock-in il click rifiuterebbe comunque.
		FRic3Scena ArcoMuore = ArcoFermato;
		ArcoMuore.bAttoreMuoreNelloScatto = true;
		FRic3Atteso AttesoArcoMuore = Ric3DopoLaCaduta(Ric3Atteso(false, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange));
		AttesoArcoMuore.bControllaCella = true;
		AttesoArcoMuore.CellaFinale = FRTCellId(1, 0, 0);
		AttesoArcoMuore.bMuoreNelDash = true;
		if (!Ric3Verifica(*this, TEXT("scatto fermato in un Fuoco, ModifyArc, chi agisce muore nel Dash"), TEXT("controparte"), ArcoMuore,
			AttesoArcoMuore,
			TEXT("D-209: un cadavere non paga un'azione che non ha tentato, anche se il click l'aveva accettata dalla cella d'arrivo dello scatto"))) { return false; }

		// Per riflesso il tiro a CELLA oltre la portata dichiarata (`R9`, D-563 (3)): il Blast lo scarta in SILENZIO (nessuna voce, nessun
		// canale), il click l'aveva accettato dalla cella d'arrivo dello scatto. Controllo: lo scatto arriva, l'area vuota in portata
		// parte (`R1`) e paga come sempre.
		FRic3Scena CellaControllo;
		CellaControllo.Azione = ERic3Azione::AttaccoACella;
		CellaControllo.Bersaglio = ERic3Bersaglio::Assente;
		CellaControllo.Visione = 0;
		CellaControllo.bScattoPrecede = true;
		FRic3Atteso AttesoCellaControllo = Ric3Atteso(true, ERic3Click::Accetta);
		AttesoCellaControllo.bControllaCella = true;
		AttesoCellaControllo.CellaFinale = FRTCellId(2, 0, 0);
		if (!Ric3Verifica(*this, TEXT("scatto compiuto, poi tiro a CELLA"), TEXT("controllo"), CellaControllo, AttesoCellaControllo,
			TEXT("lo scatto arriva, l'area vuota in portata parte e paga (R1)"))) { return false; }

		FRic3Scena CellaFermata = CellaControllo;
		CellaFermata.bOstacoloSulloScatto = true;
		FRic3Atteso AttesoCellaFermata = Ric3Atteso(true, ERic3Click::Accetta); // nessuna voce: lo scarto e' silenzioso
		AttesoCellaFermata.bControllaCella = true;
		AttesoCellaFermata.CellaFinale = FRTCellId(1, 0, 0);
		AttesoCellaFermata.bAttoreNonUltimo = true; // l'ostacolo in (2,0,0) ordina dopo chi tira
		if (!Ric3Verifica(*this, TEXT("scatto fermato, poi tiro a CELLA (R9 per riflesso)"), TEXT("caso"), CellaFermata, AttesoCellaFermata,
			TEXT("il click ha accettato dalla cella d'arrivo dello scatto e il tiro, oltre la portata da dove l'unita' si e' fermata, sparisce in silenzio: paga"))) { return false; }

		FRic3Scena CellaFermataEOltre = CellaFermata;
		CellaFermataEOltre.bMiraOltreLaFineDelloScatto = true;
		FRic3Atteso AttesoCellaFermataEOltre = Ric3Atteso(false, ERic3Click::Rifiuta);
		AttesoCellaFermataEOltre.bControllaCella = true;
		AttesoCellaFermataEOltre.CellaFinale = FRTCellId(1, 0, 0);
		AttesoCellaFermataEOltre.bAttoreNonUltimo = true;
		if (!Ric3Verifica(*this, TEXT("scatto fermato, tiro a CELLA oltre la portata anche dalla cella d'arrivo (R9)"), TEXT("controparte"), CellaFermataEOltre,
			AttesoCellaFermataEOltre,
			TEXT("il click rifiutava anche dalla cella d'arrivo: R9 resta gratis (D-563 (3))"))) { return false; }
	}

	// --- D-575 (1bis), la CARICA RIUSCITA seguita da un tiro a un ALTRO bersaglio -------------------------
	{
		// Lettura ESTESA oltre lo scatto fermato, decisa dall'autore il 2026-11-04 («Sì, paga»): paga ogni caso in cui la cella reale di chi
		// agisce differisce dall'origine con cui il click ha giudicato. Qui lo scatto NON si ferma: la carica riesce, e per [D-464] (5) il
		// click giudica comunque la portata del tiro dalla cella di PARTENZA (`AimOriginCell` non sposta l'origine per una carica). La carica
		// urta il nemico in (-2,0,0) e si ferma in (-1,0,0): il bersaglio del tiro, sul lato opposto, sta alla portata esatta da (0,0,0) e una cella
		// oltre da (-1,0,0). Il controllo e' la stessa scena SENZA carica: il tiro colpisce (l'esito dipende dallo spostamento). La controparte
		// gratis e' il bersaglio una cella oltre la portata anche dalla partenza: il click rifiutava.
		FRic3Scena Controllo;
		Controllo.bDistanzaPariAllaPortata = true;
		FRic3Atteso AttesoControllo = Ric3Atteso(true, ERic3Click::Accetta);
		AttesoControllo.bDanno = true;
		AttesoControllo.bControllaCella = true;
		AttesoControllo.CellaFinale = FRTCellId(0, 0, 0);
		if (!Ric3Verifica(*this, TEXT("tiro alla portata esatta, nessuna carica"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("senza carica chi agisce resta dov'e', il tiro colpisce e paga"))) { return false; }

		FRic3Scena Carica = Controllo;
		Carica.bCaricaPrecede = true;
		FRic3Atteso AttesoCarica = Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange);
		AttesoCarica.bControllaCella = true;
		AttesoCarica.CellaFinale = FRTCellId(-1, 0, 0);
		if (!Ric3Verifica(*this, TEXT("carica riuscita, poi tiro a un altro bersaglio"), TEXT("caso"), Carica, AttesoCarica,
			TEXT("D-575 (1bis): il click ha giudicato dalla cella di partenza, il tiro fallisce dalla cella d'arrivo della carica: paga"))) { return false; }

		FRic3Scena CaricaEOltre = Carica;
		CaricaEOltre.bDistanzaPariAllaPortata = false;
		CaricaEOltre.bOltrePortataDichiarata = true;
		FRic3Atteso AttesoCaricaEOltre = Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::OutOfRange);
		AttesoCaricaEOltre.bControllaCella = true;
		AttesoCaricaEOltre.CellaFinale = FRTCellId(-1, 0, 0);
		if (!Ric3Verifica(*this, TEXT("carica riuscita, poi tiro oltre la portata anche dalla partenza"), TEXT("controparte"), CaricaEOltre,
			AttesoCaricaEOltre,
			TEXT("il click rifiutava anche dalla cella di partenza: gratis"))) { return false; }
	}
	return true;
}

/**
 * **Cio' che il click RIFIUTA, o non produce, resta GRATIS** ([D-200], [D-414] (2), [D-571] (1)).
 *
 * Una riga per causa, ognuna col proprio controllo positivo: il controllo e' lo STESSO piano con UNA variabile cambiata
 * che lo rende un piano accettabile, e prova che l'azione parte (colpisce, apre l'arco) e paga.
 *
 * - bersaglio-UNITA' oltre la portata DICHIARATA: il click lo rifiuta (`ClassifyHexTargeting`, `OutOfRange`);
 * - bersaglio-UNITA' o tiro a CELLA dietro un Fumo OSSERVATO (`R7` per il tiro a cella, decisione d'autore del 2026-11-03):
 *   la mappa del click vede il cap, il click rifiuta;
 * - bersaglio-UNITA' o tiro a CELLA dietro un Fumo ESPLORATO ma FUORI VISTA: «osservato» per la mappa del click e'
 *   `ExploredCells` U `VisibleCells`, e una cella esplorata conserva il contenuto ATTUALE (D-534 (3), `Q4`). Il click legge il
 *   Fumo e rifiuta: gratis, anche se la squadra «non l'ha mai visto». Il criterio dell'autore e' il click, non la lettera;
 * - tiro a CELLA con una compagna che ordina DOPO chi tira: l'origine del tiro cappato e' la cella di chi tira
 *   (`Attacker.Cell`), non quella dell'ultima unita' dell'ordine di risoluzione. Le righe di `R7` dichiarano e MISURANO se chi
 *   tira e' l'ultima (`bAttoreNonUltimo`); con la compagna a tre celle dalla mira, un'origine sbagliata farebbe pagare il tiro
 *   anche col Fumo osservato;
 * - bersaglio ALLEATO: per un'unita' comandabile il click non bersaglia ma SELEZIONA (`ResolveOutcome`); ⚠️ per un
 *   alleato non comandabile il click NON controlla la squadra e lo accetta: **[D-575] (b), si ripara il click e intanto e'
 *   gratis** (`bClickDaRiparare`: la riga diventa rossa quando il click rifiutera', e va riscritta);
 * - bersaglio PERSO (`TargetGone`): il click dichiara sempre un'unita' o una cella, quindi non produce questo piano;
 * - bersaglio SCONOSCIUTO (`TargetUnknown`): il click non bersaglia un'unita' che la squadra non vede
 *   (`IsKnownToObserver`), e un'unita' vista in pianificazione lascia comunque un contatto fino al Blast;
 * - `ModifyArc` oltre la portata dichiarata, senza scatti: il click lo rifiuta;
 * - strutture di BORDO in Prep: fuori portata e nessun bersaglio, che il click rifiuta o non produce; e **[D-575] (b), si ripara il
 *   click e intanto e' gratis — nessun lato dichiarato**, che il click accetta (`bClickDaRiparare`);
 * - **[D-575] (c), gratis e non decisa — la reazione armata che non scatta**: non e' un'azione fallita ma lo stato normale dello
 *   slot (`ERTReactionOutcome::NotTriggered`), e pagarla cambierebbe l'economia della reazione ([D-092]). Stessa classe, nel file:
 *   le azioni non d'attacco (`NonAggressivaBloccata`, `NonAggressivaBersaglioMorto` in `StartedActionPaysWithoutAHit`) e il `Dash`
 *   rifiutato per causa (`ResolveDash`, #3973).
 *
 * ⌫ *Fino al 2026-11-03 queste righe erano tre, senza controllo, e dichiaravano «DA DECIDERE» cio' che il click decide. Dal 2026-11-04
 * ([D-574], [D-575]: registrate in #3981, i numeri valgono al merge) il Fumo messo in vista da un compagno e lo scatto fermato sono
 * passati a `ResolutionEmergingCausesPay`: pagano.*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUndecidedCausesStayFreeTest,
	"RefactorTactics.Actions.Blast.UndecidedCausesStayFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUndecidedCausesStayFreeTest::RunTest(const FString&)
{
	FRic3Atteso Colpisce = Ric3Atteso(true, ERic3Click::Accetta);
	Colpisce.bDanno = true;

	// --- bersaglio-unita' oltre la portata dichiarata ---------------------------------------------
	{
		FRic3Scena Controllo; // distanza 3: dentro la portata
		if (!Ric3Verifica(*this, TEXT("bersaglio-unita' dentro la portata"), TEXT("controllo"), Controllo, Colpisce,
			TEXT("colpisce e paga"))) { return false; }
		FRic3Scena Oltre = Controllo;
		Oltre.bOltrePortataDichiarata = true;
		if (!Ric3Verifica(*this, TEXT("bersaglio-unita' oltre la portata dichiarata"), TEXT("caso"), Oltre,
			Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::OutOfRange),
			TEXT("D-200: la portata e' geometria nota in pianificazione, il click la rifiuta"))) { return false; }
	}

	// --- bersaglio-unita' dietro un Fumo OSSERVATO --------------------------------------------------
	{
		// Come la riga del Fumo non osservato (`ResolutionEmergingCausesPay`), con UNA variabile cambiata: il Fumo sta in
		// (1,0,0), nell'intorno di chi tira, quindi la mappa del click lo vede e cappa la portata a 2: il click RIFIUTA.
		// ⚠️ **TDD, non bisectabile in quel punto**: sul commit dei test (`d9b39df54`) questa riga e' ROSSA — il predicato di
		// `d06b54302` pagava ogni fuori portata entro la dichiarata, click che rifiuta o no — ed e' il fix (`e6c01420d`) a
		// renderla verde. Il commit dei test da solo non e' un punto di bisect pulito.
		FRic3Scena Osservato;
		Osservato.Distanza = 5;
		Osservato.PortataSovrascritta = 6;
		Osservato.bTestimone = true;
		Osservato.FumoX = 1;
		if (!Ric3Verifica(*this, TEXT("Fumo OSSERVATO fra chi tira e il bersaglio"), TEXT("caso"), Osservato,
			Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::OutOfRange),
			TEXT("il click vede il Fumo e ne subisce il cap: geometria nota in pianificazione, gratis"))) { return false; }
	}

	// --- tiro a CELLA dietro un Fumo OSSERVATO (R7, il controllo gratis) ----------------------------
	{
		// Decisione d'autore del 2026-11-03: `R7` paga solo col Fumo NON osservato ([D-563] (2)). Con il Fumo in (1,0,0), nell'intorno
		// di chi tira (vista 0), la mappa del click lo vede e cappa la portata a 2: il click a cella (`HandleTargetCell`, la stessa
		// `PlayerAimMap`) RIFIUTA, e il tiro e' gratis. Il caso con il Fumo NON osservato (stessa scena, Fumo in (3,0,0)) paga ed e'
		// in `ResolutionEmergingCausesPay`, insieme al controllo senza Fumo. La voce `Fallback`/`Cancelled` `OutOfRange` resta:
		// cambia il pagamento, non la traccia.
		FRic3Scena Osservato;
		Osservato.Azione = ERic3Azione::AttaccoACella;
		Osservato.Bersaglio = ERic3Bersaglio::Assente;
		Osservato.Distanza = 4;
		Osservato.Visione = 0;
		Osservato.FumoX = 1;
		if (!Ric3Verifica(*this, TEXT("R7: tiro a CELLA, Fumo OSSERVATO"), TEXT("caso"), Osservato,
			Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::OutOfRange),
			TEXT("il click vede il cap del Fumo e rifiuta: geometria nota in pianificazione, gratis"))) { return false; }
	}

	// --- il Fumo ESPLORATO ma FUORI VISTA: il criterio e' il click, non «la squadra lo vedeva» ------------
	{
		// ⚠️ **«Osservato» per la mappa del click vuol dire «in `ExploredCells` U `VisibleCells`», non «in vista adesso».** Una cella
		// esplorata ma ora fuori vista conserva il contenuto ATTUALE (D-534 (3), `Q4`, confermato dall'autore): un Fumo nato dopo
		// l'esplorazione, su una cella che la squadra non vede piu', e' LETTO dal click, che rifiuta. Il tiro e' quindi GRATIS anche
		// se la squadra «non l'ha mai visto» quel Fumo. ⚠️ **La lettera di «non lo vedeva la squadra» e il click DIVERGONO qui, e il
		// criterio dell'autore e' il click** («il click accettava/rifiutava»): il codice e' coerente col click, e la riga lo fissa,
		// per il tiro a cella e per il bersaglio-unita'. La stessa scena dei casi NON osservati (`ResolutionEmergingCausesPay`), con
		// UNA variabile cambiata: la cella del Fumo, in (3,0,0) e fuori vista, e' in `ExploredCells`.
		FRic3Scena Cella;
		Cella.Azione = ERic3Azione::AttaccoACella;
		Cella.Bersaglio = ERic3Bersaglio::Assente;
		Cella.Distanza = 4;
		Cella.Visione = 0;
		Cella.FumoX = 3;
		Cella.bFumoEsploratoFuoriVista = true;
		if (!Ric3Verifica(*this, TEXT("R7: tiro a CELLA, Fumo ESPLORATO ma fuori vista"), TEXT("caso"), Cella,
			Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::OutOfRange),
			TEXT("DIVERGE DALLA LETTERA «non lo vedeva la squadra»: la cella e' esplorata, il click legge il Fumo e rifiuta, gratis"))) { return false; }

		FRic3Scena Unita;
		Unita.Distanza = 5;
		Unita.PortataSovrascritta = 6;
		Unita.bTestimone = true;
		Unita.FumoX = 3;
		Unita.bFumoEsploratoFuoriVista = true;
		if (!Ric3Verifica(*this, TEXT("bersaglio-unita', Fumo ESPLORATO ma fuori vista"), TEXT("caso"), Unita,
			Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::OutOfRange),
			TEXT("DIVERGE DALLA LETTERA «non lo vedeva la squadra»: la cella e' esplorata, il click legge il Fumo e rifiuta, gratis"))) { return false; }
	}

	// --- l'ORIGINE del tiro cappato e' la cella di CHI TIRA, non quella dell'ultima unita' dell'ordine ------------
	{
		// Il predicato misura da `Attacker.Cell` (`Ctx.HexUnits[Intent.AttackerId]`). Nelle altre righe di `R7` chi tira e' spesso
		// l'ULTIMA unita' di `SortUnitsForResolution`, e allora `Attacker.Cell` e `HexUnits.Last().Cell` coincidono: un'origine
		// sbagliata non farebbe cadere niente. Qui una compagna a tre celle dalla mira ordina DOPO chi tira (premessa MISURATA dalla
		// riga: `bAttoreNonUltimo`), e dalla sua cella la linea fino alla mira non incontra il Fumo. Il controllo e' la STESSA scena
		// col Fumo NON osservato: dall'origine vera il click accetta e il tiro paga, con o senza la compagna.
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::AttaccoACella;
		Controllo.Bersaglio = ERic3Bersaglio::Assente;
		Controllo.Distanza = 4;
		Controllo.Visione = 0;
		Controllo.FumoX = 3; // fuori dall'intorno di chi tira e della compagna (a tre celle da (3,0,0))
		Controllo.bCompagnaPressoLaMira = true;
		FRic3Atteso AttesoControllo = Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::OutOfRange);
		AttesoControllo.bAttoreNonUltimo = true;
		if (!Ric3Verifica(*this, TEXT("R7: tiro a CELLA, Fumo NON osservato, compagna che ordina dopo"), TEXT("controllo"), Controllo,
			AttesoControllo,
			TEXT("la compagna non cambia niente: dall'origine vera il click accetta e R7 paga"))) { return false; }

		FRic3Scena Caso = Controllo;
		Caso.FumoX = 1; // nell'intorno di chi tira: OSSERVATO
		FRic3Atteso AttesoCaso = Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::OutOfRange);
		AttesoCaso.bAttoreNonUltimo = true;
		if (!Ric3Verifica(*this, TEXT("R7: tiro a CELLA, Fumo OSSERVATO, compagna che ordina dopo"), TEXT("caso"), Caso,
			AttesoCaso,
			TEXT("dall'origine vera (chi tira) il click vede il cap e rifiuta: gratis. Da quella dell'ultima unita' pagherebbe"))) { return false; }
	}

	// --- bersaglio alleato -------------------------------------------------------------------------
	{
		FRic3Scena Controllo;
		if (!Ric3Verifica(*this, TEXT("bersaglio avversario"), TEXT("controllo"), Controllo, Colpisce,
			TEXT("colpisce e paga"))) { return false; }
		FRic3Scena Alleato = Controllo;
		Alleato.Bersaglio = ERic3Bersaglio::Alleato;
		if (!Ric3Verifica(*this, TEXT("bersaglio alleato comandabile"), TEXT("caso"), Alleato,
			Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::TargetFriendly),
			TEXT("il click non bersaglia un'unita' comandabile della propria squadra: la seleziona"))) { return false; }

		// D-575 (b), SI RIPARA IL CLICK e intanto e' gratis: lo stesso alleato, ma pianificato dal bot (non comandabile da questo
		// giocatore). `ResolveOutcome` lo risponde `Confirm` e `HandleClickOnUnit` non controlla la squadra: il click ACCETTA un piano
		// che non dovrebbe accettare, il resolver lo scarta. Quando il click rifiutera' la causa sara' gratis per [D-571], senza altro
		// codice nel pagamento: la premessa dell'oracolo cade rossa, e la riga si riscrive come «il click rifiuta».
		FRic3Scena NonComandabile = Alleato;
		NonComandabile.bAlleatoNonComandabile = true;
		FRic3Atteso AttesoNonComandabile = Ric3Atteso(false, ERic3Click::Accetta, ERTActionInvalidReason::TargetFriendly);
		AttesoNonComandabile.bClickDaRiparare = true;
		if (!Ric3Verifica(*this, TEXT("bersaglio alleato NON comandabile"), TEXT("caso"), NonComandabile, AttesoNonComandabile,
			TEXT("D-575 (b) — il click non controlla la squadra e accetta un alleato che non dovrebbe: si ripara il click, intanto gratis"))) { return false; }
	}

	// --- bersaglio perso ---------------------------------------------------------------------------
	{
		FRic3Scena Controllo;
		if (!Ric3Verifica(*this, TEXT("bersaglio dichiarato"), TEXT("controllo"), Controllo, Colpisce,
			TEXT("colpisce e paga"))) { return false; }
		FRic3Scena Perso = Controllo;
		Perso.Bersaglio = ERic3Bersaglio::Assente;
		if (!Ric3Verifica(*this, TEXT("bersaglio perso"), TEXT("caso"), Perso,
			Ric3Atteso(false, ERic3Click::ND, ERTActionInvalidReason::TargetGone),
			TEXT("il click dichiara sempre un'unita' o una cella: questo piano non nasce da un click"))) { return false; }
	}

	// --- bersaglio sconosciuto ---------------------------------------------------------------------
	{
		FRic3Scena Controllo; // visione di default: il bersaglio a distanza 3 si vede
		if (!Ric3Verifica(*this, TEXT("bersaglio visto dalla squadra"), TEXT("controllo"), Controllo, Colpisce,
			TEXT("colpisce e paga"))) { return false; }
		FRic3Scena Ignoto = Controllo;
		Ignoto.Visione = 1; // la squadra non vede piu' a distanza 3, e non ha mai avuto un contatto
		// ⚠️ **LIMITE DICHIARATO, non copertura finta**: il click rifiuta perche' la riga scrive A MANO
		// `SetKnownToObserver(false)`, il flag che il velo dell'HUD produrrebbe — non perche' la squadra non veda il bersaglio.
		// La riga prova il CANCELLO (`IsKnownToObserver`, `HandleClickOnUnit`) e che il resolver lo scarta come `TargetUnknown`;
		// NON prova l'ingresso del cancello (che il velo produca davvero quel flag), che sta in `Veil.*`.
		Ignoto.bBersaglioIgnotoAlVelo = true; // e il velo dell'HUD non lo conosce: il click non lo bersaglia
		if (!Ric3Verifica(*this, TEXT("bersaglio mai visto dalla squadra"), TEXT("caso"), Ignoto,
			Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::TargetUnknown),
			TEXT("il click non bersaglia cio' che non si vede (IsKnownToObserver)"))) { return false; }
	}

	// --- ModifyArc oltre la portata dichiarata -----------------------------------------------------
	{
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::ModifyArc;
		Controllo.Distanza = 2;
		FRic3Atteso AttesoControllo = Ric3Atteso(true, ERic3Click::Accetta);
		AttesoControllo.bArco = true;
		if (!Ric3Verifica(*this, TEXT("ModifyArc dentro la portata"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("l'arco si apre e paga"))) { return false; }
		FRic3Scena Oltre = Controllo;
		Oltre.bOltrePortataDichiarata = true;
		if (!Ric3Verifica(*this, TEXT("ModifyArc oltre la portata dichiarata"), TEXT("caso"), Oltre,
			Ric3Atteso(false, ERic3Click::Rifiuta, ERTActionInvalidReason::OutOfRange),
			TEXT("D-200: la portata e' geometria nota in pianificazione, il click la rifiuta"))) { return false; }
	}

	// --- strutture di bordo in Prep ----------------------------------------------------------------
	{
		// ⚠️ **LIMITE DICHIARATO**: l'oracolo di queste righe e' il click su UN'UNITA' (`DispatchUnitClick`), non
		// `HandleTargetEdge`. Il verdetto di PORTATA coincide (stessa `ClassifyHexTargeting`, stessa origine in Prep), e
		// la riga «nessun lato» e' proprio la prova che il click sull'unita' e' un percorso di produzione per un'abilita' di
		// bordo; ma il percorso di produzione dei BORDI (`HandleTargetEdge`, che scrive il lato) non e' esercitato: il lato
		// dichiarato del controllo e' scritto a mano, come lo scrive quella funzione.
		// Controllo: il lato dichiarato (come lo scrive `HandleTargetEdge`) -> la copertura sorge e l'azione paga.
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::CreateCover;
		Controllo.Distanza = 2;
		Controllo.bBordoDichiarato = true;
		FRic3Atteso AttesoControllo = Ric3Atteso(true, ERic3Click::Accetta);
		AttesoControllo.CopertureErette = 1;
		if (!Ric3Verifica(*this, TEXT("CreateCover col lato dichiarato"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("la copertura sorge e l'azione paga (il verdetto del click e' quello del click sull'unita': stessa portata)"))) { return false; }

		// 🔴 D-575 (b), SI RIPARA IL CLICK e intanto e' gratis — «nessun bordo dichiarato»: `GetPointerContext` risponde `Targeting` per
		// ogni abilita' armata che non sia mobilita' rapida, `ResolveOutcome` risponde `Confirm` per un nemico osservato e
		// `HandleClickOnUnit` NON ha la guardia sul kind del bersaglio che hanno `HandleTargetCell` e `HandleTargetEdge`. Un'abilita' di
		// BORDO armata accetta quindi il click su un'unita': il piano nasce senza lato, e `ResolveCoverStructures` lo rifiuta prima del
		// pagamento. Il click ACCETTA un piano che non dovrebbe accettare e l'azione e' gratis; l'autore ha deciso che si ripara il click.
		FRic3Scena SenzaLato = Controllo;
		SenzaLato.bBordoDichiarato = false;
		FRic3Atteso AttesoSenzaLato = Ric3Atteso(false, ERic3Click::Accetta);
		AttesoSenzaLato.CopertureRifiutate = 1;
		AttesoSenzaLato.bClickDaRiparare = true;
		AttesoSenzaLato.bControllaBordo = true;
		AttesoSenzaLato.bBordo = false;
		if (!Ric3Verifica(*this, TEXT("CreateCover armata, click su un'unita' (nessun lato)"), TEXT("caso"), SenzaLato,
			AttesoSenzaLato,
			TEXT("D-575 (b) — il click accetta un piano di bordo senza lato, il resolver lo rifiuta gratis: si ripara il click"))) { return false; }

		// «fuori portata»: il click rifiuta (stessa `ClassifyHexTargeting` di `HandleTargetEdge`), e in Prep l'origine e'
		// sempre la cella di partenza. Il lato e' dichiarato, cosi' il resolver arriva alla guardia di portata (che nel ciclo
		// viene DOPO quella del lato). Verificato: gratis.
		FRic3Scena FuoriPortata = Controllo;
		FuoriPortata.bOltrePortataDichiarata = true;
		FRic3Atteso AttesoFuori = Ric3Atteso(false, ERic3Click::Rifiuta);
		AttesoFuori.CopertureRifiutate = 1;
		if (!Ric3Verifica(*this, TEXT("CreateCover armata, bersaglio oltre la portata"), TEXT("caso"), FuoriPortata, AttesoFuori,
			TEXT("il click rifiuta il fuori portata: geometria nota in pianificazione"))) { return false; }

		// «nessun bersaglio»: nessuna scrittura del click lascia `PlannedAbilityIndex` senza una dichiarazione di bersaglio
		// per un'abilita' che non sia su se stessi. Il resolver lo rifiuta gratis.
		FRic3Scena SenzaBersaglio = SenzaLato;
		SenzaBersaglio.Bersaglio = ERic3Bersaglio::Assente;
		FRic3Atteso AttesoSenzaBersaglio = Ric3Atteso(false, ERic3Click::ND);
		AttesoSenzaBersaglio.CopertureRifiutate = 1;
		if (!Ric3Verifica(*this, TEXT("CreateCover armata, nessun bersaglio"), TEXT("caso"), SenzaBersaglio, AttesoSenzaBersaglio,
			TEXT("il click non produce un piano di bordo senza bersaglio"))) { return false; }
	}

	// --- Interact armato: INT-* paga SOLO se il piano ha un lato dichiarato ([D-575] (a) e (b)) -------------------
	{
		// `DoorlessIntents` raccoglie ANCHE gli intenti senza lato dichiarato (il ramo di CP 9.3, dove l'operazione nasce dalla traiettoria), e
		// la commutazione rifiutata (`DoorToggleRefusals`) puo' nascere da una porta trovata sulla traiettoria invece che sul lato. `INT-*` paga
		// perche' il click dei bordi (`HandleTargetEdge`, oggi raggiunto solo da test e harness) accetta quel piano E scrive sempre il lato: un
		// piano SENZA lato non viene da li'. Viene dal click su un'UNITA' (`HandleClickOnUnit` non controlla il kind del bersaglio), che accetta
		// un'`Interact` armata: non e' il piano che `INT-*` paga. ⚠️ Che sia il difetto del click di [D-575] (b) e #3983 e' confermato
		// dall'autore il 2026-11-04 (estensione all'`Interact`): la riga la pinna gratis, e il rifiuto del click la renderebbe rossa da sola. Le righe sono misurate su questa fixture: la porta sta sul bordo W di una cella non nota alla squadra.
		//
		// Il controllo e' lo STESSO piano con il lato dichiarato come lo scrive `HandleTargetEdge`: nessuna porta sul bordo, paga (`NoEffect`).
		// ⚠️ L'oracolo di queste righe e' il click su un'UNITA' adiacente, non `HandleTargetEdge`: l'equivalenza «il click accetta <=> paga»
		// del controllo e' asserita su un click che ha scritto un altro piano, poi riscritto a mano (cella e lato). Il controllo sta qui solo come
		// controparte delle righe senza lato. La commutazione rifiutata CON il lato dichiarato (`Locked`, `Destroyed`) la misura, col click dei
		// bordi, `Actions.Blast.DoorInteractionRefusalsPay`.
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::Interact;
		Controllo.Distanza = 1;
		Controllo.bBordoDichiarato = true;
		if (!Ric3Verifica(*this, TEXT("Interact armata, lato dichiarato, nessuna porta"), TEXT("controllo"), Controllo,
			Ric3Atteso(true, ERic3Click::Accetta, ERTActionInvalidReason::NoEffect),
			TEXT("INT-*: la porta senza varco sul lato dichiarato, che il click dei bordi accetta, paga"))) { return false; }

		FRic3Scena SenzaLato = Controllo;
		SenzaLato.bBordoDichiarato = false;
		FRic3Atteso AttesoSenzaLato = Ric3Atteso(false, ERic3Click::Accetta, ERTActionInvalidReason::NoEffect);
		AttesoSenzaLato.bClickDaRiparare = true;
		AttesoSenzaLato.bControllaBordo = true;
		AttesoSenzaLato.bBordo = false;
		if (!Ric3Verifica(*this, TEXT("Interact armata, click su un'unita' (nessun lato), nessuna porta"), TEXT("caso"), SenzaLato, AttesoSenzaLato,
			TEXT("D-575 (b) — il click accetta un piano di bordo senza lato, il resolver non trova nessuna porta: non e' `INT-*`, si ripara il click, intanto gratis"))) { return false; }

		// Il piano a CELLA senza lato: stesso bersaglio a cella del controllo e stessa assenza di porta, e la SOLA differenza e' che il
		// lato non e' dichiarato. Isola la variabile (`bHasDeclaredDoorEdge`): la riga qui sopra differisce dal controllo anche nella forma
		// del bersaglio (unita' contro cella), e un filtro sulla forma invece che sul lato non la distinguerebbe.
		FRic3Scena CellaSenzaLato = Controllo;
		CellaSenzaLato.bBordoDichiarato = false;
		CellaSenzaLato.bPianoACellaSenzaLato = true;
		FRic3Atteso AttesoCellaSenzaLato = Ric3Atteso(false, ERic3Click::Accetta, ERTActionInvalidReason::NoEffect);
		AttesoCellaSenzaLato.bClickDaRiparare = true;
		AttesoCellaSenzaLato.bControllaBordo = true;
		AttesoCellaSenzaLato.bBordo = false;
		if (!Ric3Verifica(*this, TEXT("Interact armata, piano a cella senza lato, nessuna porta"), TEXT("caso"), CellaSenzaLato, AttesoCellaSenzaLato,
			TEXT("D-575 (b) — stesso piano a cella del controllo, senza lato: `INT-*` guarda il lato dichiarato e non la forma del bersaglio, gratis"))) { return false; }

		// La commutazione rifiutata SENZA lato: una porta `Locked` sul bordo fra chi agisce e l'unita' adiacente. MISURATO: il click su un'unita'
		// la ACCETTA lo stesso (su questa fixture la porta non toglie la vista: sta su una cella non nota alla squadra), e il piano non ha il lato: `FirstDoorEdge` trova la porta sulla
		// traiettoria e il resolver rifiuta la commutazione (`DoorToggleRefusals`, `DoorLocked`). E' lo stesso difetto del click della riga
		// sopra, non `INT-*`: gratis, e si ripara il click. (Il controllo col lato dichiarato, che paga, e' `Actions.Blast.DoorInteractionRefusalsPay`.)
		FRic3Scena Bloccata = SenzaLato;
		Bloccata.bPortaBloccataTraLeCelle = true;
		FRic3Atteso AttesoBloccata = Ric3Atteso(false, ERic3Click::Accetta, ERTActionInvalidReason::DoorLocked);
		AttesoBloccata.bClickDaRiparare = true;
		AttesoBloccata.bControllaBordo = true;
		AttesoBloccata.bBordo = false;
		if (!Ric3Verifica(*this, TEXT("Interact armata, click su un'unita' (nessun lato), porta bloccata sulla traiettoria"), TEXT("caso"), Bloccata,
			AttesoBloccata,
			TEXT("D-575 (b) — senza lato il piano non viene dal click dei bordi: la commutazione rifiutata non e' `INT-*`, si ripara il click, intanto gratis"))) { return false; }
	}

	// --- D-575 (c), GRATIS E NON DECISA: reazione armata che non scatta -------------------------------
	{
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::ReazioneCounter;
		Controllo.bNemicoAttacca = true;
		FRic3Atteso AttesoControllo = Ric3Atteso(true, ERic3Click::ND);
		AttesoControllo.Attivate = 1;
		if (!Ric3Verifica(*this, TEXT("reazione armata e colpita"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("la reazione scatta e paga, come sempre"))) { return false; }
		FRic3Scena Muta = Controllo;
		Muta.bNemicoAttacca = false;
		FRic3Atteso AttesoMuta = Ric3Atteso(false, ERic3Click::ND);
		AttesoMuta.NonInnescate = 1;
		if (!Ric3Verifica(*this, TEXT("reazione armata, nessun trigger"), TEXT("caso"), Muta, AttesoMuta,
			TEXT("D-575 (c) — gratis e non decisa: non e' un'azione fallita ma lo stato normale dello slot, e pagarla cambia l'economia delle reazioni (D-092)"))) { return false; }
	}
	return true;
}

// =====================================================================================================
// `R7` per squadra: due tiri cappati dalla STESSA cella di Fumo, una squadra lo vede e l'altra no.
// =====================================================================================================

namespace
{
	struct FPagaDueSquadreEsito
	{
		bool bValido = false;
		int32 CooldownChiVede = -1;     //< il tiratore la cui squadra vede il Fumo
		int32 CooldownChiNonVede = -1;  //< il tiratore la cui squadra non lo vede
		int32 VociFuoriPortata = 0;     //< voci Fallback/Cancelled `OutOfRange` del turno, di entrambi
		uint32 HashDiStato = 0;         //< `HashMatchState` a fine turno
	};

	/**
	 * UN Blast con DUE tiri a cella cappati dalla stessa cella di Fumo, lungo l'asse X. Il tiratore «che vede» (squadra 0) sta in
	 * 0 e ha il Fumo in 1, nel suo intorno di 2 celle (vista 0); quello «che non vede» (squadra 1) sta in 5, a quattro celle dal
	 * Fumo, fuori dall'intorno. Ognuno mira alla portata ESATTA dell'attacco base (4) su una cella VUOTA, con il Fumo sulla linea:
	 * chi vede mira in 4, chi non vede mira nella cella stessa del Fumo, in 1. Nessun bersaglio-unita': un'unita' sulla cella mirata
	 * trasformerebbe il tiro in un attacco a un'unita' (`LineResolution`), e non sarebbe piu' un tiro a cella cappato.
	 *
	 * `bLatiScambiati` specchia la geometria lungo l'asse X (x -> 5 - x): chi vede passa dall'ultimo al primo nell'ordine di
	 * risoluzione (la cella e' la prima chiave di `SortUnitsForResolution`), cioe' la cache a slot singolo di `PlanningMapFor` si
	 * ricostruisce in un ordine di squadre diverso. `bSpawnInvertito` cambia solo l'ordine di creazione degli Actor: l'identita'
	 * stabile non ne dipende (`EnsureMatchRoster`), quindi lo stato finale dev'essere identico.
	 *
	 * `bCompagnoDiChiNonVedeScatta` aggiunge UN compagno a chi non vede, a tre celle dal Fumo (fuori dal suo intorno di 2): in
	 * pianificazione la squadra continua a non vedere il Fumo, ma con uno scatto di due celle il compagno gli arriva a una cella e
	 * lo mette in vista NEL BLAST ([D-574]). La conoscenza del Blast contiene allora il Fumo anche per la squadra 1, mentre quella
	 * di pianificazione dice che non lo vedeva: leggere la conoscenza sbagliata, o quella della squadra sbagliata, fa pagare chi
	 * non deve o non fa pagare chi deve.
	 */
	FPagaDueSquadreEsito PagaGiraDueSquadre(FAutomationTestBase& Test, const TCHAR* Nome, bool bLatiScambiati, bool bSpawnInvertito,
		bool bCompagnoDiChiNonVedeScatta = false)
	{
		FPagaDueSquadreEsito Esito;
		UWorld* World = PagaMakeWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), Nome), World)) { return Esito; }

		// Specchio lungo X: (x,0,0) -> (5 - x,0,0) porta chi vede da (0,0,0) a (5,0,0), chi non vede da (5,0,0) a (0,0,0), il Fumo da 1 a 4.
		const auto Cella = [bLatiScambiati](int32 X) { return FRTCellId(bLatiScambiati ? 5 - X : X, 0, 0); };
		const FRTCellId CellaChiVede = Cella(0);
		const FRTCellId CellaChiNonVede = Cella(5);
		const FRTCellId CellaDelFumo = Cella(1);
		const FRTCellId MiraChiVede = Cella(4);        // a distanza 4, con il Fumo sulla linea
		const FRTCellId MiraChiNonVede = CellaDelFumo; // a distanza 4, e il Fumo e' proprio l'ultima cella della linea

		ARTHexMapActor* MapActor = PagaSpawnMap(World, /*Radius=*/ 9, {}, { CellaDelFumo }, {});
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

		// L'ordine di creazione e' l'unica cosa che `bSpawnInvertito` cambia.
		ARTUnit* ChiVede = nullptr;
		ARTUnit* ChiNonVede = nullptr;
		const auto CreaChiVede = [&]() { ChiVede = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), CellaChiVede); };
		const auto CreaChiNonVede = [&]() { ChiNonVede = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), CellaChiNonVede); };
		if (bSpawnInvertito) { CreaChiNonVede(); CreaChiVede(); } else { CreaChiVede(); CreaChiNonVede(); }
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: TM"), Nome), TM)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: chi vede"), Nome), ChiVede)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: chi non vede"), Nome), ChiNonVede))
		{
			PagaDestroyWorld(World);
			return Esito;
		}

		for (ARTUnit* Tiratore : { ChiVede, ChiNonVede })
		{
			Tiratore->VisionRange = 0; // solo l'intorno di 2 celle
			Tiratore->Abilities[0]->CooldownTurns = PagaCooldown;
			Tiratore->PlannedAbilityIndex = 0;
		}
		if (bCompagnoDiChiNonVedeScatta)
		{
			// A tre celle dal Fumo (la distanza di (x,3) da (x,0) e' 3): fuori dall'intorno di 2, quindi in pianificazione non lo vede.
			// Lo scatto di due celle lungo l'asse r lo porta a una cella dal Fumo: nel Blast (`RefreshTeamKnowledgeForBlast` osserva le
			// posizioni post-Dash) lo vede. Compagno della squadra 1, la stessa di chi non vede: la squadra 0 non cambia.
			ARTUnit* Compagno = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(CellaDelFumo.X, 3, 0));
			if (!Test.TestNotNull(*FString::Printf(TEXT("%s: compagno di chi non vede"), Nome), Compagno))
			{
				PagaDestroyWorld(World);
				return Esito;
			}
			Compagno->VisionRange = 0;
			const int32 ScattoCompagno = RTAbilityFixtures::AddCoreAbility(Compagno, TEXT("Action.Reposition"));
			Compagno->PlannedDashAbility = ScattoCompagno;
			Compagno->PlannedDashCell = FRTCellId(CellaDelFumo.X, 1, 0);
		}
		if (!Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la mira sta alla portata dichiarata"), Nome),
			ChiVede->Abilities[0]->RangeCells, 4))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		ChiVede->DeclareAttackOnCell(MiraChiVede);
		ChiNonVede->DeclareAttackOnCell(MiraChiNonVede);

		// Le premesse sulla conoscenza: la squadra 0 ha il Fumo in vista, la squadra 1 non lo conosce affatto.
		TM->RefreshTeamKnowledgeNow();
		const FRTTeamKnowledge ConoscenzaChiVede = TM->KnowledgeForTeamPublic(0);
		const FRTTeamKnowledge ConoscenzaChiNonVede = TM->KnowledgeForTeamPublic(1);
		Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la squadra 0 vede il Fumo"), Nome),
			ConoscenzaChiVede.VisibleCells.Contains(CellaDelFumo));
		Test.TestFalse(*FString::Printf(TEXT("%s: premessa: la squadra 1 non lo vede"), Nome),
			ConoscenzaChiNonVede.VisibleCells.Contains(CellaDelFumo));
		Test.TestFalse(*FString::Printf(TEXT("%s: premessa: la squadra 1 non lo ha nemmeno esplorato"), Nome),
			ConoscenzaChiNonVede.ExploredCells.Contains(CellaDelFumo));

		PagaRunTurn(TM);

		Esito.CooldownChiVede = ChiVede->GetAbilityCooldown(0);
		Esito.CooldownChiNonVede = ChiNonVede->GetAbilityCooldown(0);
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange))
			{
				++Esito.VociFuoriPortata;
			}
		}
		TArray<ARTUnit*> Unita = { ChiVede, ChiNonVede };
		Esito.HashDiStato = URTMatchStateHashLibrary::HashMatchState(MapActor->MapAsset,
			URTMatchStateHashLibrary::BuildUnitDigests(Unita), TArray<int32>{ 0, 0 });

		Esito.bValido = true;
		PagaDestroyWorld(World);
		return Esito;
	}
}

/**
 * **`R7` e' per SQUADRA, e non dipende dall'ordine in cui le unita' vengono risolte** (decisione d'autore del 2026-11-03).
 *
 * Due tiratori di squadre diverse, due tiri a cella cappati dalla STESSA cella di Fumo: la squadra che la vede
 * (`VisibleCells`) ha un click che rifiutava, quindi il suo tiro e' GRATIS; quella che non la conosce ha un click che accettava,
 * quindi il suo tiro PAGA. La cache a slot singolo di `PlanningMapFor` si ricostruisce a ogni cambio di squadra: il risultato
 * non deve dipendere ne' da QUALE squadra e' risolta per prima ne' dall'ordine di creazione degli Actor.
 *
 * Quattro esecuzioni: la geometria base e la sua specchiata (chi vede passa dall'ultimo al primo nell'ordine di risoluzione), ciascuna
 * con l'ordine di creazione degli Actor diritto e invertito. Il pagamento dei due ruoli e' lo stesso in tutte e quattro, e per le
 * coppie che differiscono solo per l'ordine di creazione lo stato finale (`HashMatchState`, che include le ricariche) e' identico.
 *
 * ⚠️ **Non c'e' l'oracolo del click**: la riga misura l'indipendenza dall'ordine, non l'equivalenza col click, che e' gia' pinnata
 * dalle righe `R7` di `ResolutionEmergingCausesPay` e `UndecidedCausesStayFree`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainCappedPaymentIsPerTeamTest,
	"RefactorTactics.Actions.Blast.TerrainCappedPaymentIsPerTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainCappedPaymentIsPerTeamTest::RunTest(const FString&)
{
	const FPagaDueSquadreEsito Base = PagaGiraDueSquadre(*this, TEXT("base"), /*bLatiScambiati=*/ false, /*bSpawnInvertito=*/ false);
	const FPagaDueSquadreEsito BaseInvertita = PagaGiraDueSquadre(*this, TEXT("base, Actor creati al contrario"), false, true);
	const FPagaDueSquadreEsito Specchio = PagaGiraDueSquadre(*this, TEXT("specchiata"), true, false);
	const FPagaDueSquadreEsito SpecchioInvertita = PagaGiraDueSquadre(*this, TEXT("specchiata, Actor creati al contrario"), true, true);
	// [D-574]: il compagno di chi non vede gli mette il Fumo in vista con uno scatto, NEL BLAST. Per la pianificazione la squadra
	// continua a non vederlo, e il pagamento non cambia: e' il discriminante fra la conoscenza della pianificazione e quella del Blast.
	const FPagaDueSquadreEsito ConScatto = PagaGiraDueSquadre(*this, TEXT("base, il compagno di chi non vede scatta"), false, false, true);
	const FPagaDueSquadreEsito ConScattoSpecchiata = PagaGiraDueSquadre(*this, TEXT("specchiata, il compagno di chi non vede scatta"), true, false, true);
	if (!Base.bValido || !BaseInvertita.bValido || !Specchio.bValido || !SpecchioInvertita.bValido
		|| !ConScatto.bValido || !ConScattoSpecchiata.bValido) { return false; }

	const struct { const TCHAR* Nome; const FPagaDueSquadreEsito* Esito; } Righe[] = {
		{ TEXT("base"), &Base },
		{ TEXT("base, Actor creati al contrario"), &BaseInvertita },
		{ TEXT("specchiata"), &Specchio },
		{ TEXT("specchiata, Actor creati al contrario"), &SpecchioInvertita },
		{ TEXT("base, il compagno di chi non vede scatta"), &ConScatto },
		{ TEXT("specchiata, il compagno di chi non vede scatta"), &ConScattoSpecchiata },
	};
	for (const auto& Riga : Righe)
	{
		// Le voci di TurnLog escono per OGNI tiro cappato, osservato o no: sono la prova che entrambi i tiri erano cappati.
		TestEqual(*FString::Printf(TEXT("%s: premessa: entrambi i tiri sono cappati (una voce OutOfRange ciascuno)"), Riga.Nome),
			Riga.Esito->VociFuoriPortata, 2);
		TestEqual(*FString::Printf(TEXT("%s: la squadra che VEDE il Fumo non paga (il click rifiutava)"), Riga.Nome),
			Riga.Esito->CooldownChiVede, 0);
		TestEqual(*FString::Printf(TEXT("%s: la squadra che NON lo vede paga (il click accettava), una volta sola"), Riga.Nome),
			Riga.Esito->CooldownChiNonVede, PagaCooldown - 1);
	}
	TestEqual(TEXT("stato finale identico se cambia solo l'ordine di creazione degli Actor (geometria base)"),
		BaseInvertita.HashDiStato, Base.HashDiStato);
	TestEqual(TEXT("stato finale identico se cambia solo l'ordine di creazione degli Actor (geometria specchiata)"),
		SpecchioInvertita.HashDiStato, Specchio.HashDiStato);
	return true;
}

// =====================================================================================================
// `Status.Stunned` nega l'azione PRINCIPALE, e la decisione del 2026-10-31 dice che chi la tenta PAGA.
// =====================================================================================================

namespace
{
	enum class EPagaSito : uint8 { Prep, Blast, Cleanup, Cura, Purificazione };

	struct FPagaStorditoCase
	{
		const TCHAR* Nome;
		EPagaSito Sito;
		bool bStordito;
		bool bGiaInRicarica; //< l'abilita' era GIA' in ricarica: `ConsumeAbility` la riporterebbe al valore pieno
		bool bAttoreCaduto;
		int32 CooldownAtteso; //< residuo a fine turno (il Cleanup decrementa di uno)
		const TCHAR* Perche;
		bool bAlleatoFuoriPortata = false; //< solo per la cura: il piano mira a un alleato oltre la portata dichiarata
	};

	struct FPagaStorditoEsito
	{
		bool bValido = false;
		int32 Cooldown = -1;
		int32 Rifiuti = 0;                   //< voci Fallback/Cancelled `Stunned` di chi agisce
		ERTMatchPhase FaseDelRifiuto = ERTMatchPhase::Planning;
		int32 PremessaCooldown = -1;         //< il residuo PRIMA del turno, per le righe «gia' in ricarica»
		bool bEffetto = false;               //< l'effetto PROPRIO dell'azione e' avvenuto (solo cura e purificazione)
		int32 Attivazioni = 0;               //< eventi `AbilityActivated` dell'attore: un rifiuto non si attiva
		int32 FuoriPortata = 0;              //< voci Fallback/Cancelled `OutOfRange` di chi agisce
		int32 RifiutiDellAzione = 0;         //< i rifiuti `Stunned` che nominano l'azione installata e la cella di chi agisce
	};

	FPagaStorditoEsito PagaGiraStordito(FAutomationTestBase& Test, const FPagaStorditoCase& C)
	{
		FPagaStorditoEsito Esito;
		UWorld* World = PagaMakeWorld();
		if (!Test.TestNotNull(TEXT("world di prova"), World)) { return Esito; }

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6);

		ARTUnit* Attore = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
		ARTUnit* Nemico = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: attore"), C.Nome), Attore)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: nemico"), C.Nome), Nemico)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: TM"), C.Nome), TM))
		{
			PagaDestroyWorld(World);
			return Esito;
		}

		// L'azione va in uno slot che l'array dei cooldown COPRE, o il contatore non e' osservabile affatto.
		int32 Idx = INDEX_NONE;
		switch (C.Sito)
		{
		case EPagaSito::Prep:
			Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.Overwatch"), 0);
			break;
		case EPagaSito::Blast:
			Idx = 0; // l'attacco base dell'eroe
			break;
		case EPagaSito::Cleanup:
			Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.Ignite"), 3);
			break;
		// ⚠️ Cura e purificazione nello slot 3 e NON nello 0: l'attore e' `Ctx.Units[0]` (la sua cella precede quella del
		// nemico), quindi con lo slot 0 indice dell'unita' e indice dell'abilita' coinciderebbero, e un pagamento scritto
		// con l'indice sbagliato (`MarkAbilitySpent(Unit, i)` al posto di `HealIdx`) lascerebbe verde ogni riga.
		case EPagaSito::Cura:
			Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.Heal"), 3);
			break;
		case EPagaSito::Purificazione:
			Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.Cleanse"), 3);
			break;
		}
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: l'abilita' e' installata"), C.Nome), Idx != INDEX_NONE))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		Attore->Abilities[Idx]->CooldownTurns = PagaCooldown;
		Attore->PlannedAbilityIndex = Idx;
		// Cura e purificazione agiscono su CHI LE USA: un nemico dichiarato come bersaglio le renderebbe un'altra azione.
		if (C.Sito == EPagaSito::Blast || C.Sito == EPagaSito::Cleanup) { Attore->DeclareAttackOnUnit(Nemico); }

		// L'effetto proprio dell'azione dev'essere OSSERVABILE, o il rifiuto e il controllo positivo non si distinguono:
		// la cura ha qualcosa da curare, la purificazione uno stato da togliere (`Root` dura 2, quindi a fine turno c'e'
		// ancora se nessuno lo ha tolto).
		int32 SaluteInPartenza = 0;
		if (C.Sito == EPagaSito::Cura)
		{
			Attore->Health = FMath::Max(1, Attore->Health - 30);
			SaluteInPartenza = Attore->Health;
			Test.TestTrue(*FString::Printf(TEXT("%s: premessa: chi cura e' ferito"), C.Nome),
				Attore->Health < Attore->MaxHealth);
		}
		if (C.Sito == EPagaSito::Purificazione)
		{
			Attore->ApplyStatus(TAG_Status_Root, 2);
			Attore->PlannedCleansePriority = { TAG_Status_Root };
			Test.TestTrue(*FString::Printf(TEXT("%s: premessa: lo stato da togliere c'e'"), C.Nome),
				Attore->HasStatus(TAG_Status_Root));
		}
		if (C.bAlleatoFuoriPortata)
		{
			// Stessa distanza della riga «cura fuori portata» di `Actions.Blast.PlannedActionPaysOnlyIfItStarted`.
			ARTUnit* Alleato = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(5, 0, 0));
			if (!Test.TestNotNull(*FString::Printf(TEXT("%s: alleato"), C.Nome), Alleato))
			{
				PagaDestroyWorld(World);
				return Esito;
			}
			Attore->PlannedAttackTarget = Alleato;
		}
		const FName AzioneAttesa = Attore->Abilities[Idx]->Def.ActionId;
		const FRTCellId CellaDiChiAgisce = Attore->Cell;

		if (C.bStordito) { Attore->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns); }
		if (C.bGiaInRicarica)
		{
			Attore->ConsumeAbility(Idx);
			Attore->TickCooldowns(); // PagaCooldown - 1: un secondo `ConsumeAbility` la riporterebbe a PagaCooldown
		}
		Esito.PremessaCooldown = Attore->GetAbilityCooldown(Idx);
		if (C.bAttoreCaduto) { Attore->Health = 0; }

		const TWeakObjectPtr<ARTUnit> AttoreDebole(Attore);
		PagaRunTurn(TM);

		ARTUnit* AttoreDopo = AttoreDebole.Get(/*bEvenIfPendingKill*/ true);
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: l'attore e' ancora leggibile"), C.Nome), AttoreDopo))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		Esito.Cooldown = AttoreDopo->GetAbilityCooldown(Idx);
		if (C.Sito == EPagaSito::Cura) { Esito.bEffetto = AttoreDopo->Health > SaluteInPartenza; }
		if (C.Sito == EPagaSito::Purificazione) { Esito.bEffetto = !AttoreDopo->HasStatus(TAG_Status_Root); }
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == AttoreDopo->StableUnitId)
			{
				++Esito.Attivazioni;
			}
		}
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned)
				&& E.UnitId == AttoreDopo->StableUnitId)
			{
				++Esito.Rifiuti;
				Esito.FaseDelRifiuto = E.Phase;
				if (E.ActionId == AzioneAttesa && E.SrcCell == CellaDiChiAgisce) { ++Esito.RifiutiDellAzione; }
			}
			if (E.Category == ERTLogCategory::Fallback && E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange)
				&& E.UnitId == AttoreDopo->StableUnitId)
			{
				++Esito.FuoriPortata;
			}
		}
		Esito.bValido = true;
		PagaDestroyWorld(World);
		return Esito;
	}
}

/**
 * **Lo stordito PAGA l'azione principale che gli viene negata, in OGNI sito che la nega — e una volta sola**
 * ([D-416], [D-414], decisione d'autore del 2026-10-31: «Lo stordito (D-416) paga?» «si»). Per la cura e la
 * purificazione la NEGAZIONE e' d'autore (2026-11-01, [D-564]); il PAGAMENTO e' l'applicazione di quella risposta del
 * 2026-10-31, fatta dall'assistente e confermata dall'autore il 2026-11-01 (risposta cumulativa, lettura «si»
 * dell'assistente).
 *
 * I siti che negano una principale hanno ognuno la propria guardia: `ResolvePrep` (Overwatch e predittive),
 * `CollectAttackIntents` (Blast), `ResolveEnvironment` (Cleanup) e, dal 2026-11-01, `ResolveCleanseActions` e
 * `CollectHealActions` (Blast, PRIMA del ciclo degli intenti: azzerano il piano, quindi il rifiuto di
 * `CollectAttackIntents` non le vedeva mai). Una riga per sito, ciascuna con il proprio mondo: pagare in alcuni
 * lascerebbe lo stordito a ricaricare gratis dall'ultimo. `ResolveCoverStructures` (le coperture, in Prep) nega e paga
 * dal [D-565]: le sue righe non stanno qui — il suo piano e' una cella e un bordo, non un'unita' — ma in
 * `Actions.CoverStructures.StunnedCasterChangesNothing` e `Actions.CoverStructures.StunnedCasterKeepsTheFreeRotation`.
 *
 * 🔴 **Cura e purificazione misurano anche l'EFFETTO e l'ATTIVAZIONE**, non solo la ricarica: un rifiuto che pagasse e
 * poi curasse lo stesso avrebbe il cooldown giusto e la voce giusta. La riga di controllo (non stordita) cura e si
 * attiva, ed e' cio' che rende lo «zero» delle altre una misura e non l'assenza della fixture.
 *
 * 🔑 **Le righe «alleato fuori portata» pinnano l'ORDINE**: il rifiuto precede la misura della portata, quindi lo
 * stordito lascia la sola causa `Stunned` e paga, mentre lo stesso piano senza stordimento lascia `OutOfRange` e non
 * paga ([D-200]). Cura e purificazione stanno nello slot 3 perche' l'indice dell'abilita' non coincida con quello
 * dell'unita' (l'attore e' la prima unita' del Blast).
 *
 * 🔴 **La riga «gia' in ricarica» e' la guardia `CanUseAbility`**: `ConsumeAbility` scrive un valore FISSO, quindi
 * chiamato su un'abilita' gia' in ricarica la RIPORTA al valore pieno. Uno stordito che pianifica a mano
 * un'abilita' non disponibile non deve vederla ricaricare piu' lenta di cosi'.
 *
 * Premessa per riga: il turno porta la voce di rifiuto `Stunned` (e nella fase giusta) — o, per i controlli non
 * storditi, non ne porta nessuna. Il residuo e' letto a fine turno: il Cleanup decrementa di uno, quindi
 * `PagaCooldown - 1` vuol dire «pagato una volta», `PagaCooldown - 2` «non ripagato», `0` «non pagato».
 *
 * ⌫ *Fino al 2026-10-31 il rifiuto «precedeva `ConsumeAbility`» e la ricarica NON partiva per un'azione negata
 * («Il piano si perde, la RICARICA no»).*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunDeniedMainActionPaysItsCooldownTest,
	"RefactorTactics.Status.StunDeniedMainActionPaysItsCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunDeniedMainActionPaysItsCooldownTest::RunTest(const FString&)
{
	const int32 Pagato = PagaCooldown - 1;       // un solo addebito, poi il decremento del Cleanup
	const int32 NonRipagato = PagaCooldown - 2;  // era gia' a `PagaCooldown - 1`, e scende di uno

	const FPagaStorditoCase Casi[] = {
		{ TEXT("Prep: controllo non stordito"), EPagaSito::Prep, false, false, false, Pagato,
		  TEXT("l'Overwatch armato paga: fissa il valore che lo stordito deve eguagliare") },
		{ TEXT("Prep: stordito"), EPagaSito::Prep, true, false, false, Pagato,
		  TEXT("armare costa l'azione principale: negata, ma pagata") },
		{ TEXT("Prep: stordito, gia' in ricarica"), EPagaSito::Prep, true, true, false, NonRipagato,
		  TEXT("non e' piu' utilizzabile: il Prep non la arma, il Blast ne rifiuta il piano e non la ripaga") },
		{ TEXT("Blast: controllo non stordito"), EPagaSito::Blast, false, false, false, Pagato,
		  TEXT("l'attacco che colpisce paga") },
		{ TEXT("Blast: stordito"), EPagaSito::Blast, true, false, false, Pagato,
		  TEXT("l'attacco negato dallo stordimento paga, UNA volta sola") },
		{ TEXT("Blast: stordito, gia' in ricarica"), EPagaSito::Blast, true, true, false, NonRipagato,
		  TEXT("[D-414]: la guardia `CanUseAbility` — `ConsumeAbility` la riporterebbe al valore pieno") },
		{ TEXT("Blast: stordito e gia' caduto"), EPagaSito::Blast, true, false, true, 0,
		  TEXT("un cadavere non paga un'azione che non ha mai tentato") },
		{ TEXT("Cleanup: controllo non stordito"), EPagaSito::Cleanup, false, false, false, Pagato,
		  TEXT("l'azione ambientale paga") },
		{ TEXT("Cleanup: stordito"), EPagaSito::Cleanup, true, false, false, Pagato,
		  TEXT("la principale ambientale negata paga: e' il sito del Cleanup") },
		{ TEXT("Cleanup: stordito, gia' in ricarica"), EPagaSito::Cleanup, true, true, false, NonRipagato,
		  TEXT("[D-414]: la guardia `CanUseAbility` anche qui") },
		{ TEXT("Cleanup: stordito e gia' caduto"), EPagaSito::Cleanup, true, false, true, 0,
		  TEXT("un cadavere non paga nemmeno nel Cleanup") },
		{ TEXT("Cura: controllo non stordito"), EPagaSito::Cura, false, false, false, Pagato,
		  TEXT("la cura che parte paga: fissa il valore che lo stordito deve eguagliare") },
		{ TEXT("Cura: stordito"), EPagaSito::Cura, true, false, false, Pagato,
		  TEXT("[D-564]: la cura negata dallo stordimento paga, UNA volta sola — prima curava e basta") },
		{ TEXT("Cura: stordito, gia' in ricarica"), EPagaSito::Cura, true, true, false, NonRipagato,
		  TEXT("[D-414]: la guardia `CanUseAbility` — il rifiuto lo scrive `CollectAttackIntents` e non ripaga") },
		{ TEXT("Cura: stordito e gia' caduto"), EPagaSito::Cura, true, false, true, 0,
		  TEXT("un cadavere non paga una cura che non ha mai tentato") },
		// 🔑 L'ORDINE: il rifiuto precede la misura della portata. Il controllo dice che lo STESSO piano, senza stordimento,
		// lascia `OutOfRange` e non paga ([D-200]); lo stordito lascia la sola causa `Stunned` e paga.
		{ TEXT("Cura: controllo non stordito, alleato fuori portata"), EPagaSito::Cura, false, false, false, 0,
		  TEXT("[D-200]: la portata decide se l'azione parte — fuori portata non paga, e lascia `OutOfRange`"), true },
		{ TEXT("Cura: stordito, alleato fuori portata"), EPagaSito::Cura, true, false, false, Pagato,
		  TEXT("[D-564]: il rifiuto precede la portata — una sola causa, `Stunned`, e la cura paga come ogni principale negata"), true },
		{ TEXT("Purificazione: controllo non stordito"), EPagaSito::Purificazione, false, false, false, Pagato,
		  TEXT("la purificazione che parte paga: fissa il valore che lo stordito deve eguagliare") },
		{ TEXT("Purificazione: stordito"), EPagaSito::Purificazione, true, false, false, Pagato,
		  TEXT("[D-564]: la purificazione negata dallo stordimento paga, UNA volta sola — prima purificava e basta") },
		{ TEXT("Purificazione: stordito, gia' in ricarica"), EPagaSito::Purificazione, true, true, false, NonRipagato,
		  TEXT("[D-414]: la guardia `CanUseAbility` — il rifiuto lo scrive `CollectAttackIntents` e non ripaga") },
		{ TEXT("Purificazione: stordito e gia' caduto"), EPagaSito::Purificazione, true, false, true, 0,
		  TEXT("un cadavere non paga una purificazione che non ha mai tentato") },
	};

	for (const FPagaStorditoCase& C : Casi)
	{
		const FPagaStorditoEsito Esito = PagaGiraStordito(*this, C);
		if (!Esito.bValido) { return false; }

		AddInfo(FString::Printf(TEXT("%s -> cooldown prima %d, residuo %d, rifiuti %d, effetto %d, attivazioni %d"),
			C.Nome, Esito.PremessaCooldown, Esito.Cooldown, Esito.Rifiuti, Esito.bEffetto ? 1 : 0, Esito.Attivazioni));

		// Le premesse: la riga misura quello che dichiara.
		if (C.bGiaInRicarica)
		{
			TestEqual(*FString::Printf(TEXT("%s: premessa: l'abilita' parte gia' in ricarica"), C.Nome),
				Esito.PremessaCooldown, Pagato);
		}
		if (!C.bAttoreCaduto)
		{
			TestEqual(*FString::Printf(TEXT("%s: premessa: voci di rifiuto per stordimento"), C.Nome),
				Esito.Rifiuti, C.bStordito ? 1 : 0);
		}
		if (C.bStordito && !C.bAttoreCaduto && Esito.Rifiuti == 1)
		{
			// ⚠️ Un'azione di Prep GIA' in ricarica non viene armata ne' rifiutata dal Prep (`CanUseAbility` falso, il ciclo
			// passa oltre senza azzerare il piano): la rifiuta il Blast, che la ritrova in cima al ciclo. La riga lo pinna
			// perche' e' il motivo per cui la guardia `CanUseAbility` del Blast conta anche per un'azione di Prep.
			const ERTMatchPhase Fase = C.Sito == EPagaSito::Prep
				? (C.bGiaInRicarica ? ERTMatchPhase::Blast : ERTMatchPhase::Prep)
				: C.Sito == EPagaSito::Cleanup ? ERTMatchPhase::Cleanup : ERTMatchPhase::Blast;
			TestTrue(*FString::Printf(TEXT("%s: premessa: il rifiuto e' nella fase attesa"), C.Nome),
				Esito.FaseDelRifiuto == Fase);
		}

		TestEqual(*FString::Printf(TEXT("%s: %s"), C.Nome, C.Perche), Esito.Cooldown, C.CooldownAtteso);

		if ((C.Sito == EPagaSito::Cura || C.Sito == EPagaSito::Purificazione) && !C.bAlleatoFuoriPortata)
		{
			// Solo il controllo non stordito fa il proprio effetto e si attiva: chi e' stordito, in ricarica o caduto non fa
			// ne' l'uno ne' l'altro. Un rifiuto che pagasse e poi curasse lo stesso passerebbe le righe sopra.
			const bool bFaEffetto = !C.bStordito;
			TestEqual(*FString::Printf(TEXT("%s: l'effetto proprio dell'azione"), C.Nome), Esito.bEffetto, bFaEffetto);
			TestEqual(*FString::Printf(TEXT("%s: attivazioni dell'attore"), C.Nome), Esito.Attivazioni, bFaEffetto ? 1 : 0);
		}
		if ((C.Sito == EPagaSito::Cura || C.Sito == EPagaSito::Purificazione) && C.bStordito && !C.bAttoreCaduto)
		{
			// La voce di rifiuto dice CHE azione e DA DOVE: una `Def` o una cella sbagliate nel blocco passerebbero cooldown e conteggio.
			TestEqual(*FString::Printf(TEXT("%s: il rifiuto nomina l'azione e la cella di chi agisce"), C.Nome),
				Esito.RifiutiDellAzione, Esito.Rifiuti);
		}
		if (C.bAlleatoFuoriPortata)
		{
			// Il controllo misura che la voce `OutOfRange` si vede quando c'e'; lo stordito non la lascia.
			TestEqual(*FString::Printf(TEXT("%s: voci di fuori portata"), C.Nome), Esito.FuoriPortata, C.bStordito ? 0 : 1);
		}
	}
	return true;
}

// =====================================================================================================
// D-575 (a): l'origine congelata e' PER UNITA'.
// =====================================================================================================

/**
 * **L'origine con cui il click ha giudicato si congela al lock-in PER UNITA'** ([D-575] (a), registrata in #3981: il numero vale al merge).
 *
 * Due attaccanti della stessa squadra, due bersagli-unita' fuori portata dalla cella dove ciascuno si trova nel Blast:
 *
 * - `A` pianifica uno scatto verso (2,0,0) che un ostacolo ferma in (1,0,0), e mira a (6,0,0): dalla cella d'arrivo dello scatto il
 *   bersaglio e' ESATTAMENTE alla portata (4), da (1,0,0) una cella oltre. Il click accettava: PAGA.
 * - `B` pianifica uno scatto che arriva (da (-5,3,0) a (-3,3,0)) e mira a (2,3,0): da dove si trova il bersaglio e' a distanza 5, oltre la
 *   portata, e il click rifiutava (il piano e' scritto a mano). GRATIS.
 *
 * Il bersaglio di `B` sta a distanza 3 dalla cella d'arrivo dello scatto di `A`: chi leggesse l'origine di un'ALTRA unita' (la prima
 * della lista, o quella dell'indice sbagliato) farebbe pagare `B`, o non farebbe pagare `A` (da (-3,3,0) il bersaglio di `A` e' a 9).
 * Le righe con una sola unita' in scatto non lo distinguono: con una sola voce, «la prima» e' sempre quella giusta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShiftedOriginPaymentIsPerUnitTest,
	"RefactorTactics.Actions.Blast.ShiftedOriginPaymentIsPerUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShiftedOriginPaymentIsPerUnitTest::RunTest(const FString&)
{
	UWorld* World = PagaMakeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	PagaSpawnMap(World, /*Radius=*/ 9, {}, {}, {});

	ARTUnit* A = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTUnit* B = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-5, 3, 0));
	ARTUnit* BersaglioA = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(6, 0, 0));
	ARTUnit* BersaglioB = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 3, 0));
	ARTUnit* Ostacolo = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("A"), A) || !TestNotNull(TEXT("B"), B) || !TestNotNull(TEXT("bersaglio di A"), BersaglioA)
		|| !TestNotNull(TEXT("bersaglio di B"), BersaglioB) || !TestNotNull(TEXT("ostacolo"), Ostacolo) || !TestNotNull(TEXT("TM"), TM))
	{
		PagaDestroyWorld(World);
		return false;
	}

	for (ARTUnit* Attaccante : { A, B })
	{
		Attaccante->Abilities[0]->CooldownTurns = PagaCooldown;
		Attaccante->PlannedAbilityIndex = 0;
		const int32 Scatto = RTAbilityFixtures::AddCoreAbility(Attaccante, TEXT("Action.Reposition"));
		Attaccante->PlannedDashAbility = Scatto;
	}
	if (!TestEqual(TEXT("premessa: la portata dell'attacco base e' 4"), A->Abilities[0]->RangeCells, 4))
	{
		PagaDestroyWorld(World);
		return false;
	}
	A->PlannedDashCell = FRTCellId(2, 0, 0);  // l'ostacolo lo ferma in (1,0,0)
	B->PlannedDashCell = FRTCellId(-3, 3, 0); // arriva
	A->DeclareAttackOnUnit(BersaglioA);
	B->DeclareAttackOnUnit(BersaglioB);

	TM->RefreshTeamKnowledgeNow();
	PagaRunTurn(TM);

	TestTrue(TEXT("premessa: lo scatto di A si e' fermato in (1,0,0)"), A->Cell == FRTCellId(1, 0, 0));
	TestTrue(TEXT("premessa: lo scatto di B e' arrivato in (-3,3,0)"), B->Cell == FRTCellId(-3, 3, 0));
	int32 FuoriPortataA = 0, FuoriPortataB = 0, Altre = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category != ERTLogCategory::Fallback || E.Outcome != static_cast<uint8>(ERTFallbackOutcome::Cancelled)) { continue; }
		if (E.Amount != static_cast<int32>(ERTActionInvalidReason::OutOfRange)) { ++Altre; }
		else if (E.UnitId == A->StableUnitId) { ++FuoriPortataA; }
		else if (E.UnitId == B->StableUnitId) { ++FuoriPortataB; }
		else { ++Altre; }
	}
	TestEqual(TEXT("premessa: una voce di fuori portata per A"), FuoriPortataA, 1);
	TestEqual(TEXT("premessa: una voce di fuori portata per B"), FuoriPortataB, 1);
	TestEqual(TEXT("premessa: nessun altro rifiuto"), Altre, 0);
	TestTrue(TEXT("A paga: il click accettava dalla cella d'arrivo dello scatto"), PagaHaPagato(A, 0));
	TestFalse(TEXT("B non paga: il click rifiutava anche dalla cella d'arrivo del suo scatto"), PagaHaPagato(B, 0));

	PagaDestroyWorld(World);
	return true;
}

// =====================================================================================================
// D-575 (a): INT-*, la porta senza varco e la commutazione rifiutata.
// =====================================================================================================

namespace
{
	enum class EPagaPorta : uint8
	{
		Riesce,     //< controllo: la porta e' chiusa e `Interact` la apre
		SenzaPorta, //< il bordo dichiarato non ha nessuna porta (`DoorlessIntents`, `NoEffect`)
		Bloccata,   //< la porta e' `Locked`: la commutazione e' rifiutata (`DoorToggleRefusals`, `DoorLocked`)
		Distrutta,  //< la porta e' `Destroyed`: terminale (`DoorToggleRefusals`, `DoorDestroyed`)
	};

	struct FPagaPortaEsito
	{
		bool bValido = false;
		bool bClickAccetta = false;
		bool bHaPagato = false;
		int32 SenzaPorta = 0;   //< voci Fallback/Cancelled `NoEffect`
		int32 Bloccata = 0;     //< voci Fallback/Cancelled `DoorLocked`
		int32 Distrutta = 0;    //< voci Fallback/Cancelled `DoorDestroyed`
		int32 AltreVoci = 0;
		int32 PorteAperte = 0;  //< voci di ambiente `DoorOpened`
		int32 PorteChiuse = 0;  //< voci di ambiente `DoorClosed`
		bool bAttaccanteVivo = true;
	};

	/**
	 * Un `Action.Interact` con una ricarica reso OSSERVABILE (`PagaCooldown`: quello spedito ne ha 0, ed e' la ragione per cui pagare o no non
	 * cambia nessuna partita oggi) contro la porta sul bordo W di (1,0,0). Si clicca con il percorso dei bordi, `HandleTargetEdge`
	 * (quello che scrive il lato, oggi senza chiamanti fuori dai test): e' l'ORACOLO di «il click avrebbe accettato il piano».
	 */
	FPagaPortaEsito PagaGiraPorta(FAutomationTestBase& Test, const TCHAR* Nome, EPagaPorta Caso)
	{
		FPagaPortaEsito Esito;
		UWorld* World = PagaMakeWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), Nome), World)) { return Esito; }
		ARTHexMapActor* MapActor = PagaSpawnMap(World, /*Radius=*/ 9, {}, {}, {});

		const FRTCellId CellaDelBordo(1, 0, 0);
		if (Caso != EPagaPorta::SenzaPorta)
		{
			const ERTHexDoorState Stato = Caso == EPagaPorta::Bloccata ? ERTHexDoorState::Locked
				: Caso == EPagaPorta::Distrutta ? ERTHexDoorState::Destroyed : ERTHexDoorState::Closed;
			FRTHexCellData Dati = *MapActor->MapAsset->FindCell(CellaDelBordo);
			Dati.Doors.Add(FRTHexDoor(ERTHexDirection::W, Stato));
			MapActor->MapAsset->AddOrUpdateCell(Dati);
			MapActor->MapAsset->SortCells();
		}

		ARTUnit* Attore = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
		ARTUnit* Spettatore = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-5, 0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: attore"), Nome), Attore)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: spettatore"), Nome), Spettatore)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: TM"), Nome), TM))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		const int32 Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.Interact"), 3);
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: Interact e' installata"), Nome), Idx != INDEX_NONE))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		Attore->Abilities[Idx]->CooldownTurns = PagaCooldown;

		TM->RefreshTeamKnowledgeNow();
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: controller"), Nome), PC))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		PC->SelectUnit(Attore, /*bRecordAsPlayerInput=*/ false);
		PC->ArmKitAbility(Idx);
		Esito.bClickAccetta = PC->HandleTargetEdge(CellaDelBordo, ERTHexDirection::W)
			&& Attore->PlannedAbilityIndex == Idx && Attore->bHasPlannedCoverEdge;
		Attore->SelectAbility(INDEX_NONE); // il turno non porta lo stato di UI dell'oracolo

		const TWeakObjectPtr<ARTUnit> AttoreDebole(Attore);
		PagaRunTurn(TM);
		const ARTUnit* AttoreDopo = AttoreDebole.Get(/*bEvenIfPendingKill*/ true);
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: l'attore e' ancora leggibile"), Nome), AttoreDopo))
		{
			PagaDestroyWorld(World);
			return Esito;
		}

		Esito.bHaPagato = PagaHaPagato(AttoreDopo, Idx);
		Esito.bAttaccanteVivo = AttoreDopo->IsAlive();
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Environment)
			{
				if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::DoorOpened)) { ++Esito.PorteAperte; }
				if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::DoorClosed)) { ++Esito.PorteChiuse; }
			}
			if (E.Category != ERTLogCategory::Fallback || E.Outcome != static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				|| E.UnitId != AttoreDopo->StableUnitId)
			{
				continue;
			}
			switch (static_cast<ERTActionInvalidReason>(E.Amount))
			{
			case ERTActionInvalidReason::NoEffect:      ++Esito.SenzaPorta; break;
			case ERTActionInvalidReason::DoorLocked:    ++Esito.Bloccata; break;
			case ERTActionInvalidReason::DoorDestroyed: ++Esito.Distrutta; break;
			default:                                    ++Esito.AltreVoci; break;
			}
		}
		Esito.bValido = true;
		PagaDestroyWorld(World);
		return Esito;
	}
}

/**
 * **`INT-*` PAGANO se il click avrebbe accettato il piano** ([D-575] (a), registrata in #3981: il numero vale al merge): la porta senza varco
 * (`NoEffect`) e la commutazione rifiutata (`DoorLocked`, `DoorDestroyed`).
 *
 * 🔑 **L'oracolo e' il click dei bordi** (`HandleTargetEdge`, che scrive il lato; oggi senza chiamanti fuori dai test). Misurato qui, riga per riga: il click
 * ACCETTA tutte e tre le cause, perche' guarda la portata e non guarda se sul bordo c'e' una porta ne' in che stato sia. Quindi pagano.
 *
 * ⚠️ **Il controllo senza causa NON paga, e non e' una scelta**: un `Action.Interact` che RIESCE non ha nessun sito di pagamento (la
 * ricarica la scrive `SpendStartedAbilities` per cio' che ha segnato `MarkAbilitySpent`, e una commutazione riuscita non lo segna). E' la
 * classe (c) di [D-575], «azioni non d'attacco: gratis e non decise». Il risultato e' un'economia in cui il rifiuto costa una ricarica e
 * il successo no: la riga lo pinna com'e' (`Riesce`), dichiarato nel report della PR come domanda per l'autore. Nessuna partita lo vede
 * oggi: l'`Interact` spedito ha ricarica 0, e qui la ricarica e' scritta dal banco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorInteractionRefusalsPayTest,
	"RefactorTactics.Actions.Blast.DoorInteractionRefusalsPay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorInteractionRefusalsPayTest::RunTest(const FString&)
{
	const FPagaPortaEsito Riesce = PagaGiraPorta(*this, TEXT("la porta chiusa si apre"), EPagaPorta::Riesce);
	const FPagaPortaEsito SenzaPorta = PagaGiraPorta(*this, TEXT("nessuna porta sul bordo"), EPagaPorta::SenzaPorta);
	const FPagaPortaEsito Bloccata = PagaGiraPorta(*this, TEXT("porta bloccata"), EPagaPorta::Bloccata);
	const FPagaPortaEsito Distrutta = PagaGiraPorta(*this, TEXT("porta distrutta"), EPagaPorta::Distrutta);
	if (!Riesce.bValido || !SenzaPorta.bValido || !Bloccata.bValido || !Distrutta.bValido) { return false; }

	// Le premesse: ogni riga vede la propria causa e nessun'altra, zero compresi. Il click accetta in tutte e quattro.
	const struct { const TCHAR* Nome; const FPagaPortaEsito* E; int32 Aperte; int32 NoEffect; int32 Locked; int32 Destroyed; } Righe[] = {
		{ TEXT("la porta chiusa si apre"), &Riesce, 1, 0, 0, 0 },
		{ TEXT("nessuna porta sul bordo"), &SenzaPorta, 0, 1, 0, 0 },
		{ TEXT("porta bloccata"), &Bloccata, 0, 0, 1, 0 },
		{ TEXT("porta distrutta"), &Distrutta, 0, 0, 0, 1 },
	};
	for (const auto& R : Righe)
	{
		TestTrue(*FString::Printf(TEXT("%s: il click dei bordi accetta il piano"), R.Nome), R.E->bClickAccetta);
		TestEqual(*FString::Printf(TEXT("%s: premessa: porte aperte"), R.Nome), R.E->PorteAperte, R.Aperte);
		TestEqual(*FString::Printf(TEXT("%s: premessa: porte chiuse"), R.Nome), R.E->PorteChiuse, 0);
		TestEqual(*FString::Printf(TEXT("%s: premessa: voci NoEffect"), R.Nome), R.E->SenzaPorta, R.NoEffect);
		TestEqual(*FString::Printf(TEXT("%s: premessa: voci DoorLocked"), R.Nome), R.E->Bloccata, R.Locked);
		TestEqual(*FString::Printf(TEXT("%s: premessa: voci DoorDestroyed"), R.Nome), R.E->Distrutta, R.Destroyed);
		TestEqual(*FString::Printf(TEXT("%s: premessa: nessun altro rifiuto"), R.Nome), R.E->AltreVoci, 0);
		TestTrue(*FString::Printf(TEXT("%s: premessa: chi agisce e' vivo"), R.Nome), R.E->bAttaccanteVivo);
	}

	TestFalse(TEXT("controllo: l'Interact che riesce NON paga (classe (c) di D-575: nessun sito di pagamento, gratis e non deciso)"),
		Riesce.bHaPagato);
	TestTrue(TEXT("INT-*: la porta senza varco, che il click accettava, paga"), SenzaPorta.bHaPagato);
	TestTrue(TEXT("INT-*: la commutazione rifiutata su una porta bloccata, che il click accettava, paga"), Bloccata.bHaPagato);
	TestTrue(TEXT("INT-*: la commutazione rifiutata su una porta distrutta, che il click accettava, paga"), Distrutta.bHaPagato);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
