# Changelog della documentazione

---

## 2026-08-12 — Ventiquattro binari diventano sei Markdown: D-009 chiusa per l'intero corpus

**Origine**: richiesta dell'utente — *«non voglio vedere pdf in docs»*, poi estesa all'ultimo `.docx`. La
regola esisteva già ed era di questo repository: PDR-00 §6 regola di manutenzione **#5**, registrata come
[D-009](decisions/RT_PDR_00_Decision_Log.md) — *«i PDF sono snapshot di consultazione: le sorgenti testuali
devono vivere nel repository Git»*. Era stata applicata a **due** documenti su ventitré.

**In `docs/` non esiste più prosa in formato binario.**

### Cosa è cambiato

| File | Modifica |
|---|---|
| [`archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md`](archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md) | **nuovo** — i tredici PDR `v0.1` in un solo documento, con la tabella «cosa vale oggi» per ciascuno |
| [`src/prd/prd-visione-e-requisiti.md`](src/prd/prd-visione-e-requisiti.md) | **nuovo** — visione, perimetro, requisiti funzionali, idee fondative |
| [`src/prd/prd-architettura-rete-e-intenti.md`](src/prd/prd-architettura-rete-e-intenti.md) | **nuovo** — architettura, stack, networking, modding, e il PRD «Intenti condivisi» integrale |
| [`src/prd/prd-personaggi-azioni-e-bilanciamento.md`](src/prd/prd-personaggi-azioni-e-bilanciamento.md) | **nuovo** — roster, azioni, terreni, coperture, catalogo di bilanciamento |
| [`src/prd/prd-percorso-didattico-e-produzione.md`](src/prd/prd-percorso-didattico-e-produzione.md) | **nuovo** — curriculum UE, roadmap, rischi, analytics, guida agli asset |
| [`src/prd/editor-griglia-esagonale-e-mappa.md`](src/prd/editor-griglia-esagonale-e-mappa.md) | **nuovo** — il prompt che ha commissionato il pivot esagonale (era `.docx`) |
| **24 binari** | rimossi dal working tree (`docs/src/prd/` ×10 PDF + 1 `.docx`, `docs/archive/pdr-v0.1/` ×13 PDF); restano nella storia Git |
| [`src/README.md`](src/README.md) · [`archive/README.md`](archive/README.md) | Indici riscritti; due affermazioni corrette (sotto) |
| [`README.md`](../README.md) · [`README.md` di docs](README.md) · [`product/piano-canonico-mvp.md`](product/piano-canonico-mvp.md) · 5 cataloghi `balance/` · ADR-0003 · Decision Log · `roadmap-v0.1.md` · `v0.1-issue-plan.md` · `RT_PDR_10_…v0.2.md` · `gameplay/spec-anima-risoluzione.md` | Riferimenti riscritti verso i nuovi target |

### Tre cose che l'estrazione ha fatto emergere, e che nessuno stava cercando

**1. `archive/README.md` rassicurava su un lavoro mai fatto.** Diceva: «*le sorgenti testuali vivono in Git
(D-009); questi restano di consultazione*». Vero per il [Decision Log](decisions/RT_PDR_00_Decision_Log.md) e
per [PDR-10 v0.2](roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md). Per gli **altri undici PDR** il PDF era
l'unica copia esistente: nessun `grep`, nessun diff, nessuna citazione per riga. Una riga di indice diceva
«fatto» di una cosa da fare, ed è il motivo per cui era rimasta da fare.

**2. Il roster canonico nasce in un documento marcato «non recepito».**
`src/README.md` classificava `idee-ruoli-characters.pdf` con «Recepito da: **—**». Ma **Flux · Riva · Bastion
· Vektor** sono lì, e le quattro abilità di Flux del [catalogo eroi](balance/RT_HeroCatalog_v0.1.md) —
`LinearDischarge`, `ConductiveNode`, `Overload`, `ReactiveCapacitor` — sono le sue, nome per nome. Il
documento era *già consumato e non registrato*: la colonna prometteva lavoro su un lavoro finito.

**3. Un PDF aveva già una trascrizione, e nessuno dei due lo sapeva.**
`sequenza-risoluzione-turno.pdf` è trascritto integralmente in
[`archive/gameplay/sequenza-turno-exploratory.md`](archive/gameplay/sequenza-turno-exploratory.md) — verificato
sezione per sezione, similarità **0,91–1,00**. È l'unico degli undici a **non** essere confluito nei nuovi
file: rimetterne il testo avrebbe creato la seconda copia che «una sola fonte logica per concetto» vieta.

### Come è stata verificata la completezza

Non «a occhio sul risultato». Ogni riga non vuota di ogni estratto è stata cercata nel montato:
**0 non ritrovate** su 7 931 per i ventidue PDF, **0 su 699** per il `.docx`. È il controllo che conta quando
l'originale viene cancellato — la somma dei byte no, perché l'header aggiunto la falsa.

### La regola che è stata riscritta, e perché non è un dietrofront

`src/README.md` diceva: «I PDF **non si archiviano**». Confondeva due assi — *dove* sta un sorgente e *in che
formato*. La posizione non è cambiata: i PRD restano in `src/prd/`, restano livello 8, restano north-star
consultati di continuo. È cambiato il formato. Un binario da 2,5 MB non è consultabile: è citabile solo per
titolo, e infatti veniva citato per titolo — anche quando il titolo era il path sbagliato, come in
`piano-canonico-mvp.md` §8.1, che per mesi ha puntato a
`docs/RefactorTactics_ Product Requirements Document e piano completo di sviluppo.pdf`, un file non più
esistente a quel percorso.

### Il `.docx` era il sorgente più consumato della cartella

`editor-griglia-esagonale-e-mappa.docx` non era un PRD: era il **prompt operativo** con cui è stato
commissionato il pivot esagonale. Da lì nascono [ADR-0002](decisions/adr-0002-griglia-esagonale.md) e le
milestone **H0–H9** di [`roadmap/hex-map-roadmap.md`](roadmap/hex-map-roadmap.md), e le sue assunzioni §2 sono
diventate tipi reali — `FRTCellId` assiale pointy-top, `URTHexMapAsset`, `ARTHexMapActor` su ISM, i due moduli
`RefactorTactics`/`RefactorTacticsEditor`. Anche le sue regole di ingaggio §1 e §23 sono sopravvissute al
documento: *«non inventare API Unreal»* e *«niente Blueprint per logica autorevole»* sono oggi in
[`CLAUDE.md`](../CLAUDE.md).

