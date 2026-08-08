# Collisioni e movimento simultaneo

> **Stato v0.1:** implementato e testato
> **Tipo:** guida giocatore, non normativa

## In breve

Il Move viene risolto a **micro-step simultanei**. Non esiste una regola nascosta del tipo «vince chi viene processato prima».

La regola base è **massimo una unità viva per cella**.

## Destinazione contesa

Se due o più unità cercano di entrare nella **stessa cella nello stesso micro-step**, nessuna delle contendenti ottiene la cella.

```text
A → X ← B

Risultato: A e B restano prima di X
Outcome: BlockedContested
```

## Unità ferma

Se una unità prova a entrare nella cella occupata da un'unità che non la sta liberando, viene bloccata.

```text
A → [B fermo]

A si ferma
Outcome: BlockedByUnit
```

## Scambio diretto

Uno **swap diretto A↔B è consentito** quando entrambe le unità si muovono simultaneamente nelle rispettive celle.

```text
A → B
B → A

Risultato: scambio consentito
```

Questa regola evita che l'ordine arbitrario degli array scelga chi passa per primo.

## Movimento a catena

La risoluzione usa un punto fisso monotono: se una unità viene fermata e quindi non libera più la propria cella, questo può bloccare chi tentava di entrarci nello stesso micro-step. Il risultato viene calcolato fino a stabilizzarsi.

## Pianificazione e occupazione

Durante il pathfinding una cella occupata da un'altra unità viene normalmente trattata come ostacolo. La Resolution, però, deve anche gestire i casi in cui più movimenti simultanei cambiano l'occupazione nello stesso istante: da qui le regole sopra.

## Indipendenza dall'ordine

Permutare l'ordine con cui gli intenti arrivano al resolver non deve cambiare il risultato finale per unità.

## Cosa deve ricordare il giocatore

- Una cella contesa non viene assegnata “al più veloce” per caso.
- Una unità ferma è un ostacolo solido.
- Lo swap diretto è legale.
- Il gioco risolve **tutti** i movimenti dello stesso micro-step insieme.

## Fonti normative

- `docs/technical/h6-hex-sim-spec.md`
- `Source/RefactorTactics/Tests/RTHexSimTests.cpp`
