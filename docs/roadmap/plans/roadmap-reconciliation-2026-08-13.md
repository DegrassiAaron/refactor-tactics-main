# Referto — Secondo passaggio sull'handoff di reconciliation (2026-08-13)

> **Partial reconciliation**, non full audit. `meta.last_full_audit` del Feature Registry **resta al
> 2026-08-08 / `2094b86`**: questo lavoro ha corretto contraddizioni misurate e registrato decisioni
> orfane, non ha riconfrontato `Source/` · `Tests/` · `Scenarios/` · `docs/` · Wiki riga per riga.

**Sorgente**: [`../../archive/src/handoff/2026-08-12-roadmap-reconciliation.md`](../../archive/src/handoff/2026-08-12-roadmap-reconciliation.md)
**Audit rifatto su**: `05bbe3dc` (`origin/main`) — il sorgente dichiarava `dda87f1a`, **28 commit indietro**
**Referto della prima passata**: [`roadmap-reconciliation-2026-08-12.md`](roadmap-reconciliation-2026-08-12.md)
**Esito**: il sorgente era **già consumato** · due residui prescritti e mai eseguiti · un difetto sistematico che nessuna delle due passate aveva cercato

---

## 1. Il documento era già stato applicato, e il modo di accorgersene è meccanico

Il sorgente è arrivato una seconda volta come file in radice. Non è una ripetizione inutile: è il caso in cui
**riapplicarlo sarebbe stato il danno**, perché avrebbe creato una seconda `CP 11.8` e riscritto correzioni
già fatte.

La verifica costa un comando, e non passa dalla lettura:

```bash
diff <(tr -d '\r' < RefactorTactics_Roadmap_Reconciliation_2026-08-12_Claude.md) \
     <(tr -d '\r' < docs/archive/src/handoff/2026-08-12-roadmap-reconciliation.md)
```

**29 righe di differenza, tutte in aggiunta, tutte in testa.** Il corpo — 685 righe — è identico. Quelle 29
righe sono il banner d'archivio che la prima passata ha scritto il 2026-08-12, e dicono già cosa fu recepito
e cosa no. **Byte-identico all'archiviato significa «già consumato», non «già fatto»**: la distinzione è
esattamente ciò che questo referto misura.

⚠️ **Le premesse operative del sorgente erano scadute in modo verificabile.** La sua §0.9 elenca due PR
«aperte» da non toccare:

| Il sorgente dice | `gh pr view` dice |
|---|---|
| **PR #688** aperta | **`MERGED`** il 2026-08-12T18:13:55Z |
| **PR #694** aperta, *stacked* su #688 | **`CLOSED`** |

L'unica PR concorrente reale al momento di questo lavoro era la **#718**, che il sorgente non poteva
nominare. Una lista di PR si **ri-elenca**, non si rilegge dall'handoff.

## 2. Cosa era stato prescritto e non eseguito

Il banner d'archivio si chiude così:

> ❌ **Non recepito**: le cinque epic proposte in §10 […]. Il documento stesso le marca **PROPOSTE**; senza
> una decisione non diventano roadmap, e il posto di una proposta senza decisione è `docs/OPEN_DECISIONS.md`.

**Non ci sono mai arrivate.** `grep` su `OPEN_DECISIONS.md` non trova nessuna delle cinque aree. È la stessa
forma del difetto che quel referto stava correggendo — una prescrizione scritta e nessuno che la esegua — ed
è la stessa che il progetto ha già registrato per il Decision Log (*«→ issue di rinomina» mai aperta*).

### 2.1 Ma «cinque» era il numero sbagliato, e trascriverle tutte avrebbe canonizzato tre falsità

Il sorgente §10 premette: *«Le ricerche live non trovano un owner dedicato completo per queste aree»*.
Rimisurato contro i **105 `feature_id`** di `feature-registry.yaml` — non contro il sorgente:

| Area §10 | Owner nel registry | Esito |
|---|---|---|
| **A. Super Actions** | `RT-FEAT-ACTION-SUPERS` — v0.2, P2, **`IMPLEMENTING`**, cinque gate `partial` | premessa **falsa** |
| **B. Modular Effects + Presentation/VFX** | nessuno per il mapping *outcome → presentazione* | **gap reale** → `FX-1` |
| **C. Seeded Map Generation** | nessuno; `RT-FEAT-TOOL-MAP-*` sono **authoring** | **gap reale** → `GEN-1` |
| **D. Production Map Generator** | `RT-FEAT-TOOL-MAP-EDITOR` **`INTEGRATED`** in v0.1 | **coperta** per l'authoring |
| **E. Networking / Dedicated server** | **tre** feature: `RT-FEAT-NET-PRIVATE-PLANNING`, `RT-FEAT-NET-AUTHORITY`, `RT-FEAT-NET-DEDICATED` | premessa **falsa** |

