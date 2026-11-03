#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo nella voce di fallback
#include "Turn/RTReactionLibrary.h"       // ERTReactionOutcome: la reazione armata che non scatta (RIC-3)
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Tests/RTAbilityFixtures.h"
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
		TiroCappatoDalFumo,     //< R7  tiro a cella cappato dal Fumo non osservato (TerrainCappedIntents)
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
		case EPagaCausa::InterrottaSenzaColpi:  A.Interrotte = 1; break;
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
		const FRTCellId CellaDelFumo(1, 0, 0);
		const FRTCellId CellaSuAltroPiano(3, 0, 1);
		TArray<FRTCellId> Muri;
		TArray<FRTCellId> Fumo;
		TArray<FRTCellId> InPiu;
		if (C.Causa == EPagaCausa::LineaBloccata || C.Causa == EPagaCausa::InterruptBloccato
			|| C.Causa == EPagaCausa::NonAggressivaBloccata)
		{
			Muri = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0) };
		}
		if (C.Causa == EPagaCausa::TiroCappatoDalFumo) { Fumo = { CellaDelFumo }; }
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
		case EPagaCausa::TiroCappatoDalFumo:
			Attaccante->DeclareAttackOnCell(CellaVuota);
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
		{
			// `Action.Interrupt` ha portata 1: l'interruttore sta accanto all'attaccante e lo coglie sulla cella dove sta.
			Aiutante = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0, 0));
			if (Aiutante)
			{
				const int32 InterruptIdx = RTAbilityFixtures::AddCoreAbility(Aiutante, TEXT("Action.Interrupt"));
				Aiutante->PlannedAbilityIndex = InterruptIdx;
				Aiutante->PlannedAttackTarget = Attaccante;
			}
			Attaccante->DeclareAttackOnCell(FRTCellId(0, 3, 0)); // in portata, e nessuno ci sta
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
 * sono le cause che si conoscono SOLO a risoluzione; le due che non pagano sono geometria nota in pianificazione
 * (`R9`) e la guardia di vita di [D-209] (`R10`). Una versione ingenua — «l'intento esiste, quindi paga» —
 * romperebbe `R9`: `ValidateInstance` risponde `None` per ogni bersaglio-CELLA, e la portata la giudica solo
 * `CollectHexAttacks`, in silenzio.
 *
 * 🔴 **Il controllo di `R9` e' la riga `AreaVuotaAlBordo`**: stessa scena di `CellaOltrePortata` con la mira spostata
 * di UNA cella, dentro la portata. Una cella dopo, l'azione non paga; sul bordo, paga. Senza la coppia non si
 * distingue «l'attacco a vuoto non paga mai» (il difetto) da «l'attacco fuori portata non paga» (la regola).
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
		{ TEXT("R7 tiro cappato dal Fumo non osservato"), EPagaCausa::TiroCappatoDalFumo, true,
		  TEXT("il click leggeva la mappa ottimistica e lo ha accettato: il costo dell'ottimismo si paga qui") },
		{ TEXT("R8 annullata da un Interrupt, senza colpi"), EPagaCausa::InterrottaSenzaColpi, true,
		  TEXT("[D-414] punto 1: un controllo che impedisce l'abilita' non restituisce la ricarica") },
		// ➕ **Fix wave di `#3134`**: tre righe che difendono tre clausole che nessun test copriva.
		{ TEXT("R4 l'Interrupt come attore, bersaglio dietro un muro"), EPagaCausa::InterruptBloccato, true,
		  TEXT("confermato dall'autore il 2026-11-01: `bCountsAsAttack` e' vero per `Action.Interrupt`, quindi R4 vale anche per chi interrompe") },
		{ TEXT("azione non aggressiva bloccata dal muro"), EPagaCausa::NonAggressivaBloccata, false,
		  TEXT("la clausola `!bCountsAsAttack` di `IntentoPartitoSenzaColpo`: l'intento e' in `BlockedIntents` ma non e' un'aggressione") },
		{ TEXT("azione non aggressiva, bersaglio-unita' gia' morto"), EPagaCausa::NonAggressivaBersaglioMorto, false,
		  TEXT("la guardia `bCountsAsAttack` di R6: come su `main`, un'azione che non e' un colpo ha le proprie regole") },
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
		AttaccoDopoScattoFermato, //< attacco base preceduto da uno scatto che un ostacolo ferma a meta'
	};

	/** UNA scena: le righe differiscono per UNA variabile fra il controllo e il caso. */
	struct FRic3Scena
	{
		ERic3Azione Azione = ERic3Azione::AttaccoBase;
		ERic3Bersaglio Bersaglio = ERic3Bersaglio::Nemico;
		int32 Distanza = 3;                //< del bersaglio, lungo +X dall'origine dove sta chi agisce
		bool bOltrePortataDichiarata = false; //< se vero la distanza e' `portata + 1` e `Distanza` si ignora
		bool bFumoSullaLinea = false;      //< Fumo sulla cella (1,0,0), fra chi agisce e il bersaglio
		int32 Visione = INDEX_NONE;        //< se >= 0 sostituisce `VisionRange` di chi agisce
		bool bOstacoloSulloScatto = false; //< solo `AttaccoDopoScattoFermato`: un avversario sulla cella d'arrivo dello scatto
		bool bNemicoAttacca = false;       //< solo `ReazioneCounter`: un avversario adiacente colpisce chi ha la reazione armata
		bool bAttoreCaduto = false;        //< chi agisce arriva al turno gia' caduto (salute 0) col piano ancora addosso
	};

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
		bool bControllaCella = false; //< solo `AttaccoDopoScattoFermato`
		FRTCellId CellaFinale;        //< dove chi agisce deve trovarsi a fine turno
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
	};

	FRic3Esito Ric3Gira(FAutomationTestBase& Test, const TCHAR* Nome, const FRic3Scena& S)
	{
		FRic3Esito Esito;
		UWorld* World = PagaMakeWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), Nome), World)) { return Esito; }

		TArray<FRTCellId> Fumo;
		if (S.bFumoSullaLinea) { Fumo.Add(FRTCellId(1, 0, 0)); }
		ARTHexMapActor* MapActor = PagaSpawnMap(World, /*Radius=*/ 9, {}, Fumo, {});

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

		// L'azione sta in uno slot che l'array dei cooldown COPRE, o il contatore non e' osservabile affatto.
		int32 Idx = 0;
		switch (S.Azione)
		{
		case ERic3Azione::ModifyArc:       Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.ModifyArc"), 3); break;
		case ERic3Azione::Ignite:          Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.Ignite"), 3); break;
		case ERic3Azione::ReazioneCounter: Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Attore, TEXT("Action.Counter"), 3); break;
		default: break; // l'attacco base e' lo slot 0
		}
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: l'azione e' installata"), Nome), Idx != INDEX_NONE))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		Attore->Abilities[Idx]->CooldownTurns = PagaCooldown;
		const int32 Portata = Attore->Abilities[Idx]->RangeCells;

		// Dove sta il bersaglio. Con lo scatto la distanza e' misurata dalla cella d'ARRIVO dello scatto (2,0,0): da la'
		// il bersaglio e' ESATTAMENTE alla portata, dalla cella dove lo scatto si ferma (1,0,0) e' una cella oltre.
		const FRTCellId CellaScatto(2, 0, 0);
		const int32 Distanza = S.Azione == ERic3Azione::AttaccoDopoScattoFermato ? CellaScatto.X + Portata
			: S.bOltrePortataDichiarata ? Portata + 1 : S.Distanza;
		const FRTCellId CellaBersaglio(Distanza, 0, 0);

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
			if (Bersaglio && S.Bersaglio == ERic3Bersaglio::Morto) { Bersaglio->Health = 0; }
			if (Bersaglio) { SaluteBersaglio = Bersaglio->Health; BersaglioDebole = Bersaglio; }

			Attore->PlannedAbilityIndex = Idx;
			Attore->DeclareAttackOnUnit(Bersaglio); // `nullptr` = piano senza bersaglio-unita' ne' cella

			if (S.Azione == ERic3Azione::AttaccoDopoScattoFermato)
			{
				// `Action.Reposition`: uno scatto lineare di 2 celle SENZA impatto ne' effetti. Sulla cella d'arrivo, se c'e'
				// l'ostacolo, lo scatto si ferma una cella prima: e' l'unica variabile fra controllo e caso.
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
		}

		if (S.bAttoreCaduto) { Attore->Health = 0; } // arriva al Blast col piano ancora addosso

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
		const ARTUnit* BersaglioDopo = BersaglioDebole.Get(/*bEvenIfPendingKill*/ true);
		Esito.bDanno = BersaglioDopo && BersaglioDopo->Health < SaluteBersaglio;
		Esito.bArco = MapActor->MapAsset->Transitions.Num() > ArchiPrima;
		const FRTHexCellData* DatiCella = MapActor->MapAsset->FindCell(CellaBersaglio);
		Esito.bFuoco = DatiCella && DatiCella->Surface == ERTHexSurface::Fire;

		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Reaction)
			{
				if (E.Outcome == static_cast<uint8>(ERTReactionOutcome::Activated)) { ++Esito.Attivate; }
				if (E.Outcome == static_cast<uint8>(ERTReactionOutcome::NotTriggered)) { ++Esito.NonInnescate; }
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

		Test.AddInfo(FString::Printf(TEXT("%s -> cooldown residuo %d, rifiuti %d, danno %d, arco %d, fuoco %d, reazioni attivate %d / non innescate %d, cella finale (%d,%d,%d)"),
			*Etichetta, E.Cooldown, E.Rifiuti.Num(), E.bDanno ? 1 : 0, E.bArco ? 1 : 0, E.bFuoco ? 1 : 0, E.Attivate,
			E.NonInnescate, E.CellaFinale.X, E.CellaFinale.Y, E.CellaFinale.Layer));

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

		// UNA volta sola: chi paga lascia il residuo di PagaCooldown, decrementato dal Cleanup, e non di piu'.
		if (A.bPaga)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: %s (residuo: pagato una volta)"), *Etichetta, Perche), E.Cooldown, PagaCooldown - 1);
		}
		else
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: %s (residuo: non pagato)"), *Etichetta, Perche), E.Cooldown, 0);
		}
		return true;
	}

	FRic3Atteso Ric3Atteso(bool bPaga, ERTActionInvalidReason Voce = ERTActionInvalidReason::None)
	{
		FRic3Atteso A;
		A.bPaga = bPaga;
		A.Voce = Voce;
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
 * - bersaglio-UNITA' fuori portata SOLO per il cap del Fumo non osservato: stessa ragione di `R7` per il tiro a cella
 *   ([D-465] (3)): il click leggeva la mappa ottimistica e lo ha accettato, e il costo dell'ottimismo si paga qui.
 *   Controparte gratis: oltre la portata DICHIARATA (`Actions.Blast.UndecidedCausesStayFree`).
 *
 * ⚠️ **Dichiarato, non coperto**: il bersaglio-unita' (o `ModifyArc`) fuori portata perche' uno SCATTO dell'attore si e'
 * fermato. Il click ha giudicato dalla cella d'arrivo dello scatto, ma il resolver non conserva quell'origine e non sa
 * distinguere il piano che il click avrebbe accettato da quello che rifiuta: la riga di `UndecidedCausesStayFree` lo
 * pinna gratis e lo riporta come `BLOCKED — DECISION REQUIRED`.
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
		FRic3Atteso AttesoControllo = Ric3Atteso(true);
		AttesoControllo.bArco = true;
		if (!Ric3Verifica(*this, TEXT("ModifyArc: bersaglio vivo"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("l'arco si apre e l'azione paga come prima"))) { return false; }

		FRic3Scena Morto = Controllo;
		Morto.Bersaglio = ERic3Bersaglio::Morto;
		if (!Ric3Verifica(*this, TEXT("ModifyArc: bersaglio-unita' gia' morto"), TEXT("caso"), Morto, Ric3Atteso(true),
			TEXT("il click lo ha accettato da vivo: la morte e' un fatto di risoluzione e la ricarica parte"))) { return false; }

		FRic3Scena Assente = Controllo;
		Assente.Bersaglio = ERic3Bersaglio::Assente;
		if (!Ric3Verifica(*this, TEXT("ModifyArc: nessun bersaglio dichiarato"), TEXT("controparte"), Assente, Ric3Atteso(false),
			TEXT("il click non produce un piano senza bersaglio: resta gratis"))) { return false; }

		FRic3Scena Caduto = Morto;
		Caduto.bAttoreCaduto = true;
		if (!Ric3Verifica(*this, TEXT("ModifyArc: bersaglio morto e attore gia' caduto"), TEXT("controparte"), Caduto, Ric3Atteso(false),
			TEXT("R10, D-209: un cadavere non paga un'azione che non ha tentato"))) { return false; }
	}

	// --- azione ambientale, bersaglio-unita' morto ------------------------------------------------
	{
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::Ignite;
		Controllo.Distanza = 3;
		FRic3Atteso AttesoControllo = Ric3Atteso(true);
		AttesoControllo.bFuoco = true;
		AttesoControllo.bDanno = true; // chi sta sulla cella in fiamme prende fuoco: l'azione ha fatto il suo lavoro
		if (!Ric3Verifica(*this, TEXT("Ignite: bersaglio vivo"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("la cella brucia e l'azione paga come prima"))) { return false; }

		FRic3Scena Morto = Controllo;
		Morto.Bersaglio = ERic3Bersaglio::Morto;
		if (!Ric3Verifica(*this, TEXT("Ignite: bersaglio-unita' gia' morto"), TEXT("caso"), Morto, Ric3Atteso(true),
			TEXT("il click lo ha accettato da vivo: la morte e' un fatto di risoluzione e la ricarica parte"))) { return false; }

		FRic3Scena Assente = Controllo;
		Assente.Bersaglio = ERic3Bersaglio::Assente;
		if (!Ric3Verifica(*this, TEXT("Ignite: nessun bersaglio dichiarato"), TEXT("controparte"), Assente, Ric3Atteso(false),
			TEXT("il click non produce un piano senza bersaglio: resta gratis"))) { return false; }

		FRic3Scena Caduto = Morto;
		Caduto.bAttoreCaduto = true;
		if (!Ric3Verifica(*this, TEXT("Ignite: bersaglio morto e attore gia' caduto"), TEXT("controparte"), Caduto, Ric3Atteso(false),
			TEXT("R10, D-209: un cadavere non paga un'azione che non ha tentato"))) { return false; }
	}

	// --- bersaglio-unita' fuori portata SOLO per il cap del Fumo ----------------------------------
	{
		FRic3Scena Controllo; // distanza 3, nessun Fumo: dentro la portata di Ivrin (>= 3)
		FRic3Atteso AttesoControllo = Ric3Atteso(true);
		AttesoControllo.bDanno = true;
		if (!Ric3Verifica(*this, TEXT("bersaglio-unita' a distanza 3"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("senza Fumo l'attacco colpisce e paga"))) { return false; }

		FRic3Scena ConFumo = Controllo;
		ConFumo.bFumoSullaLinea = true;
		if (!Ric3Verifica(*this, TEXT("bersaglio-unita' dietro il Fumo non osservato"), TEXT("caso"), ConFumo,
			Ric3Atteso(true, ERTActionInvalidReason::OutOfRange),
			TEXT("la distanza e' dentro la portata dichiarata ma oltre il cap del Fumo: stessa causa di R7"))) { return false; }
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
 * - bersaglio ALLEATO: per un'unita' comandabile il click non bersaglia ma SELEZIONA (`ResolveOutcome`); ⚠️ per un
 *   alleato non comandabile il click NON controlla la squadra — scarto del click, non del resolver: resta gratis finche'
 *   non lo decide l'autore (`BLOCKED — DECISION REQUIRED`);
 * - bersaglio PERSO (`TargetGone`): il click dichiara sempre un'unita' o una cella, quindi non produce questo piano;
 * - bersaglio SCONOSCIUTO (`TargetUnknown`): il click non bersaglia un'unita' che la squadra non vede
 *   (`IsKnownToObserver`), e un'unita' vista in pianificazione lascia comunque un contatto fino al Blast;
 * - `ModifyArc` oltre la portata dichiarata, senza scatti: il click lo rifiuta;
 * - **DA DECIDERE, e gratis oggi — l'attacco fuori portata dopo uno SCATTO FERMATO**: il click ha giudicato dalla cella
 *   d'arrivo dello scatto e avrebbe accettato, ma il resolver non conserva quell'origine (vedi sopra);
 * - **DA DECIDERE, e gratis oggi — la reazione armata che non scatta**: non e' un'azione fallita ma lo stato normale dello
 *   slot (`ERTReactionOutcome::NotTriggered`), e pagarlo cambierebbe l'economia della reazione ([D-092]).
 *
 * ⌫ *Fino al 2026-11-03 queste righe erano tre, senza controllo, e dichiaravano «DA DECIDERE» cio' che il click decide.*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUndecidedCausesStayFreeTest,
	"RefactorTactics.Actions.Blast.UndecidedCausesStayFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUndecidedCausesStayFreeTest::RunTest(const FString&)
{
	FRic3Atteso Colpisce = Ric3Atteso(true);
	Colpisce.bDanno = true;

	// --- bersaglio-unita' oltre la portata dichiarata ---------------------------------------------
	{
		FRic3Scena Controllo; // distanza 3: dentro la portata
		if (!Ric3Verifica(*this, TEXT("bersaglio-unita' dentro la portata"), TEXT("controllo"), Controllo, Colpisce,
			TEXT("colpisce e paga"))) { return false; }
		FRic3Scena Oltre = Controllo;
		Oltre.bOltrePortataDichiarata = true;
		if (!Ric3Verifica(*this, TEXT("bersaglio-unita' oltre la portata dichiarata"), TEXT("caso"), Oltre,
			Ric3Atteso(false, ERTActionInvalidReason::OutOfRange),
			TEXT("D-200: la portata e' geometria nota in pianificazione, il click la rifiuta"))) { return false; }
	}

	// --- bersaglio alleato -------------------------------------------------------------------------
	{
		FRic3Scena Controllo;
		if (!Ric3Verifica(*this, TEXT("bersaglio avversario"), TEXT("controllo"), Controllo, Colpisce,
			TEXT("colpisce e paga"))) { return false; }
		FRic3Scena Alleato = Controllo;
		Alleato.Bersaglio = ERic3Bersaglio::Alleato;
		if (!Ric3Verifica(*this, TEXT("bersaglio alleato"), TEXT("caso"), Alleato,
			Ric3Atteso(false, ERTActionInvalidReason::TargetFriendly),
			TEXT("il click non bersaglia un'unita' comandabile della propria squadra"))) { return false; }
	}

	// --- bersaglio perso ---------------------------------------------------------------------------
	{
		FRic3Scena Controllo;
		if (!Ric3Verifica(*this, TEXT("bersaglio dichiarato"), TEXT("controllo"), Controllo, Colpisce,
			TEXT("colpisce e paga"))) { return false; }
		FRic3Scena Perso = Controllo;
		Perso.Bersaglio = ERic3Bersaglio::Assente;
		if (!Ric3Verifica(*this, TEXT("bersaglio perso"), TEXT("caso"), Perso,
			Ric3Atteso(false, ERTActionInvalidReason::TargetGone),
			TEXT("il click dichiara sempre un'unita' o una cella: questo piano non nasce da un click"))) { return false; }
	}

	// --- bersaglio sconosciuto ---------------------------------------------------------------------
	{
		FRic3Scena Controllo; // visione di default: il bersaglio a distanza 3 si vede
		if (!Ric3Verifica(*this, TEXT("bersaglio visto dalla squadra"), TEXT("controllo"), Controllo, Colpisce,
			TEXT("colpisce e paga"))) { return false; }
		FRic3Scena Ignoto = Controllo;
		Ignoto.Visione = 1; // la squadra non vede piu' a distanza 3, e non ha mai avuto un contatto
		if (!Ric3Verifica(*this, TEXT("bersaglio mai visto dalla squadra"), TEXT("caso"), Ignoto,
			Ric3Atteso(false, ERTActionInvalidReason::TargetUnknown),
			TEXT("il click non bersaglia cio' che non si vede"))) { return false; }
	}

	// --- ModifyArc oltre la portata dichiarata -----------------------------------------------------
	{
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::ModifyArc;
		Controllo.Distanza = 2;
		FRic3Atteso AttesoControllo = Ric3Atteso(true);
		AttesoControllo.bArco = true;
		if (!Ric3Verifica(*this, TEXT("ModifyArc dentro la portata"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("l'arco si apre e paga"))) { return false; }
		FRic3Scena Oltre = Controllo;
		Oltre.bOltrePortataDichiarata = true;
		if (!Ric3Verifica(*this, TEXT("ModifyArc oltre la portata dichiarata"), TEXT("caso"), Oltre,
			Ric3Atteso(false, ERTActionInvalidReason::OutOfRange),
			TEXT("D-200: la portata e' geometria nota in pianificazione, il click la rifiuta"))) { return false; }
	}

	// --- DA DECIDERE: attacco fuori portata dopo uno scatto fermato --------------------------------
	{
		FRic3Scena Controllo; // lo scatto arriva a (2,0,0): da li' il bersaglio e' esattamente alla portata
		Controllo.Azione = ERic3Azione::AttaccoDopoScattoFermato;
		FRic3Atteso AttesoControllo = Colpisce;
		AttesoControllo.bControllaCella = true;
		AttesoControllo.CellaFinale = FRTCellId(2, 0, 0);
		if (!Ric3Verifica(*this, TEXT("scatto compiuto, poi attacco"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("lo scatto arriva, l'attacco colpisce e paga"))) { return false; }
		FRic3Scena Fermato = Controllo;
		Fermato.bOstacoloSulloScatto = true;
		FRic3Atteso AttesoFermato = Ric3Atteso(false, ERTActionInvalidReason::OutOfRange);
		AttesoFermato.bControllaCella = true;
		AttesoFermato.CellaFinale = FRTCellId(1, 0, 0);
		if (!Ric3Verifica(*this, TEXT("scatto fermato, poi attacco"), TEXT("caso"), Fermato, AttesoFermato,
			TEXT("DA DECIDERE — il click avrebbe accettato, ma il resolver non conserva l'origine con cui ha giudicato"))) { return false; }
	}

	// --- DA DECIDERE: reazione armata che non scatta -----------------------------------------------
	{
		FRic3Scena Controllo;
		Controllo.Azione = ERic3Azione::ReazioneCounter;
		Controllo.bNemicoAttacca = true;
		FRic3Atteso AttesoControllo = Ric3Atteso(true);
		AttesoControllo.Attivate = 1;
		AttesoControllo.bControllaCella = false;
		if (!Ric3Verifica(*this, TEXT("reazione armata e colpita"), TEXT("controllo"), Controllo, AttesoControllo,
			TEXT("la reazione scatta e paga, come sempre"))) { return false; }
		FRic3Scena Muta = Controllo;
		Muta.bNemicoAttacca = false;
		FRic3Atteso AttesoMuta = Ric3Atteso(false);
		AttesoMuta.NonInnescate = 1;
		if (!Ric3Verifica(*this, TEXT("reazione armata, nessun trigger"), TEXT("caso"), Muta, AttesoMuta,
			TEXT("DA DECIDERE — non e' un'azione fallita ma lo stato normale dello slot: paga solo al trigger"))) { return false; }
	}
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

#endif // WITH_DEV_AUTOMATION_TESTS
