// Le classi BASE dei widget dello Screen HUD (§4.1, CP 11.7 / #613).
//
// Cio' che questi test possono provare e' la SUPERFICIE: cosa un Blueprint puo' leggere, e cosa non trova
// perche' non esiste. Il layout, l'aspetto e il «centro libero» stanno nel `.uasset` e restano a
// `PIE-V01-HUD` — per costruzione, non per rinuncia.

#include "Misc/AutomationTest.h"
#include "UI/RTScreenHudWidgets.h"
#include "UI/RTIconLibrary.h"
#include "UI/RTIconCatalogData.h" // URTIconCatalogData: esplicito, non ereditato da RTIconLibrary.h
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Turn/RTTurnLog.h" // FRTTurnLogEntry: il feed si prova iniettando una voce nel log
#include "Unit/RTUnit.h" // ARTUnit: e' una delle classi AUTOREVOLI che nessun widget deve esporre
#include "UI/RTReactionWindowViewModel.h" // idem, ed e' quella che porterebbe `SubmitResponse` nel grafo
#include "UI/RTHudViewModel.h"           // il feed si prova anche SOTTO il widget: l'insieme vuoto (#2744)
#include "UI/RTPlayerEventProjector.h"   // IsAuthorized: il predicato si interroga da solo, ed e' il punto
#include "UI/RTHUD.h"                    // ComposeAbilityLine: l'oracolo dell'uguaglianza, non una seconda riga
#include "Misc/ScopeExit.h"              // ON_SCOPE_EXIT: il mondo si distrugge anche sui ritorni anticipati
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "UObject/UObjectIterator.h" // TObjectIterator: le classi widget si interrogano, non si elencano
#include "Blueprint/WidgetTree.h"    // l'albero si COSTRUISCE qui: e' l'unico modo di provare la ricorsione

#if WITH_DEV_AUTOMATION_TESTS

// ⚠️ Le basi NON sono `Abstract`, e questa e' la ragione: `UCLASS()` non si puo' dichiarare in un `.cpp`
// (UHT processa solo gli header), quindi delle sottoclassi concrete di comodo non sono scrivibili qui — e
// un header di test dentro il modulo di gioco costerebbe piu' di quanto valga. Istanziabili direttamente,
// le classi si guidano dal test; in gioco restano comunque da derivare, perche' senza layout non disegnano
// nulla.

namespace
{
	UWorld* MakeHudWidgetWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHudWidgetWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}
}

/**
 * Il contatore di round distingue TRE stati, e i due che si confondono sono quelli che contano.
 *
 * `RoundLimit == 0` non e' «su zero»: una partita senza formato non e' una partita gia' scaduta. E senza
 * contesto non si mostra `Round 0`, che sembra un dato, ma un trattino.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudRoundTextTest,
	"RefactorTactics.ScreenHud.RoundCounterDistinguishesNoLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudRoundTextTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTTurnHeaderWidget* Header = NewObject<URTTurnHeaderWidget>(World);
	if (!TestNotNull(TEXT("widget"), Header)) { DestroyHudWidgetWorld(World); return false; }

	// 1. Senza contesto: un trattino, non un numero.
	TestEqual(TEXT("senza contesto mostra un trattino"),
		Header->GetRoundCounterText().ToString(), FString(TEXT("—")));
	TestFalse(TEXT("e dichiara di non avere contesto"), Header->HasMatchContext());

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudWidgetWorld(World); return false; }
	Header->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	// 2. Con un limite dichiarato: `Round N/Limite`.
	{
		FRTMatchRules Rules;
		Rules.RoundLimit = 9;
		TM->SetMatchRules(Rules);
		TestTrue(TEXT("col limite, il testo lo mostra"),
			Header->GetRoundCounterText().ToString().Contains(TEXT("/9")));
	}

	// 3. 🔴 Senza limite: NIENTE «/0». E' il caso che un binding ingenuo sbaglia, e si legge come una
	//    partita gia' finita.
	{
		FRTMatchRules Rules;
		Rules.RoundLimit = 0;
		TM->SetMatchRules(Rules);
		const FString Text = Header->GetRoundCounterText().ToString();
		TestFalse(TEXT("senza limite non compare uno «/0»"), Text.Contains(TEXT("/")));
		TestTrue(TEXT("ma il round c'e' comunque"), Text.Contains(TEXT("Round")));
	}

	DestroyHudWidgetWorld(World);
	return true;
}

/**
 * Lo slot azione porta la CHIAVE dell'icona, mai un asset — ed e' la chiave che il catalogo si aspetta.
 *
 * Il caso `None` non e' difensivo: le azioni create in codice prima del motore azioni non hanno `ActionId`,
 * e comporre `UI.Icon.` a vuoto produrrebbe una risoluzione che nomina una chiave mai dichiarata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudIconKeyTest,
	"RefactorTactics.ScreenHud.ActionSlotCarriesAnIconKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudIconKeyTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	if (!TestNotNull(TEXT("widget"), Slot)) { return false; }

	// Senza azione: nessuna chiave.
	TestEqual(TEXT("uno slot vuoto non inventa una chiave"), Slot->GetIconId(), FName(NAME_None));

	FRTAbilityCooldownView Action;
	Action.ActionId = TEXT("Action.Move");
	Slot->SetAction(Action, /*bArmed=*/ true);

	TestTrue(TEXT("lo slot registra l'azione"), Slot->Action.ActionId == FName(TEXT("Action.Move")));
	TestTrue(TEXT("e lo stato armato"), Slot->bArmed);

	// La chiave e' quella che il catalogo si aspetta: si CHIEDE a `MakeIconId`, non si compone qui.
	TestEqual(TEXT("la chiave e' quella del catalogo"),
		Slot->GetIconId(), URTIconLibrary::MakeIconId(TEXT("Action.Move")));

	DestroyHudWidgetWorld(nullptr); // no-op: questo test non ha un mondo
	return true;
}

