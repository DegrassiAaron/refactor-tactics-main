#pragma once

#include "CoreMinimal.h"
#include "RTGraykitTypes.generated.h"

/**
 * I punti nominati del corpo graykit, in spazio LOCALE all'unita'.
 *
 * 🔑 **Sono nomi, non numeri**, ed e' l'intera ragione per cui l'enum esiste. Un descriptor che dicesse
 * «ruota attorno a `(0,0,-90)`» smetterebbe di essere vero il giorno in cui `UnitHalfHeight` cambia; uno che
 * dice `Ground` resta vero perche' il numero lo risolve `URTGraykitLibrary::AnchorOffset` da quella costante.
 *
 * ⛔ **Nessun anchor e' una posizione di gioco.** Sono offset di presentazione risolti attorno al pivot
 * visivo: la cella autorevole resta `FRTCellId`, e nulla qui la tocca. La regola sta in `CLAUDE.md` §11
 * (`Logical vs Presentation`) ed e' verificata da `FRTGraykitNoGameplayMutationTest`.
 *
 * Aggiungere valori solo IN CODA: `AllAnchors()` li enumera e i test contano.
 */
UENUM(BlueprintType)
enum class ERTGraykitAnchor : uint8
{
	/** Il pivot visivo, cioe' il centro del cilindro. Offset nullo per definizione. */
	Center,
	/** Il piano della cella, sotto i piedi. E' l'anchor della VERITA' TATTICA: la base non mente mai. */
	Ground,
	/** La sommita' del cilindro. */
	Top,
	/** Davanti, lungo il forward dell'attore (+X). */
	Front,
	/** Dietro (-X). */
	Back,
	/** A sinistra (-Y). ⚠️ Unreal e' left-handed: la sinistra dell'attore e' -Y, non +Y. */
	Left,
	/** A destra (+Y). */
	Right,
	/** Dove il braccio sinistro tiene cio' che tiene. */
	LeftHand,
	/** Dove il braccio destro tiene cio' che tiene. */
	RightHand
};

/**
 * La parte del corpo che un operatore muove.
 *
 * ⚠️ **Separata da `ERTGraykitAnchor` e non dedotta da esso**: l'anchor dice *attorno a quale punto* si
 * ruota, la parte dice *cosa* si muove. Confonderli renderebbe impossibile l'unico caso che GK-03 chiede
 * davvero — un braccio che oscilla attorno alla spalla mentre il corpo si inclina attorno ai piedi.
 */
UENUM(BlueprintType)
enum class ERTGraykitPart : uint8
{
	Body,
	LeftArm,
	RightArm
};

/**
 * Come il tempo normalizzato viene rimappato prima di pesare la magnitudine.
 *
 * ⛔ **Non e' un `UCurveFloat`, ed e' una scelta contro la via che il mandato suggeriva.** Una curva e' un
 * `.uasset`: non si diffa, non si valuta in un test headless, e la sua assenza produce un fallback muto.
 * Queste sono funzioni, quindi il test le misura senza aprire l'Editor — che e' la condizione posta dal
 * mandato stesso (*«non usare Unreal Editor se il test puo' essere eseguito come automation test»*).
 *
 * ⚠️ Se un giorno servisse una curva autorata, il posto dove aggiungerla e' un valore NUOVO di questo enum
 * con un riferimento asset accanto — non la sostituzione di questi, che restano valutabili a secco.
 */
UENUM(BlueprintType)
enum class ERTGraykitEasing : uint8
{
	/** Nessuna rimappatura. */
	Linear,
	/** Parte piano (`t^2`). Anticipazione. */
	EaseIn,
	/** Arriva piano (`1-(1-t)^2`). Assestamento. */
	EaseOut,
	/** Piano ai due estremi. Il default di una locomozione che parte e si ferma. */
	EaseInOut,
	/** Andata e ritorno dentro la finestra: `0 -> 1 -> 0`. E' cio' che rende un `Pulse` un impulso. */
	PingPong
};

