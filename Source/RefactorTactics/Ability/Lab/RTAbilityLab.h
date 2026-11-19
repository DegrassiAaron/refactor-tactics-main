// Ability Lab (#2599) — eseguire UNA ability canonica in una fixture deterministica.
//
// ## Perche' e' una facade e non un motore
//
// Lo Scenario Harness esprime gia' tutto cio' che serve: `FRTScenarioUnit` posa caster e bersaglio,
// `FRTScenarioIntent::Ability` dichiara *quale* ability eseguire, `FRTTestScenario::Seed` fissa il
// determinismo, e `URTScenarioRunner::Run` esegue il percorso reale. Il before/after esiste come
// `FRTTestResult::UnitStateDiff`; il TurnLog esiste come `FRTTestResult::TurnTraces`.
//
// ∴ questo file **compone**, non decide. L'invariante che #2599 chiede — *«mai un mini-resolver
// separato»* — qui non e' una convenzione da rispettare: e' strutturale, perche' questa libreria non ha
// un altro modo di far succedere qualcosa. Non esiste una funzione che applichi un effetto, calcoli un
// danno o legga una portata per deciderne l'esito.
//
// ⚠️ **Nessuna formula di gameplay sotto questo header.** Se un giorno ne comparisse una, il difetto non
// sarebbe l'aritmetica: sarebbe la seconda risposta alla stessa domanda.
//
// ## Cosa NON e'
//
// Non e' lo Skill Workbench (`TD 0.3`, #1950, `out_of_release_scope`): quello **autora una variante**
// senza toccare il dato di produzione. Questo **esegue il dato di produzione** e non lo scrive mai.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/Catalog/RTActionReadout.h"
#include "Map/RTCellId.h"
#include "ScenarioHarness/Authoring/RTTestScenario.h"
#include "ScenarioHarness/Report/RTTestResult.h"
#include "RTAbilityLab.generated.h"

/**
 * Una voce del selettore: l'identita' di una ability canonica piu' il minimo che serve a sceglierla.
 *
 * Non porta valori di bilanciamento oltre a forma, portata e raggio — quelli si leggono da
 * `URTActionReadoutLibrary::DescribeActionParameters`, che e' la loro unica casa e sa dire da **dove**
 * ogni numero viene. Duplicarli qui creerebbe la quarta casa che `#1953` ha misurato.
 */
USTRUCT(BlueprintType)
struct FRTAbilityLabEntry
{
	GENERATED_BODY()

	/** `Hero.Gadget.LinearDischarge`, `Action.Move`, ... */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	FName AbilityId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	FText DisplayName;

