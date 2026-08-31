#pragma once

#include "CoreMinimal.h"
#include "Perception/RTTeamKnowledge.h"
#include "ScenarioHarness/RTScenarioDraft.h"

class URTHeroData;
class URTHexMapAsset;

/**
 * La conoscenza di squadra dello **stato iniziale** di uno scenario, senza mondo e senza `ARTTurnManager`.
 *
 * 🔑 **Perche' esiste, e perche' sta nel RUNTIME.** In PIE la conoscenza si chiede all'autorita'
 * (`ARTTurnManager::KnowledgeForTeamPublic`). Fuori da PIE — nel Tactical Designer — **non esiste un
 * TurnManager**: la preview e' un `ARTHexMapActor` piu' un attore di marcatori nel mondo dell'Editor, e
 * `URTScenarioAuthoring::Run` un mondo lo costruisce e lo smonta subito. La via, li', e'
 * `URTTeamKnowledgeLibrary::Observe`, che e' pura ed e' **lo stesso produttore** che il TurnManager chiama a
 * `RTTurnManager.cpp:515`. Non e' una seconda verita': e' la stessa funzione con gli stessi ingressi.
 *
 * ⛔ **E per questo non sta nel modulo Editor.** Costruire un `FRTPerceiver` significa decidere `VisionRange`
 * e `Facing`, cioe' *chi vede cosa* — e quella e' una risposta del runtime. L'Editor decide quale squadra
 * osservare e come disegnarla; non deriva gli ingressi della percezione. Il guardrail di #1754 e' esplicito,
 * e questo file e' dove smette di essere una buona intenzione.
 *
 * 🔴 **Nessun predicato nuovo.** Chi conosca chi lo decide `URTTeamKnowledgeLibrary::ClassifyTarget`, che e'
 * gia' l'owner della regola (CP 13.2) e sa gia' che una squadra conosce sempre i propri. Qui si costruiscono
 * gli **ingressi** e si legge la **risposta**: riscrivere il confronto sarebbe la seconda definizione che
 * [D-223] vieta.
 *
 * ⚠️ **`REFACTORTACTICS_API` su ogni funzione, e non sul namespace.** Il consumatore vive nel modulo
 * **Editor**, che e' una DLL diversa: senza l'export il link non risolve — misurato, `LNK2019` su tre
 * simboli. Una macro sul namespace non esiste, e le funzioni si marcano una per una come fa gia'
 * `RTFrontendScreenIds.h` per le sue costanti.
 *
 * ⚠️ **E' lo stato INIZIALE, e non il playback.** Uno scenario aperto ha un solo istante: il turno 0, quello
 * che il file dichiara. La memoria — cosa resta di un nemico uscito dalla vista — richiede un turno 2, e
 * arriva col playback (#1625) leggendo `KnowledgeForTeamPublic` dal mondo che il runner costruisce. Qui
 * `Previous` e' vuota apposta: fingere un ricordo che nessuno ha osservato sarebbe la vista sotto mentite
 * spoglie.
 */
namespace RTScenarioKnowledge
{
	/**
	 * Il `TeamId` della prospettiva ONNISCIENTE: nessuna squadra possiede questa vista.
	 *
	 * E' `INDEX_NONE` e non `-1` scritto a mano, ed e' un valore **nominato** invece di una convenzione da
	 * ricordare: un `TeamId` valido non lo eguaglia mai, quindi il selettore puo' portarlo come una
	 * posizione qualunque senza un booleano parallelo che dica «questa e' speciale».
	 */
	inline constexpr int32 OmniscientTeamId = INDEX_NONE;

	/**
	 * L'identita' con cui un'unita' dello scenario entra ed esce dalla conoscenza, in QUESTA lettura.
	 *
	 * ⚠️ **Non e' lo `StableUnitId` della partita**, che `ARTTurnManager` assegna per indice sul roster vivo
	 * (`RTTurnManager.cpp:2396`) e che qui non esiste: fuori da PIE non c'e' nessun `ARTUnit`. E' un ponte
	 * fra `FRTScenarioUnitView::Id` — che e' una `FString` — e l'`int32` che `FRTLastKnownContact` e
	 * `ClassifyTarget` richiedono.
	 *
	 * 🔴 **Vale solo dentro un array `Units`, e le funzioni qui sotto vanno percio' chiamate con lo STESSO
	 * array.** Passare a `VisibleUnits` una lista diversa da quella data a `ForTeam` farebbe corrispondere
	 * gli id alle unita' sbagliate, e l'esito non sarebbe un errore ma **le unita' sbagliate a schermo** —
	 * cioe' un leak che si legge come una scelta di presentazione.
	 *
	 * `+ 1` perche' `INDEX_NONE` e lo zero sono gia' presi: il primo come «nessuna unita'», il secondo come
	 * il valore che `FRTLastKnownContact` ha appena costruito.
	 */
	inline int32 LocalUnitId(int32 IndexInUnits) { return IndexInUnits + 1; }

