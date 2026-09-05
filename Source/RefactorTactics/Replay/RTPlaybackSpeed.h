// Copyright RefactorTactics. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTPlaybackSpeed.generated.h"

/**
 * LE SEI VELOCITA' DI RIPRODUZIONE del playback d'autore — `#1625`.
 *
 * 🔑 **È presentazione, e non tocca il risultato logico.** Cambia quanto tempo passa fra due posizioni
 * della traccia, non quali posizioni esistano: `Instant` e `Normal` arrivano allo stesso stato finale, ed
 * è il criterio d'accettazione che questa scala esiste per rendere verificabile.
 *
 * ⚠️ **`Instant` non è «velocità infinita»**, e non si esprime con un numero: un moltiplicatore enorme
 * resterebbe una moltiplicazione, con un tempo di attesa piccolo ma non nullo e un limite che dipende dal
 * frame. È un caso a sé — *«non aspettare»* — e chi lo consuma lo chiede **per nome**, non confrontando
 * un `float` con una soglia.
 *
 * ⛔ **Il ViewModel non ha un orologio, e non deve averne uno.** Questa scala è un tipo puro: chi possiede
 * il tempo — la UI — moltiplica. Mettere un `Tick` in `FRTReplayViewModel` farebbe entrare la
 * presentazione nella logica, che è l'opposto di ciò che `ADR-0010` prescrive.
 */
UENUM(BlueprintType)
enum class ERTPlaybackSpeed : uint8
{
	Quarter		UMETA(DisplayName = "0.25x"),
	Half		UMETA(DisplayName = "0.5x"),
	Normal		UMETA(DisplayName = "1x"),
	Double		UMETA(DisplayName = "2x"),
	Quadruple	UMETA(DisplayName = "4x"),

	/** Nessuna attesa fra i passi. Si riconosce per NOME: non ha un moltiplicatore. */
	Instant		UMETA(DisplayName = "Instant")
};

UCLASS()
class REFACTORTACTICS_API URTPlaybackSpeedLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Il moltiplicatore di TEMPO di una velocità: quanto dura un passo rispetto a `1x`.
	 *
	 * ⚠️ Per `Instant` non c'è un moltiplicatore sensato e questa funzione risponde **`0`**, che significa
	 * *«nessuna attesa»* e non *«tempo fermo»*. Chi consuma non deve dividere per questo valore: deve
	 * chiedere `IsInstant` prima, ed è la ragione per cui quella funzione esiste separata.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static float SecondsMultiplier(ERTPlaybackSpeed Speed);

	/**
	 * Vero se la velocità significa «non aspettare».
	 *
	 * 🔑 **Esiste perché il confronto giusto è sul nome, non sul numero.** Un consumatore che scrivesse
	 * `if (Multiplier <= KINDA_SMALL_NUMBER)` legherebbe il proprio comportamento a un valore che questa
	 * libreria può cambiare, e il giorno che `Instant` diventasse `0.001` per un motivo di rendering
	 * quella riga smetterebbe di riconoscerlo **senza un errore**.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static bool IsInstant(ERTPlaybackSpeed Speed);

	/** Le sei velocità in ordine di scala, dalla più lenta a `Instant`. L'ordine è quello dei controlli. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static TArray<ERTPlaybackSpeed> AllSpeeds();

	/**
	 * La velocità successiva, ciclando. È ciò che un singolo tasto di trasporto consuma.
	 *
	 * ⚠️ Cicla — da `Instant` si torna a `Quarter` — perché un controllo a un tasto che si fermasse in
	 * fondo lascerebbe l'autore senza modo di tornare indietro senza un secondo comando.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Playback")
	static ERTPlaybackSpeed NextSpeed(ERTPlaybackSpeed Speed);
};
