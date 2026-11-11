#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTVeilTransition.generated.h"

/**
 * Le due costanti di tempo del velo, e la soglia oltre la quale una transizione e' finita (`#2874`).
 *
 * 🔑 **Reveal e hide sono due numeri e non uno, e la differenza non e' estetica.** Aprire il velo e'
 * informazione che ARRIVA: deve essere pronta quando il giocatore la cerca, quindi rapida. Chiuderlo e'
 * informazione che SCADE — la cella non e' piu' osservata, ma lo era un istante fa — e una chiusura
 * altrettanto rapida si legge come uno sfarfallio ogni volta che un'unita' oscilla sul bordo del cono.
 * Un solo numero costringerebbe a scegliere quale dei due casi rendere sbagliato.
 *
 * ⚠️ **Sono SECONDI, non «frame».** Un filtro tarato in frame cambia comportamento col frame rate, che e'
 * precisamente cio' che `URTVeilTransitionLibrary::Advance` esiste per non fare.
 */
USTRUCT(BlueprintType)
struct FRTVeilTransitionParams
{
	GENERATED_BODY()

	/**
	 * Costante di tempo dell'apertura: **120 ms**, dentro la banda 80-150 dichiarata dal prototipo.
	 *
	 * ⚠️ E' un `Tau`, non una durata: dopo `Tau` la transizione ha coperto il `63%` della distanza, non il
	 * `100%`. Il `100%` lo decide `SnapEpsilon`, ed e' l'unica ragione per cui il valore ci arriva davvero.
	 */
	static constexpr float DefaultRevealSeconds = 0.12f;

	/** Costante di tempo della chiusura: **300 ms**, dentro la banda 250-400. Piu' lenta dell'apertura. */
	static constexpr float DefaultHideSeconds = 0.30f;

	/**
	 * Sotto questa distanza la transizione **finisce**, e il valore diventa il target ESATTAMENTE.
	 *
	 * 🔴 **Non e' una tolleranza di comodo: e' l'unico modo per cui un esponenziale arriva.** `Lerp` con
	 * `Alpha = 1 - exp(-Dt/Tau)` si avvicina al target e non lo tocca mai — resterebbe una cella
	 * perpetuamente «in transizione», che costa una riscrittura per frame per sempre e che nessun test di
	 * uguaglianza potrebbe verificare.
	 *
	 * ⚠️ **Il valore e' `1/512`, e il denominatore ha una ragione.** Il consumatore naturale di questo
	 * filtro scrive un canale colore, e un canale a 8 bit distingue `1/255`: mezzo passo di quantizzazione e'
	 * la soglia sotto la quale «arrivato» e «quasi arrivato» sono lo stesso pixel. Piu' stretto sarebbe
	 * lavoro che nessuno vede; piu' largo sarebbe un salto visibile all'ultimo tratto.
	 */
	static constexpr float DefaultSnapEpsilon = 1.f / 512.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	float RevealSeconds = DefaultRevealSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	float HideSeconds = DefaultHideSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	float SnapEpsilon = DefaultSnapEpsilon;

	FRTVeilTransitionParams() = default;

	/**
	 * I parametri che **spengono** il filtro: ogni passo arriva al target, cioe' il comportamento che il velo
	 * ha avuto fino a `#2874`.
	 *
	 * 🔑 **Esiste per essere il ramo di confronto, non per essere una preferenza.** Il consumatore
	 * (`#2875`) deve poter dimostrare che con il filtro spento la board e' identica a quella di prima: senza
	 * un modo NOMINATO di spegnerlo, quella dimostrazione richiederebbe di rimuovere il codice, e un ramo che
	 * si prova solo cancellandolo non si prova in una suite.
	 */
	static FRTVeilTransitionParams Instant()
	{
		FRTVeilTransitionParams P;
		P.RevealSeconds = 0.f;
		P.HideSeconds = 0.f;
		return P;
	}
};

