#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

class ARTCameraPawn;

/** `#1838` — gli stati dell'edge pan, per il log delle transizioni. Un enum e non un testo: la macchina non dipende da come e' scritto il messaggio. */
enum class ERTEdgePanState : uint8
{
	Rest,
	Active,
	RefusedSwitch,
	RefusedBlocked,
	RefusedGesture,
	RefusedKeys,
};

/**
 * I numeri tarabili dei gesti camera.
 *
 * ⚠️ **La sede della taratura resta il controller**: sono `UPROPERTY` `EditAnywhere` di `ARTPlayerController`, e
 * spostarli cambierebbe il percorso della proprieta' per chi li ha gia' sovrascritti. Il controller li copia qui a ogni
 * gesto (`ARTPlayerController::GetCameraGestureTuning`); i default sono gli stessi, per i test che costruiscono i gesti
 * senza un controller.
 */
struct REFACTORTACTICS_API FRTCameraGestureTuning
{
	/** Pixel oltre i quali `Ctrl`+`LMB` e' un trascinamento e non un click. */
	float ClickDragThreshold = 6.f;

	/** Sensibilita' del precision pan (`Ctrl`+`MMB`) rispetto al pan normale. */
	float PrecisionPanScale = 0.25f;

	/** Conversione pixel → valore d'asse per i gesti che riusano `AddPlanarMovement`. */
	float PixelsPerPanUnit = 10.f;

	/** Conversione pixel → passi di zoom per il dolly. */
	float DollySensitivity = 0.02f;

	/** Unita' mondo di peek per pixel di movimento del cursore. */
	float PeekSensitivity = 3.f;

	/** Frazione di peek residuo riassorbita per secondo quando `Ctrl` e' rilasciato. */
	float PeekReturnSpeed = 8.f;
};

/** Cosa e' stato il gesto `Ctrl`+`LMB` al rilascio del sinistro. */
enum class ERTPrimaryRelease : uint8
{
	/** Il sinistro non era armato da `Ctrl`: un click di selezione normale, non un gesto camera. */
	NotAGesture,
	/** Il gesto era armato, ma una schermata bloccante copre la partita: si disarma e basta. */
	Disarmed,
	/** Ha superato la soglia: era un'orbita, e non si emette altro. */
	Drag,
	/** Sotto la soglia: un click, cioe' un Set Pivot sulla cella sotto il cursore. */
	Click,
};

/**
 * **I GESTI DELLA CAMERA TATTICA**: lo stato dei tasti tenuti e l'aritmetica che trasforma un movimento del puntatore
 * in un'orbita, un dolly, un pan o un peek (#4065, S3).
 *
 * Stavano nel controller. Ora il controller **inoltra**: riceve l'input, legge dal mondo cio' che solo lui sa (la cella
 * sotto il cursore, se il cursore e' nella scena, se una schermata copre la partita) e passa qui la camera e i fatti.
 * Qui non si legge il mondo, e i gesti si provano con la sola camera.
 *
 * ⚠️ **Nessuna riflessione**, come `FRTPlaybackDirector`: lo stato e' di presentazione e muore col controller.
 */
class REFACTORTACTICS_API FRTCameraGestures
{
public:
	// ---- i tasti tenuti ------------------------------------------------------------------------------

	/** `Ctrl` premuto: arma i gesti camera (#1771). */
	void PressModifier() { bModifier = true; }

	/**
	 * `Ctrl` rilasciato, o annullato. I gesti armati cadono con il modificatore: senza, rilasciare `Ctrl` tenendo il tasto
	 * del mouse lascerebbe il gesto attivo e il movimento successivo orbiterebbe senza modificatore.
	 *
	 * ⚠️ Il peek **non** si azzera qui: rientra progressivamente in `UpdatePeekReturn`. Azzerarlo di colpo farebbe
	 * saltare la vista, ed e' esattamente cio' che il rientro esiste per evitare.
	 */
	void ReleaseModifier();

	/** Il tasto centrale tenuto arma l'orbita continua (#863). */
	void PressOrbit() { bOrbiting = true; }

	/**
	 * Il tasto centrale rilasciato, o annullato. ⚠️ Nessuno snap al rilascio: la spec lo vieta esplicitamente. Chi si e'
	 * fermato *fra* due file di celle — il caso d'uso per cui `D-142` tiene lo step a 45° — vedrebbe la vista scattare via.
	 */
	void ReleaseOrbit() { bOrbiting = false; }

	/** `Ctrl`+`LMB` premuto: comincia un gesto che al rilascio sara' un click o un trascinamento (#1772). */
	void ArmPrimary();

	/** `Ctrl`+`RMB` premuto: il dolly. Sopprime l'annullamento del waypoint per tutta la durata del gesto. */
	void ArmSecondary() { bSecondaryDown = true; }

	/** Il destro rilasciato, o annullato: chiude il dolly. */
	void ReleaseSecondary() { bSecondaryDown = false; }

	/**
	 * Il sinistro rilasciato, o annullato: cosa e' stato il gesto. Disarma sempre, anche quando una schermata bloccante
	 * copre la partita — altrimenti il gesto resterebbe armato per sempre.
	 */
	ERTPrimaryRelease ReleasePrimary(bool bGameplayInputBlocked, float ClickDragThreshold);

	// ---- il movimento --------------------------------------------------------------------------------

