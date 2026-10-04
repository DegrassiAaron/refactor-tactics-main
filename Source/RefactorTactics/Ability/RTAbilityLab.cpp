#include "Ability/RTAbilityLab.h"

#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Turn/RTTurnLogLibrary.h"
#include "UObject/Package.h"

// Namespace NOMINATO, non anonimo: la unity build condivide la translation unit, e un helper con un nome
// generico in un namespace anonimo rompe la compilazione del prossimo file che ne dichiari uno uguale.
namespace RTAbilityLabInternal
{
	/**
	 * L'azione canonica di `AbilityId`, cercata prima nei kit d'eroe e poi nel catalogo core.
	 *
	 * L'ordine non e' arbitrario: se un `ActionId` esistesse in entrambi, il kit vince, perche' e' quello
	 * che l'eroe porta davvero in partita.
	 */
	static bool FindCanonicalDef(const FName& AbilityId, FRTActionDef& OutDef, FName& OutOwnerHeroId,
		bool& bOutIsCore)
	{
		for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (!Hero) { continue; }
			for (const TObjectPtr<URTActionData>& ActionPtr : Hero->Actions)
			{
				const URTActionData* Action = ActionPtr.Get();
				if (Action && Action->Def.ActionId == AbilityId)
				{
					OutDef = Action->Def;
					OutOwnerHeroId = Hero->HeroId;
					bOutIsCore = false;
					return true;
				}
			}
		}

		for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
		{
			if (Def.ActionId == AbilityId)
			{
				OutDef = Def;
				OutOwnerHeroId = NAME_None;
				bOutIsCore = true;
				return true;
			}
		}

		return false;
	}

	/** Un eroe del roster diverso da `Excluded`. `NAME_None` solo se il roster non ne offre nessun altro. */
	static FName FirstHeroOtherThan(const FName& Excluded)
	{
		for (const FName& HeroId : URTHeroCatalogLibrary::GetHeroIds())
		{
			if (HeroId != Excluded) { return HeroId; }
		}
		return NAME_None;
	}
}

TArray<FRTAbilityLabEntry> URTAbilityLabLibrary::ListCanonicalAbilities()
{
	TArray<FRTAbilityLabEntry> Out;

	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		for (const TObjectPtr<URTActionData>& ActionPtr : Hero->Actions)
		{
			const URTActionData* Action = ActionPtr.Get();
			// Un'azione senza `ActionId` non e' indirizzabile: il selettore non puo' offrirla, perche'
			// nessuno potrebbe poi eseguirla per nome. Non e' un difetto del Lab — e' il dato dell'MVP
			// quadrato che vive ancora nei campi specchio.
			if (!Action || Action->Def.ActionId.IsNone()) { continue; }

			FRTAbilityLabEntry Entry;
			Entry.AbilityId = Action->Def.ActionId;
			Entry.DisplayName = Action->DisplayName;
			Entry.OwnerHeroId = Hero->HeroId;
			Entry.bIsCoreAction = false;
			Entry.Shape = Action->Shape;
			Entry.RangeCells = Action->RangeCells;
			Entry.AreaRadius = Action->AreaRadius;
			Out.Add(Entry);
		}
	}

	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		if (Def.ActionId.IsNone()) { continue; }

		FRTAbilityLabEntry Entry;
		Entry.AbilityId = Def.ActionId;
		Entry.DisplayName = FText::FromName(Def.ActionId);
		Entry.OwnerHeroId = NAME_None;
		Entry.bIsCoreAction = true;
		// `Shape` e `AreaRadius` vivono sui campi SPECCHIO di `URTActionData`, non su `FRTActionDef`:
		// un'azione core letta dal catalogo non li porta, e inventarli qui sarebbe una quarta casa.
		Entry.Shape = ERTAbilityShape::Single;
		Entry.RangeCells = Def.RangeCells;
		Entry.AreaRadius = 0;
		Out.Add(Entry);
	}

	return Out;
}