Registrate **due** domande, non cinque. Le tre righe «owner esiste» restano scritte in `OPEN_DECISIONS.md`
apposta: il prossimo kit le riproporrà, e trovarle già risposte costa meno che ridiscuterle.

> 🔴 La lezione non è «il sorgente sbagliava». È che **una proposta si registra dopo aver cercato chi possiede
> già l'area**, e la ricerca va fatta sull'elenco completo — 105 voci — non sui nomi che il sorgente suggerisce.
> Cercare `RT-FEAT-SUPER*` non avrebbe trovato `RT-FEAT-ACTION-SUPERS`.

### 2.2 Il rinvio su `#687` era motivato, e la motivazione è scaduta

La prima passata aveva rinviato la decisione su `#687` (`FormatVersion` non serializzato) con una ragione
precisa: *«finché il meccanismo non è verificato su asset serializzato con binario vecchio/nuovo — ed è ciò
che fa la PR #688, **aperta**»*.

**#688 è mergiata.** Ha portato la **verifica, non la correzione**:
`RefactorTactics.HexMap.SerializedAssetMigratesWithoutGainingData`
(`Source/RefactorTactics/Tests/RTHexMapTests.cpp:671`), col commento a `:664` che scrive l'esito — *«la
migrazione e' inerte»*. Il difetto è ora dimostrato sul binario e **nessuna** delle quattro direzioni è stata
scelta.

Registrata come `FMT-1`, insieme a `FMT-2` — perché il piano
[`mappe-generate-o-dipinte-2026-08-12.md`](mappe-generate-o-dipinte-2026-08-12.md) §6 conclude che *«le due
cose vanno decise insieme, o si costruisce un secondo controllo sopra un meccanismo inerte»*.

⚠️ **Quel piano era orfano**: `grep -rl "mappe-generate-o-dipinte" docs/` non restituiva **nulla**. Un
documento che nessuno linka, con due domande aperte in §6, che il link checker non può segnalare — perché un
file non raggiunto non è un link rotto. Ora è citato da `OPEN_DECISIONS.md`.

## 3. Il difetto che nessuna delle due passate aveva cercato

La prima passata ha corretto le checkbox stale di **tre** epic (`#14`, `#25`, `#152`), che erano quelle
nominate dal sorgente. Nessuno ha chiesto **se ce ne fossero altre**.

Verificate **tutte e 40** le epic del repository, confrontando ogni riga `- [ ] #NNN` con lo stato live della
figlia: **7 divergenze su 4 epic**, tutte nello stesso verso — figlia chiusa, casella vuota.

| Epic | Figlie non spuntate ma `CLOSED` | Conseguenza |
|---|---|---|
| **#214** — E19 · Classe di mappa | `#215`, `#216` — **entrambe** | l'epic non ha checkpoint residui: **candidata alla chiusura** |
| **#217** — E20 · Icon Language | `#218` | — |
| **#221** — E17 · Stress 4v4 | `#222`, `#223` | resta **solo `#224`**: tagliare E17 costa un checkpoint, non tre |
| **#225** — E18 · Predictive *(epic già chiusa)* | `#226`, `#227` | epic chiusa con checklist vuota: chi la riapre legge «niente fu fatto» |

➡️ **#214 non è stata chiusa d'ufficio.** Lo stato autorevole vive in `feature-registry.yaml` ed è **derivato
dai gate**: nessuna epic si chiude perché le sue figlie lo sono. La riga è una segnalazione, non un'azione.

> 🔴 Il difetto è **di ricerca, non di scrittura**. Ogni passata ha corretto esattamente le epic che il proprio
> sorgente nominava, e un sorgente nomina ciò che ha guardato. Il controllo costa poche righe e gira su tutte
> e 40 — qui sotto, così che la prossima passata non debba riscoprirlo.

### Il controllo, da rieseguire e non da ricordare

```bash
gh issue list --state all --limit 500 --json number,state,title > /tmp/issues.json
```