	/** L'eroe nel cui kit vive. `NAME_None` per un'azione core, che non e' di nessuno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	FName OwnerHeroId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	bool bIsCoreAction = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	ERTAbilityShape Shape = ERTAbilityShape::Single;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	int32 RangeCells = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|AbilityLab")
	int32 AreaRadius = 0;
};

/**
 * Il bersaglio di una scena del banco (`#4072`, [D-607] (2)). `Auto` lo deduce dall'azione; gli altri lo forzano.
 *
 * ⚠️ **E' una scelta dello STRUMENTO, non una regola di gioco**: dice al banco come posare la scena perche' la
 * skill si veda all'opera. Il gameplay non la legge.
 */
UENUM(BlueprintType)
enum class ERTAbilityLabTarget : uint8
{
	/** Lo deduce il banco: la forma da `URTPointerLibrary::TargetKindForAction`, il lato da `BenefitsItsRecipient`. */
	Auto,
	/** Il secondo eroe e' un nemico, e l'azione lo mira. */
	Enemy,
	/** Il secondo eroe e' un alleato, e l'azione lo mira. */
	Ally,
	/** Una cella vuota a portata. */
	Cell,
	/** Il bordo della cella del bersaglio rivolto a chi lancia; con la porta, per un'azione che cambia una porta. */
	Edge,
	/** Nessun bersaglio. */
	None
};

/**
 * Come il `TARGET` provoca una reazione nella scena del banco (`#4073`). Si legge dal CATALOGO, mai dall'id: dal
 * trigger dello slot reazione (`FRTActionDef::ReactionTrigger`), o dall'azione principale che arma una reazione
 * (`URTCatalogLibrary::ArmsReaction`: l'Overwatch), che irrigidisce chi la usa (`Status.Braced`: il Brace) o che predice
 * (`PredictiveTargeting`: `InterceptShot`, `#4088`).
 *
 * ⚠️ Ogni provocazione e' un INTENTO dello scenario, eseguito dal resolver: il banco non applica effetti da se'.
 */
UENUM()
enum class ERTAbilityLabProvocation : uint8
{
	/** Non e' una reazione. */
	None,
	/** `HitByDirectAttack`: il `TARGET` colpisce chi reagisce con un attacco diretto `Single`. */
	DirectHit,
	/** `AllyHitByDirectAttack`: un alleato accanto a chi reagisce, e il `TARGET` colpisce lui. */
	AllyHit,
	/** `AboutToBeDisplaced`, e il Brace: il `TARGET` spinge chi reagisce. */
	Push,
	/** `AboutToReceiveControl`: il `TARGET` applica a chi reagisce uno stato di controllo. */
	Control,
	/** `CellBecameHazardous`: il fuoco nasce sotto chi reagisce. ⛔ Nessuna unita' lo sa accendere ([D-046]). */
	Hazard,
	/** L'Overwatch: il `TARGET` entra nella linea di chi guarda. */
	EnterLine,
	/** Una predittiva (`PredictiveTargeting`): il `TARGET` entra nella cella bloccata (`#4088`). */
	EnterCell
};

/**
 * Come si vuole la fixture. Tutti i campi hanno un default utilizzabile: il caso normale e' non
 * dichiarare niente e ottenere una posa a due unita' con un seed fisso.
 */
USTRUCT(BlueprintType)
struct FRTAbilityLabFixtureSpec
{
	GENERATED_BODY()

	/** Fissa il determinismo. Due `BuildFixture` con lo stesso seed producono lo stesso scenario. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	int32 Seed = 1;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	int32 MapRadius = 3;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	FRTCellId CasterCell = FRTCellId(0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	FRTCellId TargetCell = FRTCellId(2, 0);

	/**
	 * La cella d'arrivo di uno SCATTO (`#3773`): un'azione di fase Dash si dichiara con `Dash`/`DashCell`, non
	 * come abilita'. Il default sta oltre il bersaglio, sulla stessa riga del caster: e' la posa di
	 * `Visual.Ability.ClipIvrin`, e da li' chi attraversa (`PassingBlade`) attraversa e chi carica (`Ram`) si
	 * ferma sul primo nemico.
	 *
	 * ⚠️ **E' una posa, non un calcolo**: il Lab non la ricava dal bersaglio, e dove lo scatto si ferma lo decide il
	 * resolver. Chi sposta `TargetCell` sposta anche questa, se vuole che lo scatto incontri il bersaglio.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	FRTCellId DashCell = FRTCellId(3, 0);

	/**
	 * L'eroe che subisce. Deve essere diverso dal caster, altrimenti la posa mette due unita' della
	 * stessa identita' in due squadre — leggibile per il motore, confondente per chi guarda. Per una REAZIONE e' solo
	 * una preferenza per chi la provoca (`#4073`): se il suo attacco base non la provoca, il banco ne sceglie un altro.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	FName TargetHeroId = FName(TEXT("Hero.Ivrin"));

	/** Il bersaglio della scena: `Auto` lo deduce dall'azione, gli altri valori lo forzano ([D-607] (2), `#4072`). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	ERTAbilityLabTarget Target = ERTAbilityLabTarget::Auto;

	/**
	 * Quanti bersagli la scena vuole, da 1 a `URTAbilityLabLibrary::MaxLabTargets()` (`#4136`): oltre il `TARGET` il banco posa
	 * `TARGET2` e `TARGET3`, nelle celle che l'azione colpisce (`URTHexCombatLibrary::HexHitCells`). Un valore fuori scala si
	 * porta fra 1 e il massimo.
	 *
	 * ⚠️ **E' una POSA dello strumento, non una regola di gioco**: il banco sceglie le celle dove stare, ma chi viene colpito
	 * lo decide comunque il resolver. Un'azione che colpisce una cella sola (`Single`), uno scatto e una reazione restano a un
	 * bersaglio; e se le celle o gli eroi finiscono, se ne posano quanti ce ne stanno. Il numero CHIESTO sta nel tag della scena
	 * (`lab-targets-N`, `URTAbilityLabLibrary::RequestedTargetsOf`); quelli POSATI si contano dalle unita'
	 * (`URTAbilityLabLibrary::PlacedTargetsOf`).
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|AbilityLab")
	int32 TargetCount = 1;
};

/**
 * Il bersaglio che il banco ha scelto per una scena (`#4072`): cosa mira l'azione, e cosa la scena deve posare
 * perche' la mira abbia senso. Lo produce `URTAbilityLabLibrary::ChooseTarget`, lo consuma `BuildFixture`.
 *
 * Struct C++ nuda, come `FRTTestResult`: e' un dettaglio della costruzione della scena, non un dato del formato.
 */
struct FRTAbilityLabTargetChoice
{
	/** Mai `Auto`: e' la scelta gia' fatta. */
	ERTAbilityLabTarget Kind = ERTAbilityLabTarget::Enemy;

