#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexDoorLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

bool URTHexCoverLibrary::EdgeDirection(const FRTCellId& From, const FRTCellId& To, ERTHexDirection& OutDirection)
{
	if (From.Layer != To.Layer)
	{
		return false; // layer diversi: non sono adiacenti, si collegano con archi espliciti
	}

	// I sei vicini sono restituiti in ordine di direzione (E..SE): l'indice E' la direzione.
	const TArray<FRTCellId> Ring = URTHexLibrary::Neighbors(From);
	for (int32 D = 0; D < Ring.Num(); ++D)
	{
		if (Ring[D] == To)
		{
			OutDirection = static_cast<ERTHexDirection>(D);
			return true;
		}
	}
	return false;
}

ERTHexCoverType URTHexCoverLibrary::CoverBetween(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To)
{
	if (Map == nullptr)
	{
		return ERTHexCoverType::None;
	}

	// Il bordo ha due facce: quella dichiarata da `From` verso `To` e quella da `To` verso `From`. Se una sola
	// e' stata disegnata, vale comunque — la barriera e' fisica, non un attributo di chi la possiede.
	ERTHexDirection Forward = ERTHexDirection::E;
	ERTHexDirection Backward = ERTHexDirection::E;
	if (!EdgeDirection(From, To, Forward) || !EdgeDirection(To, From, Backward))
	{
		return ERTHexCoverType::None; // non adiacenti: non condividono un bordo
	}

	// L'ordine dell'enum (None < Low < High) e' la sua gerarchia: la faccia piu' alta decide.
	auto Higher = [](ERTHexCoverType A, ERTHexCoverType B)
	{
		return static_cast<uint8>(A) >= static_cast<uint8>(B) ? A : B;
	};

	ERTHexCoverType Best = ERTHexCoverType::None;
	if (const FRTHexCellData* Near = Map->FindCell(From))
	{
		Best = Higher(Best, Near->CoverOn(Forward));
	}
	if (const FRTHexCellData* Far = Map->FindCell(To))
	{
		Best = Higher(Best, Far->CoverOn(Backward));
	}
	return Best;
}

bool URTHexCoverLibrary::BlocksTraversal(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
{
	// L'OR e' RESTRITTIVO: se un bordo porta sia un muro alto sia una porta, aprire la porta non buca il muro.
	// Il muro resta il dato piu' forte, e un livello mal disegnato non produce un varco a sorpresa (la coppia
	// la segnala `ValidateMap`). Questa e' l'UNICA funzione che vista, grafo e combat interrogano.
	return CoverBetween(Map, From, To) == ERTHexCoverType::High
		|| URTHexDoorLibrary::BlocksBetween(Map, From, To);
}

namespace
{
	/**
	 * Scala l'integrita' della copertura che `Cell` dichiara sul bordo `Edge`, se c'e'. La rimuove quando
	 * arriva a zero: un riparo a zero punti struttura non e' un riparo, e CP 9.1 lo considera gia' un dato
	 * incoerente (`ValidateMap`). Aggiunge una voce a `Changes` solo se qualcosa e' cambiato davvero.
	 */
	void DamageFace(URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge, const FRTCellId& Toward,
		int32 Amount, TArray<FRTCoverDamageResult>& Changes)
	{
		const FRTHexCellData* Existing = Map->FindCell(Cell);
		if (Existing == nullptr)
		{
			return;
		}

		FRTHexCellData Updated = *Existing;
		for (int32 I = 0; I < Updated.Covers.Num(); ++I)
		{
			if (Updated.Covers[I].Edge != Edge || Updated.Covers[I].Type == ERTHexCoverType::None)
			{
				continue;
			}

			FRTCoverDamageResult Result;
			Result.Cell = Cell;
			Result.Toward = Toward;
			Result.Type = Updated.Covers[I].Type;
			Result.RemainingIntegrity = Updated.Covers[I].Integrity - Amount;
			Result.bDestroyed = Result.RemainingIntegrity <= 0;

			if (Result.bDestroyed)
			{
				Result.RemainingIntegrity = 0;
				Updated.Covers.RemoveAt(I);
			}
			else
			{
				Updated.Covers[I].Integrity = Result.RemainingIntegrity;
			}

			// Passa da AddOrUpdateCell: e' il punto che incrementa la REVISIONE, cioe' il segnale con cui
			// snapshot e cache di percorso si accorgono che il grafo non e' piu' quello di prima.
			Map->AddOrUpdateCell(Updated);
			Changes.Add(Result);
			return; // un bordo ha al massimo una copertura (invariante validata da ValidateMap)
		}
	}
}

TArray<FRTCoverDamageResult> URTHexCoverLibrary::ApplyStructureDamage(URTHexMapAsset* Map,
	const TArray<FRTStructureHit>& Hits)
{
	TArray<FRTCoverDamageResult> Changes;
	if (Map == nullptr)
	{
		return Changes;
	}

	for (const FRTStructureHit& Hit : Hits)
	{
		if (Hit.Amount <= 0)
		{
			continue;
		}

		ERTHexDirection Forward = ERTHexDirection::E;
		ERTHexDirection Backward = ERTHexDirection::E;
		if (!EdgeDirection(Hit.From, Hit.To, Forward) || !EdgeDirection(Hit.To, Hit.From, Backward))
		{
			continue; // non adiacenti: nessun bordo condiviso da danneggiare
		}

		// Le due facce sono la STESSA barriera vista dai due lati: se la mappa le dichiara entrambe, il colpo
		// le scala insieme — altrimenti un muro disegnato due volte reggerebbe il doppio.
		DamageFace(Map, Hit.From, Forward, Hit.To, Hit.Amount, Changes);
		DamageFace(Map, Hit.To, Backward, Hit.From, Hit.Amount, Changes);
	}

	// Ordine canonico: da qui escono voci di TurnLog, e due esecuzioni della stessa partita devono scriverle
	// nello stesso ordine (la stessa disciplina di TickDynamicSurfaces).
	Changes.Sort([](const FRTCoverDamageResult& A, const FRTCoverDamageResult& B)
	{
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		return static_cast<uint8>(A.Type) < static_cast<uint8>(B.Type);
	});
	return Changes;
}