	/**
	 * Le squadre che lo scenario schiera davvero, crescenti e senza ripetizioni.
	 *
	 * ⚠️ **Dal DATO, mai da un letterale.** `{0, 1}` cablato e' il difetto che #1535 ha gia' registrato su
	 * `ARTHUD`: il 4v4 e' *un cambio di dato*, e un selettore che ne mostrasse due mentirebbe sulla terza.
	 * Uno scenario a squadra sola ne restituisce una, e il selettore avra' due posizioni invece di tre —
	 * che e' corretto, non un caso limite da tappare.
	 */
	REFACTORTACTICS_API TArray<int32> TeamIds(const TArray<FRTScenarioUnitView>& Units);

	/**
	 * Gli OSSERVATORI di una squadra: posizione, orientamento e raggio visivo, pronti per `Observe`.
	 *
	 * 🔑 **`VisionRange` si risolve dall'`HeroId`, e non ha un ripiego silenzioso.** E' l'ingresso che
	 * `FRTScenarioUnitView` non porta — la vista e' quella che l'eroe dichiara (`URTHeroData::VisionRange`),
	 * e inventarla qui darebbe una percezione che il gioco poi non conferma. Un `HeroId` che il roster non
	 * conosce **non produce un osservatore**: quell'unita' non vede, invece di vedere con un raggio
	 * inventato. E' fail-closed, come `VisibleCells` senza mappa.
	 *
	 * @param Roster gli eroi gia' letti. Chiamare `URTHeroCatalogLibrary::GetHeroRoster()` per unita'
	 *               costruirebbe quattro `URTHeroData` **con tutte le loro abilita'** a ogni giro: la lista
	 *               si passa, come `FRTUnitPlacementScratch::KnownHeroes` fa per la stessa ragione.
	 */
	REFACTORTACTICS_API TArray<FRTPerceiver> Observers(const TArray<FRTScenarioUnitView>& Units, int32 TeamId,
		const TArray<URTHeroData*>& Roster);

	/**
	 * La conoscenza dello stato iniziale per una squadra, o quella onnisciente se `TeamId` e'
	 * `OmniscientTeamId`.
	 *
	 * 🔑 **`Omniscient` e' una posizione NOMINATA, non «il filtro spento».** `ApplyKnowledgeVeil` riceve un
	 * `FRTTeamKnowledge` e non un predicato, quindi l'onniscienza si esprime costruendo la conoscenza che
	 * vede tutto — e il percorso di codice resta **lo stesso**. Un ramo che saltasse il velo sarebbe una
	 * seconda strada che nessun test attraversa, e divergerebbe dalla prima al primo cambiamento. E' anche
	 * il motivo per cui la scelta della prospettiva sta in questo parametro e non in un `if` del chiamante.
	 *
	 * Un `TeamId` che non compare fra le unita' produce una conoscenza **vuota**, non un'eccezione: e' cio'
	 * che il dato dice, e velare tutto e' la risposta onesta a «questa squadra non c'e'».
	 */
	REFACTORTACTICS_API FRTTeamKnowledge ForTeam(const URTHexMapAsset* Map, const TArray<FRTScenarioUnitView>& Units,
		int32 TeamId,
		const TArray<URTHeroData*>& Roster);

	/**
	 * Le unita' che la prospettiva permette di DISEGNARE, nell'ordine dell'array in ingresso.
	 *
	 * 🔴 **E' il filtro che il velo non fa.** `ApplyKnowledgeVeil` copre le cinque famiglie di istanze della
	 * mappa; i marcatori delle unita' stanno su un altro attore e non li tocca nessuno. Velare la board e
	 * lasciare i marcatori mostrerebbe ogni nemico mai visto **rispettando alla lettera** il resto: e'
	 * l'hidden-state leak piu' facile da introdurre qui, e questa funzione esiste per non lasciarlo alla
	 * memoria di chi disegna.
	 *
	 * La regola non e' scritta qui: si chiede a `ClassifyTarget`, e `Rejected` e' l'unico esito che toglie
	 * l'unita' dallo schermo. ⚠️ `CellOnly` — il ricordo — **resta**: e' un'unita' di cui si sa la cella e
	 * non l'identita', e cancellarla farebbe sparire un ricordo che [D-227] dice di conservare. Come
	 * mostrarla la decide chi disegna; che ci sia lo decide questa riga.
	 *
	 * @param Knowledge l'esito di `ForTeam` **sullo stesso array `Units`** — vedi `LocalUnitId`.
	 */
	REFACTORTACTICS_API TArray<FRTScenarioUnitView> VisibleUnits(const TArray<FRTScenarioUnitView>& Units,
		const FRTTeamKnowledge& Knowledge);
}
