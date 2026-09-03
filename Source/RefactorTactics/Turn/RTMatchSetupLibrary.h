#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTMatchSetupLibrary.generated.h"

class URTHexMapAsset;

/**
 * Una posizione di partenza dello scenario showcase: QUALE eroe, in QUALE squadra, su QUALE cella.
 * Legare i tre dati insieme evita l'ordine implicito di tre array paralleli, che si disallineano in silenzio.
 */
USTRUCT(BlueprintType)
struct FRTShowcaseSpawn
{
	GENERATED_BODY()

	/** `HeroId` del catalogo eroi (`Hero.Gadget`, `Hero.Phase`, `Hero.Riktor`, `Hero.Wraith`), non un archetipo legacy. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Showcase")
	FName HeroId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Showcase")
	int32 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Showcase")
	FRTCellId Cell;

	FRTShowcaseSpawn() = default;
	FRTShowcaseSpawn(FName InHeroId, int32 InTeamId, const FRTCellId& InCell)
		: HeroId(InHeroId), TeamId(InTeamId), Cell(InCell) {}
};

/**
 * Funzioni PURE di allestimento della partita su mappa esagonale: scelta delle celle di partenza e
 * ricostruzione dell'occupazione dallo stato delle unita'. Nessuno stato, nessun Actor, nessun World:
 * il GameMode le chiama, i test le verificano headless.
 */
UCLASS()
class REFACTORTACTICS_API URTMatchSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Celle di partenza per un NumPerTeam vs NumPerTeam sul layer indicato: le prime NumPerTeam al team 0,
	 * le successive al team 1. Prende le celle percorribili in ordine STABILE (Layer, X, Y) e assegna i due
	 * team dalle due estremita' dell'ordine, cosi' le squadre partono lontane senza dipendere da RNG.
	 * Mappa nulla o celle percorribili insufficienti -> array vuoto (il chiamante non allestisce a meta').
	 */
	static TArray<FRTCellId> PickStartCells(const URTHexMapAsset* Map, int32 NumPerTeam, int32 Layer);

	/**
	 * Arena di RIPIEGO: esagono pieno di raggio `Radius` sul layer 0, tutte le celle percorribili.
	 *
	 * Serve quando il livello non porta una mappa esagonale: senza, il GameMode spawnerebbe un
	 * `ARTHexMapActor` con l'asset vuoto e la partita non si allestirebbe — premere Play non mostrerebbe
	 * nulla. Non sostituisce una mappa d'autore: e' un fondo di scena giocabile, dichiarato nel log.
	 * `Radius < 1` o `Outer` nullo -> nullptr (nessuna arena a meta').
	 */
	static URTHexMapAsset* MakeDemoArena(UObject* Outer, int32 Radius);

	/**
	 * **Mappa di prova** generata da codice: l'arena che serve a esercitare le regole, non un fondo di scena.
	 * Esagono pieno di raggio 4 sul layer 0 con, in aggiunta:
	 * - celle che **bloccano il movimento** (ostacoli),
	 * - celle che **bloccano la vista** allineate fra le due meta' del campo (copertura/LOS),
	 * - una fascia di terreno **costoso** (Mud, costo 3) per vedere il budget mordere,
	 * - una **piattaforma sul layer 1** raggiungibile SOLO tramite un'unica transizione.
	 *
	 * Esiste in codice e non come `.uasset` per una ragione precisa: `Content/**` e' gitignorato, quindi una
	 * mappa dipinta a mano non sopravvive a un clone e non e' riproducibile fra macchine. Questa si'.
	 *
	 * `Outer` nullo -> nullptr.
	 */
	static URTHexMapAsset* MakeTestArena(UObject* Outer);

	/**
	 * **Arena PIATTA** di raggio `Radius` attorno a `Center`: un esagono pieno su un layer, pavimento a
	 * costo 1, niente ostacoli, niente coperture, niente terreni. Le celle escono gia' ordinate.
	 *
	 * E' il banco di prova piu' usato del progetto, ed era **duplicato in 49 punti** sotto quattordici nomi
	 * diversi (`SpawnMap`, `MakeFlatMap`, `SpawnHexMap`, `SpawnCleanInteractionMap`, ...).
	 *
	 * ✅ **Muove `Revision` UNA volta** ([D-196], 2026-08-26). Ogni copia che sostituisce ne faceva una per
	 * cella — 127 per un raggio 6 — e quel numero entra in `GraphRevision`, quindi nell'identita' delle
	 * tracce archiviate: la correzione e' stata rimandata due volte proprio perche' fa cadere il corpus
	 * golden, e alla fine e' stata fatta con la rigenerazione dichiarata nella stessa PR.
	 *
	 * ⚠️ **Vale per QUESTO builder, non per il file**: gli altri di `RTMatchSetupLibrary.cpp` — `MakeTestArena`
	 * e le arene di showcase — chiamano ancora `AddOrUpdateCell` per cella, quindi consegnano a
	 * `CurrentGraphRevision()` un numero a forma di conteggio celle. Generalizzarlo e' un lavoro suo
	 * (`#1435`), non un effetto di questa riga.
	 *
	 * `Center` esiste perche' non tutte le arene stanno sull'origine — `HexMap.CenterCell` ne costruisce una
	 * su `(5,-2,0)` proprio per provare che il centro segue la mappa.
	 *
	 * Non sostituisce `MakeTestArena`: quella esercita le REGOLE (ostacoli, copertura, terreno costoso, un
	 * secondo layer), questa e' il foglio bianco su cui si misura una regola alla volta.
	 *
	 * `Outer` nullo o `Radius` negativo -> `nullptr`.
	 */
	static URTHexMapAsset* MakeFlatArena(UObject* Outer, int32 Radius,
		const FRTCellId& Center = FRTCellId(0, 0, 0));

	/**
	 * **Arena della showcase «Il Relè» — versione Lite** (CP 15.2): la fixture d'integrazione riproducibile.
	 *
	 * Esagono pieno di raggio 5 sul layer 0 (91 celle), con le sole superfici gia' atterrate e nessuna
	 * regola nuova: acqua al centro, conduttivo a contatto, `Rough`, `Ice`, `Fire`, `Smoke`. Il layout e'
	 * **simmetrico rispetto al centro** — la superficie di `(q,r)` e quella di `(-q,-r)` coincidono — cosi'
	 * un esito non e' mai spiegabile con «quella meta' campo era piu' comoda».
	 *
	 * I costi di movimento li detta il catalogo terreni: la fixture non incide numeri propri.
	 *
	 * `Outer` nullo -> nullptr (nessuna arena a meta').
	 */
	static URTHexMapAsset* MakeShowcaseRelayLiteArena(UObject* Outer);

	/** Posizioni di partenza canoniche della showcase: Gadget + Phase (team 0) contro Riktor + Wraith (team 1). */
	static TArray<FRTShowcaseSpawn> GetShowcaseRelayLiteSpawns();

	/**
	 * **Arena «Relay Basin» — la mappa CANONICA della showcase** (`RT_Showcase_Relay_v01`).
	 *
	 * 45 celle su un solo layer, forma irregolare (7 righe, `q` da -4 a +4 sulle due centrali), con l'obiettivo
	 * `Relay` a `(0,0,0)` e le due squadre agli estremi ovest ed est. Terreni: `Smoke` a ovest, lane
	 * `ShallowWater` -> `Conductive` a sud, `Rough` a sbarrare la via diretta est->Relay, `Fire` sulla soglia
	 * est, cresta `HighGround` a nord-est, ripiano `Ice` a sud. In piu': una **copertura bassa** sull'approccio
	 * nord al Relay e un **gate chiuso** (una porta, CP 9.3) sulla lane sud.
	 *
	 * **Non sostituisce `MakeShowcaseRelayLiteArena`.** Quella e' un esagono simmetrico che serve al
	 * determinismo; questa e' la mappa degli 8 turni. Rispondono a due domande diverse — «l'esito e'
	 * riproducibile?» e «il gioco sa mostrare cio' che dice di essere?» — e una non implica l'altra.
	 *
	 * Il layout e' **autorato** (`docs/product/showcase-v0.1.md` §2): la spec di scenario dichiarata
	 * dall'handoff non esiste nel repository. `RefactorTactics.ShowcaseRelay.BasinLayoutMatchesSpec` e' cio'
	 * che gli impedisce di derivare in silenzio.
	 *
	 * I costi di movimento li detta il **catalogo terreni**: la fixture non incide numeri propri.
	 *
	 * `Outer` nullo -> nullptr (nessuna arena a meta').
	 */
	static URTHexMapAsset* MakeShowcaseRelayBasinArena(UObject* Outer);

	/** Spawn canonici del Relay Basin: Gadget `(-4,0)` + Phase `(-4,1)` contro Riktor `(4,0)` + Wraith `(4,1)`. */
	static TArray<FRTShowcaseSpawn> GetShowcaseRelayBasinSpawns();

	/**
	 * **Cover Yard**: esagono di raggio 3, tutto pavimento, con due bordi coperti e nient'altro.
	 *
	 * Esiste perche' la copertura **alta** (CP 9.2) non compare in nessun'altra fixture, e senza una cella
	 * dove trovarla non e' possibile scrivere uno scenario che la metta alla prova. Il Relay Basin non e' il
	 * posto: e' la mappa autorata degli 8 turni, protetta da `BasinLayoutMatchesSpec`, e aggiungerle una
	 * barriera per comodita' di test cambierebbe la showcase per una ragione che con la showcase non c'entra.
	 *
	 * Due bordi, scelti per essere confrontabili:
	 *
	 * - **alta** fra `(0,0)` e `(1,0)`: nega vista, passo e proiettili nei DUE versi, integrita' 50;
	 * - **bassa** fra `(0,1)` e `(1,1)`: lascia passare tutto e toglie 10 al danno diretto dal lato riparato.
	 *
	 * Sono sulla stessa direzione a una riga di distanza, quindi la differenza fra «riduce» e «nega» si
	 * osserva muovendo il bersaglio di una cella invece di cambiare mappa.
	 *
	 * Niente terreni: una fixture che serve a studiare i BORDI non deve avere anche superfici che spiegano
	 * un esito diverso. `Outer` nullo -> nullptr.
	 */
	static URTHexMapAsset* MakeCoverYardArena(UObject* Outer);

	/**
	 * 🔑 **I TRE casi di blocco affiancati: solo-movimento, entrambi, solo-vista.** E' la fixture di
	 * `PIE-HEX-VIZ-BLOCCHI`, che chiede di capire *guardando dall'alto* quale cella si attraversa e quale no.
	 *
	 * Sorella di `CoverYard` — stesso esagono di raggio 3, stesso pavimento ovunque, stessa disciplina: niente
	 * superfici, perche' un terreno che cambia colore darebbe una seconda spiegazione a «quella cella si legge
	 * diversa» e la voce chiede della FORMA.
	 *
	 * ⚠️ **Esiste perche' il caso solo-movimento non c'era in nessuna delle otto fixture precedenti**, ed era
	 * sempre lui a mancare: `ArenaV01` e `GrayKitYard` hanno solo-vista ed entrambi, `CoverYard` non ha celle
	 * bloccanti. Chi eseguiva la voce doveva dipingerne una a mano, e fermarsi prima lasciava il confronto a
	 * due.
	 *
	 * ⚠️ **Non e' una scena di gameplay e non va usata come tale**: tre celle bloccanti in fila sulla riga
	 * centrale non descrivono un percorso, una copertura o un trade-off. Per quelli ci sono `GrayKitYard` e
	 * `ArenaV01`. `Outer` nullo -> nullptr.
	 */
	static URTHexMapAsset* MakeBlockYardArena(UObject* Outer);

	/**
	 * 🔑 **La scena della seduta `U25`+`U35`: tutto cio' che le dieci verifiche guardano, in una fixture sola.**
	 *
	 * E' `CoverYard` con tre aggiunte, e la base identica e' un REQUISITO e non pigrizia: `PIE-GBX-FIT`
	 * dichiara che `GBX-1` si decide provando valori di inset, e **due letture su scene diverse non sono
	 * confrontabili** — la seconda misurerebbe la scena invece dell'inset.
	 *
	 *     le due coperture      alta e bassa, sulle stesse celle di `CoverYard`     -> PIE-GBX-COVER
	 *     quattro porte         Destroyed | Open | Closed | Locked, in fila         -> PIE-GBX-DOOR
	 *     acqua e ghiaccio      due celle CIASCUNA, adiacenti                       -> PIE-GBX-SURFACE
	 *     coppie stessa superficie  acqua|acqua e ghiaccio|ghiaccio                 -> PIE-GRID-CONFINE
	 *
	 * 🔴 **Perche' due celle per superficie e non una.** `PIE-GBX-SURFACE` vuole due superfici DIVERSE
	 * adiacenti, e le ha. `PIE-GRID-CONFINE` vuole l'opposto — due celle della **stessa** superficie, dove il
	 * colore non dice dove finisce una — e con una cella per tipo quel caso non sarebbe allestito: il giudizio
	 * piu' difficile della griglia resterebbe non guardabile proprio nella scena costruita per guardarlo.
	 *
	 * ⚠️ **Porta il DATO, non le mesh del kit.** `SM_Graybox_*` sono asset da posare in scena, e nessuna
	 * fixture li posa: `PIE-GBX-COVER`, `-DOOR` e `-SURFACE` guardano **il kit**, quindi restano manuali per
	 * quella meta'. Cio' che questa fixture toglie e' l'allestimento del dato — incluso lo `Cells[i].Doors`
	 * scritto a mano nel Details e il ridisegno da forzare, che e' la trappola che faceva sembrare fallita
	 * una voce solo non allestita.
	 */
	static URTHexMapAsset* MakeGrayKitYardArena(UObject* Outer);

	/**
	 * **Arena della v0.1** — il layout che soddisfa i tre criteri del `done_when` di U1
	 * (`docs/roadmap/editor-sessions.yaml`, passi 3, 4 e 7), verificati da
	 * `URTArenaCriteriaLibrary` e non a occhio.
	 *
	 * Esagono pieno di raggio 4 sul layer 0, con:
	 * - una **barriera centrale** che blocca passo e vista, cosi' la mappa ha due vie invece di un corridoio;
	 * - una **fascia di fango** trasversale che ogni via deve attraversare — se si aggirasse non sarebbe una
	 *   scelta ma un dettaglio;
	 * - un **muro** che scherma la via meridionale dallo spawn orientale, cosi' la rotta piu' cara e' anche
	 *   la piu' coperta: e' cio' che rende le due vie un trade-off invece che una migliore e una peggiore.
	 *
	 * Non sostituisce `MakeTestArena`, che resta il banco della parita' hex: quella esercita le regole, questa
	 * e' il contenuto della v0.1. `MakeTestArena` infatti soddisfa **uno** dei tre criteri, e non e' un
	 * difetto — non era stata costruita per questo.
	 *
	 * `Outer` nullo -> nullptr.
	 */
	static URTHexMapAsset* MakeArenaV01(UObject* Outer);

	/**
	 * **Arena della vista divisa** — il banco su cui la percezione di squadra e' OSSERVABILE, che le altre
	 * arene generate non sono.
	 *
	 * 🔴 **Esiste per due difetti misurati, non per completezza** ([#1738]). Le arene di raggio **4**
	 * (`MakeTestArena`, `MakeArenaV01`) hanno un raggio piu' corto del `VisionRange` piu' corto del roster
	 * (**5**, Phase e Riktor; Wraith 6, Gadget 7): una squadra vede quasi tutto al primo turno. E poiche'
	 * `ExploredCells` **non scade** ([D-227]), cio' che resta si esaurisce comunque: misurato, le celle mai
	 * viste vanno a **zero al turno 2**. ∴ su quelle arene i tre stati del velo non si distinguono a schermo,
	 * e un confine della visibilita' coincide col bordo della mappa.
	 *
	 * Esagono pieno di **raggio 8** sul layer 0 — piu' largo del `VisionRange` massimo, quindi resta terreno
	 * mai visto anche dopo diversi turni — con in aggiunta:
	 * - un **muro che blocca vista e passo** lungo `r = 1`, da `q = -8` a `q = 0`: divide la meta' occidentale
	 *   in due camere. Le celle di partenza del team 0 sono le prime due percorribili in ordine `(q, r)`
	 *   (`PickStartCells`), cioe' `(-8, 0)` a nord del muro e `(-8, 2)` a sud — quindi i due compagni nascono
	 *   in camere diverse e la loro vista di squadra e' **disgiunta dal turno 1**, senza dipendere da come
	 *   si muovono;
	 * - due **nicchie** che rendono il perimetro visibile **concavo** invece che a ventaglio;
	 * - una **piattaforma sul layer 1**, che nel grafo tattico e' una regione a se': `Neighbors` non
	 *   attraversa i layer, quindi le sue celle visibili non confinano con quelle del suolo.
	 *
	 * ⚠️ **Non sostituisce `MakeTestArena` ne' `MakeArenaV01`**, e non ne cambia una riga: quelle sono
	 * tarate su criteri di movimento e copertura (`URTArenaCriteriaLibrary`, `done_when` di `U1`) e allargarle
	 * cambierebbe il bilanciamento e il corpus golden. Questa e' additiva e serve a una domanda sola.
	 *
	 * `Outer` nullo -> `nullptr`.
	 */
	static URTHexMapAsset* MakeVisionSplitArena(UObject* Outer);

	/**
	 * **`ProbeYard` — la mappa su cui la sonda di movimento (#711) si guarda in seduta.**
	 *
	 * Porta, in un esagono piccolo, le **tre condizioni che separano i motivi** che la sonda puo' dare:
	 *
	 * | Condizione | Dove | Quale motivo rende osservabile |
	 * |---|---|---|
	 * | superficie costosa | fascia a `q = -2` | `OutOfBudget` **vicino**, non al bordo |
	 * | cella che blocca | i sei vicini di `(3,0)` | `BlocksMovement` |
	 * | zona isolata | `(3,0)`, murata dai suoi sei | `NoRoute` |
	 *
	 * 🔴 **Perche' esiste, misurato invece che supposto.** Il 2026-08-31 la mappa aperta in `L_DevSandbox`
	 * (`DA_HexMap_Scratch_Basin`) e' stata interrogata cella per cella attraverso il ponte MCP con lo stesso
	 * A* che la sonda usa: `Reachable 57 · BlocksMovement 3 · OutOfBudget 1 · **NoRoute 0**`. Su una mappa
	 * senza zone isolate la seduta non puo' distinguere «ti manca movimento» da «non c'e' strada» — cioe'
	 * proprio la distinzione per cui #711 e' stata fatta.
	 *
	 * ⚠️ **La camera e' murata da CELLE, non da bordi chiusi.** Un varco negato da un arco sarebbe una
	 * topologia diversa e la sonda direbbe lo stesso `NoRoute`, ma il designer in seduta non vedrebbe
	 * **perche'**: sei celle nere attorno a una libera si leggono a colpo d'occhio.
	 *
	 * ⚠️ **Quattro motivi, non cinque.** `Occupied` non e' ottenibile dal tool d'editor, che mette una sola
	 * unita' nel suo snapshot. La fixture non prova a produrlo, e un test lo pinna.
	 *
	 * `Outer` nullo -> `nullptr`.
	 */
	static URTHexMapAsset* MakeProbeYardArena(UObject* Outer);

	/**
	 * **Fixture di mappa per nome.** I nomi validi li dice `KnownFixtureIds()`, che li deriva dalla stessa
	 * tabella che li dispaccia: elencarli qui a mano e' cio' che ha prodotto `#1459`.
	 *
	 * E' il punto d'ingresso che permette a uno scenario di **riferire** una geometria invece di duplicarla.
	 * Deliberatamente una funzione con un elenco chiuso e non un registry generico: le fixture sono poche,
	 * nominate e versionate col codice, e un registry a runtime aggiungerebbe un modo di sbagliare (una
	 * fixture registrata da qualche parte e non da un'altra) senza aggiungere niente.
	 *
	 * Nome sconosciuto o vuoto -> **nullptr**, mai un'arena vuota: il chiamante deve poter dire *quale* nome
	 * non esiste, invece di far girare una partita su una mappa senza celle.
	 */
	static URTHexMapAsset* MakeFixtureArena(UObject* Outer, const FString& FixtureId);

	/**
	 * I nomi che `MakeFixtureArena` accetta, **derivati dalla stessa tabella che li dispaccia**.
	 *
	 * ⚠️ Esiste perche' l'elenco era scritto a mano in tre posti e nessuno dei tre coincideva col codice
	 * (`#1459`): la doc di `MakeFixtureArena` e il messaggio d'errore di `GenerateFixtureIntoAsset`
	 * nominavano `DemoArena`, che non ha un ramo, e la doc ometteva `ArenaV01`, che ce l'ha. Chi chiedeva
	 * `DemoArena` riceveva «fixture sconosciuta» seguito da un elenco che la conteneva.
	 *
	 * Chi mostra i nomi all'utente chiede QUI. L'elenco chiuso resta una scelta dichiarata nel commento di
	 * `MakeFixtureArena` — cambia solo che ora e' chiuso in **un** posto.
	 */
	static TArray<FString> KnownFixtureIds();

	/**
	 * Occupazione cella -> UnitId ricostruita dallo stato delle unita': solo le vive occupano.
	 * I tre array sono paralleli; lunghezze incoerenti -> mappa vuota (input non fidato, nessun indovinare).
	 */
	static TMap<FRTCellId, int32> BuildOccupancy(const TArray<FRTCellId>& Cells,
		const TArray<int32>& UnitIds, const TArray<bool>& Alive);
};
