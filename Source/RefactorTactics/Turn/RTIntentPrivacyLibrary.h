#pragma once

#include "CoreMinimal.h"
#include "Ability/RTActionDef.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTIntentPrivacyLibrary.generated.h"

/**
 * Quanto e' certo che un intento si realizzi come e' scritto (CP 11.2).
 *
 * ⚠️ **Non e' una probabilita' e non si deriva dai piani avversari.** Sarebbe il calcolo piu' naturale —
 * `FilterForTeam` riceve anche gli intenti nemici — ed e' esattamente cio' che l'invariante #6 vieta: un
 * tratteggio che comparisse solo quando un avversario incrocia la mia rotta gli direbbe dove si trova. Una
 * deduzione affidabile e' un'esposizione, e l'occultamento «non e' grafico»: il livello e' funzione del
 * SOLO intento osservato, e due scene identiche per l'osservatore danno lo stesso livello quali che siano
 * i piani altrui. `RefactorTactics.UI.NoEnemyIntentExposed` e' il test che lo pinna, con due scene che
 * differiscono SOLO per i piani nemici.
 */
UENUM(BlueprintType)
enum class ERTIntentCertainty : uint8
{
	/** Niente puo' cambiarlo: nessun movimento, nessun bersaglio. Una `Guard` sul posto. */
	Confirmed,

	/** Valido nello snapshot corrente: c'e' un bersaglio, ma l'unita' non si sposta. */
	Predicted,

	/**
	 * Dipende da una scelta che l'avversario puo' ancora fare. Muoversi basta: le celle del percorso sono
	 * contendibili, e il resolver puo' troncare la rotta a meta'. Vale **sempre** per una reazione armata,
	 * che per definizione attende un trigger che non decidiamo noi.
	 */
	Uncertain
};

/**
 * Piano COMPLETO di un'unita': il dato autorevole, che vive dove vive lo stato del turno (il server, quando
 * la rete arrivera' a M10). Non va mai spedito a un client cosi' com'e' — e' `FRTIntentView` cio' che si
 * spedisce, dopo il filtro per squadra.
 */
USTRUCT(BlueprintType)
struct FRTPlannedIntent
{
	GENERATED_BODY()

	/** Identita' stabile dell'unita': la sua cella (max 1 unita' per cella), mai un pointer. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	FRTCellId OwnerCell;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	int32 TeamId = 0;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	bool bAlive = true;

	/** `Status.Reveal`: rende l'intento visibile anche agli avversari. NON la reazione (vedi FilterForTeam). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	bool bRevealed = false;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	bool bMoving = false;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	FRTCellId PlannedCell;

	/** Nome leggibile dell'azione principale pianificata; vuoto = nessuna. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	FText ActionName;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	bool bHasTarget = false;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	FRTCellId TargetCell;

	/** Reazione tenuta pronta (CP 5.4); vuoto = nessuna. E' il campo che non deve MAI raggiungere un avversario. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	FText ReactionName;

	/** Rotta composita pianificata (parte del movimento, quindi esposta anche da `Status.Reveal`). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	TArray<FRTCellId> PlannedPath;

	/** Waypoint cliccati: dettaglio di AUTORIA del piano, mai esposto a un avversario. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	TArray<FRTCellId> PlannedWaypoints;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	bool bDashing = false;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	FRTCellId DashCell;

	/**
	 * COME si muove lo scatto pianificato. Serve alla PREVIEW: una mobilita' lineare va in linea retta, una a
	 * budget segue il grafo, e disegnare l'una per l'altra mostrerebbe al giocatore un percorso che il resolver
	 * non percorrera' (#142). Stessa classe d'informazione di `DashCell` — e' movimento, non autoria del piano.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	ERTMovementStyle DashStyle = ERTMovementStyle::None;

	/**
	 * Orientamento ATTUALE dell'unita' (CP 16.1). Pubblico: e' una posa osservabile sul campo, non un piano —
	 * chi guarda vede da che parte e' girata una figura, e nasconderlo non renderebbe il gioco piu' segreto,
	 * solo meno leggibile di cio' che si ha davanti agli occhi.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	ERTHexDirection Facing = ERTHexDirection::E;

	/** Vero se il piano dichiara una rotazione (D-020): senza questo, `DeclaredFacing` non e' interpretabile. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	bool bDeclaresRotation = false;

	/**
	 * Rotazione dichiarata in planning e non ancora applicata: dove l'unita' GUARDERA'. E' un intento, quindi
	 * appartiene alla stessa classe di `PlannedCell` — ma piu' vicina all'autoria del piano, perche' rivela
	 * l'arco che il giocatore ha scelto di coprire prima che si veda. Un avversario non la riceve.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Privacy")
	ERTHexDirection DeclaredFacing = ERTHexDirection::E;

	FRTPlannedIntent() = default;
};

/**
 * Cio' che UN osservatore puo' sapere del piano di un'unita': il DTO che si spedisce.
 *
 * Non e' `FRTPlannedIntent` con un flag "nascondi": e' una struttura DIVERSA, che i campi proibiti non li ha
 * proprio valorizzati. La differenza conta — un client non puo' rivelare cio' che non ha ricevuto, mentre un
 * campo spedito e nascosto a schermo si legge con qualunque strumento (invariante #6).
 */
USTRUCT(BlueprintType)
struct FRTIntentView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	FRTCellId OwnerCell;

