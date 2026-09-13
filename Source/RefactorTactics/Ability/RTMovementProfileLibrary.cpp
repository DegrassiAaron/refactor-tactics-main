#include "Ability/RTMovementProfileLibrary.h"

const FName URTMovementProfileLibrary::ProfileStill(TEXT("MovementProfile.Still"));
const FName URTMovementProfileLibrary::ProfileMove(TEXT("MovementProfile.Move"));
const FName URTMovementProfileLibrary::ProfileSprint(TEXT("MovementProfile.Sprint"));
const FName URTMovementProfileLibrary::ProfileWithdraw(TEXT("MovementProfile.Withdraw"));
const FName URTMovementProfileLibrary::ProfileSneak(TEXT("MovementProfile.Sneak"));

namespace
{
	// ⚠️ **Un solo `Percent` per profilo, e non due parametri**: [D-412] dichiara il moltiplicatore **del
	// profilo**, non uno per ciascuno dei due budget di [D-117]. Passarne due qui inviterebbe a divergerli
	// senza una decisione che lo autorizzi — e la separazione dei VALORI e' [#666], non questa firma.
	FRTMovementProfile MakeProfile(FName Id, int32 BudgetPercent, int32 Stability, bool bPlannable = true)
	{
		FRTMovementProfile Profile;
		Profile.Id = Id;
		Profile.StepBudgetPercent = BudgetPercent;
		Profile.MoveBudgetPercent = BudgetPercent;
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
	// ⛔ **Il budget e' quello dell'unita', e non `0`.** Il numero nello snapshot e' una CAPACITA' — «quanto
	// potrei spendere» — non una dichiarazione di spesa: chi non ha pianificato movimento riceve oggi
	// `GetEffectiveMoveRange()` come tutti, e da quel valore dipende cio' che vede chi sta ancora
	// pianificando (le celle raggiungibili) e chi valuta un'alternativa (il bot). Azzerarlo qui avrebbe
	// reso immobili le unita' senza piano. La spec assegna a «fermo» una `Stability`, non un budget: qui
	// non se ne inventa uno, e il `100%` dice esattamente «quello dell'unita'».
	Catalog.Add(MakeProfile(ProfileStill, FRTMovementProfile::NeutralPercent, /*Stability*/ 3));

	// `Move` — il profilo neutro, ×1 per definizione ([D-412]). Il budget del movimento normale e' sempre
	// venuto da `ARTUnit::MoveRange` via `GetEffectiveMoveRange()`, che varia per eroe: il `100%` conserva
	// quel comportamento invece di cablare il `5` del catalogo markdown, che abbasserebbe o alzerebbe in
	// silenzio ogni eroe che non vale 5.
	Catalog.Add(MakeProfile(ProfileMove, FRTMovementProfile::NeutralPercent, /*Stability*/ 1));

	// `Sprint` — **×2** ([D-412]). `Stability 0`: «hai speso il turno a coprire distanza» ([D-116] voce 3).
	//
	// ⏱️ *Valeva `8` assoluti fino al 2026-09-13, il numero che `Action.Sprint` porta come `RangeCells`.*
	// 🔴 **Il moltiplicatore non e' una riscrittura dello stesso valore**: un `8` cablato rendeva lo Sprint
	// piu' LENTO del `Move` di un eroe che ne vale 9, ed e' precisamente l'*upgrade puro* rovesciato. Con
	// `200` il rapporto e' garantito per ogni eroe, che e' cio' che [D-015] chiede a un profilo.
	Catalog.Add(MakeProfile(ProfileSprint, /*×2*/ 200, /*Stability*/ 0));

	// `Withdraw` — **×0,25** ([D-412]), il ripiegamento che [D-070] riserva allo slot movimento di chi arma
	// l'Overwatch.
	//
	// ⏱️ *Valeva `2` assoluti fino al 2026-09-13.* ⚠️ **Con un eroe da 5 il nuovo valore e' `1`, non `2`**:
	// la divisione tronca, ed e' l'*«arrotondare per difetto»* di [D-412]. La sorgente lo sapeva — «Withdraw
	// diventava 2 solo con base almeno 8» — e non ha corretto il moltiplicatore per conservare il vecchio
	// numero: e' il numero a seguire la regola.
	//
	// ⚠️ **`Stability 1` e' un SEGNAPOSTO, e va detto invece di lasciarlo sembrare derivato.** La tabella di
	// `spec-compatibilita-azioni-movimento.md` §3.2 dichiara quattro valori — fermo `3`, Sneak `2`, Move `1`,
	// Sprint `0` — e `Withdraw` **non e' fra questi**. Gli si da' il valore del profilo neutro perche' e'
	// quello che non cambia nessun verdetto (nessuna azione dichiara oggi un `MinStability`), e la taratura
	// e' [#606], che la spec stessa dichiara «la parte da playtestare».
	Catalog.Add(MakeProfile(ProfileWithdraw, /*×0,25*/ 25, /*Stability*/ 1));

	// `Sneak` — **×0,5, e PIANIFICABILE** ([D-412], che chiude `AE-5`).
	//
	// ⏱️ *Fino al 2026-09-13 stava qui senza numeri e con `bPlannable = false`, ed era la dichiarazione di
	// una lacuna: «il profilo e' previsto, il dato non c'e'».* ✅ Ora il dato c'e', e sono tre: budget ×0,5,
	// cadenza 1 passo ogni 2 tick, **sempre silenzioso** indipendentemente dal terreno.
	//
	// ⛔ **La cadenza NON e' qui, e non e' una dimenticanza**: «1 passo ogni 2 tick» e' una proprieta' della
	// risoluzione, non del budget, e il calendario dei sotto-passi che la renderebbe deterministica e'
	// `SKB-2` in `OPEN_DECISIONS.md` — dichiarato `CRITICO` e aperto dalla sorgente stessa. Scriverla qui
	// come un numero significherebbe inventare l'ordine che quella domanda deve ancora decidere.
	Catalog.Add(MakeProfile(ProfileSneak, /*×0,5*/ 50, /*Stability*/ 2));

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
