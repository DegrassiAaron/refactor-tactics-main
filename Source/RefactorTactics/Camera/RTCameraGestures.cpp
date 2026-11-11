#include "Camera/RTCameraGestures.h"
#include "Camera/RTCameraPawn.h"
#include "RefactorTactics.h"

void FRTCameraGestures::ReleaseModifier()
{
	bModifier = false;
	bPrimaryDown = false;
	bSecondaryDown = false;
	PrimaryDragDistance = 0.f;
}

void FRTCameraGestures::ArmPrimary()
{
	bPrimaryDown = true;
	PrimaryDragDistance = 0.f;
}

ERTPrimaryRelease FRTCameraGestures::ReleasePrimary(bool bGameplayInputBlocked, float ClickDragThreshold)
{
	if (!bPrimaryDown)
	{
		return ERTPrimaryRelease::NotAGesture; // rilascio di un click di selezione normale: non e' un gesto camera
	}

	// 🔴 **Il gesto puo' finire dentro una modale aperta a meta'** (code review, 2026-08-30): si preme il modificatore
	// col sinistro mentre si pianifica, il turno finisce o si apre la pausa, si rilascia — e senza questa guardia il Set
	// Pivot spostava la camera dietro la schermata. Lo stato del gesto si disarma comunque, altrimenti resterebbe armato
	// per sempre.
	const bool bWasDrag = PrimaryDragDistance > FMath::Max(ClickDragThreshold, 0.f);
	bPrimaryDown = false;
	PrimaryDragDistance = 0.f;
	if (bGameplayInputBlocked)
	{
		return ERTPrimaryRelease::Disarmed;
	}

	// 🔑 **Un gesto, una operazione.** Se ha superato la soglia era un'orbita, e il Set Pivot non si emette: altrimenti un
	// trascinamento lungo produrrebbe *anche* uno spostamento del pivot al rilascio, cioe' due comandi da un movimento solo.
	return bWasDrag ? ERTPrimaryRelease::Drag : ERTPrimaryRelease::Click;
}

void FRTCameraGestures::MoveMouse(ARTCameraPawn& Cam, const FVector2D& Delta, const FRTCameraGestureTuning& Tuning)
{
	if (Route(Cam, Delta, Tuning))
	{
		return;
	}
	Orbit(Cam, Delta);
}

bool FRTCameraGestures::Route(ARTCameraPawn& Cam, const FVector2D& Delta, const FRTCameraGestureTuning& Tuning)
{
	// ⚠️ **Nessuna guardia su `IsGameplayInputBlocked` qui, ed e' deliberato lasciarlo com'e'.** Il movimento del mouse
	// arriva da `OnOrbit`, che non l'ha mai avuta: con una modale aperta l'orbita di `#863` si e' sempre mossa.
	// Aggiungerla ai soli gesti nuovi darebbe due comportamenti diversi per lo stesso gesto a seconda del tasto premuto,
	// e cambiarla per tutti e' una modifica al comportamento pre-esistente che nessuna issue ha misurato. Il ramo bloccato
	// e' quello che **inizia** un gesto (`OnSelect`), che e' dove il blocco serve davvero.

	// `Ctrl`+`LMB`: si accumula la strada percorsa **prima** di decidere. Sotto la soglia non si orbita ancora —
	// altrimenti un click con un pixel di tremolio ruoterebbe la vista di un grado, e il set pivot arriverebbe su
	// un'inquadratura gia' cambiata.
	if (bPrimaryDown)
	{
		PrimaryDragDistance += Delta.Size();
		if (PrimaryDragDistance > FMath::Max(Tuning.ClickDragThreshold, 0.f))
		{
			Cam.AddOrbit(Delta.X, Delta.Y);
		}
		return true;
	}

	// `Ctrl`+`RMB`: dolly. Il verticale, perche' e' il gesto che tutti gli editor 3D usano per avvicinare.
	//
	// 🔴 **`Delta` e' in PIXEL, `AddZoom` moltiplica per `ZoomStep` (150), e il gesto era inusabile** (code review,
	// 2026-08-30): un movimento ordinario di 10 px/frame cambiava il braccio di 1500 unita', cioe' quasi mezzo intervallo
	// `[100, 4000]` in un frame. La rotella consegna ±1 e per quella `ZoomStep` e' tarato; un delta di puntatore no.
	// `DollySensitivity` fa la conversione, come `PeekSensitivity` la fa per il peek e `PrecisionPanScale` per il pan.
	if (bSecondaryDown)
	{
		Cam.AddZoom(Delta.Y * FMath::Max(Tuning.DollySensitivity, 0.f));
		return true;
	}

	if (!bModifier)
	{
		return false; // nessun gesto `Ctrl` armato: se ne occupa l'orbita di #863
	}

	// `Ctrl`+`MMB`: precision pan. Stessa direzione del pan normale, sensibilita' ridotta.
	//
	// 🔴 **Era piu' VELOCE del pan che doveva raffinare** (code review, 2026-08-30). `AddPlanarMovement` si aspetta un
	// valore d'ASSE — `WASD` consegna 1, cioe' 25 unita' per frame (`PanUnitDistance`) — mentre qui arrivava un delta in
	// **pixel**: 10 px/frame davano `10 * 0.25 * 25 = 62,5` unita', due volte e mezzo la tastiera. Un gesto chiamato
	// «precision» che si muove piu' in fretta di quello grezzo non e' una taratura sbagliata: e' un'unita' di misura
	// sbagliata, e `PrecisionPanScale` clampato a `1.0` non poteva ripararlo. `PixelsPerPanUnit` fa la conversione
	// **prima** che la scala si applichi.
	if (bOrbiting)
	{
		const float ToAxis = 1.f / FMath::Max(Tuning.PixelsPerPanUnit, 1.f);
		Cam.AddPlanarMovement(FVector2D(Delta.X, -Delta.Y) * ToAxis
			* FMath::Clamp(Tuning.PrecisionPanScale, 0.01f, 1.f));
		return true;
	}

	// `Ctrl` da solo: peek. Un offset **temporaneo** verso la direzione del cursore, in coordinate della vista — come il
	// pan, e per la stessa ragione: chi guida pensa in «su, giu', destra, sinistra» rispetto a cio' che vede, non in assi
	// della mappa.
	const float Yaw = Cam.GetCameraYaw();
	const FVector Forward = FRotator(0.f, Yaw, 0.f).RotateVector(FVector::ForwardVector);
	const FVector Right   = FRotator(0.f, Yaw, 0.f).RotateVector(FVector::RightVector);
	const FVector Wanted  = Cam.GetPeekOffset()
		+ (Right * Delta.X - Forward * Delta.Y) * FMath::Max(Tuning.PeekSensitivity, 0.f);
	Cam.SetPeekOffset(FVector(Wanted.X, Wanted.Y, 0.f)); // il peek scorre sul piano, non solleva la vista
	return true;
}

