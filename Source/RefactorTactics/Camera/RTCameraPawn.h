#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * Pawn camera tattica top-down/tre-quarti: braccio inclinato con zoom e pan sul piano.
 * Non usa fisica: si sposta con AddActorWorldOffset.
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
	}

	/**
	 * Fissa le sensibilita' (per i test).
	 *
	 * ⚠️ Senza, un test che chiama `AddYawContinuous(20)` pinna implicitamente `YawSensitivity = 0.5`,
	 * cioe' un valore che questa stessa issue dichiara **taratura aperta da playtest**: il giorno in cui
	 * il playtest lo cambiasse, il test cadrebbe per una decisione di tuning invece che per un difetto.
	 */
	void SetSensitivitiesForTest(float InYaw, float InPitch)
	{
		YawSensitivity = InYaw;
		PitchSensitivity = InPitch;
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
	 * ⚠️ **E' una preferenza, e il default NON e' stato verificato con le mani.** Il segno corretto dipende
	 * da come l'engine consegna `Mouse2D.Y`, e la prima stesura di #863 lo asseriva in un commento che si
	 * contraddiceva da solo — diceva «trascinare in basso abbassa lo sguardo» e poi ne derivava l'opposto.
	 * Invece di scegliere a tavolino: il verso e' un campo, il default e' dichiarato **non verificato**, e
	 * la voce PIE lo mette alla prova. Chi lo trova rovesciato lo cambia dal Details senza ricompilare.
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

#if WITH_EDITOR
	/** Rende immediate le modifiche di distanza/inclinazione fatte nel Details, anche durante il PIE. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
