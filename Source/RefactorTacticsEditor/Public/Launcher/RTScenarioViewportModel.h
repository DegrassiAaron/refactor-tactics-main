#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"

struct FRTScenarioUnitView;

/**
 * Le DECISIONI di presentazione dello stato iniziale di uno scenario nel viewport, separate da chi le
 * disegna.
 *
 * 🔑 **Perche' esiste come funzioni libere invece che come metodi dell'actor di preview.** Di una slice
 * fatta di componenti e transform un automation test vede pochissimo: puo' costruire un actor, non puo'
 * guardare a schermo. Cio' che invece puo' esaminare e' *dove* un marcatore finisce, *verso dove* punta e
 * *quali layer* lo scenario occupa — e sono esattamente le tre cose che, sbagliate, producono un viewport
 * plausibile e falso. Quindi stanno qui, pure, e l'actor resta un guscio che le chiama.
 *
 * E' la stessa scelta di `FRTLauncherScenarioBrowser` per #1705, per la stessa ragione: una regola scritta
 * dentro il widget e' una regola che nessun test guarda.
 *
 * ⛔ **Nessuna regola di gioco qui dentro.** Se una cella sia legale, se un'unita' ci possa stare, quale
 * copertura si applichi: sono risposte del runtime (`URTScenarioLoader::ValidateUnitPlacement`,
 * `URTHexCoverLibrary`) e questo file non le ripete ne' le anticipa. Qui c'e' solo *come si mostra* cio' che
 * il dato dichiara.
 */
namespace RTScenarioViewport
{
	/**
	 * Dove sta e verso dove guarda il marcatore di un'unita' sulla sua cella.
	 *
	 * 🔴 **Lo yaw e' DERIVATO, non inciso.** Non esiste — ed e' stato cercato — una conversione canonica
	 * `ERTHexDirection -> yaw` nel runtime: il facing e' definito come *dove sta il vicino*, e
	 * `URTHexLibrary` espone `Neighbor` e `AxialToWorld`. La rotazione qui e' la direzione fra i due centri
	 * di cella, cioe' la composizione delle due primitive canoniche — lo stesso modo in cui
	 * `EdgeMidpointWorld` ricava il proprio punto (*«derivato, non inciso: il punto viene dai due centri»*).
	 * Una tabella di sei angoli scritta a mano si scollegherebbe da `AxialDirection` al primo cambio di
	 * convenzione degli assi, e nessun compilatore lo direbbe.
	 *
	 * La quota e' `RTCellTopZ` sopra il centro cella: sotto, il marcatore finirebbe **dentro** il prisma
	 * della cella e a schermo sarebbe indistinguibile da un marcatore mai disegnato — il difetto che
	 * `RTMapVisuals.h` documenta essere gia' costato due volte.
	 *
	 * La scala e' unitaria: la taglia dei singoli componenti la decide chi disegna, non questa funzione.
	 */
	FTransform MarkerTransform(const FRTCellId& Cell, ERTHexDirection Facing,
		const FVector& Origin, float HexSize, float LayerHeight);

	/**
	 * I `Layer` occupati dalle unita' dello scenario, crescenti e senza ripetizioni.
	 *
	 * ⚠️ **Serve a DICHIARARE cosa si sta mostrando, non a filtrare.** `FRTCellId` porta il layer, e due
	 * celle con lo stesso `X/Y` su layer diversi sono celle diverse: un viewport che le fondesse mostrerebbe
	 * una sola unita' dove ce ne sono due, e nessun errore lo direbbe. Chi disegna deve poter scrivere su
	 * quale piano sta ragionando, e questa e' la risposta.
	 */
	TArray<int32> LayersInUse(const TArray<FRTScenarioUnitView>& Units);

	/** Forma leggibile di `LayersInUse`: `L0` · `L0, L1` · `nessun layer` se non ci sono unita'. */
	FString DescribeLayers(const TArray<int32>& Layers);