void FRTCameraGestures::Orbit(ARTCameraPawn& Cam, const FVector2D& Delta) const
{
	// Il movimento del mouse arriva a ogni frame: senza questa riga la vista ruoterebbe di continuo, senza che nessuno
	// abbia chiesto di orbitare.
	if (!bOrbiting)
	{
		return;
	}
	// X orizzontale → yaw, Y verticale → pitch. **Una sola** scrittura di trasformata: chiamare i due `Add*` in fila
	// aggiornerebbe il braccio due volte per frame di trascinamento. Il verso verticale e' una preferenza del pawn
	// (`bInvertOrbitPitch`), non una costante decisa qui: vedi la nota su quel campo — il default non e' stato verificato
	// con le mani.
	Cam.AddOrbit(Delta.X, Delta.Y);
}

void FRTCameraGestures::KeyPan(ARTCameraPawn& Cam, const FVector2D& Axis, float DeltaTime)
{
	// #3792: `WASD` e' un ingresso a velocita', come l'edge pan: la distanza dipende dal tempo del fotogramma.
	Cam.AddPlanarVelocity(Axis, DeltaTime);
	// #1838: in questo fotogramma i tasti hanno mosso la vista, e l'edge pan non deve sommarsi.
	bKeyPanThisFrame |= !Axis.IsNearlyZero();
}

void FRTCameraGestures::UpdatePeekReturn(ARTCameraPawn& Cam, float DeltaTime, float PeekReturnSpeed) const
{
	if (bModifier)
	{
		return; // con `Ctrl` premuto il peek e' guidato dal giocatore: non gli si tira contro
	}

	const FVector Current = Cam.GetPeekOffset();
	if (Current.IsNearlyZero())
	{
		return;
	}

	// Rientro esponenziale, con una soglia di arresto: senza, l'offset si avvicina a zero senza mai arrivarci e la camera
	// resterebbe per sempre a frazioni di unita' dal proprio pivot — vero per il motore, invisibile a schermo, e
	// sufficiente a far fallire un confronto esatto.
	const float Alpha = FMath::Clamp(FMath::Max(PeekReturnSpeed, 0.f) * DeltaTime, 0.f, 1.f);
	const FVector Next = FMath::Lerp(Current, FVector::ZeroVector, Alpha);
	Cam.SetPeekOffset(Next.SizeSquared() < 1.0 ? FVector::ZeroVector : Next);
}