	/**
	 * Il secondo eroe della scena sta nella squadra di chi lancia: per `Ally`, e per una `Cell` di un'azione che fa
	 * bene a chi la riceve — una cura ad area (`Hero.Muiren.CircularTide`) deve prendere un alleato, non un nemico.
	 */
	bool bSecondHeroIsAlly = false;

	/** La cella mirata, per `Cell` ed `Edge`. */
	FRTCellId Cell;

	/** Per `Edge`: il lato di `Cell` rivolto a chi lancia. */
	ERTHexDirection Edge = ERTHexDirection::E;

	/** Per `Edge` su un'azione che cambia una porta (`ERTStructureOp::SetDoorState`): la scena posa una porta chiusa. */
	bool bPlacesDoor = false;

	/** La porta, dichiarata dalla parte di chi lancia, come nel corpus (`Spec/Map/InteractClosesOpenDoor`). */
	FRTCellId DoorCell;
	ERTHexDirection DoorEdge = ERTHexDirection::E;
};

UCLASS()
class REFACTORTACTICS_API URTAbilityLabLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * L'insieme canonico delle ability: l'**unione** dei kit d'eroe (`URTHeroCatalogLibrary::GetHeroRoster`
	 * → `URTHeroData::Actions`) e del catalogo core (`URTCatalogLibrary::GetCoreActionCatalog`).
	 *
	 * ⚠️ Sono due sorgenti e non una: il corpo di #2599 diceva *«via `URTCatalogLibrary`»*, e quella
	 * libreria conosce solo le azioni core. Un selettore costruito su di essa non mostrerebbe **nessuna**
	 * ability d'eroe — cioe' esattamente cio' che l'Ability Lab esiste per eseguire.
	 *
	 * Ordine stabile: prima i kit nell'ordine del roster, poi le core nell'ordine del catalogo. Nessun
	 * `Sort` su `FName`, che varia con l'ordine di creazione dei nomi.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|AbilityLab")
	static TArray<FRTAbilityLabEntry> ListCanonicalAbilities();

