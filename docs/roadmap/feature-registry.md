# Feature Registry — owner del modello

> `CURRENT` · **Owner** del modello di tracciabilità delle feature. **Ultimo aggiornamento**: 2026-08-08.
> I dati vivono in [`feature-registry.yaml`](feature-registry.yaml); qui c'è **cosa significano** e
> **come si usano**. Lo stato di esecuzione resta in [`roadmap-checkpoint.md`](roadmap-checkpoint.md),
> lo scope di release in [`roadmap-v0.1.md`](roadmap-v0.1.md).

## 1. Il problema che risolve

Il repository ha già pagato quattro volte lo stesso difetto: **un numero scritto a mano in due viste
diverge**, e la seconda copia diventa una bugia con la data sbagliata. È successo col conteggio dei
test (172 → 179 → 324 → due valori entrambi corretti alla propria base al merge di CP 9.3) ed è
successo con lo stato delle epic (M8/M9 dichiarate ⏳ mentre E4–E6 erano chiuse).

La Wiki è il posto dove il difetto costa di più, perché è la vista che leggono gli altri: oggi
`Meccanica-overwatch.md` porta in testa un blocco `> **Stato v0.1:** …` scritto a mano, e nessuno lo
rigenera quando il codice cambia.

Il registry esiste perché **lo stato si aggiorni una volta sola**:

```text
FEATURE ID → SPEC → ROADMAP → ISSUE → CODE → TEST → SCENARIO → WIKI
```

Tutte le viste leggono; nessuna copia.

## 2. Cosa è e cosa non è

| È | Non è |
|---|---|
| La vista di **prodotto**: quali capacità esistono e a che punto sono | Una roadmap: l'ordine del lavoro resta nelle due roadmap |
| Un indice stabile di `feature_id` | Un elenco di issue: le issue si spezzano, gli id no |
| Un file **verificato da una macchina** | Un riassunto discorsivo |

Una feature può attraversare più checkpoint; una epic può implementare più feature. I due assi non
si comprimono in una lista sola.

## 3. Feature ID

`RT-FEAT-<AREA>-<NOME>`, stabile e indipendente dal titolo visuale. **Non si usa il numero di una
issue come id**: le issue cambiano, si spezzano, si chiudono per errore e si riaprono.

Rinominare un `feature_id` è un'operazione con costo: rompe i blocchi generati nelle pagine e i
riferimenti del workbook. Se serve, si rinomina e si rigenera tutto nello stesso commit.

## 4. I gate

| Gate | `done` quando |
|---|---|
| `spec` | Un documento owner descrive le regole e dichiara chi le possiede |
| `data` | Cataloghi, data asset e config sono coerenti con quelle regole |
| `runtime` | L'implementazione esiste in `Source/` e **qualcuno la consuma** |
| `log_debug` | L'esito è osservabile: TurnLog, reason code, comando di debug |
| `automation` | Test automatici pertinenti, verdi, che chiamano il gameplay reale |
| `scenario` | Almeno uno scenario in `Scenarios/` la **dimostra** — vedi la nota qui sotto |
| `ui_wiki` | È spiegata all'utente **e** leggibile in gioco — servono entrambe |
| `packaged` | Verificata nella build packaged **della release corrente** |
| `network_privacy` | Corretta in rete: autorità server e nessuna fuga di intento |

Valori: `done` · `partial` · `todo` · `na`.

Tre regole che vale la pena scrivere, perché sono i modi in cui questo schema si corrompe:

1. **`na` è una risposta, `partial` no.** Una feature offline dichiara `network_privacy: na`; una
   feature che *dovrà* funzionare online dichiara `todo`, anche se oggi non è verificabile. Marcare
   `done` una privacy che è banale solo perché il gioco è offline è il modo più facile di mentire.
2. **`runtime: done` richiede un consumatore.** Il difetto ricorrente di questo progetto non è la
   formula sbagliata, è il dato che nessuno legge: `Marked` a catalogo senza codice che lo
   interroghi, `Effects` vuoto nelle reazioni d'eroe. Un dato senza consumatore è `partial`.
3. **`packaged` guarda la release corrente.** Il packaging di M7.4 (2026-08-06) ha verificato la
   partita, ma **precede** E8, E9 e gli scenari: non copre ciò che è arrivato dopo. Finché la
   release interna E12.5 non lo rifà, nessuna feature di gameplay dichiara `packaged: done`.
