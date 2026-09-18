#include "Ability/RTMovementProfileLibrary.h"
#include "Ability/RTCatalogLibrary.h"

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
	//
	// ⏱️ *La firma prendeva `StepBudget` e `MoveBudget` come ASSOLUTI fino al merge di [#641]: [D-412] li
	// fonde in una percentuale sola. `bIsRun` arriva da [D-406] e resta dov'era — i due cambiamenti sono
	// ortogonali, e questa riga e' il punto in cui si sono incontrati.*
	FRTMovementProfile MakeProfile(FName Id, int32 BudgetPercent, int32 Stability,
		bool bPlannable = true, bool bIsRun = false)
	{
		FRTMovementProfile Profile;
		Profile.Id = Id;
		Profile.StepBudgetPercent = BudgetPercent;
		Profile.MoveBudgetPercent = BudgetPercent;
		Profile.Stability = Stability;
		Profile.bPlannable = bPlannable;
		Profile.bIsRun = bIsRun;
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
	// ⚠️ **L'unico profilo che e' una CORSA** ([D-406]): e' il soggetto di [D-319] «chi ha perso
	// l'equilibrio non corre». Con [#641] il criterio esce dal ciclo del Dash, e senza questo campo
	// rifiuterebbe anche il Move normale — cioe' renderebbe `Unbalanced` un'immobilizzazione totale.
	//
	// ⏱️ *Valeva `8` assoluti fino al 2026-09-13, il numero che `Action.Sprint` portava allora come
	// `RangeCells` — campo che dal 2026-09-18 e' `0`, perche' il budget vive qui e in nessun altro posto
	// ([D-427]).*
	// 🔴 **Il moltiplicatore non e' una riscrittura dello stesso valore**: un `8` cablato rendeva lo Sprint
	// piu' LENTO del `Move` di un eroe che ne vale 9, ed e' precisamente l'*upgrade puro* rovesciato. Con
	// `200` il rapporto e' garantito per ogni eroe, che e' cio' che [D-015] chiede a un profilo.
	//
	// 🔑 **E i due cambiamenti non si toccano**: `bIsRun` dice *che cosa* il profilo e', la percentuale dice
	// *quanto* concede. Il merge di [#641] e [D-412] li ha messi sulla stessa riga senza fonderli.
	Catalog.Add(MakeProfile(ProfileSprint, /*×2*/ 200, /*Stability*/ 0,
		/*bPlannable*/ true, /*bIsRun*/ true));

	// `Withdraw` — **×0,25** ([D-412]), il ripiegamento che [D-070] riserva allo slot movimento di chi arma
	// l'Overwatch.
	//
	// ⏱️ *Valeva `2` assoluti fino al 2026-09-13, su `Action.Withdraw.RangeCells`, che dal 2026-09-18 e' `0`
	// ([D-427]).* ⚠️ **Con un eroe da 5 il nuovo valore e' `1`, non `2`**:
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


FRTActionDef URTMovementProfileLibrary::FindActionForProfile(FName ProfileId)
{
	if (ProfileId.IsNone())
	{
		return FRTActionDef();
	}

	// Il catalogo si scorre UNA volta: `GetCoreActionCatalog()` costruisce a ogni invocazione una trentina
	// di `FRTActionDef` con i loro `TArray` annidati, e questa funzione la chiama il compositore del piano —
	// cioe' la HUD a ogni click. Stessa cura, stessa causa, di `MakePlanFor` e `GetReactionProfileCatalog`.
	static const TMap<FName, FRTActionDef> ByProfile = []
	{
		TMap<FName, FRTActionDef> Map;
		for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
		{
			if (!Def.MovementProfileId.IsNone())
			{
				// ⚠️ `Add` e non `FindOrAdd`: se due azioni dichiarassero lo STESSO profilo vincerebbe
				// l'ultima, e sarebbe un difetto di catalogo da vedere in un test — non da mediare qui.
				Map.Add(Def.MovementProfileId, Def);
			}
		}
		return Map;
	}();

	const FRTActionDef* Found = ByProfile.Find(ProfileId);
	return Found ? *Found : FRTActionDef();
}

namespace
{
	/**
	 * Il profilo che una **dichiarazione** o una **riserva** nomina, o un profilo vuoto quando non ce n'e'
	 * nessuna — cioe' quando il tetto e' quello nudo.
	 *
	 * La riserva vince sulla dichiarazione, ed e' l'ordine di [D-425] punto (1): l'`Overwatch` riserva lo
	 * slot movimento al `Withdraw` ([D-070]), e chi ha armato non sceglie di sgusciare.
	 *
	 * ⚠️ **Un `Id` che il catalogo non conosce si comporta come «nessuna dichiarazione», e non solleva
	 * niente**: e' la stessa scelta PERMISSIVA che `ProfileForPlan` dichiara poche righe sopra — un `Id`
	 * scritto male e' un difetto di catalogo, e il posto in cui deve farsi vedere e' il test del catalogo,
	 * non la partita di qualcuno.
	 *
	 * ⛔ **Una corsa dichiarata mentre la corsa e' negata non e' un tetto**, e cade qui invece che nei
	 * chiamanti: [D-319] nega l'andatura, e un tetto che restasse in piedi la concederebbe per la porta di
	 * servizio. Oggi nessuna dichiarazione e' una corsa — si dichiara il solo `Sneak` — e la riga vale per
	 * il giorno in cui una lo fosse.
	 */
	FRTMovementProfile DeclaredOrImposedCeiling(FName DeclaredProfileId, FName ReservedProfileId,
		bool bRunDenied)
	{
		// ⚠️ **I due canali non accettano lo stesso insieme, ed e' l'`AC-4` di `#1410`.** La RISERVA vale
		// per qualunque profilo un'azione dichiari di imporre — chi la scrive e' il catalogo, non il
		// giocatore. La DICHIARAZIONE passa da `IsDeclarableProfile`, senza il quale scrivere
		// `MovementProfile.Sprint` nel campo del piano sarebbe un canale per dichiarare la corsa: cioe'
		// esattamente cio' che [D-425] toglie, rientrato dalla porta di servizio.
		const FRTMovementProfile Imposed = URTMovementProfileLibrary::FindProfile(ReservedProfileId);
		const FRTMovementProfile Declared =
			URTMovementProfileLibrary::IsDeclarableProfile(DeclaredProfileId)
				? URTMovementProfileLibrary::FindProfile(DeclaredProfileId)
				: FRTMovementProfile();

		for (const FRTMovementProfile& Candidate : { Imposed, Declared })
		{
			if (Candidate.IsValid() && !(bRunDenied && Candidate.bIsRun))
			{
				return Candidate;
			}
		}
		return FRTMovementProfile();
	}
}

bool URTMovementProfileLibrary::IsDeclarableProfile(FName ProfileId)
{
	// ⛔ **Un nome solo, e sta qui.** Un predicato che elencasse le quattro esclusioni si romperebbe in
	// silenzio il giorno in cui un sesto profilo entrasse nel catalogo: nascerebbe dichiarabile senza che
	// nessuno lo avesse deciso. Elencare invece cio' che si dichiara fa nascere il nuovo **non**
	// dichiarabile, che e' il default giusto — e obbliga chi lo vuole offrire a passare di qui.
	return ProfileId == ProfileSneak;
}

FRTMovementProfile URTMovementProfileLibrary::CeilingProfile(FName DeclaredProfileId,
	FName ReservedProfileId, bool bRunDenied)
{
	const FRTMovementProfile Named =
		DeclaredOrImposedCeiling(DeclaredProfileId, ReservedProfileId, bRunDenied);
	if (Named.IsValid())
	{
		return Named;
	}

	// Il tetto NUDO e' `Sprint`, cioe' **2×** ([D-425] punto (9)): tutte le bande sono raggiungibili senza
	// dichiarare niente, e superare `1×` fa scattare da se' i prezzi che lo Sprint gia' porta —
	// `Status.Exposed` per due turni e nessuna reazione. ⛔ Con `Move` la banda alta sarebbe irraggiungibile
	// col movimento normale, cioe' una banda che non e' una banda: e' l'alternativa che [D-425] scarta.
	//
	// ⚠️ **E con la corsa negata il nudo scende a `Move`** ([D-319]): non si rifiuta una dichiarazione —
	// non ce n'e' una — si toglie la distanza che rendeva quella banda raggiungibile.
	return FindProfile(bRunDenied ? ProfileMove : ProfileSprint);
}

FRTMovementProfile URTMovementProfileLibrary::ProfileForPlannedSteps(int32 PlannedSteps,
	int32 UnitMoveRange, FName DeclaredProfileId, FName ReservedProfileId, bool bRunDenied)
{
	// (a)+(b) Un tetto imposto o dichiarato **e'** il profilo, e sopra di esso non si legge nessuna banda:
	// chi ripiega ripiega anche di un passo solo, e chi sguscia sguscia. E' la meta' di [D-425] punto (2)
	// per cui questi due si DICHIARANO invece di essere letti — e la ragione per cui le bande `¼` e `½` si
	// sovrappongono in basso senza che la distanza debba separarle.
	const FRTMovementProfile Named =
		DeclaredOrImposedCeiling(DeclaredProfileId, ReservedProfileId, bRunDenied);
	if (Named.IsValid())
	{
		return Named;
	}

	// (c) Nessun passo pianificato: `Still`. E' la lettura di «non ho pianificato movimento», non una
	// scelta — la ragione per cui non si offre da nessuna parte (`#1410` `AC-4`).
	const int32 Steps = FMath::Max(0, PlannedSteps);
	if (Steps == 0)
	{
		return FindProfile(ProfileStill);
	}

	// (d) La banda, letta sui passi CLAMPATI al tetto.
	//
	// 🔑 **Il clamp non e' una difesa, e' la giuntura fra le due meta'.** `TruncatePathToBudget` accorcia
	// il percorso al budget FRESCO al momento della risoluzione; senza il clamp, un piano scritto quando il
	// tetto era piu' alto — o da un'unita' diventata `Unbalanced` dopo averlo scritto — leggerebbe qui la
	// banda `Sprint`, e l'unita' pagherebbe `Status.Exposed` per una corsa che il troncamento le ha gia'
	// tolto. Le due riduzioni devono essere la stessa riduzione.
	const FRTMovementProfile Ceiling = CeilingProfile(DeclaredProfileId, ReservedProfileId, bRunDenied);
	const int32 Capped = FMath::Min(Steps, Ceiling.ResolveStepBudget(UnitMoveRange));

	const FRTMovementProfile Walk = FindProfile(ProfileMove);
	if (Capped <= Walk.ResolveStepBudget(UnitMoveRange))
	{
		return Walk;
	}
	// ⛔ **Oltre `1×` si legge `Sprint`, e una distanza oltre OGNI tetto la legge comunque.** Non dovrebbe
	// accadere — il troncamento la previene — ma dire «cammino» di un percorso che nessun tetto copre
	// nasconderebbe il difetto invece di mostrarlo, e il prezzo sbagliato sarebbe quello piu' basso.
	return FindProfile(ProfileSprint);
}


FName URTMovementProfileLibrary::ReservedProfileForPlan(const TArray<FRTPlannedAction>& Plan)
{
	for (const FRTPlannedAction& Planned : Plan)
	{
		if (!Planned.Def.ReservesMovementProfileId.IsNone())
		{
			return Planned.Def.ReservesMovementProfileId;
		}
	}
	return NAME_None;
}
