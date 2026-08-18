#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Map/RTCellId.h"
#include "RTHUD.generated.h"

/**
 * Una riga della terna di slot, gia' composta e pronta da disegnare.
 *
 * Tiene separato il TESTO dal fatto che lo slot sia occupato perche' il Canvas usa i due dati in modi
 * diversi — l'uno lo scrive, l'altro lo colora — e ricavare il secondo dal primo significherebbe cercare una
 * sottostringa, cioe' rendere il colore dipendente dalla lingua dell'etichetta.
 */
struct FRTSlotLine
{
	/** Composto per intero: «Movimento: Scatto», «Reazione: libero». */
	FString Text;

	bool bOccupied = false;
};

/**
 * Come disegnare un intento, dato quanto e' certo (CP 11.2). Tre parametri, uno per ciascun elemento che la
 * grammatica visiva del 2026-08-07 nomina: la linea, il ghost, il punto interrogativo.
 *
 * ⚠️ **Non contiene il livello, e non e' una dimenticanza.** Se portasse anche `ERTIntentCertainty` il Canvas
 * potrebbe fare `if (Style.Certainty == ...)` e la regola tornerebbe a vivere nel widget, che e' cio' che
 * l'invariante #1 di questa issue vieta — *«la classificazione arriva dal resolver: la UI non ricalcola il
 * perche'»*. Qui arriva il **cosa disegnare**, non il perche'.
 */
struct FRTIntentCertaintyStyle
{
	/**
	 * Moltiplicatore d'opacita' del ghost: `1` pienamente leggibile, valori bassi «attenuato» e «dissolto».
	 * Moltiplica l'alpha gia' scelto da chi disegna, invece di sostituirlo: la destinazione pianificata era
	 * gia' a `0.35` per conto suo, e un valore assoluto la riporterebbe a piena opacita' proprio nel caso
	 * incerto — cioe' l'opposto della grammatica.
	 */
	float GhostOpacity = 1.f;

	/**
	 * La linea che accompagna l'intento va tratteggiata invece che piena.
	 *
	 * ⚠️ **Quale linea dipende dal livello, ed e' una proprieta' di `ClassifyPlan`, non una scelta di qui.**
	 * `Confirmed` significa fermo E senza bersaglio: non ha ne' rotta ne' collegamento al bersaglio, quindi
	 * per lui «linea piena» non ha oggetto e resta una garanzia sul caso in cui una linea ci fosse.
	 * `Predicted` e' fermo con un bersaglio — la sua linea e' il collegamento. `Uncertain` si muove — la sua
	 * linea e' la rotta.
	 */
	bool bDashedLine = false;

	/** Da appendere all'etichetta: vuoto, oppure `?`. La grammatica lo da' al solo livello incerto. */
	FString Mark;

	/**
	 * Una reazione e' armata, e va resa con la grammatica «incerto» qualunque sia il livello del PIANO.
	 *
	 * 🔴 **Si abilita su `ReactionName`, mai su un livello della reazione**, e la ragione e' scritta sul DTO:
	 * `ReactionCertainty` e' uscito da `FRTIntentView` il 2026-08-16 perche' non ha mai potuto assumere un
	 * secondo valore, e finche' esisteva il suo default `Confirmed` significava insieme «nessuna reazione» e
	 * «reazione certa» — una UI che lo leggesse senza controllare prima il nome disegnerebbe una reazione
	 * confermata dove non ce n'e' nessuna. Il nome vuoto e' l'unico segnale che distingue i due casi.
	 */
	bool bReactionArmed = false;
};

/**
 * HUD disegnato in C++ (nessun asset UMG): barra HP/scudo sopra ogni unita' viva
 * e, a partita conclusa, il messaggio di esito al centro dello schermo.
 */
