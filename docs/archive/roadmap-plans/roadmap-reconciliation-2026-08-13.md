# Referto — Secondo passaggio sull'handoff di reconciliation (2026-08-13)

> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> `SNAPSHOT` · **Data**: 2026-08-13
> **Cosa e'**: il secondo passaggio sullo stesso handoff, riconciliazione **parziale**.
> **Cosa non e'**: una fonte di stato. ⚠️ Contiene affermazioni **superate lo stesso giorno**: dice che
> `FMT-1` e' da decidere, e [`D-137`](../../decisions/RT_PDR_00_Decision_Log.md) l'ha chiusa la sera. E'
> il comportamento normale di uno `SNAPSHOT`, ed e' il motivo per cui ne porta il banner.
>
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

La verifica costa un comando, e non passa dalla lettura. ⚠️ Il sorgente arrivava come file **untracked nella
radice del repository**, quindi il comando non è rieseguibile da questo albero: va rifatto contro il file che
si ha in mano, ogni volta che ne arriva uno.

```bash
# $SORGENTE = il file appena ricevuto, ovunque stia
diff <(tr -d '\r' < "$SORGENTE") \
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

L'unica PR concorrente reale era la **#718**, che il sorgente non poteva nominare — ed è stata **mergiata
mentre questo lavoro era in corso** (vedi §6). Una lista di PR si **ri-elenca**, non si rilegge
dall'handoff: la sua è datata, e la concorrente vera può non esserci.

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
[`mappe-generate-o-dipinte-2026-08-12.md`](../../roadmap/plans/mappe-generate-o-dipinte-2026-08-12.md) §6 conclude che *«le due
cose vanno decise insieme, o si costruisce un secondo controllo sopra un meccanismo inerte»*.

⚠️ **Quel piano era orfano**: `grep -rl "mappe-generate-o-dipinte" docs/` non restituiva **nulla**. Un
documento che nessuno linka, con due domande aperte in §6, che il link checker non può segnalare — perché un
file non raggiunto non è un link rotto. Ora è citato da `OPEN_DECISIONS.md`.

## 3. Il difetto che nessuna delle due passate aveva cercato

La prima passata ha corretto le **tre** epic nominate dal sorgente — `#14`, `#25`, `#152` — ma solo su `#14`
il difetto era una **checkbox** (`#22` e `#175` chiuse e non spuntate); in `#25` era un'affermazione falsa su
`FAutoConsoleCommand` e in `#152` una lista di checkpoint incompleta. Nessuno ha chiesto **se di checkbox
stale ce ne fossero altre**, in altre epic.

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
| **E20 (v0.1)** | chiavi che i widget **usano davvero** | **60** + missing-icon | `#219`, `#220` |
| **#637** | il **linguaggio** del manifest di design | **10** decisioni tassonomiche | E25 (`#265`), post-v0.1 |

L'insieme è **derivato a runtime** da `URTIconLibrary::RequiredIconIds()`
(`Source/RefactorTactics/UI/RTIconLibrary.cpp:27`), e `FindMissingRequiredIcons` vuoto è il gate.

