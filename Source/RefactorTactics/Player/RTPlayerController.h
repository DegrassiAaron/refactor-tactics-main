#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/RTTypes.h"
#include "Player/RTPointerInteraction.h" // il contesto esplicito di CP 11.8 e i suoi tipi
#include "RTPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class URTKnowledgeVeilPresenter;
struct FInputActionValue;

/**
 * Controller tattico. Costruisce Enhanced Input interamente in C++ (nessun .uasset richiesto):
 * pan della camera con WASD, zoom con la rotellina, selezione col click sinistro.
 */
UCLASS()
class REFACTORTACTICS_API ARTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/**
	 * Squadra comandata da questo giocatore: si selezionano e si pianificano SOLO le unita' con questo TeamId
	 * (regola in URTCombatLibrary::CanPlayerControlUnit). Nel demo il team 1 e' del bot. In multiplayer arrivera'
	 * dal PlayerState, ma la regola di autorita' resta la stessa.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|Player")
	int32 PlayerTeamId = 0;

	/**
	 * Il presenter del velo di QUESTO client, creato alla prima richiesta.
	 *
	 * 🔑 **Il viewer appartiene al giocatore, non alla partita** (`E-SOLID` fetta 4). Fino a quel refactor la
	 * domanda «di chi e' la vista?» la rispondeva `ARTGameMode`, cioe' l'oggetto che in multiplayer e' **uno
	 * solo e sta sul server**: la risposta giusta e' qui, dove `ARTPlayerState::TeamIdOf` risale — la stessa
	 * fonte che `ARTCameraPawn::FrameOwnTeam` e `CanPlayerControlUnit` leggono.
	 *
	 * ⚠️ **Uno per controller, e l'`Outer` E' il viewer**: il presenter risale al proprio `ARTPlayerState` da
	 * `GetOuter()`, quindi non esiste una seconda sede del valore da tenere allineata.
	 *
	 * ⚠️ **Non lo aggancia il controller**, e non e' una svista: il `TurnManager` puo' non esistere ancora
	 * quando questo `BeginPlay` corre, e l'ordine di spawn fra Actor non e' garantito. L'aggancio lo fa
	 * `ARTGameMode::HookKnowledgeVeil`, che e' l'unico punto in cui il `TurnManager` esiste per certo perche'
	 * e' il GameMode a spawnarlo.
	 */
	URTKnowledgeVeilPresenter* GetKnowledgeVeilPresenter();

	/**
	 * Il **piano attivo del giocatore**: il layer su cui hover e click risolvono la cella (`D-255`).
	 *
	 * 🔑 **Vive QUI e non sulla camera, e la ragione e' un invariante e non un gusto.** Da `D-255`
	 * l'`ActiveLayer` decide *quale cella un click seleziona*, cioe' entra nel gameplay: metterlo su
	 * `ARTCameraPawn` avrebbe reso la camera un'autorita' sull'esito, che e' esattamente cio' che
	 * [D-143] vieta. Il piano attivo appartiene a chi pianifica, e chi pianifica e' questo controller —
	 * la stessa sede di `PlayerTeamId`.
	 *
	 * ⚠️ **Non e' `ARTHexMapActor::ActiveLayer`**, che ha lo stesso nome e un altro mestiere: quello e'
	 * stato di **authoring**, guidato dall'editor mode (`RTHexEditorClick.cpp`) perche' li' si dipinge su
	 * un piano scelto. Due scrittori su un campo solo sarebbero un difetto che si manifesta in editor e
	 * si diagnostica in partita.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Player")
	int32 ActiveLayer = 0;

	int32 GetActiveLayer() const { return ActiveLayer; }

	/**
	 * Cambia il piano attivo di `Delta` livelli, **restando fra i layer che la mappa contiene davvero**.
	 *
	 * Ritorna `true` se il piano e' cambiato. I limiti si misurano sulle celle e non si assumono: una
	 * mappa dipinta a mano puo' avere buchi, e permettere di salire a un piano vuoto darebbe un hover che
	 * non trova mai niente senza dire perche'.
	 */
	bool StepActiveLayer(int32 Delta);

	/** Imposta il piano attivo (clampato ai layer della mappa). Ritorna `true` se e' cambiato. */
	bool SetActiveLayer(int32 NewLayer);

	/**
	 * Porta `ActiveLayer` dentro l'intervallo dei layer che la mappa contiene davvero.
	 *
	 * Chiamata in `BeginPlay`: il campo nasce a `0`, e su una mappa che comincia a quota 1 quello sarebbe
	 * un piano vuoto su cui hover e click non troverebbero mai nulla.
	 */
	void AlignActiveLayerToMap();

	/**
	 * La cella indicata dal cursore, risolta **sul piano attivo**.
	 *
	 * `bOutHitWasOnAnotherLayer` dice che il raggio ha colpito geometria di un piano diverso: non e' un
	 * errore, ed e' l'unica informazione con cui il chiamante puo' spiegare al giocatore perche' il click
	 * su una mesh visibile non ha selezionato quella cella.
	 */
	bool ResolveCellUnderCursor(FRTCellId& OutCell, bool& bOutHitWasOnAnotherLayer) const;

	/**
	 * Costruisce (se serve) e restituisce il mapping context, **per i test**.
	 *
	 * 🔴 Esiste perche' `PlayerInput.HotkeysDoNotCollide` non esisteva: un commento di
	 * `GenericHotkeys()` dichiarava *«e' un controllo che `PlayerInput.HotkeysDoNotCollide` rifa'»*, e
	 * `grep -rn "DoNotCollide" Source/` rispondeva **una sola riga** — quel commento. L'unica verifica
	 * dell'assenza di collisioni era la lista scritta a mano in un altro commento, cioe' una promessa che
	 * invecchia al primo tasto aggiunto. E i tasti aggiunti da `#1771` sono quattro.
	 */
	const UInputMappingContext* BuildAndGetMappingContextForTest()
	{
		BuildInputMappings();
		return MappingContext;
	}

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	/** Crea (una sola volta) Input Action e Mapping Context via codice. */
	void BuildInputMappings();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PanAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RotateAction;

	/**
	 * #863 — orbita continua: il modificatore arma il gesto, l'asse 2D lo guida (X = yaw, Y = pitch).
	 * `Transient` come tutti i fratelli: sono `UInputAction` creati con `NewObject` in
	 * `BuildInputMappings`, e senza entrerebbero nella serializzazione dell'actor.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> OrbitAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> OrbitModifierAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SelectAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LockInAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RestartAction;

	/**
	 * Un `UInputAction` per POSIZIONE del kit, nell'ordine di `AbilityHotkeys()`: l'indice qui e' l'indice
	 * nell'elenco dell'unita' selezionata.
	 *
	 * 🔴 **Erano quattro campi distinti — `Ability1Action`..`Ability4Action` — e la forma era il difetto.**
	 * Con quattro campi *«quante posizioni del kit raggiunge l'input»* non era una domanda interrogabile, e
	 * un test poteva solo confrontare il letterale `4`, cioe' invecchiare al primo tasto aggiunto. La
	 * collezione e' cio' che [#1409] e [#1034] dichiarano parte del proprio lavoro proprio per questo.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> AbilityActions;

	/**
	 * Un `UInputAction` per AZIONE GENERICA, nell'ordine di `GenericHotkeys()`. Sono l'altro canale di
	 * selezione, e la differenza con `AbilityActions` non e' cosmetica: quelle scelgono una **posizione**,
	 * queste una **azione per nome**.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> GenericActions;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> UndoAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RecenterAction;

	/** Centra la camera sull'unita' selezionata (lo zoom orbita attorno al pawn: senza spostarlo ci si
	    avvicina al centro della mappa invece che al personaggio). */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FocusAction;

	/**
	 * `#1771` — **il modificatore camera** (`Alt`). Arma i gesti di vista senza toccare quelli di gioco.
	 *
	 * ⚠️ Non collide con `Alt`+`Enter`: quello e' `bAltEnterTogglesFullscreen` in `DefaultInput.ini`, cioe'
	 * una scorciatoia del motore su una combinazione che qui non e' mappata.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CameraModifierAction;

	/** `#1775` — piano attivo sopra / sotto (`PageUp` / `PageDown`). */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LayerUpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LayerDownAction;

	/**
	 * 🔴 **Il tasto che mancava a `#291`.** Le regole della rotazione dichiarata erano complete e testate
	 * dal 2026-08-09 — `TryApplyDeclaredFacing`, `LegalFacings`, il consumo nel TurnManager, il rifiuto
	 * invece della correzione silenziosa — ma **nessuno le raggiungeva**: `BeginFacingDeclaration` e
	 * `HandleFacingSector` avevano come unici chiamanti dei test, e non erano `UFUNCTION`. Il giocatore non
	 * aveva modo di chiedere una rotazione, e a fine percorso l'unita' si girava dove diceva l'ultimo passo.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FacingAction;

	/** Cicla la velocita' di riproduzione `x1 · x2 · x4` (CP 47.7, #1015). */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PlaybackSpeedAction;

	/**
	 * `ESC`: apre e chiude il menu di pausa (CP 46.6, `#941`).
	 *
	 * ⚠️ **Nessun `.uasset`, come tutti i fratelli**: gli `UInputAction` di questo controller nascono da
	 * `NewObject` in `BuildInputMappings`, quindi aggiungere un tasto e' C++ e non lavoro d'editor. Vale la
	 * pena scriverlo perche' la conclusione opposta — «serve un Input Action, quindi serve una seduta» —
	 * avrebbe rimandato l'intero checkpoint a un binario che non serve.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PauseAction;

	/** Attore attualmente selezionato (se implementa IRTSelectable). */
	UPROPERTY()
	TObjectPtr<AActor> SelectedActor;

	void OnPan(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnRotate(const FInputActionValue& Value);

	/** #863 — orbita continua. `bOrbiting` distingue «il mouse si muove» da «il giocatore sta orbitando». */
	void OnOrbit(const FInputActionValue& Value);
	void OnOrbitPressed(const FInputActionValue& Value);
	void OnOrbitReleased(const FInputActionValue& Value);

	bool bOrbiting = false;

	// --- #1771 / #1772 · il modificatore camera e i suoi gesti -------------------------------------

	void OnCameraModifierPressed(const FInputActionValue& Value);
	void OnCameraModifierReleased(const FInputActionValue& Value);
	void OnSelectReleased(const FInputActionValue& Value);
	void OnUndoReleased(const FInputActionValue& Value);
	void OnLayerUp(const FInputActionValue& Value);
	void OnLayerDown(const FInputActionValue& Value);

	/** `Alt` tenuto: arma i gesti camera. */
	bool bCameraModifier = false;

	/** `Alt`+`LMB` in corso. Diventa Set Pivot al rilascio se non ha superato la soglia, orbita se l'ha superata. */
	bool bAltPrimaryDown = false;

	/** `Alt`+`RMB` in corso: dolly. Sopprime l'`UndoAction` per tutta la durata del gesto. */
	bool bAltSecondaryDown = false;

	/**
	 * Quanta strada ha percorso il puntatore da quando `Alt`+`LMB` e' stato premuto.
	 *
	 * 🔑 **E' cio' che separa un click da un drag**, e la separazione dev'essere netta: superata la soglia
	 * il gesto e' un'orbita e il click **non si emette piu'**, altrimenti un trascinamento lungo
	 * produrrebbe anche un set-pivot al rilascio — due operazioni da un gesto solo.
	 */
	float AltPrimaryDragDistance = 0.f;

	/**
	 * Soglia in pixel oltre la quale `Alt`+`LMB` e' un trascinamento e non un click.
	 *
	 * ⏳ **Taratura aperta**: la distanza che una mano percorre senza volerlo non e' una costante
	 * universale, e il numero si tara in `L_CameraFeatureLab` ([#1780]).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.0"))
	float ClickDragThreshold = 6.f;

	/** Sensibilita' del precision pan (`Alt`+`MMB`) rispetto al pan normale. Taratura aperta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float PrecisionPanScale = 0.25f;

	/**
	 * Conversione **pixel → valore d'asse** per i gesti che riusano `AddPlanarMovement`, che si aspetta un
	 * asse (`WASD` consegna 1) e non un delta di puntatore. Taratura aperta.
	 *
	 * 🔴 Esiste perche' il precision pan era piu' veloce del pan normale: mancava l'unita' di misura, e
	 * nessuna scala poteva ripararlo.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "1.0"))
	float PixelsPerPanUnit = 10.f;

	/**
	 * Conversione **pixel → passi di zoom** per il dolly, che riusa `AddZoom` — tarato sulla rotella, che
	 * consegna ±1. Taratura aperta.
	 *
	 * 🔴 Senza, un movimento ordinario di 10 px/frame consumava quasi mezzo intervallo di zoom in un frame.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.0"))
	float DollySensitivity = 0.02f;

	/** Unita' mondo di peek per pixel di movimento del cursore. Taratura aperta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.0"))
	float PeekSensitivity = 3.f;

	/** Frazione di peek residuo riassorbita per secondo quando `Alt` e' rilasciato. Taratura aperta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.0"))
	float PeekReturnSpeed = 8.f;

	/** Riassorbe il peek residuo. Chiamata da `PlayerTick`: e' l'unico movimento automatico che esista oggi. */
	void UpdatePeekReturn(float DeltaTime);

	// --- #1773 · doppio click = Select + Focus ------------------------------------------------------

	/**
	 * Istante dell'ultimo click di selezione andato a buon fine, e su cosa.
	 *
	 * Servono **insieme**: due click rapidi su due unita' diverse sono due selezioni, non un doppio click.
	 * Con il solo tempo, cliccare in fretta A e poi B avrebbe inquadrato B senza che nessuno lo chiedesse.
	 */
	double LastSelectTime = -1.0;

	UPROPERTY(Transient)
	TObjectPtr<AActor> LastSelectActor;

	/**
	 * Finestra del doppio click, in secondi. Taratura aperta.
	 *
	 * ⚠️ Non si legge da `GetDoubleClickTime()` del sistema: quello vale per i widget Slate e qui il
	 * gesto e' sul mondo, non sulla UI. Un campo tarabile e' anche cio' che permette di provarlo headless.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.05"))
	float DoubleClickInterval = 0.3f;

	/**
	 * Instrada un movimento del puntatore al gesto camera armato, se ce n'e' uno.
	 *
	 * Ritorna `true` se il movimento e' stato consumato: cosi' l'orbita di `#863` resta il ramo di
	 * default e non deve conoscere i gesti nuovi.
	 */
	bool RouteCameraGesture(const FVector2D& Delta);

public:
	/** Stato dei gesti camera, per i test: nessuno di questi ha un viewport da interrogare. */
	void SetCameraModifierForTest(bool bDown) { bCameraModifier = bDown; }
	bool IsCameraModifierForTest() const { return bCameraModifier; }
	void SetAltPrimaryDownForTest(bool bDown, float Distance = 0.f)
	{
		bAltPrimaryDown = bDown;
		AltPrimaryDragDistance = Distance;
	}
	float GetAltPrimaryDragDistanceForTest() const { return AltPrimaryDragDistance; }
	void SetClickDragThresholdForTest(float InThreshold) { ClickDragThreshold = InThreshold; }
	bool RouteCameraGestureForTest(const FVector2D& Delta) { return RouteCameraGesture(Delta); }
	void UpdatePeekReturnForTest(float DeltaTime) { UpdatePeekReturn(DeltaTime); }

protected:

public:
	/**
	 * L'orbita a partire da un delta di mouse, senza `FInputActionValue` (per i test).
	 *
	 * Stessa disciplina di `FocusCameraOnUnit` e `HandleClickOnUnitForTest`: la **decisione** — quale asse
	 * guida cosa — si verifica headless; l'handler resta il solo punto che decide *quando*. Senza questo,
	 * scambiare `Delta.X` con `Delta.Y` non farebbe cadere niente.
	 */
	void OrbitCameraForTest(const FVector2D& Delta);

	/** Arma o disarma il gesto, come farebbe il tasto centrale (per i test). */
	void SetOrbitingForTest(bool bInOrbiting) { bOrbiting = bInOrbiting; }

	/** Se il gesto e' armato. Serve a verificare che il rilascio lo chiuda davvero. */
	bool IsOrbitingForTest() const { return bOrbiting; }

	/**
	 * Porta la velocita' di riproduzione al passo successivo della scala (CP 47.7, #1015).
	 *
	 * ⚠️ **E' l'UNICO punto in cui la UI scrive nel modello, e la firma lo rende verificabile.** Statica e
	 * con il `ARTTurnManager` come solo parametro: non puo' leggere la selezione, non puo' toccare un
	 * altro attore, e un test la esercita allestendo il solo turn manager — senza montare un controller,
	 * che e' cio' che renderebbe la voce 1 del DoD non misurabile headless.
	 *
	 * Scrive `ViewerPlaybackSpeed` e nient'altro: in particolare **non** tocca `MaxPlaybackSeconds`, che
	 * e' l'altro produttore della velocita' effettiva. Pinnato da
	 * `RefactorTactics.HUD.PlaybackSpeedControlWritesOnlyTheViewerField`.
	 *
	 * Nullo = nessuna partita: non fa nulla. L'HUD e il controller esistono prima del turn manager.
	 */
	static void ApplyNextPlaybackSpeed(class ARTTurnManager* TurnManager);

	/**
	 * I tasti che armano una POSIZIONE del kit, in ordine: l'indice nell'elenco **e'** l'indice nel kit
	 * dell'unita' selezionata. Dieci, `1`..`9` piu' `0`.
	 *
	 * ⚠️ **E' la sorgente unica del conteggio**, ed e' il motivo per cui esiste come funzione invece che
	 * come sequenza di `MapKey` scritti a mano: la mappatura la percorre, la bindatura la percorre, e un
	 * test puo' chiedere *«quante posizioni raggiunge l'input?»* senza confrontare un letterale che
	 * invecchia al primo tasto aggiunto. Interrogata da `PlayerInput.EveryKitEntryIsReachable`.
	 *
	 * Il tasto sceglie una **posizione**, non un'azione: quale abilita' occupi l'indice N dipende dall'eroe.
	 */
	static const TArray<FKey>& AbilityHotkeys();

	/**
	 * Le azioni GENERICHE e il tasto che le arma, in coppia. Sono l'altro canale di selezione del kit, e
	 * risolvono per **nome** invece che per posizione.
	 *
	 * 🔴 **Il nome non e' un vezzo, e' l'unica forma corretta.** Le generiche sono ACCODATE al kit
	 * (`ARTUnit::EnsureDefaultAbilities`), quindi il loro indice dipende da quante azioni porta l'eroe: con
	 * cinque stanno a `5..9`, con sei a `6..10`. Un tasto legato a una posizione fissa punterebbe a cose
	 * diverse su eroi diversi — e su quello che ne ha una in piu' punterebbe a un'abilita' d'eroe.
	 *
	 * ⚠️ **Perche' esistono**: dieci posizioni non bastano piu'. Un eroe con sei azioni porta il kit a
	 * **undici** voci contro i dieci tasti numerici, e `PlayerInput.EveryKitEntryIsReachable` lo dichiara.
	 * Togliendo le cinque generiche dalla fila dei numeri, quella fila torna a servire i soli eroi.
	 */
	static const TArray<TPair<FName, FKey>>& GenericHotkeys();

private:
	void OnSelect(const FInputActionValue& Value);
	void OnLockIn(const FInputActionValue& Value);
	void OnRestart(const FInputActionValue& Value);
	// Uno per posizione del kit, e sono one-liner che passano tutti da `SelectAbilityForCurrent`. Uno per
	// posizione e non un handler solo perche' l'indice deve arrivare dalla BINDATURA: `FInputActionValue`
	// porta il valore, non l'azione che l'ha prodotto, quindi un handler unico non saprebbe quale tasto e'
	// stato premuto. La tabella che li lega ai tasti sta in `SetupInputComponent`, in un punto solo.
	void OnAbility1(const FInputActionValue& Value);
	void OnAbility2(const FInputActionValue& Value);
	void OnAbility3(const FInputActionValue& Value);
	void OnAbility4(const FInputActionValue& Value);
	void OnAbility5(const FInputActionValue& Value);
	void OnAbility6(const FInputActionValue& Value);
	void OnAbility7(const FInputActionValue& Value);
	void OnAbility8(const FInputActionValue& Value);
	void OnAbility9(const FInputActionValue& Value);
	void OnAbility10(const FInputActionValue& Value);
	// Uno per azione generica, e per la stessa ragione dei dieci qui sopra: la bindatura e' l'unico posto da
	// cui puo' arrivare QUALE azione e' stata premuta. La tabella che li lega ai tasti sta in
	// `GenericHotkeys()`, e questi handler ne leggono l'`ActionId` invece di ripeterlo.
	void OnGeneric1(const FInputActionValue& Value);
	void OnGeneric2(const FInputActionValue& Value);
	void OnGeneric3(const FInputActionValue& Value);
	void OnGeneric4(const FInputActionValue& Value);
	void OnGeneric5(const FInputActionValue& Value);
	void OnUndoWaypoint(const FInputActionValue& Value);
	void OnCyclePlaybackSpeed(const FInputActionValue& Value);
	void OnRecenter(const FInputActionValue& Value);
	void OnFocusSelected(const FInputActionValue& Value);

	/** Ricostruisce PlannedPath dell'unita' dai PathWaypoints correnti (o lo azzera se vuoti). */
	void RebuildPlannedPath();

	void SelectAbilityForCurrent(int32 Index);

	/**
	 * Arma l'azione con questo `ActionId` nel kit dell'unita' selezionata, cercandone l'indice.
	 *
	 * Delega a `SelectAbilityForCurrent` invece di duplicarne il corpo: cooldown, slot reazione e
	 * self-target restano un percorso solo, e un tasto generico non puo' aggirare un controllo che il
	 * numero rispetta.
	 */
	void SelectAbilityByIdForCurrent(const FName& ActionId);

	/** Handler comune dei tasti generici: legge l'`ActionId` dalla riga `GenericHotkeys()[Slot]`. */
	void SelectGenericSlot(int32 Slot);

public:
	/** Unita' attualmente selezionata dal giocatore (nullo se nessuna). */
	class ARTUnit* GetSelectedUnit() const;

	/**
	 * Decide cosa fare per un click su una CELLA: waypoint di movimento, scatto, oppure rifiuto col motivo.
	 *
	 * Separata dal raycast (`OnSelect` fa solo punto-mondo -> cella) perche' la decisione e' verificabile
	 * headless mentre il raycast no: richiede un viewport. Stessa disciplina di `PlanBotsForTest`.
	 * NON e' una scorciatoia per l'autorita': valida sempre sullo snapshot del TurnManager.
	 */
	void HandleClickOnCell(const FRTCellId& Cell);

	/**
	 * Decide cosa fare per un click su un'UNITA' avversaria: attacco, carica, o rifiuto col motivo. Estratta da
	 * `OnSelect` per la stessa ragione di `HandleClickOnCell` — la decisione e' verificabile headless, il
	 * raycast che la precede no.
	 */
	void HandleClickOnUnit(class ARTUnit* ClickedUnit);

	/**
	 * SELEZIONA un actor, con tutto cio' che la selezione comporta: evidenziazione, anteprima del piano,
	 * telemetria di ritmo.
	 *
	 * Da non confondere con `HandleClickOnUnit`, che **non seleziona**: quella presuppone una selezione e
	 * tratta l'argomento come BERSAGLIO. Scambiarle non produce un errore, produce silenzio — la chiamata
	 * esce subito e a schermo non cambia niente (successo il 2026-08-08).
	 *
	 * @param bRecordAsPlayerInput false quando a selezionare non e' una persona (uno scenario che allestisce
	 *        un'anteprima): la telemetria di ritmo misura quanto impiega un GIOCATORE a decidere, e una
	 *        selezione automatica falserebbe i numeri di `PIE-V01-MATCHLEN`.
	 */
	void SelectUnit(AActor* Actor, bool bRecordAsPlayerInput = true);

	/** Come sopra, per i test: il nome dichiara che il raycast e' stato saltato. */
	void HandleClickOnUnitForTest(class ARTUnit* ClickedUnit) { HandleClickOnUnit(ClickedUnit); }

	/**
	 * Arma l'azione in posizione `Index` come farebbe il tasto corrispondente (per i test).
	 *
	 * `SelectAbilityForCurrent` e' il punto comune dei dieci tasti abilita' ed e' privata: senza questo, il
	 * secondo dei cinque siti che registrano un `ERTPlanningInput::Order` non sarebbe raggiungibile da un
	 * test, e la sua guardia sarebbe l'unica delle cinque affermata invece che misurata (#971).
	 */
	void SelectAbilityForCurrentForTest(int32 Index) { SelectAbilityForCurrent(Index); }

	/**
	 * Il tasto di lock-in (Spazio) senza passare da un `FInputActionValue`, per i test.
	 *
	 * Stessa disciplina di `HandleClickOnUnitForTest`: `OnLockIn` e' privata e legata alla bindatura, e
	 * cio' che va verificato e' la **decisione** — chiude il turno, o non lo chiude — non il trasporto
	 * dell'input. Esiste per #971: durante una sessione non presidiata questo tasto non deve saltare il
	 * playback ne' chiudere la pianificazione, ed e' l'unico percorso di input che **non** produce un piano
	 * e quindi non sarebbe entrato da nessun criterio scritto sui siti `Order`.
	 *
	 * ⚠️ Definita nel `.cpp` e non qui: `FInputActionValue` in questo header e' solo dichiarato in avanti,
	 * e costruirne uno inline non compilerebbe.
	 */
	void OnLockInForTest();

	/**
	 * Inquadra un'unita' con la camera: quello che fa il tasto `F` una volta stabilito CHI inquadrare.
	 *
	 * Estratto da `OnFocusSelected` perche' la scelta della quota — la **cella**, non la posizione
	 * dell'attore — e' la sostanza di `#887` e va verificata senza passare da un `FInputActionValue`.
	 * `OnFocusSelected` resta il solo punto che decide *quale* unita': questo decide *dove*.
	 */
	void FocusCameraOnUnit(const class ARTUnit* Unit);

	/**
	 * Seleziona un actor come farebbe un click su di esso (per i test dell'interazione). Prende `AActor*` e non
	 * `ARTUnit*` perche' qui `ARTUnit` e' solo dichiarato: la conversione al puntatore base non sarebbe visibile.
	 */
	void SelectActorForTest(AActor* Actor) { SelectedActor = Actor; }

	/** Ricostruisce il percorso dai waypoint correnti, come fa l'annullamento (per i test dell'interazione). */
	void RebuildPlannedPathForTest() { RebuildPlannedPath(); }

	// ---- Contratto del puntatore (CP 11.8) ------------------------------------------------------------
	//
	// Owner della regola: `docs/technical/systems/spec-pointer-interaction.md`. Qui c'e' lo STATO ESPLICITO che il
	// DoD chiedeva al posto della cascata di `if` sul tipo di Actor colpito.

	/**
	 * Il contesto corrente, **derivato** e non memorizzato.
	 *
	 * Derivato di proposito: un secondo stato accanto a `SelectedActor`, `SelectedAbilityIndex` e
	 * `PlannedWaypoints` sarebbe una copia che diverge, ed e' il difetto che questo repository paga di
	 * continuo con i totali scritti a mano. Qui il contesto e' una *lettura* di quei tre, piu' la fase.
	 *
	 * ⚠️ **Due contesti non sono ancora producibili, e non e' una svista.** `ReactionWindow` e `Modal`
	 * esistono nell'enum e in `URTPointerLibrary::ResolveBack` — che li ordina correttamente — ma nessuno li
	 * produce: la finestra di reazione e' **E14** e il modale e' **#613**. Aggiungere qui un flag che nessuno
	 * scrive avrebbe creato un campo senza produttore, che e' il difetto che CP 11.8 ha appena finito di
	 * documentare. Quando quegli owner arrivano, aggiungono il proprio ramo qui.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pointer")
	ERTPointerContext GetPointerContext() const;

	/** Che forma di bersaglio chiede l'azione armata. `None` se non c'e' targeting in corso. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pointer")
	ERTPointerTargetKind GetPointerTargetKind() const;

	/**
	 * `ESC`: apre la pausa se e' chiusa, la chiude se e' aperta.
	 *
	 * ⚠️ **Un toggle e non due tasti**, perche' e' cosi' che `ESC` si comporta ovunque — e perche' il
	 * navigatore rifiuta un secondo `ShowPause` con `ScreenIsAlreadyOnStack`: senza il toggle, la seconda
	 * pressione produrrebbe un rifiuto invece della cosa che il giocatore si aspetta.
	 *
	 * ⛔ **Non tocca la simulazione.** Chiama il navigatore e basta: nessun `SetPause`, nessuna dilatazione
	 * del tempo. E' il vincolo offline-only di CP 46.6, e sta qui perche' questo e' l'unico punto in cui una
	 * pressione di tasto potrebbe diventare una sospensione.
	 */
	UFUNCTION()
	void OnTogglePause();

	/**
	 * **Una schermata bloccante copre la partita**: nessun input di gioco deve arrivare al mondo.
	 *
	 * 🔴 **Esiste perche' il contesto `Modal` da solo NON toglieva niente, e per un'intera revisione la
	 * pausa e' stata una promessa.** `GetPointerContext()` era letto da tre soli consumatori —
	 * `HandleTargetCell`, `HandleTargetUnit`, `HandleDeclareFacing`, cioe' i **click sul mondo** — mentre
	 * `OnLockIn` (Spazio), `OnSelect`, `OnRestart`, le `OnAbility*` e `OnUndoWaypoint` non lo guardavano
	 * affatto. Con la pausa aperta, **Spazio risolveva il turno dietro la schermata**. Il DoD dice «una
	 * schermata copre la partita e le toglie il puntatore»: era vero del puntatore e falso della tastiera.
	 * Trovato in code review sulla PR #1304.
	 *
	 * ⚠️ **Non blocca la CAMERA**, ed e' deliberato: pan, zoom, orbita e recenter non toccano il piano ne'
	 * la simulazione. Bloccare anche quelli sarebbe un contratto piu' largo di quello che serve, e la
	 * precedenza dichiarata da CP 11.8 parla di *chi consuma un click*, non di chi muove la vista.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pointer")
	bool IsGameplayInputBlocked() const;

	/**
	 * **La sessione non e' presidiata**: la pianificazione umana non deve agganciare niente (#971).
	 *
	 * Con l'autobattle in vigore entrambe le squadre sono del bot, ma il percorso di input non lo sapeva:
	 * `URTCombatLibrary::CanPlayerControlUnit` decide su `UnitTeamId == PlayerTeamId` e per lui la squadra 0
	 * resta del giocatore. Un click selezionava ancora, un ordine si scriveva ancora, e `PlanBots` lo
	 * sovrascriveva il turno dopo senza dirlo. La modalita' esiste per essere **registrata in video**: il
	 * difetto e' di coerenza, e il filmato e' l'artefatto.
	 *
	 * 🔴 **Perche' non e' una seconda causa dentro `IsGameplayInputBlocked`**, che sarebbe stata una riga
	 * sola e copriva sei punti d'ingresso per costruzione: quel funnel include `OnRestart`, che agisce
	 * **solo a `MatchEnded`**. Bloccarlo toglierebbe allo spettatore l'unico modo di rilanciare la demo
	 * finita — un input che non produce nessun piano e non tocca nessuna simulazione in corso. Le due
	 * cause hanno insiemi diversi, quindi restano due predicati.
	 *
	 * ⚠️ **Non blocca la CAMERA**, per la stessa ragione dichiarata dal fratello qui sopra e con un motivo
	 * in piu': qui l'attore **e' lo spettatore**. Pan, zoom, orbita, recenter e focus non toccano il piano
	 * ne' la simulazione, e una telecamera che smette di rispondere e' il terzo modo di rovinare la
	 * registrazione — dopo il piano che evapora e il turno che si chiude da solo.
	 *
	 * ⚠️ **Blocca invece `OnLockIn` (Spazio)**, che non produce un piano e quindi non sarebbe entrato da
	 * nessun criterio scritto sui cinque siti `Order`: durante una partita non presidiata quel tasto
	 * **salta il playback** o **chiude il turno in anticipo**. Non rende il filmato confuso, lo taglia.
	 *
	 * ⛔ **Legge il GameMode, non `bIsBotControlled`.** L'autorita' e' la sessione — un valore latchato in
	 * `SetupHexMatch` prima che le unita' entrino in campo — non l'attributo della singola unita', che ha
	 * piu' siti di scrittura e che un'unita' gia' posata nel livello non attraversa affatto (vedi l'elenco
	 * alla dichiarazione di `ARTUnit::bIsBotControlled`). E' anche cio' che tiene la guardia fuori dai test
	 * di pacing, che pilotano `ARTTurnManager` direttamente su fixture senza GameMode.
	 *
	 * ⚠️ **Limite**: in multiplayer `GetAuthGameMode` non esiste sul client, e questo predicato risponde
	 * `false` — cioe' l'input resta vivo. E' il verso sicuro (nessun input sparisce per un'autorita' che
	 * non si e' potuta interrogare), ma la modalita' non presidiata e' offline per costruzione in v0.1.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pointer")
	bool IsPlanningInputInert() const;

	/**
	 * §5.5 — applica il Back e dichiara **quale livello** ha smontato.
	 *
	 * Restituisce il livello invece di `void` perche' l'ordine e' la cosa da provare: un test che guarda solo
	 * l'effetto non distingue «ha rimosso il waypoint perche' era il livello giusto» da «ha rimosso il
	 * waypoint perche' e' l'unica cosa che sa fare».
	 */
	ERTPointerBackStep ApplyBack();

	/**
	 * `Targeting`/`Cell` -> `PlannedAttackCell` + `bAttackTargetsCell`. Primo produttore di questo campo che
	 * non sia lo Scenario Harness (#737).
	 * @return true se il bersaglio e' stato registrato.
	 */
	bool HandleTargetCell(const FRTCellId& Cell);

	/**
	 * `Targeting`/`Edge` -> `PlannedCoverEdge` + `bHasPlannedCoverEdge`, insieme alla cella su cui agire.
	 * @return true se il lato e' stato registrato.
	 */
	bool HandleTargetEdge(const FRTCellId& Cell, ERTHexDirection Edge);

	/**
	 * `Facing` -> `PlannedFacing` + `bDeclaresPlannedFacing`. Chiude l'anello che #291 aveva lasciato aperto:
	 * le regole della rotazione dichiarata esistevano e nessun input le raggiungeva.
	 *
	 * ⚠️ Rifiuta un settore **illegale** invece di correggerlo in silenzio: e' la regola di
	 * `URTFacingLibrary`, e qui la si chiede, non la si riscrive.
	 * @return true se la rotazione e' stata dichiarata.
	 */
	bool HandleFacingSector(ERTHexDirection Sector);

	/** Entra nel contesto `Facing` (l'unita' selezionata deve esserci). */
	void BeginFacingDeclaration();

	/**
	 * Cicla il facing dichiarato fra le direzioni **legali** per il movimento pianificato, e lo applica.
	 *
	 * ⚠️ **Cicla fra le legali invece di offrirle tutte e sei**: l'insieme dipende dallo stile — tre dopo
	 * un Move a budget, una sola dopo uno scatto lineare, sei da fermo — e senza l'indicatore a schermo
	 * (`#613`) un giocatore che potesse chiedere una direzione qualunque riceverebbe un rifiuto muto. Qui
	 * una direzione illegale non e' proprio raggiungibile, che e' la stessa garanzia ottenuta senza HUD.
	 */
	void CycleDeclaredFacing();

	/**
	 * Ruota la MESH verso il facing che l'unita' avra' a fine mossa: la rotazione dichiarata se c'e',
	 * altrimenti quella derivata dal percorso pianificato, altrimenti l'orientamento attuale.
	 *
	 * ⚠️ **Solo presentazione, e in ANTICIPO.** Il facing logico non cambia in pianificazione — lo scrive
	 * il resolver a fine Move — quindi fra qui e la risoluzione la mesh mostra un orientamento che le regole
	 * non hanno ancora. E' la stessa natura del percorso verde e del ventaglio: un'anteprima del piano, non
	 * lo stato. `PIE-FACING-1` chiede che mesh e regola coincidano **a fine playback**, ed e' li' che il
	 * TurnManager le riallinea.
	 *
	 * Senza, il giocatore pianifica un percorso, non vede cambiare niente, e a fine risoluzione l'unita'
	 * scatta a un orientamento che non ha mai visto arrivare — che a schermo si legge come casuale.
	 */
	void PreviewPlannedFacing(ARTUnit* Unit) const;

	/** Esce da `Facing` senza dichiarare nulla. */
	void EndFacingDeclaration();

	/** Vero mentre si sta dichiarando una rotazione. */
	bool IsDeclaringFacing() const { return bDeclaringFacing; }

	/**
	 * Inspector pinnato: livello 3 del Back. Il produttore e' lo Screen HUD (#613); qui c'e' il flag perche'
	 * l'ORDINE del Back e' di questo contratto e senza il livello l'elenco sarebbe incompleto.
	 */
	void SetInspectorPinned(bool bPinned) { bInspectorPinned = bPinned; }
	bool IsInspectorPinned() const { return bInspectorPinned; }

	/**
	 * `PhaseFocus` pinnato: livello 7, l'ultimo prima di `NoOp`. Owner dello scrubbing: CP 11.6 (#173).
	 * Qui c'e' solo il fatto che `RMB` lo smonta **per ultimo**.
	 */
	void SetPhaseFocusPinned(bool bPinned) { bPhaseFocusPinned = bPinned; }
	bool IsPhaseFocusPinned() const { return bPhaseFocusPinned; }

protected:
	/** Vero fra `BeginFacingDeclaration` e la conferma/annullamento. */
	bool bDeclaringFacing = false;

	bool bInspectorPinned = false;

	bool bPhaseFocusPinned = false;

private:
	/**
	 * Vedi `GetKnowledgeVeilPresenter()`. `Transient` come i fratelli creati con `NewObject`: non entra
	 * nella serializzazione del controller, ma la `UPROPERTY` serve — senza, il GC se lo porterebbe via
	 * fra un refresh e l'altro e la board resterebbe all'ultimo velo steso.
	 */
	UPROPERTY(Transient)
	TObjectPtr<URTKnowledgeVeilPresenter> KnowledgeVeilPresenter;
};