	/**
	 * Il raggio dell'anello a terra che distingue una squadra, nella scala ASSOLUTA di `ARTUnit::TeamRing`.
	 *
	 * 🔴 **Distingue per DIMENSIONE e non per colore, e non e' una preferenza estetica.** Il kit graybox
	 * esce con lo **slot materiale vuoto** (#1714): un colore per squadra oggi non arriverebbe a schermo, e
	 * il viewport mostrerebbe due squadre identiche restando verde in ogni test. Una differenza di raggio si
	 * legge senza materiali. Quando #1714 chiude, il colore si **aggiunge** a questa — non la sostituisce:
	 * una lettura che dipende dal solo colore esclude chi non lo distingue.
	 *
	 * `1.6` per la squadra `0` e' il valore di `ARTUnit::TeamRing`, riusato perche' il marcatore di
	 * authoring e l'unita' in partita si somiglino invece di essere due convenzioni.
	 *
	 * ⚠️ Il passo e' limitato a quattro squadre: v0.1 ne ha due, ma il 4v4 e' *un cambio di dato*. Oltre la
	 * quarta il raggio si ferma invece di crescere fino a invadere le celle vicine.
	 */
	float TeamRingScale(int32 TeamId);

	/** Scala del raggio piu' grande che `TeamRingScale` puo' restituire. Il test la confronta col passo della griglia. */
	float MaxTeamRingScale();

	/**
	 * Il colore con cui il CORPO di una unita' dichiara la propria squadra (#3104).
	 *
	 * 🔴 **Esiste perche' la seduta `U44` del 2026-09-12 e' stata giudicata ❌**, verbatim: *«si vedono dei
	 * cilindri grigi che si muovono nell'editor, non si capisce»*. L'anteprima posava per ogni unita' istanze
	 * di mesh engine **senza alcun materiale**, e l'unica cosa che distingueva due squadre era il RAGGIO
	 * dell'anello: chi guardava non aveva modo di sapere da che parte stesse cio' che si muoveva.
	 *
	 * ⛔ **La regola id -> colore non nasce qui**: la possiede `ARTUnit::TeamColorFor`, che il HUD gia' riusa
	 * (`RTHudViewModel.cpp:235`). Questa funzione sceglie la COPPIA e delega a quella. Una seconda tabella
	 * dentro il modulo Editor divergerebbe dal HUD al primo cambio, in silenzio.
	 *
	 * ⚠️ **Assoluto per `TeamId`, non relativo alla prospettiva selezionata**, ed e' una decisione, non un
	 * default: il HUD colora alleato/nemico perche' in partita esiste un «chi guarda»; il Tactical Designer
	 * e' uno strumento d'authoring il cui default e' `Omniscient`, dove quel soggetto non esiste. E' anche la
	 * convenzione che il pannello gia' applica — `DescribePerspective` numera le posizioni con l'**id della
	 * squadra**, non con la posizione nel selettore, per poterle confrontare con `rt.Debug.Knowledge <team>`.
	 *
	 * ⚠️ **Oltre l'ultima squadra distinta si FERMA invece di riavvolgersi**, esattamente come
	 * `TeamRingScale` e per la stessa ragione scritta li': due squadre che condividono l'ultimo colore sono
	 * leggibili, due che se lo scambiano no.
	 */
	FLinearColor TeamBodyColor(int32 TeamId);

	/**
	 * Dove sta e come e' ruotato il pannello che segna un lato ESPOSTO del confine visibile (#1754).
	 *
	 * 🔑 **Il punto e l'orientamento si CHIEDONO alla libreria** — `URTHexLibrary::EdgeMidpointWorld` e
	 * `EdgeRotation`, che li derivano dai due centri di cella — esattamente come fanno i pannelli di
	 * copertura e porta in `ARTHexMapActor`: se la convenzione dei sei lati cambiasse, la geometria
	 * seguirebbe invece di mentire. E' la stessa disciplina di `MarkerTransform` per il facing.
	 *
	 * La quota e' `RTCellTopZ` sopra il bordo: sotto, il pannello finirebbe **dentro** il prisma della cella
	 * e a schermo sarebbe indistinguibile da un pannello mai disegnato — il difetto che `RTMapVisuals.h`
	 * documenta essere gia' costato due volte.
	 *
	 * La scala e' quella del cubo engine da 100 uu: sottile lungo X (spessore), lungo il bordo su Y.
	 */
	FTransform BorderEdgeTransform(const FRTCellId& Cell, ERTHexDirection Dir,
		const FVector& Origin, float HexSize, float LayerHeight);
}
