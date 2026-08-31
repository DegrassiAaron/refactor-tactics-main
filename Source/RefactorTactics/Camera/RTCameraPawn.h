#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * Pawn camera tattica top-down/tre-quarti: braccio inclinato con zoom e pan sul piano.
 *
 * Non usa fisica: si sposta scrivendo la posizione, che da `#864` passa sempre per `ClampToSoftBounds`.
 * (Prima usava `AddActorWorldOffset`, e questa riga lo diceva ancora dopo che il codice era cambiato.)
 */
UCLASS()
class REFACTORTACTICS_API ARTCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ARTCameraPawn();

	/**
	 * Sposta la camera sul piano XY **relativamente a dove guarda**: `input.Y` e' «avanti sullo schermo»,
	 * `input.X` e' «a destra sullo schermo».
	 *
	 * Relativo e non in assi mondo, ed e' la meta' che rende usabile la rotazione: con il pan ancorato al
	 * mondo, dopo aver girato la vista di 90 gradi premere W avrebbe spostato l'inquadratura di lato. Chi
	 * guida non pensa in coordinate della mappa — pensa in «su, giu', destra, sinistra» rispetto a cio' che
	 * vede.
	 */
	void AddPlanarMovement(const FVector2D& Axis);

	/** Zoom variando la lunghezza del braccio (valore positivo = allontana). */
	void AddZoom(float AxisValue);
	/**
	 * Posizione normalizzata dentro l'intervallo di zoom: `0` al braccio minimo, `1` al massimo.
	 *
	 * E' la **sorgente unica** da cui i consumatori dello zoom derivano il proprio valore, invece di
	 * rileggere `TargetArmLength` e normalizzarlo ciascuno a modo suo. Prima di #1834 esisteva una sola
	 * derivazione — la scala del pan — scritta accanto al proprio consumatore come rapporto grezzo
	 * `TargetArmLength / DefaultArmLength`: corretta, ma non normalizzata e senza tetto (`0.125` a zoom
	 * minimo, `5.0` al massimo). Il difetto non era quella riga: era che la seconda curva sarebbe stata
	 * scritta allo stesso modo, e due formule accanto ai rispettivi consumatori non si possono confrontare
	 * ne' tarare insieme.
	 *
	 * Monotona crescente con la distanza. Senza `SpringArm` restituisce `0`.
	 */
	float GetZoomAlpha() const;

	/**
	 * Porta di scrittura equivalente a `GetZoomAlpha`: `alpha` fuori da `[0..1]` viene clampato.
	 *
	 * Esiste perche' zoom e alpha non possano divergere. Scrive `TargetArmLength` e riallinea tutto cio'
	 * che dipende dalla distanza — stato strategico, limiti del pivot, inclinazione — passando dagli
	 * stessi path di `AddZoom`, non da una seconda strada.
	 */
	void SetZoomAlpha(float InAlpha);

	/**
	 * Zoom **ancorato a un punto del mondo**: quel punto resta dov'e' sullo schermo mentre la distanza
	 * cambia. E' cio' che serve per avvicinarsi a una cella ai bordi del viewport senza alternare zoom e
	 * pan.
	 *
	 * Il pivot si sposta verso l'ancora in proporzione a quanto la distanza si e' ridotta:
	 *
	 *     NuovoPivot = Ancora + (Pivot - Ancora) * (NuovoBraccio / VecchioBraccio)
	 *
	 * ⚠️ **E' esatta in proiezione ortografica**; in prospettiva e' un'approssimazione, tanto migliore
	 * quanto piu' l'ancora sta sul piano di lavoro — che e' il caso d'uso, perche' l'ancora viene dalla
	 * **cella** sotto il cursore e non dal punto d'impatto del raycast.
	 *
	 * ⏳ **L'errore di prospettiva NON e' misurato**: il test verifica che il pivot si sposti come la
	 * formula prescrive, non di quanto il punto scivoli a schermo in proiezione prospettica — servirebbe
	 * proiettare, e in headless non c'e' un viewport. La tolleranza di mezza cella vincola la formula, e
	 * il giudizio «lo zoom va dove guardo» resta una verifica PIE. Una stesura precedente affermava qui
	 * che «questa formula sta molto sotto» la tolleranza: era una dichiarazione senza evidenza.
	 *
	 * Se il braccio e' gia' al limite (`Min`/`Max`) il pivot **non si muove**: senza quella guardia,
	 * continuare a girare la rotellina a fondo corsa trascinerebbe la vista verso il cursore all'infinito.
	 */
	void ZoomTowards(float AxisValue, const FVector& WorldAnchor);

	/**
	 * Ruota la vista attorno al punto inquadrato (valore positivo = orario visto dall'alto).
	 *
	 * Su una griglia esagonale girare non e' un vezzo: le unita' sono cilindri e le anteprime si disegnano a
	 * terra, quindi da una sola angolazione una cella dietro a qualcuno resta illeggibile. La rotazione e' il
	 * modo economico di guardarci dietro — l'alternativa sarebbe rendere trasparente mezza scena.
	 */
	void AddYaw(float AxisValue);

	/**
	 * Ruota la vista in modo CONTINUO, di un angolo proporzionale all'input (trascinamento).
	 *
	 * Convive con `AddYaw`, che resta lo scatto di `Q`/`E`: sono due gesti diversi sullo stesso stato.
	 * Guardare *fra* due file di celle — la ragione per cui `D-142` fissa lo step a 45° e non a un
	 * divisore di 60 — richiede di potersi fermare dove si vuole, non solo su otto orientamenti.
	 */
	void AddYawContinuous(float AxisValue);

	/**
	 * Inclina la vista, clampata in `[-89, 0]`.
	 *
	 * ⚠️ Il clamp e' **qui, in codice**: il `meta = (ClampMin/ClampMax)` del campo vincola solo il widget
	 * del Details, e la issue lo dava per un vincolo a runtime che non e'.
	 */
	void AddPitch(float AxisValue);

	/**
	 * Orbita: yaw e pitch nello stesso gesto, con **una sola** scrittura di trasformata.
	 *
	 * Chiamare `AddYawContinuous` e `AddPitch` in fila funziona ma aggiorna il braccio due volte per
	 * frame di trascinamento — e la seconda scrittura scarta il risultato della prima. Sul percorso caldo
	 * dell'input, con `bEnableCameraLag` attivo, e' lavoro doppio per niente.
	 */
	void AddOrbit(float DeltaYaw, float DeltaPitch);

	/**
	 * Il **pivot**: il punto del mondo che la camera sta guardando, e il riferimento di tutto il resto.
	 * La posizione dell'attore ne e' **derivata** — `Pivot + PeekOffset` — e non il contrario.
	 *
	 * 🔴 **Esiste da `#1770` perche' prima non c'era**, e la ragione e' la stessa che ha prodotto
	 * `DefaultPitch` in `#863`: «dove guarda la camera» e «dove sta il pawn» erano la stessa variabile.
	 * Finche' nessuno applicava offset temporanei la cosa non si vedeva; ma un peek, una transizione o
	 * uno shake si sarebbero scritti *sopra* il riferimento invece che *accanto*, e al rilascio non ci
	 * sarebbe stato niente a cui tornare.
	 *
	 * ⚠️ **Da qui in poi `SetActorLocation` non e' piu' il canale per spostare la camera**: chi la muove
	 * chiama `SetCameraPivot`, e chi la legge chiama questo. Scrivere la posizione direttamente lascia il
	 * pivot indietro, e il prossimo `ApplyPivot` riporta la camera dov'era.
	 */
	const FVector& GetCameraPivot() const { return CameraPivot; }

	/**
	 * Sposta il pivot, **clampato ai soft bounds**, e riapplica la trasformata conservando il peek.
	 *
	 * E' il punto unico di scrittura: `FocusOn`, `AddPlanarMovement`, `ZoomTowards`, `RecenterView` e il
	 * set-pivot di `#1772` passano tutti da qui, cosi' il clamp non puo' essere dimenticato in uno dei
	 * cinque — che e' il difetto gia' pagato da `#873` sulla distanza e da `#864` sullo zoom.
	 */
	void SetCameraPivot(const FVector& NewPivot);

	/**
	 * Offset visuale **temporaneo** verso cui la camera sbircia: non entra nel pivot, e viene azzerato dal
	 * rientro senza lasciare traccia.
	 *
	 * Limitato in lunghezza a `MaxPeekDistance`: un peek che arriva lontano quanto un pan sarebbe un pan,
	 * e nessuno saprebbe piu' dove si trova davvero la camera.
	 */
	void SetPeekOffset(const FVector& Offset);
	const FVector& GetPeekOffset() const { return PeekOffset; }

	/**
	 * `#1774` — la vista e' **strategica**: conseguenza dello zoom, non una modalita' (`D-252`).
	 *
	 * Non c'e' un comando che la accenda e non c'e' uno stato in piu' da mantenere: la distanza la sta gia'
	 * tenendo `TargetArmLength`, e questa e' la sua lettura semantica.
	 */
	bool IsStrategicView() const { return bStrategicView; }

	/**
	 * Rilegge lo stato strategico dalla distanza corrente, **con isteresi**.
	 *
	 * ⚠️ Due soglie e non una. Con una sola, la vista oscillerebbe fra due presentazioni a ogni tacca di
	 * rotella nell'intorno del valore: si vede subito e si diagnostica tardi, perche' il sintomo sembra uno
	 * sfarfallio di rendering mentre la causa e' una disuguaglianza.
	 *
	 * Ritorna `true` se lo stato e' cambiato.
	 */
	bool UpdateStrategicState();

	/** Fissa le due soglie strategiche (per i test). Taratura aperta: si tarano in `L_CameraFeatureLab`. */
	void SetStrategicThresholdsForTest(float InEnter, float InExit)
	{
		StrategicEnterThreshold = InEnter;
		StrategicExitThreshold = InExit;
	}

	/**
	 * `#1778` — i limiti del pivot che tengono conto del VIEWPORT (`D-251`).
	 *
	 * Pura e senza mondo: prende come **parametri** cio' che a runtime verrebbe dal viewport, ed e' la
	 * ragione per cui e' verificabile headless — dove un viewport non esiste, e dove `ZoomTowards` ha gia'
	 * dovuto dichiarare di non poter misurare l'errore di prospettiva.
	 *
	 * `AllowedArea` e' la zona che si puo' mostrare (mappa + buffer scenico, `D-250`).
	 * `AllowedOutsideFraction` e' quanta parte del semiquadro puo' cadere fuori da quella zona: `0` incolla
	 * il quadro dentro l'area, `1` non limita nulla. ⚠️ Non e' «zero pixel fuori», che sarebbe una gabbia e
	 * renderebbe il bordo inavvicinabile.
	 *
	 * Se l'area e' piu' piccola dell'inquadratura — mappa piccola, o zoom al massimo — l'intervallo si
	 * ridurrebbe a un rovescio: in quel caso il pivot si **inchioda al centro** invece di produrre un
	 * `Clamp` su estremi invertiti, che lo bloccherebbe a un angolo senza che nulla lo dica.
	 */
	static FBox2D ComputeEffectivePivotBounds(const FBox2D& AllowedArea, float ArmLength,
		float PitchDegrees, float YawDegrees, float HorizontalFovDegrees, float AspectRatio,
		float AllowedOutsideFraction);

	/**
	 * Metriche del viewport per il calcolo dei limiti (per i test, e per chi non ha un viewport).
	 *
	 * `AspectRatio <= 0` disattiva i limiti viewport-aware e riporta al clamp per sole celle: e' il **caso
	 * degenere dichiarato**, non un ripiego silenzioso.
	 */
	void SetViewportMetricsForTest(float InAspectRatio, float InHorizontalFovDegrees)
	{
		ViewportAspectRatio = InAspectRatio;
		ViewportHorizontalFov = InHorizontalFovDegrees;
	}

	/** Direzione di sguardo corrente sul piano, in gradi. Serve al pan relativo e ai test. */
	float GetCameraYaw() const { return CameraYaw; }

	/** Inclinazione corrente, in gradi (negativa = verso il basso). */
	float GetCameraPitch() const { return CameraPitch; }

	/**
	 * Riporta la camera al centro della griglia e ripristina l'inquadratura di default: distanza
	 * `DefaultArmLength` **clampata** in `[MinArmLength, MaxArmLength]`, e orientamento dritto.
	 */
	void RecenterView();

	/**
	 * Porta il pivot su un punto del mondo — **quota compresa** — e lascia stare lo zoom, che resta quello
	 * che il giocatore si e' regolato.
	 *
	 * ⚠️ La quota passata deve essere quella del **piano**, non quella visiva di un attore: `ARTUnit` sta
	 * mezzo corpo sopra la cella (`VisualZOffset`), quindi chi inquadra un'unita' converte la sua cella
	 * invece di leggerne la posizione.
	 *
	 * 🔴 Questo commento diceva «mantenendo la quota e lo zoom correnti», e descriveva il difetto di
	 * `#887`: conservare la quota significava tenere quella del PlayerStart, con cui il pawn nasce.
	 */
	void FocusOn(const FVector& WorldLocation);

	/** Vista di MISURA a picco sull'intera board (`rt.Camera.TopDown`). `false` se la mappa non e' utilizzabile. */
	bool ApplyTopDownView(const class ARTHexMapActor* HexMap, const FVector& Origin, float HexSize, float LayerHeight);

	/** Programma lo scatto di confronto della vista a picco (`rt.Camera.TopDownShot`). */
	void ScheduleTopDownShot();

	/**
	 * Inquadra la squadra del giocatore: centra sul punto medio delle sue unita' e avvicina a
	 * `MatchStartArmLength`. Ritorna falso se non c'e' nulla da inquadrare (nessuna mappa, o nessuna unita'
	 * della propria squadra ancora nel mondo), cosi' il chiamante puo' riprovare o ripiegare su `RecenterView`.
	 *
	 * Sola presentazione: legge lo stato, non lo cambia.
	 */
	bool FrameOwnTeam();

	/**
	 * Imposta i tre limiti di distanza in un colpo solo (per i test).
	 *
	 * Servono insieme perche' insieme definiscono uno **stato di partenza**: il difetto che questo setter
	 * esiste per rendere verificabile — `Home` che porta il braccio fuori intervallo — si manifesta solo
	 * con un default fuori da `[Min, Max]`, cioe' con una combinazione, non con un valore.
	 *
	 * Il nome dichiara che si sta scavalcando la taratura dell'editor: in partita questi valori vengono
	 * dal Details, e restano `protected` perche' sono configurazione del designer, non API.
	 */
	/**
	 * Imposta l'inclinazione di default e quella corrente (per i test).
	 *
	 * Servono insieme per la stessa ragione dei tre limiti di distanza: quello che un test deve poter
	 * stabilire e' uno **stato di partenza**. Il nome dichiara che si sta scavalcando la taratura
	 * dell'editor.
	 */
	void SetPitchForTest(float InDefault, float InCurrent)
	{
		DefaultPitch = InDefault;
		CameraPitch = InCurrent;
		// ⚠️ **L'offset va riallineato**, altrimenti il primo ricalcolo dopo uno zoom riporterebbe il
		// pitch al derivato e lo stato di partenza che il test ha appena stabilito sparirebbe. Vale
		// finche' `PitchZoomDelta` e' `0` — il default — dove il derivato coincide con `DefaultPitch`;
		// un test che tara la curva imposta il delta **dopo** questa chiamata.
		ManualPitchOffset = InCurrent - InDefault;
	}

	/**
	 * Fissa le sensibilita' (per i test).
	 *
	 * ⚠️ Senza, un test che chiama `AddYawContinuous(20)` pinna implicitamente `YawSensitivity = 0.5`,
	 * cioe' un valore che questa stessa issue dichiara **taratura aperta da playtest**: il giorno in cui
	 * il playtest lo cambiasse, il test cadrebbe per una decisione di tuning invece che per un difetto.
	 */
	/**
	 * Fissa l'inclinazione aggiuntiva a zoom massimo (per i test).
	 *
	 * ⚠️ Serve a rendere **discriminante** il test della derivazione: il default e' `0`, cioe' una curva
	 * neutra, e un test che verificasse la derivazione senza impostare un delta passerebbe anche se la
	 * funzione ignorasse del tutto lo zoom.
	 */
	void SetPitchZoomDeltaForTest(float InDelta) { PitchZoomDelta = InDelta; }

	/** Inclinazione accumulata dall'input manuale, separata dalla parte derivata dallo zoom. */
	float GetManualPitchOffset() const { return ManualPitchOffset; }
	void SetSensitivitiesForTest(float InYaw, float InPitch)
	{
		YawSensitivity = InYaw;
		PitchSensitivity = InPitch;
	}

	/**
	 * Fissa il margine dei soft bounds (per i test).
	 *
	 * Serve a dimostrare che il limite **viene dal campo** e non da un caso: un test che verifica solo
	 * «il centro resta entro N» passa anche con margine zero, se N e' abbastanza largo.
	 */
	void SetBoundsMarginForTest(float InCells) { BoundsMarginCells = InCells; }

	/**
	 * Fissa il limite del peek (per i test).
	 *
	 * Stessa disciplina delle sensibilita': senza, un test che verifica il limite pinnerebbe
	 * implicitamente il default — cioe' un valore che questa stessa issue dichiara **taratura aperta**, e
	 * il test cadrebbe il giorno in cui il playtest lo cambia.
	 */
	void SetMaxPeekDistanceForTest(float InMax) { MaxPeekDistance = InMax; }

	/**
	 * Fissa il passo dello zoom (per i test).
	 *
	 * ⚠️ Serve per la stessa ragione di `SetSensitivitiesForTest`: un test sull'isteresi che chiama
	 * `AddZoom(-1)` una volta **pinna implicitamente** `ZoomStep = 150`, cioe' un valore di taratura. Quel
	 * test distingue l'isteresi da una soglia sola soltanto finche' un passo di zoom resta piu' piccolo
	 * del divario fra le due soglie — e anche quel divario e' taratura aperta. Con due numeri liberi in
	 * gioco il test cadrebbe per una decisione di tuning invece che per un difetto, ed e' esattamente il
	 * modo in cui una suite comincia a mentire.
	 */
	void SetZoomStepForTest(float InStep) { ZoomStep = InStep; }

	/**
	 * Porta il pivot dove si vuole **scavalcando i soft bounds** (per i test).
	 *
	 * Serve per la stessa ragione degli altri `*ForTest`: cio' che un test deve poter stabilire e' uno
	 * **stato di partenza**, e `SetCameraPivot` lo clampa — su una fixture centrata lontano dall'origine,
	 * chiedere `(0,0,0)` darebbe un punto diverso da quello scritto, e le misure di spostamento
	 * partirebbero da una base che il test non ha scelto.
	 *
	 * Il nome dichiara che si sta scavalcando un invariante: in partita il pivot passa **sempre** dal
	 * clamp.
	 */
	void SetCameraPivotForTest(const FVector& InPivot)
	{
		CameraPivot = InPivot;
		ApplyPivot();
	}

	void SetArmLengthRangeForTest(float InDefault, float InMin, float InMax)
	{
		// Un intervallo rovesciato non e' uno scenario da coprire, e' un test scritto male: senza questa
		// guardia ogni `Clamp` inchioderebbe la distanza a un estremo e le assertion passerebbero o
		// fallirebbero per ragioni scollegate dal codice in prova. E' raggiungibile anche dall'editor,
		// perche' `MinArmLength` e `MaxArmLength` non hanno `ClampMin`/`ClampMax`.
		checkf(InMin <= InMax, TEXT("SetArmLengthRangeForTest: intervallo rovesciato (Min=%.1f > Max=%.1f)"),
			InMin, InMax);
		DefaultArmLength = InDefault;
		MinArmLength = InMin;
		MaxArmLength = InMax;
	}

