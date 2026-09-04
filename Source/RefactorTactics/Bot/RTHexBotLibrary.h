#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionData.h"
#include "Map/RTCellId.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTReactionOpportunityTypes.h" // FRTReactionOpportunity: l'UNICO ingresso della decisione del bot
#include "RTHexBotLibrary.generated.h"

class URTHexMapAsset;

/** Mossa candidata del bot su griglia esagonale: dove finisce e, se serve, chi colpisce da lì. */
USTRUCT()
struct FRTHexBotPlan
{
	GENERATED_BODY()

	/** Cella in cui il bot finisce (o resta). */
	UPROPERTY() FRTCellId DestCell;

	/** Vero se il piano include un attacco sferrato da DestCell. */
	UPROPERTY() bool bHasAttack = false;

	/** Indice del bersaglio in FRTHexBotContext::Enemies (INDEX_NONE = nessuno). Puro: nessun Actor. */
	UPROPERTY() int32 TargetIndex = INDEX_NONE;

	/** Danno dell'attacco pianificato. */
	UPROPERTY() int32 AttackDamage = 0;

	/** HP+scudo del bersaglio (per riconoscere il colpo letale). */
	UPROPERTY() int32 TargetHealth = 0;

	/**
	 * Forma dell'attacco pianificato, copiata dal contesto da BuildCandidates come gia' avviene per
	 * AttackDamage. Sta QUI e non nel contesto perche' `ChooseBestPlan` confronta in una sola lista le
	 * candidate di abilita' DIVERSE: un solo contesto descriverebbe la forma di una sola di esse.
	 */
	UPROPERTY() ERTAbilityShape Shape = ERTAbilityShape::Single;

	/** Raggio dell'area, letto solo quando Shape e' Area. */
	UPROPERTY() int32 AreaRadius = 0;

	/** Gittata dichiarata dall'azione: serve alla profondita' del Cone. */
	UPROPERTY() int32 RangeCells = 0;

	/** Vero se l'attacco colpisce anche gli alleati (`FRTActionDef::bFriendlyFire`). */
	UPROPERTY() bool bFriendlyFire = false;

	/**
	 * Cella da cui il bot ARRIVA su `DestCell` — il predecessore che `ReachableCells` ora conserva.
	 *
	 * Serve al facing (CP 13.5, ADR-0005): l'orientamento deriva dall'ultimo passo, e senza questo campo
	 * valutare l'esposizione di una candidata richiederebbe un pathfinding per candidata dentro il ciclo di
	 * scoring. Per chi resta fermo vale `DestCell`, e il facing non cambia.
	 */
	UPROPERTY() FRTCellId FromCell;
};

/**
 * Contesto puro per valutare una candidata su hex. Stessi pesi e stessa politica del bot quadrato
 * (FRTBotContext): cambiano solo la distanza (esagonale) e la copertura, che qui viene dai dati della mappa
 * invece che da una lista di ostacoli passata dal chiamante.
 */
USTRUCT()
struct FRTHexBotContext
{
	GENERATED_BODY()

	/** Posizione attuale del bot: tie-break di ChooseBestPlan (a parita' di punteggio, mossa minima da qui). */
	UPROPERTY() FRTCellId Origin;

	/** Posizioni dei nemici vivi. */
	UPROPERTY() TArray<FRTCellId> Enemies;

	/** Gittata di ciascun nemico (parallelo a Enemies): serve a stimare la minaccia sulla cella. */
	UPROPERTY() TArray<int32> EnemyRanges;

	/** HP+scudo di ciascun nemico (parallelo a Enemies): serve a riconoscere il colpo letale. */
	UPROPERTY() TArray<int32> EnemyHealth;

	/**
	 * Orientamento di ciascun nemico (parallelo a Enemies), come e' ALL'INIZIO del turno.
	 *
	 * ⚠️ **Non e' una fuga d'informazione**: il facing corrente e' osservabile — `ARTUnit::Facing` e' cio' che
	 * le regole leggono e che la mesh mostra a fine playback, quindi il giocatore umano vede lo stesso dato.
	 * A restare privato e' l'INTENTO di rotazione, che [ADR-0005] filtra per squadra
	 * (`Facing.IntentIsTeamFiltered`) e che questo campo non contiene.
	 *
	 * ⚠️ **E' una previsione destinata a sbagliare, a volte**: chi attacca ruota verso il proprio bersaglio
	 * PRIMA che si valuti l'arco (`RTTurnManager.cpp`, `TargetingReoriented` precede il controllo direzionale).
	 * Un nemico che attacca il bot gli si gira contro, e il fianco che il bot aveva visto non c'e' piu'. Il
	 * bonus sopravvive quando il bersaglio e' impegnato con qualcun ALTRO — il fuoco incrociato, non il duello.
	 * La sovrastima che ne deriva si misura sul TurnLog (`RearHitBypassedCover`), non si stima a priori.
	 */
	UPROPERTY() TArray<ERTHexDirection> EnemyFacings;

