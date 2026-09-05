#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Unit/RTUnit.h"
#include "Combat/RTCombatLibrary.h" // BaseShield: il valore lo dichiara il combattimento, non il test
#include "Map/RTMapVisuals.h"
#include "Unit/RTUnitAnimInstance.h" // #288: il grafo di locomozione vive in C++ // #983: si include invece di fidarsi della transitivita' di RTUnit.h

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
	Unit->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeRiktor());

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
	// 🔴 **I due valori si DERIVANO dal clearance invece di ricopiarlo**, e la riga qui sotto e' costata un
	// rosso il 2026-08-28. Il test diceva `-88.2f` e `1.8f`: quando `RingGroundClearance` e' salito a `6.8`
	// per seguire lo spessore del tile, i due letterali sono diventati falsi e il test e' caduto — non
	// perche' la formula fosse sbagliata, ma perche' pinnava un NUMERO invece di una relazione. E' il
	// difetto di #983 un piano piu' in basso, in un test che al difetto di #983 fa da guardia.
	//
	// ⚠️ **E non diventa tautologico**, che e' l'obiezione naturale: `RingLocalZ` restituisce
	// `-VisualZOffset + RingGroundClearance`, quindi cio' che resta verificato e' la FORMA — che il pivot si
	// SOTTRAGGA e il clearance si SOMMI. Scriverla `+VisualZOffset` farebbe cadere la prima riga, invertire i
	// segni la seconda. Il valore assoluto del clearance ha gia' il suo oracolo altrove, ed e' migliore:
	// `RingClearsCellDisc` lo misura contro `RTCellTopZ`, cioe' contro la cosa che deve superare.

	// Cilindro segnaposto: pivot al CENTRO (offset 90) -> l'anello scende di 90 e risale del clearance.
	TestEqual(TEXT("cilindro (90): scende del pivot, risale del clearance"),
		ARTUnit::RingLocalZ(90.f), -90.f + ARTUnit::RingGroundClearance);
	// Skeletal: pivot ai PIEDI (offset 0) -> resta al clearance sopra il piano.
	TestEqual(TEXT("skeletal (0): resta al clearance"),
		ARTUnit::RingLocalZ(0.f), ARTUnit::RingGroundClearance);

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
 * 🔴 **L'anello EMERGE dal disco della cella**, ed e' il test che #593 non aveva e che gli e' costato un
 * difetto vero.
 *
 * Il tile della cella e' il prisma di `GetCellPrismMesh` schiacciato da `RTCellFlatScale`: la sua faccia
 * superiore sta a `RTCellTopZ` — meta' spessore — sopra il centro cella. Un anello il cui bordo superiore
 * resti sotto quella quota e' **dentro** un volume opaco, cioe' invisibile — e a schermo non si distingue
 * da un anello che non e' stato disegnato affatto.
 *
 * ⚠️ **I numeri non si scrivono piu' qui**, e non e' pedanteria: questo commento diceva «`50 * 0.05 = 2.5`»
 * fino al 2026-08-28, quando lo spessore del tile e' passato a `0.06 H` e quella riga e' diventata falsa
 * mentre il test accanto restava verde — perche' il test legge `RTCellTopZ`, il commento no. Un commento che
 * ricopia una costante e' la stessa classe di difetto di #983, un piano piu' in basso.
 *
 * Una stesura di #593 aveva messo `RingGroundClearance = 1.0`, deducendolo dal `+1` della vecchia formula
 * invece che dal disco: faccia dell'anello a `2.0`, mezza unita' sotto. La suite era **verde**, perche'
 * nessun test guardava da questo lato.
 *
 * ✅ **Il `2.5` non e' piu' scritto qui a mano** (#983): `RTCellTopZ` e' uscito dal namespace anonimo di
 * `Map/RTHexMapActor.cpp` e vive in `Map/RTMapVisuals.h`, quindi il giorno in cui lo spessore del disco
 * cambia questo test se ne accorge. Prima questa riga dichiarava di essere una seconda copia e ne accettava
 * il limite.
 *
 * ⚠️ **E non e' diventato ridondante rispetto allo `static_assert` accanto a `RingGroundClearance`**: quello
 * copre il margine con i numeri, questo chiama `RingLocalZ` per **entrambi** i pivot — cilindro e skeletal —
 * cioe' misura la formula che lo `static_assert` deve semplificare per poter esistere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRingClearsCellDiscTest,
	"RefactorTactics.Unit.RingClearsCellDisc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRingClearsCellDiscTest::RunTest(const FString&)
{
	// ✅ **Non e' piu' una copia**: `RTCellTopZ` arriva da `Map/RTMapVisuals.h` (#983). Prima questo numero
	// era un `50.f * 0.05f` scritto qui, che dichiarava sé stesso come seconda copia perche' l'originale
	// viveva in un namespace anonimo e non era raggiungibile.
	constexpr float FacciaDelDisco = RTCellTopZ;

	// Quota-mondo del bordo SUPERIORE dell'anello, per un'unita' col pivot al centro (cilindro).
	const float BordoSuperiore = 90.f + ARTUnit::RingLocalZ(90.f) + ARTUnit::RingHalfHeight;
	TestTrue(FString::Printf(TEXT("l'anello emerge dal disco: %.2f > %.2f"), BordoSuperiore, FacciaDelDisco),
		BordoSuperiore > FacciaDelDisco);

	// E lo stesso per il pivot ai piedi (skeletal): non basta che UNO dei due emerga.
	const float BordoSkeletal = 0.f + ARTUnit::RingLocalZ(0.f) + ARTUnit::RingHalfHeight;
	TestTrue(TEXT("anche col pivot ai piedi l'anello emerge"), BordoSkeletal > FacciaDelDisco);
	return true;
}

/**
 * Il figlio del root che si chiama `Nome`, oppure `nullptr`.
 *
 * ⚠️ **Si enumera con `GetComponents`, non con `Root->GetAttachChildren()`**, e la differenza e' costata
 * due test rossi: `SetupAttachment` in costruttore imposta l'`AttachParent` del FIGLIO, ma l'array
 * `AttachChildren` del genitore viene popolato alla **registrazione** dei componenti, che su un
 * `NewObject` fuori dal mondo non avviene mai. `GetComponents` legge invece cio' che
 * `CreateDefaultSubobject` ha gia' creato.
 *
 * ⚠️ **Per nome e non per puntatore** perche' `Mesh`, `TeamRing` e `SelectionRing` sono `protected`:
 * allargarne la visibilita' per un test significherebbe cambiare l'incapsulamento per misurarlo.
 */
static const USceneComponent* FigliaDelRootChiamata(const AActor* Attore, const TCHAR* Nome)
{
	if (!Attore) { return nullptr; }
	const USceneComponent* Root = Attore->GetRootComponent();
	TInlineComponentArray<USceneComponent*> Componenti;
	Attore->GetComponents(Componenti);
	for (const USceneComponent* Comp : Componenti)
	{
		if (Comp && Comp->GetAttachParent() == Root && Comp->GetFName() == FName(Nome))
		{
			return Comp;
		}
	}
	return nullptr;
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

	// (2) E la scala non e' sparita: e' SCESA sul SEGNAPOSTO. Senza questa meta' il test passerebbe anche
	// su un'unita' che ha perso il proprio cilindro.
	//
	// 🔴 **Una stesura contava «i figli con scala != 1», e non provava niente**: `TeamRing` (1.6,1.6,0.02)
	// e `SelectionRing` (1.9,1.9,0.02) sono figli del root e non unitari, quindi il conteggio era `>= 2`
	// **anche cancellando `Mesh` dal costruttore** — esattamente la regressione che il commento diceva di
	// intercettare. Trovato in code review. Il segnaposto va identificato per NOME.
	const USceneComponent* Segnaposto = FigliaDelRootChiamata(Unit, TEXT("Mesh"));
	if (TestNotNull(TEXT("il segnaposto `Mesh` esiste ed e' figlio del root"), Segnaposto))
	{
		TestTrue(TEXT("e porta lui la scala non uniforme"),
			!Segnaposto->GetRelativeScale3D().Equals(FVector::OneVector));
	}
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

	// 🔴 **Una stesura sommava le scale di TUTTI i figli, e la somma si puo' cancellare**: un `OnSelected`
	// che rimpicciolisse un anello di quanto ingrandisce il segnaposto avrebbe lasciato il totale
	// invariato — e il test verde su un difetto. Peggio: bastava che si ingrandisse un ANELLO invece del
	// cilindro perche' l'asserzione passasse mentre il riscontro visivo era sparito. Trovato in code
	// review. Si misurano i due componenti che contano, per nome.
	const USceneComponent* Segnaposto = FigliaDelRootChiamata(Unit, TEXT("Mesh"));
	const USceneComponent* Anello = FigliaDelRootChiamata(Unit, TEXT("TeamRing"));
	if (!TestNotNull(TEXT("il segnaposto esiste"), Segnaposto)) { return false; }
	if (!TestNotNull(TEXT("l'anello di team esiste"), Anello)) { return false; }

	const FVector SegnapostoPrima = Segnaposto->GetRelativeScale3D();
	const FVector AnelloPrima = Anello->GetRelativeScale3D();

	Unit->OnSelected();
	TestEqual(TEXT("selezionata: il root resta unitario"), Root->GetRelativeScale3D(), FVector::OneVector);
	// Il segnaposto SI' ingrandisce: e' il riscontro visivo, e non deve sparire. Senza questa asserzione
	// il test passerebbe anche su un `OnSelected` svuotato.
	TestTrue(TEXT("il segnaposto si ingrandisce davvero"),
		Segnaposto->GetRelativeScale3D().Z > SegnapostoPrima.Z);
	// E l'anello NO: la sua scala e' assoluta, e la selezione non deve toccarlo.
	TestEqual(TEXT("l'anello di team non cambia scala"), Anello->GetRelativeScale3D(), AnelloPrima);

	Unit->OnDeselected();
	TestEqual(TEXT("deselezionata: il root resta unitario"), Root->GetRelativeScale3D(), FVector::OneVector);
	TestEqual(TEXT("e il segnaposto torna alla propria scala"),
		Segnaposto->GetRelativeScale3D(), SegnapostoPrima);
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
	URTHeroData* Riktor = URTHeroCatalogLibrary::MakeRiktor();
	if (!TestNotNull(TEXT("Riktor dal catalogo"), Riktor)) { return false; }

	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->ConfigureFromHeroData(Riktor);

	const int32 DashIdx = Unit->FindDashAbilityIndex();
	if (!TestTrue(TEXT("l'unita' riconosce la sua mobilita' rapida"), DashIdx != INDEX_NONE)) { return false; }

	const URTActionData* Dash = Unit->GetAbility(DashIdx);
	if (!TestNotNull(TEXT("l'abilita' trovata esiste"), (void*)Dash)) { return false; }
	TestTrue(TEXT("ed e' proprio la carica di Riktor"), Dash->Def.ActionId == FName(TEXT("Hero.Riktor.Ram")));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRingsAreNotCoplanarTest,
	"RefactorTactics.Unit.RingsAreNotCoplanar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRingsAreNotCoplanarTest::RunTest(const FString&)
{
	// 🔴 **Il difetto che questo test esiste per fermare si vedeva solo a schermo.** `TeamRing` (raggio
	// `1.6`) e `SelectionRing` (raggio `1.9`) sono concentrici e stavano alla STESSA quota: con la stessa
	// semi-altezza le facce superiori coincidevano nella corona interna, e l'unita' selezionata lampeggiava
	// fra colore di squadra e colore di selezione. Nessun test lo intercettava.
	for (const float Pivot : { 0.f, 90.f })   // piedi (skeletal) e centro (cilindro): valgono entrambi
	{
		const float Team = ARTUnit::TeamRingLocalZ(Pivot);
		const float Selezione = ARTUnit::SelectionRingLocalZ(Pivot);

		// (1) Non complanari, e con un margine grande almeno quanto la semi-altezza di un anello: una
		// separazione piu' piccola dello spessore lascerebbe le facce dentro la stessa fascia di profondita'.
		TestTrue(*FString::Printf(TEXT("pivot %.0f: le facce non coincidono"), Pivot),
			FMath::Abs(Team - Selezione) >= ARTUnit::RingHalfHeight);

		// (2) E il TEAM sta SOPRA: il colore di squadra e' l'informazione permanente e deve restare leggibile
		// al centro; la selezione fa da cornice esterna, dove il TeamRing non arriva. Invertirli passerebbe
		// il controllo (1) e nasconderebbe l'identita' di squadra a ogni unita' selezionata.
		TestTrue(*FString::Printf(TEXT("pivot %.0f: il TeamRing sta sopra"), Pivot), Team > Selezione);

		// (3) E il piu' BASSO dei due emerge comunque dal disco della cella. Senza questa riga, separare le
		// quote abbassando il SelectionRing passerebbe (1) e (2) e lo farebbe sprofondare nel disco: il
		// margine e' `RingGroundClearance + RingHalfHeight - RTCellTopZ`, cioe' **0.3**.
		TestTrue(*FString::Printf(TEXT("pivot %.0f: anche il piu' basso emerge dal disco"), Pivot),
			Pivot + Selezione + ARTUnit::RingHalfHeight > RTCellTopZ);
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitAnimClipsTest,
	"RefactorTactics.Unit.LocomotionClipsMatchThePacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitAnimClipsTest::RunTest(const FString&)
{
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	if (!TestNotNull(TEXT("CDO dell'AnimInstance"), Cdo)) { return false; }

	// 🔴 **Il valore di questo test sta nelle SEI caselle che non si chiamano come ci si aspetta.**
	// §AS.3b della guida animazioni le ha misurate sul disco: `Cast` regge 4 volte su 4, ma `Idle` e
	// `Jog_Fwd` solo 3, e nessun ruolo tranne l'attacco si trasferisce sempre. Dedurre il nome non
	// funziona mai per un pack intero, e un nome sbagliato non da' nessun errore: la clip semplicemente
	// non si carica e l'unita' resta in posa di riferimento.
	const TMap<FName, TPair<FString, FString>> Attese = {
		{ FName(TEXT("Hero.Gadget")), { TEXT("Idle"),           TEXT("Run_Fwd") } },
		{ FName(TEXT("Hero.Phase")),  { TEXT("Idle"),           TEXT("Jog_Fwd") } },
		{ FName(TEXT("Hero.Riktor")), { TEXT("Idle"),           TEXT("Jog_Fwd") } },
		{ FName(TEXT("Hero.Wraith")), { TEXT("Idle_NonCombat"), TEXT("Jog_Fwd") } },
	};

	TestEqual(TEXT("il default copre i quattro eroi del roster"), Cdo->ClipsPerHero.Num(), Attese.Num());

	for (const TPair<FName, TPair<FString, FString>>& Attesa : Attese)
	{
		const FString Chi = Attesa.Key.ToString();
		const FRTLocomotionClips* Clips = Cdo->FindClipsFor(Attesa.Key);
		if (!TestNotNull(*FString::Printf(TEXT("clip per %s"), *Chi), (const void*)Clips)) { continue; }

		// Il PACK e' parte dell'asserto quanto la clip: lo scambio fra due eroi — l'errore facile in una
		// tabella di quattro righe simili — passerebbe un controllo scritto sul solo nome della clip,
		// perche' tre eroi su quattro condividono `Idle` e `Jog_Fwd`.
		const FString Pack = Chi.RightChop(5);   // `Hero.Gadget` -> `Gadget`
		const FString Radice = FString::Printf(
			TEXT("/Game/FabAsset/Paragon/Paragon%s/Characters/Heroes/%s/Animations/"), *Pack, *Pack);

		const FString VistoIdle = Clips->Idle.ToSoftObjectPath().ToString();
		const FString VistoRun = Clips->Run.ToSoftObjectPath().ToString();

		TestEqual(*FString::Printf(TEXT("%s: idle"), *Chi),
			VistoIdle, FString::Printf(TEXT("%s%s.%s"), *Radice, *Attesa.Value.Key, *Attesa.Value.Key));
		TestEqual(*FString::Printf(TEXT("%s: corsa"), *Chi),
			VistoRun, FString::Printf(TEXT("%s%s.%s"), *Radice, *Attesa.Value.Value, *Attesa.Value.Value));

		// E le due sono DIVERSE: un copia-incolla che lasciasse l'idle anche nella corsa passerebbe i due
		// controlli sopra solo se anche l'attesa fosse sbagliata allo stesso modo, ma questo lo esclude
		// comunque — e un'unita' che corre in idle e' precisamente il difetto che `#288` chiude.
		TestNotEqual(*FString::Printf(TEXT("%s: idle e corsa sono clip diverse"), *Chi), VistoIdle, VistoRun);
	}

	// E l'unita' punta a questo grafo per default: senza, le clip giuste non le legge nessuno.
	const ARTUnit* UnitCdo = GetDefault<ARTUnit>();
	if (TestNotNull(TEXT("CDO dell'unita'"), UnitCdo))
	{
		TestEqual(TEXT("l'unita' usa il grafo C++ per default"),
			UnitCdo->UnitAnimClass.Get(), URTUnitAnimInstance::StaticClass());
	}
	return true;
}

// ─── [D-224] Lo scudo base che torna ────────────────────────────────────────────────────────────────
//
// `RechargeBaseShield` e' l'UNICO punto che stabilisce il valore dello scudo base: lo assegna in
// `BeginPlay` e lo ripristina in coda al Cleanup. Questi test usano `NewObject`, quindi `BeginPlay` non
// gira e lo stato di partenza e' quello che il test scrive — cio' che si misura qui e' la funzione, non
// il ciclo del turno.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitRechargesBaseShieldTest,
	"RefactorTactics.Unit.RechargeRestoresBaseShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitRechargesBaseShieldTest::RunTest(const FString&)
{
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->MaxHealth = 100;
	Unit->Health = 100;
	Unit->Shield = 2; // base erosa da un colpo del turno precedente

	Unit->RechargeBaseShield();
	TestEqual(TEXT("la base torna piena"), Unit->Shield, URTCombatLibrary::BaseShield);
	TestEqual(TEXT("nessun temporaneo introdotto"), Unit->GetTemporaryShield(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitDefeatedDoesNotRechargeTest,
	"RefactorTactics.Unit.DefeatedUnitDoesNotRechargeShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitDefeatedDoesNotRechargeTest::RunTest(const FString&)
{
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->MaxHealth = 100;
	Unit->Health = 0;
	Unit->Shield = 0;

	Unit->RechargeBaseShield();
	TestEqual(TEXT("un'unita' abbattuta non si ricarica"), Unit->Shield, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitRechargeKeepsTemporaryTest,
	"RefactorTactics.Unit.RechargeAddsBaseOnTopOfTemporaryShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitRechargeKeepsTemporaryTest::RunTest(const FString&)
{
	// La ricarica gira DOPO `ExpireTemporaryShield`, quindi in produzione il temporaneo vale sempre 0 qui.
	// Il test esercita il caso che quella posizione rende impossibile, perche' l'invariante
	// `Shield = base + temporaneo` deve reggere anche se un giorno le due chiamate si invertissero.
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->MaxHealth = 100;
	Unit->Health = 100;
	Unit->Shield = 0;

	Unit->AddTemporaryShield(25);
	Unit->RechargeBaseShield();
	TestEqual(TEXT("la base si somma ai 25 temporanei"),
		Unit->Shield, URTCombatLibrary::BaseShield + 25);
	TestEqual(TEXT("il temporaneo non e' stato toccato"), Unit->GetTemporaryShield(), 25);

	// E scadendo lascia esattamente la base.
	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("dopo la scadenza resta la sola base"), Unit->Shield, URTCombatLibrary::BaseShield);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitBaseShieldSurvivesTemporaryExpiryTest,
	"RefactorTactics.Unit.BaseShieldSurvivesPartialTemporaryConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitBaseShieldSurvivesTemporaryExpiryTest::RunTest(const FString&)
{
	// Il caso che vale la pena pinnare: 5 di base + 25 di temporaneo, dieci danni incassati, e a fine
	// turno la base deve essere ANCORA li'. Regge perche' `ApplyCombatState` decrementa il temporaneo in
	// proporzione allo scudo perso — comportamento gia' corretto prima di [D-224] e mai testato.
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->MaxHealth = 100;
	Unit->Health = 100;
	Unit->Shield = URTCombatLibrary::BaseShield;

	Unit->AddTemporaryShield(25);
	TestEqual(TEXT("totale 30"), Unit->Shield, 30);

	const FRTDamageResult Colpo = URTCombatLibrary::ApplyDamage(
		10, ERTDamageSource::Direct, Unit->Shield, Unit->GetTemporaryShield(), Unit->Health);
	Unit->ApplyCombatState(Colpo.Health, Colpo.Shield);
	TestEqual(TEXT("i dieci danni escono dal temporaneo"), Unit->GetTemporaryShield(), 15);

	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("la base e' sopravvissuta alla scadenza"), Unit->Shield, URTCombatLibrary::BaseShield);
	TestEqual(TEXT("HP intatti"), Unit->Health, 100);
	return true;
}



/**
 * **La sagoma dell'ultimo contatto punta all'IDLE dell'eroe, non alla corsa e non al nulla** (#1750).
 *
 * 🔴 Il difetto che chiude: un `USkeletalMeshComponent` con una mesh e senza `AnimInstance` disegna la
 * **posa di riferimento** dello skeleton — la T-pose — e su Riktor quella posa stende le catene attraverso
 * lo schermo. Osservato a schermo il 2026-09-03, dopo che le altre due cause dello stesso sintomo erano
 * state chiuse (#1719 la scala, #1784 le ossa a LOD basso): è l'unica delle tre rimasta.
 *
 * ⚠️ **Questo test asserisce il PATH, non l'asset, e non è pigrizia.** I pack Paragon vivono in
 * `Content/FabAsset/`, che **non è versionato**: su un clone appena creato non ci sono, e
 * `LoadSynchronous` restituisce `nullptr`. Un test che caricasse non potrebbe distinguere *«la clip giusta
 * non si carica»* da *«punto alla clip sbagliata»* — sarebbe rosso per l'ambiente e non per il codice.
 * È la stessa scelta di `Unit.LocomotionClipsMatchThePacks`, che confronta `ToSoftObjectPath()`.
 *
 * 🔴 **E il criterio che il referto di spec-panel proponeva NON è eseguibile qui, misurato.**
 * `sagoma-ultimo-contatto-posa-spec-panel-2026-08-30.md` §3 lo dà per headless:
 *
 * > *«dato un `ARTUnit` con skeletal e posa avanzata di N frame, quando la visibilità passa a `false` e la
 * > sagoma si mostra, allora almeno un osso della sagoma differisce dalla posa di riferimento»*
 *
 * Due cose lo impediscono, entrambe verificate: **la skeletal la aggiunge il Blueprint e non il C++** —
 * `FindHeroSkeletal` cerca fra i componenti, e un `ARTUnit` spawnato da codice non ne ha nessuna — e le
 * **clip non esistono** su un checkout senza i pack. Confrontare le ossa richiederebbe di caricare
 * `BP_Unit_Riktor` **e** i suoi 44 GB di dipendenze: sarebbe un test verde solo sulle macchine che li hanno,
 * cioè un test che dichiara l'ambiente e non il codice.
 * ∴ quel criterio resta la **verifica PIE**, dove è già registrato, e questo test copre ciò che si può
 * asserire senza schermo: che il ripiego sappia **quale** clip usare, per ciascuno dei quattro eroi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitGhostFallbackClipTest,
	"RefactorTactics.Unit.ContactGhostFallbackPointsAtTheIdleClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitGhostFallbackClipTest::RunTest(const FString&)
{
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	if (!TestNotNull(TEXT("CDO dell'AnimInstance"), Cdo)) { return false; }

	// ⛔ **Anti-vacuità, e qui morde davvero**: se il CDO non portasse clip, ogni asserzione sotto
	// confronterebbe due stringhe vuote e passerebbe. Il difetto sarebbe «il ripiego non punta a niente»,
	// cioè esattamente quello che il test deve trovare.
	if (!TestTrue(TEXT("il CDO porta le clip dei quattro eroi"), Cdo->ClipsPerHero.Num() >= 4))
	{
		return false;
	}

	const FName Eroi[] = {
		FName(TEXT("Hero.Gadget")), FName(TEXT("Hero.Phase")),
		FName(TEXT("Hero.Riktor")), FName(TEXT("Hero.Wraith")),
	};

	for (const FName& Eroe : Eroi)
	{
		const FRTLocomotionClips* Clips = Cdo->FindClipsFor(Eroe);
		if (!TestNotNull(*FString::Printf(TEXT("clip di %s"), *Eroe.ToString()), (const void*)Clips))
		{
			continue;
		}

		// 🔑 **Si CHIAMA la funzione del ripiego, non si rilegge il CDO.** La prima stesura di questo
		// test confrontava `Clips->Idle` con `Clips->Run` letti qui: verde, e **vacuo rispetto al fix** —
		// scambiare i due campi dentro `GhostFallbackClipFor` lo lasciava verde, perche' non ci passava.
		// L'ha trovata una verifica di mutazione, non una rilettura del codice.
		const FString Idle = ARTUnit::GhostFallbackClipFor(Cdo, Eroe).ToSoftObjectPath().ToString();
		const FString Run = Clips->Run.ToSoftObjectPath().ToString();

		// (1) Il ripiego ha un bersaglio. Un path vuoto lascerebbe la T-pose, che è il difetto di partenza.
		TestFalse(*FString::Printf(TEXT("%s: l'idle del ripiego non e' vuoto"), *Eroe.ToString()),
			Idle.IsEmpty());

		// (2) 🔑 **È l'IDLE, non la corsa**, e questa è l'asserzione per cui il test esiste. Una sagoma che
		// corre sul posto è peggio di una T-pose: è un ricordo che si muove. Tre eroi su quattro condividono
		// il nome `Idle`, quindi uno scambio fra i due campi passerebbe qualunque controllo scritto sul nome
		// — va confrontato il campo con l'altro campo.
		TestNotEqual(*FString::Printf(TEXT("%s: il ripiego non e' la clip di corsa"), *Eroe.ToString()),
			Idle, Run);
	}

	// (3) Un eroe fuori catalogo non è un errore: `FindClipsFor` dà `nullptr` e la sagoma resta in posa di
	// riferimento, che è il comportamento dichiarato da #287 — *«se una clip manca, l'unità resta in posa di
	// riferimento e la partita si gioca uguale»*. Il ripiego non aggiunge un modo nuovo di fallire.
	TestTrue(TEXT("un eroe fuori catalogo non ha clip di ripiego, e non e' un errore"),
		ARTUnit::GhostFallbackClipFor(Cdo, FName(TEXT("Hero.NonEsiste"))).IsNull());
	// ⛔ E senza clip del tutto: `nullptr` non deve crashare, deve dare un ripiego vuoto.
	TestTrue(TEXT("senza AnimInstance il ripiego e' vuoto invece di crashare"),
		ARTUnit::GhostFallbackClipFor(nullptr, FName(TEXT("Hero.Riktor"))).IsNull());
	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