> 🔴 **La prima stesura di questa sezione diceva «33», e l'ha trascritto.** È l'errore esatto che questo
> referto rimprovera al sorgente, commesso mentre lo si rimproverava — e per la stessa ragione: la
> scomposizione «4 fasi · **9 azioni** · 11 status · 3 certezza · 6 identità» viene da `#219` e da
> `brief-icone-v01.md`, che sono fonti **interne**, e a quelle non era stato applicato il vaglio.
>
> Misurato: `RequiredIconIds()` itera **tutto** `URTCatalogLibrary::GetCoreActionCatalog()` aggiungendo una
> chiave per `ActionId`, e quel catalogo dichiara **36** azioni (`RTCatalogLibrary.cpp:709-1172`), non nove.
> Le altre quattro categorie reggono: 4 · 11 · 3 · 6 verificate una per una. **Totale 60.**
>
> ```bash
> sed -n '709,1172p' Source/RefactorTactics/Ability/RTCatalogLibrary.cpp | grep -c 'Catalog.Add('   # 36
> ```
>
> ⚠️ **Non è un numero cosmetico**: era già stato pubblicato su `#217` come confine di scope autorevole fra
> E20 e `#637`. **Cinque copie corrette**, e la prima passata ne aveva prese solo due — il riepilogo, non il
> posto dove il lavoro si esegue:
>
> | Copia | Cosa diceva | Perché conta |
> |---|---|---|
> | `#217` | `33` nel confine di scope | l'epic che **riassume** |
> | referto §4 | «le 33 sono derivate» | qui |
> | `roadmap-v0.1.md` | «le 33 chiavi» | lane C |
> | `brief-icone-v01.md` ×4 | titolo, `Action — 9`, «oltre alle 33», **«popolare `Icons` con le 33 voci»** | l'ultima è **l'istruzione che una persona esegue** |
> | **`#219`** ×3 | riga dell'insieme, **DoD non spuntata «33 icone»**, «33 texture reali» | il checkpoint che **si fa**, e la sua DoD **è il gate** |
>
> 🔴 Le ultime due righe erano sopravvissute alla prima correzione, che aveva sistemato il riepilogo e
> lasciato il numero **dove il lavoro atterra**. È la forma esatta di `C1` — un documento che esce dalla
> correzione contraddicendosi — applicata al numero di `C2`, nello stesso commit che diagnosticava `C1`.
> La sezione «Le 33 chiavi» del brief è stata **rinominata** in «Le chiavi richieste»: verificato che nessun
> link punti alla sua ancora, e un numero in un titolo non viene mai riletto come un dato.
>
> ⚠️ **Conseguenza reale**: il gate di `#219` pretende *tutte* le chiavi derivate, quindi la v0.1 chiede oggi
> **60 disegni + il missing-icon**, non 34. Se sono troppi, la leva è decidere quali azioni entrano
> nell'insieme richiesto — una decisione di `#219`, non un ritocco all'elenco: oggi `RequiredIconIds()` non
> filtra nulla.
>
> La lezione, in una riga: **un numero scritto per un insieme che è una funzione invecchia in silenzio**, e
> qui era stato scritto tre volte. È lo stesso difetto dei totali epic/CP e del registro PIE, sulla terza
> famiglia di numeri in due giorni.

## 4-bis. Il registro PIE diceva 117 e ne erano 135 — e una condizione d'innesco era già scattata

Trovato consolidando la scenario map, non cercandolo. `scenario-map.md` dichiarava il totale del registro PIE
in **due modi diversi nello stesso file**: **116** in §1 e **117** in fondo alla tabella delle classi.

Il valore reale, col comando che quel documento pubblica nella propria §7:

```bash
grep -c '^| \*\*PIE-'     docs/technical/test-manuali-pie.md   # 135
grep -c '^| \*\*PIE-VIS-' docs/technical/test-manuali-pie.md   #  21
grep -c '^| \*\*PIE-MUT-' docs/technical/test-manuali-pie.md   #   2
```

⚠️ **Non è un difetto di merge.** `135` era già il valore su `origin/main` (`6e109776`) prima che questa
sessione iniziasse; il `117` fu scritto il **2026-08-09** (`362b717a`) e non è più stato toccato mentre il
registro cresceva di **diciotto** voci. Nessun gate confronta quel totale col registro: è sopravvissuto
quattro giorni a ogni `validate` verde, esattamente come i totali epic/CP prima del 2026-08-12.

La partizione ora è **esaustiva e verificabile da sé** — `B 21 + C 112 + 2 fuori classe = 135` — e `C` non è
stata contata a mano: è la sottrazione, cioè la definizione della sua riga resa aritmetica. Prima
`21 + 95 + 1 = 117` tornava, e **nessuna delle tre parti veniva dal registro**: è il caso peggiore, un totale
che si verifica contro sé stesso.

> 🔴 **E la scoperta vera non è un numero: è una condizione d'innesco già scattata.** Il riquadro sulla voce
> fuori classe si chiudeva così, dal 2026-08-09:
>
> > «Il modello a quattro classi divide per *dove sta l'oracolo*. […] **Se ne arriva una seconda, vale la pena
> > farne una classe.**»
>
> La seconda è arrivata: `PIE-MUT-ACTIONS-ZERO` sta nel registro accanto a `PIE-MUT-BASTION-SLOW`. Le voci di
> mutazione sono **due**.
>
> Un totale sbagliato prima o poi stona. Una **condizione già soddisfatta non stona mai** — resta a sembrare
> futura, e si legge come una previsione invece che come un arretrato. Era scritta bene, verificabile con un
> `grep`, e nessuno l'ha rieseguita. ➡️ Se `PIE-MUT-*` debba diventare una classe **E** — *oracolo automatico,
> precondizione umana* — è una scelta sul modello e **non** è stata presa qui.