	/** Orientamento attuale del bot: da qui parte la stima di come sara' orientato a fine turno. */
	UPROPERTY() ERTHexDirection SelfFacing = ERTHexDirection::E;

	/**
	 * Posizioni degli alleati vivi, ESCLUSO il bot che sta pianificando: `CollectHexAttacks` salta sempre
	 * l'attaccante (`u == Intent.AttackerId`), quindi contarsi qui renderebbe il bot timido su un danno che
	 * non subirebbe mai.
	 */
	UPROPERTY() TArray<FRTCellId> Allies;

	/** HP+scudo di ciascun alleato (parallelo a Allies): serve a riconoscere il collaterale letale. */
	UPROPERTY() TArray<int32> AllyHealth;

	/**
	 * Le celle OBIETTIVO della mappa (`FRTHexCellData::bIsObjective`, formato mappa v11, `#75`).
	 *
	 * 🔴 **E' l'ingresso che mancava, e senza di lui il bot non conosceva la condizione di vittoria del
	 * formato che la v0.1 spedisce** (`#2269`). Misurato il 2026-09-04 su `L_HexArena`: una partita 2v2
	 * bot contro bot finita `obiettivo 0-3`, con i tre punti presi da un'unita' che era arrivata sulla cella
	 * come miglior candidata di SOLO MOVIMENTO, a punteggio negativo — cioe' per avvicinamento e quota, per
	 * ragioni che con l'obiettivo non c'entrano.
	 *
	 * ⚠️ **E' geometria PUBBLICA, e per questo non passa dalla Team Knowledge.** La mappa la vedono
	 * entrambe le squadre: dov'e' l'obiettivo non e' informazione nascosta piu' di quanto lo sia dov'e' un
	 * muro, che `ScorePlan` gia' legge dall'asset. Il filtro di percezione (CP 13.5) protegge le UNITA'
	 * avversarie, non il terreno — e allargarlo al terreno renderebbe il bot cieco a cio' che il giocatore
	 * umano vede sullo schermo dal primo fotogramma.
	 *
	 * ⚠️ **Un array e non una cella sola**, benche' oggi il perimetro sia **un** obiettivo contendibile su
	 * **una** cella: `URTHexMapAsset::FirstObjectiveCell()` esiste e sarebbe bastata, ma CP 31.1 (`#1583`)
	 * porta piu' obiettivi simultanei, e la forma plurale costa qui zero righe mentre la' ne costerebbe una
	 * firma da cambiare.
	 *
	 * ⛔ **Sta nel contesto e non si rilegge dalla mappa dentro `ScorePlan`, ed e' una scelta di scala.**
	 * `FirstObjectiveCell()` scandisce TUTTE le celle dell'asset, e `ScorePlan` gira una volta per
	 * candidata: su una mappa d'autore sarebbero decine di migliaia di scansioni per unita' per turno. Qui
	 * si paga una volta, e la distanza riusa la cache per goal di `StepsToGoalField` (`#1436`).
	 */
	UPROPERTY() TArray<FRTCellId> ObjectiveCells;

	/** Gittata dell'attacco del bot. */
	UPROPERTY() int32 AttackRange = 0;

	/** Danno dell'attacco del bot. */
	UPROPERTY() int32 AttackDamage = 0;

	/**
	 * Forma dell'attacco valutato: decide QUANTE celle prende, quindi quanti nemici in piu' e quanti alleati.
	 * Con `Single` il calcolo resta quello di prima (una cella, un bersaglio).
	 */
	UPROPERTY() ERTAbilityShape AttackShape = ERTAbilityShape::Single;

	/** Raggio dell'area, letto solo quando AttackShape e' Area. */
	UPROPERTY() int32 AttackAreaRadius = 0;

	/**
	 * Vero se l'attacco valutato colpisce anche gli alleati (`FRTActionDef::bFriendlyFire`). Il bot deve
	 * modellare il resolver, non un'idea del resolver: senza fuoco amico l'alleato nell'area non subisce nulla.
	 */
	UPROPERTY() bool bAttackFriendlyFire = false;