/**
 * 🔴 **Lo slot dice il TASTO, lo stato armato e il MOTIVO — e li dice in TESTO**, che e' il canale non
 * cromatico che `#2826` chiede: *«uno slot in cooldown o non disponibile e' distinguibile **senza**
 * affidarsi al colore»*.
 *
 * L'oracolo non e' che la riga «ci sia»: sono le tre sottostringhe, ciascuna legata a un campo della vista.
 * Una riga che perdesse il numero passerebbe un test scritto come *«non e' vuota»* — e il giocatore non
 * saprebbe piu' quale tasto arma quello slot.
 *
 * ⚠️ **Il numero e' `AbilityIndex + 1` perche' e' il TASTO, non l'indice.** Vale finche' il dock mostra il
 * solo kit numerato: `URTActionDockWidget::GetActions()` inoltra a `BuildAbilityCooldowns`, che cammina le
 * abilita' dell'unita'. I cinque generici (`G` `B` `C` `X` `Z`) arrivano da `GenericHotkeys()` e non entrano
 * in questa lista; se un giorno ci entrassero, questo test resterebbe verde mentre la riga mostrerebbe un
 * numero che non arma nulla — e allora la lettera va aggiunta **qui insieme** al ramo che la produce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudActionLineTest,
	"RefactorTactics.ScreenHud.ActionSlotLineCarriesKeyArmedAndReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudActionLineTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	if (!TestNotNull(TEXT("widget"), Slot)) { return false; }

	FRTAbilityCooldownView Action;
	Action.ActionId = TEXT("Action.Overload");
	Action.DisplayName = FText::FromString(TEXT("Sovraccarico"));
	Action.AbilityIndex = 3;        // quarto slot -> tasto `4`
	Action.TurnsRemaining = 2;      // in ricarica
	Action.TotalTurns = 3;
	Action.bUsableNow = false;

	Slot->SetAction(Action, /*bArmed=*/ false);
	const FString Spenta = Slot->GetActionLine().ToString();

	TestTrue(TEXT("la riga nomina il tasto che arma lo slot"), Spenta.Contains(TEXT("4. ")));
	TestTrue(TEXT("e il nome dell'azione"), Spenta.Contains(TEXT("Sovraccarico")));
	TestTrue(TEXT("il motivo d'indisponibilita' e' leggibile senza aprire un log"),
		Spenta.Contains(TEXT("(ricarica 2)")));
	TestFalse(TEXT("non armata: nessun prefisso di selezione"), Spenta.StartsWith(TEXT("> ")));

	// Armata: il canale NON cromatico e' il prefisso. E' la meta' che il colore da solo non puo' dare.
	Slot->SetAction(Action, /*bArmed=*/ true);
	const FString Armata = Slot->GetActionLine().ToString();

	TestTrue(TEXT("armata: il prefisso lo dichiara in testo"), Armata.StartsWith(TEXT("> ")));
	TestNotEqual(TEXT("armata e non armata NON sono la stessa riga"), Armata, Spenta);

	// Pronta: il motivo sparisce invece di dire «ricarica 0», che sarebbe un motivo inventato.
	FRTAbilityCooldownView Pronta = Action;
	Pronta.TurnsRemaining = 0;
	Pronta.bUsableNow = true;
	Slot->SetAction(Pronta, /*bArmed=*/ false);

	TestFalse(TEXT("un'azione pronta non porta un motivo"),
		Slot->GetActionLine().ToString().Contains(TEXT("ricarica")));

	DestroyHudWidgetWorld(nullptr); // no-op: questo test non ha un mondo
	return true;
}

/**
 * 🔑 **L'oracolo e' l'UGUAGLIANZA con `ARTHUD::ComposeAbilityLine`, e serve a impedire un secondo
 * produttore** — la stessa disciplina che `#2826` impone al proprio percorso di armamento: *«stesso
 * percorso, non un secondo»*.
 *
 * ⚠️ **Detto onestamente: oggi questo test e' tautologico**, perche' `GetActionLine` inoltra a quella
 * funzione e a null'altro. Non e' un oracolo di contenuto — quello e'
 * `ActionSlotLineCarriesKeyArmedAndReason` — ed e' un **rilevatore di cambiamento**: il giorno in cui
 * qualcuno sostituisse l'inoltro con una composizione locale, o il grafo di `WBP_RT_ActionSlot`
 * concatenasse numero e nome per conto proprio, le due stringhe divergerebbero al primo cambio di formato e
 * questo test cadrebbe. E' l'unica cosa che promette, e la promette su tre forme di vista diverse perche'
 * una sola non distinguerebbe un inoltro da una copia fortunata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudActionLineSameComposerTest,
	"RefactorTactics.ScreenHud.ActionSlotLineIsTheSameComposerAsTheHud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudActionLineSameComposerTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	if (!TestNotNull(TEXT("widget"), Slot)) { return false; }

	FRTAbilityCooldownView InRicarica;
	InRicarica.ActionId = TEXT("Action.Overload");
	InRicarica.DisplayName = FText::FromString(TEXT("Sovraccarico"));
	InRicarica.AbilityIndex = 3;
	InRicarica.TurnsRemaining = 2;
	InRicarica.TotalTurns = 3;

	FRTAbilityCooldownView Pronta;
	Pronta.ActionId = TEXT("Action.Move");
	Pronta.DisplayName = FText::FromString(TEXT("Muovi"));
	Pronta.AbilityIndex = 0;
	Pronta.bUsableNow = true;

	// Terza forma: la vista NUDA, quella che un grafo o un test possono costruire ai default. Se un ramo
	// locale comparisse, e' la forma su cui divergerebbe per prima.
	const FRTAbilityCooldownView Nuda;

	for (const FRTAbilityCooldownView& Vista : { InRicarica, Pronta, Nuda })
	{
		for (const bool bArmed : { false, true })
		{
			Slot->SetAction(Vista, bArmed);
			TestEqual(
				FString::Printf(TEXT("la riga dello slot e' quella dell'HUD (%s, armata=%d)"),
					*Vista.ActionId.ToString(), bArmed ? 1 : 0),
				Slot->GetActionLine().ToString(),
				ARTHUD::ComposeAbilityLine(Vista, bArmed).Text);
		}
	}

	DestroyHudWidgetWorld(nullptr); // no-op: questo test non ha un mondo
	return true;
}

/**
 * 🔴 **Nessuna superficie dei widget e' una texture.** E' la voce di DoD «nessun widget referenzia una
 * texture direttamente» (D-031, `#220`) resa verificabile invece che raccomandata.
 *
 * Il controllo gira sulla REFLECTION, non sul testo del sorgente: una `UPROPERTY` o una `UFUNCTION`
 * aggiunta domani viene vista anche se nessuno rilegge questo file.
 *
 * ⚠️ **Il limite va detto, perche' e' meta' della verita'.** Copre la superficie C++. Un Blueprint derivato
 * puo' sempre aggiungersi una variabile `Texture2D`, e nessun gate lo impedisce: i `.uasset` non sono
 * versionati in questo repository. La guida lo scrive come regola per chi costruisce i `WBP_RT_*`; questo
 * test tiene la parte che il codice controlla.
 *
 * 🔴 **Tre buchi chiusi il 2026-08-26**, e tutti e tre erano del genere che passa inosservato perche' il
 * test era verde:
 *
 *  1. **Le firme delle `UFUNCTION` non erano guardate.** Era il buco piu' grave, perche' l'header dichiara
 *     proprio del tipo di ritorno che *«e' la regola: un `FName` non si puo' collegare a un `Image` senza
 *     passare dal catalogo»*. Un `UFUNCTION(BlueprintPure) UTexture2D* GetIcon()` passava.
 *  2. **I contenitori non erano guardati.** Un `TArray<TObjectPtr<UTexture2D>>` e' un `FArrayProperty`, non
 *     un `FObjectPropertyBase`, e il cast falliva in silenzio.
 *  3. **L'elenco delle classi era scritto a mano**, con un `TestEqual(..., 7)` che sembrava una controprova
 *     e non lo era: fissava la lunghezza della lista letterale, non la copertura. Un ottavo widget non
 *     faceva cadere niente — semplicemente non veniva guardato.
 *
 * Ora le classi si INTERROGANO. La derivazione non puo' essere «cio' che eredita da
 * `URTScreenHudWidgetBase`»: `URTActionSlotWidget` deriva direttamente da `UUserWidget`, e una regola cosi'
 * lo perderebbe proprio mentre sembra piu' rigorosa di una lista.
 */
