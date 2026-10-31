#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo nella voce di fallback
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
		InterruptBloccato,      //< R4  l'INTERRUPT come attore: il bersaglio-unita' sta dietro un muro (DA CONFERMARE dall'autore)
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
		  TEXT("DA CONFERMARE dall'autore: `bCountsAsAttack` e' vero per `Action.Interrupt`, quindi R4 vale anche per chi interrompe") },
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
// Le cause NON DECISE: pinnano il comportamento di OGGI, perche' una decisione futura li cambi di proposito.
// =====================================================================================================

namespace
{
	enum class EPagaNonDeciso : uint8
	{
		BersaglioUnitaOltrePortata, //< `ValidateInstance` -> `OutOfRange`
		BersaglioAlleato,           //< `ValidateInstance` -> `TargetFriendly`
		BersaglioPerso,             //< nessun bersaglio dichiarato -> `TargetGone`
	};

	struct FPagaNonDecisoCase
	{
		const TCHAR* Nome;
		EPagaNonDeciso Causa;
		ERTActionInvalidReason Voce; //< la voce di rifiuto che il turno DEVE portare, o la riga misura un'altra scena
	};
}

/**
 * **DA DECIDERE — cio' che oggi resta GRATIS e la decisione del 2026-10-31 non ha toccato.**
 *
 * La decisione d'autore copre le cause note SOLO a risoluzione elencate in `StartedActionPaysWithoutAHit`. Queste
 * tre sono raggiungibili con un fixture semplice e fermano l'azione prima che diventi un intento (fallback
 * `Cancel`): non sono ne' nella tabella delle cause decise ne' nei confini del click, e il comportamento di oggi —
 * gratis — viene pinnato qui. Se una decisione futura le fa pagare, queste righe diventano rosse e vanno
 * riscritte di proposito, non aggirate.
 *
 * - bersaglio-UNITA' fuori portata solo a risoluzione (l'unita' e' scattata o la portata e' cambiata): DA DECIDERE;
 * - bersaglio ALLEATO (`TargetFriendly`): DA DECIDERE;
 * - bersaglio PERSO (`TargetGone`: nessun bersaglio dichiarato): DA DECIDERE.
 *
 * ⚠️ Le altre cause non decise (`TargetUnknown`, porta senza varco, commutazione rifiutata, mappa assente,
 * `ModifyArc`, azioni ambientali, reazione armata senza trigger, fallimenti di Prep, Dash rifiutato) non hanno un
 * fixture semplice qui: stanno nei `followups` della sessione che ha scritto questo file, col motivo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUndecidedCausesStayFreeTest,
	"RefactorTactics.Actions.Blast.UndecidedCausesStayFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUndecidedCausesStayFreeTest::RunTest(const FString&)
{
	const FPagaNonDecisoCase Casi[] = {
		{ TEXT("bersaglio-unita' fuori portata a risoluzione"), EPagaNonDeciso::BersaglioUnitaOltrePortata, ERTActionInvalidReason::OutOfRange },
		{ TEXT("bersaglio alleato"), EPagaNonDeciso::BersaglioAlleato, ERTActionInvalidReason::TargetFriendly },
		{ TEXT("bersaglio perso"), EPagaNonDeciso::BersaglioPerso, ERTActionInvalidReason::TargetGone },
	};

	for (const FPagaNonDecisoCase& C : Casi)
	{
		UWorld* World = PagaMakeWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		PagaSpawnMap(World, 9, {}, {}, {});

		ARTUnit* Attaccante = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
		ARTUnit* Spettatore = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-5, 0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TestNotNull(*FString::Printf(TEXT("%s: attaccante"), C.Nome), Attaccante)
			|| !TestNotNull(*FString::Printf(TEXT("%s: spettatore"), C.Nome), Spettatore)
			|| !TestNotNull(*FString::Printf(TEXT("%s: TM"), C.Nome), TM))
		{
			PagaDestroyWorld(World);
			return false;
		}

		URTActionData* Azione = Attaccante->Abilities[0];
		Azione->CooldownTurns = PagaCooldown;
		const int32 Portata = Azione->RangeCells;
		Attaccante->PlannedAbilityIndex = 0;

		ARTUnit* Bersaglio = nullptr;
		switch (C.Causa)
		{
		case EPagaNonDeciso::BersaglioUnitaOltrePortata:
			Bersaglio = PagaSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(Portata + 1, 0, 0));
			if (Bersaglio) { Attaccante->DeclareAttackOnUnit(Bersaglio); }
			break;
		case EPagaNonDeciso::BersaglioAlleato:
			Bersaglio = PagaSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 0, 0));
			if (Bersaglio) { Attaccante->DeclareAttackOnUnit(Bersaglio); }
			break;
		case EPagaNonDeciso::BersaglioPerso:
			Attaccante->DeclareAttackOnUnit(nullptr); // piano senza bersaglio-unita' ne' cella
			break;
		}
		if (C.Causa != EPagaNonDeciso::BersaglioPerso && !TestNotNull(*FString::Printf(TEXT("%s: bersaglio"), C.Nome), Bersaglio))
		{
			PagaDestroyWorld(World);
			return false;
		}

		PagaRunTurn(TM);

		int32 Voci = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.UnitId == Attaccante->StableUnitId && E.Amount == static_cast<int32>(C.Voce))
			{
				++Voci;
			}
		}
		AddInfo(FString::Printf(TEXT("%s -> cooldown residuo %d, voci del motivo %d"),
			C.Nome, Attaccante->GetAbilityCooldown(0), Voci));
		TestEqual(*FString::Printf(TEXT("%s: premessa: il turno porta la voce di rifiuto attesa"), C.Nome), Voci, 1);
		TestFalse(*FString::Printf(TEXT("%s: DA DECIDERE — oggi non paga"), C.Nome), PagaHaPagato(Attaccante, 0));

		PagaDestroyWorld(World);
	}
	return true;
}

// =====================================================================================================
// `Status.Stunned` nega l'azione PRINCIPALE, e la decisione del 2026-10-31 dice che chi la tenta PAGA.
// =====================================================================================================

namespace
{
	enum class EPagaSito : uint8 { Prep, Blast, Cleanup };

	struct FPagaStorditoCase
	{
		const TCHAR* Nome;
		EPagaSito Sito;
		bool bStordito;
		bool bGiaInRicarica; //< l'abilita' era GIA' in ricarica: `ConsumeAbility` la riporterebbe al valore pieno
		bool bAttoreCaduto;
		int32 CooldownAtteso; //< residuo a fine turno (il Cleanup decrementa di uno)
		const TCHAR* Perche;
	};

	struct FPagaStorditoEsito
	{
		bool bValido = false;
		int32 Cooldown = -1;
		int32 Rifiuti = 0;                   //< voci Fallback/Cancelled `Stunned` di chi agisce
		ERTMatchPhase FaseDelRifiuto = ERTMatchPhase::Planning;
		int32 PremessaCooldown = -1;         //< il residuo PRIMA del turno, per le righe «gia' in ricarica»
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
		}
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: l'abilita' e' installata"), C.Nome), Idx != INDEX_NONE))
		{
			PagaDestroyWorld(World);
			return Esito;
		}
		Attore->Abilities[Idx]->CooldownTurns = PagaCooldown;
		Attore->PlannedAbilityIndex = Idx;
		if (C.Sito != EPagaSito::Prep) { Attore->DeclareAttackOnUnit(Nemico); }

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
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned)
				&& E.UnitId == AttoreDopo->StableUnitId)
			{
				++Esito.Rifiuti;
				Esito.FaseDelRifiuto = E.Phase;
			}
		}
		Esito.bValido = true;
		PagaDestroyWorld(World);
		return Esito;
	}
}

/**
 * **Lo stordito PAGA l'azione principale che gli viene negata, nei TRE siti — e una volta sola** ([D-416], [D-414],
 * decisione d'autore del 2026-10-31: «Lo stordito (D-416) paga?» «si»).
 *
 * I siti che consumano una principale sono tre e ognuno ha la propria guardia: `ResolvePrep` (Overwatch e
 * predittive), `CollectAttackIntents` (Blast) e `ResolveEnvironment` (Cleanup). Una riga per sito, ciascuna con il
 * proprio mondo: pagare in due su tre lascerebbe lo stordito a ricaricare gratis dall'ultimo.
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
		  TEXT("la principale ambientale negata paga: e' il terzo sito") },
		{ TEXT("Cleanup: stordito, gia' in ricarica"), EPagaSito::Cleanup, true, true, false, NonRipagato,
		  TEXT("[D-414]: la guardia `CanUseAbility` anche qui") },
		{ TEXT("Cleanup: stordito e gia' caduto"), EPagaSito::Cleanup, true, false, true, 0,
		  TEXT("un cadavere non paga nemmeno nel Cleanup") },
	};

	for (const FPagaStorditoCase& C : Casi)
	{
		const FPagaStorditoEsito Esito = PagaGiraStordito(*this, C);
		if (!Esito.bValido) { return false; }

		AddInfo(FString::Printf(TEXT("%s -> cooldown prima %d, residuo %d, rifiuti %d"),
			C.Nome, Esito.PremessaCooldown, Esito.Cooldown, Esito.Rifiuti));

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
				: C.Sito == EPagaSito::Blast ? ERTMatchPhase::Blast : ERTMatchPhase::Cleanup;
			TestTrue(*FString::Printf(TEXT("%s: premessa: il rifiuto e' nella fase attesa"), C.Nome),
				Esito.FaseDelRifiuto == Fase);
		}

		TestEqual(*FString::Printf(TEXT("%s: %s"), C.Nome, C.Perche), Esito.Cooldown, C.CooldownAtteso);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
