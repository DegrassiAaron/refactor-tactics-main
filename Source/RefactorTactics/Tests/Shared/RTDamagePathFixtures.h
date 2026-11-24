#pragma once

#include "CoreMinimal.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTHeroData.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * DA DOVE PASSA IL DANNO DI UNA RIGA: le righe che `URTCatalogLibrary::ValidateActions` deve distinguere quando rifiuta
 * elemento e perforante fuori dal Blast (#4189).
 *
 * `FRTActionDef::ElementId` e `FRTActionDef::bPiercing` viaggiano sull'intento del Blast (`CollectAttackIntents`,
 * `AppendChargeImpactIntents`) e da li' sul colpo: un danno che non nasce da un intento non li vede. Il validatore lo
 * dice con lo slot e la fase, e i due test che lo provano (`Damage.Element.RefusedWhereTheDamageSkipsTheBlast`,
 * `Damage.Piercing.RefusedWhereTheDamageSkipsTheBlast`) leggono le stesse righe da qui: una sede sola per «quali righe»,
 * che cambia il giorno in cui uno di questi percorsi impara a portare l'elemento.
 *
 * 🔑 **Righe VERE del catalogo**, non sintetiche, dove la riga esiste: il test aggiunge elemento o perforante e nient'altro,
 * quindi l'errore che compare e' quello della regola. Sintetiche sono solo le due che provano il confine dal lato opposto
 * (vedi `ThroughTheBlast`).
 */
namespace RTDamagePathFixtures
{
	/** Una riga, col nome che i messaggi del test le danno. */
	struct FRow
	{
		FString Name;
		FRTActionDef Def;
	};

	/** La riga `ActionId` del kit di `Hero`; un `Def` vuoto se manca, che il chiamante vede dall'`ActionId`. */
	inline FRTActionDef HeroRow(const URTHeroData* Hero, const TCHAR* ActionId)
	{
		if (Hero)
		{
			for (const URTActionData* Action : Hero->Actions)
			{
				if (Action && Action->Def.ActionId == FName(ActionId)) { return Action->Def; }
			}
		}
		return FRTActionDef();
	}

	/**
	 * Righe il cui danno NON nasce da un intento del Blast, una per sede:
	 *   - `Action.Counter`: una REAZIONE. Per fase sta nel Blast (`Control`), ma il suo danno entra fra i colpi come
	 *     `FRTCounterAttack` (`RunReactionPass`), senza intento: e' la riga che obbliga a guardare lo slot e non solo la fase;
	 *   - `Hero.Ivrin.InterceptShot`: la PREDITTIVA, armata nel Prep e sparata al boundary del Move (`ResolvePredictiveBoundary`);
	 *   - `Action.Overwatch`: armato nel Prep, spara nel Move (`ApplyReactionDecision`);
	 *   - `Action.SuppressiveLine`: una riga di Prep che dichiara un danno, che il Prep non applica (`ResolvePrep`);
	 *   - `Action.Electrify`: ambientale, scarica nel Cleanup come `Environmental` (`CollectElectricPropagation`).
	 */
	inline TArray<FRow> OffTheBlast()
	{
		return {
			{ TEXT("reazione (Action.Counter)"), URTCatalogLibrary::FindCoreAction(TEXT("Action.Counter")) },
			{ TEXT("predittiva (Hero.Ivrin.InterceptShot)"),
				HeroRow(URTHeroCatalogLibrary::MakeIvrin(), TEXT("Hero.Ivrin.InterceptShot")) },
			{ TEXT("Overwatch (Action.Overwatch)"), URTCatalogLibrary::FindCoreAction(TEXT("Action.Overwatch")) },
			{ TEXT("Prep (Action.SuppressiveLine)"), URTCatalogLibrary::FindCoreAction(TEXT("Action.SuppressiveLine")) },
			{ TEXT("Cleanup (Action.Electrify)"), URTCatalogLibrary::FindCoreAction(TEXT("Action.Electrify")) },
		};
	}

	/**
	 * Righe il cui danno nasce da un intento del Blast: l'attacco base (`CollectAttackIntents`) e la carica (il suo impatto,
	 * `AppendChargeImpactIntents`). Le altre due sono SINTETICHE e stanno qui per il confine: un predicato piu' largo le
	 * rifiuterebbe, e il resolver le porta nel Blast come ogni altra riga.
	 *   - una predittiva che risolve nell'`Attack`: il Prep arma solo le righe della sua fase, e `CollectAttackIntents` non
	 *     guarda `PredictiveTargeting`, quindi `PredictiveTargeting != None` da solo non e' la domanda;
	 *   - un attacco base di slot `Main` con un `ReactionTrigger`: il pass delle reazioni legge le sole righe di slot
	 *     `Reaction` (`ArmedReactionsInCanonicalOrder`), quindi nemmeno `ReactionTrigger != None` lo e'.
	 */
	inline TArray<FRow> ThroughTheBlast()
	{
		FRTActionDef PredittivaNelBlast = HeroRow(URTHeroCatalogLibrary::MakeIvrin(), TEXT("Hero.Ivrin.InterceptShot"));
		PredittivaNelBlast.ResolutionPhase = ERTResolutionPhase::Attack;
		PredittivaNelBlast.ActionPointCost = URTCatalogLibrary::DefaultActionPointCost(PredittivaNelBlast);

		FRTActionDef AttaccoConTrigger = URTCatalogLibrary::FindCoreAction(TEXT("Action.BasicAttack"));
		AttaccoConTrigger.ReactionTrigger = ERTReactionTrigger::HitByDirectAttack;

		return {
			{ TEXT("attacco base (Action.BasicAttack)"), URTCatalogLibrary::FindCoreAction(TEXT("Action.BasicAttack")) },
			{ TEXT("carica (Action.Charge)"), URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge")) },
			{ TEXT("predittiva che risolve nell'Attack"), PredittivaNelBlast },
			{ TEXT("attacco base di slot Main con un trigger"), AttaccoConTrigger },
		};
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
