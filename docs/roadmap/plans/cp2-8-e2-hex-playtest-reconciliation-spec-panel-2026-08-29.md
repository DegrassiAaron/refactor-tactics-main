# CP 2.8 / E2 — riconciliazione del playtest hex · spec panel

> `CURRENT` · **Stato**: revisione chiusa. Il sorgente è **consumato e archiviato**; le correzioni del §12
> sono state **applicate** su richiesta dell'autore nello stesso giorno — vedi §12, che dichiara quali e dove ·
> **Data**: 2026-08-29
> **Base di misura**: `a345137d`, branch `feat/1535-velo-in-partita`. Le correzioni sono poi state scritte su
> un branch dedicato da `origin/main` = **`8295d00c`**: nessuno dei documenti misurati differisce fra i due
> alberi (`git diff a345137d..8295d00c` sui sette file tocca **un solo** referto di questa cartella), quindi
> le misure valgono su entrambi. §13.
> **Oggetto**: `CLAUDE_CP2_8_E2_HEX_PLAYTEST_DOC_RECONCILIATION.md` (754 righe, diciassette sezioni), un
> mandato di allineamento documentale fra la issue [#38](https://github.com/DegrassiAaron/refactor-tactics-main/issues/38)
> (CP 2.8, playtest della partita hex) e l'epic [#16](https://github.com/DegrassiAaron/refactor-tactics-main/issues/16)
> (E2, parità hex del substrato): perimetro del gate, una sola apertura d'Editor per `U2`–`U6`, terreno
> generato invece dell'asset d'autore, e l'abbandono del requisito «fino alla vittoria».
> Letto contro [`test-manuali-pie.md`](../../technical/test-manuali-pie.md),
> [`editor-sessions.yaml`](../editor-sessions.yaml), i corpi lato server di **#16** e **#38**,
> `Source/RefactorTactics/{Ability,Combat,Turn}/`, gli scenari di `Scenarios/`, e i cinque documenti di
> roadmap che citano `PIE-HEXPLAY`.
> **Panel**: Wiegers (lead) · Cockburn · Adzic · Nygard · Crispin · Doumont
> **Modo**: critique
> **Archiviato in**: [`../../archive/src/handoff/2026-08-29-cp2-8-e2-hex-playtest-reconciliation.md`](../../archive/src/handoff/2026-08-29-cp2-8-e2-hex-playtest-reconciliation.md)
> **Predecessori sullo stesso oggetto**: [`m6-8-playtest-hex-spec-panel-2026-08-14.md`](m6-8-playtest-hex-spec-panel-2026-08-14.md)
> (il *perché* della DoD di #38) e [`m6-8-sequenza-sedute-u2-u6-2026-08-14.md`](m6-8-sequenza-sedute-u2-u6-2026-08-14.md)
> (il *come* della seduta) — vedi §10, che è la scoperta principale di questa lettura.

---

## 1. Il verdetto in una riga

**Il kit ha ragione su quasi tutto e porta quasi nulla di nuovo: undici delle sue diciassette sezioni
descrivono decisioni già prese — il 2026-08-25 in #16, il 2026-08-14 nei due referti fratelli — e la
«procedura canonica» che il §5 chiede di scrivere esiste da quindici giorni.** Ciò che resta, e che vale la
lettura, è **una** divergenza reale e non chiusa: **#38 e #16 portano due Definition of Done incompatibili**,
e #38 è la issue che si esegue.

| | Voci |
|---|---:|
| 🔴 Critico | **2** |
| 🟠 Alto | **3** |
| 🟡 Medio | **4** |
| ⛔ Trappola del mandato stesso | **1** |
| ➕ Trovato misurando, non è del sorgente | **2** |

**Raccomandazione operativa**: **non eseguire il kit come mandato.** Applicarlo alla lettera riscriverebbe
documenti già corretti e — per il §3.1 — ne romperebbe di corretti (§11). Ciò che va fatto sono **cinque
correzioni puntuali** (§12), quattro delle quali il kit non nomina, più **una decisione d'autore** su quale
DoD sopravvive fra #38 e #16.

⚠️ Nessuna suite eseguita, nessuna build, nessuna scrittura su GitHub, nessun file owner toccato. Issue lette
lato server con `gh` il 2026-08-29; `docs/`, `Source/` e `Scenarios/` ad `a345137d`.

---

## 2. Baseline misurata

Il kit apre chiedendo di non fidarsi del proprio testo (§2, *«non assumere che il testo riportato in questo
brief sia più recente del repository»*) e di rimisurare lo stato (§4, *«non copiare il numero 6/14 o 8
residui»*). Rimisurato. **Il kit era accurato**: nulla si è mosso dal 2026-08-25.

| Fatto | Fonte misurata | Esito |
|---|---|---|
| Voci `PIE-HEXPLAY` nel registro | `grep -oE 'PIE-HEXPLAY-[0-9]+[a-z]?'` su [`test-manuali-pie.md`](../../technical/test-manuali-pie.md) | **15** distinte |
| Perimetro del gate E2 | corpo di **#16**, §*Gate di chiusura*, riscritto il 2026-08-25 | **14** — `-11` esplicitamente fuori |
| Stato del gate | prime colonne di stato delle righe `**PIE-HEXPLAY-N**` | **6 su 14 ✅** |
| Residui | idem | **8**: `-3b` 🟡 · `-4` ⏳ · `-4b` ⏳ · `-6` 🟡 · `-6b` ⏳ · `-6c` ⏳ · `-7` 🟡 · `-8` 🟡 |
| `RoundLimit` spedito | `RTMatchFormatLibrary.cpp`, commento su `FindShippedFormat` | **12**, allineato a `D-010` |
| `ERTAbilityShape::Cone` usato da un'abilità | `git grep` su `Source/` | **zero** — l'unica occorrenza è l'implementazione della forma in `RTHexCombatLibrary.cpp` |
| Un'azione che spinge di più di una cella | `git grep -E 'Push(Distance\|Cells)?\s*=\s*[2-9]'` | **zero** occorrenze |
| Abilità citate dal kit per le run | `RTHeroCatalogLibrary.cpp` | `LinearDischarge`, `Overload`, `PressureJet`, `CircularTide`, `Hero.Riktor.Ram` — **tutte presenti** |
| Scenari citati per `PIE-HEXPLAY-6` | `Scenarios/` | `Combat/BlockedByWall.json`, `Combat/LineHitsThrough.json`, `Visual/Map/HighGroundNoBonus.json` — **tutti presenti** |
| `shares_setup_with` di `U2`…`U6` | [`editor-sessions.yaml`](../editor-sessions.yaml) | **già reciproco e completo**, nessuna correzione da fare |

**WIEGERS** — *«Un mandato che chiede di rimisurare prima di scrivere ha già fatto metà del lavoro di
qualità. Il problema di questo non è la disciplina, è la tempestività: chiede di rendere canoniche cinque
decisioni che sono canoniche da quattro giorni.»*

---

## 3. 🔴 C1 — #38 e #16 portano due DoD incompatibili, e nessuno le legge insieme

È il reperto per cui il kit vale la lettura, ed è l'unico dei suoi che sopravvive intero.

**#16, gate riscritto il 2026-08-25:**

> **Quattordici voci `PIE-HEXPLAY`, tutte ✅**: `-1` `-2` `-3` `-3b` `-4` `-4b` `-5` `-6` `-6b` `-6c` `-7`
> `-8` `-9` `-10`

**#38, DoD riscritta il 2026-08-14 e mai riallineata:**

> **Le quattordici voci `PIE-HEXPLAY` del registro hanno un esito reale**, non un'attesa. Verdi le nove che
> compongono il verdetto di M6 — `-1 -2 -3 -4 -5 -6 -7 -8 -9`; per `-3b -4b -6b -6c -10` è ammesso 🟡 **con
> la ragione scritta accanto**

Le due frasi contano lo stesso perimetro — **quattordici** — e chiedono due cose diverse su cinque di esse.
Sotto #38 la seduta chiude con `-3b` 🟡, `-4b` 🟡, `-6b` 🟡, `-6c` 🟡, `-10` 🟡. Sotto #16 non chiude.
**Quattro di quelle cinque sono fra gli otto residui**, quindi la differenza non è teorica: decide se la
prossima seduta all'Editor termina il lavoro o no.

**COCKBURN** — *«Chi esegue apre #38, non l'epic. La DoD che legge è quella permissiva. Chiuderà la issue in
buona fede, e l'epic resterà aperta senza che nessuno sappia dire quale delle due frasi era la regola.»*

**NYGARD** — *«E il fallimento è silenzioso: nessun gate rilegge due issue insieme. Il difetto si manifesta
settimane dopo, quando qualcuno chiede perché E2 è ancora aperta con #38 chiusa.»*

⚠️ **Il kit lo diagnostica correttamente** (§10, *«Non lasciare due Definition of Done concorrenti»*) e
propone di allineare #38 a #16. **Ma la scelta non è meccanica**, ed è il motivo per cui questo referto non
la esegue: la clausola 🟡 di #38 non è sciatteria, è la registrazione di un fatto — `-6b` dichiara che il
**cono non è verificabile in partita**, e la misura di oggi lo conferma (zero abilità con
`ERTAbilityShape::Cone`). Se #16 pretende `-6b` ✅ e il cono resta non osservabile, il gate pretende
un'osservazione che il roster non consente per **quella parte** della voce.

**ADZIC** — *«"✅ con una parte dichiarata non verificabile" e "🟡 con la ragione scritta accanto" sono lo
stesso stato con due nomi. La domanda da decidere non è quale issue vince: è se il registro ammette una voce
verde con un'esenzione motivata. Se sì, le due DoD collassano in una. Se no, `-6b` non può chiudersi finché
il roster non ha un cono, e va detto in #16.»*

### Correzione (decisione d'autore, non applicata qui)

Una delle due, non entrambe:

1. **#38 si allinea a #16** — la clausola 🟡 esce, e `-6b` porta in #16 l'esenzione esplicita sul cono;
2. **#38 conserva la propria semantica**, e lo **dichiara**: «CP 2.8 chiude con esito reale su 14; E2 chiude
   con 14 ✅. Sono due gate diversi, e questo è il più permissivo dei due».

La preferenza del kit è la 1. Questo referto non ha elementi per contraddirla, ma osserva che senza
l'esenzione su `-6b` la 1 produce un gate insoddisfacibile — lo stesso difetto che #16 ha già pagato una
volta con «fino alla vittoria».

---

## 4. 🔴 C2 — il `done_when` di `U6` dichiara due numeri, ed entrambi sono sbagliati per il gate corrente

[`editor-sessions.yaml`](../editor-sessions.yaml), riga 432:

```yaml
done_when: le voci `PIE-HEXPLAY` sono verdi, rilette tutte insieme — sono **15** righe, non nove, e tre lo sono gia'
```

Due difetti in una riga, e **il kit ne vede solo uno**.

- **`15`** è il numero delle righe del **registro**, non del **perimetro E2**, che è 14. Come `done_when` di
  una seduta il cui `produces` è *«chiusura di M6 / E2»*, il numero giusto è **14**: eseguire `-11` non
  avvicina la chiusura di E2, e non eseguirlo non la blocca.
- **`tre`** è una misura del **2026-08-21** presentata al presente. Oggi sono **sei**. Il kit non lo nomina:
  la sua §3.1 cerca il `15` e passa accanto al numero che invecchia più in fretta.

**DOUMONT** — *«La riga usa due numeri e non dice di cosa siano. "Sono 15 righe" è vero del registro; "tre lo
sono già" era vero di nove voci il 21 agosto. Un lettore che li prende entrambi per correnti conclude 3 su 15,
che non è mai stato vero di niente.»*

⚠️ **La stessa misura stantia compare a riga 523** — *«la famiglia ha 15 righe e tre sono gia' ✅ — `-1`,
`-3`, `-5`. Misurato il 2026-08-21»* — e lì **è corretta**, perché è **datata** e dichiarata come misura di
quel giorno. Stessa forma in [`roadmap-v0.1.md`](../roadmap-v0.1.md) riga 1675. Sono dati storici, e la §3.1
del kit lo dice bene: *«distinguere dato storico da regola corrente»*. **Solo la riga 432 è una regola
corrente**, e solo quella va corretta.

---

## 5. 🟠 A1 — `U2` dichiara i cilindri, e i cilindri sono usciti dalla partita il 2026-08-25

[`editor-sessions.yaml`](../editor-sessions.yaml), passo 2 di `U2`:

> Play. Le unita' sono **cilindri**: i `BP_Unit_*` non esistono in `Content/` e il fallback e' previsto — non
> e' un difetto.

**Falsa due volte**, e il registro lo dichiara già nella nota di `PIE-HEXPLAY-1`:

1. i quattro `BP_Unit_*` **sono versionati** — la frase era falsa già quando fu scritta; ciò che mancava era
   l'aggancio al GameMode, non l'asset;
2. dal **2026-08-25**, con [#287](https://github.com/DegrassiAaron/refactor-tactics-main/issues/287), in
   partita entrano le skeletal.

**La conseguenza non è cosmetica**: la stessa riga sopravvive in
[`m6-8-sequenza-sedute-u2-u6-2026-08-14.md`](m6-8-sequenza-sedute-u2-u6-2026-08-14.md) §1 (*«Le unità sono
cilindri e non è un difetto»*), che è la procedura che chi esegue legge davanti allo schermo. E soprattutto
**è la precondizione di `PIE-FACING-1`**, che `U6` verifica: il registro la dà per **sbloccata** dal
2026-08-25 (*«la domanda della voce è di nuovo ponibile»*), mentre le due procedure che la seduta usa
continuano a dire che a schermo ci sono cilindri.

**CRISPIN** — *«Chi apre l'Editor con questa istruzione e vede figure umanoidi non conclude "il documento è
vecchio". Conclude "sto guardando la cosa sbagliata", e la voce che dipende da quella osservazione non viene
eseguita.»*

Il kit lo sfiora nel §7 (*«il blocker storico dei cilindri dovrebbe essere superato se `BP_Unit_*` sono
correttamente agganciati»*) — al **condizionale**, e senza indicare le due righe da correggere.

---

## 6. 🟠 A2 — `U2` chiede lo scambio A↔B come «l'unico caso che i test headless non coprono», ed è falso da tre settimane

[`editor-sessions.yaml`](../editor-sessions.yaml), passo 4 di `U2`:

> Ripeti con lo **scambio diretto A↔B**: e' l'unico caso che i test headless non coprono.

Il registro dichiara il contrario, su `PIE-HEXPLAY-5`, dal **2026-08-10**: la clausola sullo scambio è stata
**rimossa dall'esito atteso** perché falsa dal 2026-08-08 — lo scambio diretto **non è pianificabile**
(`FindPathForUnit`: goal occupato → `NoPath`), quindi non c'è nulla da osservare in PIE. Ed è **pinnato
headless** da `Scenario.RunnerSwapRejectedByPlanning`, cioè esattamente il contrario di *«l'unico caso che i
test headless non coprono»*.

Chi esegue `U2` alla lettera spende tempo su una manovra che il gioco rifiuta in pianificazione, e non ha modo
di sapere se il rifiuto sia il comportamento atteso o il difetto che sta cercando.

**WIEGERS** — *«Un passo di procedura che chiede di riprodurre un caso già dichiarato impossibile è peggio di
un passo mancante: produce un'osservazione, e l'osservazione è priva di significato.»*

Il kit **non lo nomina affatto**.

---

## 7. 🟠 A3 — il registro porta un TERZO insieme, «sessione D = 9 voci», che non coincide né con 14 né con 15

[`test-manuali-pie.md`](../../technical/test-manuali-pie.md), riga 404, tabella dei gruppi:

> **Partita hex** (sessione D) | `HEXPLAY-4/4b/5/6/6b/6c/7/9/10` | **9** — è il **gate di M6** (CP 6.8)

Nove voci, e **non sono le nove numerate**: mancano `-1`, `-2`, `-3`, `-3b`, `-8`, e sono incluse quattro
delle suffissate. È un terzo insieme, che si autodichiara *«il gate di M6»* — cioè lo stesso oggetto di cui
#16 dice quattordici e `U6` dice quindici.

Il difetto era già stato registrato dal referto del 2026-08-14 (§F1, *«`PIE-HEXPLAY-1..9` e "sessione D" sono
due insiemi diversi presentati come uno»*) e **non è stato chiuso**: la riga 404 è ancora lì, ed è la riga
che un esecutore incontra **per prima**, perché sta nella tabella dei gruppi e non nel corpo delle voci.

**COCKBURN** — *«Tre insiemi, tre numeri, tre documenti, un solo lavoro. L'esecutore non ha un criterio per
scegliere, e il documento non gli dice che una scelta esiste.»*

Il kit **non lo nomina**: la sua §3.1 confronta 14 con 15 e non si accorge del 9.

---

## 8. 🟡 Medi

| # | Dove | Cosa | Nota |
|---|---|---|---|
| M1 | [`roadmap-editor.md`](../roadmap-editor.md) righe 235–239 | *«rimuovi l'arco e verifica che il path fallisca. Poi una partita intera, dall'avvio alla vittoria»* e *«Finita quando: le nove voci `PIE-HEXPLAY` sono ✅»* | **entrambe superate**: la rimozione dell'arco non si fa su `GeneratedTestArena` (lo YAML lo dichiara dal 2026-08-10, la transizione è creata da `MakeTestArena`), e «alla vittoria» cade con `D-184`. Non datate: si presentano come regola corrente |
| M2 | [`roadmap-checkpoint.md`](../roadmap-checkpoint.md) riga 232 · [`roadmap-v0.1.md`](../roadmap-v0.1.md) riga 617 | DoD di M6.8 / CP 2.8: *«Mappa di prova costruita con l'editor mode … partita completa fino alla vittoria»* e *«Sessione D: `PIE-HEXPLAY-1..9` tutte ✅»* | portano **entrambi** i requisiti ritirati — l'asset da costruire a mano e la vittoria. Sono le due righe di roadmap che il kit cerca nel §11 senza nominarle |
| M3 | [`editor-sessions.yaml`](../editor-sessions.yaml) riga 436 | *«Poi una partita intera, dall'avvio alla vittoria, rileggendo `PIE-HEXPLAY-1..9` insieme»* | stessa coppia di difetti dentro `U6`, e nello **stesso blocco** il cui `done_when` dice 15 (§4): tre insiemi diversi in venti righe |
| M4 | il kit, §2 punto 5 | cita `docs/tooling/scenario-map.md` | il file è [`docs/technical/tooling/scenario-map.md`](../../technical/tooling/scenario-map.md). Il kit dubita di sé (*«se esiste ancora con questo path»*) e ha ragione a dubitare — ma è comunque un percorso morto in un elenco di sorgenti da rileggere |

---

## 9. Cosa il kit ha ragione, e va tenuto

Nulla di quanto segue va corretto: è **accurato e già canonico**. Vale come conferma indipendente, non come
lavoro da fare.

| Sezione del kit | Verdetto | Dove era già |
|---|---|---|
| §3.1 perimetro 14, `-11` fuori | ✅ **corretto** | **#16**, gate riscritto il 2026-08-25 — con la stessa motivazione (presentazione, epic E21) |
| §3.2 una sola apertura per `U2`→`U6` | ✅ **corretto** | `shares_setup_with` reciproco nello YAML, e §1 del piano del 2026-08-14: *«una apertura, cinque sedute»* |
| §3.3 niente `L_HexArena`, usare `GeneratedTestArena` | ✅ **corretto** | primo criterio della DoD di **#38** e §0 del piano del 2026-08-14, con la Binary Asset Lease nominata |
| §3.4 UE 5.8 | ✅ **corretto** | pin di [`CLAUDE.md`](../../../CLAUDE.md) §3 — **5.8.1** |
| §3.5 `-10` chiede un esito dichiarato | ✅ **corretto** | **#16** e la riqualificazione di `PIE-HEXPLAY-10` del 2026-08-25, entrambe su `D-184` |
| §4 stato di partenza 6 verdi / 8 residui | ✅ **verificato oggi** | identico: nulla si è mosso |
| §6 «non pretendere un test `Push 2`» | ✅ **corretto** | già scritto in `-6c`; misurato: zero azioni con spinta > 1 |
| §6 «non inventare una prova Cone» | ✅ **corretto** | già scritto in `-6b`; misurato: zero abilità con `ERTAbilityShape::Cone` |
| §6 usare gli scenari per `-6` invece di allestire a mano | ✅ **corretto** | i tre scenari esistono; `Combat.BlockedByWall` è già ✅ dal 2026-08-24 |
| §8.3 «una voce manuale è ✅ solo dopo l'osservazione umana» | ✅ **corretto e importante** | è la regola del registro, e il §9 del piano del 2026-08-14 la scrive per esteso |
| §16 «storia conservata come storia» | ✅ **corretto** | è la convenzione *stato precedente* già in uso nel registro |

**Il §5 del kit — «inserire nella documentazione una procedura equivalente a questa» — chiede di scrivere un
documento che esiste.** [`m6-8-sequenza-sedute-u2-u6-2026-08-14.md`](m6-8-sequenza-sedute-u2-u6-2026-08-14.md)
copre gli stessi punti — preparazione condivisa, `MapSource`, `RoundLimit` 12, comandi letti da
`MappingContext`, che cosa catturare dal log per ciascuna voce, come si registra un esito — e in più porta due
cose che il kit non ha: **la riga di log che conferma l'allestimento** (`Board 2v2 esagonale avviata su N
celle con 4 eroi`) e l'avvertimento che *premere Spazio fino alla fine non è una run valida*.

---

## 10. ➕ La scoperta principale: il kit riscopre un lavoro già fatto, senza saperlo

Il kit non cita **nessuno** dei due referti del 2026-08-14, e ne ripercorre entrambi:

| Reperto del kit | Già registrato il | Dove |
|---|---|---|
| «sessione D» come unità indipendente è ambigua | 2026-08-14 | §F1 e §F2 del referto |
| «fino alla vittoria» chiede un esito che l'esecutore non controlla | 2026-08-14 | §F4 del referto — poi confermato da `D-184` il 2026-08-22 |
| `1..9` non è l'insieme delle voci | 2026-08-14 | §F1, *«nel registro le righe sono 14, non 9»* |
| non costruire `L_HexArena`, la Lease non è emessa | 2026-08-14 | §0 del piano di sequenza |
| una apertura, cinque sedute | 2026-08-14 | §1 del piano di sequenza |

**NYGARD** — *«Un mandato che riscopre lo stato invece di leggerlo non è inutile: è una prova
d'indipendenza. Ma il suo costo è reale — chi lo eseguisse scriverebbe una terza copia della stessa
procedura, e da domani ce ne sarebbero tre da tenere allineate.»*

⚠️ **E c'è un motivo strutturale per cui è successo**, che vale oltre questo kit: la DoD di #38 rimanda al
referto per il *perché* e allo YAML per il *come*, ma **nessuno dei due è linkato dal registro**, che è il
documento che l'esecutore apre. Chi arriva da `test-manuali-pie.md` non ha modo di scoprire che la procedura
esiste.

---

## 11. ⛔ La trappola del mandato: `15` è un numero **vero**, e il §3.1 chiede di cancellarlo

> §3.1: *«Correggere ogni `15` residuo che pretenda di essere il numero delle voci E2.»*
> §11: *«Eseguire una ricerca per … `15` …»*

Chi eseguisse questa istruzione con un `git grep 15` troverebbe **quattro** occorrenze rilevanti, e **tre
sono corrette**:

- `editor-sessions.yaml:432` — 🔴 **da correggere**: è il `done_when` di `U6`, regola corrente (§4);
- `editor-sessions.yaml:523` — ✅ **da lasciare**: nota di `U23`, **datata 2026-08-21**, dichiara la famiglia;
- `roadmap-v0.1.md:1675` — ✅ **da lasciare**: correzione datata che spiega *perché* la riga precedente era falsa;
- `#16` stessa — ✅ **da lasciare**: il blocco in testa spiega che le voci sono 15 e che il gate ne prende 14.

**Le due grandezze sono entrambe vere e diverse**: **15** è la famiglia del registro, **14** è il perimetro del
gate. Un'istruzione che tratta `15` come un errore da estirpare cancella la distinzione che #16 ha stabilito
quattro giorni fa.

**DOUMONT** — *«Il mandato ha diagnosticato correttamente l'ambiguità e ha prescritto di risolverla
eliminando uno dei due significati. La risoluzione giusta è nominarli: "quindici righe nel registro, quattordici
nel gate E2". Un solo numero non può portare due domande.»*

⚠️ Lo stesso vale, in scala minore, per `sessione D`: il kit chiede di correggere *«eventuale testo che chiama
"sessione D" una singola entità invece del gruppo U2–U6»*, ma il nome compare in **undici** punti, e in almeno
tre è una citazione storica o il titolo stesso di #38 — che non si rinomina senza rinominare la issue.

---

## 12. Le cinque correzioni che restano — e il loro owner

Ordinate per costo di non farle. ✅ **Applicate tutte il 2026-08-29**, su decisione dell'autore presa alla
consegna del referto — il consumo del kit resta *revisione + archiviazione*, l'applicazione è l'atto separato
che questa riga registra.

| # | Correzione | File | Il kit la chiede? | Esito |
|---|---|---|---|---|
| 1 | Decidere quale DoD sopravvive fra #38 e #16, ed esplicitarlo nella issue che perde | **#38** e **#16** | ✅ sì, §10 — ed è l'unico suo reperto ancora aperto | ✅ **#38 allineata a #16**: la clausola 🟡 su cinque voci esce; #16 dichiara l'esenzione sul cono |
| 2 | `done_when` di `U6`: **14** invece di 15, e togliere «tre lo sono gia'» o datarlo | [`editor-sessions.yaml`](../editor-sessions.yaml) | 🟡 a metà: vede il 15, non il «tre» | ✅ riscritto: 14 col perimetro esplicito, e lo stato non si copia più dal `done_when` |
| 3 | Togliere i cilindri da `U2` e dal §1 del piano di sequenza; nominare `#287` | [`editor-sessions.yaml`](../editor-sessions.yaml) · [`m6-8-sequenza-sedute-u2-u6-2026-08-14.md`](m6-8-sequenza-sedute-u2-u6-2026-08-14.md) | ⛔ no | ✅ entrambe, con la falsità dichiarata come *stato precedente* |
| 4 | Togliere il passo «scambio diretto A↔B» da `U2`, o riscriverlo come «verifica che la pianificazione lo rifiuti» | [`editor-sessions.yaml`](../editor-sessions.yaml) passo 4 di `U2` | ⛔ no | ✅ riscritto: il passo dice ora che il caso **non si prova**, e perché |
| 5 | La cella «sessione D» del registro: `9` non è il gate di M6 | [`test-manuali-pie.md`](../../technical/test-manuali-pie.md) | ⛔ no | ✅ allineata a **14**, col terzo insieme dichiarato come difetto corretto |

E i quattro 🟡 del §8, della stessa famiglia — *«fino alla vittoria»* e *«le nove»* in tre documenti di roadmap
che nessun gate rilegge — **applicati insieme**: [`roadmap-editor.md`](../roadmap-editor.md) (`U6`, che portava
anche la rimozione dell'arco già ritirata il 2026-08-10),
[`roadmap-checkpoint.md`](../roadmap-checkpoint.md) (`M6.8`), [`roadmap-v0.1.md`](../roadmap-v0.1.md) (CP 2.8 e
la citazione del gate al §D-145) e i passi di `U6` in [`editor-sessions.yaml`](../editor-sessions.yaml).

⚠️ **Ciò che NON è stato toccato, e perché**: il nome *«sessione D»* resta ovunque compaia — è il titolo di
**#38** e di undici punti in `docs/`, e il §11 di questo referto avverte contro la rinomina massiva; e le tre
occorrenze **datate** di `15` (§11) restano, perché sono vere del registro.

---

## 13. Stato E2 misurato

```text
E2 HEX gate: 6/14 ✅
Verdi:   PIE-HEXPLAY-1 · -2 · -3 · -5 · -9 · -10
Residui:
- PIE-HEXPLAY-3b   🟡  metà «coperto/LOS» — serve una cella con bBlocksLineOfSight
- PIE-HEXPLAY-4    ⏳  fluidità del playback (logica coperta headless)
- PIE-HEXPLAY-4b   ⏳  playback visivo del Dash (la fase è già osservata nel log)
- PIE-HEXPLAY-6    🟡  1 dei 3 esiti fatto; restano LineHitsThrough, elevazione e il giudizio UX
- PIE-HEXPLAY-6b   ⏳  leggibilità della preview di forma (cono NON verificabile: nessuna abilità lo usa)
- PIE-HEXPLAY-6c   ⏳  scivolamento della spinta (Push 1; Push 2 non esiste più)
- PIE-HEXPLAY-7    🟡  resta il giudizio sul comportamento del bot, non solo le coordinate assiali
- PIE-HEXPLAY-8    🟡  playback del cambio di quota (LayerHeight)

Fuori perimetro: PIE-HEXPLAY-11 (presentazione, E21)
```

Identico a quanto **#16** dichiara al 2026-08-25. Le otto residue sono **tutte** lavoro umano in PIE, e sette
delle otto chiedono un **giudizio visivo** che nessun test headless può dare — è il motivo per cui il gate non
si muove da solo.

---

## 14. Come è stata protetta la misura

- **Nessuna suite lanciata.** Questa passata scrive in `docs/`, e per [`D-222`](../../decisions/RT_PDR_00_Decision_Log.md)
  una `rt-suite` che gira mentre l'albero cambia esce **NON VALIDA** anche con zero fallimenti — il digest
  copre l'albero, non i soli sorgenti. Non c'era nulla da misurare col motore: il kit è documentale.
- **Working directory condivisa.** `git status` all'inizio: due soli untracked, entrambi kit esterni. Nessun
  lavoro di un'altra sessione è stato toccato. Il secondo kit
  (`Claude_RefactorTactics_Cell_Sector12_Edge6_Issues.md`) **non è stato letto né rimosso**: non è nel mandato.
- **HEAD dichiarato in testa**, con la divergenza da `origin/main` verificata e il suo unico commit ispezionato.
- **Issue lette lato server** con `gh`, non da una copia locale: #16 aggiornata `2026-08-25T10:21Z`, #38
  `2026-08-25T08:39Z`.
- **Nessun `D-nnn` preso**: questa passata non registra decisioni. Le due che servirebbero — quale DoD vince,
  e se una voce può essere ✅ con un'esenzione motivata — sono d'autore.

---

## 15. Cose non fatte

> 🔄 **Riscritta il 2026-08-29 dopo l'approvazione dell'autore.** La versione precedente diceva *«nessuna
> scrittura su GitHub, nessun documento owner modificato»*, ed era vera alla consegna del referto: il consumo
> del kit si era fermato a revisione + archiviazione. L'applicazione è arrivata dopo, come atto separato, ed è
> quella che questa sezione registra ora.

**Fatto dopo la consegna** (§12): #38 allineata a #16 · l'esenzione sul cono dichiarata nel gate di #16 · sei
documenti owner corretti · i due blocchi diagnostici in testa a #16, che descrivevano il gate precedente al
presente, marcati come chiusi.

**Non fatto, e resta tale:**

- ⛔ **Nessun C++, nessun Blueprint, nessun `.uasset`/`.umap`, nessun tuning di gameplay** — vincolo del kit,
  rispettato per costruzione: questa passata ha solo letto `Source/`.
- ⛔ **Nessuna voce manuale promossa**: il gate resta **6/14** e nessun `⏳`/`🟡` è diventato `✅`. Nessuna
  seduta PIE è stata eseguita da questa sessione — le otto residue restano lavoro umano all'Editor.
- ⛔ **Nessun `D-nnn` preso**: l'allineamento fra due DoD non è una decisione nuova, è l'applicazione di una
  già presa (il gate del 2026-08-25).
- ⛔ **Nessuna `rt-suite` lanciata**: modifiche puramente documentali, e per `D-222` una run concorrente
  uscirebbe NON VALIDA senza dire nulla di più.
- ⛔ **Nessuna rinomina di «sessione D»** e nessuna correzione delle tre occorrenze **datate** di `15` (§11).