	/** La voce di `AbilityId`, se canonica. `false` senza toccare `OutEntry` quando non lo e'. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|AbilityLab")
	static bool FindAbility(const FName& AbilityId, FRTAbilityLabEntry& OutEntry);

	/**
	 * Il readout dei parametri, delegato per intero a `URTActionReadoutLibrary`.
	 *
	 * Non riassume e non sceglie: il designer vede **entrambe** le case di ogni numero — cio' che il
	 * catalogo dichiara e cio' che il consumatore legge — perche' uno strumento che ne scegliesse una
	 * mostrerebbe un valore che il gioco puo' non usare.
	 *
	 * Il «letto» di una generica e' quello dell'istanza che ogni unita' riceve (`MakeGenericActions`).
	 * Una core che nessuna unita' porta col suo id rende `CatalogOnly` — `#3473`: esiste solo la casa del
	 * catalogo, e in `OutParameters` vanno letti i soli `DeclaredValue`. ⚠️ La funzione e' `BlueprintCallable`:
	 * chi la chiama legge l'esito prima dei campi, come fa `SRTLabPanel::TestoParametri`.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|AbilityLab")
	static ERTActionReadoutResult DescribeAbility(const FName& AbilityId,
		TArray<FRTActionParameterView>& OutParameters);

	/**
	 * Costruisce lo scenario che esegue `AbilityId` una volta sola.
	 *
	 * **Fail closed**: se `AbilityId` non e' canonica, ritorna `false`, scrive `OutError` e lascia
	 * `OutScenario` intatto. Non produce una fixture parziale — una fixture a meta' verrebbe eseguita, e
	 * il suo esito sarebbe indistinguibile da quello di un'ability che semplicemente non fa nulla.
	 *
	 * Stesso rifiuto, e stessa forma, per una core che **nessuna unita' impugna** — `#3472`: un'unita' porta il
	 * proprio kit e le generiche, quindi una core fuori da quell'insieme finirebbe rifiutata dall'harness in
	 * ERROR. Uno SCATTO (fase Dash) si dichiara con `Dash` e la `DashCell` della spec, mai come abilita'
	 * (`#3773`). Una REAZIONE dello slot si arma (`Intent.Reaction`) e la provoca il `TARGET` (`ProvocationFor`,
	 * `#4073`); il suo bersaglio non si forza. Ogni altra azione mira il bersaglio di `ChooseTarget` (`#4072`): nessuno,
	 * un nemico, un alleato, una cella o un bordo, con la porta per un'azione che ne cambia una. Una predittiva blocca la
	 * cella di `ChooseTarget` e il `TARGET` ci entra (`#4088`); il suo bersaglio si forza solo a «cella», e il banco rifiuta gli altri.
	 *
	 * `Spec.TargetCount` chiede fino a `MaxLabTargets()` bersagli (`#4136`): gli altri sono `TARGET2` e `TARGET3`, nelle celle che
	 * `URTHexCombatLibrary::HexHitCells` dice colpite dall'azione, e per una linea mirata a un'unita' il `TARGET` si sposta in
	 * fuori finche' la linea ha le celle che servono. Il numero chiesto sta sempre nel tag `lab-targets-N`.
	 *
	 * Lo scenario porta sempre almeno una `Expect`: l'harness **rifiuta** uno scenario senza assertion,
	 * perche' passerebbe sempre. L'assertion di default e' `TurnsCompleted == 1` — dice che il turno e'
	 * stato giocato, e non pretende un esito di gameplay che il Lab non ha il compito di decidere.
	 */
	// ⛔ **Deliberatamente NON una `UFUNCTION`.** `FRTTestScenario` e' `USTRUCT()` e **non**
	// `BlueprintType`: ADR-0010 tiene le nove struct del formato fuori dalla portata di Blueprint, e il
	// gate `RefactorTactics.Scenario.AuthoringContractIsReachableFromBlueprint` verifica **entrambi i
	// versi**. Esporre questa firma renderebbe rosso quel gate — cioe' il Lab comprerebbe una comodita'
	// d'editor al prezzo dell'invariante che lo Scenario Harness esiste per difendere.
	static bool BuildFixture(const FName& AbilityId, const FRTAbilityLabFixtureSpec& Spec,
		FRTTestScenario& OutScenario, FString& OutError);