4. **Uno scenario che esce `BLOCKED` non è `scenario: done`.** Il corpus contiene scenari `Spec.*`
   scritti **prima** della capability — sono specifiche eseguibili, e lo dichiarano nelle proprie
   note: «esce BLOCKED finché `DecisionBoundary` non esiste, e va bene». Il loro valore è che la
   feature ha una forma eseguibile prima di essere costruita, e che il giorno in cui atterra si
   accendono da soli. Ma descrivere non è dimostrare: il gate resta `partial` finché lo scenario
   non passa davvero.

### 4.1 Quando un pezzo lo porta un'altra feature

`completed_by:` dichiara **chi** completa ciò che a questa feature manca.

Nasce da un difetto trovato nella revisione di granularità del 2026-08-08:
`RT-FEAT-ACTION-GENERIC` aveva quattro gate su otto deformati da due mancanze —
`Overwatch` e `Interact` — che appartengono ad **altre due feature** già tracciate (E14.4 ed
E10.1). La stessa mancanza era contata tre volte, e chiudere E14 avrebbe richiesto di ricordarsi
di aggiornare due righe: esattamente il meccanismo che questo registry esiste per eliminare.

```yaml
completed_by:
  - RT-FEAT-REACTION-OVERWATCH
  - RT-FEAT-OBJECTIVE-SYSTEM
```

I gate **restano** aperti — la feature davvero non è completa — ma il rimando è un dato, non una
frase nelle note: compare nella §2.2 della roadmap e nei blocchi Wiki, e il validator verifica che
gli id esistano. Se una feature dichiara `completed_by` senza avere gate aperti, il validator
avvisa: il rimando è vecchio.

**Non è `dependencies`.** Una dipendenza è un prerequisito — ciò che deve esistere *prima*.
`completed_by` è il contrario: ciò che arriverà *dopo* e chiuderà il buco.

## 5. Lo stato è derivato, non dichiarato

`status` **non è un giudizio**: è una funzione dei gate, e il validator la verifica. Se i due
divergono, `validate` esce con errore — in entrambe le direzioni, anche quando i gate dicono che la
feature è più avanti di quanto scritto.

```text
DONE            core + scenario + ui_wiki + packaged + network_privacy
RELEASE_READY   core + scenario + ui_wiki
INTEGRATED      core + scenario
TESTABLE        spec + runtime + automation
IMPLEMENTING    runtime done o partial
SPECIFIED       spec done
DESIGNED        spec partial
IDEA            nient'altro
```

dove `core` = `spec` + `data` + `runtime` + `log_debug` + `automation`, e un gate `na` conta come
soddisfatto.

`DEFERRED` e `BLOCKED` sono **fuori scala**: dichiarano una decisione, non un grado di
completezza, e il controllo di coerenza li salta. Vanno usati con una `notes` che dica chi ha deciso.

**Niente percentuali.** Il progresso è «`6/8` gate», che si verifica; «73%» si contratta.

## 6. Comandi

```bash
python scripts/feature_registry.py validate    # gate: esce 1 se ci sono errori
python scripts/feature_registry.py generate    # riscrive feature-registry.json
python scripts/feature_registry.py wiki        # blocchi di stato + pagina Stato delle feature
                                               #   + tabella §2.2 di roadmap-v0.1.md
python scripts/feature_registry.py workbook    # sheet 15_Wiki_Feature_Refs del workbook character
python scripts/feature_registry.py report      # tabella di audit su stdout
```

`generate`, `wiki` e `workbook` accettano `--check`: non scrivono e falliscono se l'output è
disallineato dalla sorgente. È la forma da usare in un gate automatico (**G15** della Definition of
Done).

### Deploy verso il clone della Wiki

```bash
python scripts/feature_registry.py deploy --wiki-root <path>          # sola lettura: elenca
python scripts/feature_registry.py deploy --wiki-root <path> --write  # scrive davvero
```

La Wiki di GitHub è un **repository separato** (`refactor-tactics-main.wiki`, branch `master`, file
flat) e **pubblico**. Per questo `deploy` è in sola lettura di default: scrivere lì è un'azione che si
chiede, non che si assume — tanto più che altre sessioni possono avere quel clone in lavorazione.