bool URTAbilityLabLibrary::FindAbility(const FName& AbilityId, FRTAbilityLabEntry& OutEntry)
{
	for (const FRTAbilityLabEntry& Entry : ListCanonicalAbilities())
	{
		if (Entry.AbilityId == AbilityId)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}

ERTActionReadoutResult URTAbilityLabLibrary::DescribeAbility(const FName& AbilityId,
	TArray<FRTActionParameterView>& OutParameters)
{
	OutParameters.Reset();

	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		for (const TObjectPtr<URTActionData>& ActionPtr : Hero->Actions)
		{
			const URTActionData* Action = ActionPtr.Get();
			if (Action && Action->Def.ActionId == AbilityId)
			{
				return URTActionReadoutLibrary::DescribeActionParameters(Action, OutParameters);
			}
		}
	}

	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		if (Def.ActionId == AbilityId)
		{
			// Il readout vuole un `URTActionData`, e un'azione core vive come `FRTActionDef` nuda. Questo
			// oggetto e' transitorio e non entra da nessuna parte: porta il `Def` del catalogo e nient'altro,
			// quindi non e' una seconda definizione dell'azione.
			URTActionData* Materialized = NewObject<URTActionData>(GetTransientPackage());
			Materialized->Def = Def;
			return URTActionReadoutLibrary::DescribeActionParameters(Materialized, OutParameters);
		}
	}

	return ERTActionReadoutResult::UnknownAction;
}