### Le copie del totale, cercate prima di fermarsi alla fonte

Correggere `scenario-map.md` e fermarsi lì avrebbe riprodotto il difetto del 2026-08-12, che era proprio
questo. `grep -rnE "\b11[567]\b" docs/ --include=*.md` filtrato sul contesto PIE:

| Copia | Esito |
|---|---|
| `technical/scenario-map.md` ×2 (§1 e tabella) | **fonte** — corrette entrambe |
| `roadmap/plans/editormap-spec.md` | spec **in revisione**, descrive lo stato corrente dei documenti fratelli — **corretta** |
| `technical/scenari-validazione-visiva.md` | misura **auto-datata** («Al 2026-08-09») — lasciata, e **affiancata** dalla misura di oggi |
| `roadmap/plans/map-editor-brief-spec-panel-2026-08-09.md` | **referto datato** — **lasciato** |

⚠️ **Le righe `A` e `D` e il totale del corpus restano NON rimisurate**: hanno per soggetto gli scenari di
`Scenarios/`, non il registro PIE, e la loro scomposizione per classe è **umana** — l'avvertenza del riquadro
del 2026-08-10 vale ancora, e questa passata non l'ha aggirata.

## 5. Cosa è stato deliberatamente **non** fatto

| Non fatto | Perché |
|---|---|
| aprire epic per `FX-1` e `GEN-1` | sono **proposte senza decisione**: il loro posto è `OPEN_DECISIONS.md`, ed è la stessa scelta della prima passata |
| chiudere `#214` (E19) | lo stato è **derivato dai gate**, non dalle issue figlie — vedi §3 |
| rimisurare le righe `A`/`D` e il totale del corpus in `scenario-map.md` | hanno per soggetto `Scenarios/`, non il registro PIE, e la loro scomposizione è **umana**: va rifatta voce per voce, non aggiustata di uno — lo dichiara il riquadro del 2026-08-10 |
| fare di `PIE-MUT-*` una classe **E** | l'innesco è scattato (§4-bis) ma è una scelta sul **modello** delle classi, e questa passata è documentale |
| decidere `FMT-1` | `#687` scrive *«la scelta ha implicazioni sul formato e va fatta da chi lo possiede»*, e resta vero |
| ribilanciare `Brace` | invariato dalla prima passata: decide `BAL-1` (`#403`), applica `#404` |
| aggiornare `meta.last_full_audit` | non è stato fatto un full audit — vedi il banner in testa |
| toccare i file della PR **#718** | unica PR concorrente; nessun file delle sue aree è stato modificato, e infatti il merge non ha dato conflitti |
| creare un secondo archivio del sorgente | è già in `docs/archive/src/handoff/` dal 2026-08-12: la copia in radice era un **duplicato**, non un documento nuovo |

## 6. Numeri

