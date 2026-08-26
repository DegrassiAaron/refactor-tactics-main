# Derivazioni delle azioni d'eroe — piano di implementazione

> ✅ **Eseguito il 2026-08-26** in [#1406](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1406).
> Le caselle sono consuntivate: questo file resta come **registro di come è andata**, non come lista di
> cose da fare. Tre task sono stati modificati in corsa e uno rimosso — ognuno dice perché, sul posto.
>
> ⚠️ **Due cose che il piano dava per vere non lo erano**, e le ha trovate la code review, non l'esecuzione:
> la guardia dell'helper era fail-**open** su `NAME_None`, e l'intero ragionamento sull'equipaggiamento
> («nessuna unità ne riceve») era falso — `DefaultLoadoutFor` lo consegna a tutti e quattro gli eroi. La
> correzione è in [D-195](../../decisions/RT_PDR_00_Decision_Log.md) e nella spec.

> **Per chi esegue:** usa `superpowers:subagent-driven-development` (consigliato) o
> `superpowers:executing-plans` per eseguire un task alla volta. Gli step usano checkbox (`- [x]`).

**Obiettivo:** rendere un dato la relazione «questa abilità d'eroe eredita i suoi valori da quell'azione
core», e usarla per un gate che dichiara, azione per azione, se il catalogo è raggiungibile in partita.

**Architettura:** un campo nuovo in `FRTActionDef` (`DerivedFromActionId`), scritto da un helper che
prende l'ID dell'azione core invece del suo `Def`; un test automation che confronta il catalogo core con
le vie di raggiungibilità e con un elenco dichiarato di eccezioni motivate.

**Stack:** C++ / Unreal Engine 5.8.1, automation test `IMPLEMENT_SIMPLE_AUTOMATION_TEST`.

**Spec:** [`docs/technical/piano-derivazioni-azioni-eroe.md`](../../technical/piano-derivazioni-azioni-eroe.md)

## Vincoli globali

- **UE 5.8.1**; nessuna dipendenza esterna nuova.
- ⛔ **`BaseActionId` non si tocca**: è di D-033 e significa «di quale delle sette generiche questa è il
  profilo». Il test `RefactorTactics.Heroes.BasicAttackDeclaresItsBaseAction` deve restare verde **senza
  essere modificato** — è il criterio che prova che le due semantiche non si sono sovrapposte.
- ⛔ **Nessun cambio di comportamento in partita**: l'helper eredita esattamente ciò che `MakeHeroAction`
  ereditava già. Le righe che copiano a mano i campi di comportamento (`PropagationLimit`,
  `MovementStyle`, `StructureOp`) **restano dove sono**, invariate.
- **Ogni task finisce con un commit.** L'implementazione va committata **prima** di qualunque verifica di
  mutazione: `git checkout <file>` per togliere una mutazione cancella anche ciò che non era committato.
- Suite: `Automation RunTests RefactorTactics+Quit` con `-nullrhi`, una run per volta, leggendo
  `Test Completed` prima del verdetto.

---

### Task 1 — Il campo `DerivedFromActionId`

**File:**
- Modifica: `Source/RefactorTactics/Ability/RTActionDef.h` (dopo `BaseActionId`, ~riga 302)
- Test: `Source/RefactorTactics/Tests/RTCatalogTests.cpp`

**Interfacce:**
- Produce: `FRTActionDef::DerivedFromActionId` di tipo `FName`, vuoto per default. I task 2-6 lo leggono
  e lo scrivono.

- [x] **Step 1: scrivi il test che fallisce**

In `RTCatalogTests.cpp`, prima di `#endif`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionDefDerivedFromIsEmptyByDefaultTest,
	"RefactorTactics.Catalog.DerivedFromIsEmptyByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionDefDerivedFromIsEmptyByDefaultTest::RunTest(const FString&)
{
	// Il default DEVE essere vuoto: il gate legge l'assenza come «non deriva da nulla», e un default
	// diverso da NAME_None darebbe a ogni azione una derivazione che nessuno ha dichiarato.
	const FRTActionDef Vuoto;
	TestTrue(TEXT("una definizione appena costruita non dichiara derivazione"),
		Vuoto.DerivedFromActionId.IsNone());

	// E non e' `BaseActionId`: due campi, due domande. Se qualcuno li fondesse, questo test cadrebbe.
	const FRTActionDef Core = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	TestTrue(TEXT("un'azione del catalogo core non deriva da se stessa"),
		Core.DerivedFromActionId.IsNone());
	return true;
}
```

- [x] **Step 2: esegui e verifica che NON compili**

Run: `Automation RunTests RefactorTactics.Catalog.DerivedFromIsEmptyByDefault+Quit`
Atteso: errore di compilazione — `DerivedFromActionId` non è un membro di `FRTActionDef`.

- [x] **Step 3: aggiungi il campo**

In `RTActionDef.h`, subito dopo la `UPROPERTY` di `BaseActionId`:

```cpp
	/** Da quale azione CORE questa eredita i suoi valori (fase, priorita', portata, fallback, effetti).
	 *
	 * ⚠️ **Non e' `BaseActionId`, ed e' la stessa domanda solo per gli attacchi base.** `BaseActionId`
	 * dice di quale delle SETTE generiche un'azione e' il profilo (D-033): `Hero.Riktor.ImpactShot` e'
	 * il profilo di `Action.BasicAttack`. Questo campo dice da dove vengono i NUMERI, e la sorgente puo'
	 * essere un'azione che generica non e': `Hero.Riktor.Ram` eredita da `Action.Charge`, e un profilo
	 * di `Charge` non esiste perche' `Charge` non e' fra le sette.
	 *
	 * Il consumatore e' `RefactorTactics.Catalog.EveryCoreActionIsReachableOrDeclared`: senza questo
	 * campo la relazione vive solo nel sorgente, e nessun test puo' dire se un'azione del catalogo e'
	 * raggiungibile da un giocatore o e' contenuto che nessuno porta.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName DerivedFromActionId;
```

- [x] **Step 4: esegui e verifica che passi**

Run: `Automation RunTests RefactorTactics.Catalog.DerivedFromIsEmptyByDefault+Quit`
Atteso: PASS, `Test Completed: 1`.

- [x] **Step 5: commit**

```bash
git add Source/RefactorTactics/Ability/RTActionDef.h Source/RefactorTactics/Tests/RTCatalogTests.cpp
git commit -m "feat(catalogo): FRTActionDef dichiara da quale azione core eredita i valori"
```

---

### Task 2 — L'helper `MakeHeroActionFromCore`

**File:**
- Modifica: `Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp` (namespace anonimo, dopo
  `MakeHeroBasicAttack`, ~riga 141)
- Test: `Source/RefactorTactics/Tests/RTHeroCatalogTests.cpp`

**Interfacce:**
- Consuma: `FRTActionDef::DerivedFromActionId` (Task 1).
- Produce: `MakeHeroActionFromCore(const FName& HeroActionId, const FName& CoreActionId, int32 Cooldown,
  ERTActionFallback Fallback, const TArray<FRTActionEffectSpec>& Effects, ERTAbilityShape Shape,
  int32 AreaRadius, ERTActionSlot Slot)` → `URTActionData*`, `nullptr` se `CoreActionId` è sconosciuto.
  I task 3-5 lo chiamano.

- [x] **Step 1: scrivi il test che fallisce**

In `RTHeroCatalogTests.cpp`, prima di `#endif`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroDerivedActionsDeclareOriginTest,
	"RefactorTactics.Heroes.DerivedActionsDeclareTheirOrigin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroDerivedActionsDeclareOriginTest::RunTest(const FString&)
{
	// Le otto derivazioni del roster v0.1, misurate sul sorgente il 2026-08-26. Chi aggiunge un eroe
	// che deriva da un'azione core aggiunge una riga qui: e' l'elenco che rende la relazione verificabile.
	//
	// ⛔ Gli attacchi base NON sono qui: dichiarano `BaseActionId` (profilo di una generica, D-033) e non
	// una derivazione di parametri — tre dei quattro hanno i numeri scritti a mano, non presi dal core.
	const TMap<FName, FName> Atteso = {
		{ TEXT("Hero.Gadget.ConductiveNode"),     TEXT("Action.Electrify")    },
		{ TEXT("Hero.Phase.FluidTrail"),          TEXT("Action.Dash")         },
		{ TEXT("Hero.Phase.MistVeil"),            TEXT("Action.Ignite")       },
		{ TEXT("Hero.Riktor.KineticPanel"),       TEXT("Action.CreateCover")  },
		{ TEXT("Hero.Riktor.Ram"),                TEXT("Action.Charge")       },
		{ TEXT("Hero.Gadget.ReactiveCapacitor"),  TEXT("Action.Counter")      },
		{ TEXT("Hero.Riktor.Interposition"),      TEXT("Action.Intercept")    },
		{ TEXT("Hero.Wraith.Deflection"),         TEXT("Action.Deflect")      },
	};

	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestEqual(TEXT("il roster v0.1 ha quattro eroi"), Roster.Num(), 4)) { return false; }

	int32 Dichiarate = 0;
	for (const URTHeroData* Hero : Roster)
	{
		if (!Hero) { continue; }
		for (const URTActionData* A : Hero->Actions)
		{
			if (!A) { continue; }
			const FName* Origine = Atteso.Find(A->Def.ActionId);
			if (Origine)
			{
				TestEqual(*FString::Printf(TEXT("%s dichiara la sua origine"), *A->Def.ActionId.ToString()),
					A->Def.DerivedFromActionId, *Origine);
				++Dichiarate;
			}
			else
			{
				// Il verso opposto conta quanto il primo: un campo messo dappertutto non direbbe piu'
				// niente. `Overload`, `CircularTide`, `Reconfigure`, `FlowReaction`, `LinearDischarge`
				// non ereditano da nessuna azione core, e devono restare vuote.
				TestTrue(*FString::Printf(TEXT("%s non deriva da nulla e non lo dichiara"),
					*A->Def.ActionId.ToString()), A->Def.DerivedFromActionId.IsNone());
			}
		}
	}

	// Anti-vacuita': se il roster tornasse vuoto o gli ID cambiassero, i cicli sopra non asserirebbero
	// niente e il test resterebbe verde raccontando che va tutto bene.
	TestEqual(TEXT("tutte le derivazioni attese sono state trovate"), Dichiarate, Atteso.Num());
	return true;
}
```

- [x] **Step 2: esegui e verifica che fallisca**

Run: `Automation RunTests RefactorTactics.Heroes.DerivedActionsDeclareTheirOrigin+Quit`
Atteso: FAIL — undici asserzioni «dichiara la sua origine» cadono, e `Dichiarate` vale 11 ma i campi sono
vuoti. È il rosso che i task 3-5 spengono.

- [x] **Step 3: scrivi l'helper**

In `RTHeroCatalogLibrary.cpp`, nel namespace anonimo, dopo `MakeHeroBasicAttack`:

```cpp
	/**
	 * Un'azione d'eroe che EREDITA i suoi valori da un'azione core, e lo dichiara.
	 *
	 * Prende l'**ID** e non il `Def` di proposito: cosi' la derivazione non e' una cosa da ricordarsi di
	 * annotare dopo aver letto il catalogo, e' il modo stesso di leggerlo. Stessa forma di
	 * `MakeHeroBasicAttack`, che fa questo per `BaseActionId`.
	 *
	 * ⛔ Fail-closed su ID sconosciuto: un'abilita' con i default di `FRTActionDef` e una derivazione
	 * falsa sarebbe peggio di nessuna abilita' — funzionerebbe, e mentirebbe al gate.
	 *
	 * ⚠️ Eredita esattamente cio' che `MakeHeroAction` ereditava: identita', fase, priorita', portata,
	 * fallback ed effetti. **Non** i campi di comportamento (`MovementStyle`, `StructureOp`,
	 * `PropagationLimit`), che i chiamanti continuano a copiare a mano dove serve.
	 */
	URTActionData* MakeHeroActionFromCore(const FName& HeroActionId, const FName& CoreActionId,
		int32 Cooldown, ERTAbilityShape Shape = ERTAbilityShape::Single, int32 AreaRadius = 0,
		const TArray<FRTActionEffectSpec>& EffectsOverride = {},
		bool bUseCoreSlot = false, ERTActionFallback FallbackOverride = ERTActionFallback::Cancel,
		bool bUseCoreFallback = true)
	{
		const FRTActionDef Core = URTCatalogLibrary::FindCoreAction(CoreActionId);
		if (Core.ActionId != CoreActionId)
		{
			return nullptr;
		}

		URTActionData* Action = MakeHeroAction(HeroActionId, Core.ResolutionPhase, Core.Priority,
			Core.RangeCells, Cooldown, bUseCoreFallback ? Core.Fallback : FallbackOverride,
			EffectsOverride.Num() > 0 ? EffectsOverride : Core.Effects, Shape, AreaRadius,
			bUseCoreSlot ? Core.Slot : ERTActionSlot::Main);

		Action->Def.DerivedFromActionId = CoreActionId;
		return Action;
	}
```

- [x] **Step 4: aggiungi il test del fail-closed**

In `RTHeroCatalogTests.cpp`, subito dopo il test dello Step 1:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroDerivedFromUnknownIsNullTest,
	"RefactorTactics.Heroes.DerivedFromUnknownCoreActionIsNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroDerivedFromUnknownIsNullTest::RunTest(const FString&)
{
	// `MakeHeroActionFromCore` sta nel namespace anonimo del .cpp, quindi si prova dal suo fratello
	// pubblico che ha lo stesso contratto: un ID che il catalogo non conosce non produce un'abilita'.
	TestNull(TEXT("un'azione core inesistente non produce una reazione"),
		URTHeroCatalogLibrary::MakeHeroReactionFromCoreAction(
			TEXT("Hero.Test.Inesistente"), TEXT("Action.NonEsiste"), 1, {}, 1));
	return true;
}
```

- [x] **Step 5: esegui, commit**

Run: `Automation RunTests RefactorTactics.Heroes.DerivedFromUnknownCoreActionIsNull+Quit` → PASS.
Il test dello Step 1 resta ROSSO: lo spengono i task 3-5.

```bash
git add Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp Source/RefactorTactics/Tests/RTHeroCatalogTests.cpp
git commit -m "feat(catalogo eroi): un helper che deriva da un'azione core e lo dichiara"
```

---

### Task 3 — Le tre reazioni dichiarano la loro origine

**File:**
- Modifica: `Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp:800-821`
  (`MakeHeroReactionFromCoreAction`)

**Interfacce:**
- Consuma: `DerivedFromActionId` (Task 1).
- Produce: `ReactiveCapacitor`, `Interposition`, `Deflection` con l'origine dichiarata.

- [x] **Step 1: aggiungi la riga**

In `MakeHeroReactionFromCoreAction`, subito dopo l'assegnazione di `ReactionTrigger`:

```cpp
	// La funzione riceve `CoreActionId` da sempre e non lo scriveva da nessuna parte: il dato attraversava
	// la firma e finiva buttato. Ora la relazione resta nel Def, dove il gate la legge.
	Action->Def.DerivedFromActionId = CoreActionId;
```

- [x] **Step 2: esegui il test del Task 2**

Run: `Automation RunTests RefactorTactics.Heroes.DerivedActionsDeclareTheirOrigin+Quit`
Atteso: ancora FAIL, ma con **tre** asserzioni in meno che cadono — `ReactiveCapacitor`, `Interposition`
e `Deflection` ora passano. Se non cala di esattamente tre, la modifica non ha raggiunto le tre reazioni.

- [x] **Step 3: commit**

```bash
git add Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp
git commit -m "feat(catalogo eroi): le tre reazioni dichiarano l'azione core da cui derivano"
```

---

### Task 4 — Le cinque derivazioni da `Def` locale

**File:**
- Modifica: `Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp` righe 306-313, 452-460, 476-485,
  588-592, 614-623

**Interfacce:**
- Consuma: `MakeHeroActionFromCore` (Task 2).

- [x] **Step 1: `ConductiveNode` (riga ~306)**

Prima:

```cpp
	const FRTActionDef ElectrifyDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Electrify"));
	URTActionData* ConductiveNode = MakeHeroAction(TEXT("Hero.Gadget.ConductiveNode"), ElectrifyDef.ResolutionPhase,
		ElectrifyDef.Priority, ElectrifyDef.RangeCells, /*Cooldown*/ 2, ElectrifyDef.Fallback,
		ElectrifyDef.Effects);
	ConductiveNode->Def.PropagationLimit = ElectrifyDef.PropagationLimit;
```

Dopo — la riga di `PropagationLimit` **resta**, e legge il core una volta sola:

```cpp
	const FRTActionDef ElectrifyDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Electrify"));
	URTActionData* ConductiveNode = MakeHeroActionFromCore(TEXT("Hero.Gadget.ConductiveNode"),
		TEXT("Action.Electrify"), /*Cooldown*/ 2);
	// La propagazione e' IL comportamento, non un dettaglio: senza questa riga l'azione elettrificherebbe
	// una cella sola e CP 8.3 resterebbe non innescabile pur avendo un owner.
	ConductiveNode->Def.PropagationLimit = ElectrifyDef.PropagationLimit;
```

- [x] **Step 2: `FluidTrail` (riga ~452)**

```cpp
	const FRTActionDef DashDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash"));
	URTActionData* FluidTrail = MakeHeroActionFromCore(TEXT("Hero.Phase.FluidTrail"),
		TEXT("Action.Dash"), /*Cooldown*/ 2, ERTAbilityShape::Single, /*AreaRadius*/ 0, {},
		/*bUseCoreSlot*/ false);
	FluidTrail->Def.Slot = ERTActionSlot::Movement; // mobilita' che ATTRAVERSA: slot movimento [D-191]
	FluidTrail->Def.MovementStyle = DashDef.MovementStyle;
```

- [x] **Step 3: `MistVeil` (riga ~476)**

`MistVeil` è il caso con più override: fallback proprio (`Cancel`) ed effetti **vuoti**. È anche l'unico
in cui la riga `const FRTActionDef IgniteDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Ignite"));`
**va rimossa** — negli altri quattro casi resta, perché serve ai campi di comportamento. Lasciarla qui
darebbe una variabile non usata.

```cpp
	URTActionData* MistVeil = MakeHeroActionFromCore(TEXT("Hero.Phase.MistVeil"),
		TEXT("Action.Ignite"), /*Cooldown*/ 3, ERTAbilityShape::Area, /*AreaRadius*/ 1, {},
		/*bUseCoreSlot*/ false, ERTActionFallback::Cancel, /*bUseCoreFallback*/ false);
	MistVeil->Def.Effects.Empty(); // «crea fumo», non brucia: nessun effetto ereditato da Ignite
	MistVeil->Def.bCreatesSurface = true;
	MistVeil->Def.SurfaceCreated = ERTHexSurface::Smoke;
	MistVeil->Def.SurfaceRadius = 1; // «crea fumo raggio 1», catalogo eroi
```

⚠️ La riga `Effects.Empty()` è necessaria: l'helper eredita gli effetti del core quando non gli passi un
override, e `{}` come override significa «non ho override». Senza, `MistVeil` erediterebbe il danno da
fuoco di `Ignite` — un cambiamento di comportamento che questo piano vieta.

- [x] **Step 4: `KineticPanel` (riga ~588)**

```cpp
	{
		const FRTActionDef CoverDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.CreateCover"));
		URTActionData* Panel = MakeHeroActionFromCore(TEXT("Hero.Riktor.KineticPanel"),
			TEXT("Action.CreateCover"), /*Cooldown*/ 2);
		Panel->Def.StructureOp = CoverDef.StructureOp; // erige: e' il dato che il resolver legge
		Riktor->Actions.Add(Panel);
	}
```

- [x] **Step 5: `Ram` (riga ~614)**

`Ram` eredita anche lo **slot** dal core ([D-191]), quindi passa `bUseCoreSlot`:

```cpp
	const FRTActionDef ChargeDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	URTActionData* Ram = MakeHeroActionFromCore(TEXT("Hero.Riktor.Ram"), TEXT("Action.Charge"),
		/*Cooldown*/ 2, ERTAbilityShape::Single, /*AreaRadius*/ 0, {}, /*bUseCoreSlot*/ true);
	Ram->Def.MovementStyle = ChargeDef.MovementStyle; // LinearCharge: si ferma ADDOSSO al primo nemico
```

- [x] **Step 6: esegui i test di struttura del roster**

Run: `Automation RunTests RefactorTactics.Heroes+Quit`
Atteso: `DerivedActionsDeclareTheirOrigin` cade ora solo sui **tre attacchi base** (Task 5). Tutti gli
altri test del roster — `HeroStatsFromData`, `ExactlyOneVariantPerHero`, `ValidateStructure`,
`BasicAttackIsIndexZeroForEveryHero`, `BasicAttackDeclaresItsBaseAction` — devono restare **verdi**: se
uno cade, l'helper ha cambiato un valore, che questo piano vieta.

- [x] **Step 7: commit**

```bash
git add Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp
git commit -m "feat(catalogo eroi): le cinque abilita' derivate passano dall'helper e dichiarano l'origine"
```

---

### Task 5 — (rimosso durante l'esecuzione)

⛔ **`MakeHeroBasicAttack` non si tocca.** Il piano prevedeva di farle scrivere anche
`DerivedFromActionId`. Misurato mentre si scriveva il Task 2: dei quattro attacchi base solo
`Hero.Gadget.ArcPulse` deriva davvero i parametri (`MakeBasicAttack(4)` parte da `FindCoreAction`);
`PressureJet`, `ImpactShot` e `PulseShot` li hanno **letterali**. Dichiararli derivati avrebbe
trasformato «i parametri vengono da lì» in «gli somiglia», che è la parentela semantica scartata.

∴ il gate del Task 6 legge **due campi** — `DerivedFromActionId` per le otto derivazioni e `BaseActionId`
per gli attacchi base, che di `Action.BasicAttack` sono il profilo. `Action.BasicAttack` resta
raggiungibile per la via che gli compete.

---

### Task 6 — Il gate

**File:**
- Modifica: `Source/RefactorTactics/Tests/RTCatalogTests.cpp`

**Interfacce:**
- Consuma: `DerivedFromActionId` sulle abilità del roster (Task 3-5), `GetGenericActionIds()`,
  `MakeReactionModules()`, `GetCoreActionCatalog()`.

- [x] **Step 1: scrivi il gate**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogReachableOrDeclaredTest,
	"RefactorTactics.Catalog.EveryCoreActionIsReachableOrDeclared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogReachableOrDeclaredTest::RunTest(const FString&)
{
	// Un'azione del catalogo core arriva in partita per quattro vie. Tre sono interrogabili qui; la
	// quarta — il motore che la scrive da se', come `Action.Move` in `MakePlanFor` — e' una proprieta'
	// del sorgente e non del dato, quindi vive nell'elenco sotto.
	TSet<FName> Raggiungibili;

	for (const FName& Id : URTCatalogLibrary::GetGenericActionIds())
	{
		Raggiungibili.Add(Id);
	}
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		for (const URTActionData* A : Hero->Actions)
		{
			if (!A) { continue; }
			// Due campi, due vie di raggiungibilita' diverse e ugualmente valide: «un eroe porta
			// un'abilita' che eredita da X» e «un eroe porta un profilo di X» (D-033, gli attacchi base).
			if (!A->Def.DerivedFromActionId.IsNone()) { Raggiungibili.Add(A->Def.DerivedFromActionId); }
			if (!A->Def.BaseActionId.IsNone())        { Raggiungibili.Add(A->Def.BaseActionId); }
		}
	}

	// ⚠️ La via «base di un modulo reazione» NON entra: i moduli si consegnano come equipaggiamento, e
	// nessuna unita' ne riceve in partita (`ARTUnit` non ha il campo). Le tre azioni che ne dipendono
	// stanno nell'elenco con la loro ragione, e la condizione di riapertura e' nel Decision Log.

	// Le eccezioni, ognuna con la sua ragione. Le tre categorie invecchiano in modo diverso: `Motore`
	// sparisce se un'azione smette di essere prodotta dal gameplay, `Modulo` quando l'equipaggiamento
	// diventa consegnabile, `NonAssegnata` quando un eroe la porta.
	const TMap<FName, FString> Dichiarate = {
		// Scritte dal motore: il gameplay le produce senza passare da un kit.
		{ TEXT("Action.Move"),            TEXT("Motore: MakePlanFor la aggiunge quando l'unita' si muove") },
		{ TEXT("Action.Cleanse"),         TEXT("Motore: il Blast la CERCA fra le abilita'; produttore assente, #1389") },
		{ TEXT("Action.Heal"),            TEXT("Motore: RTTurnManager la scrive come voce di cura") },
		{ TEXT("Action.Interrupt"),       TEXT("Motore: raccolta e applicata dal TurnManager") },
		{ TEXT("Action.ModifyArc"),       TEXT("Motore: scritta dal TurnManager") },
		// Basi di moduli reazione, e i moduli non arrivano a un'unita'.
		{ TEXT("Action.Anchor"),          TEXT("Modulo: base di Reaction.Anchor, equipaggiamento non consegnabile") },
		{ TEXT("Action.Evade"),           TEXT("Modulo: base di Reaction.HazardEscape, idem") },
		{ TEXT("Action.Purge"),           TEXT("Modulo: base di Reaction.Cleanse, idem") },
		// Bloccata da una migrazione decisa e non fatta.
		{ TEXT("Action.Sprint"),          TEXT("E38: forma canonica profilo Move (D-015/D-116), il codice ha FastMovement") },
		// Contenuto che nessun eroe porta (E6).
		{ TEXT("Action.CircularAoE"),     TEXT("NonAssegnata") },
		{ TEXT("Action.CreateWater"),     TEXT("NonAssegnata: showcase-v0.1 la elenca fra le consegne") },
		{ TEXT("Action.HeavyAttack"),     TEXT("NonAssegnata") },
		{ TEXT("Action.Interact"),        TEXT("NonAssegnata: due delle sette di D-025, mai messa in un kit") },
		{ TEXT("Action.Leap"),            TEXT("NonAssegnata") },
		{ TEXT("Action.LineAttack"),      TEXT("NonAssegnata") },
		{ TEXT("Action.MarkTarget"),      TEXT("NonAssegnata") },
		{ TEXT("Action.PrecisionAttack"), TEXT("NonAssegnata") },
		{ TEXT("Action.Pull"),            TEXT("NonAssegnata") },
		{ TEXT("Action.Push"),            TEXT("NonAssegnata") },
		{ TEXT("Action.Reposition"),      TEXT("NonAssegnata") },
		{ TEXT("Action.Root"),            TEXT("NonAssegnata") },
		{ TEXT("Action.Shield"),          TEXT("NonAssegnata: adr-0003 la da' per arrivata") },
		{ TEXT("Action.Slow"),            TEXT("NonAssegnata") },
		{ TEXT("Action.SuppressiveLine"), TEXT("NonAssegnata") },
	};

	// Anti-vacuita', e non e' teorico: se il roster tornasse vuoto ogni azione risulterebbe non
	// raggiungibile, e con un elenco lungo abbastanza il test resterebbe verde raccontando che va tutto
	// bene. Queste tre righe fanno cadere quel caso.
	TestTrue(TEXT("almeno tredici azioni sono raggiungibili"), Raggiungibili.Num() >= 13);
	TestTrue(TEXT("Guard e' raggiungibile: e' generica"),
		Raggiungibili.Contains(FName(TEXT("Action.Guard"))));
	TestTrue(TEXT("Charge e' raggiungibile: la porta Hero.Riktor.Ram"),
		Raggiungibili.Contains(FName(TEXT("Action.Charge"))));

	int32 Coperte = 0;
	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		const bool bRaggiungibile = Raggiungibili.Contains(Def.ActionId);
		const FString* Ragione = Dichiarate.Find(Def.ActionId);

		// Verso 1 — un'azione nuova senza portatore non passa in silenzio.
		if (!bRaggiungibile && !Ragione)
		{
			AddError(FString::Printf(
				TEXT("%s non e' raggiungibile da nessun kit e non e' dichiarata: assegnala a un eroe, ")
				TEXT("oppure aggiungila all'elenco di questo test con la ragione per cui non lo e'."),
				*Def.ActionId.ToString()));
		}
		// Verso 2 — l'elenco non si fossilizza: chi assegna un'azione toglie la sua riga.
		if (bRaggiungibile && Ragione)
		{
			AddError(FString::Printf(
				TEXT("%s ORA e' raggiungibile ma e' ancora dichiarata come «%s»: togli la riga."),
				*Def.ActionId.ToString(), **Ragione));
		}
		++Coperte;
	}

	// Verso 3 — una voce che non corrisponde a nessuna azione del catalogo e' un residuo.
	for (const TPair<FName, FString>& Voce : Dichiarate)
	{
		const bool bEsiste = URTCatalogLibrary::FindCoreAction(Voce.Key).ActionId == Voce.Key;
		TestTrue(*FString::Printf(TEXT("%s dichiarata esiste ancora nel catalogo"), *Voce.Key.ToString()),
			bEsiste);
	}

	TestTrue(TEXT("il catalogo core non e' vuoto"), Coperte > 30);
	return true;
}
```

- [x] **Step 2: esegui**

Run: `Automation RunTests RefactorTactics.Catalog.EveryCoreActionIsReachableOrDeclared+Quit`
Atteso: **PASS**. Se compare un errore del verso 1, l'inventario della spec era incompleto: aggiungi la
voce **con la ragione vera**, non con `NonAssegnata` per farlo tacere.

- [x] **Step 3: commit**

```bash
git add Source/RefactorTactics/Tests/RTCatalogTests.cpp
git commit -m "test(catalogo): ogni azione core e' raggiungibile o dichiarata, con la sua ragione"
```

---

### Task 7 — Mutazioni, Decision Log, adr-0007

**File:**
- Modifica: `docs/decisions/RT_PDR_00_Decision_Log.md` (voce nuova)
- Modifica: `docs/decisions/adr-0007-attacco-base-per-eroe.md` (nota)

- [x] **Step 1: verifica di mutazione — la derivazione**

Con tutto committato (task 1-6):

```bash
# togli la dichiarazione da Ram e ricompila
# (riga: Ram->Def.DerivedFromActionId, dentro MakeHeroActionFromCore per Action.Charge)
```

Muta `MakeHeroActionFromCore` commentando `Action->Def.DerivedFromActionId = CoreActionId;`.
Run: `Automation RunTests RefactorTactics.Catalog+RefactorTactics.Heroes+Quit`
Atteso: **rossi** su `EveryCoreActionIsReachableOrDeclared` (verso 1, otto azioni) e su
`DerivedActionsDeclareTheirOrigin`. Poi `git checkout` del file — sicuro, perché è committato.

- [x] **Step 2: verifica di mutazione — il verso 2**

Aggiungi `{ TEXT("Action.Guard"), TEXT("NonAssegnata") }` all'elenco.
Atteso: **rosso** del verso 2 su `Action.Guard`. Ripristina.

- [x] **Step 3: verifica di mutazione — l'anti-vacuità**

Fai restituire a `GetHeroRoster()` un array vuoto.
Atteso: rosso su «almeno tredici azioni sono raggiungibili» e su «Charge e' raggiungibile», **non** un
verde da elenco completo. Ripristina.

- [x] **Step 4: la voce del Decision Log**

Leggi l'ultimo `D-nnn` assegnato **e** verifica le PR aperte (`gh pr list --state open`) prima di
prendere il numero: una PR in volo che rivendica lo stesso ID è una collisione. La voce registra:

- il campo nuovo e perché non è `BaseActionId` (D-033 resta intatta);
- la scelta che **la via «base di modulo» non basta** finché l'equipaggiamento non arriva a un'unità;
- la condizione di riapertura: il giorno in cui `ARTUnit` riceve equipaggiamento, le tre voci `Modulo`
  escono dall'elenco e il gate cambia significato.

- [x] **Step 5: la nota in adr-0007**

Una riga che dice che accanto a `BaseActionId` esiste `DerivedFromActionId`, e che le due domande sono
diverse: *«di quale generica sono il profilo»* contro *«da quale azione core eredito i valori»*.

- [x] **Step 6: gate documentali e commit**

```bash
node tools/radar/doc-links.ts --check   # exit 0
node tools/radar/doc-tables.ts --check  # exit 0
git add docs/decisions/
git commit -m "docs: la derivazione e' un dato, e la via dei moduli non basta [D-nnn]"
```

---

## Chiusura

- [x] Suite completa: `Automation RunTests RefactorTactics+Quit` con `-nullrhi`, `Test Completed`
      confrontato col totale e non con zero
- [x] PR verso `main`, con base verificata (`git config branch.<corrente>.parent`)
- [x] Code review, e **attesa** del suo esito prima del merge
- [x] Aggiornamento di [#1403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1403): il
      gate esiste, e la domanda «per scelta o difetto?» è ora una dichiarazione per riga
