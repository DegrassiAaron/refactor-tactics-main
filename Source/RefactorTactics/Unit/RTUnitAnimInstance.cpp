#include "Unit/RTUnitAnimInstance.h"

#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Unit/RTUnit.h"

namespace
{
	/**
	 * Il default del roster, dai pack Paragon.
	 *
	 * ⚠️ **I nomi sono MISURATI sul disco**, non dedotti: §AS.3b della guida animazioni li ha contati, e
	 * **sei caselle su venti** divergono da quelle di Gideon. Le tre che si vedono qui sono la corsa di
	 * Gadget (`Run_Fwd`, non `Jog_Fwd`) e l'idle di Wraith (`Idle_NonCombat`).
	 */
	FString ClipPath(const TCHAR* Pack, const TCHAR* Clip)
	{
		return FString::Printf(
			TEXT("/Game/FabAsset/Paragon/Paragon%s/Characters/Heroes/%s/Animations/%s.%s"),
			Pack, Pack, Clip, Clip);
	}

	/**
	 * Un ruolo con una sola variante, gia' attiva.
	 *
	 * `RTVarianteRoster` e' l'id riservato ai default del C++. E' lo stesso su ogni eroe e su ogni ruolo,
	 * e va bene: l'unicita' richiesta e' dentro `(eroe, ruolo)`, e qui dentro ce n'e' una sola.
	 */
	FRTAnimRoleClips MakeRuolo(const TCHAR* Pack, const TCHAR* Clip)
	{
		FRTAnimRoleClips Ruolo;
		Ruolo.AddVariant(
			RTVarianteRoster(),
			FName(FString::Chr(RTPrimaEtichettaNeutra)),
			TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(ClipPath(Pack, Clip))));
		Ruolo.MakeActive(RTVarianteRoster());
		return Ruolo;
	}

	/** Le due clip di locomozione di un eroe del roster, ciascuna attiva nel proprio ruolo. */
	FRTHeroPresentationClips MakeClips(const TCHAR* Pack, const TCHAR* Idle, const TCHAR* Move)
	{
		FRTHeroPresentationClips Clips;
		Clips.PerRole.Add(ERTPresentationRole::Idle, MakeRuolo(Pack, Idle));
		Clips.PerRole.Add(ERTPresentationRole::Move, MakeRuolo(Pack, Move));
		return Clips;
	}
}

const FRTAnimVariant* FRTAnimRoleClips::FindVariant(const FName& VariantId) const
{
	if (VariantId.IsNone())
	{
		return nullptr;
	}
	return Variants.FindByPredicate(
		[&VariantId](const FRTAnimVariant& V) { return V.VariantId == VariantId; });
}

const FRTAnimVariant* FRTAnimRoleClips::FindActive() const
{
	// Passa da `FindVariant`, e non da un indice memorizzato, perche' un `ActiveClipVariant` che nomina
	// una variante rimossa deve leggersi come «nessuna attiva» invece che come un accesso fuori range.
	return FindVariant(ActiveClipVariant);
}

FName FRTAnimRoleClips::AddVariant(
	const FName& VariantId, const FName& Label, const TSoftObjectPtr<UAnimSequenceBase>& Clip)
{
	FRTAnimVariant Nuova;
	Nuova.VariantId = VariantId;
	Nuova.Label = Label.IsNone() ? PrimaEtichettaNeutraLibera() : Label;
	Nuova.Clip = Clip;
	Variants.Add(MoveTemp(Nuova));

	// ⛔ Nessun tocco a `ActiveClipVariant`. La variante nuova entra INATTIVA anche quando e' la prima:
	// «e' l'unica, quindi sara' lei» e' esattamente la deduzione che l'autore non ha chiesto.
	return VariantId;
}

