#include "Ability/Lab/RTAbilityLab.h"

#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/Lab/RTHeroLab.h"
#include "HAL/FileManager.h"
#include "Map/RTHexLibrary.h"
#include "Misc/Paths.h"
#include "Player/RTPointerInteraction.h" // la forma del bersaglio la decide il gioco (`#4072`)
#include "ScenarioHarness/Authoring/RTScenarioIndex.h"
#include "ScenarioHarness/Authoring/RTScenarioLoader.h"
#include "Turn/Log/RTTurnLogLibrary.h"
#include "UObject/Package.h"

// Namespace NOMINATO, non anonimo: la unity build condivide la translation unit, e un helper con un nome
// generico in un namespace anonimo rompe la compilazione del prossimo file che ne dichiari uno uguale.
namespace RTAbilityLabInternal
{
	/**
	 * Il prefisso degli Id che il Lab scrive: `AbilityLab.<AbilityId>`. In un posto solo, perche' adesso lo
	 * leggono in due — chi costruisce la scena e il passo del banco dentro il PIE (`#3774`) — e due copie della
	 * stessa stringa divergono al primo rename.
	 */
	static const TCHAR* const LabScenarioPrefix = TEXT("AbilityLab.");

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

	/**
	 * L'istanza GENERICA che ogni unita' riceve per `AbilityId`, o `nullptr` se l'azione non e' una generica.
	 *
	 * 🔑 **Si chiede alla stessa funzione che la da' all'unita'** — `MakeGenericActions`, che `ARTUnit` accoda
	 * al kit — invece di ricostruirla qui. Una copia costruita dal Lab divergerebbe dall'originale nei campi
	 * specchio, ed e' esattamente la divergenza di `#3473`: il readout mostrava come «letta» una portata 5 che
	 * nessuna unita' ha mai portato.
	 */
	static const URTActionData* FindGenericInstance(const FName& AbilityId)
	{
		for (const URTActionData* Action : URTCatalogLibrary::MakeGenericActions(GetTransientPackage()))
		{
			if (Action && Action->Def.ActionId == AbilityId) { return Action; }
		}
		return nullptr;
	}

