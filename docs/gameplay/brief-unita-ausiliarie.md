# Brief — Unità ausiliarie: il concetto, non il gameplay

> **Stato**: brief di scope · **Data**: 2026-08-07 · **Origine**: `/sc:brainstorm` su
> [`../archive/src/design/auxiliary-units.md`](../archive/src/design/auxiliary-units.md) (28 §)
> **Decisione**: `OD-2` chiusa — **concetto unico, gameplay fuori dalla v0.1**.
> **Cosa è**: l'elenco dei vincoli da rispettare **mentre si costruisce altro**, perché costano quasi nulla ora
> e molto dopo.
> **Cosa non è**: una epic. Nessun drone, nessuna torretta, nessun pet entra nella v0.1.
> **Autorità**: subordinato a [`../product/piano-canonico-mvp.md`](../product/piano-canonico-mvp.md).

## 1. Perché un brief per qualcosa che non si costruisce

Il documento sorgente descrive sette famiglie — pet, drone, torretta, summon, gadget, construct, decoy — e
chiede di trattarle come **un concetto solo**, `AuxiliaryUnit`, data-driven.

La parte di gameplay è chiaramente fuori scope: il canone lo dice già («la v0.1 deve usare questi sistemi solo
se necessari al roster definitivo», sorgente §19) e il roster definitivo — Flux, Riva, Bastion, Vektor — non ne
ha bisogno.

Ma c'è una parte che **non si può rimandare**, ed è la sola ragione di questo file: l'assunzione
*«ogni unità in campo è uno dei quattro eroi»* si insinua nei tipi senza che nessuno la scriva. E le epic che
stanno per introdurre tipi nuovi sono **E13** (conoscenza), **E14** (reazioni) ed **E16** (orientamento):
percezione, reazioni e facing sono esattamente i sistemi che un'ausiliaria deve poter usare.

Scoprirlo dopo significa riaprire tre epic chiuse. Scriverlo ora costa questo documento.

## 2. I vincoli — da rispettare in E13, E14, E16

| # | Vincolo | Come si viola, in pratica |
|---|---|---|
| **A1** | Un'unità è identificata da uno `StableUnitId`, **mai** da un `HeroId` | `TMap<FName /*HeroId*/, ...>` come chiave di stato runtime |
| **A2** | Ogni unità ha **owner** e **team** espliciti; owner ≠ team | assumere che l'owner di un'unità sia l'unità stessa |
| **A3** | Le firme che accettano «un'unità» accettano **un'unità qualsiasi**, non un eroe | `void Foo(const URTHeroData* Hero)` dove basterebbe l'id o lo stato |
| **A4** | Il numero di unità per squadra **non è una costante**: né `2`, né `3`, né `4`. Chi itera, itera sullo stato | `for (int i = 0; i < 2; ++i)`, o `Team0Heroes.Num()` usato come verità |
| **A5** | Occupazione, visibilità, rumore e TurnLog non fanno eccezioni per tipo | un ramo `if (IsHero)` in un resolver |
| **A6** | **Nessun simulatore parallelo**: un'ausiliaria passa dallo stesso snapshot/resolver/TurnLog | un `Tick` o un componente che muove l'unità fuori dal resolver |

> A4 ha già un'occorrenza da tenere d'occhio: `ARTGameMode::Team0Heroes`/`Team1Heroes` sono `TArray<FName>` con
> **due** elementi di default. È il posto giusto per la composizione, ma non deve diventare il posto da cui
> altri sistemi deducono «quante unità ci sono».
>
> **Il vincolo vale in entrambe le direzioni**: `TeamSize == 2` è l'assunzione ovvia da evitare, ma `== 3` e
> `== 4` lo sono altrettanto. Un'unità ausiliaria aggiunge unità **fuori dal roster**, quindi rompe anche
> l'idea che il numero coincida con la dimensione della squadra. L'epic **E17** (stress 4v4) serve proprio a
> **scoprire questi hard-code** prima che lo faccia un giocatore: se un `if (Num == 2)` sopravvive da qualche
> parte, è lì che si manifesta.

## 3. La baseline, quando il tema entrerà davvero

Registrata perché non vada persa, **non** perché sia in scope (sorgente §7):

```text
Max attive per proprietario:  1
Abilità proprie:              0
Comandi per turno:            0..1
Ready indipendente:           NO
Timer di planning proprio:    NO
Cattura obiettivo:            NO
Fast Reaction manuale:        NO
StableUnitId · Snapshot · TurnLog:  SÌ
```

Regola di action economy del sorgente, che è ciò che rende il tema pericoloso per il bilanciamento:

> Un'unità ausiliaria non concede normalmente **un secondo turno completo**.

Primi prototipi suggeriti quando il tema si apre: **Scout Drone** (HP 30, Move 3, nessun attacco, occupazione
non bloccante — valida percezione, path, TeamKnowledge, ownership, rumore) e **Turret** (riusa l'Overwatch con
policy `Automatic`, quindi **nessuna** Fast Reaction manuale: vedi [D-012](../decisions/RT_PDR_00_Decision_Log.md)).

## 4. Fuori scope, dichiarato

Tutta la §4 del sorgente: pet, summon, gadget, construct, decoy, proiezioni · comandi `FOLLOW`/`GUARD`/`COMMAND` ·
seconda barra delle abilità · interazione con porte e dispositivi · spawn/despawn · UI dedicata · validator ·
Gameplay Tag propri · le 8 issue proposte in §27.

## 5. Rapporto con gli altri documenti

| Documento | Relazione |
|---|---|
| [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md) | E13 introduce i tipi della percezione: **A1, A3, A5** si applicano lì per primi |
| [`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md) | E14: una torretta è un consumatore di `Automatic`, non un sistema parallelo (**A6**) |
| [ADR-0005](../decisions/adr-0005-orientamento.md) | E16: il facing è dell'unità, non dell'eroe (**A3**) |
| [`../archive/src/design/auxiliary-units.md`](../archive/src/design/auxiliary-units.md) | Sorgente. Resta non normativo |
