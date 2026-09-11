# Sedute componibili — la seduta smette di essere un dato e diventa un calcolo

> `DESIGN` · **Data**: 2026-09-11 · **Owner del documento**: questo file, fino all'accettazione della
> decisione di §10.
>
> 🔑 **Convenzione sui numeri, dichiarata una volta.** Ogni conteggio qui sotto porta accanto il comando
> che lo produce ed è un **esito del passaggio corrente**, non un totale da mantenere. Chi rilegge
> **rimisura**, non aggiorna a mente. Dove un'enumerazione è possibile, il documento usa i **nomi**.

---

## 1. Il problema, misurato

Lo stesso lavoro — *quale verifica interattiva, con quale allestimento, in che ordine* — vive oggi in
cinque posti, e nessuno dei cinque possiede gli altri.

| Rappresentazione | Cosa contiene |
|---|---|
| `docs/roadmap/editor-sessions.yaml` | le sedute `U*` |
| `plans/piano-sessioni-pie-consolidato-2026-09-10.md` | le fasi `S0`–`S9`, marcate `CURRENT` |
| `plans/roadmap-esecuzione-pie-2026-09-03.md` | citato da `test-manuali-pie.md` come owner della *sequenza* |
| `docs/technical/test-manuali-pie.md` | un raggruppamento per allestimento: `A`, `B`, `C`, `D` |
| `docs/technical/runbooks/guida-seduta-*.md` | le guide per seduta, agganciate a mano |

Più il documento Drive `RefactorTactics_v0.1_Editor_Sessions_Roadmap`, sezione `CURRENT v2`, che lo yaml
cita come piano operativo corrente.

### 1.1 Nessuno legge il registro delle sedute

Lo yaml lo dichiara da sé nell'intestazione: `editormap.shortlist.md` e il suo generatore sono usciti dal
repository con [D-181], e `scripts/` con [D-182]. Chi aggiunge una seduta scrive in un file che nessuno
rende.

### 1.2 La tassonomia è collassata

```bash
python -c "import yaml,collections;d=yaml.safe_load(open('docs/roadmap/editor-sessions.yaml',encoding='utf-8'));print(collections.Counter(r.get('block') for r in d['sessions']))"
```

Al 2026-09-11 il blocco `6` contiene **34** sedute su **53**. Un campo che raccoglie due terzi della
popolazione in un valore non raggruppa più niente.

### 1.3 `shares_setup_with` non è un'equivalenza, ma viene usato come se lo fosse

La chiusura transitiva della relazione dà **30** componenti, **23** delle quali singoletti. La più grande
ne fonde tredici — `U21 U22 U25 U26 U31 U32 U33 U34 U35 U36 U38 U39 U50` — mescolando il graybox di
`L_DevSandbox`, il `L_GrayKitPlayground` e il pannello Tactical Designer, che allestimenti diversi sono.
La relazione è dichiarata a coppie, nessuno la verifica, e la sua chiusura produce gruppi che nessun
autore ha scelto.

### 1.4 Cinque voci PIE hanno due padroni

`PIE-HEXPLAY-6` → `U4`,`U46` · `PIE-HEXPLAY-8` → `U6`,`U46` · `PIE-V01-LOG` → `U15`,`U46` ·
`PIE-V01-ROSTER` → `U11`,`U46` · `PIE-TD-CLEAN` → `U31`,`U32`.

Quattro su cinque puntano a `U46`, che è **una seduta composta a mano di residui**: la prova che la
composizione serve, e che qualcuno l'ha già fatta una volta senza avere lo strumento per farla.

### 1.5 Le dipendenze grafiche esistono, e nessun campo le esprime

In questo clone `git ls-files Content` è un **oracolo valido**, e non era scontato: `.gitignore:48` ignora
`Content/**/*.uasset` e i file tracciati sono eccezioni negate. Disco e indice però coincidono —

```bash
find Content -name '*.uasset' | wc -l          # 134
git ls-files Content | grep -c '\.uasset$'     # 134
```

— quindi *asset non tracciato* significa *asset che non esiste*, non *asset non versionato*.

Undici sedute nominano asset che non esistono. Le pesanti:

| Seduta | Nomina | Realtà |
|---|---|---|
| `U7` (`asset`, critical) | `BP_Unit_Gadget`, `-Guardian`, `-Phase`, `-Ranger`, `-Riktor`, `-Wraith`, `DA_Hero_Flux` | esistono `BP_Unit_Aevik`, `-Branth`, `-Ivrin`, `-Muiren` — **quattro, non sei**; `Guardian` e `Ranger` stanno fuori dal roster v0.1; `Flux` è un nome che [D-130] ha rimosso; e i primi quattro sono **slot Paragon**, che [D-321] ha ristabilito non essere l'identità |
| `U24`, `U30` | `WBP_RT_PauseMenu` | non tracciato |
| `U24`, `U29` | `WBP_RT_ResultScreen` | non tracciato |
| `U23`, `U46` | `WBP_RT_EventLogRight` | non tracciato — è il feed di [#2697] |
| `U37` | `L_CameraFeatureLab` | mappa inesistente |

La dipendenza grafica ha almeno **quattro forme**, e il registro delle sedute non ne modella nessuna:

1. **asset assente** — verificabile;
2. **widget presente e non montato** — il feed di [#2697];
3. **cue mai disegnata** — `PIE-VIS-DEFLECT` e `PIE-VIS-INTERPOSE`, che [#2454] dichiara da non eseguire
   perché «gli eventi esistono, ma nessuno li disegna ancora»;
4. **animazione** — la riga «Animazioni:» che il piano `S0`–`S9` scrive per ogni fase, e che vive quindi
   nel documento che *non* possiede le sedute.

### 1.6 La tesi di questo documento è già scritta nel repository

`test-manuali-pie.md` la enuncia su un sottoinsieme di nove voci:

> «Le nove restanti si eseguono in **quattro allestimenti, non in nove**. Raggruppate per **precondizione**.»

e poco sotto applica la regola che ne discende: «`PIE-V01-ROSTER` **non entra in nessun allestimento**
finché la sua precondizione non è riscritta.»

Questo design non inventa un criterio. **Generalizza a tutto il registro un raggruppamento che il
progetto ha già fatto a mano, e di cui ha già dichiarato il criterio.**

---

## 2. La decisione di forma

**La seduta smette di essere un oggetto scritto e diventa il risultato di un raggruppamento calcolato.**

Si scrivono i mattoni: l'allestimento, il prerequisito, il cablaggio. La seduta è ciò che ne esce. Il
numero delle sedute smette di essere una scelta d'autore e diventa una misura — che è la risposta a
«ottimizziamo il numero»: non si pota, si smette di sceglierlo.

⚠️ **Questo non ribalta [D-181] e [D-182].** Quelle decisioni hanno rimosso una **vista generata e
committata** e la cartella `scripts/`. Il pattern che questo design usa esiste già e ha un precedente
accettato: `tools/decision-log/` genera da un owner markdown, non committa l'output (`build/` è ignorato,
`.gitignore:317`) e versiona una cache GitHub popolata da `gh`.

---

## 3. Il modello dati

`docs/roadmap/editor-sessions.yaml` viene riscritto. Tre sezioni, nessun record di seduta.

### 3.1 `setups:` — l'allestimento, con un ID

```yaml
setups:
  - id: SET-HEX-MATCH          # ex «A — partita hex avviata»
    map: L_HexArena
    format: Format.Skirmish2v2
    who: { human: [team0], bot: [team1] }
  - id: SET-HEX-TURN           # ex «B — un turno pianificato e risolto»
    extends: SET-HEX-MATCH
    given: "piani impostati, lock-in con Spazio, un turno con un fallback e una modifica ambientale"
  - id: SET-SCEN               # ex «C» — parametrico: lo scenario porta arena, unità e piani
    param: scenario_id
    command: "rt.Test.Scenario <scenario_id>"
  - id: SET-HEX-BOT            # ex «D — partita lunga col bot»
    extends: SET-HEX-MATCH
    cvars: [ "rt.Match.Autobattle=1" ]
  - id: SET-FRONTEND           # da L_Frontend, PLAY, poi Home
  - id: SET-SANDBOX            # L_DevSandbox — tool Geometry, griglia di lavoro
  - id: SET-GRAYKIT            # L_GrayKitPlayground
  - id: SET-TD                 # il pannello Tactical Designer
```

Gli otto ID nascono dalle citazioni misurate — `L_DevSandbox` sedici volte, `L_HexArena` sei,
`rt.Test.Scenario` sei, `L_GrayKitPlayground` tre — e dai quattro allestimenti `A`/`B`/`C`/`D` che
`test-manuali-pie.md` ha già nominato.

`extends` è la composizione vera: `SET-HEX-TURN` non riscrive la mappa, aggiunge uno stato.

**Due termini, definiti una volta perché il calcolo li distingue.** Una **apertura** è un avvio
dell'Editor; un **Play** è un ingresso in PIE dentro quell'apertura. Il KPI delle due aperture parla del
primo, non del secondo.

∴ `SET-SCEN` produce **una apertura, molti Play**: `rt.Test.Scenario <Id>` si riemette senza chiudere
l'Editor, quindi due check su scenari diversi si raggruppano insieme e l'ordine del giorno li stampa come
una lista di Play sotto la stessa apertura. Se domani un parametro **richiedesse** un riavvio
dell'Editor, quel setup dichiarerebbe `reopen: true` e il calcolo lo spezzerebbe; oggi nessuno lo fa.

### 3.2 `requires:` — il prerequisito, tipizzato e con un owner

| tipo | significato | oracolo |
|---|---|---|
| `asset` | il file non esiste | `git ls-files Content`; i riferimenti *interni* ai `.uasset` restano di `tools/asset-refs` |
| `mount` | il widget esiste e nessuno lo monta | l'issue owner — `WBP_RT_EventLogRight` → [#2697] |
| `cue` | l'evento esiste e nessuno lo disegna | l'issue owner — `PIE-VIS-DEFLECT`, `-INTERPOSE` → [#2454] |
| `anim` | la clip serve al verdetto, oppure è dichiarata non-oracolo | la riga «Animazioni:» del piano `S0`–`S9` |
| `feature` | la condizione non è ottenibile | un `git grep` misurato, oppure un checkpoint il cui owner resta `roadmap-checkpoint.md` |

Il tipo `feature` ha già un caso misurato: il runbook `guida-seduta-u46-residui-g9.md` riporta che
`git grep` di `bHumanPlanning|WaitForPlayer|Interactive|PauseForPlanning` in `ScenarioHarness/` dà **0**,
quindi una finestra di pianificazione umana su un banco oggi non esiste.

### 3.3 `wiring:` — il cablaggio, e soltanto quello

```yaml
wiring:
  - check: PIE-V01-SCREENHUD
    setup: SET-FRONTEND
    requires: [ mount:WBP_RT_TacticalHUD ]
    issue: 613
```

Nessun esito, nessun criterio, nessuna prosa. `test-manuali-pie.md` resta l'unico owner di *cosa deve
succedere* e di *com'è andata*. La regola che lo yaml oggi **enuncia** — «qui si citano gli ID, mai
l'esito atteso» — diventa struttura: nel nuovo schema manca il campo dove scriverlo.

---

## 4. Il generatore e i due output

`tools/editor-sessions/`, Python, sul modello di `tools/decision-log/`.

**Ingressi**: i tre registri · `test-manuali-pie.md` per lo stato di ogni voce · `git ls-files Content`
per gli `asset` · `github-cache.json` versionata per lo stato delle issue.

⛔ **Non reimplementa `tools/asset-refs`.** Quel tool risponde a una domanda diversa e già coperta — *un
asset versionato ne referenzia uno che git non ha?* — e legge i package path dai byte. Qui serve
soltanto *esiste?*, e a quello risponde `git ls-files`.

### 4.1 Il calcolo

1. Per ogni `check`: lo stato dal registro, e i `requires` non soddisfatti.
2. **Scarta i verdi.** Qui «ottimizzare il numero» diventa un effetto invece di una potatura.
3. Raggruppa per `setup`, risolvendo `extends`: un check su `SET-HEX-TURN` e uno su `SET-HEX-MATCH`
   cadono nella stessa apertura, perché il secondo è il primo più uno stato.
4. Un gruppo con almeno un check libero è **una apertura**. I check bloccati **restano stampati** sotto
   il gruppo, col bloccante e l'issue owner. Farli sparire li renderebbe invisibili, che è il difetto di
   oggi.
5. Ordina: prima il subset `RELEASE-V01` — l'unico insieme che, come dichiara il registro, ha una
   scadenza — poi per numero di check liberi.

### 4.2 Output 1 — l'ordine del giorno

`build/ordine-del-giorno.md`, rigenerato e **mai committato**. Per ogni apertura: l'allestimento in forma
eseguibile (mappa, comando, chi è umano), i check da guardare per ID, e sotto i bloccati col perché.

Risponde a una domanda sola: **apro l'Editor adesso — cosa allestisco e cosa guardo.**

### 4.3 Output 2 — le issue convocanti

Una issue **per apertura**, non per check. Tre vincoli:

- **Idempotenza.** Il corpo porta una marca `<!-- rt-session: SET-HEX-TURN -->`. Il generatore cerca
  quella, aggiorna quella, e non ne apre una seconda.
- **Niente totali volatili** nel titolo né nel corpo (`AGENTS.md` §14): si elencano gli **ID** dei check.
- **La rete si tocca solo con `--apply`.** Il default stampa il diff.

Chiude il difetto che [#2476] nomina nel proprio titolo — *«la seduta esiste, nessuna issue la convoca»* —
corretto il 2026-09-09 su `U46` come caso singolo, senza toccare la popolazione.

### 4.4 Errori: rifiutare, non ignorare

Il precedente è [D-182], dove `--wiki-root` esce con `sys.exit(2)` invece di venire lasciato cadere,
perché un argomento ignorato produce un verde falso.

| Condizione | Esito |
|---|---|
| un `check` cita un `setup` inesistente | **errore duro**, col nome |
| un `check` del wiring manca dal registro PIE | **errore duro** |
| un `requires` non è soddisfatto | **non è un errore**: è un bloccante, e si stampa |
| una voce del registro PIE manca dal wiring | **coda scoperta**, stampata a parte |

L'ultima riga misura la completezza. È il posto dove oggi vive `PIE-V01-ROSTER`, e senza di essa il
generatore dichiarerebbe di coprire tutto mentre copre un sottoinsieme.

### 4.5 Test

`tools/asset-refs/refs.test.ts` e `registry.test.ts` sono il precedente: qui i tool hanno test. Quattro
casi minimi — la risoluzione di `extends`, il raggruppamento per setup, un `requires` non soddisfatto che
blocca senza far sparire il check, e l'idempotenza della marca sulla issue.

---

## 5. La migrazione

### 5.1 Dove finisce ogni campo

| Campo | Destino |
|---|---|
| `verifies` | → `wiring`, una riga per check |
| `shares_setup_with` | **muore** — lo sostituiscono gli ID di `setups` |
| `issues` | → `wiring.issue` |
| `artifacts` | si **inverte**: ciò che una seduta produceva diventa ciò che un check richiede |
| `steps` | → i runbook, indicizzati per **setup** |
| `unblocked_by`, `unblocks` | → `requires: feature:` |
| `done_when` | **muore** — diceva sempre «la voce ha esito reale nel registro», che il registro possiede |
| `notes` | vedi §5.2 |
| `block` | **muore** |

### 5.2 `notes`: nessuna riga si cancella prima di essere riallocata

Sono archeologia misurata: date, comandi, contraddizioni trovate sul campo. Quattro destinazioni:

1. un fatto che vincola il comportamento → **Decision Log**, se non c'è già;
2. un fatto utile a chi apre l'Editor → il **runbook** del suo setup;
3. un fatto sullo stato di una voce → **`test-manuali-pie.md`**, che lo possiede già;
4. il resto → **`docs/archive/`**, col vecchio yaml intero, sul precedente di `docs/archive/roadmap-plans/`.

La quarta applica la clausola (4) di [D-321]: l'archivio conserva ciò che era vero allora, e le note di
supersessione si **aggiungono** invece di cancellare la provenienza.

### 5.3 Le fette

| # | Cosa | Perché qui |
|---|---|---|
| **0** | Scrivere `setups` e `wiring` **accanto** al vecchio yaml, senza cancellare. Far girare il generatore | **È il gate della migrazione**: si confrontano le aperture calcolate con le sedute scritte. Se il calcolo non riproduce ciò che l'autore avrebbe scelto, il modello è sbagliato e non si è perso niente |
| **1** | Tipizzare i `requires`, dagli undici casi di §1.5 | Qui muoiono i nomi morti |
| **2** | Migrare la prosa **per setup**, otto passaggi | È la fetta che fa risparmiare |
| **3** | Stampare la coda scoperta | Misura quanto il vecchio registro non copriva |
| **4** | Le issue convocanti, con `--apply` | Dopo che il calcolo è stato creduto |
| **5** | Dismettere: vecchio yaml in `archive/`, `S0`–`S9` marcato superato, i gruppi `A`/`B`/`C`/`D` sostituiti da un puntatore a `setups`, il documento Drive dichiarato superato con la data | Ultima, perché fino a qui il confronto resta possibile |

⚠️ **Fino alla fetta 5 il vecchio yaml resta l'owner**, e il nuovo porta un marcatore esplicito di
non-owner. Un solo owner in ogni istante: fermarsi a metà lascia uno stato dichiarato, non un limbo.

---

## 6. Confini

Questo design **non**:

- tocca `test-manuali-pie.md` come owner dell'esito — quel file perde soltanto il raggruppamento
  `A`/`B`/`C`/`D`, che duplicava;
- introduce un gate CI;
- rinomina gli eroi: [D-321] ha differito la migrazione post-v0.1 a [#2297], e i `requires` useranno i
  nomi **degli asset reali**, che sono un fatto di filesystem e non una posizione sull'identità;
- genera i runbook, che restano lavoro d'autore;
- decide l'ordine oltre «`RELEASE-V01` per primo»: la priorità resta d'autore;
- tocca `execution-graph.yaml` o i `roadmap-*.md`.

---

## 7. Rischi

**Il generatore diventa la sesta rappresentazione che nessuno legge.** È il modo esatto in cui è morto
`editormap.shortlist.md`. Mitigazione strutturale: l'ordine del giorno **non si committa**, quindi non può
invecchiare in silenzio — o lo rigeneri, o non esiste. E le issue sono il canale che chi lavora già apre.

**Un bloccante può nascondere un check per sempre.** Se `mount:WBP_RT_EventLogRight` resta dichiarato dopo
la chiusura di [#2697], quel check esce dall'agenda e nessuno se ne accorge. Mitigazione con macchinario
già previsto: la cache GitHub conosce lo stato delle issue, quindi un `requires` il cui issue owner è
**chiusa** viene stampato come sospetto. Sostituisce il gate senza costare un CI.

**La migrazione della prosa stalla a metà e restano due verità.** Lo previene la regola di §5.3: un solo
owner in ogni istante.

**Gli otto allestimenti possono essere la partizione sbagliata.** Lo verifica la fetta 0, che costa poco
proprio perché non cancella niente.

---

## 8. Criteri di accettazione

1. Per ogni voce PIE non verde del registro esiste una riga di `wiring` **oppure** la voce compare nella
   coda scoperta. Nessuna cade in mezzo.
2. Il generatore gira **senza rete**, leggendo `github-cache.json`.
3. Un check con `requires` non soddisfatto **compare** nell'ordine del giorno come bloccato, col
   bloccante e l'issue. Non sparisce.
4. Due esecuzioni consecutive con `--apply` non creano una seconda issue.
5. **Nessun campo dello schema può contenere l'esito atteso di un check** — si verifica leggendo lo
   schema, non la prosa.
6. Fetta 0: le aperture calcolate coprono tutti i check che le sedute scritte convocavano. La differenza
   si elenca **per ID** e si giudica.

### Come si misura che ha funzionato

Lo yaml dichiara già il proprio KPI — due aperture Editor, `E1` per gli `asset` ed `E2` per i `pie`,
«un KPI atteso, non un divieto». Oggi è un'asserzione che nessuno può calcolare. Dopo, è ciò che l'ordine
del giorno stampa.

---

## 9. Verification

- Compile: `N/A`
- Tests: `N/A` — documento di design, nessun codice
- Determinism: `N/A`
- Replay: `N/A`
- Privacy: `N/A`
- PIE: `N/A`
- Packaged: `N/A`

Le misure citate in §1 provengono da comandi eseguiti il 2026-09-11 su `main` a `1b9f6f36`, e ogni
comando è riportato accanto al proprio numero.

---

## 10. La decisione da scrivere

Questo repository gira sul Decision Log. Un cambio di questa portata senza una voce `D-nnn` resta
invisibile a chi rilegge fra un mese, e le cinque rappresentazioni ricrescono perché nessuno sa che erano
state collassate di proposito.

La voce deve dichiarare tre cose:

1. la seduta è **derivata**, e il registro contiene mattoni;
2. `test-manuali-pie.md` resta l'unico owner dell'esito;
3. le altre quattro rappresentazioni sono **superate**, con la data.

---

## 11. Follow-up candidates

- Il gate CI di coerenza, escluso da questo design per scelta esplicita.
- La correzione dei nomi d'eroe nel registro PIE, che al 2026-09-11 contiene righe con gli slot Paragon
  usati come identità; il perimetro e l'owner stanno in [#2297].
- `PIE-V01-ROSTER`: la sua precondizione è già stata riscritta il 2026-09-04, e la riga «non entra in
  nessun allestimento» va rimisurata contro il nuovo `wiring`.

[D-130]: ../../decisions/RT_PDR_00_Decision_Log.md
[D-181]: ../../decisions/RT_PDR_00_Decision_Log.md
[D-182]: ../../decisions/RT_PDR_00_Decision_Log.md
[D-321]: ../../decisions/RT_PDR_00_Decision_Log.md
[#2297]: https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297
[#2454]: https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454
[#2476]: https://github.com/DegrassiAaron/refactor-tactics-main/issues/2476
[#2697]: https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697
