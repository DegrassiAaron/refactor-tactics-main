#include "Unit/RTUnitAnimInstance.h"

#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Unit/RTUnit.h"

#include <initializer_list>

namespace
{
	/**
	 * Il default del roster, dai pack Paragon.
	 *
	 * ⚠️ **I nomi sono MISURATI sul disco**, non dedotti: §AS.3b della guida animazioni li ha contati, e
	 * **sei caselle su venti** divergono da quelle di Gideon. Le tre che si vedono qui sono la corsa di
	 * Aevik (`Run_Fwd`, non `Jog_Fwd`) e l'idle di Ivrin (`Idle_NonCombat`).
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

	/**
	 * I ruoli di un eroe del roster, ciascuno con la sua clip attiva.
	 *
	 * 🔴 **La clip dei pack che si CHIAMA `Cast` riempie DUE ruoli, e non e' un errore** (#2450, #3549). Sul
	 * ruolo `Attack` e' il colpo; sul ruolo `Cast` e' il gesto di attivazione di `AbilityActivated`. Sul RUOLO
	 * cast e colpo suonano la stessa sequenza in due momenti diversi (spec «il momento» D2).
	 *
	 * 🔑 **Da #3563 sopra il ruolo ci sono le clip per ABILITA'** (`MakeActionClips`, qui sotto): il ruolo resta
	 * il RIPIEGO, e una clip diversa per un'abilita' si scrive li', non cambiando questo ruolo.
	 *
	 * ⚠️ I nomi si MISURANO: §AS.3b li ha letti sul disco, e **quattro caselle su dodici** fra i tre
	 * ruoli discreti non si chiamano come ci si aspetta.
	 */
	FRTHeroPresentationClips MakeClips(const TCHAR* Pack, const TCHAR* Idle, const TCHAR* Move,
		const TCHAR* Attack, const TCHAR* Hit, const TCHAR* Death)
	{
		FRTHeroPresentationClips Clips;
		Clips.PerRole.Add(ERTPresentationRole::Idle, MakeRuolo(Pack, Idle));
		Clips.PerRole.Add(ERTPresentationRole::Move, MakeRuolo(Pack, Move));
		Clips.PerRole.Add(ERTPresentationRole::Attack, MakeRuolo(Pack, Attack));
		// La STESSA clip del ruolo `Attack`: vedi il commento qui sopra.
		Clips.PerRole.Add(ERTPresentationRole::Cast, MakeRuolo(Pack, Attack));
		Clips.PerRole.Add(ERTPresentationRole::Hit, MakeRuolo(Pack, Hit));
		Clips.PerRole.Add(ERTPresentationRole::Death, MakeRuolo(Pack, Death));
		return Clips;
	}

	/** Una voce della mappa abilita'→clip: quale azione, su quale beat, con quale clip del pack. */
	struct FRTVoceClipAzione
	{
		const TCHAR* ActionId;
		ERTPresentationRole Role;
		const TCHAR* Clip;
	};