/**
 * Gli operatori procedurali che agiscono sulla POSA del corpo.
 *
 * 🔴 **Sono sette, e il mandato ne elencava diciassette. La differenza non e' una riduzione di ambizione:
 * e' un confine di ownership, misurato.** Degli altri dieci:
 *
 * - `Line`, `Arrow`, `Arc`, `Ring`, `Cone`, `Disc`, `Beam` non sono pose, sono **geometrie tattiche**, e
 *   hanno gia' un owner: `FRTOverlayArea` con `ERTOverlayMeaning` ed `ERTOverlayCertainty`
 *   (`Map/RTOverlayArea.h`, `Map/RTOverlayPalette.h`). Reimplementarle qui e' esattamente cio' che il
 *   mandato vieta quando scrive *«Graykit e Tactical HUD non devono implementare geometrie tattiche
 *   duplicate»*. Il modo di rispettare quella riga e' **non aggiungerle a questo enum**.
 * - `Trail`, `Ghost`, `ImpactBurst` non sono pose ma **emissioni**: producono elementi che sopravvivono al
 *   frame che li ha generati, quindi hanno uno stato che una funzione pura non ha. Restano fuori dal primo
 *   slice con una ragione, non per dimenticanza — e la loro forma corretta e' un consumatore di questa
 *   posa, non un valore qui dentro.
 *
 * Aggiungere valori solo IN CODA. Un valore sconosciuto a `Evaluate` contribuisce **zero** e non annulla
 * gli altri: e' il criterio di fallback che `FRTGraykitUnknownOperatorTest` misura.
 */
UENUM(BlueprintType)
enum class ERTGraykitOperator : uint8
{
	/** Sposta la parte lungo `Direction` di `Magnitude` centimetri. */
	Translate,
	/** Ruota la parte di `Magnitude` gradi attorno all'asse `Direction`. */
	Rotate,
	/**
	 * Inclina lungo `Direction` tenendo i piedi fermi.
	 *
	 * ⚠️ **Il perno e' sempre `Ground` e l'`Anchor` dichiarato viene IGNORATO**: un'inclinazione con perno
	 * alla testa non e' un'inclinazione, e' un rovesciamento. Chi vuole scegliere il perno usa `Rotate`.
	 */
	Lean,
	/** Schiaccia lungo Z e allarga in XY, a volume grossolanamente costante. */
	Squash,
	/** Allunga lungo Z e stringe in XY. L'inverso di `Squash`. */
	Stretch,
	/**
	 * Scala uniforme oscillante attorno a 1.
	 *
	 * 🔑 **E' un OSCILLATORE: prende la fase da `Cycles` e IGNORA `Easing`.** Pesare un oscillatore con un
	 * easing che a sua volta oscilla (`PingPong`) produrrebbe un battimento che nessun autore ha chiesto e
	 * che nessuno saprebbe leggere rileggendo il descriptor.
	 */
	Pulse,
	/** Oscillazione verticale del pivot. Il passo. Stesso patto di `Pulse`: `Easing` ignorato. */
	Bob
};

/**
 * Lo stato visivo di UNA parte: cio' che un consumatore applica a un componente.
 *
 * ⚠️ **Sono delta, non trasformazioni assolute.** `Offset` e' relativo alla posizione di riposo della parte,
 * `Rotation` alla sua rotazione di riposo, e `Scale` e' un MOLTIPLICATORE che vale `(1,1,1)` a riposo. Un
 * consumatore che li trattasse come assoluti piazzerebbe l'unita' nell'origine del mondo al primo frame in
 * cui nessun operatore e' attivo.
 */
USTRUCT(BlueprintType)
struct FRTGraykitPart
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	FVector Scale = FVector::OneVector;
};

/**
 * La posa completa del graykit a un istante: l'uscita di `Evaluate`.
 *
 * ⛔ **Non contiene una posizione di mondo, e l'assenza e' deliberata.** Dove l'unita' STA lo decide
 * `ARTUnit::SetVisualLocation` a partire da `URTPlaybackLibrary::InterpolateAlongPath`, che e' l'owner del
 * percorso; questa struct dice soltanto come il corpo si deforma **attorno** a quel punto. Se qui comparisse
 * una `FVector WorldLocation`, nascerebbe la seconda autorita' sul movimento visivo.
 */
USTRUCT(BlueprintType)
struct FRTGraykitPose
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	FRTGraykitPart Body;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	FRTGraykitPart LeftArm;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	FRTGraykitPart RightArm;

	/** Accesso per parte, cosi' che `Evaluate` non ripeta tre volte lo stesso `switch`. */
	FRTGraykitPart& PartFor(ERTGraykitPart Which)
	{
		switch (Which)
		{
		case ERTGraykitPart::LeftArm:  return LeftArm;
		case ERTGraykitPart::RightArm: return RightArm;
		default:                       return Body;
		}
	}

	const FRTGraykitPart& PartFor(ERTGraykitPart Which) const
	{
		return const_cast<FRTGraykitPose*>(this)->PartFor(Which);
	}
};