	/**
	 * Il bersaglio adatto a un'azione, nella posa della spec (`#4072`, [D-607] (2)). Funzione pura: non posa niente.
	 *
	 * - **La forma** la decide la funzione che il gioco usa gia', `URTPointerLibrary::TargetKindForAction`: nessuno,
	 *   unita', cella o bordo. Il banco non ne scrive una seconda.
	 * - **Un'unita'** e' un alleato se l'azione fa bene a chi la riceve (`BenefitsItsRecipient`), altrimenti un nemico.
	 *   Una REAZIONE mira sempre un nemico: la provoca lui (`#4073`), e i suoi effetti li riceve chi reagisce.
	 * - **Una cella** e' vuota e a portata (`RangeCells` e `MinRangeCells`, come le legge il resolver), accanto al
	 *   bersaglio sulla linea da chi lancia: prima oltre, cosi' un'area lo prende senza prendere chi lancia; poi prima.
	 *   Se nessuna delle due va bene, la cella del bersaglio. Un'area i cui effetti fanno tutti bene prende un alleato.
	 *   Una predittiva colpisce chi entra, e la cella del bersaglio per lei non va: al suo posto la prima cella accanto al
	 *   bersaglio a portata, nell'ordine delle direzioni (`#4088`).
	 * - **Un bordo** e' il lato della cella del bersaglio rivolto a chi lancia. Se il bersaglio e' oltre la portata,
	 *   la cella e' quella accanto a chi lancia, verso il bersaglio: `Action.Interact` ha portata 1. Per un'azione che
	 *   cambia una porta la scena posa una porta chiusa su quel bordo.
	 * - `Spec.Target` diverso da `Auto` forza il tipo; la cella e il bordo si scelgono con le stesse regole.
	 *
	 * ⚠️ Usa la geometria esagonale condivisa (`URTHexLibrary`) solo per POSARE la scena: dove il colpo arriva e se
	 * la porta si apre lo decide il resolver.
	 */
	static FRTAbilityLabTargetChoice ChooseTarget(const FRTActionDef& Def, ERTAbilityShape Shape,
		const FRTAbilityLabFixtureSpec& Spec);

	/**
	 * Vero se ogni effetto dell'azione fa bene a chi lo riceve: `Heal`, `Shield`, `CancelStatus`, `DamageReduction`
	 * ([D-607] (2)). Un'azione senza effetti, o con un effetto che non e' fra questi, non lo e'.
	 *
	 * ⚠️ **E' una regola del BANCO**: decide di che squadra e' il secondo eroe della scena. Il gameplay non la legge,
	 * e non deve: chi puo' bersagliare chi lo dice la validazione del piano.
	 */
	static bool BenefitsItsRecipient(const FRTActionDef& Def);

	/**
	 * Come la scena provoca la reazione `Def` (`#4073`), letto dal catalogo (vedi `ERTAbilityLabProvocation`). `None`
	 * per un'azione che non reagisce.
	 *
	 * La scena che ne segue (`BuildFixture`): chi reagisce arma la reazione (`Intent.Reaction`) e non la usa come
	 * principale; il `TARGET` e' un eroe il cui attacco base EFFETTIVO — col loadout di default, che la sessione monta —
	 * sa provocarla da dove sta. Per `AllyHit` si posa un alleato accanto a chi reagisce. Il Brace lo impugna l'eroe per
	 * cui la sua finestra si apre (`URTCatalogLibrary::BraceExecutableResponses`), e la scena dell'Overwatch e del Brace
	 * apre le finestre di reazione del giocatore. `Control` e `Hazard` li ascoltano solo moduli d'equipaggiamento, che
	 * il banco non esegue: per loro `BuildFixture` rifiuta. Una predittiva blocca la cella di `ChooseTarget`, e il `TARGET`
	 * ci entra (`#4088`): senza, la scena registrerebbe solo un colpo mancato.
	 */
	static ERTAbilityLabProvocation ProvocationFor(const FRTActionDef& Def);

	/**
	 * Il TurnLog della run, in righe leggibili.
	 *
	 * Percorso: `FRTTestResult::TurnTraces` (byte canonici) → `URTTurnLogLibrary::DeserializeTurnLog` →
	 * `DescribeTurnLog`. Nessuna riga viene composta qui: le stringhe sono quelle che il TurnLog produce
	 * per chiunque altro.
	 */
	// ⛔ **Deliberatamente NON una `UFUNCTION`.** `RTTestResult.h` non dichiara **nessuna** `USTRUCT`:
	// `FRTTestResult` e `FRTTurnTrace` sono struct C++ nude, invisibili alla riflessione. UHT lo dice in
	// otto secondi — *«Unable to find 'class', 'delegate', 'enum', or 'struct' with name
	// 'FRTTestResult'»* — e ha ragione: il risultato di una run e' un oggetto di dominio, non un DTO.
	static TArray<FString> DescribeRunTurnLog(const FRTTestResult& Result);