	/** >0 = kiter (mantiene la distanza di sicurezza); 0 = mischia (chiude la distanza). */
	UPROPERTY() int32 KiteStandoff = 0;

	// Pesi interi (bilanciabili senza toccare la logica; invariante #4: niente float). Default: il kill domina.
	UPROPERTY() int32 WKill = 10000;
	UPROPERTY() int32 WDamage = 10;
	/**
	 * Peso del danno inflitto a un ALLEATO dal collaterale di un'area. Pari a WDamage per default: un punto
	 * di danno al compagno annulla esattamente un punto di danno al nemico, quindi prendere due nemici e un
	 * alleato resta conveniente e prenderne uno solo non lo e'. E' un peso, non un veto: si tara senza
	 * toccare la logica.
	 */
	UPROPERTY() int32 WAllyDamage = 10;
	UPROPERTY() int32 WThreat = 100;
	UPROPERTY() int32 WKiteViolation = 50;
	UPROPERTY() int32 WApproach = 10;
	/**
	 * Bonus per la quota (`Layer`) della cella di destinazione (#1088).
	 *
	 * ⚠️ **INVARIANTE: `WElevation * MaxLayer < WApproach`.** Sopra quella soglia scendere per avvicinarsi
	 * non conviene, i punteggi pareggiano e il tie-break «restare vince» riapre il parcheggio. Pinnato da
	 * `HexBot.ElevationNeverOutweighsClosingOneCell`, che lo misura sull'ESITO di `ChooseBestPlan` — non sul
	 * punteggio di un piano isolato, che non vede il tie-break.
	 */
	UPROPERTY() int32 WElevation = 4;

	/**
	 * Bonus per una cella da cui si VEDE un contatto noto, quando il piano non contiene un attacco
	 * (#1300, D-185). E' il termine che risponde a «da qui posso ingaggiare», e che la spec elencava fra i
	 * termini del punteggio **senza nominare la linea di tiro**.
	 *
	 * ⚠️ **Guarda DOVE VAI, non da dove parti**, ed e' cio' che lo distingue dal filtro sul dominio di
	 * #1287: quel filtro si accendeva quando eri cieco e si spegneva appena vedevi, cioe' era un ciclo di
	 * periodo due. Qui la condizione sta sulla destinazione, e uscire dalla ricerca non riporta indietro.
	 *
	 * ⚠️ **Vale solo sui piani SENZA attacco.** Un piano che spara vale gia' `WDamage * danno`, cioe' due
	 * ordini di grandezza in piu': aggiungerci un bonus di posizione non cambierebbe nulla e renderebbe il
	 * termine illeggibile.
	 */
	UPROPERTY() int32 WEngage = 15;

	/**
	 * Quanto `WEngage` CALA per ogni turno consecutivo in cui l'unita' non ingaggia (`IdleTurns`).
	 *
	 * 🔴 **Non e' una rifinitura: senza decadimento il termine non funziona a NESSUN peso.** Un bonus
	 * posizionale sulla linea di tiro paga per *guardare*, e la cella che massimizza il guardare e' una
	 * vedetta in quota da cui non si spara — lo stato assorbente di #1088 con un nome nuovo. Misurato
	 * intero per intero il 2026-08-24: `Match.Autobattle.EngagesOnTheGeneratedTestArena` cade **da `W = 7`**
	 * (dove `WElevation * MaxLayer + W < WApproach` lo prevede) e `NobodyParksOnTheAuthoredMap` si sblocca
	 * solo **da `W = 11`**. La finestra e' **vuota**, e fra 7 e 10 sono rossi entrambi.
	 *
	 * Con il decadimento la vedetta vale molto appena ci arrivi e sempre meno finche' resti senza sparare:
	 * e' l'unica forma misurata che fa passare i due oracoli di parcheggio **insieme**.
	 *
	 * ⚠️ **La coppia si tara sull'ESITO, non con una formula: non e' il rapporto a decidere.** Misurati
	 * quattro punti — `15/5` ✅ e `20/10` ✅, `20/5` 🔴 e `30/10` 🔴 — e `30/10` e `15/5` muoiono entrambi
	 * dopo tre turni dando esiti opposti. Si spedisce `15/5`, il punto su cui e' girata la suite intera, e
	 * a pinnarlo e' `HexBot.EngageBonusFadesWithIdleTurns` sull'ESITO di `ChooseBestPlan`. La taratura fine
	 * resta bilanciamento: #149 e D-102.
	 */
	UPROPERTY() int32 WEngageDecay = 5;