| Misura | Prima | Dopo | Comando |
|---|---:|---:|---|
| epic verificate contro le figlie | 3 *(quelle nominate)* | **40** *(tutte)* | script §3 |
| checkbox divergenti | 7 *(non cercate)* | **0** | idem |
| voci in `OPEN_DECISIONS.md` | — | **+5** (`FMT-1`, `FMT-2`, `FX-1`, `FX-2`, `GEN-1`) | — |
| copie del «33» corrette | 2 *(solo il riepilogo)* | **12** su 5 documenti/issue | §4 |
| piani orfani fra i documenti citati | 1 (`mappe-generate-o-dipinte`) | **0** | `grep -rl` |
| totale registro PIE dichiarato | 116 *(§1)* · 117 *(tabella)* | **135 · 135** | `grep -c '^\| \*\*PIE-'` |
| copie vive di quel totale | 3 *(nessuna aggiornata)* | **3 allineate** *(+2 datate, lasciate)* | ⚠️ `grep -rnE "\b11[567]\b" docs/` — **insufficiente**: cerca il totale, non le parti. Vedi §7, Famiglia 2 |
| copie delle **parti** *(`C 95`, `1 fuori classe`)* | 3 *(non cercate)* | **3 allineate** | `grep -nE "\b95\b\|fuori classe" docs/roadmap/*.md docs/technical/*.md` |
| voci `PIE-MUT-*` *(fuori classe)* | 1 *(dichiarata)* | **2** *(misurate)* | `grep -c '^\| \*\*PIE-MUT-'` |
| link relativi controllati | 3091 su 308 file | **3178 su 317 file** | `check-docs-links.py` |
| affermazioni fattuali respinte dalla code review | — | **3 critiche · 5 importanti** | §7 |
| `feature_id` nel registry | 105 | **105** | `grep -c "^  - feature_id:"` |
| `validate` | 0 errori · 34 warning | **0 errori · 34 warning** | `feature_registry.py validate` |
| pagine Wiki da aggiornare | — | **0** | `deploy --wiki-root <clone>` |

> ⚠️ **Il conteggio dei link è salito due volte, e la prima volta non per merito del contenuto.** Prima dello
> `stage` il checker leggeva **3091 link su 308 file**: il referto appena scritto era **untracked**, e
> `check-docs-links.py` guarda i file **versionati**. Un documento nuovo non è controllato finché non entra
> nell'indice — quindi la misura che conta si fa **a commit fatto**, non a file salvato. Dopo lo stage: 3094
> su 309.
>
> 🔴 **E poi è stato un merge. Tre volte.** `origin/main` si è mossa **tre volte** mentre questo lavoro era
> in corso — **#718** (hex geometry), **#723** (soglia d'udito) e i loro seguiti — e ogni volta il conteggio
> dei link è cambiato sotto: `3094` → `3142` → `3143` → **`3178` su 317 file**, che è il valore sull'albero
> unito finale. È la stessa lezione del §6-bis del referto del 2026-08-12, ripresentatasi a ogni occasione:
> un numero misurato prima del merge è corretto sulla propria base e falso dopo l'unione.
>
> ✅ **Rimisurati tutti gli altri dopo l'ultimo merge, non ricalcolati**: `feature_id` **105**, voci PIE
> **135**, `PIE-VIS-` **21**, `PIE-MUT-` **2**, azioni del catalogo **36**, `validate` **0 errori · 34
> warning**. Reggono tutti: solo i link dipendevano dai file che i merge portavano.
>
> ⚠️ I generati non hanno mai dato conflitto, e `generate --check` / `shortlist --check` li dichiarano
> allineati sull'albero unito — **verificato, non dedotto dall'assenza di conflitto**: `feature-registry.json`
> si auto-mergia in silenzio, ed è il caso in cui git non avverte. Anche le modifiche alla **prosa fuori dal
> blocco generato** di `scenariomap.shortlist.md` sono state ricontrollate dopo ogni merge, perché quel file
> è generato e un rigenerato avrebbe potuto perderle.
>
> ➕ L'ultimo merge ha portato un **gate nuovo**, `scripts/check-docs-naming.py`, eseguito su questo albero:
<!-- rename-exempt: la riga dichiara la rinomina: sostituirla la renderebbe muta -->
> **exit 0**. Cerca i nomi legacy del roster (`Flux`, `Riva`, `Bastion`, `Vektor`) usati come prosa
> player-facing, e per scelta dichiarata fallisce **solo** sui file in `ENFORCED` — che oggi sono **3 su
> 220**, cioè l'1% di copertura, con **832** occorrenze di arretrato in 72 file. `OPEN_DECISIONS.md` vi
> compare con 29 occorrenze, **preesistenti**: verificato che il diff di questo lavoro non ne aggiunga
> nessuna (`git diff origin/main...HEAD | grep '^+' | grep -cE '\b(Gadget\|Phase\|Riktor\|Wraith)\b'` → **0**,
> su tutti i file toccati).

## 7. Cosa la code review ha respinto — e il difetto comune alle otto voci

La revisione ha rimisurato invece di rileggere, e ha bocciato **tre** affermazioni critiche e **cinque**
importanti di questo stesso lavoro. Restano scritte qui perché sono la parte più istruttiva.