bool URTAbilityLabLibrary::BuildFixture(const FName& AbilityId, const FRTAbilityLabFixtureSpec& Spec,
	FRTTestScenario& OutScenario, FString& OutError)
{
	FRTActionDef Def;
	FName OwnerHeroId = NAME_None;
	bool bIsCore = false;

	if (!RTAbilityLabInternal::FindCanonicalDef(AbilityId, Def, OwnerHeroId, bIsCore))
	{
		OutError = FString::Printf(
			TEXT("AbilityId non canonica: '%s'. Non e' nel kit di nessun eroe del roster ne' nel catalogo core."),
			*AbilityId.ToString());
		// `OutScenario` resta com'era: fail closed. Una fixture a meta' verrebbe eseguita, e il suo esito
		// sarebbe indistinguibile da quello di un'ability che semplicemente non fa nulla.
		return false;
	}

	// Un'azione core si esegue solo se un'unita' la IMPUGNA — `#3472`. Un'unita' porta il proprio kit piu'
	// le generiche che `MakeGenericActions` le accoda, e `FindCanonicalDef` cerca prima nei kit: se siamo nel
	// ramo core, nessun kit porta questo id, quindi l'unica via e' che sia una generica. Le altre core vivono
	// nel catalogo come sorgente da cui i kit derivano le proprie azioni `Hero.*`.
	//
	// ⛔ Prima la si costruiva lo stesso, e l'harness la rifiutava in ERROR con «'CASTER' non possiede
	// l'abilita'»: vero, ma detto da chi esegue invece che da chi offre, e dopo aver posato una fixture che non
	// poteva girare. Il rifiuto sta qui, fail closed come per un id sconosciuto, e dice quali core girano.
	if (bIsCore && !URTCatalogLibrary::GetGenericActionIds().Contains(AbilityId))
	{
		TArray<FString> Generiche;
		for (const FName& Id : URTCatalogLibrary::GetGenericActionIds())
		{
			Generiche.Add(Id.ToString());
		}
		OutError = FString::Printf(
			TEXT("'%s' e' un'azione core che nessuna unita' impugna: un'unita' porta il proprio kit e le azioni "
				 "generiche (%s). Il Lab ne legge i parametri, ma non la puo' eseguire."),
			*AbilityId.ToString(), *FString::Join(Generiche, TEXT(" · ")));
		return false;
	}

	// Una generica non appartiene a un eroe in particolare, perche' la portano tutti: la impugna il primo del
	// roster. E' una scelta di POSA, non di bilanciamento.
	const FName CasterHeroId = bIsCore
		? RTAbilityLabInternal::FirstHeroOtherThan(NAME_None)
		: OwnerHeroId;

	if (CasterHeroId.IsNone())
	{
		OutError = TEXT("Roster vuoto: nessun eroe puo' impugnare l'ability.");
		return false;
	}

	// Due unita' della stessa identita' in due squadre sono leggibili per il motore e confondenti per chi
	// guarda: si sceglie un altro bersaglio invece di fallire, perche' il default della spec non e' una
	// dichiarazione del chiamante.
	FName TargetHeroId = Spec.TargetHeroId;
	if (TargetHeroId.IsNone() || TargetHeroId == CasterHeroId)
	{
		TargetHeroId = RTAbilityLabInternal::FirstHeroOtherThan(CasterHeroId);
	}
	if (TargetHeroId.IsNone())
	{
		OutError = TEXT("Il roster non offre un secondo eroe da usare come bersaglio.");
		return false;
	}

	FRTTestScenario Scenario;
	Scenario.ScenarioId = FString::Printf(TEXT("AbilityLab.%s"), *AbilityId.ToString());
	Scenario.Seed = Spec.Seed;
	Scenario.MapRadius = Spec.MapRadius;
	Scenario.Tags.Add(TEXT("ability-lab"));

	FRTScenarioUnit Caster;
	Caster.Id = TEXT("CASTER");
	Caster.HeroId = CasterHeroId;
	Caster.TeamId = 0;
	Caster.Cell = Spec.CasterCell;
	Scenario.Units.Add(Caster);

	FRTScenarioUnit Target;
	Target.Id = TEXT("TARGET");
	Target.HeroId = TargetHeroId;
	Target.TeamId = 1;
	Target.Cell = Spec.TargetCell;
	Scenario.Units.Add(Target);

	FRTScenarioIntent Intent;
	Intent.UnitId = TEXT("CASTER");
	Intent.Ability = AbilityId;

	// Un'abilita' che si applica a chi la usa (`bSelfTarget`) resta SENZA bersaglio: e' cosi' che il formato
	// la esprime (`AbilityResolvesOnSelf`, `#2283`, che legge lo stesso flag), e un intent che nomina come
	// bersaglio la propria unita' e' rifiutato di proposito — *«l'unita' bersaglia se stessa»*. Scriverci
	// `CASTER` mandava in ERROR ogni azione `bSelfTarget` offerta dal Lab, generiche e di kit (`#3472`).
	if (Def.bSelfTarget)
	{
		// nessun bersaglio
	}
	else if (Def.bCreatesSurface || Def.StructureOp != ERTStructureOp::None)
	{
		// Le azioni che POSANO qualcosa non colpiscono un'unita': indirizzano una cella. Il Lab non sceglie
		// quale bordo o quale faccia — quello e' authoring, e non e' di questa fetta.
		Intent.TargetCell = Spec.TargetCell;
		Intent.bTargetsCell = true;
	}
	else
	{
		Intent.Target = TEXT("TARGET");
	}

	FRTScenarioTurn Turn;
	Turn.Intents.Add(Intent);
	Scenario.Turns.Add(Turn);

	// L'harness RIFIUTA uno scenario senza assertion, e ha ragione: passerebbe sempre. Questa dice che il
	// turno e' stato giocato, e non pretende un esito di gameplay — deciderlo qui sarebbe il Lab che
	// risponde al posto del resolver.
	FRTTestExpectation Played;
	Played.Kind = ERTAssertionKind::TurnsCompleted;
	Played.Value = 1;
	Scenario.Expect.Add(Played);

	// La validita' geometrica della posa (celle dentro `MapRadius`, bersaglio a portata) la decide
	// l'harness. Ricontrollarla qui vorrebbe dire portare aritmetica esagonale dentro il Lab, cioe'
	// esattamente la seconda risposta che questo file esiste per non dare.
	OutScenario = Scenario;
	return true;
}

TArray<FString> URTAbilityLabLibrary::DescribeRunTurnLog(const FRTTestResult& Result)
{
	TArray<FString> Lines;

	for (const FRTTurnTrace& Trace : Result.TurnTraces)
	{
		TArray<FRTTurnLogEntry> Entries;
		if (!URTTurnLogLibrary::DeserializeTurnLog(Trace.Bytes, Entries))
		{
			// Fail closed e VISIBILE: una traccia illeggibile che sparisse in silenzio farebbe sembrare
			// «nessun evento» cio' che e' «non sono riuscito a leggerli».
			Lines.Add(TEXT("<traccia non deserializzabile>"));
			continue;
		}
		Lines.Append(URTTurnLogLibrary::DescribeTurnLog(Entries));
	}

	return Lines;
}