	/**
	 * Categoria `Objective` del punteggio (`spec-bot-tattico.md` §5): quanto vale CONTROLLARE la cella
	 * obiettivo — cioe' terminare il piano sopra di essa (`#2269`).
	 *
	 * ⚠️ **`120` viene dalla spec ed e' dichiarato indicativo**, accanto a `Damage +290` e
	 * `KillPotential +210`. Non e' un numero deciso: §5 scrive *«i valori sono tuning, non regola»*, e la
	 * sede del bilanciamento resta `#149` con il banco di prova che `D-102` richiede. Cio' che qui e'
	 * MISURATO e' che a questo valore nessun oracolo di parcheggio, ingaggio o oscillazione cambia verdetto.
	 *
	 * ⚠️ **INVARIANTE dichiarata: `WObjective < WKill`.** Un obiettivo non vale mai quanto un colpo letale —
	 * e con `120` contro `10000` il margine e' di due ordini di grandezza. E' una dichiarazione di tuning e
	 * **non** un gate: nessun valore sensato la viola, quindi un test che la asserisse non potrebbe fallire.
	 * A essere pinnato e' l'ESITO — `HexBot.ObjectiveNeverOutweighsAKill` — che e' la stessa proprieta'
	 * misurata dove si decide invece che dove si dichiara.
	 */
	UPROPERTY() int32 WObjective = 120;

	/**
	 * Quanto `WObjective` cala per ogni PASSO che manca all'obiettivo piu' vicino.
	 *
	 * 🔴 **INVARIANTE: `WObjectiveFalloff > WApproach`, e questa e' l'invariante che PUO' fallire.** Il
	 * termine tira verso l'obiettivo mentre `WApproach` tira verso il nemico: se i due gradienti si
	 * pareggiano, un passo che avvicina l'obiettivo e allontana il nemico vale esattamente zero, il
	 * tie-break «a parita' vince la mossa minima» fa restare, e il bot non ci va **proprio nel caso per cui
	 * il termine esiste**. Con `WApproach` a 10 servono almeno 11; si spedisce 15. Pinnata da
	 * `HexBot.ObjectivePullBeatsClosingOneCell`, che la misura sull'esito di `ChooseBestPlan` — non sul
	 * punteggio di una candidata isolata, che il tie-break non lo vede.
	 *
	 * ⚠️ **Da qui esce un RAGGIO d'attrazione dichiarato**: a `120/15` il termine e' zero da otto passi in
	 * poi. Oltre quella distanza l'obiettivo e' invisibile al punteggio, ed e' voluto — un'attrazione che
	 * arrivasse da qualunque punto della mappa sarebbe indistinguibile da un secondo `WApproach` con un goal
	 * diverso, e renderebbe ogni altra decisione una funzione di dove sta l'obiettivo.
	 *
	 * ⛔ **Zero non «disattiva il decadimento»: rende il bonus PIATTO su tutta la mappa**, che e' la forma
	 * assorbente di `#1088` con un goal nuovo. Per spegnere il termine si azzera `WObjective`, non questo.
	 */
	UPROPERTY() int32 WObjectiveFalloff = 15;

	/**
	 * Da quanti turni consecutivi il piano scelto per questa unita' **non contiene un attacco**. E' la
	 * memoria per unita' che `E26` (#326) portera' per intero; qui ne entra il minimo che serve al termine
	 * qui sopra.
	 *
	 * ⚠️ **Conta l'INTENTO, non l'esito**: si azzera quando il bot *pianifica* un attacco, non quando il
	 * colpo va a segno. E' la memoria che il bot ha di se' stesso, e ricostruirla dal TurnLog farebbe
	 * dipendere la pianificazione dalla risoluzione del turno prima.
	 *
	 * ⚠️ **Chi la tiene e' `ARTTurnManager`, non `ARTUnit`**, per la stessa ragione per cui il kiting non
	 * sta sull'eroe: e' un comportamento del BOT, e un'unita' che muovi tu non lo consulta mai.
	 */
	UPROPERTY() int32 IdleTurns = 0;
};

/**
 * Una reazione che il bot puo' armare, con il suo punteggio e la sua ORIGINE ([D-268], `#1802`).
 *
 * ⚠️ **L'origine e' un dato, non una posizione nell'array.** Fino a [D-220] «prima il kit» funzionava
 * perche' `EquipLoadout` accoda, cioe' per accidente: chi costruisce le candidate la chiede al catalogo, e
 * lo spareggio la legge di qui.
 */