Il testo era in un `.docx` prodotto da pandoc: liste vere (`w:numPr`) e blocchi `Source Code`, non prosa
piatta. La conversione le ha ricostruite leggendo `numbering.xml` invece del nome dello stile — che in Word
localizzato cambia lingua e avrebbe prodotto 496 paragrafi sciolti al posto delle liste.

### Cosa resta binario in `docs/`

Due `.xlsx`, e sono dati tabellari, non prosa: `balance/RefactorTactics_Balance_Matrices_v0.1.xlsx`
(`RESEARCH`, [D-023](decisions/RT_PDR_00_Decision_Log.md)) e
`characters/data/RefactorTactics_Characters_Wiki_Data_v0.4.xlsx`. Restano dove sono.

---

## 2026-08-08 — Un gate per i link, che il consolidamento ha reso necessario

**Origine**: il consolidamento della voce sotto. Spostare 25 sorgenti ha rotto link in tre modi e ne ha
rivelato un quarto; tutto è stato trovato con script usa-e-getta, che è il modo migliore per non accorgersi
la prossima volta.

### Cosa è cambiato

| File | Modifica |
|---|---|
| `scripts/check-docs-links.py` | **nuovo** — gemello di `check-docs-symbols.py`: target inesistenti, **etichette che mentono**, target non versionati, debito stantio |
| [`README.md`](README.md) | La sezione «Gate anti-deriva» ora ne documenta due |
| 6 documenti | Etichette corrette: `../PDR/`, `../guides/` e uno `../spec-…` a profondità sbagliata — **cartelle che non esistono**, in `piano-canonico-mvp.md`, `RT_TerrainCatalog_v0.1.md`, `spec-durata-partita-e-scala-mappe.md`, `roadmap-editor.md` (×2), `brief-consolidamento-documentale.md` |

### Il controllo che nessuno fa

L'etichetta di un link markdown può essere essa stessa un percorso, e restare al valore vecchio mentre il
target viene corretto. Il link funziona, quindi ogni checker lo dichiara sano; il lettore legge un percorso
che non esiste. Erano **36** dopo lo spostamento, più **6** già presenti da una riorganizzazione precedente —
cartelle `PDR/` e `guides/` che il repository non ha mai avuto nella struttura corrente.

Il gate distingue però lo stile legittimo: ``[`../balance/`](../balance/README.md)`` non è un errore, perché
`../balance/` esiste. Segnala solo le etichette-percorso che **non risolvono**.

### ⚠️ Le dieci immagini «rotte» non lo erano

La voce precedente dichiarava un difetto preesistente: dieci infografiche mai committate in
`src/data/wiki/CLAUDE_INTEGRATION_PROMPT.md`. **Era falso, ed è stato scritto qui.**

Quei dieci `![…](…)` stanno dentro blocchi ```` ```md ````: sono **markdown suggerito** da incollare nelle
pagine wiki, e i loro path sono relativi alla **destinazione**, non al documento che li contiene. Erano
corretti dall'inizio. A sbagliare era lo script usa-e-getta con cui li ho misurati, che non gestiva i blocchi
recintati — e la misura sbagliata è diventata un'affermazione in tre documenti.

> **La lezione, che vale più del difetto.** Uno strumento scritto per verificare non è verificato da nessuno.
> Il gate ha trovato l'errore del proprio prototipo appena ha imparato a leggere il markdown per davvero — e
> ha trovato anche i propri due esempi nel README. Un controllo che non sopravvive alla propria
> documentazione non è pronto: **la prima cosa che deve passare è il documento che lo descrive.**

`DEBITO_NOTO` resta perciò **vuoto**, ed è la notizia buona: nel repository non c'è un solo link rotto. Il
meccanismo sopravvive perché un gate senza via d'uscita viene disattivato per intero al primo blocco
legittimo, ed è verificato per mutazione anche da vuoto.

### Verificato per mutazione

Rotta una cosa per volta, il gate becca **esattamente** quella — e, altrettanto importante, **non** becca ciò
che non deve:

| Mutazione | Atteso | Esito |
|---|---|---|
| Link a un file inesistente | fallisce | ✅ |
| Etichetta-percorso che non risolve | fallisce | ✅ |
| Target presente in locale ma non versionato | fallisce | ✅ |
| Voce di `DEBITO_NOTO` non più vera | fallisce | ✅ |
| `![img](…)` dentro un blocco ```` ```md ```` | **non** fallisce | ✅ |
| Link vero *fuori* dal fence, stesso file | fallisce lo stesso | ✅ |
| Link citato fra backtick | **non** fallisce | ✅ |

Senza mutazioni resta verde sui **1518** link del repository: zero falsi positivi.

---

## 2026-08-08 — Tre aggiunte alle Signature, trasformazioni, e `docs/src/` che si svuota

**Origine**: due sorgenti arrivati insieme in `docs/src/` — un'esplorazione sulle trasformazioni e un handoff
che chiedeva di consolidare tre estensioni al framework delle Signature Mechanics.

### Il problema

L'handoff proponeva tre concetti come nuovi: `ConditionalIntent`, `GenericActionModifier`,
`Misplay / Failure State`. **Solo uno lo era.**

| Proposta | Verdetto | Cosa esisteva già |
|---|---|---|
| `GenericActionModifier` | ⛔ nome respinto | Il repository lo chiama **profilo** dal 2026-08-08: `MoveProfile` (D-015) e «il profilo dipende dall'eroe» (D-014). Mancava solo la *regola generale*, scritta due volte per casi particolari e mai una volta per tutti |
| `ConditionalIntent` | 📅 rinviato, forma fissata | La **condizione dichiarata in planning** di D-012, che distingue il regime `Conditional` dell'Overwatch. Lo stesso predicato, spostato dal profilo di reazione all'intento |
| `Misplay / Failure State` | ✅ adottato | Nulla — ma un **caso** era già regola: il whiff della Predictive Action (D-016) |

> **La lezione si ripete.** È la stessa forma di `URTMatchFormatData` in E19: un concetto assente dall'indice
> non è un concetto assente dal repository. Il costo di cercarlo per **sinonimi** — *profilo* invece di
> *modifier* — è di qualche minuto; il costo di non farlo è una seconda verità che va poi riconciliata.
> Il documento che proponeva il nome nuovo conteneva anche la clausola che lo escludeva: «*salvo che il
> repository abbia già un nome migliore*».

### Cosa è cambiato