bool FRTAnimRoleClips::MakeActive(const FName& VariantId)
{
	if (FindVariant(VariantId) == nullptr)
	{
		// 🔑 Si esce PRIMA di scrivere. Un `Make Active` su un id inesistente che azzerasse
		// `ActiveClipVariant` sarebbe una disattivazione travestita da errore.
		return false;
	}

	// L'atomicita' e' qui, ed e' banale solo perche' lo stato e' UNO: assegnare la nuova attiva
	// disattiva la precedente nello stesso passo. Due booleani per variante avrebbero avuto uno stato
	// intermedio con due attive, e qualcuno lo avrebbe letto.
	ActiveClipVariant = VariantId;
	return true;
}

bool FRTAnimRoleClips::RemoveVariant(const FName& VariantId)
{
	const int32 Indice = Variants.IndexOfByPredicate(
		[&VariantId](const FRTAnimVariant& V) { return V.VariantId == VariantId; });
	if (Indice == INDEX_NONE)
	{
		return false;
	}

	const bool EraAttiva = (ActiveClipVariant == VariantId);
	Variants.RemoveAt(Indice);

	if (EraAttiva)
	{
		// ⚠️ `NAME_None`, e NON la prima rimasta. Eleggere una sostituta toglierebbe all'autore una
		// scelta che e' sua, senza dirglielo: il ruolo torna in posa di riferimento e si vede.
		ActiveClipVariant = NAME_None;
	}
	return true;
}

FName FRTAnimRoleClips::PrimaEtichettaNeutraLibera() const
{
	for (int32 i = 0; i < RTNumEtichetteNeutre; ++i)
	{
		const FName Candidata(FString::Chr(static_cast<TCHAR>(RTPrimaEtichettaNeutra + i)));
		// Si cerca il primo LIBERO, non il primo dopo l'ultimo: con `A` e `C` presenti la risposta e'
		// `B`. Contare le varianti darebbe `C`, che e' gia' presa.
		const bool Presa = Variants.ContainsByPredicate(
			[&Candidata](const FRTAnimVariant& V) { return V.Label == Candidata; });
		if (!Presa)
		{
			return Candidata;
		}
	}
	return NAME_None;
}

const FRTAnimRoleClips* FRTHeroPresentationClips::FindRole(ERTPresentationRole Role) const
{
	return PerRole.Find(Role);
}

URTUnitAnimInstance::URTUnitAnimInstance()
{
	ClipsPerHero.Add(FName(TEXT("Hero.Gadget")), MakeClips(TEXT("Gadget"), TEXT("Idle"), TEXT("Run_Fwd")));
	ClipsPerHero.Add(FName(TEXT("Hero.Phase")), MakeClips(TEXT("Phase"), TEXT("Idle"), TEXT("Jog_Fwd")));
	ClipsPerHero.Add(FName(TEXT("Hero.Riktor")), MakeClips(TEXT("Riktor"), TEXT("Idle"), TEXT("Jog_Fwd")));
	ClipsPerHero.Add(FName(TEXT("Hero.Wraith")), MakeClips(TEXT("Wraith"), TEXT("Idle_NonCombat"), TEXT("Jog_Fwd")));
}

TSoftObjectPtr<UAnimSequenceBase> URTUnitAnimInstance::ActiveClipFor(
	const FName& HeroId, ERTPresentationRole Role) const
{
	const FRTHeroPresentationClips* Eroe = FindClipsFor(HeroId);
	if (Eroe == nullptr)
	{
		return nullptr;
	}
	const FRTAnimRoleClips* Ruolo = Eroe->FindRole(Role);
	if (Ruolo == nullptr)
	{
		return nullptr;
	}
	const FRTAnimVariant* Attiva = Ruolo->FindActive();
	return Attiva ? Attiva->Clip : TSoftObjectPtr<UAnimSequenceBase>(nullptr);
}

FAnimInstanceProxy* URTUnitAnimInstance::CreateAnimInstanceProxy()
{
	return new FRTUnitAnimProxy(this);
}

void FRTUnitAnimProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);

	// Il grafo, montato a mano: Idle e Run entrano nel blend, il blend entra nello slot, lo slot e' la
	// radice. Senza questi `SetLinkNode` i `FPoseLink` restano scollegati — nell'AnimBlueprint li
	// risolve il compilatore, qui non c'e' nessun compilatore.
	Blend.A.SetLinkNode(&IdleNode);
	Blend.B.SetLinkNode(&RunNode);
	Slot.Source.SetLinkNode(&Blend);

	// Lo slot di default: e' quello che `PlayAnimMontage` usa quando non gliene si passa un altro, ed e'
	// il punto in cui i montaggi `Cast`/`Hit`/`Death` entreranno **in override** su idle e corsa.
	Slot.SlotName = FAnimSlotGroup::DefaultSlotName;

	// `bLoopAnimation` e' protected sulla `_Standalone`: si passa dal setter, che e' il modo previsto.
	IdleNode.SetLoopAnimation(true);
	RunNode.SetLoopAnimation(true);
	Blend.Alpha = 0.f; // si parte fermi

	// Le clip dell'eroe di QUESTA unita'. `HeroId` sta sull'attore, non sull'AnimInstance: e' il dato
	// che il GameMode ha gia' scritto quando ha allestito la partita.
	const URTUnitAnimInstance* Owner = Cast<URTUnitAnimInstance>(InAnimInstance);
	if (Owner == nullptr)
	{
		return;
	}

	// ⚠️ `GetOwningActor`, non `TryGetPawnOwner`: `ARTUnit` deriva da `AActor` e **non** da `APawn`.
	// `TryGetPawnOwner` e' il nodo che ogni tutorial usa, qui restituisce null, e la macchina resterebbe
	// ferma senza un errore, senza un warning e senza un log. E' lo stesso scoglio che la guida
	// animazioni scrive in chiaro per chi lavora in Blueprint.
	const ARTUnit* Unit = Cast<ARTUnit>(InAnimInstance->GetOwningActor());
	if (Unit == nullptr)
	{
		return;
	}

	// ⚠️ **DUE ruoli, non nove.** `ERTPresentationRole` ne nomina nove perche' servono all'authoring, ma
	// questo grafo ha due sequence player e legge solo `Idle` e `Move`. Gli altri sette non hanno ancora
	// un consumatore a runtime: `Attack`/`Hit`/`Death` passano dai `BlueprintImplementableEvent` di
	// `ARTUnit`, gli altri quattro da niente.
	//
	// `ActiveClipFor` copre da solo le tre vie che danno «nessuna clip» — eroe fuori catalogo, ruolo non
	// popolato, nessuna variante attiva — e nessuna delle tre e' un errore.
	//
	// `LoadSynchronous` su un soft pointer che non risolve restituisce `nullptr` senza rumore: e' il
	// caso di chi ha clonato il repository senza i pack Paragon, che sono gitignorati. In tutti questi
	// casi il nodo resta senza sequenza e l'unita' in posa di riferimento: la partita si gioca uguale.
	IdleNode.SetSequence(Owner->ActiveClipFor(Unit->HeroId, ERTPresentationRole::Idle).LoadSynchronous());
	RunNode.SetSequence(Owner->ActiveClipFor(Unit->HeroId, ERTPresentationRole::Move).LoadSynchronous());
}

void FRTUnitAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	// 🔴 **Qui, e non in `Update`.** `PreUpdate` gira sul GAME THREAD, dove leggere lo stato di un attore
	// e' lecito; `Update_AnyThread` puo' girare su un worker, e toccare l'unita' da li' sarebbe una
	// lettura in corsa. Si copia il valore adesso e il grafo lavora sulla copia.
	const ARTUnit* Unit = InAnimInstance ? Cast<ARTUnit>(InAnimInstance->GetOwningActor()) : nullptr;
	Blend.Alpha = (Unit != nullptr && Unit->bIsMovingVisually) ? 1.f : 0.f;
}

void FRTUnitAnimProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	Slot.Update_AnyThread(InContext);
}

bool FRTUnitAnimProxy::Evaluate(FPoseContext& Output)
{
	Slot.Evaluate_AnyThread(Output);
	return true;
}
