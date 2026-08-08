# Combattimento e targeting

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-UI-WARNINGS -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-UI-WARNINGS` · Release: `v0.1` · Roadmap: `E11.1, E11.2`  
> Stato: **IMPLEMENTING** · Gate: `3/7`  
> Scenario: `Combat.FriendlyFire`  
> I pezzi che mancano li porta: `RT-FEAT-UI-CERTAINTY`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-UI-WARNINGS -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-LOS -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-LOS` · Release: `v0.1` · Roadmap: `E2.4`  
> Stato: **RELEASE_READY** · Gate: `6/7`  
> Scenario: `Combat.BlockedByWall`  
> Vista e targeting esistono; la **percezione** completa (chi sa cosa, E13) e' un altro sistema e non c'e' ancora.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-LOS -->

## Il combattimento è legato alla fase

Un attacco non avviene quando lo selezioni: avviene nella **fase di Resolution dichiarata dall'azione**. La maggior parte degli attacchi offensivi della v0.1 risolve nel Blast.

## Forme di attacco

Le azioni possono usare forme diverse, per esempio:

- bersaglio singolo;
- linea;
- area circolare;
- direzione;
- self.

La forma decide quali celle o unità possono essere coinvolte.

## Linea di vista e coperture

La mappa può bloccare visione e tiro. Le coperture sono **direzionali**: conta da quale lato arriva l'attacco.

La copertura alta può bloccare passaggio, linea di vista e proiettili attraverso il relativo bordo. La copertura bassa protegge senza trasformarsi automaticamente in occultamento completo.

## Target che cambia posizione

Un'azione deve avere una politica chiara quando il bersaglio non si trova più dove previsto. Il sistema usa fallback espliciti invece di inventare una correzione nascosta all'ultimo momento.

Esempi di risultato possibile:

- l'azione colpisce la cella pianificata;
- l'azione viene cancellata;
- il percorso viene fermato all'ultima cella valida;
- una specifica ability può seguire un target se le sue regole lo dichiarano.

## Ordine dentro una fase

Le azioni sono ordinate in modo deterministico usando macro-fase, priorità intera, ID dell'azione, unità sorgente e sequenza evento. Il risultato non deve dipendere dall'ordine casuale di un container.

## Fonti normative

- `docs/gameplay/spec-sequenza-turno.md`
- `docs/balance/RT_ActionCatalog_v0.1.md`
- `docs/gameplay/spec-copertura-cp91.md`
- `docs/gameplay/spec-copertura-alta-cp92.md`
