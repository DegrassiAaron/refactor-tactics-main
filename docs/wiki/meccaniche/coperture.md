# Coperture

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-DYNAMIC-COVER -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-DYNAMIC-COVER` · Release: `v0.1` · Roadmap: `E9.5`  
> Stato: **IMPLEMENTING** · Gate: `2/8`  
> Scenario: `Spec.Cover.TemporaryCoverExpires`  
> La **distruzione** della copertura esiste; la **creazione temporanea** (CP 9.5) no.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-DYNAMIC-COVER -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-COVER -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-COVER` · Release: `v0.1` · Roadmap: `E9.1, E9.2`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Map.LowCoverEdge`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-COVER -->

## In breve

Le coperture vivono sui **bordi degli esagoni**, non al centro della cella. Per questo sono direzionali: conta da quale lato arriva un colpo o un movimento.

RefactorTactics distingue due categorie principali:

| Tipo | Effetto |
|---|---|
| **Low Cover** | riduce di **10** il danno diretto che attraversa il bordo protetto |
| **High Cover** | blocca **vista, movimento e proiettili** attraverso quel bordo |

## Copertura bassa

La Low Cover protegge solo dal lato corretto. Se il colpo arriva da un altro bordo, non applica la riduzione.

```text
Attaccante → [ bordo con Low Cover ] → Bersaglio
                                   = -10 danni diretti
```

Un attacco ad area non viene trattato come un proiettile che attraversa il bordo: la Low Cover non riduce automaticamente il danno AoE.

### Integrità

La copertura bassa ha **integrità 30** e può essere distrutta dal sistema di danno alle strutture.

## Copertura alta

La High Cover è una barriera fisica:

- blocca il passaggio;
- blocca la linea di vista;
- blocca i proiettili;
- ha **integrità 50**;
- quando viene distrutta, il bordo torna utilizzabile dalla fase successiva.

Il danno alla struttura viene raccolto durante la fase e applicato insieme a fine fase: due colpi simultanei non producono un risultato diverso in base all'ordine interno con cui sono stati iterati.

## Facing e copertura

La direzione della copertura e il facing dell'unità sono **due cose diverse**:

- la copertura appartiene al bordo della mappa;
- il facing appartiene all'unità.

La regola di facing decisa per una versione successiva della v0.1 prevede che un attacco fuori dall'arco frontale possa annullare la protezione della Low Cover. Questa parte è **decisa ma non ancora implementata**.

## Intercept

Se un attacco viene rediretto da una reazione come `Intercept`, la geometria difensiva deve essere rivalidata sul **bersaglio effettivo**: LOS, copertura e facing non restano quelli della vittima originale.

## Cosa deve ricordare il giocatore

- Low Cover = **mitigazione direzionale**, non muro.
- High Cover = **barriera vera**.
- Girarsi non sposta una copertura.
- Distruggere una barriera può cambiare LOS e pathfinding per le fasi successive.

## Fonti normative

- `docs/gameplay/spec-copertura-cp91.md`
- `docs/gameplay/spec-copertura-alta-cp92.md`
- `docs/decisions/adr-0005-orientamento.md`