```python
import json, re, subprocess
issues = json.load(open('/tmp/issues.json', encoding='utf-8'))
state = {i['number']: i['state'] for i in issues}
epics = [i['number'] for i in issues if i['title'].startswith('[EPIC')]
ROW = re.compile(r'^\s*-\s*\[([ xX])\]\s*#(\d+)', re.M)

for e in sorted(epics):
    body = subprocess.run(['gh', 'issue', 'view', str(e), '--json', 'body', '-q', '.body'],
                          capture_output=True, text=True, encoding='utf-8').stdout
    for mark, num in ROW.findall(body):
        live = state.get(int(num))
        if live and (mark.lower() == 'x') != (live == 'CLOSED'):
            print(f"Epic #{e}: figlia #{num} spuntata={mark.lower()=='x'} ma live={live}")
```

Confronta **la casella** con **lo stato live**, nei due versi: una figlia spuntata e riaperta è lo stesso
difetto al contrario, e finora non si è mai presentata solo perché nessuno riapre.

## 4. E20: il confine di scope che la DoD chiedeva e nessuno aveva scritto

La DoD del sorgente §13 chiede: *«E20 distingue scope v0.1 dalle 10 decisioni tassonomiche future di #637»*.
La riduzione **17 → 10** è stata applicata dentro `#637` il 2026-08-12, ma `#217` **non nominava `#637`
affatto**: l'epic elencava tre checkpoint e taceva su dove vivessero le dieci decisioni aperte.

Il confine ora è scritto in `#217`, e i due soggetti sono diversi:

| | Soggetto | Numero | Dove si chiude |
|---|---|---|---|
| **E20 (v0.1)** | chiavi che i widget **usano davvero** | **33** + missing-icon | `#219`, `#220` |
| **#637** | il **linguaggio** del manifest di design | **10** decisioni tassonomiche | E25 (`#265`), post-v0.1 |

Le 33 non sono trascritte: sono **derivate a runtime** da `RequiredIconIds()`
(`Source/RefactorTactics/UI/RTIconLibrary.cpp`), e `FindMissingRequiredIcons` vuoto è il gate. La
scomposizione dichiarata da `#219` — 4 fasi · 9 azioni · 11 status · 3 certezza · 6 identità — somma a 33.

## 5. Cosa è stato deliberatamente **non** fatto

| Non fatto | Perché |
|---|---|
| aprire epic per `FX-1` e `GEN-1` | sono **proposte senza decisione**: il loro posto è `OPEN_DECISIONS.md`, ed è la stessa scelta della prima passata |
| chiudere `#214` (E19) | lo stato è **derivato dai gate**, non dalle issue figlie — vedi §3 |
| decidere `FMT-1` | `#687` scrive *«la scelta ha implicazioni sul formato e va fatta da chi lo possiede»*, e resta vero |
| ribilanciare `Brace` | invariato dalla prima passata: decide `BAL-1` (`#403`), applica `#404` |
| aggiornare `meta.last_full_audit` | non è stato fatto un full audit — vedi il banner in testa |
| toccare i file della PR **#718** | unica PR aperta durante il lavoro; nessun file delle sue aree è stato modificato |
| creare un secondo archivio del sorgente | è già in `docs/archive/src/handoff/` dal 2026-08-12: la copia in radice era un **duplicato**, non un documento nuovo |

## 6. Numeri

| Misura | Prima | Dopo | Comando |
|---|---:|---:|---|
| epic verificate contro le figlie | 3 *(quelle nominate)* | **40** *(tutte)* | script §3 |
| checkbox divergenti | 7 *(non cercate)* | **0** | idem |
| voci in `OPEN_DECISIONS.md` | — | **+4** (`FMT-1`, `FMT-2`, `FX-1`, `GEN-1`) | — |
| piani orfani fra i documenti citati | 1 (`mappe-generate-o-dipinte`) | **0** | `grep -rl` |
| link relativi controllati | 3091 | **3094** | `check-docs-links.py` |
| `validate` | 0 errori · 34 warning | **0 errori · 34 warning** | `feature_registry.py validate` |

## 7. Next action

Le lane di `roadmap-v0.1.md` §3 restano quelle della prima passata e **non si aspettano fra loro**:

```text
Lane A — Reactions:   #165 → #166        (poi, e solo poi, #314 → #319)
Lane B — Perception:  #690 + #686 → #159 → #160
Lane C — UI:          #219/#637 → #220 → #77/#613 → #705 → #291
Lane D — Consistency: #625 + #687 + #649 → #512 → #170  (prima del golden)
```

Due voci si aggiungono, entrambe **fuori dalle lane** perché aspettano una persona, non un checkpoint:

- **`FMT-1`** — la direzione su `#687`. È in Lane D come *fix*, ma la **decisione** viene prima del fix, e
  ora non ha più scuse per aspettare;
- **`#214`** — verificare i gate di E19 e, se verdi, chiuderla.