UCLASS()
class REFACTORTACTICS_API ARTHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/**
	 * Chi verrebbe colpito dai piani d'attacco **delle proprie unita'**: celle bersagliate e, fra queste,
	 * quelle occupate da un ALLEATO — il fuoco amico.
	 *
	 * Si calcola dai PIANI e non dall'anteprima. La differenza non e' interna: l'anteprima appartiene
	 * all'unita' **selezionata**, quindi l'avviso di fuoco amico spariva appena si selezionava qualcun altro
	 * — per esempio per muoverlo, cioe' mentre si finisce il turno. E' l'opposto di cio' che l'avviso serve a
	 * fare: «un alleato dentro l'area va visto PRIMA del lock-in». Segnalato in PIE il 2026-08-09
	 * (`PIE-PREVIEW-PERSIST`).
	 *
	 * Statica e senza accesso alla selezione **di proposito**: l'indipendenza dalla selezione diventa cosi'
	 * una proprieta' della firma, non una disciplina da ricordare.
	 *
	 * Legge solo i piani di `PlayerTeamId` (invariante #6, privacy dell'intento): i piani avversari non
	 * entrano, nemmeno per dedurne una cella.
	 */
	static void ComputePlannedHitMarks(const TArray<class ARTUnit*>& Units, int32 PlayerTeamId,
		TSet<FRTCellId>& OutHitCells, TSet<FRTCellId>& OutAllyHitCells);

	/**
	 * Vincola l'ancora di una sovrapposizione ai bordi del viewport.
	 *
	 * Serve perche' l'ancora nasce da un punto in WORLD space (`ActorLocation + WorldHeadOffset`) che viene
	 * proiettato: un offset fisso nel mondo produce uno spostamento **variabile** sullo schermo, tanto piu'
	 * grande quanto l'unita' e' vicina alla camera. Senza vincolo la sovrapposizione di un'unita' vicina
	 * finisce sopra il bordo e sparisce — nome **e** barre insieme, perche' condividono questa ancora.
	 * Osservato in PIE il 2026-08-13 (`PIE-NAME`), diagnosticato in #729.
	 *
	 * ⚠️ **Sceglie di ancorare al margine, non di nascondere.** Il motivo per cui l'etichetta esiste e' che
	 * quattro cilindri identici rendono impossibile dire chi sta facendo cosa: un'etichetta appiccicata al
	 * bordo si legge ancora e conserva il colore di squadra, una assente no. La via alternativa — offset
	 * costante in *screen* space invece che nel mondo — e' stata scartata perche' smetterebbe di seguire
	 * l'altezza dell'unita' e finirebbe **sopra la mesh** da vicino.
	 *
	 * @param Anchor      punto desiderato: X centro del blocco, Y riga della barra HP.
	 * @param HalfWidth   meta' larghezza del blocco piu' largo (barra o nome).
	 * @param AboveAnchor quanto il blocco sale sopra `Anchor.Y` (nome e status).
	 * @param BelowAnchor quanto scende sotto `Anchor.Y` (barra ed energia).
	 * @param Viewport    dimensioni del canvas.
	 * @param Margin      distanza minima dal bordo.
	 *
	 * Se il blocco e' piu' grande del viewport il vincolo e' insoddisfacibile: si preferisce il bordo
	 * **superiore/sinistro**, perche' il nome sta in alto ed e' la parte che identifica l'unita'.
	 */
	static FVector2D ClampOverlayAnchor(const FVector2D& Anchor, float HalfWidth,
		float AboveAnchor, float BelowAnchor, const FVector2D& Viewport, float Margin);

	/**
	 * Le tre righe della terna movimento / principale / reazione, nell'ordine in cui vanno disegnate.
	 *
	 * ⚠️ **Restituisce sempre tre righe, anche quando il piano e' vuoto**, ed e' il punto: la riga d'intento
	 * che il Canvas gia' disegna salta le unita' senza ordini (`if (bOwn && !bHasPlan) continue`), quindi uno
	 * slot LIBERO non si vedeva mai. Il DoD di CP 11.1 chiede «con indicazione di cosa e' gia' stato scelto»,
	 * che senza il complemento — cosa NON e' ancora stato scelto — non si puo' leggere.
	 *
	 * ⚠️ **Non arbitra, come la vista da cui legge.** Se `Action.Sprint` occupa movimento E principale, le
	 * due righe portano lo stesso nome invece di sceglierne una: chi decide la legalita' e'
	 * `URTCatalogLibrary::ValidateActionSlots`, e una terna «pulita» nasconderebbe il piano che il validatore
	 * rifiutera'.
	 *
	 * Statica e pura per la stessa ragione di `ComputePlannedHitMarks`: non tocca la selezione, quindi non
	 * puo' dipenderne.
	 */
	static TArray<FRTSlotLine> ComposeSlotLines(const struct FRTUnitSlotsView& Slots);

	/**
	 * Come rendere un intento, dato il livello di certezza che la vista **porta gia' calcolato** (CP 11.2).
	 *
	 * ⚠️ **Legge `View.Certainty` e non lo ricalcola**, ed e' il primo invariante della issue: `ClassifyPlan`
	 * vive in `URTIntentPrivacyLibrary` perche' e' il simulatore a decidere, e una seconda copia della regola
	 * dentro la HUD divergerebbe senza che nessun test se ne accorga — il precedente e' `IsIntentVisibleTo`
	 * (#507). Qui non c'e' nessun `bMoving`, nessun `bHasTarget`: se comparissero, sarebbe la regola riscritta.
	 *
	 * ⚠️ **`Unknown` non ha una resa propria e riceve la piu' PRUDENTE**, cioe' quella del livello incerto.
	 * L'enum lo dichiara — *«un `Unknown` che arriva alla presentazione e' un difetto a monte»* — e il difetto
	 * non si ripara qui; si evita solo di **premiarlo**. Un campo mai calcolato che ereditasse la resa piena
	 * affermerebbe a schermo la garanzia piu' forte del dominio, che e' esattamente il difetto per cui
	 * `Unknown` vale zero. Distinguerlo *visivamente* da `Uncertain` chiederebbe una quarta grammatica per uno
	 * stato che non deve arrivare a schermo, e il catalogo icone ne pretende **tre** apposta.
	 *
	 * Statica e pura come `ComposeSlotLines`: cosi' i tre livelli si verificano senza montare una partita, che
	 * e' cio' che il DoD chiede quando dice «un test headless su `ARTHUD`».
	 */
	static FRTIntentCertaintyStyle ComposeIntentCertaintyStyle(const struct FRTIntentView& View);

	/**
	 * La velocita' successiva sulla scala `x1 · x2 · x4` (CP 47.7, #1015).
	 *
	 * ⚠️ **Totale, e non per prudenza: il valore di partenza non e' sotto il controllo dell'HUD.**
	 * `ViewerPlaybackSpeed` e' `EditAnywhere` e `BlueprintReadWrite` — nei dettagli dell'attore, o in un
	 * Blueprint scritto prima di questo checkpoint, ci puo' essere `3` o `0`. La regola e' una sola e
	 * copre insieme il ciclo e le guardie: **la piu' piccola velocita' legale strettamente maggiore di
	 * quella corrente, e se non esiste si torna a `x1`**. Un «raddoppia e a 8 riparti» darebbe `6` da un
	 * `3`, cioe' una velocita' che #955 non ha deciso.
	 *
	 * La scala vive QUI e non nel modello perche' e' vocabolario di presentazione: il simulatore accetta
	 * qualunque float positivo, ed e' il controllo a scegliere quali tre offrire.
	 */
	static float NextViewerPlaybackSpeed(float Current);

	/**
	 * L'etichetta del controllo: dice la velocita' SCELTA e, quando il tetto la supera, anche quella reale.
	 *
	 * ⚠️ **Due numeri quando divergono, uno quando coincidono**, e la divergenza non e' un caso limite: e'
	 * la composizione `Max(Viewer, Cap)` decisa in #955. Mostrare la sola scelta produrrebbe un'etichetta
	 * che contraddice il ritmo — il difetto che il controllo esiste per togliere, aggravato dal fatto che
	 * un numero c'e' e mente.
	 *
	 * ⚠️ **Non ricompone: interroga.** La velocita' reale viene da `URTPlaybackLibrary::EffectivePlaybackSpeed`,
	 * la stessa funzione che `TickPlayback` usa per scorrere. Rifare il conto qui dentro creerebbe la
	 * seconda verita' che si scollega al primo cambio di regola.
	 *
	 * @param ViewerSpeed la scelta di chi guarda. Non positiva = «non scelta», letta come `x1`.
	 * @param CapSpeed    il fattore di accelerazione automatica in vigore (`GetPlaybackCapSpeed`).
	 */
	static FString ComposePlaybackSpeedLabel(float ViewerSpeed, float CapSpeed);

protected:
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HUD")
	float BarWidth = 64.f;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HUD")
	float BarHeight = 8.f;

	/** Altezza (uu) sopra l'origine dell'unita' a cui ancorare la barra. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HUD")
	float WorldHeadOffset = 200.f;
};