namespace
{
	/** Vero se il tipo di questa proprieta' e' — o contiene — una `UTexture2D`.
	 *
	 *  Ricorsiva perche' un contenitore nasconde il tipo: la texture puo' stare dentro un `TArray`, la
	 *  chiave o il valore di una `TMap`, un `TSet`. `FObjectPropertyBase` copre gia' hard, soft e weak. */
	bool HudWidgetCarriesTexture(const FProperty* Prop)
	{
		if (!Prop)
		{
			return false;
		}
		if (const FObjectPropertyBase* AsObject = CastField<FObjectPropertyBase>(Prop))
		{
			return AsObject->PropertyClass
				&& AsObject->PropertyClass->IsChildOf(UTexture2D::StaticClass());
		}
		if (const FArrayProperty* AsArray = CastField<FArrayProperty>(Prop))
		{
			return HudWidgetCarriesTexture(AsArray->Inner);
		}
		if (const FSetProperty* AsSet = CastField<FSetProperty>(Prop))
		{
			return HudWidgetCarriesTexture(AsSet->ElementProp);
		}
		if (const FMapProperty* AsMap = CastField<FMapProperty>(Prop))
		{
			return HudWidgetCarriesTexture(AsMap->KeyProp) || HudWidgetCarriesTexture(AsMap->ValueProp);
		}
		return false;
	}

	/**
	 * Il nome della classe AUTOREVOLE che questa proprieta' porta, o vuoto (CP 14.6, `#166`).
	 *
	 * Stessa forma ricorsiva di `HudWidgetCarriesTexture`, e per la stessa ragione: un contenitore nasconde
	 * il tipo. Cio' che cambia e' cosa si cerca — non un asset, ma una **porta**: un widget che raggiunge il
	 * `TurnManager`, un `ARTUnit` o il view model della finestra ha il modo di ricalcolare e di rispondere,
	 * che e' esattamente cio' che §4.1 chiude e che la DoD di CP 14.6 chiede al widget della finestra.
	 */
	FString HudWidgetAuthorityCarried(const FProperty* Prop)
	{
		if (!Prop)
		{
			return FString();
		}
		if (const FObjectPropertyBase* AsObject = CastField<FObjectPropertyBase>(Prop))
		{
			const UClass* Carried = AsObject->PropertyClass;
			if (!Carried)
			{
				return FString();
			}
			if (Carried->IsChildOf(ARTTurnManager::StaticClass())
				|| Carried->IsChildOf(ARTUnit::StaticClass())
				|| Carried->IsChildOf(URTReactionWindowViewModel::StaticClass()))
			{
				return Carried->GetName();
			}
			return FString();
		}
		if (const FArrayProperty* AsArray = CastField<FArrayProperty>(Prop))
		{
			return HudWidgetAuthorityCarried(AsArray->Inner);
		}
		if (const FSetProperty* AsSet = CastField<FSetProperty>(Prop))
		{
			return HudWidgetAuthorityCarried(AsSet->ElementProp);
		}
		if (const FMapProperty* AsMap = CastField<FMapProperty>(Prop))
		{
			const FString FromKey = HudWidgetAuthorityCarried(AsMap->KeyProp);
			return FromKey.IsEmpty() ? HudWidgetAuthorityCarried(AsMap->ValueProp) : FromKey;
		}
		return FString();
	}