La corrispondenza segue le convenzioni già in uso nel clone:

| Sorgente | Pagina Wiki |
|---|---|
| `docs/wiki/game/<x>.md` | `<x>.md` |
| `docs/wiki/meccaniche/<x>.md` | `Meccanica-<x>.md` |
| `docs/wiki/fazioni/<x>.md` | `Fazione-<X>.md` (`index` → `Fazioni.md`) |
| `docs/characters/v0.1\|v0.2/<x>.md` | `Personaggio-<x>.md` |
| `docs/wiki/feature-status.md` | `Stato-delle-feature.md` |

`validate --wiki-root <path>` verifica anche i riferimenti nella forma `wiki:<PageName>`, quelli per
le pagine che vivono **solo** nel clone e non hanno una sorgente nel repository.

Errori (bloccanti): id duplicato o malformato · epic, milestone o checkpoint inesistenti ·
ScenarioId inesistente · owner spec inesistente · riferimento a un test che la suite non ha ·
dipendenza verso un id inesistente · valori fuori dominio · `status` che diverge dai gate ·
`last_verified` assente per stati `TESTABLE` o superiori · blocco generato per un id non nel registry.

Warning (da vedere, non bloccanti): feature senza pagina Wiki · feature di gameplay testabile senza
scenario che la dimostri · `SPECIFIED` senza issue né assegnazione · scenari dichiarati `planned`.

## 7. Blocchi di stato nelle pagine

Ogni pagina referenziata da `wiki_refs` riceve un blocco delimitato:

```markdown
<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-REACTION-OVERWATCH -->
...
<!-- RT_FEATURE_STATUS:END RT-FEAT-REACTION-OVERWATCH -->
```

**Non si edita a mano**: `wiki` lo riscrive. Una pagina può averne più d'uno (la pagina di Flux ne ha
tre: roster, elettricità, interazioni sistemiche). Il blocco va dopo il titolo e la citazione
introduttiva, prima del corpo.

Un blocco che cita un `feature_id` non più nel registry è un **errore**; un blocco per una feature
che il registry non referenzia più su quella pagina è un **warning** (`wiki` lo segnala, non lo
rimuove: cancellare testo da una pagina è una decisione dell'autore).

## 8. Come si aggiunge una feature

1. Scegli un `feature_id` che descriva la **capacità**, non l'implementazione né il titolo della
   pagina.
2. Compila i gate **guardando il repository**, non la memoria: `grep` nel codice, nomi dei test,
   `Scenarios/`, la pagina Wiki.
3. Lascia che `status` cada dove i gate lo mandano. Se ti sembra sbagliato, il difetto è nei gate.
4. Metti `last_verified` con la data e il commit su cui hai guardato.
5. `validate` → `generate` → `wiki` → `workbook`, nello stesso commit.

Una feature che nessun documento definisce resta `IDEA` con `owner_specs: []`. Registrarla è utile —
significa che qualcuno l'ha pensata — ma promuoverla senza una fonte è il modo in cui una seed list
diventa un impegno che nessuno ha preso.

## 9. Rapporto con gli altri documenti

| Documento | Ruolo |
|---|---|
| [`feature-registry.yaml`](feature-registry.yaml) | **Sorgente** dello stato: si edita qui |
| `feature-registry.json` | **Generato**: consumatori automatici, mai a mano |
| [`roadmap-checkpoint.md`](roadmap-checkpoint.md) | Vista di **esecuzione**: milestone e checkpoint |
| [`roadmap-v0.1.md`](roadmap-v0.1.md) | Vista di **release**: epic, scope, gate della v0.1 |
| [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) | Gate `G1`–`G15` della release (`G15` è questo registry) |
| [`v0.1-issue-plan.md`](v0.1-issue-plan.md) | **Snapshot** delle issue: non è fonte di verità |
| `docs/wiki/feature-status.md` | **Generata**: vista pubblica del registry |
| `docs/characters/data/*.xlsx` → `15_Wiki_Feature_Refs` | **Generata**: riferimenti entità → feature, senza stato |

Il workbook di bilanciamento (`docs/balance/*.xlsx`) **non** entra in questa catena: contiene target e
metodi di misura, non stato di implementazione. Non c'era niente da deduplicare.