	/** Vero se l'unita' e' della squadra dell'osservatore. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	bool bIsAlly = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	bool bMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	FRTCellId PlannedCell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	FText ActionName;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	bool bHasTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	FRTCellId TargetCell;

	/** Reazione pronta: valorizzata SOLO per gli alleati. Per un avversario resta vuota, sempre. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	FText ReactionName;

	/** Rotta pianificata: movimento, quindi esposta anche a un avversario rivelato. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	TArray<FRTCellId> PlannedPath;

	/** Waypoint: dettaglio di autoria del piano, SOLO per gli alleati. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	TArray<FRTCellId> PlannedWaypoints;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	bool bDashing = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	FRTCellId DashCell;

	/** Stile dello scatto: movimento, quindi esposto quanto `DashCell`. Vedi `FRTPlannedIntent::DashStyle`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	ERTMovementStyle DashStyle = ERTMovementStyle::None;

	/** Posa ATTUALE: pubblica, si vede guardando l'unita'. Valorizzata anche per un avversario rivelato. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	ERTHexDirection Facing = ERTHexDirection::E;

	/** Rotazione dichiarata: SOLO per gli alleati, come i waypoint e la reazione. Per un avversario resta falso. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	bool bDeclaresRotation = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	ERTHexDirection DeclaredFacing = ERTHexDirection::E;

	/**
	 * Quanto e' certo il PIANO (movimento e azione). Calcolato dal simulatore, mai dal widget.
	 *
	 * ⚠️ **Il default e' il livello piu' DEBOLE, e non e' un dettaglio di stile.** Era `Confirmed`, cioe' un
	 * campo mai popolato prometteva al giocatore la garanzia piu' forte del dominio: il modo di fallire
	 * affermava piu' di quanto qualunque percorso avesse calcolato. Oggi `FilterForTeam` assegna sempre
	 * (`ClassifyPlan`), quindi in partita non e' osservabile — lo diventa quando questa struttura viaggera'
	 * in rete (M10) e una deserializzazione parziale, un campo nuovo o un default di engine lasceranno il
	 * valore dov'era. Un dato che manca deve degradare in «non fidarti».
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	ERTIntentCertainty Certainty = ERTIntentCertainty::Uncertain;

	/**
	 * Quanto e' certa la REAZIONE armata. Campo separato perche' i due livelli DIVERGONO: un'unita' ferma che
	 * tiene pronto un contrattacco ha un piano `Confirmed` e una reazione `Uncertain`.
	 *
	 * 🔴 **NON usare questo campo per sapere se una reazione esiste: usare `ReactionName`.** Fino al
	 * 2026-08-16 il default era `Confirmed`, e quel valore significava due cose incompatibili — «non c'e'
	 * nessuna reazione» (il campo non veniva assegnato) e «la reazione e' certa». Un consumatore che leggesse
	 * il livello senza controllare prima `ReactionName` disegnava «reazione confermata» dove reazione non ce
	 * n'era: informazione falsa a schermo, che per un HUD e' il difetto peggiore. Lo pinna
	 * `RefactorTactics.UI.IntentCertaintyClassification`.
	 *
	 * ⚠️ **Oggi il livello e' `Uncertain` in entrambi i casi**, quindi non esiste piu' un valore che finga di
	 * distinguerli — e questo e' voluto: una reazione armata attende per definizione un trigger che decide
	 * l'avversario, quindi `Uncertain` e' l'unico livello che possa assumere. Il campo resta perche' il
	 * giorno in cui una reazione potra' essere `Predicted` (trigger gia' soddisfatto nello snapshot) il dato
	 * ha gia' il suo posto — ma finche' quel giorno non arriva **non aggiunge informazione alla resa**, e la
	 * grammatica «incerto» si abilita sulla presenza di `ReactionName`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Privacy")
	ERTIntentCertainty ReactionCertainty = ERTIntentCertainty::Uncertain;

	FRTIntentView() = default;
};

/**
 * Filtro di privacy degli intenti (invariante #6), esteso alle reazioni con CP 5.4.
 *
 * E' il punto in cui la privacy smette di essere "non disegnare" e diventa "non esiste per quell'osservatore":
 * la funzione COSTRUISCE la vista di chi guarda, invece di consegnare lo stato completo e chiedere alla UI di
 * comportarsi bene. Quando arrivera' la rete (M10), e' questa struttura a viaggiare — non `ARTUnit`.
 *
 * Funzione PURA: nessun Actor, nessun UWorld. Cosi' la regola di privacy si verifica senza montare una partita,
 * ed e' verificabile anche dove un test di UI non arriverebbe.
 */
UCLASS()
class REFACTORTACTICS_API URTIntentPrivacyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Le viste che l'osservatore `ObserverTeamId` ha diritto di ricevere, nell'ordine dell'input.
	 *
	 * - Unita' NON viva -> nessuna voce (non ha un piano da mostrare).
	 * - ALLEATO (stessa squadra) -> vista completa, reazione inclusa: il coordinamento di squadra e' un pilastro
	 *   di prodotto, e senza vedere la reazione dell'alleato non lo si puo' coprire.
	 * - AVVERSARIO non rivelato -> **nessuna voce**: non un campo vuoto, proprio nessuna riga. Un avversario non
	 *   deve nemmeno sapere che un piano esiste.
	 * - AVVERSARIO rivelato (`Status.Reveal`) -> vista dell'intento, ma **senza reazione**. `Reveal` mostra cosa
	 *   l'unita' sta per FARE, non cosa e' pronta a PARARE: la DoD di CP 5.4 dice che la reazione non e' mai
	 *   visibile ai nemici, e "mai" include il caso rivelato.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Privacy")
	static TArray<FRTIntentView> FilterForTeam(int32 ObserverTeamId, const TArray<FRTPlannedIntent>& Intents);

	/**
	 * Il livello di certezza del PIANO di un intento (CP 11.2). Il DoD chiede che la classificazione sia
	 * «calcolata dal simulatore, non dall'UI»: questa e' la funzione, e vive accanto al filtro di privacy
	 * perche' e' la stessa classe di decisione.
	 *
	 * ⚠️ Prende UN intento, non la lista, e la firma e' il punto: cosi' non PUO' guardare i piani avversari
	 * neanche per sbaglio. Il precedente e' `IsIntentVisibleTo` (#507) — una regola riscritta inline resta
	 * verde su una copia che nessuno chiama, e la divergenza non la vede nessuno.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Privacy")
	static ERTIntentCertainty ClassifyPlan(const FRTPlannedIntent& Intent);
};