	/** I widget NOSTRI: nativi, dentro questo modulo. Un Blueprint caricato non entra — il suo caso e'
	 *  dichiarato fuori scope sopra, e includerlo renderebbe l'esito dipendente da cosa e' in memoria. */
	TArray<UClass*> ModuleHudWidgetClasses()
	{
		TArray<UClass*> Out;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Class = *It;
			if (Class == UUserWidget::StaticClass()
				|| !Class->IsChildOf(UUserWidget::StaticClass())
				|| !Class->HasAnyClassFlags(CLASS_Native))
			{
				continue;
			}
			if (Class->GetOutermost() == URTScreenHudWidgetBase::StaticClass()->GetOutermost())
			{
				Out.Add(Class);
			}
		}
		Out.Sort([](const UClass& A, const UClass& B) { return A.GetName() < B.GetName(); });
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudNoTextureTest,
	"RefactorTactics.ScreenHud.WidgetApiExposesNoTexture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudNoTextureTest::RunTest(const FString&)
{
	const TArray<UClass*> WidgetClasses = ModuleHudWidgetClasses();

	// Controprova della PREMESSA, e stavolta verifica la copertura invece della lunghezza di una lista:
	// i widget dell'HUD devono essere fra quelli trovati. Se l'enumerazione smettesse di funzionare, il
	// test cadrebbe qui invece di passare senza guardare niente.
	//
	// ⚠️ **Due voci mancavano e sono state aggiunte il 2026-09-09** (`#166`): `URTFastDecisionWidget`, che
	// nasce con questo checkpoint, e `URTPlayerEventLogWidget`, che era entrato con `#2697` senza passare di
	// qui. La lista e' scritta a mano e non si aggiorna da sola: chi aggiunge un widget alla famiglia
	// aggiunge una riga anche qui, o la «copertura» resta vera di una famiglia piu' piccola di quella reale.
	const TArray<UClass*> MustBeCovered = {
		URTScreenHudWidgetBase::StaticClass(),
		URTTurnHeaderWidget::StaticClass(),
		URTPlayerEventLogWidget::StaticClass(),
		URTTeamRosterWidget::StaticClass(),
		URTSelectedUnitPanelWidget::StaticClass(),
		URTActionDockWidget::StaticClass(),
		URTActionSlotWidget::StaticClass(),
		URTTacticalHUDWidget::StaticClass(),
		URTFastDecisionWidget::StaticClass(),
	};
	for (UClass* Class : MustBeCovered)
	{
		TestTrue(*FString::Printf(TEXT("l'enumerazione copre %s"), *GetNameSafe(Class)),
			WidgetClasses.Contains(Class));
	}

	int32 Inspected = 0;
	for (const UClass* Class : WidgetClasses)
	{
		// Solo cio' che la classe DICHIARA: `UUserWidget` porta proprieta' sue (brush di stile, cursori)
		// che non sono nostre e non sono il difetto che il DoD vieta.
		for (TFieldIterator<FProperty> It(Class, EFieldIterationFlags::None); It; ++It)
		{
			++Inspected;
			if (HudWidgetCarriesTexture(*It))
			{
				AddError(FString::Printf(
					TEXT("%s::%s e' — o contiene — una texture: le icone viaggiano per CHIAVE ")
					TEXT("(`UI.Icon.*`) e si risolvono dal catalogo (D-031). Un widget che riceve un ")
					TEXT("asset rende la regola una raccomandazione."),
					*Class->GetName(), *It->GetName()));
			}
		}

		// E le firme. Il tipo di ritorno E' la regola (§4.1): un `FName` non si puo' collegare a un
		// `Image` senza passare dal catalogo, un `UTexture2D*` si'.
		for (TFieldIterator<UFunction> Fn(Class, EFieldIterationFlags::None); Fn; ++Fn)
		{
			for (TFieldIterator<FProperty> Param(*Fn, EFieldIterationFlags::None); Param; ++Param)
			{
				++Inspected;
				if (HudWidgetCarriesTexture(*Param))
				{
					const bool bIsReturn = Param->HasAnyPropertyFlags(CPF_ReturnParm);
					AddError(FString::Printf(
						TEXT("%s::%s espone una texture (%s `%s`): il tipo di ritorno e' la regola — la ")
						TEXT("chiave si risolve dal catalogo, l'asset no."),
						*Class->GetName(), *Fn->GetName(),
						bIsReturn ? TEXT("valore di ritorno") : TEXT("parametro"), *Param->GetName()));
				}
			}
		}
	}

	// Senza questa riga il test sarebbe verde anche se l'iterazione non vedesse nulla.
	TestTrue(TEXT("l'iterazione ha davvero guardato delle superfici"), Inspected > 0);

	return true;
}

/**
 * 🔴 **«NESSUNA LOGICA DI GIOCO NEL WIDGET» SMETTE DI ESSERE UNA PROMESSA** (CP 14.6, `#166`, voce 2).
 *
 * Fino a qui quella riga di DoD si verificava **leggendo il diff**: nessun test poteva dire se un widget
 * avesse acquistato una porta sul core. Questo test la rende falsificabile, e in due modi che vanno tenuti
 * distinti perche' chiudono difetti diversi.
 *
 * **(1) Nessuna porta, su NESSUN widget della famiglia.** `URTReactionWindowViewModel::SubmitResponse` e'
 * `BlueprintCallable`: un accessore **pubblico** sulla base — la forma che questo checkpoint stava per
 * prendere — metterebbe un nodo che **spara un Overwatch** nel grafo di tutte e sei le classi derivate,
 * roster ed event log compresi. Percio' si guarda l'intera famiglia e non il solo widget nuovo:
 * `GetReactionWindow()` e' `protected` e non riflessa, e questa riga e' cio' che lo mantiene vero.
 *
 * **(2) La risposta non si NOMINA.** `ChooseOption` prende un `int32`. Se prendesse una `FString`, il
 * widget diventerebbe il nono produttore del letterale `FIRE`/`HOLD` — otto siti dello scenario harness lo
 * confrontano `CaseSensitive` — e il primo fuori dai test del core. E un `FRTReactionWindowOptionView` come
 * parametro non basterebbe: un Blueprint ne costruisce uno ai default, con `Response` **vuota**, che il
 * core legge come **scadenza**.
 *
 * ⚠️ **Il limite e' lo stesso di `WidgetApiExposesNoTexture` e va ripetuto**: si vede la superficie C++, non
 * il grafo di un `.uasset`. Un `WBP_RT_*` puo' sempre aggiungersi una variabile — nessun gate lo impedisce,
 * perche' i Blueprint non sono versionati qui. Cio' che questo test garantisce e' che il C++ non gliela
 * **offra**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudFastDecisionNoAuthorityTest,
	"RefactorTactics.ScreenHud.FastDecisionApiCarriesNoAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudFastDecisionNoAuthorityTest::RunTest(const FString&)
{
	// --- 1. NESSUNA PORTA, su tutta la famiglia -----------------------------------------------------
	const TArray<UClass*> WidgetClasses = ModuleHudWidgetClasses();
	if (!TestTrue(TEXT("premessa: l'enumerazione trova il widget della finestra"),
			WidgetClasses.Contains(URTFastDecisionWidget::StaticClass())))
	{
		return false;
	}

	// 🔴 **Si guarda cio' che i Blueprint VEDONO, non cio' che la riflessione elenca — e la distinzione e'
	// stata misurata, non prevista.** La prima stesura iterava tutte le `UPROPERTY` e segnalava tre campi
	// PRIVATI della base: `TurnManager`, `SelectedUnitForTest` e `ReactionWindow`. Sono `UPROPERTY(Transient)`
	// **senza** flag Blueprint, e la `UPROPERTY` li' serve al **GC** — un weak pointer al view model raccolto
	// spegnerebbe il ramo interattivo a partita in corso. Nessun grafo li raggiunge.
	//
	// ⚠️ **Il criterio sbagliato non era «troppo severo»: diceva una cosa falsa.** Il messaggio d'errore
	// affermava *«espone ai Blueprint»* di campi che ai Blueprint non arrivano — e chi lo avesse letto
	// avrebbe tolto la `UPROPERTY` giusta per il motivo sbagliato.
	//
	// ⚠️ Vale anche per le funzioni: solo quelle raggiungibili da un grafo (`BlueprintCallable`,
	// `BlueprintPure`, `BlueprintImplementableEvent`) sono una superficie. Una `UFUNCTION()` nuda esiste per
	// il delegate binding, non per l'autore di un `.uasset`.
	static constexpr uint64 BlueprintReachable =
		FUNC_BlueprintCallable | FUNC_BlueprintEvent;

	int32 Inspected = 0;
	for (const UClass* Class : WidgetClasses)
	{
		for (TFieldIterator<FProperty> It(Class, EFieldIterationFlags::None); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_BlueprintVisible))
			{
				continue;
			}
			++Inspected;
			const FString Carried = HudWidgetAuthorityCarried(*It);
			if (!Carried.IsEmpty())
			{
				AddError(FString::Printf(
					TEXT("%s::%s espone `%s` ai Blueprint: e' una PORTA sul core. §4.1 chiude questa ")
					TEXT("strada — se non c'e' il puntatore, non c'e' il modo di ricalcolare — e per la ")
					TEXT("finestra di reazione significherebbe un nodo che risponde `FIRE` nel grafo di ")
					TEXT("un widget che non deve poterlo fare."),
					*Class->GetName(), *It->GetName(), *Carried));
			}
		}

		for (TFieldIterator<UFunction> Fn(Class, EFieldIterationFlags::None); Fn; ++Fn)
		{
			if ((Fn->FunctionFlags & BlueprintReachable) == 0)
			{
				continue;
			}
			for (TFieldIterator<FProperty> Param(*Fn, EFieldIterationFlags::None); Param; ++Param)
			{
				++Inspected;
				const FString Carried = HudWidgetAuthorityCarried(*Param);
				if (!Carried.IsEmpty())
				{
					AddError(FString::Printf(
						TEXT("%s::%s espone `%s` nella propria firma: la presentazione riceve VISTE, ")
						TEXT("non attori."),
						*Class->GetName(), *Fn->GetName(), *Carried));
				}
			}
		}
	}
	TestTrue(TEXT("l'iterazione ha davvero guardato delle superfici"), Inspected > 0);

	// ⛔ **Controprova della PREMESSA, e senza di essa il punto 1 sarebbe verde a vuoto.** I tre campi che
	// hanno fatto cadere la prima stesura devono esistere e **non** essere Blueprint-visibili: e' il fatto
	// su cui poggia tutto il filtro qui sopra, e un giorno qualcuno potrebbe marcarne uno `BlueprintReadOnly`
	// «per comodita' di debug».
	for (const TCHAR* Nome : { TEXT("TurnManager"), TEXT("SelectedUnitForTest"), TEXT("ReactionWindow") })
	{
		const FProperty* Prop =
			URTScreenHudWidgetBase::StaticClass()->FindPropertyByName(FName(Nome));
		if (TestNotNull(*FString::Printf(TEXT("la base dichiara `%s`"), Nome), Prop))
		{
			TestFalse(
				*FString::Printf(TEXT("`%s` NON e' visibile ai Blueprint: e' riflessa per il GC"), Nome),
				Prop->HasAnyPropertyFlags(CPF_BlueprintVisible));
		}
	}

	// --- 2. LA RISPOSTA NON SI NOMINA: `ChooseOption` prende un indice ------------------------------
	const UFunction* Choose =
		URTFastDecisionWidget::StaticClass()->FindFunctionByName(TEXT("ChooseOption"));
	if (!TestNotNull(TEXT("`ChooseOption` e' esposta ai Blueprint"), Choose))
	{
		return false;
	}

	int32 Parametri = 0;
	for (TFieldIterator<FProperty> Param(Choose, EFieldIterationFlags::None); Param; ++Param)
	{
		if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}
		++Parametri;
		TestTrue(
			*FString::Printf(TEXT("`ChooseOption::%s` e' un intero, non una risposta da comporre"),
				*Param->GetName()),
			CastField<FIntProperty>(*Param) != nullptr);
	}
	TestEqual(TEXT("`ChooseOption` prende un parametro solo: l'indice"), Parametri, 1);

	// --- 3. E nessuna funzione del widget accetta una stringa --------------------------------------
	// E' la meta' che chiude la porta di lato: un secondo mutatore che prendesse una `FString` renderebbe
	// il punto 2 vero e la regola falsa.
	for (TFieldIterator<UFunction> Fn(URTFastDecisionWidget::StaticClass(), EFieldIterationFlags::None);
		 Fn; ++Fn)
	{
		if ((Fn->FunctionFlags & BlueprintReachable) == 0)
		{
			continue;
		}
		for (TFieldIterator<FProperty> Param(*Fn, EFieldIterationFlags::None); Param; ++Param)
		{
			if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				continue;
			}
			if (CastField<FStrProperty>(*Param) || CastField<FNameProperty>(*Param))
			{
				AddError(FString::Printf(
					TEXT("`URTFastDecisionWidget::%s` accetta `%s` come testo: la risposta si INDICA per ")
					TEXT("indice, non si nomina. `FIRE:<indice>` e' un formato con un solo produttore ")
					TEXT("(`URTReactionOpportunityLibrary::FireResponse`)."),
					*Fn->GetName(), *Param->GetName()));
			}
		}
	}

	return true;
}

/**
 * ⛔ **SENZA VIEW MODEL IL WIDGET NON MOSTRA NIENTE, e non esplode** (CP 14.6, `#166`).
 *
 * 🔴 **Non e' un caso limite: e' il percorso normale per qualche frame.** `ARTGameMode::BeginPlay` presenta
 * l'HUD prima di spawnare il `TurnManager`, e `URTFrontendNavigator::PresentMatchHud` crea i widget con
 * `CreateWidget(GameInstance, ...)` — quindi puo' non esserci ancora un proprietario da cui risolvere la
 * finestra. Un widget che in quel momento chiamasse `GetWindow()` su un puntatore nullo chiuderebbe la
 * partita al primo frame.
 *
 * ⚠️ **Il residuo e' `-1`, non `0`, e la differenza e' semantica**: zero direbbe «scaduta adesso», che e' lo
 * stato in cui il countdown NON deve piu' accettare input. E' la convenzione di
 * `FRTMatchHeaderView::PlanningSecondsRemaining`, tenuta uguale di proposito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudFastDecisionEmptyTest,
	"RefactorTactics.ScreenHud.FastDecisionWithoutAWindowShowsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudFastDecisionEmptyTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTFastDecisionWidget* Widget = NewObject<URTFastDecisionWidget>(World);
	if (!TestNotNull(TEXT("widget"), Widget)) { DestroyHudWidgetWorld(World); return false; }

	TestFalse(TEXT("nessuna finestra da disegnare"), Widget->IsWindowOpen());

	const FRTReactionWindowView Vista = Widget->GetWindow();
	TestFalse(TEXT("la vista e' ai default"), Vista.bOpen);
	TestEqual(TEXT("e non offre nessuna opzione da premere"), Vista.Options.Num(), 0);
	TestTrue(TEXT("il residuo dice «nessuna finestra», non «scaduta adesso»"),
		Widget->GetRemainingSeconds() < 0.f);

	// Un click senza finestra non deve fare nulla ne' esplodere: e' il ramo che un `.uasset` prende ogni
	// volta che un bottone sopravvive di un frame alla propria finestra. ⛔ Nessuna warning attesa qui: si
	// esce PRIMA di valutare l'indice, perche' senza view model non c'e' un elenco su cui giudicarlo.
	Widget->ChooseOption(0);
	TestFalse(TEXT("e dopo il click continua a non esserci nessuna finestra"), Widget->IsWindowOpen());

	DestroyHudWidgetWorld(World);
	return true;
}

/**
 * Il dock spento e' uno stato REALE, non un caso limite: e' il neutro di D-128, quello in cui nessuna azione
 * e' armata e un click su un nemico ispeziona. Senza selezione vale lo stesso, e le due cose coincidono di
 * proposito — a schermo il giocatore deve vedere la stessa cosa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudDockNeutralTest,
	"RefactorTactics.ScreenHud.ActionDockShowsTheNeutralState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudDockNeutralTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTActionDockWidget* Dock = NewObject<URTActionDockWidget>(World);
	if (!TestNotNull(TEXT("widget"), Dock)) { DestroyHudWidgetWorld(World); return false; }

	TestEqual(TEXT("senza selezione nessuna azione e' armata"),
		Dock->GetArmedActionIndex(), (int32)INDEX_NONE);
	TestEqual(TEXT("e il dock e' vuoto"), Dock->GetActions().Num(), 0);

	DestroyHudWidgetWorld(World);
	return true;
}

/**
 * `GetIconCatalog` risale alla radice, e la radice legge SE STESSA.
 *
 * 🔴 **I tre rami sono asseriti insieme perche' due di loro si coprono a vicenda in modo ingannevole.**
 * Un'implementazione col solo `GetTypedOuter` passerebbe il caso del dock e fallirebbe **solo** sul
 * `TacticalHUD` — cioe' l'unico widget che il catalogo ce l'ha davvero. Un'implementazione col solo
 * `Cast<>(this)` farebbe l'opposto. Testarne uno alla volta lascerebbe verde meta' del difetto.
 *
 * ⚠️ Il terzo caso — outer senza HUD — non e' un contorno: e' la condizione in cui il widget vive in un
 * test o in un'anteprima d'editor, e la risposta corretta e' `nullptr` **senza crash**. Chi consuma passa
 * da `ResolveIcon`, che con catalogo nullo da' il missing-icon e logga la chiave.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudIconCatalogReachTest,
	"RefactorTactics.ScreenHud.IconCatalogReachesTheRoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudIconCatalogReachTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTIconCatalogData* Catalog = NewObject<URTIconCatalogData>(World);
	URTTacticalHUDWidget* Root = NewObject<URTTacticalHUDWidget>(World);
	if (!TestNotNull(TEXT("catalogo"), Catalog) || !TestNotNull(TEXT("radice"), Root))
	{
		DestroyHudWidgetWorld(World);
		return false;
	}
	Root->IconCatalog = Catalog;

	// (1) La radice legge il PROPRIO catalogo. `GetTypedOuter` cerca fra gli outer e non guarda `this`:
	// senza il ramo dedicato, il widget su cui il dato vive sarebbe l'unico a non vederlo.
	TestEqual(TEXT("la radice legge il proprio catalogo"),
		Root->GetIconCatalog(), (const URTIconCatalogData*)Catalog);

	// (2) Un figlio che ha la radice per outer lo raggiunge risalendo. E' il caso reale del dock innestato
	// nel `WBP_RT_TacticalHUD`.
	const URTActionDockWidget* Figlio = NewObject<URTActionDockWidget>(Root);
	if (TestNotNull(TEXT("figlio della radice"), Figlio))
	{
		TestEqual(TEXT("il figlio risale alla radice"),
			Figlio->GetIconCatalog(), (const URTIconCatalogData*)Catalog);
	}

	// (3) Fuori dall'HUD: `nullptr`, e nessun crash. Controprova della premessa — se questo caso desse il
	// catalogo, i due sopra sarebbero veri per costruzione invece che per meccanismo.
	const URTActionDockWidget* Orfano = NewObject<URTActionDockWidget>(World);
	if (TestNotNull(TEXT("widget fuori dall'HUD"), Orfano))
	{
		TestNull(TEXT("fuori dall'HUD il catalogo non si raggiunge"), Orfano->GetIconCatalog());
	}

	DestroyHudWidgetWorld(World);
	return true;
}

/**
 * Lo slot risolve la propria icona dal catalogo che ha RICEVUTO.
 *
 * 🔴 **La coppia e' il test, non la singola asserzione.** Senza catalogo `ResolveIcon` restituisce
 * `bResolved = false`; con il catalogo giusto `true`. Asserire solo il secondo caso lascerebbe passare
 * un'implementazione che risponde sempre «risolta» — e a schermo si vedrebbe il missing-icon con la
 * barra verde, cioe' il difetto piu' difficile da attribuire.
 *
 * ⚠️ Il test NON verifica che l'icona si veda: `Asset` e' una soft reference, e questa suite non carica
 * texture. La leggibilita' a schermo e' `PIE-ICON-01`, e nessun test la sostituisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotResolvesFromCatalogTest,
	"RefactorTactics.ScreenHud.ActionSlotResolvesFromCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionSlotResolvesFromCatalogTest::RunTest(const FString&)
{
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>();
	if (!TestNotNull(TEXT("slot"), Slot)) { return false; }

	FRTAbilityCooldownView Azione;
	Azione.ActionId = TEXT("Action.Move");
	const FName ChiaveAttesa = URTIconLibrary::MakeIconId(Azione.ActionId);

	// (1) Senza catalogo: il missing-icon, e `bResolved` lo DICHIARA. E' lo stato in cui lo slot vive
	// finche' qualcuno non gli passa il catalogo, ed e' voluto — non un errore da nascondere.
	Slot->SetAction(Azione, /*bArmed=*/ false);
	TestFalse(TEXT("senza catalogo l'icona non e' risolta"), Slot->GetResolvedIcon().bResolved);

	// (2) Con un catalogo che contiene la chiave: risolta.
	URTIconCatalogData* Catalogo = NewObject<URTIconCatalogData>();
	if (!TestNotNull(TEXT("catalogo"), Catalogo)) { return false; }
	Catalogo->Icons.Add(FRTIconDef(ChiaveAttesa, ERTIconCategory::Action,
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Prova/T_Prova.T_Prova")))));

	Slot->SetAction(Azione, /*bArmed=*/ false, Catalogo);
	const FRTIconResolution Risolta = Slot->GetResolvedIcon();
	TestTrue(TEXT("col catalogo l'icona e' risolta"), Risolta.bResolved);
	TestEqual(TEXT("ed e' l'asset dichiarato per quella chiave"),
		Risolta.Asset.ToString(), FString(TEXT("/Game/Prova/T_Prova.T_Prova")));

	// (3) La chiave che lo slot chiede e' quella dell'AZIONE, non una costante: cambiando azione cambia
	// chiave, e una chiave che il catalogo non ha torna non risolta. Senza questo caso, un'implementazione
	// che ignora `Action` e risolve sempre la stessa icona passerebbe i due sopra.
	FRTAbilityCooldownView Altra;
	Altra.ActionId = TEXT("Action.Guard");
	Slot->SetAction(Altra, /*bArmed=*/ false, Catalogo);
	TestFalse(TEXT("un'altra azione chiede un'altra chiave, che il catalogo non ha"),
		Slot->GetResolvedIcon().bResolved);

	return true;
}

/**
 * IL WIDGET DEL FEED LEGGE IL CANALE FILTRATO, E NON NE HA UN ALTRO — `#2697`.
 *
 * 🔴 **E' l'assertion che il difetto originale non aveva.** `GetRecentEventsForTeam` era corretto, testato
 * e verde — e senza chiamanti: un canale che nessuno legge non produce nessun rosso, ed e' la forma di
 * `#2549` e `#2492`. Questo test lega il **consumatore** al filtro: cancellare la chiamata nel widget lo
 * rende rosso, che e' l'unica cosa che il verde precedente non poteva fare.
 *
 * ⛔ **E copre la regressione che sarebbe un LEAK, non un difetto di UI.** Un widget che leggesse il canale
 * non filtrato mostrerebbe fatti che l'osservatore non ha diritto di conoscere. La difesa e' strutturale —
 * `URTScreenHudWidgetBase` non espone il `TurnManager` ai Blueprint, quindi non c'e' una seconda porta da
 * cui passare — ma la struttura si puo' cambiare, e l'assertion no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudEventLogReadsFilteredFeedTest,
	"RefactorTactics.ScreenHud.EventLogWidgetReadsTheFilteredFeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudEventLogReadsFilteredFeedTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTPlayerEventLogWidget* Feed = NewObject<URTPlayerEventLogWidget>(World);
	if (!TestNotNull(TEXT("widget"), Feed)) { DestroyHudWidgetWorld(World); return false; }

	// 1. Senza contesto: nessuna riga, e nessun crash. Un widget costruito prima del manager e' il percorso
	//    normale, non un caso limite — `NativeConstruct` gira prima che l'orchestratore esista.
	TestEqual(TEXT("senza contesto il feed e' vuoto"), Feed->GetFeed().Num(), 0);

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudWidgetWorld(World); return false; }

	// Il tiro rifiutato di un'unita' della squadra 1, con un muro che la SUA squadra conosce.
	FRTTurnLogEntry Rifiutato;
	Rifiutato.Category = ERTLogCategory::Combat;
	Rifiutato.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	Rifiutato.UnitId = 9;
	Rifiutato.SrcCell = FRTCellId(-1, 0, 0);
	Rifiutato.TgtCell = FRTCellId(1, 0, 0);
	Rifiutato.SightBlockerCell = FRTCellId(0, 0, 0);
	Rifiutato.Verdict.AllowTeam(1);
	TM->AppendTurnLogEntryForTest(Rifiutato);

	// 2. L'osservatore AUTORIZZATO legge la riga, e il riferimento nel mondo per trovarla.
	Feed->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 1);
	const TArray<FRTPlayerEventLineView> Autorizzato = Feed->GetFeed();
	if (!TestEqual(TEXT("chi e' autorizzato legge la riga dal widget"), Autorizzato.Num(), 1))
	{
		DestroyHudWidgetWorld(World);
		return false;
	}
	TestFalse(TEXT("e la riga non e' vuota"), Autorizzato[0].Text.IsEmpty());
	TestTrue(TEXT("e porta l'ostacolo da marcare"), Autorizzato[0].bHasBlocker);
	TestTrue(TEXT("nella cella che la voce portava"), Autorizzato[0].BlockerCell == FRTCellId(0, 0, 0));

	// 3. L'altro osservatore, STESSO widget e stesso log: niente. E' la meta' che rende il test una prova
	//    del filtro invece che una prova dell'esistenza.
	Feed->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	TestEqual(TEXT("chi non e' autorizzato non legge nulla"), Feed->GetFeed().Num(), 0);

	DestroyHudWidgetWorld(World);
	return true;
}

namespace
{
	/** Un'unita' in campo, con l'identita' minima che il roster legge. */
	ARTUnit* SpawnRosterUnit(UWorld* World, int32 TeamId, const TCHAR* HeroId)
	{
		ARTUnit* Unit = World->SpawnActor<ARTUnit>();
		if (Unit)
		{
			Unit->TeamId = TeamId;
			Unit->HeroId = FName(HeroId);
			Unit->MaxHealth = 10;
			Unit->Health = 10;
		}
		return Unit;
	}

	/**
	 * Gli `HeroId` di una lista di carte, come stringa unica.
	 *
	 * ⚠️ **Si confronta CHI c'e', non quanti**: un roster rotto che restituisse la squadra 0 due volte
	 * conterebbe anche lui quattro carte. La forma a stringa esiste perche' `TestEqual` non confronta
	 * `TArray`, e perche' il messaggio di fallimento dice subito **quale** elenco e' arrivato.
	 */
	FString HeroIdsOf(const TArray<FRTUnitCardView>& Cards)
	{
		TArray<FString> Ids;
		Ids.Reserve(Cards.Num());
		for (const FRTUnitCardView& Card : Cards) { Ids.Add(Card.HeroId.ToString()); }
		return FString::Join(Ids, TEXT(","));
	}
}