	/**
	 * Le clip per AZIONE di un eroe del roster (#3563, spec «la clip per abilita'» §2.4, D4).
	 *
	 * 🔑 **Una riga per (abilita', beat)**, ciascuna una variante `AV_Roster` gia' attiva — la stessa forma di
	 * `MakeRuolo`. Solo `Cast` e `Attack` (D3): gli altri ruoli non conoscono l'azione, e una voce li' non suonerebbe.
	 *
	 * ⚠️ **La mappa e' un giudizio dell'autore, scelto dai NOMI** (spec §2.6, approvata il 2026-10-07): la seduta
	 * `PIE-CLIP-ABILITA` puo' cambiarne ogni riga. Chi la cambia cambia anche la seconda copia dichiarata,
	 * `ClipAtteseDefault` in `Tests/RTAnimChannelTests.cpp`. Ogni nome e' stato MISURATO sul disco prima di
	 * entrare qui (spec §2.6, che porta il comando di misura): i nomi non si deducono.
	 */
	TMap<FName, FRTActionPresentationClips> MakeActionClips(const TCHAR* Pack, std::initializer_list<FRTVoceClipAzione> Voci)
	{
		TMap<FName, FRTActionPresentationClips> PerAzione;
		for (const FRTVoceClipAzione& Voce : Voci)
		{
			PerAzione.FindOrAdd(FName(Voce.ActionId)).PerRole.Add(Voce.Role, MakeRuolo(Pack, Voce.Clip));
		}
		return PerAzione;
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
	// ⚠️ Il riferimento restituito da `Add` si usa SUBITO: l'`Add` dell'eroe successivo puo' riallocare la mappa.
	ClipsPerHero.Add(FName(TEXT("Hero.Aevik")), MakeClips(TEXT("Gadget"), TEXT("Idle"), TEXT("Run_Fwd"),
		TEXT("Cast"), TEXT("Hitreact_Fwd"), TEXT("Death_Fwd"))).PerAction = MakeActionClips(TEXT("Gadget"), {
		{ TEXT("Hero.Aevik.ArcPulse"),        ERTPresentationRole::Attack, TEXT("LMB_Fire_A") },
		{ TEXT("Hero.Aevik.LinearDischarge"), ERTPresentationRole::Cast,   TEXT("Ability_Q_Target") },
		{ TEXT("Hero.Aevik.LinearDischarge"), ERTPresentationRole::Attack, TEXT("LMB_Fire_B") },
		{ TEXT("Hero.Aevik.Overload"),        ERTPresentationRole::Cast,   TEXT("Throw_Ready") },
		{ TEXT("Hero.Aevik.Overload"),        ERTPresentationRole::Attack, TEXT("LMB_Fire_C") },
	});
	ClipsPerHero.Add(FName(TEXT("Hero.Muiren")), MakeClips(TEXT("Phase"), TEXT("Idle"), TEXT("Jog_Fwd"),
		TEXT("Cast"), TEXT("HitReact_Fwd"), TEXT("Death"))).PerAction = MakeActionClips(TEXT("Phase"), {
		{ TEXT("Hero.Muiren.PressureJet"),  ERTPresentationRole::Attack, TEXT("Primary_Attack_A_Medium") },
		{ TEXT("Hero.Muiren.CircularTide"), ERTPresentationRole::Cast,   TEXT("R_Ability_Intro") },
		{ TEXT("Hero.Muiren.FluidTrail"),   ERTPresentationRole::Cast,   TEXT("Ability_E") },
		{ TEXT("Hero.Muiren.TideGuard"),    ERTPresentationRole::Cast,   TEXT("Ability_R_Alt") },
	});
	ClipsPerHero.Add(FName(TEXT("Hero.Branth")), MakeClips(TEXT("Riktor"), TEXT("Idle"), TEXT("Jog_Fwd"),
		TEXT("Cast"), TEXT("HitReact_Front"), TEXT("Death_Fwd"))).PerAction = MakeActionClips(TEXT("Riktor"), {
		{ TEXT("Hero.Branth.ImpactShot"),   ERTPresentationRole::Attack, TEXT("PrimaryAttack_A_Slow") },
		{ TEXT("Hero.Branth.KineticPanel"), ERTPresentationRole::Cast,   TEXT("Ability_Lockdown") },
		{ TEXT("Hero.Branth.Reconfigure"),  ERTPresentationRole::Cast,   TEXT("Ability_Hook_Pull") },
		// L'impatto di una carica porta l'ActionId dello SCATTO (`Impact.Def = Dash->Def`): Ram ha un beat Attack.
		{ TEXT("Hero.Branth.Ram"),          ERTPresentationRole::Cast,   TEXT("Ability_Hook_Start") },
		{ TEXT("Hero.Branth.Ram"),          ERTPresentationRole::Attack, TEXT("Ability_ShockingPunch") },
		{ TEXT("Hero.Branth.MortarShot"),   ERTPresentationRole::Cast,   TEXT("Ability_Hook_Cast") },
		{ TEXT("Hero.Branth.MortarShot"),   ERTPresentationRole::Attack, TEXT("PrimaryAttack_B_Slow") },
	});
	ClipsPerHero.Add(FName(TEXT("Hero.Ivrin")), MakeClips(TEXT("Wraith"), TEXT("Idle_NonCombat"), TEXT("Jog_Fwd"),
		TEXT("Cast"), TEXT("HitReact_Front"), TEXT("Death_Forward"))).PerAction = MakeActionClips(TEXT("Wraith"), {
		{ TEXT("Hero.Ivrin.PulseShot"),     ERTPresentationRole::Attack, TEXT("Fire_A_Fast_V1") },
		// Il colpo predittivo non emette un `Attack` (`RTTurnManager.cpp:7133-7136`): solo il beat Cast.
		{ TEXT("Hero.Ivrin.InterceptShot"), ERTPresentationRole::Cast,   TEXT("Ability_E_Targeting_Start") },
		{ TEXT("Hero.Ivrin.PassingBlade"),  ERTPresentationRole::Cast,   TEXT("Ability_R_InMotion") },
		{ TEXT("Hero.Ivrin.PassingBlade"),  ERTPresentationRole::Attack, TEXT("Ability_Q_Fire_Fwd") },
		{ TEXT("Hero.Ivrin.Feint"),         ERTPresentationRole::Cast,   TEXT("Ability_E") },
		{ TEXT("Hero.Ivrin.PhaseGuard"),    ERTPresentationRole::Cast,   TEXT("Ability_RMB_Start") },
	});
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

TSoftObjectPtr<UAnimSequenceBase> URTUnitAnimInstance::ActiveClipFor(const FName& HeroId, ERTPresentationRole Role,
	const FName& ActionId, const FName& BaseActionId) const
{
	if (const FRTHeroPresentationClips* Eroe = FindClipsFor(HeroId))
	{
		// 🔑 L'ORDINE dei livelli e' la decisione D2: il profilo, poi la generica condivisa fra eroi.
		for (const FName& Chiave : { ActionId, BaseActionId })
		{
			if (Chiave.IsNone())
			{
				continue;   // un livello senza chiave si salta: non si indovina
			}
			const FRTActionPresentationClips* Azione = Eroe->PerAction.Find(Chiave);
			if (Azione == nullptr)
			{
				continue;
			}
			const FRTAnimRoleClips* Pool = Azione->PerRole.Find(Role);
			const FRTAnimVariant* Attiva = Pool ? Pool->FindActive() : nullptr;
			if (Attiva != nullptr)
			{
				return Attiva->Clip;
			}
			// ⚠️ La voce dell'azione c'era ma non per questo ruolo, o senza attiva: si prosegue (Review Focus (a)).
		}
	}
	// Il ripiego e' la clip di ruolo di oggi, con le sue tre uscite a nulla tutte normali.
	return ActiveClipFor(HeroId, Role);
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

	// ⚠️ **DUE ruoli, non tutti.** `ERTPresentationRole` ne nomina di piu' perche' servono all'authoring, ma
	// questo grafo ha due sequence player e legge solo `Idle` e `Move`. Gli altri non passano da qui:
	// `Attack`/`Hit`/`Death`/`Cast` dai `BlueprintImplementableEvent` di `ARTUnit` (`Cast` da #3549),
	// `Dash`/`Defend`/`Fall` da niente, ancora.
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