	/**
	 * Il PASSO del banco dentro il PIE (`#3774`): da una scena del Lab, l'azione a `Step` posizioni dalla sua.
	 *
	 * L'elenco e' il kit dell'eroe che la possiede — lo stesso, nello stesso ordine, che il pannello mostra col
	 * filtro per eroe (`URTHeroLabLibrary::ListHeroKit`) — o le core, per un'azione core; ne restano le voci
	 * che `BuildFixture` costruisce. Il giro agli estremi vale nei due versi.
	 *
	 * `OutPosition` e' la posizione, da 1, dell'azione PROPOSTA; `OutCount` quante sono.
	 *
	 * `false`, senza toccare le uscite, per un Id che non e' una scena del Lab o la cui azione non e' nel ciclo:
	 * e' cio' che tiene inerti i due tasti fuori dal banco.
	 */
	static bool PlanLabStep(const FString& CurrentScenarioId, int32 Step, FName& OutAbilityId,
		int32& OutPosition, int32& OutCount);

	/**
	 * La riga della banda che, in una scena del Lab, dice dove si e' nell'elenco e quali tasti premere
	 * (`#3774`), e in fondo quanti bersagli la scena chiede (`DescribeLabTargets`, `#4136`). Vuota per ogni altro Id.
	 *
	 * ⚠️ **Legge la scena dal file** (`ReadLabScene`), per sapere quanti bersagli chiede e quanti ne posa: senza file, o senza
	 * radice del Lab, la scena vale 1/1. Compone ogni volta che la chiami, e costa: chi la chiama a ogni fotogramma la ricorda
	 * per scena (`ARTGameMode::GetLabStepText`).
	 */
	static FString DescribeLabStep(const FString& ScenarioId);

	/**
	 * Il numero di bersagli che `U` propone dopo `Current` (`#4136`): 1 → 2 → 3 → 1. Un valore fuori scala (0, un negativo, uno
	 * oltre `MaxLabTargets()`) riparte da 1.
	 */
	static int32 NextLabTargetCount(int32 Current);

	/**
	 * Il pezzo della banda che dice i bersagli: `«U» bersagli N`, e ` (qui ne stanno K)` se `Placed < Requested`: la scena ne
	 * chiede N, e la mappa o l'azione ne reggono meno (`#4136`). I tasti fra virgolette basse, come nel resto della riga.
	 */
	static FString DescribeLabTargets(int32 Requested, int32 Placed);

	/**
	 * Dove sta il file di una scena del banco: `<LabScenariosRoot>/<ScenarioId>.json`, reso assoluto (`#4136`). Lo scrive
	 * `WriteLabScenario` e lo rilegge `ReadLabScene`: una sede sola per il percorso.
	 *
	 * 🔴 **Stringa VUOTA se la radice del Lab e' vuota** (sotto automation senza override). Mai una radice vuota combinata con un
	 * nome relativo: sarebbe un percorso dentro la cartella corrente, e chi lo legge o lo scrive sporcherebbe il progetto.
	 */
	static FString LabScenarioPath(const FString& ScenarioId);

	/**
	 * Rilegge dal disco la scena del banco `ScenarioId` (`#4136`). `false` per un Id che non e' del banco, senza radice del Lab,
	 * senza file, o se il file non si interpreta; `OutScene` resta com'era.
	 *
	 * Controlla `FPaths::FileExists` prima del loader: un file assente non passa dal loader, che scriverebbe un errore nel log di
	 * un test che ha solo chiesto «e' gia' stata scritta?».
	 */
	static bool ReadLabScene(const FString& ScenarioId, FRTTestScenario& OutScene);