	/**
	 * La FORMA dell'azione che un'unita' impugna davvero (`#4072`): `FRTActionDef` non la porta, la porta l'istanza.
	 * Per un'azione di kit e' quella del kit; per una generica quella di `MakeGenericActions`; `Single` altrimenti, come
	 * per ogni core che non ha un'istanza da cui leggerla.
	 */
	static ERTAbilityShape ShapeOf(const FName& AbilityId)
	{
		for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (!Hero) { continue; }
			for (const TObjectPtr<URTActionData>& ActionPtr : Hero->Actions)
			{
				const URTActionData* Action = ActionPtr.Get();
				if (Action && Action->Def.ActionId == AbilityId) { return Action->Shape; }
			}
		}
		const URTActionData* Generica = FindGenericInstance(AbilityId);
		return Generica ? Generica->Shape : ERTAbilityShape::Single;
	}

	/** La vita massima di un eroe del roster, o `INDEX_NONE` se non c'e'. */
	static int32 MaxHealthOf(const FName& HeroId)
	{
		for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Hero && Hero->HeroId == HeroId) { return Hero->MaxHealth; }
		}
		return INDEX_NONE;
	}

	/** Un effetto che fa bene a chi lo riceve ([D-607] (2)). Regola del banco, non di gioco. */
	static bool IsBeneficialEffect(ERTActionEffect Effect)
	{
		return Effect == ERTActionEffect::Heal || Effect == ERTActionEffect::Shield
			|| Effect == ERTActionEffect::CancelStatus || Effect == ERTActionEffect::DamageReduction;
	}

	/** Vero se `Cell` sta nella mappa del banco, un esagono di raggio `MapRadius` centrato in (0,0). */
	static bool InLabMap(const FRTCellId& Cell, int32 MapRadius)
	{
		return URTHexLibrary::HexDistance(FRTCellId(0, 0), Cell) <= MapRadius;
	}

	/**
	 * Vero se `Cell` e' a portata di chi lancia da `From`, letta come la legge il resolver: non oltre `RangeCells` (0 e'
	 * la propria cella, non «illimitata»: `ResolveCoverStructures` rifiuta per `HexDistance > RangeCells`) e non sotto
	 * `MinRangeCells` (il mortaio).
	 */
	static bool WithinRange(const FRTActionDef& Def, const FRTCellId& From, const FRTCellId& Cell)
	{
		const int32 Distanza = URTHexLibrary::HexDistance(From, Cell);
		return Distanza <= Def.RangeCells && Distanza >= Def.MinRangeCells;
	}

	/**
	 * La cella del terzo eroe di una scena con l'alleato: il simmetrico del bersaglio rispetto a chi lancia, cioe'
	 * dall'altra parte, lontano dalla cella o dall'area mirata; se esce dalla mappa, la cella accanto a chi lancia da
	 * quella parte.
	 */
	static FRTCellId OpponentCellFor(const FRTAbilityLabFixtureSpec& Spec)
	{
		const FRTCellId Simmetrico(2 * Spec.CasterCell.X - Spec.TargetCell.X, 2 * Spec.CasterCell.Y - Spec.TargetCell.Y,
			Spec.CasterCell.Layer);
		if (InLabMap(Simmetrico, Spec.MapRadius) && Simmetrico != Spec.CasterCell) { return Simmetrico; }
		ERTHexDirection VersoIlBersaglio = ERTHexDirection::E;
		URTHexLibrary::DirectionTowards(Spec.CasterCell, Spec.TargetCell, VersoIlBersaglio);
		return URTHexLibrary::Neighbor(Spec.CasterCell, URTHexLibrary::OppositeDirection(VersoIlBersaglio));
	}

	/** Un eroe del roster diverso da entrambi. `NAME_None` se il roster non ne offre un terzo. */
	static FName FirstHeroOtherThanBoth(const FName& A, const FName& B)
	{
		for (const FName& HeroId : URTHeroCatalogLibrary::GetHeroIds())
		{
			if (HeroId != A && HeroId != B) { return HeroId; }
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
		if (Def.ActionId != AbilityId) { continue; }

		// Una GENERICA ha un consumatore reale: l'istanza che ogni unita' riceve. Il valore «letto» e' il suo,
		// e si legge da lei (`#3473`).
		if (const URTActionData* Generica = RTAbilityLabInternal::FindGenericInstance(AbilityId))
		{
			return URTActionReadoutLibrary::DescribeActionParameters(Generica, OutParameters);
		}

		// Ogni altra core NON la porta nessuna unita' col suo id: vive nel catalogo come sorgente da cui i kit
		// derivano le proprie (`Hero.*`). Un valore «letto» non esiste, e lo si dice con l'esito invece di
		// mostrare quello di un oggetto costruito qui — che sarebbe il default legacy dello specchio, cioe' un
		// numero inventato (portata 5 su un'azione che ne dichiara 0).
		//
		// ⚠️ L'oggetto serve solo a leggere il `Def` con la stessa funzione degli altri rami: i suoi campi
		// specchio non si mostrano, e `CatalogOnly` e' cio' che lo dice al chiamante.
		URTActionData* SoloCatalogo = NewObject<URTActionData>(GetTransientPackage());
		SoloCatalogo->Def = Def;
		URTActionReadoutLibrary::DescribeActionParameters(SoloCatalogo, OutParameters);
		return ERTActionReadoutResult::CatalogOnly;
	}

	return ERTActionReadoutResult::UnknownAction;
}

bool URTAbilityLabLibrary::BenefitsItsRecipient(const FRTActionDef& Def)
{
	if (Def.Effects.Num() == 0) { return false; }
	for (const FRTActionEffectSpec& Effect : Def.Effects)
	{
		if (!RTAbilityLabInternal::IsBeneficialEffect(Effect.Effect)) { return false; }
	}
	return true;
}