USTRUCT()
struct FRTReactionCandidate
{
	GENERATED_BODY()

	/** Indice dell'abilita' sull'unita'. */
	UPROPERTY() int32 AbilityIndex = INDEX_NONE;

	/** Punteggio tattico, da `ScoreReaction`. */
	UPROPERTY() int32 Score = 0;

	/** `true` se viene dal KIT dell'eroe, `false` se da un modulo di loadout. */
	UPROPERTY() bool bFromKit = false;
};

/**
 * CHE COSA ha deciso la scelta, che non e' la stessa cosa di quale sia stata ([D-245]).
 *
 * 🔴 Serve perche' senza, uno spareggio che decide **sempre** sarebbe indistinguibile da un punteggio che
 * funziona: chi legge il log vedrebbe due volte la stessa riga per due meccanismi diversi.
 */
UENUM()
enum class ERTReactionTieBreak : uint8
{
	/** Nessuna candidata: non c'e' niente da armare. */
	None,

	/** Ha vinto per punteggio: nessuna la pareggiava in cima. */
	Utility,

	/** Pareggio in cima sciolto dall'ORIGINE: il kit prima del loadout. */
	Kit,

	/** Pareggio in cima fra candidate della STESSA origine, sciolto dall'indice piu' basso. */
	Index
};

/** La reazione scelta, con il perche'. */
USTRUCT()
struct FRTReactionChoice
{
	GENERATED_BODY()

	UPROPERTY() int32 AbilityIndex = INDEX_NONE;
	UPROPERTY() int32 Score = 0;
	UPROPERTY() ERTReactionTieBreak DecidedBy = ERTReactionTieBreak::None;
};

/**
 * Decisioni del bot su griglia ESAGONALE: logica pura, nessun Actor, solo interi (invariante #4).
 * Politica ereditata dal bot quadrato che ha sostituito (rimosso al CP 7.2) — focus-fire, minaccia mitigata
 * dalla copertura, kiting o avvicinamento, bonus di elevazione — con linea di vista letta dall'asset mappa.
 *
 * ⚠️ **La MINACCIA si misura in distanza esagonale, l'AVVICINAMENTO in PASSI sul grafo** (dal 2026-08-23,
 * #1296): un proiettile non cammina, un'unita' si'. Questa riga diceva «distanza esagonale» per entrambi, ed
 * era la premessa che ha prodotto il ciclo di periodo due sulla mappa d'autore.
 *
 * Le mosse candidate arrivano da URTHexSimLibrary::ReachableCells, che ha gia' applicato budget di movimento,
 * celle bloccate, unita' occupanti e archi verticali: il bot **non propone mosse illegali**. ⚠️ Diceva anche
 * «non rifa' pathfinding», e non e' piu' vero: `ScorePlan` percorre il grafo per misurare l'avvicinamento. La
 * differenza che regge e' un'altra — il bot non sceglie il PERCORSO, sceglie la destinazione.
 * Vedi docs/gameplay/spec-bot-hex.md.
 */