/**
 * Il filtro temporale della presentazione del velo: `DisplayVisibility` insegue un target nel tempo.
 *
 * ## Cosa NON e'
 *
 * ⛔ **Non e' conoscenza, e non ne conosce nemmeno la forma.** Non include `FRTTeamKnowledge`, non sa cosa
 * sia una cella, un `FRTCellId`, una squadra o una mappa: prende dei `float` e ne restituisce altri. E' la
 * condizione perche' non possa diventare una seconda autorita' sulla visibilita' — il difetto piu' facile
 * da introdurre qui e' un filtro che, per comodita', ricalcoli qualcosa.
 *
 * ⛔ **Non decide niente di competitivo.** Line of sight, targeting, concealment, line of fire, bot e
 * `TurnLog` leggono `FRTTeamKnowledge` e non passano di qui. Un valore prodotto da questo file non entra nel
 * `TurnLog`, nello `StateHash` ne' nello snapshot.
 *
 * ## L'indipendenza dal frame rate e' un'IDENTITA', non un'approssimazione
 *
 * 🔑 La forma e' `Alpha = 1 - exp(-Dt/Tau)`, poi `Lerp(Current, Target, Alpha)`. Sviluppata:
 *
 *     Next = Target + (Current - Target) * exp(-Dt/Tau)
 *
 * ∴ due passi consecutivi a target costante moltiplicano i due esponenziali, e
 * `exp(-a/T) * exp(-b/T) = exp(-(a+b)/T)`: **un passo da `Dt` e `N` passi da `Dt/N` danno lo stesso
 * numero**, a meno dell'arrotondamento in virgola mobile. Non e' un caso fortunato dell'esponenziale: e' la
 * ragione per cui si sceglie l'esponenziale invece di un `Lerp` a coefficiente fisso, che con lo stesso
 * codice darebbe una velocita' diversa a 30 e a 120 fps.
 *
 * ⚠️ L'unica eccezione dichiarata e' lo **snap**: l'ultimo tratto si chiude di scatto, quindi due
 * discretizzazioni diverse possono chiuderlo a istanti diversi. Lo scarto e' per costruzione minore di
 * `SnapEpsilon`, che e' mezzo passo di un canale a 8 bit — sotto la soglia di cio' che si puo' vedere.
 *
 * ## Riprendere non e' ricominciare
 *
 * 🔴 **Non c'e' nessuno stato di «transizione in corso»: c'e' solo il valore corrente.** Un target nuovo a
 * meta' strada non fa ripartire niente, perche' non c'e' niente che possa ripartire — la condizione
 * iniziale del passo successivo e' dove il valore si trova adesso. E' cio' che rende corretti, senza un
 * ramo dedicato, il movimento interrotto, quello concatenato e la sequenza rapida di celle: casi che un
 * filtro con durata e progresso avrebbe dovuto trattare uno per uno.
 */
UCLASS()
class REFACTORTACTICS_API URTVeilTransitionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Un passo del filtro: dove sta `Current` dopo `DeltaSeconds` passati a inseguire `Target`.
	 *
	 * Il VERSO sceglie la costante di tempo: si sale con `RevealSeconds`, si scende con `HideSeconds`.
	 *
	 * ⚠️ **`DeltaSeconds <= 0` restituisce `Current` identico, e non e' una guardia difensiva: e' la
	 * PAUSA.** Un playback in pausa pompa il proprio passo a zero, e il velo deve fermarsi a meta'
	 * dissolvenza invece di saltare al target — che e' precisamente cio' che farebbe una guardia scritta
	 * come «se il passo e' nullo, concludi».
	 *
	 * ⚠️ **Costante di tempo `<= 0` restituisce `Target`**: e' il filtro spento, `FRTVeilTransitionParams::Instant`,
	 * cioe' il comportamento del velo prima di `#2874`. Non e' un errore da segnalare, e' un ramo dichiarato.
	 *
	 * ⚠️ **Non clampa `Current` ne' `Target` in `[0, 1]`**, di proposito. Il filtro non sa cosa siano quei
	 * numeri; se qualcuno gli passasse un livello fuori intervallo, restituire un valore corretto per un
	 * ingresso sbagliato nasconderebbe il difetto del chiamante. L'intervallo lo garantisce chi produce il
	 * target — e per costruzione un `Lerp` fra due valori in `[0, 1]` non ne esce.
	 *
	 * ⚠️ **Un `DeltaSeconds` NaN cade nel ramo della pausa** e restituisce `Current`: il confronto e'
	 * scritto come `!(DeltaSeconds > 0.f)` e non come `DeltaSeconds <= 0.f` proprio per questo. Un NaN
	 * propagato nel valore disegnato spegnerebbe una cella senza che nessuno sappia perche'.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	static float Advance(float Current, float Target, float DeltaSeconds,
		const FRTVeilTransitionParams& Params);

	/**
	 * Lo stesso passo su un'intera board, e **quante celle sono ancora in movimento** dopo averlo fatto.
	 *
	 * 🔑 **Il conteggio e' la meta' del valore di questa funzione.** Senza, un consumatore non ha modo di
	 * sapere quando smettere: continuerebbe a riscrivere ogni istanza per sempre, e la differenza fra «il
	 * velo sta lavorando» e «il velo ha finito e continua a pagarne il costo» sarebbe invisibile — lo stesso
	 * difetto che `ARTHexMapActor::GetLastVeilTouchedCells` esiste per rendere misurabile sull'altra meta'
	 * del problema. **Zero significa converso**, ed e' il segnale con cui un `Tick` puo' spegnersi.
	 *
	 * ⚠️ **Fail-closed sul disallineamento: `INDEX_NONE`, e `Display` non viene toccato.** Non `0`, che
	 * significherebbe «tutto converso» e farebbe smettere di aggiornare proprio chi ha un ingresso rotto.
	 * E' la stessa disciplina di `URTTeamKnowledgeLibrary::ObservedPrefixLength`, che su verdetti e celle
	 * disallineati risponde «niente» invece di leggere fuori — qui pero' il «niente» ha bisogno di un valore
	 * PROPRIO, perche' lo zero e' gia' preso da un esito legittimo.
	 *
	 * @param Display  i valori disegnati, aggiornati sul posto.
	 * @param Targets  i target, uno per elemento di `Display`.
	 * @return quante celle non hanno ancora raggiunto il target; `INDEX_NONE` se le due lunghezze differiscono.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Perception")
	static int32 AdvanceAll(UPARAM(ref) TArray<float>& Display, const TArray<float>& Targets,
		float DeltaSeconds, const FRTVeilTransitionParams& Params);
};