| # | Affermazione respinta | Cosa dice la misura |
|---|---|---|
| C1 | «ho corretto il totale PIE» | la §5 dello **stesso file** continuava a dire `95`: il documento usciva dalla correzione **contraddicendosi ancora** |
| C2 | «le **33** chiavi sono derivate a runtime» | derivate sì, ma sono **60**: `RequiredIconIds()` itera 36 azioni, non 9 — ed era già pubblicato su `#217` |
| C3 | «`MakeArenaV01` non è nel registry» | c'è, `RTMatchSetupLibrary.cpp:322-325`, dal 2026-08-12 03:03 |
| I1 | «nessun documento nomina l'*outcome event*» | `RTResolvedEvent.h` lo dichiara, owner `RT-FEAT-CORE-PLAYBACK`, **`INTEGRATED`** |
| I2 | — | pipe non escapato in una cella di `editormap-spec.md`: la riga si rompe in render |
| I3 | «cercate le copie del totale» | cercato il **totale**, non le **parti**: `C 95` e `1 voce fuori classe` erano rimaste in `scenariomap.shortlist.md` |
| I4 | «le tre `RT-FEAT-NET-*` (`IDEA`, `future`)» | `NET-PRIVATE-PLANNING` è **`TESTABLE` in v0.1** |
| I5 | «cinque gate `partial`» | sono **sei** |

### Le famiglie sono **due**, e la seconda ha un rimedio diverso

> ⚠️ La prima stesura di questa sezione diceva *«sei voci su otto hanno la stessa causa»*. **Non è vero**, e
> il difetto della frase è lo stesso che elenca: un'affermazione più larga della misura. Ricontate sul
> **meccanismo** invece che sull'origine, le famiglie sono due e tre voci restano isolate.

**Famiglia 1 — un numero trascritto da una fonte interna** *(C1, C2, C3, e M1)*

C2 viene da `brief-icone-v01.md` e `#219`; C3 e M1 dal piano `mappe-generate-o-dipinte`; C1 da un'altra
sezione del documento che si stava correggendo. Ognuna è stata **trascritta** — lo stesso verbo che questo
referto usa per accusare il sorgente, tre sezioni più su.

Un handoff esterno arriva col sospetto addosso e viene misurato. Un documento del repository arriva con la
presunzione di essere già stato verificato **da qualcuno, una volta**, e quella presunzione non scade mai. È
il motivo per cui `117` è sopravvissuto quattro giorni e `9 azioni` parecchi di più.

➡️ **Rimedio: se stai per scrivere un numero che un comando può produrre, esegui il comando** — anche quando
la fonte è di casa.

**Famiglia 2 — un'assenza dichiarata senza enumerare il dominio** *(I1, I3)*

Queste due **non sono trascrizioni**: sono conclusioni **originali**, prodotte da una ricerca che non ha
dichiarato su quale insieme girava. *«Nessun documento nomina l'outcome event»* nasce da un `grep` sull'area
`Characters` che non ha guardato `Core`; *«cercate le copie del totale»* ha cercato il **totale** e non le
**parti**, lasciando `C 95` e `1 voce fuori classe` nella shortlist.

🔴 È la regola che questo stesso referto scrive **bene** in §2.1 — *«la ricerca va fatta sull'elenco completo,
105 voci, non sui nomi che il sorgente suggerisce»* — e che non ha applicato a sé stesso due sezioni dopo.
Fondere queste due nella Famiglia 1 farebbe perdere proprio la lezione già imparata: «esegui il comando» non
avrebbe prevenuto né I1 né I3, perché il comando c'era ed era **il comando sbagliato**.

➡️ **Rimedio: un'assenza si afferma solo dicendo su quale insieme completo è stata cercata.** «Non esiste X»
senza «cercato fra questi N» è un'opinione.

**Isolate — nessun pattern** *(I2, I4, I5)*

`I2` è una regola di sintassi nota e non applicata in un punto su due; `I4` una sintesi imprecisa di una
lettura corretta; `I5` un conteggio sbagliato, sei letti come cinque. Dichiararle parte di un pattern
sarebbe la terza sovra-generalizzazione della stessa pagina.

## 8. Next action

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
