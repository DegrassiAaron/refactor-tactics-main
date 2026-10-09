# Consolidamento della conoscenza delle chat — secondo giro di #2606

> `CURRENT` · **Stato**: secondo giro chiuso, perimetro **non** chiuso · **Data**: 2026-09-20
> **Base di misura**: `origin/main` @ `f7aa7b32`, albero pulito.
> **Owner**: [#2606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2606)
> **Precedente**: [`chat-knowledge-consolidation-2606-primo-giro-2026-09-06.md`](chat-knowledge-consolidation-2606-primo-giro-2026-09-06.md)
> — `CURRENT`, e questo referto **non lo sostituisce**: ne rimisura i residui.
> **Cosa non è**: non è una roadmap, non ristruttura `docs/` ([#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165)),
> non tocca la navigazione delle capability ([#2325](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2325)),
> non dice nulla sullo stato della v0.1.

---

## In una riga

Il primo giro lasciava quattro residui. **Due erano chiudibili senza giudicare il contenuto di niente**, e
questo giro li chiude; il terzo è cresciuto di uno e nessuno dei sedici è stato consumato; il quarto — il
perimetro — resta la domanda dell'autore, ma **non era registrata dove #2606 prescrive** che stia.

---

## 1. La `OPEN_QUESTION` del primo giro non era registrata: #2606 non aveva applicato a sé stessa la propria tabella

Il primo giro chiude il §7 così: *«È la prima `OPEN_QUESTION` del perimetro, ed è registrata come tale
invece di essere risolta a intuito»*.

⛔ **Era registrata nel referto, e un referto non è un owner.** La tabella di classificazione di #2606 —
quella che l'issue prescrive per ogni elemento estratto da una chat — assegna a `OPEN_QUESTION` un solo
owner canonico: [`docs/OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). Misurato su `f7aa7b32`, prima di
questa passata, la ricerca di `chat` in quel file dava **una riga sola, e parlava d'altro** — un triage
del 2026-08-10.

🔑 **È il difetto che #2606 esiste per prevenire, applicato al suo stesso prodotto**: un'informazione
durevole che vive solo nel documento che l'ha scoperta. Un piano in `roadmap/plans/` è *provenance*, non
*authority* — lo dice [`README.md`](README.md) della cartella in prima riga: *«nessun documento qui è owner
di qualcosa»*.

✅ **Chiuso**: la domanda è ora `CHAT-1` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), con le tre
uscite, il costo dell'attesa e l'innesco. La formulazione è quella del primo giro: non è stata riaperta né
riscritta, è stata **spostata dove ha un owner**.

## 2. Il contenitore che non dichiarava niente ora lo dichiara

Il §6 del primo giro misura che
[`archive/consolidazione-chat-openai/`](../../archive/consolidazione-chat-openai/README.md) non aveva un
`README`, *«quindi, a differenza dei sorgenti di `archive/src/`, non ne eredita nessuno»*, e si ferma lì:
chiuderlo sembrava richiedere di giudicare le 470 righe del master.

🔑 **Non lo richiede.** Sono due affermazioni diverse, e solo la seconda è un giudizio:

| Affermazione | Come si stabilisce |
|---|---|
| *nessun owner cita questo file, nessun referto lo ha consumato* | **misura** — `git grep` e la ricerca su GitHub |
| *di queste 470 righe sopravvive X e cade Y* | **giudizio** di chi lo recepisce |

✅ **Chiuso**: la cartella ha un `README` che scrive la prima e lascia esplicitamente la seconda, nella
forma di disposizione già in uso in [`../../archive/src/README.md`](../../archive/src/README.md). ⛔ Non
dice che il contenuto sia superato: dice che **non è stato verificato**, e distingue le due cose perché
confonderle è il modo in cui un documento utile viene buttato o uno stantio viene creduto.

## 3. Il difetto misurabile del §3 è cresciuto, e nessuno dei sedici è stato chiuso

Selettore invariato, quello dichiarato dal primo giro: un sorgente è indicizzato se `README.md` contiene
una **riga di tabella** che lo linka.

| Misura | `c5412238` (2026-09-06) | `f7aa7b32` (2026-09-20) |
|---|---:|---:|
| sorgenti archiviati | 124 | **125** |
| di cui in `handoff/` | 76 | **77** |
| senza riga d'indice | 16 | **17** → **16** |

I due comandi che li producono:

```bash
find docs/archive/src -name '*.md' ! -name README.md | wc -l

for f in $(find docs/archive/src -name '*.md' ! -name README.md); do
  b=$(basename "$f")
  grep -qE "^\|.*\]\([^)]*$b\)" docs/archive/src/README.md || echo "$b"
done | wc -l
```

🔴 **Il delta è di uno in ciascuna direzione, e non si compensa:** il nuovo arrivato è
`2026-09-09-prompt-scenario-lab-non-eseguito.md`; dei sedici del primo giro ne è stato consumato **zero**.
∴ il meccanismo che produce il difetto — una sessione archivia un sorgente senza scrivere la riga — è
**attivo**, quello che lo consuma no. In quattordici giorni il saldo sarebbe stato `+1`.

✅ **Uno è stato chiuso qui, e solo perché non richiedeva un giudizio.** Il nuovo arrivato **dichiara da sé
la propria disposizione** nel banner in prima riga — `ARCHIVE` · *«Prompt conservato, NON eseguito»* — con
i tre difetti misurati dallo spec panel del 2026-09-09 che lo hanno respinto: baseline v0.1 non ferma,
controllo duplicati che non trova [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105),
e un terzo nome per la stessa superficie. Scriverne la riga d'indice è **trascrizione**, non revisione.

⛔ **Gli altri sedici restano**, e la regola che li trattiene è dell'indice, non di questo referto:

> *«una riga d'indice dichiara cosa sopravvive e cosa è falsificato di un kit, e quel giudizio appartiene a
> chi l'ha revisionato»*

## 4. Il criterio che separa i due casi, scritto perché il terzo giro non lo ridebba dedurre

I §1, §2 e §3 hanno chiuso tre cose che il primo giro aveva lasciate aperte, e **nessuna delle tre ha
richiesto di giudicare un contenuto**. La linea è questa:

| Si può fare in una passata documentale | Va a chi consuma il kit |
|---|---|
| spostare una `OPEN_QUESTION` dal referto al suo owner | **rispondere** alla domanda |
| dichiarare lo **stato del contenitore** | dichiarare cosa sopravvive del **contenuto** |
| **trascrivere** una disposizione che il file dichiara da sé | **derivarne** una che il file non dichiara |

⚠️ **Il verso sbagliato di questa linea ha già un nome nel primo giro**: il §4 registra un falso positivo
da 43 voci prodotto da un selettore che misurava sé stesso. Un giro che «chiude» righe d'indice deducendo
disposizioni produce lo stesso genere di debito, con l'aggravante che sembra lavoro fatto.

---

## Registro di decommissioning

#2606 prescrive un registro per ogni chat. **Nessuna chat è stata decommissionata in questo giro**, e il
motivo è `CHAT-1`: senza il perimetro non c'è una riga da aprire. Il registro di questa passata è quindi
sui **residui**, non sulle chat:

| Residuo del primo giro | Stato dopo questo giro |
|---|---|
| `OPEN_QUESTION` del perimetro senza owner canonico | ✅ **CONSOLIDATED** — `CHAT-1` in `OPEN_DECISIONS.md` |
| `archive/consolidazione-chat-openai/` senza stato dichiarato | ✅ **CONSOLIDATED** — `README.md` scritto |
| sorgenti archiviati senza riga d'indice | 🔄 **APERTO** — uno chiuso per trascrizione, sedici restano a chi li ha consumati |
| righe `pending repo sync` del Drive mai rimisurate | ⏳ **BLOCKED** — richiede il Drive, che questa passata non ha letto |
| perimetro delle chat | ⛔ **BLOCKED** su `CHAT-1` — owner: l'autore |

⚠️ **Nessuna chat è `SAFE_TO_DELETE`**, e questo giro non ne dichiara nessuna. Sarebbe l'affermazione senza
denominatore che `CHAT-1` esiste per impedire.

---

## Cosa questo giro NON ha fatto, e perché

| Non fatto | Perché |
|---|---|
| le sedici righe d'indice | il giudizio appartiene a chi ha consumato ciascun kit (§3) |
| una issue per `RT_Common_Actions_Master` | sarebbe un owner senza misura — invariato dal primo giro |
| rimisurare le righe `pending repo sync` | richiede il Google Sheet, non letto in questa passata: **`NOT RUN`**, non `N/A` |
| rispondere a `CHAT-1` | l'owner è l'autore, e dedurre il perimetro dalle tracce dà copertura `100 %` per costruzione |
| toccare `Source/` | nessun cambiamento di runtime: **`N/A`**, non `NOT RUN` |
| build UE, Automation, PIE, packaged | idem — la verifica di questa issue è documentale |

## Verification

| Gate | Esito |
|---|---|
| `node tools/radar/doc-links.ts --check` | **PASS** (`0`) |
| `node tools/radar/doc-links.ts --check --with-archive` | **PASS** (`0`) |
| `node tools/radar/doc-tables.ts --check` | **PASS** (`0`) |
| Compile · Automation · PIE · Packaged · Determinism · Replay · Privacy | **`N/A`** — nessun file non documentale toccato |

⚠️ **I conteggi di questo referto sono gli esiti della passata che lo scrive**, su `f7aa7b32`, e restano
scritti per questo ([`AGENTS.md`](../../../AGENTS.md) §14). Non sono lo stato corrente della cartella: per
quello ci sono i comandi del §3.