/**
 * La GEMELLA non presidiata di `RosterShowsOnlyOwnTeamAndKeepsTheFallen` (`RTHudScenarioTests.cpp`), e il
 * nome le accoppia di proposito.
 *
 * 🔴 **Senza questa, quel test resta verde e il suo NOME diventa una mezza verita'.** Afferma una regola
 * incondizionata — *«solo la propria squadra»* — che `#2744` rende condizionata alla sessione presidiata; e
 * il suo scenario non arma mai una sessione non presidiata, quindi nessun rosso avviserebbe. Il verde di la'
 * e' il controllo positivo di qui: senza, un roster che mostrasse **sempre** tutto passerebbe.
 *
 * ⚠️ **Si guarda CHI c'e', non quanti.** Un roster rotto che restituisse la squadra 0 due volte conterebbe
 * anche lui quattro carte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudRosterUnattendedTest,
	"RefactorTactics.ScreenHud.RosterSplitsTheTeamsOnlyInAnUnattendedSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudRosterUnattendedTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyHudWidgetWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	SpawnRosterUnit(World, /*TeamId=*/ 0, TEXT("Alfa"));
	SpawnRosterUnit(World, /*TeamId=*/ 0, TEXT("Bravo"));
	SpawnRosterUnit(World, /*TeamId=*/ 1, TEXT("Charlie"));
	SpawnRosterUnit(World, /*TeamId=*/ 1, TEXT("Delta"));

	URTTeamRosterWidget* Roster = NewObject<URTTeamRosterWidget>(World);
	if (!TestNotNull(TEXT("widget"), Roster)) { return false; }
	Roster->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	// ── 1. CONTROLLO POSITIVO: sessione presidiata. Il comportamento di oggi, bit per bit.
	if (!TestFalse(TEXT("premessa: la sessione e' presidiata"), TM->IsUnattendedSession())) { return false; }

	TestEqual(TEXT("presidiata: il roster e' la propria squadra"),
		HeroIdsOf(Roster->GetRoster()), FString(TEXT("Alfa,Bravo")));

	// 🔑 **La riga che vale il test.** Se questa lista non fosse vuota, la lista avversaria esisterebbe
	// anche per chi sta giocando — cioe' il difetto sarebbe stato spostato, non chiuso.
	TestEqual(TEXT("presidiata: non c'e' NESSUNA lista avversaria"), Roster->GetOpposingRoster().Num(), 0);

	// ── 2. Sessione non presidiata: chi guarda non gioca in nessuna delle due.
	TM->SetUnattendedSession(true);

	const FString Propria = HeroIdsOf(Roster->GetRoster());
	const FString Altra = HeroIdsOf(Roster->GetOpposingRoster());

	TestEqual(TEXT("non presidiata: la propria lista non cambia"), Propria, FString(TEXT("Alfa,Bravo")));
	TestEqual(TEXT("non presidiata: l'altra squadra ha la sua lista"), Altra, FString(TEXT("Charlie,Delta")));

	// ── 3. Le due liste sono DISGIUNTE: e' la proprieta' per cui due chiamate a `BuildTeamRoster` possono
	//      comporre senza doppioni, e senza di essa il disegno sarebbe sbagliato invece che solo il codice.
	for (const FRTUnitCardView& Card : Roster->GetOpposingRoster())
	{
		TestFalse(FString::Printf(TEXT("'%s' non compare in entrambe le liste"), *Card.HeroId.ToString()),
			Propria.Contains(Card.HeroId.ToString()));
	}

	return true;
}

