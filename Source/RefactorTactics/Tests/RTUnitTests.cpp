#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il cooldown di un'abilita' viene registrato anche senza `BeginPlay` (#135).
 *
 * `AbilityCooldowns` e' un array PARALLELO ad `Abilities`, e veniva dimensionato solo in `ARTUnit::BeginPlay`
 * e in `ConfigureFromHeroData`. I world di test non chiamano `World->BeginPlay()`, quindi per un'unita'
 * configurata come archetipo l'array restava vuoto: `ConsumeAbility` trovava `IsValidIndex` falso e non
 * scriveva nulla, `GetAbilityCooldown` rispondeva 0 — sempre, in ogni test della suite, comunque fosse
 * scritto il codice sotto esame.
 *
 * Non e' solo un limite dell'infrastruttura: l'invariante «i cooldown sono paralleli al kit» vale ovunque il
 * kit venga popolato, non nei soli due percorsi che si ricordavano di risincronizzarlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitArchetypeCooldownTest,
	"RefactorTactics.Unit.ArchetypeKitRecordsCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitArchetypeCooldownTest::RunTest(const FString&)
{
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeBastion());

	// L'abilita' la sceglie il KIT, non un indice scritto a mano: se i numeri dell'archetipo cambiano, il
	// test resta valido invece di verificare la cosa sbagliata in silenzio.
	int32 Index = INDEX_NONE;
	int32 Declared = 0;
	for (int32 i = 0; i < Unit->NumAbilities(); ++i)
	{
		const URTActionData* Ability = Unit->GetAbility(i);
		if (Ability && Ability->CooldownTurns > 0)
		{
			Index = i;
			Declared = Ability->CooldownTurns;
			break;
		}
	}
	if (!TestTrue(TEXT("il kit dichiara almeno un'abilita' con cooldown"), Index != INDEX_NONE))
	{
		return false;
	}

	TestEqual(TEXT("prima del consumo il cooldown e' zero"), Unit->GetAbilityCooldown(Index), 0);

	Unit->ConsumeAbility(Index);

	TestEqual(TEXT("dopo il consumo il cooldown e' quello dichiarato dall'azione"),
		Unit->GetAbilityCooldown(Index), Declared);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTeamColorForTest,
	"RefactorTactics.Unit.TeamColorFor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTeamColorForTest::RunTest(const FString&)
{
	const FLinearColor A(0.10f, 0.40f, 1.00f); // team 0 (blu)
	const FLinearColor B(1.00f, 0.20f, 0.15f); // team 1 (rosso)
	TestTrue(TEXT("team 0 -> A"), ARTUnit::TeamColorFor(0, A, B) == A);
	TestTrue(TEXT("team 1 -> B"), ARTUnit::TeamColorFor(1, A, B) == B);
	TestTrue(TEXT("default (>1) -> B"), ARTUnit::TeamColorFor(5, A, B) == B);
	return true;
}

