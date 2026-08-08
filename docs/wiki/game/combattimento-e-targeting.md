# Combattimento e targeting

> **Stato nel gioco:** core di combattimento e targeting esistenti; percezione completa ancora in sviluppo
> **Tipo:** guida giocatore, non normativa

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