	/**
	 * Un movimento del mouse. I gesti `Ctrl` hanno la precedenza e **consumano** il movimento (`Route`); se nessuno e'
	 * armato, e' l'orbita di #863 (`Orbit`), che cosi' resta il ramo di default e non deve sapere che i gesti esistono.
	 */
	void MoveMouse(ARTCameraPawn& Cam, const FVector2D& Delta, const FRTCameraGestureTuning& Tuning);

	/**
	 * Instrada un movimento del puntatore al gesto camera armato, se ce n'e' uno. `true` se il movimento e' stato
	 * consumato.
	 */
	bool Route(ARTCameraPawn& Cam, const FVector2D& Delta, const FRTCameraGestureTuning& Tuning);

	/**
	 * L'orbita continua: X → yaw, Y → pitch. Senza il tasto centrale tenuto non fa niente.
	 *
	 * 🔴 **La guardia sta QUI, non nel chiamante.** Un'estrazione l'aveva lasciata in `OnOrbit`, quindi il percorso
	 * testabile scavalcava proprio il gate che doveva verificare — e il test l'ha fatto cadere alla prima esecuzione.
	 * Una decisione estratta per essere verificabile deve portarsi dietro **tutta** la decisione.
	 */
	void Orbit(ARTCameraPawn& Cam, const FVector2D& Delta) const;

	/** Un fotogramma di `WASD` (#3792): un ingresso a velocita', come l'edge pan. Segna che i tasti hanno mosso la vista. */
	void KeyPan(ARTCameraPawn& Cam, const FVector2D& Axis, float DeltaTime);

	/**
	 * Riassorbe il peek residuo, se `Ctrl` non e' premuto. La chiama il tick del controller, come l'edge pan (#1838): i
	 * due movimenti che non vogliono un tasto.
	 */
	void UpdatePeekReturn(ARTCameraPawn& Cam, float DeltaTime, float PeekReturnSpeed) const;

	/**
	 * `#1838` — l'edge pan di un fotogramma, dal cursore e dal viewport. Ritorna se la camera si e' mossa.
	 *
	 * Consuma il segno lasciato da `KeyPan`: lo legge e lo azzera all'inizio, perche' gira una volta per fotogramma,
	 * dopo l'input. Senza camera consuma il segno e non fa altro.
	 *
	 * ⚠️ **`IsGameplayInputBlocked` e' una funzione, non un valore**, perche' si chiede solo col cursore sulla banda: a
	 * riposo il lavoro e' un asse calcolato e basta, e la domanda costa una lettura del contesto del puntatore.
	 */
	bool ApplyEdgePan(ARTCameraPawn* Cam, const FVector2D& Cursor, const FVector2D& ViewportSize, float DeltaTime,
		TFunctionRef<bool()> IsGameplayInputBlocked);

	/**
	 * La rotella. Zooma solo col cursore nella scena (`#3748`): sopra una fascia dell'HUD la rotella e' dell'HUD. `Anchor`
	 * e' il centro della cella sotto il cursore, se ce n'e' una valida: lo zoom va verso di lei. Senza, zooma sul centro.
	 */
	static void Zoom(ARTCameraPawn& Cam, float Axis, bool bCursorInsideScene, const FVector* Anchor);

	/**
	 * `#1838` — l'edge pan si attiva solo se nessuna schermata bloccante copre la partita e nessun gesto camera e' in
	 * corso. «Bloccante» e' `ARTPlayerController::IsGameplayInputBlocked()`, cioe' il contratto del puntatore (#705).
	 */
	static bool EdgePanAllowed(bool bGameplayInputBlocked, bool bCameraGestureActive)
	{
		return !bGameplayInputBlocked && !bCameraGestureActive;
	}

	// ---- le letture ----------------------------------------------------------------------------------

	bool IsModifierDown() const { return bModifier; }
	bool IsOrbiting() const { return bOrbiting; }
	bool IsPrimaryDown() const { return bPrimaryDown; }
	bool IsSecondaryDown() const { return bSecondaryDown; }

	/** Quanta strada ha percorso il puntatore da quando `Ctrl`+`LMB` e' stato premuto. */
	float GetPrimaryDragDistance() const { return PrimaryDragDistance; }

private:
	/** Il tasto centrale e' tenuto: distingue «il mouse si muove» da «il giocatore sta orbitando». */
	bool bOrbiting = false;

	/** `Ctrl` tenuto: arma i gesti camera. */
	bool bModifier = false;

	/** `Ctrl`+`LMB` in corso. Diventa Set Pivot al rilascio se non ha superato la soglia, orbita se l'ha superata. */
	bool bPrimaryDown = false;

	/** `Ctrl`+`RMB` in corso: dolly. */
	bool bSecondaryDown = false;

	/**
	 * 🔑 **E' cio' che separa un click da un drag**, e la separazione dev'essere netta: superata la soglia il gesto e'
	 * un'orbita e il click **non si emette piu'**, altrimenti un trascinamento lungo produrrebbe anche un set-pivot al
	 * rilascio — due operazioni da un gesto solo.
	 */
	float PrimaryDragDistance = 0.f;

	/** L'ultimo stato dell'edge pan scritto nel log: si scrive solo quando cambia, non a ogni fotogramma. */
	ERTEdgePanState LastEdgePanState = ERTEdgePanState::Rest;

	/** Un tasto del pan ha mosso la vista in questo fotogramma: l'edge pan cede. Lo alza `KeyPan`, lo consuma `ApplyEdgePan`. */
	bool bKeyPanThisFrame = false;
};
