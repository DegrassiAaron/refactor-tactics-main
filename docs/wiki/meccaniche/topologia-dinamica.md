# Topologia dinamica

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-SPECIAL-TRANSITIONS -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-SPECIAL-TRANSITIONS` · Release: `v0.1` · Roadmap: `E9.4`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Map.MultiLevel`  
> Verificato il `2026-08-09` su `4a3fd20`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-SPECIAL-TRANSITIONS -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-INTERACTIVE-EDGES -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-INTERACTIVE-EDGES` · Release: `v0.1` · Roadmap: `E9.3`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Map.ClosedDoor`  
> Dato, regole di blocco e mutazione del bordo sono implementati e testati; l'**interazione del giocatore** (`Interact`/`Activate`) arriva con E10.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-INTERACTIVE-EDGES -->

## Che cosa significa

In RefactorTactics la mappa non è necessariamente identica fra Planning e Move. Porte, barriere e ponti possono cambiare durante la Resolution.

La topologia del grafo è quindi **stato di gioco**.

## Tre oggetti, tre significati

### High Cover / muro

È una barriera sul bordo. Nega passaggio, vista e proiettili finché resta in piedi.

### Porta

È una barriera **reversibile** sul bordo. Chiusa toglie un passaggio planare; aperta lo restituisce.

### Ponte

È un arco **additivo**. Attivo crea una connessione; distrutto la elimina.

## Quando cambia la mappa

Le modifiche strutturali del Blast vengono raccolte e applicate a fase conclusa. Il Move che segue usa la topologia aggiornata.

```text
Planning: pianifico il percorso
        ↓
Blast: una porta si chiude / un muro cade / un ponte viene rimosso
        ↓
Move: il percorso viene validato contro la mappa nuova
```

## Niente path fantasma

Prima di eseguire il Move, ogni passo del percorso viene confrontato con il grafo corrente. Se una transizione non esiste più, il percorso viene troncato.

Il personaggio si ferma nell'ultima cella valida: il gioco **non** attraversa una porta chiusa e **non** cammina su un ponte distrutto soltanto perché il percorso era stato pianificato prima.

## Revisione della mappa

Ogni modifica topologica incrementa una revisione/hash usata dal simulatore per accorgersi che snapshot e cache non descrivono più la stessa mappa.

Per il giocatore la conseguenza è semplice: **quello che vedi cambiare sulla mappa deve cambiare anche nelle regole di percorso e tiro**.

## Collegamenti

- [Coperture](coperture.md)
- [Porte](porte.md)
- [Ponti](ponti.md)
- [Collisioni](collisioni.md)

## Fonti normative

- `docs/gameplay/spec-copertura-alta-cp92.md`
- `docs/gameplay/spec-porte-cp93.md`
- `docs/gameplay/spec-ponti-cp94.md`
