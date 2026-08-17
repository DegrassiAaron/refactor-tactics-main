# Triage — `RefactorTactics_Facing_VisualDocs_Claude_Bundle_2026-08-13`

> `CURRENT` · **Referto di consumo** del secondo pacchetto Facing · **Data**: 2026-08-13
> **Sorgente archiviata**: [`../../archive/src/handoff/2026-08-13-facing-visualdocs.md`](../../archive/src/handoff/2026-08-13-facing-visualdocs.md)
> **Primo passaggio**: [`facing-consolidation-triage-2026-08-10.md`](facing-consolidation-triage-2026-08-10.md)

Il pacchetto conteneva un apply pack di 28 sezioni e **sette diagrammi PNG**. Non era una nuova sorgente di
verità e lo dichiarava: chiedeva di misurare `main` e riconciliare. Questo referto dice cosa è stato recepito,
cosa è stato **respinto sui fatti**, e cosa il pacchetto **non sapeva di sbagliare**.

## Esito in una riga

| | |
|---|---:|
| Decisione formalizzata | **1** (`FAC-11` → [D-126](../../decisions/RT_PDR_00_Decision_Log.md)) |
| Scenari creati | **1** su 5 proposti |
| Issue create | **1** ([#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726)) |
| Issue corrette perché stale | **2** ([#339](https://github.com/DegrassiAaron/refactor-tactics-main/issues/339), [#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172)) |
| Difetti trovati **negli asset** | **2** (nomi scambiati · decisione aperta disegnata come canone) |
| Difetti trovati **nel repository** | **3** (nota Wiki, conteggio scenari, riga E16 della roadmap) |
| Gate del registry cambiati | **0** — ed è il risultato, non una mancanza |

## 1. `FAC-11` — decisa, ma non nel verso che sembrava

Il pacchetto chiedeva di formalizzare «i sei lati sono la primitiva». È stato fatto, **e la parte difficile
della domanda ha ricevuto una risposta opposta a quella implicita nella richiesta**.

`OPEN_DECISIONS` registrava dal 2026-08-10 una riserva precisa: *«per un attaccante lontano un cono e un
insieme di tre lati non coincidono»*. Era una stima. **È stata misurata**, replicando `HexCone`/`HexLine` con
le costanti reali (`RT_HEX_DX/DY`, `RT_HEX_LINE_SCALE = 1024`) su un difensore e raggio `1..10`:

| | |
|---|---:|
| celle su cui le due letture divergono | **45** |
| divergenze «tre-lati **dentro** / cono **fuori**» | **45** |
| divergenze nel verso opposto | **0** |
| distanza della prima divergenza | **2** — e a quella distanza ce n'è **una sola**, `(1, -2)` |

⚠️ **Le prime due righe dicevano `50` fino al 2026-08-16**, ed erano la misura della regola a **linea** che
questo stesso referto ha scartato: le cifre erano sopravvissute alla regola. Corrette da [D-147](../../decisions/RT_PDR_00_Decision_Log.md). La
conclusione — contenimento stretto, quindi buff difensivo — non cambia.

Il cono è **strettamente contenuto** nell'insieme dei tre lati. Sostituire la primitiva nei consumatori
d'area non sarebbe stata una rinomina: sarebbe stato un **buff difensivo** — `Guard` che tiene e copertura
che regge su colpi che oggi le annullano — cioè un cambio di bilanciamento fatto per via lessicale.

Quindi D-126 separa i due ruoli: la **relazione a sei lati** dice *da quale lato* è arrivato un colpo, il
**cono** dice *quale area* un'unità copre. `IsInFrontalArc` non si tocca, e ADR-0005 §4 — che vieta due
definizioni di «davanti» — resta valido perché nessun consumatore d'area si sposta.

⚠️ **Il lavoro che ne nasce era davvero non posseduto**: `grep` di `RelativeDirection`/`ERTRelativeDir`/
`RelativeFacing` in `Source/` dà **zero**. È #726, `post-v0.1`.

## 2. Cosa il pacchetto proponeva e **non** è stato fatto

| Proposta | Esito | Perché |
|---|---|---|
| `Spec.Facing.SixRelativeSides` | ⛔ **non creato** | Nessuna assertion sa leggere la relazione, e il loader **rifiuta i tipi sconosciuti per scelta** (`RTScenarioLoader`: *«il nome sconosciuto è un errore dichiarato, non un ripiego»*). Uno scenario così sarebbe una scatola vuota, e l'harness diventerebbe più capace del gioco — la regola con cui [D-119](../../decisions/RT_PDR_00_Decision_Log.md) tiene `BLOCKED` il teletrasporto. Nasce nel DoD di #726, insieme alla funzione che dimostra |
| `Spec.Facing.BlockedStepKeepsFacing` | ⛔ **non creato** | Il comportamento è pinnato dai test unitari, e uno scenario end-to-end richiederebbe di **provocare** un passo bloccato: col pathfinder attuale il percorso aggira l'ostacolo invece di fermarsi, quindi lo scenario passerebbe per la ragione sbagliata. Da scrivere quando esiste un modo dichiarativo di bloccare una transizione |
| `Visual.Planning.FacingGhost` | ⛔ **non creato** | Richiede l'input della rotazione dichiarata, che **non ha un produttore**: è precisamente il residuo di [#291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/291) |
| Coppia `Spec.Overwatch.*FacingCoverage*` | ⛔ **non creata** | `Spec.Overwatch.HoldThenFire` esiste ed è `BLOCKED` su `DecisionBoundary`+`Facing`: aggiungere una coppia bloccata sulla stessa capability moltiplica i file senza aggiungere una verifica |
| Rompere `RT-FEAT-MAP-FACING` in più feature | ⛔ **rifiutata dal pacchetto stesso** | Ed è stata rispettata: **zero** gate cambiati |
| Nuova epic Facing | ⛔ **non creata** | **E16** ([#175](https://github.com/DegrassiAaron/refactor-tactics-main/issues/175)) copre il dominio ed è chiusa: #726 è collegata come *related work*, senza riaprirla |

### L'unico scenario creato

`Spec.Facing.TurningPathUsesLastCompletedStep`, che porta la cartella da **otto** scenari a **nove**. Il gap
era **reale e misurato**: degli **otto** che esistevano prima di oggi, **uno solo** aveva un `move`
(`DerivesFromMove`) ed era **rettilineo**. Su una linea dritta
«ultimo passo compiuto», «primo passo» e «direzione del viaggio» **coincidono**, quindi nessuno scenario
distingueva la regola di ADR-0008 §2 dalle sue approssimazioni. Il nuovo percorso svolta: `E` poi `NE`, e
pretende `NE`.

## 3. Difetti trovati **negli asset** — l'audit visivo non era una formalità

**(a) Due file avevano il nome scambiato.** `F4_Overwatch_Reaction_Facing.png` conteneva il diagramma
**F6 – Privacy**, e `F6_Privacy_Team_only_UI_del_Facing.png` conteneva **F4 – Overwatch**.

🔴 **Nessun controllo prescritto poteva accorgersene.** I sette SHA-256 e i sette conteggi di byte
corrispondono **esattamente** al manifest §27 del pacchetto, e l'audit richiesto era *«confronta gli hash
contro gli asset già presenti»* — che dà verde. Gli hash erano giusti: mentivano i **nomi**. Corretti in
entrambe le sedi; la tabella di provenienza è in
[`../../archive/src/handoff/2026-08-13-facing-visualdocs.md`](../../archive/src/handoff/2026-08-13-facing-visualdocs.md).

**(b) `F3` disegna una decisione aperta con la grafica del canone.** Il pannello *«Direzione in entrata»*
(`FromSource · FromTrajectory · FromImpactCenter · ExplicitDirection · NonDirectional`) è la policy di
**`FAC-13`**, aperta. Il pacchetto avvertiva su `FAC-12` per F1/F2 e dichiarava F3 «canonica dopo FAC-11»:
**non sapeva** che metà di F3 fosse aperta. Segnalato nella caption della pagina Wiki e nel README asset.

**(c) L'avvertenza su `FAC-12` non si è materializzata.** Il pacchetto temeva che F1/F2 presentassero il
pivot come **costo in punti movimento**. Verificato aprendo le immagini: **non lo fanno** — mostrano il pivot
finale come tetto, cioè ADR-0008 §1. Le due tavole sono allineate.

**(d) `F7` era datata alla consegna.** Dà `PLANNED` a `#164`, che è **chiusa**, e `MISSING` allo scenario del
percorso curvo, che **esiste dal giorno stesso**. Pubblicata come fotografia con caption esplicita e rimando
alla tabella generata; la sua legenda `GREEN/PLANNED/…` **non** è il vocabolario del registry.

## 4. Difetti trovati **nel repository**, non nel pacchetto

Il pacchetto chiedeva di correggere documenti stale. Cercandoli ne sono emersi tre che non elencava:

| Difetto | Dove | Correzione |
|---|---|---|
| Il blocco di stato pubblicato sulla Wiki diceva *«Decisione accettata (ADR-0005) ma **non implementata**»* — contraddetto dai gate della sua stessa voce (`scenario: done`, `automation: done`) e da nove scenari verdi. Il giocatore leggeva «non implementata» di una meccanica che il gioco esegue | `wiki_note` di `RT-FEAT-MAP-FACING` | Riscritta: la regola c'è, mancano **input** e **indicatore HUD** |
| La scomposizione del corpus scenari per classe era ferma al **2026-08-09** e sbagliava di **13**: `A 27 + B 21 + D 12 = 60` contro un corpus reale di **73**. Il documento **sapeva** di essere disallineato e chiedeva di rifare il conto «voce per voce» | [`../../technical/scenario-map.md`](../../technical/scenario-map.md) §Conteggio | Rifatta classificando ogni file con la regola dichiarata: `A 40 + B 21 + D 12 = 73`, verificata contro il generato |
| La riga **E16** della roadmap dichiarava `HexCone` *«una sola primitiva»* | [`../roadmap-v0.1.md`](../roadmap-v0.1.md) | Emendata con D-126 |

## 5. Decisioni che restano aperte

`FAC-3` · `FAC-5`…`FAC-9` · `FAC-12` · `FAC-13` · `FAC-14` — **nove**, ed è il numero che #339 ora dichiara nel
titolo (diceva «dieci» nel titolo e «nove» nel corpo, elencando come aperte tre voci chiuse da ADR-0008).

Nessuna di esse è stata decisa da questo apply, e in particolare **`Guard` e `Brace` non sono stati toccati**:
il pacchetto lo vietava esplicitamente, e la misura del §1 dice perché il divieto era fondato.