/**
 * In autobattle il feed si allarga a entrambe le squadre — e una voce autorizzata a TUTTE E DUE resta UNA.
 *
 * 🔴 **E' il doppione che due proiezioni concatenate produrrebbero**, ed e' il motivo per cui il feed non
 * ha la stessa forma del roster: `BuildTeamRoster` filtra per `TeamId ==` e da' insiemi disgiunti, mentre
 * `AllowsTeam` autorizza per voce e gli insiemi si sovrappongono. La stessa trappola che `ARTHUD::DrawHUD`
 * documenta per gli intenti.
 *
 * ⚠️ Senza la voce pubblica il test sarebbe vacuo: due voci private, una per squadra, passerebbero anche
 * con l'implementazione sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudFeedUnattendedTest,
	"RefactorTactics.ScreenHud.EventFeedWidensToBothTeamsWithoutDuplicatingAnEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudFeedUnattendedTest::RunTest(const FString&)
{
	UWorld* World = MakeHudWidgetWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyHudWidgetWorld(World); };

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	SpawnRosterUnit(World, /*TeamId=*/ 0, TEXT("Alfa"));
	SpawnRosterUnit(World, /*TeamId=*/ 1, TEXT("Charlie"));

	// Un tiro rifiutato che solo la squadra 1 puo' conoscere.
	FRTTurnLogEntry SoloUno;
	SoloUno.Category = ERTLogCategory::Combat;
	SoloUno.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	SoloUno.UnitId = 9;
	SoloUno.SrcCell = FRTCellId(-1, 0, 0);
	SoloUno.TgtCell = FRTCellId(1, 0, 0);
	SoloUno.Verdict.AllowTeam(1);
	TM->AppendTurnLogEntryForTest(SoloUno);

	// Un fatto PUBBLICO: autorizzato a entrambe. E' la voce su cui il doppione si manifesterebbe.
	FRTTurnLogEntry Pubblico;
	Pubblico.Category = ERTLogCategory::Combat;
	Pubblico.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	Pubblico.UnitId = 7;
	Pubblico.SrcCell = FRTCellId(-2, 0, 0);
	Pubblico.TgtCell = FRTCellId(2, 0, 0);
	Pubblico.Verdict.AllowTeam(0);
	Pubblico.Verdict.AllowTeam(1);
	TM->AppendTurnLogEntryForTest(Pubblico);

	URTPlayerEventLogWidget* Feed = NewObject<URTPlayerEventLogWidget>(World);
	if (!TestNotNull(TEXT("widget"), Feed)) { return false; }
	Feed->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	// ── 1. CONTROLLO POSITIVO: presidiata. La squadra 0 vede solo il fatto pubblico.
	if (!TestFalse(TEXT("premessa: la sessione e' presidiata"), TM->IsUnattendedSession())) { return false; }
	TestEqual(TEXT("presidiata: la squadra 0 legge solo il fatto pubblico"), Feed->GetFeed().Num(), 1);

	// ── 2. Non presidiata: si aggiunge la voce dell'altra squadra, e il pubblico NON si sdoppia.
	TM->SetUnattendedSession(true);
	TestEqual(TEXT("non presidiata: due righe, non tre"), Feed->GetFeed().Num(), 2);

	return true;
}