| File | Modifica |
|---|---|
| [`characters/_Template.md`](characters/_Template.md) | Campo **`Misplay / Failure State`** nello schema, con la regola che lo distingue dal `Counterplay` |
| `characters/v0.1/{flux,riva,bastion,vektor}.md` | Il campo compilato, **quattro modi diversi di sbagliare**: whiff nel turno · carica spesa in silenzio · struttura che persiste · superficie regalata all'avversario |
| [`characters/README.md`](characters/README.md) | Copertura del campo: 4/4 su v0.1, ⏳ v0.2, ⛔ candidati (senza kit non c'è modo definito di fallire) |
| [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) | §4-bis: il **profilo** come forma generale delle sette generiche, 7 guardrail, e dove esiste già |
| [`gameplay/brief-delayed-actions.md`](gameplay/brief-delayed-actions.md) | §7: `ConditionalIntent`, il confine con Fast Action/Reaction/Predictive/fallback, i vincoli e i 9 test |
| [`gameplay/brief-stati-personaggio-e-trasformazioni.md`](gameplay/brief-stati-personaggio-e-trasformazioni.md) | **nuovo** — owner del Character State System: cinque famiglie, complexity budget, anti-pattern |
| [`decisions/RT_PDR_00_Decision_Log.md`](decisions/RT_PDR_00_Decision_Log.md) | **D-032…D-035**. Nessun ADR: due sono schema documentale, due sono rinvii |
| [`roadmap/roadmap-post-v0.1.md`](roadmap/roadmap-post-v0.1.md) | Epic **E33** (v0.3) ed **E34** (v0.4) |
| [`azioni-e-movimento` (Wiki)](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/azioni-e-movimento) | Nota player-facing sui profili. **Nessuna feature futura pubblicizzata** |
| `docs/src/` → `docs/archive/src/` | **25 sorgenti spostati**; ~130 link riscritti in 23 file |
| [`archive/README.md`](archive/README.md) | Indicava come fonti `docs/design/piano-canonico-mvp.md` e `docs/design/roadmap-checkpoint.md`: **quella cartella non esiste** |

### Decisioni prese

- **nessuna trasformazione nella v0.1**: il sorgente ne proponeva quattro sui quattro eroi. Scope chiuso,
  rischio già alto, e due dei quattro dipendono da E13 e dal sistema strutture. `Vektor: Mobile ↔ Siege` è
  respinta anche nel merito — una forma che toglie il Dash spegne `Slancio` e la player question;
- **uno `Stance` è un profilo commutabile**, non un sistema nuovo: metà di E34 non richiede codice nuovo;
- **`docs/src/` si svuota quando un sorgente è recepito.** La convenzione precedente lo lasciava sul posto con
  un banner, e la cartella mescolava *da lavorare* e *già lavorato*: la differenza stava solo in una colonna
  di un indice. Ora sta nella posizione del file.

### Verifica dei link

I riferimenti sono stati controllati risolvendoli sul filesystem, non a occhio: **1518** link relativi, le 23
rotture introdotte dallo spostamento chiuse, e **36 etichette** che mostravano il path vecchio con il target
già corretto — link funzionanti, testo bugiardo, che nessun controllo sui soli target vede.

> ⚠️ **Una misura di questa passata era sbagliata, ed è stata corretta il giorno stesso.** Lo script usa-e-getta
> usato qui non gestiva i blocchi recintati, e ha riportato come rotte dieci immagini di
> `src/data/wiki/CLAUDE_INTEGRATION_PROMPT.md`. Stanno dentro ```` ```md ````: sono *markdown suggerito* con
> path relativi alla **destinazione**, ed erano corrette dall'inizio. Uno strumento scritto per verificare non
> è verificato da nessuno — ed è la ragione per cui il controllo è diventato subito dopo un gate versionato,
> `scripts/check-docs-links.py`, invece di restare uno script di sessione.

---

## 2026-08-08 — Il corpus diventa un test, e cinque scenari-specifica

**Origine**: seguito diretto della voce sotto. Nasce da una domanda semplice — «gli scenari servono anche
come test automatici quando si implementa?» — la cui risposta ha scoperto che non lo erano affatto.

### Il problema

`Scenario.ShippedScenariosAreValid` **carica** ogni scenario e verifica che l'ID risolva al file. Non lo
esegue. Uno scenario diventava un test solo se qualcuno gli scriveva accanto la propria
`IMPLEMENT_SIMPLE_AUTOMATION_TEST`.

I diciassette scenari della voce precedente erano stati committati **senza**. Per un commit intero sono
sembrati coperti: il verde diceva soltanto che il JSON era ben formato.

### Cosa è cambiato

| File | Modifica |
|---|---|
| `Tests/RTScenarioCorpusTests.cpp` | **nuovo** — `Scenario.EveryShippedScenarioRuns` scopre il corpus dall'indice e lo esegue tutto; `Scenario.ExpectedFailScenariosReallyFail` verifica che i dichiarati-rossi lo siano davvero |
| `Scenarios/Spec/**` | **5 scenari-specifica**: Overwatch, Objective, Predictive, Perception, CreateCover |
| [`technical/scenari-validazione-visiva.md`](technical/scenari-validazione-visiva.md) | §6-bis: il corpus come test, gli scenari-spec, e tre lacune dichiarate |
| [`technical/scenario-index-e-tag.md`](technical/scenario-index-e-tag.md) | vocabolario 20 → **22** (`perception`, `spec`) |

### Le due decisioni

**`BLOCKED` è verde.** È il meccanismo che permette di versionare uno scenario prima dei suoi sistemi:
trattarlo come rosso renderebbe irrazionale scriverne in anticipo, e si perderebbe il solo modo che il
progetto ha di dichiarare una feature futura in forma eseguibile. `FAIL` resta un difetto del **gioco**,
`ERROR` un difetto dello **scenario**, e i due messaggi lo dicono.

**Gli scenari-spec pongono la domanda prima.** `Spec.Predictive.WhiffOnEmptyCell` è l'esempio: a
implementazione finita si testa che il colpo arrivi quando la previsione è giusta, e un'implementazione che
rivaluta il bersaglio al momento dello sparo passerebbe quel test e fallirebbe questo. Scritto dopo, quel
caso non verrebbe in mente. Due di loro dichiarano anche un'**assertion che non esiste** — `TeamScoreEquals`
e un modo di asserire sulla conoscenza di squadra: meglio saperlo ora.

### Eseguito, e cosa ha trovato

Build `RefactorTacticsEditor` **Succeeded** al primo colpo. Alla prima esecuzione del corpus, **cinque
scenari su diciassette erano rossi** — e la ripartizione delle cause è ciò che rende questo test utile:

- **tre difetti miei, una causa sola**: lo scatto si dichiara con `dash` + `dashTo`, non con `ability`
  (dopo D-028 occupano slot diversi). Dichiarato con `ability`, `Bastion.Ram` finiva nello slot del Blast e
  non partiva;
- **un quarto scenario con lo stesso errore era verde**: `RoughRefusesCharge` si aspettava che non accadesse
  nulla, e non accadeva nulla — ma perché la carica non partiva, non perché il Rough la vietasse. Senza gli
  altri quattro rossi a indicare la causa comune, sarebbe rimasto a dare falsa sicurezza;
- **una mia assunzione sbagliata sulla regola**: il fallback del bersaglio mosso scatta sulla **portata**,
  non sull'allineamento. La forma `Line` descrive chi altro viene preso sulla traiettoria;
- **due difetti del gioco**, che nessun test esistente vedeva → [#241] e [#242].

`PushResistance` (**#241**) è un *dato senza consumatore*: dichiarato nel catalogo, copiato su `ARTUnit`,
verificato dai test come **valore**, e mai letto quando si applica una spinta.

La combo Riva→Flux (**#242**) è il caso più istruttivo della sessione: documentata nella showcase, con un
test verde (`Heroes.Flux.WetBonus`, che verifica l'aritmetica senza passare dal `TurnManager`), e
**ineseguibile** — il `Wet` di `PressureJet` arriva durante il Blast quando i colpi sono già preparati, e su
due turni scade nel Cleanup prima di servire.

`Visual.Combat.WaterElectric` è stato riscritto nell'unica forma che funziona — il bersaglio entra
nell'acqua in fase **Dash**, prima del Blast — ed è oggi il primo test end-to-end della combo firma.

### Limite dichiarato

`Visual.Combat.PushResistance` porta il tag `expected-fail` con la nota che ne spiega il motivo: l'assertion
resta quella **giusta**, la suite non è rossa e il difetto non è nascosto. Quando #241 verrà chiusa, sarà
`ExpectedFailScenariosReallyFail` a diventare rosso e a chiedere di promuoverlo.

Tre abilità del kit (`Riva.CircularTide`, `Riva.FluidTrail`, `Vektor.Feint`) restano **senza scenario**, e
sono elencate col perché: il loro comportamento non è derivabile dal catalogo, e un'assertion scritta sul
design invece che sul comportamento reale produrrebbe un rosso che accusa il gioco di un difetto già noto.

---

## 2026-08-08 — Corpus di scenari di validazione visiva

**Origine**: sessione di brainstorming a partire dai sorgenti `docs/src/` consolidati. Nessun payload esterno:
il materiale nuovo del 2026-08-08 (Cover Window, muri/porte) è **v0.2** — E22/E23 — e qui entra solo come
`requires` dichiarati.

### Il problema

Il corpus scenari serviva a verificare **regole**. Quando si vuole giudicare un effetto a schermo — un VFX,
una scivolata, una spinta assorbita — non c'era un insieme di fixture pensate per portare l'occhio sulla cosa
giusta, e ognuno se le allestiva a mano.

Cercando di scriverle sono emersi due limiti che nessun documento dichiarava:

1. **il canale di presentazione conosce quattro eventi** (`Move`, `Attack`, `HazardDamage`, `Defeated`).
   Nessun evento per stati, spinte, reazioni o mutazioni d'ambiente: un VFX di `Wet` non ha nulla a cui
   agganciarsi. È il difetto «dato senza consumatore» applicato alla presentazione;
2. **il formato scenario non sa esprimere** azioni con bersaglio-cella, azioni core e superfici d'autore —
   tre blocchi di feature v0.1 **già atterrate** che il corpus non può mostrare.

> ⚠️ **Un quarto limite era stato dichiarato, e non esisteva.** Il documento nasceva affermando che le
> reazioni non fossero esprimibili: vero sulla working copy in uso, **falso su `origin/main`**, dove il campo
> `reaction` esiste, è letto dal loader ed è validato contro il kit e lo slot. La differenza è emersa solo
> perché `git diff origin/main..HEAD` mostrava righe **in meno** nell'harness — cioè il ramo condiviso era
> avanti, non indietro. Da lì `Visual.Reaction.Interposition` e `Visual.Reaction.Deflection`, che senza quella
> verifica non sarebbero stati scritti.
>
> **La regola**: un limite si verifica contro il ramo condiviso, non contro la copia che si ha sotto mano.
> Altrimenti si documenta come mancante ciò che un'altra sessione ha appena costruito — ed è la stessa
> famiglia di errore degli ID `D-028`/`D-029` di poche voci fa.

### Cosa è cambiato

| File | Modifica |
|---|---|
| [`technical/scenari-validazione-visiva.md`](technical/scenari-validazione-visiva.md) | **nuovo** — owner del corpus: le tre fasce, la tavolozza delle fixture, i quattro eventi disponibili, e in §8 cosa manca in ordine di resa |
| `Scenarios/Visual/**` | **17 scenari nuovi**, i primi del progetto a riferire una **fixture** invece di generare l'arena |
| [`technical/test-manuali-pie.md`](technical/test-manuali-pie.md) | 17 voci `PIE-VIS-*`; conteggio **rimisurato col comando**: 83 (25/21/37) → **100 (25/21/54)** |
| [`technical/scenario-index-e-tag.md`](technical/scenario-index-e-tag.md) | vocabolario dei tag da 10 a **20** voci: la fotografia era indietro di **dieci**, e solo quattro erano di questa sessione |

### Decisioni prese

- **nessuna categoria nuova**: il tag `animation`, che il modello dei tag prevedeva già come *lente*, è il
  meccanismo di navigazione. `Visual.` è un prefisso leggibile, non un asse;
- **nessuna regia nel dato** (camera, pause, loop) e nessun replay come artefatto: «replay» qui significa
  *rigiocare in Visual*, che l'harness fa già;
- **nessuna validazione automatica del grafico** — l'oracolo è l'occhio. Le assertion logiche restano, e
  servono a garantire che ciò che si guarda sia lo stato giusto: senza, si può ammirare l'animazione di un
  colpo che ha mancato;
- i due scenari **muti** (`Wet`, rete conduttiva) sono deliberatamente **non scritti**: un file che si apre e
  non mostra niente insegna a diffidare del corpus.

### Limite dichiarato

Nessuno dei 15 è stato eseguito: i valori vengono dal catalogo e dal codice letto, non da un run. Tre hanno
un numero che il primo run deve confermare, e lo dichiarano nel file. **«Tutte le feature della v0.1» non è
oggi raggiungibile dal corpus**, e la parte mancante è di formato da estendere, non di scenari da scrivere.

---

## 2026-08-08 — Ownership di abilità, interazioni e sinergie (terzo passaggio)

**Origine**: payload `Docs-Consolidation-v0.9` + handoff `docs/archive/src/design/fazioni-v0.2-identita-visiva-e-roster.md`
(input, non autorità). Baseline dichiarata dal payload: `13cacb5`; **lavorato su `b057c67`** e poi **mergiato con
`367790e`**, che nel frattempo era atterrato su `main`.

> ⚠️ **Questa decisione è nata come `D-028` e si chiama `D-029`.** Due sessioni parallele hanno preso lo stesso
> ID lo stesso giorno; quella sullo **slot dello scatto** ha mergiato prima. La regola che se ne ricava:
> **l'ID di una decisione si assegna al merge, non quando si scrive la riga**. Chi arriva secondo rinumera —
> non contende, e non lo scopre in produzione.

### Il problema

La documentazione descriveva `Water-Electric` come **«combo Flux + Riva»** in più punti — Wiki, showcase,
handoff fazioni. Nel codice il bonus legge `Status.Wet` e **non conosce** l'eroe che l'ha applicato. Finché i
documenti chiamavano «coppia» una regola sistemica, la prima implementazione futura avrebbe avuto ragione a
cablare la coppia: `if HeroA && HeroB` sarebbe stato *conforme alla documentazione*.

### Decisione registrata

| | |
|---|---|
| **D-029** | Le abilità hanno **ownership singola**; le sinergie sono **derivate**. Ability/Action Definition → singolo owner · status/surface/event/geometry → sistema · combinazioni fra eroi/fazioni → esempi e fixture. Owner: [ADR-0006](decisions/adr-0006-ownership-abilita-sinergie.md) + [`gameplay/spec-ownership-abilita-interazioni-sinergie.md`](gameplay/spec-ownership-abilita-interazioni-sinergie.md) |

### Cosa è cambiato

- **42 pagine personaggio** (4 v0.1 · 4 v0.2 · 34 candidati) + i due template portano la nota
  **`Ownership del kit`**; nuove pagine `wiki/game/sinergie-e-combinazioni.md` e `wiki/fazioni/` (4 fazioni).
- Canone, `AGENTS.md`, `CLAUDE.md`, `balance/README.md`, `RT_HeroCatalog_v0.1.md`, showcase e roadmap recepiscono
  la regola. La roadmap **non** guadagna un'epic: guadagna un requisito di **DoD** (producer · consumer ·
  indipendenza dall'identità del partner).
- **Nessuna riga di runtime toccata.** L'audit di `Source/` non ha trovato `PairBonus`, `FactionSetBonus`,
  `ComboAbility` né branch `if HeroA && HeroB`. L'unico `if (Flux && Riva && ...)` è un null-guard in
  `RTHeroSpawnTests.cpp`, e la riga 157 di quel test *asserisce* che i due eroi non condividono istanze d'azione.

### Deriva collaterale chiusa: D-025

`AGENTS.md`, `CLAUDE.md`, il brief azioni e la riga 27 della matrice elencavano ancora **sei** azioni generiche
con `Guard` declassata — la forma di D-014, superata da **D-025** lo stesso 2026-08-08. Ora sono sette:
`Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`. L'economia `Attack | Ability | Overwatch`
non cambia. Era il ⏳ dichiarato nella riga 42 della matrice.

### Limiti dichiarati

Modifica **solo documentale**: nessuna build, nessun test, nessun PIE eseguito. `git diff --check` pulito,
**0 link relativi rotti** in `docs/wiki/` e `docs/characters/` (i 5 verso `src/CLAUDE_Showcase_*` sono stati
corretti da `367790e`, non da questo passaggio).

---

## 2026-08-08 — Consolidamento dei documenti **non**-Gameplay (secondo passaggio)

**Origine**: audit `docs/archive/src/audit/2026-08-08-docs-non-gameplay-v2.md`.
73 file in scope (tutto `docs/` **escluse** `gameplay/`, `src/`, `archive/`). Baseline: `HEAD 3335e36`,
**419 test unici in 64 file**.

Il primo passaggio aveva sistemato la **struttura**; questo trova il problema opposto — documenti nella
struttura giusta che contengono ancora stato storico, snapshot intermedi e correzioni stratificate.

### Decisioni registrate

| | |
|---|---|
| **D-020** | Un'azione con bersaglio **orienta l'unità prima di risolvere**; timeline del facing a sei punti. Emenda [ADR-0005](decisions/adr-0005-orientamento.md) §1–§2 |
| **D-021** | Una Decision Window privata **non è deducibile nemmeno dal tempo**: la sospensione logica resta globale, la presentazione avversaria no. Estende l'invariante #6 |
| **D-022** | **UE 5.8.1** consolidata; chiude `D-007` |
| **D-023** | Il workbook `.xlsx` è **`RESEARCH`**; i cataloghi `.md` sono i dati canonici |
| **D-024** | La quota **non aumenta il danno**; completa D-018 sul lato danno |
| **D-025** | **Sette** azioni generiche: `Guard` torna universale, `Activate` resta assorbita da `Interact`. Emenda D-014 |

### Il paradosso di governance

`README.md` dichiarava «il canone prevale su tutto», ma ADR-0004 e ADR-0005 *correggevano* il canone. Ora la
regola è esplicita: **un ADR accettato si recepisce nel canone nello stesso commit**, e un ADR accettato ma non
recepito è un difetto registrabile. Aggiunta la tassonomia dei tag — `CANONICAL`, `CURRENT`, `AS-BUILT`,
`DELIVERED PLAN`, `HISTORICAL`, `RESEARCH`, `OPEN` — con la regola che uno storico «sbagliato» non si corregge:
il difetto è lo storico *senza etichetta*.

### Contraddizioni interne trovate, non ereditate

- **ADR-0004** diceva due cose incompatibili: la §5 sospende la simulazione «così chi guarda vede il mondo
  fermarsi», la §7 che l'avversario non riceve nulla. Una pausa globale osservabile **è** il dato. → §7-bis.
- **`roadmap-v0.1.md`** conteneva due viste dello stesso stato, ognuna correttiva dell'altra: nella stessa
  pagina E8 risultava «da costruire» **e** «chiusa», con due «stato in una riga» diversi. → una sola tabella.
- **`test-manuali-pie.md`** dichiarava 34 voci aperte in testa e «le 31 aperte, somma verificata
  `2+9+9+4+4+1+2 = 31`» venti righe sotto. La somma tornava **con sé stessa**.
- **D-014 vs l'handoff** si contraddicevano sulle azioni generiche, chiusi lo stesso giorno: unico `CONFLICT`
  vero del passaggio, risolto dall'autore → D-025.

### Documenti che non descrivevano più il progetto

`test-automatico-unreal.md` era ancora il **prompt di implementazione** dell'harness (1114 righe di «TASK —
progettare e implementare»), e proponeva un `ARTTestDirector` che non è mai stato costruito → prompt in `src/`,
spec as-built al suo posto. `debug-vs-unreal.md` era una guida **M1 quadrata**. `spec-pathfinding-pf3-pf4.md` e
`spec-mappa-multilivello.md`, indicate come owner correnti, descrivevano `FRTGridCoord` a 4 vicini → riscritte
dal codice, corpi originali in `archive/technical/`. `architettura-codice.md` elencava 25 classi su 40 e dava
per «north-star da costruire» PF.4 e le reazioni, entrambe fatte.

### Divergenze documento↔codice registrate, non nascoste

Tre restano aperte come issue, perché la decisione è presa e la migrazione no:
`Action.Sprint` è in `FastMovement` e consuma due slot mentre D-015 lo vuole profilo del `Move`;
`OccupantDamageBonus` è generico e ogni call site passa `0`, ma il test si chiama `...WithTerrainBonus`;
`Overwatch` è deciso da D-012 e compariva **zero volte** in `balance/`.

### Riclassificazioni

20 piani in `roadmap/plans/` con header uniforme (corpi invariati) · `roadmap-editor.md` **ritirata** (terza
vista di stato da allineare a mano) · `hex-map-roadmap.md` congelata · `v0.1-issue-plan.md` a snapshot ·
`use-case-list.md` e l'handoff in `archive/session-notes/` · `plan-turnlog.md` e
`brief-consolidamento-documentale.md` in `roadmap/plans/`.

### Verifiche

`python scripts/check-docs-symbols.py` ✅ · **916 link relativi, 0 rotti** · test count rimisurato ·
ricerca dei termini obsoleti sui soli documenti `CURRENT`/`CANONICAL`.

---

## 2026-08-08 — Consolidamento di `docs/gameplay/`

**Origine**: audit `docs/archive/src/audit/2026-08-08-docs-gameplay.md`, con
`D-014`…`D-019` approvate. 23 file passati in rassegna.

### Decisioni registrate

| | |
|---|---|
| **D-014** | Generiche canoniche `Wait · BasicAttack · Interact · Brace · Move · Overwatch`; `Activate`→`Interact`; `Guard` non più universale |
| **D-015** | `Sneak · Normal · Sprint` sono **profili di `Move`**; `Sprint` **non è un Dash** |
| **D-016** | **Un** thin slice di Predictive Action nella v0.1 (`Vektor.InterceptShot`) |
| **D-017** | `Intercept` **rivalida la geometria sul bersaglio effettivo** |
| **D-018** | `HighGround` **senza bonus numerico** alla vista in v0.1 |
| **D-019** | `Fast Action` ≠ `Fast Reaction` — categorie distinte sulla stessa `DecisionWindow` |

### `spec-sequenza-turno.md` riscritta come **spec canonica del round**

Diceva due cose opposte: §2 che le finestre live erano in scope (C1 chiuso da ADR-0004), §4 e §5 di «non
implementare finestre live nell'MVP» perché «serve il multiplayer». La seconda era in forma di **divieto**, la
più facile da prendere per buona. Il gate a due condizioni è caduto il 2026-08-07 — la ragione di gameplay
esisteva (bait/bluff non recuperabili con condizioni dichiarate) e la riconciliazione **(b)**, già scritta lì
come ipotesi, si è rivelata sufficiente.

Ora il documento è normativo e corto: sequenza, tassonomia temporale, ordine deterministico, confine
corrente/north-star. La storia è compressa in fondo.

### Il difetto trovato guardando il codice

**APNAP a sei gruppi non esiste.** Il canone §5.1 lo dichiara vincolante come `FR-RESOLVE-01`, ma
`grep -rn "APNAP\|FR-RESOLVE" Source/` trova **un solo commento**. L'ordine reale è quello della coda azioni —
`MacroPhase → Priority → ActionId → SourceUnitId → EventSequence` — che assorbe la parte *intra-gruppo* di
APNAP ma non la partizione per appartenenza.

Non è urgente: il canone dichiara l'implementazione *gated*. Ma è la solita forma — **una regola normativa che
nessun consumer legge** — e ora è scritta, non da dedurre. Registrata come riga `OPEN` nella matrice.

### Altre correzioni di fatto

| | |
|---|---|
| `spec-anima-risoluzione.md` | Assumeva «lock-in calcola tutto una volta sola». Con ADR-0004 al lock-in **il futuro del round non è ancora scritto**: riscritta a segmenti, con la durata del round che smette di essere calcolabile in anticipo |
| `spec-dash.md` | Elencava «dash = pathfinding (aggira ostacoli)» fra le **decisioni prese**, smentito quattro righe sotto. Rimosso dall'elenco, non barrato |
| `spec-propagazione-elettrica-cp83.md` | Dichiarava `Action.Electrify` **assente dal catalogo core**. Oggi **esiste** (`RTCatalogLibrary.cpp`); nessun eroe la usa |
| `brief-conoscenza-parziale.md` | §10.1 dichiarava **chiusa** la vista da quota con `Sight_Mod = +1/+2/−1`. **D-018 la chiude nel verso opposto**: il numero veniva dal workbook, non da un playtest |
| `brief-conoscenza-parziale.md` | Il veto `D15` sulle finestre acustiche poggiava su `D7`, caduto. Sostituito da un permesso **condizionato**, non da un obbligo |
| `spec-motore-azioni-e4.md` | Diceva ancora «proposta da approvare» a epic chiusa. Ora **AS-BUILT**, con la sezione delle decisioni che l'hanno superata |

### Collisione di ID risolta

I brief usano ID **locali** `D1`…`D22`, il Decision Log usa `D-001`…`D-019`. Le sigle si sovrappongono: il
`D14` di `brief-conoscenza-parziale` è la propagazione a flood fill, il `D-014` globale sono le azioni
generiche. Regola scritta in testa a entrambi i brief: **il trattino distingue** — `D-0xx` globale e
vincolante, `Dxx` locale.

### Archiviati

`spec-bot-utility.md` · `spec-knockback.md` · `spec-terreni.md` · `sequenza-turno.md` → `archive/gameplay/`.
L'ultimo rinominato `sequenza-turno-exploratory.md`: due file quasi omonimi in cartelle diverse erano una
trappola di lettura.

### Cosa NON è stato fatto, deliberatamente

Nessuna migrazione di Stable ID. `Action.Guard`, `Action.Activate` e `Action.Sprint` **esistono e sono
consumati** — `Action.Sprint` in 9 file di codice e 6 di test. La tassonomia di D-014/D-015 è **semantica di
gameplay**, non un rename già avvenuto: farlo in una PR documentale romperebbe test e replay. Tracciato come
issue.

---

## 2026-08-07 (2) — Chiusura delle cinque decisioni aperte

**Origine**: sessione `/sc:brainstorm`. Chiude `OD-1`…`OD-5` aperte poche ore prima dalla revisione documentale.
La matrice dei conflitti passa a **0 `OPEN`, 0 `CONFLICT`**.

| Decisione | Esito |
|---|---|
| **D-011** | Formato principale **non deciso**: `D-001` declassata da *Consolidata* ad *Assunzione da bloccare*. Il 3v3 resta baseline, il 4v4 è **solo** stress test |
| **D-012** | L'Overwatch è **universale** e **compete** con l'azione offensiva. I tre regimi emergono dai dati, **non** da un enum di policy |
| **D-013** | Un trigger su transizione è **possesso della trap**, non della mappa. `FRTHexEdge` resta per i soli salti di layer |
| **E17** | Nuova epic: validazione di stress 4v4 (3 CP), dopo E15. Totale **17 epic, 85 checkpoint** |
| Nuovi brief | [`gameplay/brief-unita-ausiliarie.md`](gameplay/brief-unita-ausiliarie.md) · [`gameplay/brief-azioni-generiche-overwatch.md`](gameplay/brief-azioni-generiche-overwatch.md) |

### Due domande erano mal poste — e lo ha detto il codice, non i documenti

- **`OD-4`** chiedeva «gli archi del grafo portano trigger?», ed era etichettata come gate di **E9**. Ma gli
  archi degli adiacenti **non esistono**: `URTHexPathLibrary::GraphNeighbors` li calcola da
  `URTHexLibrary::Neighbors`, e solo le transizioni fra layer sono `FRTHexEdge`. La domanda vera — *dove vive
  la coppia `(From→To)`* — ha una risposta che non tocca la mappa, non ne versiona il formato e **non ha
  scadenze**.
- **`OD-1`** era etichettata **bloccante**. La dimensione della squadra è un `TArray<FName>` su `ARTGameMode`,
  non un campo di `FRTMatchRules`: in v0.1 non si costruisce né un 3v3 né un 4v4. Bloccava la *documentazione*.

Conseguenza di metodo: l'urgenza era stata dedotta **dai documenti** e l'ordine di priorità ne era uscito
sbagliato. L'unica decisione che bloccava lavoro costruibile era `OD-3`.

### Una contraddizione evitata

Il brief sulle azioni generiche, come scritto la prima volta, introduceva
`enum ERTOverwatchResolutionPolicy { Automatic, Conditional, FastSelect }` copiandolo dal documento sorgente.
[`roadmap/roadmap-v0.1.md`](roadmap/roadmap-v0.1.md) §E14 rischio (b) aveva **già deciso il contrario**: un enum
di policy affiancato ad `AllowedResponses` sarebbe una seconda verità sullo stesso comportamento.
Riformulato: i tre regimi **emergono** da `AllowedResponses` più una **condizione dichiarata in planning**, che
è l'unica aggiunta reale rispetto ad ADR-0004.

### Deriva corretta nel passaggio

`brief-overwatch-reazioni.md` conteneva una tabella di checkpoint **duplicata** e divergente da quella della
roadmap: 5 checkpoint contro 6, perché la rinumerazione che ha inserito `CP 14.2` non era mai tornata sul brief.
Le issue `#161`–`#166` seguono la roadmap. La tabella è stata **rimossa** dal brief, che ora possiede le
decisioni e non il piano.

---

> **Cosa registra**: le modifiche **strutturali** alla documentazione — spostamenti, nuovi owner, correzioni di
> fatti sbagliati. **Cosa non registra**: le revisioni di contenuto di una singola regola, che vivono accanto
> alla regola stessa come blocco `⚠️ Revisione <data>` nel documento che la possiede. Il changelog di una
> decisione sta dove sta la decisione; qui sta solo la storia dell'impalcatura.

---

## 2026-08-07 — Riorganizzazione in `product/gameplay/technical/balance/roadmap/decisions/`

**Origine**: revisione `/sc:spec-panel` su `docs/archive/src/handoff/consolidamento-prd-source-of-truth.md`,
registrata in [`brief-consolidamento-documentale.md`](roadmap/plans/brief-consolidamento-documentale.md).
**HEAD di partenza**: `50159c6`.

### Struttura

**101 file spostati.** `docs/design/` (72 file) si è dissolta nelle cartelle per dominio; `docs/HUD/`,
`docs/guides/`, `docs/Data/` e `docs/PDR/` sono state assorbite.

| Da | A | Criterio |
|---|---|---|
| `design/piano-canonico-mvp.md`, `showcase-v0.1.md` | `product/` | visione e canone |
| `design/spec-*` di regole di gioco, `brief-*` di gameplay | `gameplay/` | cosa fa il gioco |
| `design/spec-*` di implementazione, `HUD/`, `guides/`, `Data/use-case-list.md` | `technical/` | come è costruito |
| `design/balance/`, `Data/*.xlsx` | `balance/` | i numeri, in un posto solo |
| `design/roadmap-*`, `v0.1-*`, `PDR/RT_PDR_10_*.md` | `roadmap/` | pianificazione ed esecuzione |
| `design/h5*`, `cp6-*`, `plan-*`, `pacing-turno-plan`, `handoff-*` | `roadmap/plans/` | piani consegnati, storico |
| `design/adr-000*`, `PDR/RT_PDR_00_Decision_Log.md` | `decisions/` | decisioni con motivazione |
| `PDR/*.pdf` | `archive/pdr-v0.1/` | snapshot di consultazione |
| `HUD/*.png` | `technical/img/` | riferimenti visuali |
| `guides/*.docx` | `src/` | sorgente orfano, non normativo |

**Link riscritti in modo programmatico**, non a mano: 474 link relativi verificati, **0 rotti**. Aggiornati
anche i 3 file di radice (`CLAUDE.md`, `AGENTS.md`, `README.md`) e i **13 file C++** che citano un percorso di
`docs/` in un commento.

**Due deviazioni dichiarate** rispetto alla struttura di riferimento: i nomi dei file restano in italiano
kebab-case invece di `UPPER_SNAKE` inglese (convenzione consolidata del repository), e `src/` sopravvive come
casella dei sorgenti grezzi, esplicitamente non normativa.

### Nuovi documenti

| File | Ruolo |
|---|---|
| [`README.md`](README.md) | **Punto d'ingresso**: gerarchia delle fonti, tabella *concetto → owner*, le risposte brevi |
| [`DOC_CONFLICT_MATRIX.md`](DOC_CONFLICT_MATRIX.md) | 26 conflitti con stato e fonte che prevale |
| [`OPEN_DECISIONS.md`](OPEN_DECISIONS.md) | `OD-1`…`OD-5`: cosa aspetta una persona |
| *questo file* | storia strutturale della documentazione |

### Fatti sbagliati corretti

Otto difetti **misurati**, non ipotizzati. Il pattern è uno solo: **i documenti normativi non vengono riletti
dopo un refactor**, e restano leggibili e falsi.

| # | Difetto | Correzione |
|---|---|---|
| D1 | `piano-canonico-mvp.md` §5 elencava **4 classi su 10 inesistenti** (`ARTGameState`, `URTTurnResolver`, `URTGridLibrary`, `URTAbilityData`) e l'invariante #2 citava `FRTGridCoord`, rimosso al CP 6.1 | tabella riallineata al codice con comando di verifica riproducibile; invariante #2 corretto a `FRTCellId`. Estesa a `CLAUDE.md`, `AGENTS.md` e alle due righe stantie di `technical/architettura-codice.md` |
| D2 | `docs/README.md` non esisteva | creato |
| D3 | `CLAUDE.md`, `AGENTS.md` e `archive/README.md` linkavano `docs/SuperClaude_...md`; il file è in `docs/src/` | link corretti |
| D4 | `brief-delayed-actions.md` affermava che `RTReactionLibrary` **non esiste**. Esiste: epic E5, 27 test, dichiarata ✅ in `roadmap-v0.1.md` lo stesso giorno | rettifica in linea, osservazione barrata |
| D5 | `brief-conoscenza-parziale.md` avvertiva che il workbook di bilanciamento non era versionato. Lo era | avviso superato; il workbook è ora in `balance/`, accanto ai cataloghi |
| D6 | 3 sorgenti in `src/` erano **untracked**, incluse le due che chiedevano il consolidamento | versionati |
| D7 | 8 documenti descrivevano il substrato **quadrato** senza dirlo | banner in testa, **distinti per natura**: ⚠️ *Superato* (4), 📦 *Piano consegnato* (1), ℹ️ *Regola vigente, esempi datati* (3). Non tutti erano superati: appiattirli sarebbe stato un errore opposto |
| D8 | `progettazione-hud.md` referenziava un PNG inesistente | corretto su `technical/img/UI-style-guide.png` |

### Conteggio dei test riallineato

Le due viste di roadmap dichiaravano **366 test in 55 file**; la misura al commit `50159c6` dà
**390 in 61 file** (+24, dal primo blocco dell'harness degli scenari) — e **397 in 62** dopo il merge con `main`,
dove sono atterrati i 7 test di CP 9.1. È la terza volta che questo scarto si
apre — 2026-08-05 (−3), 2026-08-07 (−145), ora (−24) — e ogni volta perché il numero è stato *citato* invece
che *misurato*. Il comando resta quello dichiarato nei due documenti:

```bash
grep -rhoE '"RefactorTactics\.[A-Za-z0-9_.]+"' Source/RefactorTactics/Tests/*.cpp | tr -d '"' | sort -u | wc -l
```

### Gate anti-deriva

Nuovo: [`scripts/check-docs-symbols.py`](../scripts/check-docs-symbols.py). Fallisce se un inventario di classi
in un documento normativo cita un simbolo non **dichiarato** in `Source/`. È la sola parte della «Fase G» del
documento sorgente che si possa automatizzare in modo affidabile: i link rotti si vedono, un simbolo inesistente
no.

Due decisioni prese costruendolo, entrambe emerse da un **test di mutazione** (reintrodurre D1 e verificare che
cada):

1. **Conta le dichiarazioni, non le occorrenze.** La prima versione considerava esistente qualunque simbolo
   apparisse in `Source/` — e lasciava passare `URTGridLibrary`, viva solo dentro un commento che ne spiegava
   la rimozione. Il gate si autoassolveva proprio sui simboli che gli interessano di più. I simboli noti sono
   passati da 542 (occorrenze) a **141** (dichiarazioni).
2. **Controlla solo gli inventari di classi**, non la prosa. Una prima versione più larga produceva 37
   segnalazioni, quasi tutte legittime: simboli *futuri* nei DoD dei checkpoint, modelli north-star dei PDF,
   frasi che documentano correttamente una rimozione. Tarare le espressioni regolari finché il gate diventava
   verde sarebbe stato l'antipattern peggiore — un gate verde perché ha smesso di guardare. Meglio stretto e
   affidabile che largo e ignorato.

Verifica di mutazione registrata: reintrodotti uno alla volta i quattro simboli di D1 nella tabella del canone
→ **4 su 4 rilevati**; ripristino → verde.

### Cosa **non** è stato fatto, deliberatamente

- **Nessuna epic nuova**: il rischio di scope della v0.1 è già `H/H` con 82 checkpoint.
- **Nessuna decisione al posto di una persona**: i 4 conflitti irrisolti sono in `OPEN_DECISIONS.md`, non
  chiusi per plausibilità.
- **Nessun brief per le tre aree scoperte** (unità ausiliarie, azioni generiche/Overwatch universale, trap
  persistenti): sono in `OPEN_DECISIONS.md` come `OD-2`, `OD-3`, `OD-4`.