bool FRTCameraGestures::ApplyEdgePan(ARTCameraPawn* Cam, const FVector2D& Cursor, const FVector2D& ViewportSize,
	float DeltaTime, TFunctionRef<bool()> IsGameplayInputBlocked)
{
	// I tasti del pan hanno gia' mosso la vista in questo fotogramma? Si legge e si consuma qui, all'inizio, perche'
	// questa funzione gira una volta per `PlayerTick`, dopo l'input.
	const bool bKeysPanned = bKeyPanThisFrame;
	bKeyPanThisFrame = false;

	if (!Cam)
	{
		return false;
	}

	// 🔑 A riposo il lavoro e' questo e basta: un asse calcolato, e uno stato che non cambia. Niente stringhe, niente
	// contesto del puntatore, niente log.
	const FVector2D Axis = ARTCameraPawn::ComputeEdgePanAxis(Cursor, ViewportSize, Cam->GetEdgePanBandPixels());
	ERTEdgePanState State = ERTEdgePanState::Rest;
	bool bMoved = false;
	FVector Before = FVector::ZeroVector;
	if (!Axis.IsNearlyZero())
	{
		if (!Cam->IsEdgePanEnabled())
		{
			State = ERTEdgePanState::RefusedSwitch;
		}
		// ⚠️ **Un'eccezione voluta a `IsGameplayInputBlocked`**, che non blocca la camera: `WASD`, zoom e orbita restano
		// vivi sotto un modale perche' non toccano il piano. L'edge pan e' diverso: lo guida il CURSORE, e con un modale
		// aperto il cursore sta sul modale (criterio 8 di #1838). Il predicato lo legge il controller e lo passa: se un
		// giorno «bloccato» si allarga, l'edge pan lo segue.
		else if (const bool bBlocked = IsGameplayInputBlocked(); !EdgePanAllowed(bBlocked, bOrbiting || bModifier))
		{
			State = bBlocked ? ERTEdgePanState::RefusedBlocked : ERTEdgePanState::RefusedGesture;
		}
		// Con un tasto del pan premuto vincono i tasti: due ingressi nello stesso fotogramma sommerebbero due
		// `AddPlanarMovement`, cioe' il doppio della velocita' di `WASD` (rilievo di review).
		else if (bKeysPanned)
		{
			State = ERTEdgePanState::RefusedKeys;
		}
		else
		{
			Before = Cam->GetCameraPivot();
			bMoved = Cam->ApplyEdgePan(Axis, DeltaTime);
			State = ERTEdgePanState::Active;
		}
	}

	// Il log scrive le TRANSIZIONI, non i fotogrammi.
	if (State != LastEdgePanState)
	{
		const TCHAR* Text = State == ERTEdgePanState::Rest ? TEXT("fermo")
			: State == ERTEdgePanState::RefusedSwitch ? TEXT("rifiutato: interruttore spento")
			: State == ERTEdgePanState::RefusedBlocked ? TEXT("rifiutato: modale aperto")
			: State == ERTEdgePanState::RefusedGesture ? TEXT("rifiutato: gesto camera in corso")
			: State == ERTEdgePanState::RefusedKeys ? TEXT("rifiutato: i tasti del pan hanno la precedenza")
			: TEXT("attivo");
		UE_LOG(LogRT, Verbose, TEXT("[RT] Edge pan: %s (asse %.2f, %.2f; %.1f uu nel primo fotogramma)"),
			Text, Axis.X, Axis.Y, State == ERTEdgePanState::Active ? FVector::Dist2D(Cam->GetCameraPivot(), Before) : 0.0);
		LastEdgePanState = State;
	}
	return bMoved;
}

void FRTCameraGestures::Zoom(ARTCameraPawn& Cam, float Axis, bool bCursorInsideScene, const FVector* Anchor)
{
	// Sopra una fascia dell'HUD la rotella e' dell'HUD (spec dell'ispettore §3 regola 6): la scheda scorre, il mondo resta.
	// ⚠️ `false` anche senza viewport o con la proiezione fallita (headless, primi fotogrammi): da li' lo zoom non parte.
	// `ZoomAction` e' legata alla sola rotella, quindi non c'e' uno zoom da tastiera che questo ramo spenga.
	if (!bCursorInsideScene)
	{
		return;
	}

	// L'ancora e' la **CELLA** sotto il cursore, convertita col centro del piano — non il punto d'impatto del raycast.
	//
	// 🔴 La prima stesura passava `Hit.ImpactPoint`, ed era lo stesso difetto di `#887` ripetuto: se il cursore sta su
	// un'unita', l'impatto e' sul cilindro, **180 unita' sopra il piano** (`UnitHalfHeight` + `VisualZOffset`). A pitch
	// -40° quella quota si traduce in ~215 unita' di scarto orizzontale — **oltre una cella**, cioe' piu' del doppio della
	// tolleranza di mezza cella che il DoD e il nome del test dichiarano. Trovato in code review.
	if (Anchor)
	{
		Cam.ZoomTowards(Axis, *Anchor);
		return;
	}

	// Il cursore e' nella scena ma non su una cella valida (fuori mappa): si zooma sul centro, com'e' sempre stato.
	// Meglio di non zoomare.
	Cam.AddZoom(Axis);
}