/**
 * L'insieme vuoto e' «nessuno guarda», mai «guardano tutti».
 *
 * 🔴 **Il fail-closed di `AllowsTeam` poteva perdersi passando per un contenitore.** Un
 * `ContainsByPredicate` invertito, o un `if (Ids.IsEmpty()) return true` scritto per «comodita'», darebbe
 * un proiettore che pubblica tutto — e nessun altro test lo direbbe, perche' tutti gli altri passano un
 * insieme non vuoto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudEmptyObserverSetTest,
	"RefactorTactics.ScreenHud.AnEmptyObserverSetAuthorizesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudEmptyObserverSetTest::RunTest(const FString&)
{
	FRTTurnLogEntry Pubblico;
	Pubblico.Category = ERTLogCategory::Combat;
	Pubblico.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	Pubblico.UnitId = 7;
	Pubblico.SrcCell = FRTCellId(-2, 0, 0);
	Pubblico.TgtCell = FRTCellId(2, 0, 0);
	Pubblico.Verdict.AllowTeam(0);
	Pubblico.Verdict.AllowTeam(1);

	const TArray<FRTTurnLogEntry> Log{ Pubblico };

	// Controllo positivo: con un osservatore autorizzato la riga c'e'. Senza, «zero righe» sarebbe vero
	// anche per un log vuoto o per una voce che nessuno classifica.
	TestEqual(TEXT("con un osservatore autorizzato la riga esiste"),
		URTHudViewModel::BuildPlayerEventFeed(Log, TArray<int32>{ 0 }).Num(), 1);

	TestEqual(TEXT("insieme VUOTO: nessuna riga"),
		URTHudViewModel::BuildPlayerEventFeed(Log, TArray<int32>{}).Num(), 0);

	TestFalse(TEXT("e il predicato dice di no anche da solo"),
		URTPlayerEventProjector::IsAuthorized(Pubblico, TArray<int32>{}));

	return true;
}

/**
 * `ComposeMountReport`: il dump dice chi del §4.1 e' stato costruito **e chi manca**.
 *
 * 🔑 **Quello che questo test protegge e' la RICORSIONE**, ed e' la ragione per cui la funzione esiste:
 * `UWidgetTree::ForEachWidget` cammina l'albero di UN Blueprint e si ferma sui `UUserWidget` innestati,
 * che hanno un albero loro — limite dichiarato in `RTMatchWidgetAssetTests.cpp`. Qui l'`ActionSlot` sta
 * DENTRO l'`ActionDock`: se la discesa sparisse, `ActionSlot` conterebbe `0` e questo test diventerebbe
 * rosso. Un dump fermo al primo livello direbbe «manca» di un widget presente, che e' peggio del
 * silenzio da cui veniamo.
 *
 * ⚠️ **Prova il camminatore, non il montaggio reale**: l'albero qui e' costruito a mano. Che una
 * PARTITA monti i sei widget resta `PIE-V01-SCREENHUD`, ed e' precisamente cio' che il dump serve a
 * leggere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScreenHudMountReportTest,
	"RefactorTactics.ScreenHud.MountReportNamesWhoIsMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScreenHudMountReportTest::RunTest(const FString&)
{
	// Senza radice il report PARLA, invece di rendere una lista vuota: una lista vuota si leggerebbe
	// come «nessun problema», che e' il contrario di cio' che sarebbe successo.
	const TArray<FString> SenzaRadice = URTTacticalHUDWidget::ComposeMountReport(nullptr);
	TestTrue(TEXT("senza radice il report dice qualcosa"), SenzaRadice.Num() > 0);

	URTTacticalHUDWidget* Radice = NewObject<URTTacticalHUDWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("radice di prova"), Radice)) { return false; }

	// Un dock, e DENTRO il dock uno slot: due livelli, che e' il minimo per distinguere una discesa
	// ricorsiva da una passata sola.
	Radice->WidgetTree = NewObject<UWidgetTree>(Radice);
	URTActionDockWidget* Dock = Radice->WidgetTree->ConstructWidget<URTActionDockWidget>(
		URTActionDockWidget::StaticClass(), TEXT("DockDiProva"));
	if (!TestNotNull(TEXT("dock di prova"), Dock)) { return false; }
	Radice->WidgetTree->RootWidget = Dock;

	Dock->WidgetTree = NewObject<UWidgetTree>(Dock);
	URTActionSlotWidget* Slot = Dock->WidgetTree->ConstructWidget<URTActionSlotWidget>(
		URTActionSlotWidget::StaticClass(), TEXT("SlotDiProva"));
	if (!TestNotNull(TEXT("slot di prova"), Slot)) { return false; }
	Dock->WidgetTree->RootWidget = Slot;

	const TArray<FString> Report = URTTacticalHUDWidget::ComposeMountReport(Radice);
	const FString Testo = FString::Join(Report, TEXT("|"));

	TestTrue(TEXT("il dock, che sta al primo livello, e' contato"),
		Testo.Contains(TEXT("[ok]    ActionDock: 1")));

	// 🔴 LA RIGA CHE PROTEGGE LA RICORSIONE: lo slot sta nell'albero del dock, non della radice.
	TestTrue(TEXT("lo slot INNESTATO nel dock e' contato: la discesa ricorsiva regge"),
		Testo.Contains(TEXT("[ok]    ActionSlot: 1")));

	// E chi non c'e' viene NOMINATO. E' la meta' del report che il log non sapeva dare: di un widget
	// mancante non si sapeva niente, nemmeno che fosse atteso.
	TestTrue(TEXT("il feed assente e' nominato, non taciuto"),
		Testo.Contains(TEXT("[MANCA] EventLog: 0")));
	TestTrue(TEXT("l'header assente e' nominato"),
		Testo.Contains(TEXT("[MANCA] TurnHeader: 0")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