/**
 * Un operatore configurato: cosa fa, quando, quanto, su cosa.
 *
 * 🔑 **`StartTime` e `Duration` sono in tempo NORMALIZZATO `[0,1]`, non in secondi.** E' cio' che rende un
 * descriptor indipendente dalla velocita' di playback: `URTPlaybackLibrary::EffectivePlaybackSpeed` puo'
 * raddoppiare il ritmo e la posa resta la stessa alla stessa frazione dell'azione. Un descriptor in secondi
 * si sfaserebbe a ogni cambio di velocita', e un replay riprodotto a 2x mostrerebbe un'altra animazione.
 */
USTRUCT(BlueprintType)
struct FRTGraykitOperator
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	ERTGraykitOperator Op = ERTGraykitOperator::Translate;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	ERTGraykitPart Target = ERTGraykitPart::Body;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	ERTGraykitAnchor Anchor = ERTGraykitAnchor::Center;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	ERTGraykitEasing Easing = ERTGraykitEasing::Linear;

	/** Inizio della finestra, in tempo normalizzato dell'azione. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StartTime = 0.f;

	/**
	 * Ampiezza della finestra, in tempo normalizzato.
	 *
	 * ⚠️ **Durata zero significa operatore INERTE, non istantaneo.** Un impulso di durata nulla non e'
	 * rappresentabile a tempo campionato — a quale alpha lo si vedrebbe? — e trattarlo come «sempre attivo»
	 * lo renderebbe indistinguibile da `Duration = 1`. Chi vuole un colpo secco usa `PingPong` con una
	 * finestra stretta.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Duration = 1.f;

	/**
	 * Numero di cicli completi dentro la finestra. Vale per gli operatori oscillanti (`Bob`, `Pulse`) e per
	 * qualunque operatore con easing `PingPong`; gli altri lo ignorano.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit", meta = (ClampMin = "0.0"))
	float Cycles = 1.f;

	/**
	 * L'asse su cui l'operatore agisce, in spazio LOCALE all'unita'.
	 *
	 * ⚠️ Locale e non di mondo: un descriptor che dicesse «inclinati verso nord» sarebbe sbagliato per ogni
	 * unita' che non guarda a nord. La direzione di marcia la porta il chiamante orientando l'attore, che e'
	 * cio' che `bFaceMovementDirection` gia' fa.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	FVector Direction = FVector::ForwardVector;

	/** Centimetri per `Translate`/`Bob`, gradi per `Rotate`/`Lean`, frazione per `Squash`/`Stretch`/`Pulse`. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	float Magnitude = 0.f;

	FRTGraykitOperator() = default;
};

/**
 * Lo stile di locomozione che un'azione dichiara.
 *
 * ⛔ **Non e' un secondo vocabolario di azioni.** `ERTPresentationRole` (`Unit/RTPresentationRole.h`, #2441)
 * resta l'owner di *quale* azione si sta presentando; questo enum dice soltanto *con quale andatura* la si
 * cammina, ed esiste perche' il ruolo `Move` da solo non distingue una camminata da una corsa furtiva.
 *
 * 🔑 **`Run` non e' `Normal` piu' veloce**, ed e' il vincolo che il mandato pone esplicitamente: i due stili
 * differiscono per COMPOSIZIONE di operatori — inclinazione, ampiezza del braccio, allungamento — non per un
 * moltiplicatore di tempo. `FRTGraykitMoveRunDistinctTest` lo misura.
 */
UENUM(BlueprintType)
enum class ERTGraykitLocomotionStyle : uint8
{
	/** Camminata tattica. Il default. */
	Normal,
	/** Corsa. Inclinazione marcata, braccio ampio, allungamento direzionale. */
	Run,
	/** Furtiva. Baricentro basso, oscillazione compressa. */
	Stealth,
	/** Movimento ridotto: ferito, rallentato, ostacolato. Passo corto e asimmetrico. */
	Reduced
};

/**
 * Un descriptor: la lista di operatori che compone una posa.
 *
 * ⚠️ **L'ordine degli operatori NON conta per il risultato**, ed e' una garanzia deliberata: `Evaluate`
 * accumula offset e rotazioni per somma e le scale per prodotto, che sono operazioni commutative. Un
 * descriptor riordinato produce la stessa posa, e `FRTGraykitOrderIndependenceTest` lo misura. E' cio' che
 * permette di comporre due descriptor concatenandone le liste senza chiedersi chi viene prima.
 */
USTRUCT(BlueprintType)
struct FRTGraykitDescriptor
{
	GENERATED_BODY()

	/** Etichetta diagnostica. ⛔ Nessuna regola la legge: non e' una chiave, e' una stringa per chi guarda. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	FString Label;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RefactorTactics|Graykit")
	TArray<FRTGraykitOperator> Operators;
};
