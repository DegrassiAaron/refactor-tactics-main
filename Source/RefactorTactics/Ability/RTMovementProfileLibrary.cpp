#include "Ability/RTMovementProfileLibrary.h"

const FName URTMovementProfileLibrary::ProfileStill(TEXT("MovementProfile.Still"));
const FName URTMovementProfileLibrary::ProfileMove(TEXT("MovementProfile.Move"));
const FName URTMovementProfileLibrary::ProfileSprint(TEXT("MovementProfile.Sprint"));
const FName URTMovementProfileLibrary::ProfileWithdraw(TEXT("MovementProfile.Withdraw"));
const FName URTMovementProfileLibrary::ProfileSneak(TEXT("MovementProfile.Sneak"));

namespace
{
	FRTMovementProfile MakeProfile(FName Id, int32 StepBudget, int32 MoveBudget, int32 Stability,
		bool bPlannable = true)
	{
		FRTMovementProfile Profile;
		Profile.Id = Id;
		Profile.StepBudget = StepBudget;
		Profile.MoveBudget = MoveBudget;
		Profile.Stability = Stability;
		Profile.bPlannable = bPlannable;
		return Profile;
	}
}

TArray<FRTMovementProfile> URTMovementProfileLibrary::GetCoreMovementProfileCatalog()
{
	TArray<FRTMovementProfile> Catalog;

	// `Still` — chi non pianifica movimento. Stabilita' PIU' ALTA: e' la lettura di
	// `spec-compatibilita-azioni-movimento.md` §3.2, «fermo 3 · tutto e' possibile». Non e' un caso
	// degenere da trattare a parte nel chiamante: e' un profilo come gli altri, ed e' il motivo per cui
	// `ProfileForPlan` non restituisce mai un profilo invalido.
	//
	// ⛔ **I budget EREDITANO, e non valgono `0`.** Il numero nello snapshot e' una CAPACITA' — «quanto
	// potrei spendere» — non una dichiarazione di spesa: chi non ha pianificato movimento riceve oggi
	// `GetEffectiveMoveRange()` come tutti, e da quel valore dipende cio' che vede chi sta ancora
	// pianificando (le celle raggiungibili) e chi valuta un'alternativa (il bot). Azzerarlo qui avrebbe
	// reso immobili le unita' senza piano, che e' esattamente il comportamento che `#653` promette di non
	// cambiare. La spec assegna a «fermo» una `Stability`, non un budget: qui non se ne inventa uno.
	Catalog.Add(MakeProfile(ProfileStill, FRTMovementProfile::InheritFromUnit,
		FRTMovementProfile::InheritFromUnit, /*Stability*/ 3));

	// `Move` — il profilo neutro. ⚠️ **I budget si EREDITANO dall'unita', e qui sta la clausola per cui
	// `#653` non cambia niente**: il catalogo markdown §2.1 gli attribuisce `5`, ma nel codice il budget
	// del movimento normale e' sempre venuto da `ARTUnit::MoveRange` via `GetEffectiveMoveRange()`, che
	// varia per eroe. Cablare `5` qui abbasserebbe o alzerebbe in silenzio ogni eroe che non vale 5.
	Catalog.Add(MakeProfile(ProfileMove, FRTMovementProfile::InheritFromUnit,
		FRTMovementProfile::InheritFromUnit, /*Stability*/ 1));

	// `Sprint` — 8, il numero che `Action.Sprint` porta gia' come `RangeCells` nel catalogo core e che oggi
	// legge solo `ResolveDash`. `Stability 0`: «hai speso il turno a coprire distanza» ([D-116] voce 3).
	Catalog.Add(MakeProfile(ProfileSprint, /*Passi*/ 8, /*Asperita'*/ 8, /*Stability*/ 0));

	// `Withdraw` — 2, il ripiegamento dichiarato che [D-070] riserva allo slot movimento di chi arma
	// l'Overwatch. Prima di `#653` non era ESPRIMIBILE: non essendoci un profilo, non c'era dove scrivere
	// un budget diverso da quello dell'unita'.
	//
	// ⚠️ **`Stability 1` qui e' un SEGNAPOSTO, e va detto invece di lasciarlo sembrare derivato.** La
	// tabella di `spec-compatibilita-azioni-movimento.md` §3.2 dichiara quattro valori — fermo `3`, Sneak
	// `2`, Move `1`, Sprint `0` — e `Withdraw` **non e' fra questi**. Gli si da' il valore del profilo
	// neutro perche' e' quello che non cambia nessun verdetto (nessuna azione dichiara oggi un
	// `MinStability`), e la taratura e' [#606], che la spec stessa dichiara «la parte da playtestare».
	Catalog.Add(MakeProfile(ProfileWithdraw, /*Passi*/ 2, /*Asperita'*/ 2, /*Stability*/ 1));

	// `Sneak` — ⛔ **senza numeri, e sta nel catalogo proprio per questo** (`AE-5`). Il catalogo markdown
	// §2.1 gli assegna «—» al posto di un budget: il profilo e' previsto, il dato non c'e'. `bPlannable`
	// falso lo rende non scegliibile finche' quel numero non viene deciso; i budget restano `InheritFromUnit`
	// perche' un `0` avrebbe significato «immobile», che e' un'affermazione diversa da «non si sa».
	Catalog.Add(MakeProfile(ProfileSneak, FRTMovementProfile::InheritFromUnit,
		FRTMovementProfile::InheritFromUnit, /*Stability*/ 2, /*bPlannable*/ false));

	return Catalog;
}

FRTMovementProfile URTMovementProfileLibrary::FindProfile(FName ProfileId)
{
	if (ProfileId.IsNone())
	{
		return FRTMovementProfile();
	}

	for (const FRTMovementProfile& Profile : GetCoreMovementProfileCatalog())
	{
		if (Profile.Id == ProfileId)
		{
			return Profile;
		}
	}
	return FRTMovementProfile();
}

FRTMovementProfile URTMovementProfileLibrary::ProfileForPlan(const TArray<FRTPlannedAction>& Plan)
{
	for (const FRTPlannedAction& Planned : Plan)
	{
		if (!Planned.Def.MovementProfileId.IsNone())
		{
			const FRTMovementProfile Profile = FindProfile(Planned.Def.MovementProfileId);
			if (Profile.IsValid())
			{
				return Profile;
			}
			// Un'azione che nomina un profilo INESISTENTE non prosegue la ricerca: mostrerebbe il profilo
			// dell'azione successiva come se fosse quello scelto. Si esce con `Still`, che eredita i
			// budget dall'unita' — cioe' il comportamento di sempre — e concede la `Stability` piu' alta.
			// ⚠️ E' una scelta PERMISSIVA, ed e' voluta: un `Id` scritto male e' un difetto di catalogo,
			// e il posto dove deve fallire e' il test del catalogo, non la partita di qualcuno.
			return FindProfile(ProfileStill);
		}
	}
	return FindProfile(ProfileStill);
}