protected:
	virtual void BeginPlay() override;

	/**
	 * Allinea il pivot alla posizione con cui il pawn nasce.
	 *
	 * Serve perche' il pawn viene spawnato dal `PlayerStart` (o da `SpawnActor` nei test) **prima** che
	 * qualcuno chiami `SetCameraPivot`: senza questa riga il pivot resterebbe a zero, e la prima scrittura
	 * di trasformata teletrasporterebbe la camera all'origine del mondo.
	 */
	virtual void PostInitializeComponents() override;

	/**
	 * Il pivot: cio' che la camera guarda. `VisibleAnywhere` e non `EditAnywhere` — si tara dal gioco, non
	 * dal Details, e mostrarlo aiuta a diagnosticare un peek che non e' rientrato.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Camera")
	FVector CameraPivot = FVector::ZeroVector;

	/** Offset temporaneo del peek (`#1772`). Zero a riposo: se non lo e', qualcosa non e' rientrato. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Camera")
	FVector PeekOffset = FVector::ZeroVector;

	/**
	 * Quanto lontano puo' arrivare il peek, in unita' mondo. **Deliberatamente piccolo**: il peek serve a
	 * sbirciare oltre un bordo senza perdere il punto di riferimento, e un valore grande lo trasformerebbe
	 * in un pan che si annulla da solo — cioe' in un comando che sposta la vista e poi la ributta
	 * indietro, che e' peggio di non averlo.
	 *
	 * ⏳ **Taratura aperta**: nessun numero e' stato istruito, e si tara in `L_CameraFeatureLab` (`#1780`).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.0"))
	float MaxPeekDistance = 400.f;

	/** Scrive `Pivot + PeekOffset` sulla trasformata. Unico punto: la posizione e' un **derivato**. */
	void ApplyPivot();

	// --- #1774 · lo stato strategico ---------------------------------------------------------------

	/**
	 * Distanza oltre la quale la vista diventa strategica. ⏳ **Taratura aperta**: nessun numero e' stato
	 * istruito, e si tara guardando (`#1780`). Il default sta fra `DefaultArmLength` e `MaxArmLength`
	 * perche' altrimenti la soglia sarebbe irraggiungibile o sempre superata — che non e' una taratura,
	 * e' un difetto.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.0"))
	float StrategicEnterThreshold = 2400.f;

	/** Distanza sotto la quale si torna tattici. **Minore** di `StrategicEnterThreshold`: e' l'isteresi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.0"))
	float StrategicExitThreshold = 1900.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Camera")
	bool bStrategicView = false;

	// --- #1778 · i limiti che conoscono il viewport ------------------------------------------------

	/**
	 * Aspect ratio del viewport. `<= 0` = ignoto: i limiti tornano a essere quelli per sole celle.
	 * Aggiornato in `BeginPlay` e a ogni scrittura di pivot quando un viewport esiste.
	 */
	float ViewportAspectRatio = 0.f;

	/** Campo visivo orizzontale in gradi, letto dal `UCameraComponent`. */
	float ViewportHorizontalFov = 90.f;

	/**
	 * Quanta parte del semiquadro puo' cadere fuori dalla zona prevista. ⏳ Taratura aperta.
	 *
	 * ⚠️ Non e' zero, e non deve esserlo: incollare il quadro dentro l'area renderebbe il bordo
	 * inavvicinabile, cioe' sostituirebbe un difetto con uno peggiore.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AllowedOutsideFraction = 0.35f;

	/** Aggiorna `ViewportAspectRatio`/`ViewportHorizontalFov` dal viewport, se ce n'e' uno. */
	void RefreshViewportMetrics();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float PanSpeed = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float ZoomStep = 150.f;

	/** Distanza (arm length) iniziale all'avvio: piu' piccola = piu' vicino alla mappa. Tra Min e Max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float DefaultArmLength = 800.f;

	/**
	 * Distanza all'avvio della partita, quando la camera si posiziona sulla squadra del giocatore: piu' piccola
	 * di `DefaultArmLength` perche' si parte guardando le proprie unita', non l'intera mappa. `Home`
	 * (`RecenterView`) resta a `DefaultArmLength` clampata: sono due inquadrature diverse.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float MatchStartArmLength = 450.f;

	/** Zoom minimo (piu' piccolo = primo piano piu' ravvicinato sui personaggi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float MinArmLength = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float MaxArmLength = 4000.f;

	/**
	 * Inclinazione a cui `Home` riporta la vista, in gradi (negativa = guarda verso il basso). -55 e' una
	 * vista a volo d'uccello; valori meno ripidi (-40, -35) danno un'inquadratura piu' laterale, in cui i
	 * personaggi si leggono meglio. E' la **taratura del designer**.
	 *
	 * 🔴 Esiste da `#863` perche' prima non c'era: `CameraPitch` faceva da default *e* da stato corrente, e
	 * finche' nessun input lo toccava la cosa non si vedeva. Appena il pitch e' diventato regolabile a
	 * runtime, il valore a cui tornare sarebbe andato perso al primo trascinamento — lo zoom ha
	 * `DefaultArmLength` accanto a `TargetArmLength`, lo yaw ha lo zero: il pitch non aveva nulla.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float DefaultPitch = -40.f;

	/**
	 * Inclinazione **corrente**: parte da `DefaultPitch` e cambia con `AddPitch`.
	 *
	 * ⚠️ Il clamp sta in `AddPitch`, **non** nel `meta`: `ClampMin`/`ClampMax` vincolano il widget del
	 * Details e non le assegnazioni da codice. Il `meta` resta perche' serve a chi tara in editor.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float CameraPitch = -40.f;

	/** Gradi di inclinazione per unita' di input. Taratura aperta: nessun numero e' stato istruito. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.01"))
	float PitchSensitivity = 0.5f;
	/**
	 * Gradi di inclinazione **aggiuntiva** a zoom massimo, rispetto a `DefaultPitch`.
	 *
	 * Il pitch derivato vale `DefaultPitch + PitchZoomDelta * GetZoomAlpha()`, e l'input manuale resta un
	 * offset sopra quel valore invece di sostituirlo: allontanandosi si guadagna la vista dall'alto senza
	 * che l'orbita fatta a mano venga silenziosamente disfatta.
	 *
	 * ⛔ **Default `0`, cioe' nessuna derivazione**, ed e' deliberato: la curva esiste e si tara in
	 * `L_CameraFeatureLab` (#1780). Un valore scelto qui prima di essere provato diventerebbe canone per
	 * inerzia — e finche' resta `0` l'inclinazione in partita e' identica a quella di prima di #1834.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float PitchZoomDelta = 0.f;

	/**
	 * Quanto l'input manuale ha inclinato la vista **oltre** il valore derivato dallo zoom.
	 *
	 * 🔴 Separarlo da `CameraPitch` e' cio' che rende la derivazione non distruttiva. Sommando
	 * direttamente su `CameraPitch`, il primo ricalcolo dopo uno scroll avrebbe cancellato l'orbita
	 * dell'utente — e il sintomo («la camera raddrizza da sola») non assomiglia alla causa.
	 *
	 * Clampato in modo che il risultato resti dentro `[MinPitch, MaxPitch]`: senza, trascinare a fondo
	 * corsa accumulerebbe un offset che il clamp nasconde, e il primo trascinamento nel verso opposto non
	 * risponderebbe.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Camera")
	float ManualPitchOffset = 0.f;

	/**
	 * Quanto il centro camera puo' andare **oltre il bordo della mappa**, in celle.
	 *
	 * Deciso da `#864` (2026-08-15) e istruito sulla scala reale: con `3` celle, su una mappa di raggio 4
	 * il centro arriva a 7 celle dall'origine — cioe' il bordo si puo' portare **al centro dello schermo**
	 * e si vede cosa c'e' appena fuori, senza perdere la mappa.
	 *
	 * ⚠️ Misura **fissa**, non proporzionale al raggio: il margine serve al *bordo*, e il bordo e' locale.
	 * Un 50% del raggio darebbe dieci celle di vuoto navigabile su una mappa Operations, dove il bordo
	 * riempie gia' lo schermo.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.0"))
	float BoundsMarginCells = 3.f;

	/**
	 * Estremi dell'inclinazione. **Costanti, non tarabili**: non sono una preferenza ma il punto in cui la
	 * vista degenera — a `0` la camera guarda l'orizzonte e il piano di gioco sparisce in una linea, a
	 * `-90` `FRotator` perde un grado di liberta' (gimbal) e lo yaw smette di avere effetto visibile.
	 * `-89` sta un grado prima di quel bordo.
	 */
	static constexpr float MinPitch = -89.f;
	static constexpr float MaxPitch = 0.f;

	/** Gradi di rotazione per unita' di input nel trascinamento continuo. Taratura aperta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "0.01"))
	float YawSensitivity = 0.5f;

	/**
	 * Verso del trascinamento verticale nell'orbita.
	 *
	 * ✅ **Verificato in PIE il 2026-08-16: il default e' corretto cosi' com'e'.**
	 *
	 * Fino a quella prova era una scommessa, e il commento lo diceva. Il segno giusto dipende da come
	 * l'engine consegna `Mouse2D.Y`, e la prima stesura di #863 lo asseriva in un commento che si
	 * contraddiceva da solo — diceva «trascinare in basso abbassa lo sguardo» e poi ne derivava l'opposto.
	 * Invece di scegliere a tavolino il verso e' diventato un campo, il default e' stato dichiarato **non
	 * verificato**, e **`PIE-CAM-ORBIT`** (`docs/technical/test-manuali-pie.md`) l'ha messo alla prova.
	 *
	 * Resta un campo: e' comunque una preferenza, e chi la vuole rovesciata la cambia dal Details senza
	 * ricompilare.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	bool bInvertOrbitPitch = true;

	/**
	 * Direzione di sguardo sul piano, in gradi. 0 = l'orientamento di partenza, quello di ogni inquadratura
	 * prima che esistesse la rotazione.
	 *
	 * Normalizzata in `[0, 360)` a ogni passo: senza, ruotando a lungo nella stessa direzione il valore
	 * cresceva senza fine e in editor il campo diventava illeggibile.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float CameraYaw = 0.f;

	/**
	 * Gradi per passo di rotazione. 45 e' deliberatamente **non** un divisore di 60: sarebbe la scelta ovvia
	 * su una griglia esagonale, ma agganciare la vista agli assi della griglia rende impossibile guardare
	 * *fra* due file di celle, che e' esattamente cio' che serve quando un cilindro copre quello che c'e'
	 * dietro.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "1.0", ClampMax = "90.0"))
	float YawStep = 45.f;

	/** Applica distanza (clampata tra Min e Max) e inclinazione correnti al braccio. */
	void ApplyViewSettings();

	/**
	 * Scrive sul braccio la rotazione corrente. Unico punto: la stessa espressione era copiata in quattro
	 * posti, ed e' il difetto che `#873` ha gia' pagato una volta — quattro copie da tenere allineate a
	 * mano si disallineano alla prima aggiunta (un roll, un clamp, un `bUsePawnControlRotation`).
	 */
	void ApplyArmRotation();
	/** Scala del pan derivata dalla sorgente unica: quanto terreno copre una unita' di input. */
	float GetPanDistanceScale() const;

	/** Riallinea `CameraPitch` a `derivato(alpha) + ManualPitchOffset`, clampando entrambi. */
	void RecomputePitchFromZoom();

	/**
	 * Riporta una posizione dentro i limiti di scorrimento: estensione reale delle celle della mappa piu'
	 * `BoundsMarginCells`. Senza mappa non c'e' un bordo, e la posizione passa intatta.
	 *
	 * ⚠️ **Non e' collisione**: `bDoCollisionTest` resta `false` e `#864` lo dichiara fuori scope.
	 * Reintrodurre il popping da SpringArm per ottenere dei limiti sarebbe un passo indietro.
	 */
	FVector ClampToSoftBounds(const FVector& Desired) const;

#if WITH_EDITOR
	/** Rende immediate le modifiche di distanza/inclinazione fatte nel Details, anche durante il PIE. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