FRTAbilityLabTargetChoice URTAbilityLabLibrary::ChooseTarget(const FRTActionDef& Def, ERTAbilityShape Shape,
	const FRTAbilityLabFixtureSpec& Spec)
{
	FRTAbilityLabTargetChoice Choice;

	// Il TIPO: forzato dalla spec, o dedotto. La forma la dice il gioco; il lato di un'unita' il banco.
	Choice.Kind = Spec.Target;
	if (Choice.Kind == ERTAbilityLabTarget::Auto)
	{
		switch (URTPointerLibrary::TargetKindForAction(Def, Def.bSelfTarget, Shape))
		{
		case ERTPointerTargetKind::None: Choice.Kind = ERTAbilityLabTarget::None; break;
		case ERTPointerTargetKind::Cell: Choice.Kind = ERTAbilityLabTarget::Cell; break;
		case ERTPointerTargetKind::Edge: Choice.Kind = ERTAbilityLabTarget::Edge; break;
		// ⚠️ `TargetKindForAction` non restituisce `Object` (lo dice la sua documentazione): se un giorno lo facesse,
		// il banco mirerebbe l'unita', che e' il comportamento di prima di `#4072`.
		// 🔴 **Una REAZIONE la provoca un nemico** (`#4073`), e i suoi effetti li riceve chi reagisce, non il bersaglio:
		// `Hero.Ivrin.Deflection` porta solo `DamageReduction`, e col solo criterio degli effetti avrebbe messo il
		// secondo eroe nella propria squadra.
		case ERTPointerTargetKind::Unit:
		case ERTPointerTargetKind::Object:
		default:
			Choice.Kind = (Def.Slot != ERTActionSlot::Reaction && BenefitsItsRecipient(Def))
				? ERTAbilityLabTarget::Ally : ERTAbilityLabTarget::Enemy;
			break;
		}
	}

	// Il lato del secondo eroe: alleato se mirato come tale, o se un'area i cui effetti fanno tutti bene lo prende.
	Choice.bSecondHeroIsAlly = Choice.Kind == ERTAbilityLabTarget::Ally
		|| (Choice.Kind == ERTAbilityLabTarget::Cell && Def.Slot != ERTActionSlot::Reaction && BenefitsItsRecipient(Def));

	ERTHexDirection VersoIlBersaglio = ERTHexDirection::E;
	const bool bHaDirezione = URTHexLibrary::DirectionTowards(Spec.CasterCell, Spec.TargetCell, VersoIlBersaglio);

	if (Choice.Kind == ERTAbilityLabTarget::Cell)
	{
		// Una cella VUOTA a portata, accanto al bersaglio sulla linea da chi lancia: prima quella oltre, cosi' un'area la
		// centra e prende il bersaglio senza prendere chi lancia; poi quella prima del bersaglio. ⚠️ Accanto al BERSAGLIO,
		// non a chi lancia: con il bersaglio sul bordo della mappa la cella oltre esce, e quella accanto a chi lancia
		// metterebbe chi lancia nell'area e il bersaglio fuori.
		Choice.Cell = Spec.TargetCell;
		if (bHaDirezione)
		{
			const FRTCellId Candidate[] = {
				URTHexLibrary::Neighbor(Spec.TargetCell, VersoIlBersaglio),
				URTHexLibrary::Neighbor(Spec.TargetCell, URTHexLibrary::OppositeDirection(VersoIlBersaglio)),
			};
			for (const FRTCellId& C : Candidate)
			{
				if (C != Spec.CasterCell && C != Spec.TargetCell && RTAbilityLabInternal::InLabMap(C, Spec.MapRadius)
					&& RTAbilityLabInternal::WithinRange(Def, Spec.CasterCell, C))
				{
					Choice.Cell = C;
					break;
				}
			}
		}
	}
	else if (Choice.Kind == ERTAbilityLabTarget::Edge)
	{
		// Il lato della cella del bersaglio rivolto a chi lancia. Oltre la portata, la cella accanto a chi lancia,
		// verso il bersaglio: `Action.Interact` ha portata 1, e la sua porta sta fra le due.
		Choice.Cell = Spec.TargetCell;
		if (bHaDirezione && !RTAbilityLabInternal::WithinRange(Def, Spec.CasterCell, Spec.TargetCell))
		{
			Choice.Cell = URTHexLibrary::Neighbor(Spec.CasterCell, VersoIlBersaglio);
		}
		ERTHexDirection VersoChiLancia = URTHexLibrary::OppositeDirection(VersoIlBersaglio);
		URTHexLibrary::DirectionTowards(Choice.Cell, Spec.CasterCell, VersoChiLancia);
		Choice.Edge = VersoChiLancia;

		if (Def.StructureOp == ERTStructureOp::SetDoorState)
		{
			// La porta si dichiara dalla parte di chi lancia, com'e' nel corpus: e' lo stesso bordo visto dall'altra
			// cella, e il resolver la trova sulla traiettoria verso la cella mirata (`FirstDoorEdge`).
			Choice.bPlacesDoor = true;
			Choice.DoorCell = URTHexLibrary::Neighbor(Choice.Cell, Choice.Edge);
			Choice.DoorEdge = URTHexLibrary::OppositeDirection(Choice.Edge);
		}
	}

	return Choice;
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
	Scenario.ScenarioId = FString(RTAbilityLabInternal::LabScenarioPrefix) + AbilityId.ToString();
	Scenario.Seed = Spec.Seed;
	Scenario.MapRadius = Spec.MapRadius;
	Scenario.Tags.Add(TEXT("ability-lab"));

	FRTScenarioUnit Caster;
	Caster.Id = TEXT("CASTER");
	Caster.HeroId = CasterHeroId;
	Caster.TeamId = 0;
	Caster.Cell = Spec.CasterCell;
	Scenario.Units.Add(Caster);

	// Il bersaglio adatto (`#4072`, [D-607] (2)). Uno scatto non ne ha uno: si dichiara con la sua cella d'arrivo.
	const bool bIsDash = URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == ERTMatchPhase::Dash;
	const FRTAbilityLabTargetChoice Choice = ChooseTarget(Def, RTAbilityLabInternal::ShapeOf(AbilityId), Spec);
	const bool bAlleato = !bIsDash && Choice.bSecondHeroIsAlly;

	// ⛔ «Nessuno» forzato su un'azione che chiede un bersaglio: l'harness la rifiuterebbe alla run (*«bersaglio '' non
	// schierato»*), dopo aver posato una scena che non poteva girare. Fail closed qui, come per un id sconosciuto.
	if (!bIsDash && Choice.Kind == ERTAbilityLabTarget::None && !Def.bSelfTarget)
	{
		OutError = FString::Printf(
			TEXT("'%s' chiede un bersaglio: forzarlo a «nessuno» darebbe una scena che l'harness rifiuta."),
			*AbilityId.ToString());
		return false;
	}

	FRTScenarioUnit Target;
	Target.Id = TEXT("TARGET");
	Target.HeroId = TargetHeroId;
	Target.TeamId = bAlleato ? 0 : 1;
	Target.Cell = Spec.TargetCell;
	// Un alleato da curare e' ferito, o la cura non si vedrebbe: meta' della sua vita. ⚠️ E' posa, non bilanciamento.
	if (bAlleato)
	{
		bool bCura = false;
		for (const FRTActionEffectSpec& Effect : Def.Effects) { bCura |= Effect.Effect == ERTActionEffect::Heal; }
		const int32 Massima = RTAbilityLabInternal::MaxHealthOf(TargetHeroId);
		if (bCura && Massima > 1) { Target.Health = Massima / 2; }
	}
	Scenario.Units.Add(Target);

	// Con l'alleato la scena avrebbe una squadra sola, e la partita finirebbe al primo Cleanup con la vittoria per
	// eliminazione: in PIE si aprirebbe la schermata del risultato. Un terzo eroe, nemico, dall'altra parte di chi
	// lancia, le da' la seconda squadra. Non agisce: e' il ruolo di `W1` in `Spec/Map/InteractOpensDoor`.
	if (bAlleato)
	{
		const FName OpponentHeroId = RTAbilityLabInternal::FirstHeroOtherThanBoth(CasterHeroId, TargetHeroId);
		if (OpponentHeroId.IsNone())
		{
			OutError = TEXT("Il roster non offre un terzo eroe da schierare contro, e la scena con l'alleato non avrebbe una seconda squadra.");
			return false;
		}
		FRTScenarioUnit Opponent;
		Opponent.Id = TEXT("OPPONENT");
		Opponent.HeroId = OpponentHeroId;
		Opponent.TeamId = 1;
		Opponent.Cell = RTAbilityLabInternal::OpponentCellFor(Spec);
		Scenario.Units.Add(Opponent);
	}

	if (!bIsDash && Choice.bPlacesDoor)
	{
		FRTScenarioDoor Door;
		Door.Cell = Choice.DoorCell;
		Door.Door.Edge = Choice.DoorEdge;
		Door.Door.State = ERTHexDoorState::Closed;
		Door.Door.DoorId = 1;
		Scenario.Doors.Add(Door);
	}

	FRTScenarioIntent Intent;
	Intent.UnitId = TEXT("CASTER");

	// Uno SCATTO prende lo slot movimento e si dichiara con `Dash`/`DashCell`, non come abilita' ([D-028],
	// `#3773`). Prima finiva nel campo dell'abilita', il turno lo ignorava, e il banco mostrava Ivrin fermo su
	// `PassingBlade` con la scena superata. La fase si chiede al CATALOGO, come fa la sessione: uno scatto
	// aggiunto domani prende questa strada senza che nessuno se ne ricordi.
	// ⚠️ La cella d'arrivo e' quella della spec, una POSA: dove lo scatto si ferma davvero lo decide il resolver.
	if (bIsDash)
	{
		Intent.Dash = AbilityId;
		Intent.DashCell = Spec.DashCell;
	}
	else
	{
		Intent.Ability = AbilityId;

		switch (Choice.Kind)
		{
		// Nessun bersaglio: e' cosi' che il formato esprime un'azione su chi la usa (`AbilityResolvesOnSelf`, `#2283`),
		// e un intent che nomina come bersaglio la propria unita' e' rifiutato di proposito — *«l'unita' bersaglia se
		// stessa»* (`#3472`).
		case ERTAbilityLabTarget::None:
			break;
		case ERTAbilityLabTarget::Cell:
			Intent.TargetCell = Choice.Cell;
			Intent.bTargetsCell = true;
			break;
		// Un bordo: la cella, e il lato che il resolver legge per una struttura (CP 9.5).
		case ERTAbilityLabTarget::Edge:
			Intent.TargetCell = Choice.Cell;
			Intent.bTargetsCell = true;
			Intent.CoverEdge = Choice.Edge;
			Intent.bHasCoverEdge = true;
			break;
		case ERTAbilityLabTarget::Enemy:
		case ERTAbilityLabTarget::Ally:
		case ERTAbilityLabTarget::Auto: // non succede: `ChooseTarget` lo risolve sempre
		default:
			Intent.Target = TEXT("TARGET");
			break;
		}
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

	// La validita' geometrica della posa (celle dentro `MapRadius`, bersaglio a portata) la decide l'harness.
	// `ChooseTarget` usa la geometria esagonale condivisa (`URTHexLibrary`) solo per SCEGLIERE dove posare la mira
	// (`#4072`); rifiutare qui una posa vorrebbe dire dare la seconda risposta che questo file esiste per non dare.
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

namespace RTAbilityLabInternal
{
	/** L'azione di una scena del Lab. `false` per ogni altro Id, compreso un `AbilityLab.` senza niente dopo. */
	static bool ParseLabScenarioId(const FString& ScenarioId, FName& OutAbilityId)
	{
		if (!ScenarioId.StartsWith(LabScenarioPrefix, ESearchCase::CaseSensitive))
		{
			return false;
		}
		const FString Resto = ScenarioId.RightChop(FCString::Strlen(LabScenarioPrefix));
		if (Resto.IsEmpty())
		{
			return false;
		}
		OutAbilityId = FName(*Resto);
		return true;
	}

	/**
	 * L'elenco in cui il passo del banco si muove (`#3774`): il kit dell'eroe che possiede l'azione, come lo
	 * mostra il pannello col filtro per eroe (`ListHeroKit`); per un'azione core, le core del catalogo. Restano
	 * le sole voci che `BuildFixture` costruisce: un tasto che portasse su una voce rifiutata fermerebbe il
	 * banco su un errore invece di passare oltre.
	 *
	 * Vuoto se `AbilityId` non e' canonica.
	 */
	static TArray<FName> StepCycleFor(const FName& AbilityId)
	{
		FRTAbilityLabEntry Voce;
		if (!URTAbilityLabLibrary::FindAbility(AbilityId, Voce))
		{
			return {};
		}

		TArray<FRTAbilityLabEntry> Candidate;
		if (!Voce.OwnerHeroId.IsNone())
		{
			Candidate = URTHeroLabLibrary::ListHeroKit(Voce.OwnerHeroId);
		}
		else
		{
			for (const FRTAbilityLabEntry& Entry : URTAbilityLabLibrary::ListCanonicalAbilities())
			{
				if (Entry.bIsCoreAction) { Candidate.Add(Entry); }
			}
		}

		TArray<FName> Ciclo;
		for (const FRTAbilityLabEntry& Entry : Candidate)
		{
			FRTTestScenario Scartata;
			FString Errore;
			if (URTAbilityLabLibrary::BuildFixture(Entry.AbilityId, FRTAbilityLabFixtureSpec(), Scartata, Errore))
			{
				Ciclo.Add(Entry.AbilityId);
			}
		}
		return Ciclo;
	}
}

bool URTAbilityLabLibrary::PlanLabStep(const FString& CurrentScenarioId, int32 Step, FName& OutAbilityId,
	int32& OutPosition, int32& OutCount)
{
	FName Corrente;
	if (!RTAbilityLabInternal::ParseLabScenarioId(CurrentScenarioId, Corrente))
	{
		return false;
	}
	const TArray<FName> Ciclo = RTAbilityLabInternal::StepCycleFor(Corrente);
	const int32 Qui = Ciclo.IndexOfByKey(Corrente);
	if (Qui == INDEX_NONE)
	{
		return false;
	}

	// Il giro agli estremi in entrambi i versi: `%` in C++ conserva il segno del dividendo, e un passo indietro
	// dalla prima voce darebbe -1 senza il secondo `+ N`.
	const int32 N = Ciclo.Num();
	const int32 Li = ((Qui + Step) % N + N) % N;
	OutAbilityId = Ciclo[Li];
	OutPosition = Li + 1;
	OutCount = N;
	return true;
}

FString URTAbilityLabLibrary::DescribeLabStep(const FString& ScenarioId)
{
	FName Corrente;
	if (!RTAbilityLabInternal::ParseLabScenarioId(ScenarioId, Corrente))
	{
		return FString();
	}
	const TArray<FName> Ciclo = RTAbilityLabInternal::StepCycleFor(Corrente);
	const int32 Qui = Ciclo.IndexOfByKey(Corrente);
	if (Qui == INDEX_NONE)
	{
		return FString();
	}
	// I tasti fra virgolette basse: una virgola e un punto nudi in mezzo a una frase non si leggono come tasti.
	return FString::Printf(TEXT("BANCO %d/%d  ·  «,» precedente  ·  «.» successiva"), Qui + 1, Ciclo.Num());
}

bool URTAbilityLabLibrary::IsLabScenarioId(const FString& ScenarioId)
{
	// La stessa lettura del passo del banco: una sola risposta a «e' una scena del Lab?».
	FName Ignorata;
	return RTAbilityLabInternal::ParseLabScenarioId(ScenarioId, Ignorata);
}

bool URTAbilityLabLibrary::WriteLabScenario(const FRTTestScenario& Scenario, FString& OutError)
{
	OutError.Reset();

	// 🔴 Radice vuota = sotto automation, senza override: non esiste e non si crea. Scrivere comunque
	// `MakeDirectory("")` + un percorso relativo sporcherebbe la cartella corrente.
	const FString Root = URTScenarioLoader::LabScenariosRoot();
	if (Root.IsEmpty())
	{
		OutError = TEXT("la radice del Lab non e' disponibile sotto automation senza override: niente da scrivere");
		return false;
	}
	IFileManager::Get().MakeDirectory(*Root, /*Tree=*/ true);
	const FString Percorso = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(Root, Scenario.ScenarioId + TEXT(".json")));

	// `SaveToFile` valida prima di toccare il disco: una fixture invalida non lascia un file a meta'.
	if (!URTScenarioLoader::SaveToFile(Scenario, Percorso, OutError))
	{
		return false;
	}

	// 🔑 Lanciabile = l'indice risolve l'Id a QUESTO file. Un doppione altrove rende l'Id ambiguo, e il
	// GameMode lo rifiuterebbe a schermo senza che chi scrive potesse dirlo prima.
	FString ErroreIndice;
	const FString Risolto = URTScenarioIndex::ResolvePath(Scenario.ScenarioId, ErroreIndice);
	if (Risolto.IsEmpty())
	{
		// Un file che rende ambiguo un Id avvelenerebbe la console e il GameMode a ogni clic: si toglie.
		IFileManager::Get().Delete(*Percorso);
		OutError = FString::Printf(TEXT("fixture scritta in '%s' ma non lanciabile: %s; il file appena scritto e' stato rimosso"),
			*Percorso, *ErroreIndice);
		return false;
	}
	if (!FPaths::IsSamePath(Risolto, Percorso))
	{
		// Stessa ragione del ramo sopra: un file che non e' quello a cui l'Id risolve non deve restare.
		IFileManager::Get().Delete(*Percorso);
		OutError = FString::Printf(TEXT("'%s' risolve a '%s', non al file appena scritto '%s'; il file appena scritto e' stato rimosso"),
			*Scenario.ScenarioId, *Risolto, *Percorso);
		return false;
	}
	return true;
}
