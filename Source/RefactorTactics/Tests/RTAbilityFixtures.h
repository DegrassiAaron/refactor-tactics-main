#pragma once

#include "CoreMinimal.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Unit/RTUnit.h"

/**
 * EQUIPAGGIARE UN'UNITA' DI PROVA con un'azione del catalogo generico, dichiarato una volta invece che due.
 *
 * ## Il duplicato che questo header chiude
 *
 * `AddCoreAbility` esisteva in `RTHexCombatIntegrationTests.cpp` e in `RTTurnLogCauseTests.cpp`, entrambe in
 * un namespace ANONIMO. UE compila piu' `.cpp` in un'unica translation unit e i namespace anonimi di quei
 * file diventano lo stesso namespace: due omonimi sono una ridefinizione (#1548, stessa famiglia di #1530).
 *
 * ## I due corpi DIVERGEVANO, e nessuno dei due era giusto
 *
 * | | `RTHexCombatIntegrationTests` | `RTTurnLogCauseTests` |
 * |---|---|---|
 * | `Power` | sempre `0` | dal primo effetto `Damage` |
 * | `bSelfTarget` | sempre `true` | mai toccato (resta `false`) |
 *
 * Nessuna delle due chiedeva al catalogo. E il catalogo lo sa: **tre** azioni core escono da
 * `URTCatalogLibrary` con `bSelfTarget = true` — `Action.Guard`, `Action.Brace` e `Action.Overwatch`, le tre
 * righe `Catalog.Last().bSelfTarget = true` — e tutte le altre con `false`, incluse quelle di controllo
 * (`Push`, `Pull`, `Root`, `Slow`). Nessuno dei test che usano questa fixture arma un Overwatch, ma
 * l'elenco e' quello: la fixture copia il campo, non lo indovina.
 *
 * 🔴 **`RTTurnLogCauseTests` equipaggiava Guard e Brace come azioni NON auto-bersaglio.** E' esattamente il
 * difetto contro cui la produzione mette in guardia nel proprio commento, in `RTCatalogLibrary.cpp`:
 *
 *     «il giorno che [l'equipaggiamento] ne concedesse una self-target — `Guard`, `Brace` — il gadget la
 *      porterebbe come azione d'attacco da 30 danni, che e' esattamente il difetto appena chiuso per gli
 *      eroi.»
 *
 * ⚠️ Il `bSelfTarget = true` fisso dell'altra versione era invece **morto**: i suoi tre siti di chiamata
 * usano solo `Action.Guard` (che dal catalogo lo ha gia' vero), e nessuno di essi passa da `PlanBots` —
 * `RunBlastTurn` chiama `LockInAndResolve` diretto — che e' l'unico lettore di quel campo in quei test.
 *
 * ## Percio' questa versione non inventa niente: e' l'idioma della produzione
 *
 * Le stesse righe di `URTCatalogLibrary::MakeGenericActions` e di `MakeEquipmentAction`. Un test che
 * costruisce l'azione diversamente da come la costruisce il gioco misura un oggetto che in partita non
 * esiste.
 */
namespace RTAbilityFixtures
{
	/**
	 * Da' all'unita' un'azione del catalogo generico e ne restituisce l'indice; `INDEX_NONE` se l'unita' e'
	 * nulla.
	 *
	 * ⚠️ I campi SPECCHIO — `RangeCells`, `Power`, `bSelfTarget`, `CooldownTurns` — si copiano dal `Def`
	 * perche' `URTActionData` li ha ai default legacy dell'MVP quadrato (portata 5, potenza 30, ricarica 0)
	 * e `ARTTurnManager` legge ancora quelli e non il `Def`. Senza la copia, `Action.Wait` entrerebbe nel
	 * kit come un colpo da 30 a distanza 5.
	 *
	 * 🔴 **La ricarica e' arrivata dopo, e per un difetto vero** (#1552): `MakeGenericActions` non la
	 * copiava, quindi `Action.Brace` — `Cooldown 1` nel catalogo — era riarmabile ogni turno da ogni eroe.
	 * Corretta in produzione, e qui di conseguenza: una fixture che dichiara di seguire l'idioma di
	 * produzione e ne segue una versione vecchia costruisce un oggetto che in partita non esiste piu'.
	 */
	inline int32 AddCoreAbility(ARTUnit* Unit, const TCHAR* ActionId)
	{
		if (!Unit)
		{
			return INDEX_NONE;
		}
		URTActionData* Action = NewObject<URTActionData>(Unit);
		Action->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Action->RangeCells = Action->Def.RangeCells;
		Action->bSelfTarget = Action->Def.bSelfTarget;
		Action->CooldownTurns = Action->Def.CooldownTurns;
		Action->Power = 0;
		for (const FRTActionEffectSpec& Spec : Action->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage)
			{
				Action->Power = Spec.Amount;
				break;
			}
		}
		Unit->Abilities.Add(Action);
		return Unit->Abilities.Num() - 1;
	}
}