UCLASS()
class REFACTORTACTICS_API URTHexBotLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Distanza di sicurezza del kiting, DERIVATA dalla portata dell'attacco base. `0` = mischia.
	 *
	 * Il kiting e' un comportamento del BOT, non una caratteristica dell'eroe: un'unita' che muovi tu non lo
	 * consulta mai — dove andare lo decidi tu. Per questo il numero non sta piu' su `ARTUnit` (dove i due
	 * archetipi legacy lo scrivevano insieme alle statistiche) ne' su `URTHeroData`: un campo sull'eroe
	 * direbbe «Phase tiene le distanze» anche quando Phase la guidi tu, dove non significa niente.
	 *
	 * La regola riproduce i due archetipi che il comportamento lo producevano: `Ranger` aveva portata 6 e
	 * standoff 4, `Guardian` portata 3 e standoff 0. Chi colpisce da lontano ha qualcosa da guadagnare a
	 * restare lontano; chi colpisce da vicino no, e arretrare gli costerebbe soltanto il turno.
	 *
	 * Sul roster v0.1 l'unica kiter e' Phase (`PressureJet`, portata 5 -> standoff 3). Gadget e Wraith (4) e
	 * Riktor (3) chiudono la distanza. Se la soglia va spostata, e' questa riga: il resto del bot legge
	 * `FRTHexBotContext::KiteStandoff` e non sa da dove venga.
	 */
	/**
	 * Quante celle separano lo standoff dalla portata di tiro. `DeriveKiteStandoff` lo SOTTRAE dalla portata
	 * per ricavare lo standoff; `ScorePlan` lo RIAGGIUNGE per sapere da dove il kiter smette di essere
	 * indifferente alla distanza. I due usi devono restare legati: se qui diventasse 3 e li' restasse 2, il
	 * kiter pagherebbe per sparare dalla propria portata massima — il difetto misurato il 2026-08-23.
	 */
	static constexpr int32 KiterStandoffMargin = 2;

	static int32 DeriveKiteStandoff(int32 AttackRangeCells)
	{
		constexpr int32 KiterMinRange = 5;
		return AttackRangeCells >= KiterMinRange ? AttackRangeCells - KiterStandoffMargin : 0;
	}

	/**
	 * Utility score (intero) di una candidata: focus-fire (danno + bonus se uccide), meno la minaccia subita
	 * nella cella di destinazione (solo dai nemici che hanno gittata E linea di vista: la copertura protegge),
	 * meno la penalita' di posizionamento (kiter sotto standoff o oltre la propria portata / mischia lontana), piu' il bonus di quota.
	 */
	static int32 ScorePlan(const URTHexMapAsset* Map, const FRTHexBotPlan& Plan, const FRTHexBotContext& Context);

	/**
	 * Il solo termine di categoria `Objective`, per la cella in cui il piano TERMINA (`#2269`).
	 *
	 * ```text
	 * max(0, WObjective - WObjectiveFalloff * passi-fino-all-obiettivo-piu-vicino)
	 * ```
	 *
	 * I **passi** sono quelli sul grafo (la stessa misura dell'avvicinamento dal 2026-08-23, `#1296`), non
	 * la distanza in linea d'aria: un obiettivo dietro un muro non e' vicino perche' lo sembra sulla
	 * griglia. Occupare la cella vale `passi = 0`, cioe' il bonus pieno — che e' il «controllo» dello scope.
	 *
	 * 🔴 **E' pubblica perche' e' il BREAKDOWN, non per comodita' dei test.** `spec-bot-tattico.md` §5 chiede
	 * che ogni candidata produca un conto in chiaro e non un totale — *«un `Score = 670` senza righe e'
	 * indebuggabile: quando il bot sbaglia non si sa quale termine ha vinto, e si finisce a ritoccare i pesi
	 * a caso»*. Il breakdown completo e' lavoro di E26; questa e' la sua prima riga, e con lei
	 * `ARTTurnManager` puo' scrivere nel log **quanto** l'obiettivo ha pesato sulla scelta invece di lasciarlo
	 * dedurre da un totale.
	 *
	 * ⚠️ **Il floor a zero e' parte del termine, non una guardia.** Senza, un'unita' lontana pagherebbe una
	 * penalita' crescente per non stare sull'obiettivo, e quella penalita' entrerebbe in OGNI confronto —
	 * comprese le scelte di combattimento dall'altra parte della mappa, dove l'obiettivo non c'entra nulla.
	 *
	 * Mappa nulla o nessuna cella obiettivo -> `0`, e il punteggio resta quello di prima riga per riga: e'
	 * la ragione per cui questo lavoro non muove nessuna arena generata, che un obiettivo non ce l'ha.
	 */
	static int32 ScoreObjectiveTerm(const URTHexMapAsset* Map, const FRTCellId& DestCell,
		const FRTHexBotContext& Context);

	/**
	 * Punteggio tattico di una REAZIONE ([D-268], `#1802`), chiavato sul suo `ReactionTrigger`.
	 *
	 * Vale in proporzione alla minaccia a cui puo' rispondere, e la minaccia si misura **solo** su cio' che
	 * `Context` contiene — che e' gia' filtrato sulla conoscenza autorizzata della squadra, con la stessa
	 * regola del targeting umano. Nessun peso nuovo: `WThreat` e' quello del tuning.
	 *
	 * 🔴 **`Map` non e' un parametro di comodo: senza, il punteggio conterebbe nemici che non possono
	 * sparare.** `ScorePlan` la minaccia la misura come gittata **E** linea di vista, e sulla mappa d'autore
	 * — quella con l'ostacolo centrale che blocca vista e passo — un punteggio a sola distanza armerebbe un
	 * contrattacco contro chi sta dietro il muro: cioe' esattamente la «reazione che non sarebbe scattata»
	 * che [D-268] esiste per togliere, col segno rovesciato.
	 *
	 * ⚠️ **Un solo peso per tutti i termini, e non e' pigrizia.** In `ScorePlan` `WAllyDamage` e' per PUNTO
	 * di danno e `WThreat` e' per NEMICO: usarli qui come se fossero la stessa unita' avrebbe reso
	 * l'interposizione (10) sempre perdente contro un contrattacco (100) — il difetto di [D-220] rovesciato.
	 * `WThreat` misura «quanta minaccia questa reazione risponde», e vale per la mia cella come per quella di
	 * un alleato.
	 *
	 * ⚠️ **Si misura dalla cella di PARTENZA.** La selezione avviene prima che il piano — e quindi lo scatto
	 * — sia scelto, mentre i due trigger si valutano nel Blast, dopo il Dash. Un bot che scatta puo' quindi
	 * aver segnato la minaccia da una cella che avra' lasciato. E' una sovrastima dichiarata, non un modello
	 * completo: il codice di prima non guardava nessuna posizione.
	 */
	static int32 ScoreReaction(const URTHexMapAsset* Map, const FRTActionDef& Def, const FRTHexBotContext& Context);

	/**
	 * La reazione da armare fra le candidate: punteggio massimo, e a **parita' esatta** vince il kit
	 * ([D-268]). Ancora pari, vince l'indice piu' basso — cosi' permutare le candidate non cambia l'esito.
	 *
	 * Restituisce anche **da che cosa** e' stata decisa, perche' [D-245] chiede che la ragione sia un dato e
	 * non una deduzione di chi legge il log.
	 */
	static FRTReactionChoice SelectReaction(const TArray<FRTReactionCandidate>& Candidates);

	/**
	 * Candidata a punteggio massimo. TIE-BREAK ASSOLUTO: a parita' di punteggio vince la MOSSA MINIMA da
	 * Context.Origin (restare vince), poi l'ordine stabile della cella (StableLess) -> permutare le candidate
	 * non cambia l'esito. Nessuna candidata -> resta a Origin.
	 */
	static FRTHexBotPlan ChooseBestPlan(const URTHexMapAsset* Map, const TArray<FRTHexBotPlan>& Candidates,
		const FRTHexBotContext& Context);

	/**
	 * Mosse candidate dell'unita': per ogni cella raggiungibile entro il budget, una candidata senza attacco e
	 * una per ciascun nemico entro gittata e in linea di vista DA QUELLA CELLA. Ordine deterministico.
	 */
	static TArray<FRTHexBotPlan> BuildCandidates(const FRTHexSnapshot& Snapshot, int32 UnitId,
		const FRTHexBotContext& Context);

	/** Piano scelto per l'unita': ChooseBestPlan sulle candidate generate. */
	static FRTHexBotPlan PlanUnit(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTHexBotContext& Context);

	/**
	 * Segna in `Snapshot` le celle che l'unita' PRENOTA per raggiungere `DestCell` — la rotta intera, non la
	 * sola destinazione — cosi' che chi pianifica dopo non le veda libere.
	 *
	 * ⚠️ Prenota con l'id dell'unita' stessa, e non e' un dettaglio: `ReachableCells` tratta come occupata
	 * solo una cella il cui occupante e' un ALTRO (`*Occupant != UnitId`), quindi l'unita' non blocca la
	 * propria rotta mentre la blocca a tutte le altre.
	 *
	 * ⚠️ **E' piu' restrittivo del necessario**, dichiarato: nella risoluzione a micro-step due unita'
	 * potrebbero attraversare la stessa cella in momenti diversi senza contendersela. Qui la seconda dovra'
	 * trovare una rotta disgiunta, e in un corridoio stretto potrebbe non trovarne. E' un prezzo scelto: lo
	 * stallo di #1088 costava ZERO mosse per dodici turni.
	 *
	 * ⛔ **Lo snapshot su cui si prenota e' PER SQUADRA, e non e' un'ottimizzazione: e' fairness.** Le
	 * prenotazioni sono informazione sui piani, e i piani di una squadra sono privati (CP 13.5,
	 * `RT-FEAT-BOT-FAIRNESS`: *«il bot non vede piu' di te»*). Prenotando su uno snapshot condiviso fra le
	 * due squadre, un bot eviterebbe la cella dove sta per andare un AVVERSARIO — cioe' schiverebbe un
	 * intento che nessun giocatore puo' vedere. Due squadre che si contendono la stessa cella devono
	 * continuare a contendersela: quella e' una collisione legittima, che il resolver risolve, e non il
	 * difetto di #1088.
	 *
	 * ⚠️ **Chi prenota rende la pianificazione dipendente dall'ORDINE**: chi decide prima ha piu' scelta.
	 * Non intacca il determinismo — l'ordine e' quello dello snapshot, stabile — ma e' una regola nuova, e
	 * chi chiama deve iterare in un ordine stabile, non nell'ordine di enumerazione degli Actor.
	 *
	 * 🔴 **Quello che questa prenotazione garantisce e' DESTINAZIONI distinte, non PERCORSI disgiunti.**
	 * `ARTTurnManager::ResolveMovement` ricalcola la rotta su uno snapshot FRESCO, dove le prenotazioni non
	 * esistono: chi pianifica dopo sceglie una destinazione libera, ma puo' poi raggiungerla per la via
	 * diretta, cioe' quella che qui era stata scartata. Il limite e' dichiarato e misurato — sulla
	 * configurazione spedita non si osserva alcuna contesa, ma non e' impedita per costruzione.
	 * ⛔ Fissare la rotta su `PlannedPath` **non** chiude il buco: quel ramo di `ResolveMovement` non
	 * riapplica l'occupazione fresca, e una rotta vecchia di due fasi produce sovrapposizioni reali.
	 *
	 * Restituisce la rotta prenotata. Vuota = l'unita' resta dov'e', oppure il pathfinding ha fallito — e in
	 * quel caso e' loggato e la sola destinazione viene prenotata comunque.
	 */
	static TArray<FRTCellId> ReservePlannedRoute(FRTHexSnapshot& Snapshot, int32 UnitId,
		const FRTCellId& DestCell);

	/**
	 * La risposta del bot a una finestra di reazione (CP 14.5). Restituisce una delle `AllowedResponses`.
	 *
	 * ⚠️ **La firma E' il requisito.** Il DoD chiede che il bot decida «con la sola opportunity sanitizzata e
	 * restituisca subito: nessuna attesa reale, nessun accesso a percorsi futuri, trigger futuri o esiti
	 * precalcolati». Questa funzione non ha modo di violarlo: non riceve la mappa, non riceve lo snapshot, non
	 * riceve i percorsi, e `FRTReactionOpportunity` ha un elenco CHIUSO di campi verificato per riflessione da
	 * `Overwatch.OpportunityLeaksNoFuture`. E' una garanzia strutturale, non una promessa da testare — un test
	 * che asserisse «non ha guardato il futuro» potrebbe solo verificare l'esito, mentre qui il futuro non e'
	 * raggiungibile nemmeno volendo.
	 *
	 * Pura anche nel senso stretto: nessun `UWorld`, nessun Actor, nessun timer. «Restituisce subito» non e'
	 * una qualita' dell'implementazione, e' l'unica cosa che puo' fare.
	 *
	 * **Politica della v0.1**: spara al primo bersaglio legale, che e' quello di `UnitId` minore perche'
	 * `AllowedResponses` e' ordinato cosi'. Deterministica e deliberatamente semplice: tenere il colpo in
	 * attesa di un bersaglio migliore e' il *bait*, e vale quanto la stima di cosa arrivera' dopo — che e'
	 * esattamente cio' che il bot non puo' sapere qui, e che appartiene al Tactical Bot v1 (E26) e alla
	 * taratura di CP 14.6. Una politica piu' ambiziosa oggi sarebbe indistinguibile da un'euristica inventata.
	 */
	static FString DecideReactionResponse(const FRTReactionOpportunity& Opportunity);

	/**
	 * Cella di FUGA: fra quelle raggiungibili entro il budget, quella piu' lontana (distanza esagonale) dalla
	 * minaccia. A parita' di distanza vince il percorso piu' economico, poi l'ordine stabile della cella —
	 * quindi l'esito non dipende dall'ordine di enumerazione. Nessuna via d'uscita (budget 0, celle bloccate
	 * od occupate) -> resta dov'e'.
	 *
	 * Serve alla guardia di "panico" del kiter, che rinuncia al tiro per non farsi raggiungere: e' una scelta
	 * di posizionamento pura, non un'utility, e resta separata da ScorePlan come nel bot quadrato.
	 */
	static FRTCellId BestKiteCell(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTCellId& Threat);
};
