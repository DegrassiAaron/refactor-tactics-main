#include "Ability/RTAbilityLab.h"

#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTHeroLab.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "Turn/RTTurnLogLibrary.h"
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

	FRTScenarioUnit Target;
	Target.Id = TEXT("TARGET");
	Target.HeroId = TargetHeroId;
	Target.TeamId = 1;
	Target.Cell = Spec.TargetCell;
	Scenario.Units.Add(Target);

	FRTScenarioIntent Intent;
	Intent.UnitId = TEXT("CASTER");

	// Uno SCATTO prende lo slot movimento e si dichiara con `Dash`/`DashCell`, non come abilita' ([D-028],
	// `#3773`). Prima finiva nel campo dell'abilita', il turno lo ignorava, e il banco mostrava Ivrin fermo su
	// `PassingBlade` con la scena superata. La fase si chiede al CATALOGO, come fa la sessione: uno scatto
	// aggiunto domani prende questa strada senza che nessuno se ne ricordi.
	// ⚠️ La cella d'arrivo e' quella della spec, una POSA: dove lo scatto si ferma davvero lo decide il resolver.
	if (URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == ERTMatchPhase::Dash)
	{
		Intent.Dash = AbilityId;
		Intent.DashCell = Spec.DashCell;
	}
	else
	{
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