	/**
	 * Cosa fa `U` in una scena del banco (`#4136`): la stessa azione, col numero di bersagli successivo (`NextLabTargetCount`) a
	 * quello che la scena in corso chiede. Una scena senza file, o scritta prima del tag `lab-targets-N`, chiede 1.
	 *
	 * `false`, senza toccare le uscite, per un Id che non e' una scena del banco o la cui azione non e' canonica: e' cio' che tiene
	 * inerte il tasto fuori dal banco, come `PlanLabStep` per `,` e `.`.
	 */
	static bool PlanLabTargets(const FString& CurrentScenarioId, FName& OutAbilityId, int32& OutNextCount);

	/**
	 * Vero se `ScenarioId` e' una scena del banco: `AbilityLab.<AbilityId>`, la forma che `BuildFixture` scrive (`#4071`).
	 * E' la domanda che il GameMode fa per la velocita' del banco ([D-607] (1)); `false` per ogni altro Id, compreso
	 * un `AbilityLab.` senza niente dopo.
	 */
	static bool IsLabScenarioId(const FString& ScenarioId);

	/**
	 * L'id dell'unita' che lancia in ogni scena del banco: `BuildFixture` la posa con questo id, e la camera del banco
	 * la cerca con lo stesso (`ARTGameMode::FindLabCaster`, `#4121`, D-618). Una sede sola, perche' due letterali
	 * uguali in due file divergono al primo rename.
	 */
	static const TCHAR* CasterUnitId() { return TEXT("CASTER"); }

	/**
	 * Il massimo di bersagli che una scena del banco posa (`#4136`): `TARGET`, `TARGET2` e `TARGET3`. Una sede sola per il tetto,
	 * che lo scrive `BuildFixture`, lo legge `RequestedTargetsOf` e lo mostra la banda del banco.
	 */
	static constexpr int32 MaxLabTargets() { return 3; }

	/**
	 * Quanti bersagli CHIEDE una scena del banco: il numero del tag `lab-targets-N` che `BuildFixture` scrive sempre (`#4136`).
	 *
	 * Un tag fuori scala si porta fra 1 e `MaxLabTargets()`; un tag che non e' un numero (`lab-targets-x`, o senza numero) vale 1,
	 * come una scena scritta prima di questo tag, che non lo porta. Si legge dal tag e non dalla spec perche' la scena e' un
	 * file: chi la rilegge non ha la spec che l'ha prodotta.
	 */
	static int32 RequestedTargetsOf(const FRTTestScenario& Scene);

	/**
	 * Quanti bersagli POSA una scena del banco: le unita' `TARGET`, `TARGET2` e `TARGET3` (`#4136`). Puo' valere meno di
	 * `RequestedTargetsOf`: un'azione che colpisce una cella sola, la fine della mappa o del roster ne posano meno di quelli chiesti.
	 */
	static int32 PlacedTargetsOf(const FRTTestScenario& Scene);

	/**
	 * La velocita' del playback del banco, x0,5 ([D-607] (1), `#4071`): l'autore guarda una skill alla volta e da' il suo
	 * OK alle voci PIE. ⛔ **Solo nel banco**: il tasto `V` della partita resta `x1 · x2 · x4` ([D-350]). La imposta il
	 * pannello del Lab su `rt.Debug.PlaybackSpeed`, e la accetta `ARTGameMode::ApplyPlaybackControlCVars` solo in una
	 * scena del banco. Presentazione: non entra nel TurnLog e non tocca l'hash.
	 */
	static float BenchPlaybackSpeed() { return 0.5f; }

	/**
	 * Scrive `Scenario` nella radice del Lab (`URTScenarioLoader::LabScenariosRoot`) e verifica che il suo Id
	 * risolva a quel file. Fail closed: un file scritto ma non lanciabile si toglie, e `OutError` dice perche'.
	 *
	 * Vive qui e non nel pannello (`#3774`): la usano il pannello dell'Editor e il passo del banco dentro il
	 * PIE, e una seconda copia della scrittura sarebbe il secondo costruttore di scene che il Lab non deve avere.
	 */
	static bool WriteLabScenario(const FRTTestScenario& Scenario, FString& OutError);
};