/**
 * `RingLocalZ`: offset Z locale che porta un anello a terra al piano della cella.
 *
 * ⚠️ **La firma ha perso `ParentScaleZ` con `#593`, e non e' una semplificazione estetica.** Gli anelli
 * erano figli del `Mesh`, che era il root con scala `(1.2, 1.2, 1.8)`: la loro posizione relativa veniva
 * moltiplicata per `1.8`, e la funzione doveva dividere per riportarla al piano. Con un root NEUTRO
 * (`SceneRoot`, scala unitaria) quel fattore non esiste piu': tenere il parametro avrebbe significato
 * conservare un argomento che vale sempre `1`, cioe' un dato che nessun consumatore legge — il difetto
 * che questo progetto insegue da mesi.
 *
 * ⚠️ **E la guardia div-by-zero e' sparita con la divisione**, non per svista: senza denominatore non c'e'
 * niente da proteggere. Il vecchio caso `(90, 0) -> 1` proteggeva da un genitore a scala zero, che oggi e'
 * inesprimibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRingLocalZTest,
	"RefactorTactics.Unit.RingLocalZ",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRingLocalZTest::RunTest(const FString&)
{
	// Cilindro segnaposto: pivot al CENTRO (offset 90) -> l'anello scende di 90 e risale del clearance.
	TestEqual(TEXT("cilindro (90) -> -89"), ARTUnit::RingLocalZ(90.f), -89.f);
	// Skeletal: pivot ai PIEDI (offset 0) -> resta al clearance sopra il piano.
	TestEqual(TEXT("skeletal (0) -> 1"), ARTUnit::RingLocalZ(0.f), 1.f);

	// L'invariante che conta, e che i due valori sopra da soli non esprimono: qualunque sia il pivot,
	// l'anello finisce alla STESSA quota-mondo. Prima non era cosi' per costruzione — lo era solo perche'
	// entrambi i casi venivano moltiplicati per la stessa scala del genitore.
	const float CilindroMondo = 90.f + ARTUnit::RingLocalZ(90.f); // attore a cellZ+90
	const float SkeletalMondo = 0.f + ARTUnit::RingLocalZ(0.f);   // attore a cellZ+0
	TestEqual(TEXT("cilindro e skeletal finiscono alla stessa quota"), CilindroMondo, SkeletalMondo);
	TestTrue(TEXT("e la quota e' SOPRA il piano, non dentro"), CilindroMondo > 0.f);
	return true;
}

/**
 * #593 — **il root di `ARTUnit` e' neutro**, e non porta scala.
 *
 * Il difetto: il cilindro segnaposto era il root con scala `(1.2, 1.2, 1.8)`, quindi **ogni componente
 * aggiunto in Blueprint la ereditava** — una Skeletal Mesh veniva stirata di `1.8/1.2 = 1.5x`, e i quattro
 * `BP_Unit_*` della seduta U7 lo compensavano a mano con «World/Absolute Scale». Un workaround che va
 * rifatto su ogni BP nuovo, e che nessun errore segnala se manca.
 *
 * Questo test pinna la CAUSA, non il sintomo: la deformazione si vede solo in editor, la scala del root si
 * misura qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitRootIsNeutralTest,
	"RefactorTactics.Unit.RootIsNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitRootIsNeutralTest::RunTest(const FString&)
{
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }

	USceneComponent* Root = Unit->GetRootComponent();
	if (!TestNotNull(TEXT("il root esiste"), Root)) { return false; }

	// ⚠️ Il test interroga la PROPRIETA' osservabile, non i membri: `Mesh` e `BaseMeshScale` sono
	// `protected`, e allargarne la visibilita' per un test significherebbe cambiare l'incapsulamento per
	// misurarlo. Cio' che conta e' comunque visibile dall'esterno — quello che un componente aggiunto in
	// Blueprint eredita e' la scala del ROOT, non il nome di chi la porta.

	// (1) Il root non porta scala: e' l'invariante che #593 esiste per stabilire.
	TestEqual(TEXT("scala del root unitaria"), Root->GetRelativeScale3D(), FVector::OneVector);

	// (2) E la scala non e' sparita: e' SCESA di un livello. Senza questa meta' il test passerebbe anche
	// su un'unita' che ha perso il proprio segnaposto — verde, e con il cilindro invisibile.
	//
	// ⚠️ **Si enumera con `GetComponents`, non con `Root->GetAttachChildren()`**, e la differenza e'
	// costata due test rossi: `SetupAttachment` in costruttore imposta l'`AttachParent` del FIGLIO, ma
	// l'array `AttachChildren` del genitore viene popolato alla **registrazione** dei componenti, che su un
	// `NewObject` fuori dal mondo non avviene mai. `GetComponents` legge invece cio' che
	// `CreateDefaultSubobject` ha gia' creato.
	int32 FigliScalati = 0;
	TInlineComponentArray<USceneComponent*> Componenti;
	Unit->GetComponents(Componenti);
	for (const USceneComponent* Comp : Componenti)
	{
		if (Comp && Comp->GetAttachParent() == Root && !Comp->GetRelativeScale3D().Equals(FVector::OneVector))
		{
			++FigliScalati;
		}
	}
	TestTrue(TEXT("almeno un figlio del root porta la scala del segnaposto"), FigliScalati > 0);
	return true;
}

/**
 * #593 — **selezionare un'unita' non scala il root.**
 *
 * `OnSelected` ingrandisce del 15% il cilindro segnaposto: era l'effetto voluto quando il cilindro ERA il
 * root, ed e' diventato un artefatto quando sotto c'e' un personaggio — che si gonfiava del 15% al click e
 * tornava indietro al deselect.
 *
 * ⚠️ Il test NON verifica che `Mesh` non scali: quello e' il comportamento voluto e resta. Verifica che la
 * scala **non arrivi al root**, che e' il punto da cui i figli in Blueprint la ereditano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitSelectionDoesNotScaleRootTest,
	"RefactorTactics.Unit.SelectionDoesNotScaleRoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitSelectionDoesNotScaleRootTest::RunTest(const FString&)
{
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	USceneComponent* Root = Unit->GetRootComponent();
	if (!TestNotNull(TEXT("il root esiste"), Root)) { return false; }

	// Somma delle scale dei figli: cambia quando il segnaposto si ingrandisce, senza nominare `Mesh`
	// (che e' `protected`) ne' `BaseMeshScale`.
	// ⚠️ `GetComponents` e non `GetAttachChildren`, per la ragione scritta in `RootIsNeutral`: fuori dal
	// mondo il secondo e' vuoto, e il test misurerebbe zero contro zero — verde, e cieco.
	auto ScalaDeiFigli = [Unit, Root]()
	{
		FVector Somma = FVector::ZeroVector;
		TInlineComponentArray<USceneComponent*> Componenti;
		Unit->GetComponents(Componenti);
		for (const USceneComponent* Comp : Componenti)
		{
			if (Comp && Comp->GetAttachParent() == Root) { Somma += Comp->GetRelativeScale3D(); }
		}
		return Somma;
	};

	const FVector AllInizio = ScalaDeiFigli();

	Unit->OnSelected();
	TestEqual(TEXT("selezionata: il root resta unitario"), Root->GetRelativeScale3D(), FVector::OneVector);
	// Il segnaposto invece SI' ingrandisce: e' il riscontro visivo, e non deve sparire. Senza questa
	// asserzione il test passerebbe anche su un `OnSelected` svuotato.
	TestTrue(TEXT("qualcosa sotto il root si ingrandisce davvero"),
		!ScalaDeiFigli().Equals(AllInizio));

	Unit->OnDeselected();
	TestEqual(TEXT("deselezionata: il root resta unitario"), Root->GetRelativeScale3D(), FVector::OneVector);
	TestEqual(TEXT("e la scala dei figli torna com'era"), ScalaDeiFigli(), AllInizio);
	return true;
}


// ---------------------------------------------------------------------------------------------------------
// Scudo temporaneo: le abilita' di supporto danno protezione per UN turno, non permanente (issue #96)
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTemporaryShieldExpiresTest,
	"RefactorTactics.Unit.TemporaryShieldExpiresAtCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTemporaryShieldExpiresTest::RunTest(const FString&)
{
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->MaxHealth = 100;
	Unit->Health = 100;
	Unit->Shield = 20; // scudo BASE dell'unita': non scade

	Unit->AddTemporaryShield(40);
	TestEqual(TEXT("lo scudo temporaneo si somma a quello base"), Unit->Shield, 60);

	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("a fine turno resta solo lo scudo base"), Unit->Shield, 20);

	// Due applicazioni nello stesso turno si sommano, e scadono insieme.
	Unit->AddTemporaryShield(25);
	Unit->AddTemporaryShield(15);
	TestEqual(TEXT("piu' scudi temporanei si sommano"), Unit->Shield, 60);
	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("scadono tutti insieme"), Unit->Shield, 20);

	// Senza scudo temporaneo la scadenza non tocca nulla.
	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("nessun temporaneo: lo scudo base resta intatto"), Unit->Shield, 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTemporaryShieldConsumedFirstTest,
	"RefactorTactics.Unit.DamageConsumesTemporaryShieldFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTemporaryShieldConsumedFirstTest::RunTest(const FString&)
{
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->MaxHealth = 100;
	Unit->Health = 100;
	Unit->Shield = 20;
	Unit->AddTemporaryShield(40); // totale 60, di cui 40 temporanei

	// Il danno erode PRIMA la parte temporanea: e' quella che sta per scadere comunque, e cosi' lo scudo
	// base dell'unita' non viene consumato finche' c'e' protezione destinata a sparire.
	Unit->ApplyCombatState(/*NewHealth*/ 100, /*NewShield*/ 30); // 30 danni assorbiti
	TestEqual(TEXT("lo scudo totale scende col danno"), Unit->Shield, 30);

	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("il temporaneo residuo (10) scade, il base (20) resta"), Unit->Shield, 20);

	// Danno che supera l'intero scudo: dopo la scadenza non resta nulla (anche il base e' stato consumato).
	Unit->AddTemporaryShield(40);
	Unit->ApplyCombatState(/*NewHealth*/ 90, /*NewShield*/ 0); // scudo esaurito, il resto agli HP
	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("scudo esaurito dal danno: resta 0, non un valore negativo"), Unit->Shield, 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Gate «questa e' un'azione di scatto»: lo dice il CATALOGO (fase), non un flag scritto a mano (#142)
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogFastMovementIsFoundAsDashTest,
	"RefactorTactics.Unit.CatalogFastMovementIsFoundAsDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogFastMovementIsFoundAsDashTest::RunTest(const FString&)
{
	// Le azioni degli eroi arrivano dal catalogo e dichiarano la FASE, non un flag: fino a #142 nessuna di
	// loro veniva riconosciuta come scatto, quindi il bot non ne pianificava mai uno per i quattro eroi.
	URTHeroData* Bastion = URTHeroCatalogLibrary::MakeBastion();
	if (!TestNotNull(TEXT("Bastion dal catalogo"), Bastion)) { return false; }

	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->ConfigureFromHeroData(Bastion);

	const int32 DashIdx = Unit->FindDashAbilityIndex();
	if (!TestTrue(TEXT("l'unita' riconosce la sua mobilita' rapida"), DashIdx != INDEX_NONE)) { return false; }

	const URTActionData* Dash = Unit->GetAbility(DashIdx);
	if (!TestNotNull(TEXT("l'abilita' trovata esiste"), (void*)Dash)) { return false; }
	TestTrue(TEXT("ed e' proprio la carica di Bastion"), Dash->Def.ActionId == FName(TEXT("Bastion.Ram")));

	// La verifica ha senso solo se il riconoscimento NON passa da un campo dell'asset: e' la fase del
	// catalogo a dirlo. Se un giorno tornasse un flag, questa asserzione cadrebbe insieme al motivo del test.
	TestTrue(TEXT("il gate e' la fase del catalogo"),
		URTCatalogLibrary::MapResolutionPhase(Dash->Def.ResolutionPhase) == ERTMatchPhase::Dash);

	// Un'azione che NON e' mobilita' rapida non deve essere scambiata per uno scatto: senza questa meta' il
	// gate potrebbe rispondere «si'» a tutto e il test passerebbe lo stesso.
	const URTActionData* BasicAttack = Unit->GetAbility(0);
	if (TestNotNull(TEXT("l'attacco base dell'eroe"), (void*)BasicAttack))
	{
		TestTrue(TEXT("l'attacco base non e' uno scatto"), DashIdx != 0
			&& URTCatalogLibrary::MapResolutionPhase(BasicAttack->Def.ResolutionPhase) != ERTMatchPhase::Dash);
	}
	return true;
}

// Chi aggiunge un test in fondo a questo file lo aggiunge PRIMA di questa riga: e' il difetto di #923,
// invisibile in Editor dove la guardia vale 1. Il controllo che lo dimostra e'
// `Build.bat RefactorTactics Win64 Shipping`, non la suite.
#endif // WITH_DEV_AUTOMATION_TESTS
