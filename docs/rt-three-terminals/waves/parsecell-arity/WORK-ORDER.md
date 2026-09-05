# Work order — `ParseCell`: arità non limitata e ramo di rifiuto non misurato

Ingresso della wave `parsecell-arity/1`. È il file che `RT3_CONTRACT.md` §4 richiede come `INPUT_HANDOFF`: un artefatto rileggibile, non testo incollato.

## Origine

Issue [#2482](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2482) — `bug` · `v0.1` · `P3`.

Misurata su `origin/main` = `5ee69775`. Questa wave parte da `39f3ec95`.

## Difetto

`ParseCell` (`Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp:48`) è l'unico ingresso delle coordinate negli scenari, con otto chiamanti. La guardia è solo sul basso:

```cpp
if (!Arr || Arr->Num() < 2) { /* errore */ }
Out = FRTCellId((*Arr)[0]->AsNumber(),
                (*Arr)[1]->AsNumber(),
                Arr->Num() >= 3 ? (*Arr)[2]->AsNumber() : 0);
```

Due conseguenze che nessuno ha deciso:

- **arità non limitata** — `[q, r, layer, 7]` è accettato e gli elementi in eccesso sono scartati in silenzio. Un array più lungo del previsto è quasi sempre un errore di battitura o una coordinata cubica scritta per abitudine: oggi produce una cella **valida e sbagliata**, non un errore;
- **layer implicito** — `[q, r]` vale `layer = 0`. Il corpus non esercita mai quel ramo (532 occorrenze, tutte a tre elementi) e il writer ne scrive sempre tre.

Il ramo d'errore non ha test: la stringa del messaggio compare una volta sola in tutto `Source/`, nel punto che la emette.

Perché conta: uno scenario è **autorevole** per il gate. Una cella accettata al posto sbagliato non produce un rosso — produce un **verde su un'altra partita**.

## Contratto richiesto

Un array di coordinate che non ha la lunghezza dichiarata dal formato è un errore di caricamento **con messaggio**, non una cella dedotta.

## Scope

- limite superiore sull'arità, con messaggio che nomina la lunghezza trovata e quella attesa;
- una decisione esplicita sulla forma a due elementi, scritta nel codice e coperta da un test;
- i test del ramo d'errore, che oggi non esistono.

## Fuori scope

- ⛔ migrazione a coordinate nominate `{"q":…}` — è una decisione, non una correzione;
- ⛔ bump di `version` dello scenario;
- ⛔ validazione semantica della cella (layer esistente, cella in mappa): è #1796, chiusa.

## Acceptance criteria

1. `ParseCell` rifiuta un array con più di tre elementi, e il messaggio nomina la lunghezza trovata.
2. L'uscita scelta per l'array a due elementi è scritta nel codice e coperta da un test.
3. Un Automation Test carica uno scenario con `"cell": [0, 0, 0, 0]` e verifica che il caricamento fallisca, **leggendo il messaggio** e non solo il bool.
4. Anti-vacuità: rimuovendo il limite superiore quel test diventa rosso, **verificato per mutazione**.
5. La copertura vale per tutti e otto i chiamanti, o il test dice quali esercita e perché gli altri sono la stessa funzione.
6. `git grep -c '"cell"' Scenarios/` resta invariato: nessun file del corpus modificato.
