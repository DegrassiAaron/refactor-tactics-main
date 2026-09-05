# Roadmap — eseguire le PIE rimaste

> `CURRENT` · **Creato**: 2026-09-03 · **Owner**: questo file, fino a quando le sedute che elenca sono aperte.
> **Misurato su** `origin/main` **dopo il merge di 8 commit** (vedi la nota della §1). ⚠️ **Questa riga
> diceva `2eb4ace2` e lo ha detto per mezza giornata**: è la base su cui il file è nato, e i suoi numeri
> non valgono più — chi rieseguisse il comando canonico contro quel commit otterrebbe `200 · 98 · 68 · 30`
> e concluderebbe che le tabelle sono sbagliate. 🔑 **Un documento che dichiara la propria base di
> misura deve aggiornarla quando rimisura**, o la dichiarazione diventa la bugia meglio nascosta del file. Il registro degli esiti resta
> [`test-manuali-pie.md`](../../technical/test-manuali-pie.md); *quale voce e quando* resta
> [`editor-sessions.yaml`](../editor-sessions.yaml). Questo file dice **in che ordine** e **cosa manca**.

## 🔑 La risposta, prima di tutto il resto

La domanda era *«quante issue dobbiamo implementare per eseguire le PIE rimaste?»*.

Per la seduta più ricca — **U42, ventuno voci in undici Play** — la risposta misurata è **zero**.

`unblocked_by: []`, e le sue quattro issue (`#231 #233 #1919 #2009`) sono **tutte CLOSED**. I 29 file di
`Scenarios/Visual/` esistono, e girano: `Scenario.EveryShippedScenarioRuns` e
`Scenario.ShippedScenariosAreValid` sono **`Success`** (run VALIDA del 2026-09-03, `168/168`).

⚠️ **Il collo di bottiglia non è l'implementazione: è che nessuno convoca la seduta.** Il titolo di U42 lo
dice da sé — *«Il corpus Visual, diciotto scenari che nessuna seduta convocava»*.

---

## 1. Lo stato, in numeri

| | |
|---|---|
| voci totali · aperte | **204** · **102** |
| aperte **schedulate** in una seduta | **73** |
| aperte **fuori da ogni seduta** | **29** |
| aperte che dichiarano un ostacolo | 21 — ⚠️ **non rimisurato**, vedi la nota |

> 🔄 **Rimisurato il 2026-09-04 su `main`, dopo #2228.** Dicevano `203 · 101 · 69 · 32`; il comando
> canonico conta `204 · 76/24/2/102`, e l’incrocio con `editor-sessions.yaml` dà **73 schedulate** e
> **29 orfane**. 🔑 **Lo scarto sulle schedulate non veniva da voci nuove**: `U45` era già nel file con
> tre voci (`PIE-V01-ARENA`, `PIE-OBJ-PUNTI`, `PIE-V01-BOARD`) e la tabella non la contava.
> ⚠️ **Le voci con ostacolo dichiarato restano non rimisurate**, per la ragione già scritta: contarle
> chiede di leggere il testo di ogni riga, non un marcatore.

> ✅ **Riconfermato dopo il merge di `origin/main` (4 commit): `204 · 102 · 73 · 29`, i quattro
> numeri NON si muovono.** ⚠️ E una conferma vale la misura quanto una correzione: il merge non ha
> chiesto attenzione — nessun conflitto, nessun marcatore — e l'unica cosa che avrebbe segnalato uno
> scarto era **rieseguire il comando**.
>
> 🔄 **Rimisurato il 2026-09-04 su `35816207`: `204 · 102 · 73 · 29`.** Totale e aperte salgono di due
> rispetto alla misura precedente — `main` è avanzato ancora, ed è il dodicesimo giro dello stesso
> meccanismo che questo file già registra.
>
> 🔴 **E lo scarto sulle schedulate NON era `main`: era il mio criterio di misura.** La prima stesura
> di questa passata contava una voce come schedulata se il suo nome compariva **in qualunque punto** di
> `editor-sessions.yaml` — una sottostringa, non un match. Dava `74 · 28`, cioè **38 falsi positivi**:
> `PIE-VIS-HIGH` risultava schedulata perché esiste `PIE-VIS-HIGHCOVER`, e `PIE-AI-01`…`-05`,
> `PIE-GBX-*`, `PIE-BAL1` comparivano nella **prosa** delle sedute senza essere in nessun `verifies:`.
>
> ⚠️ **E la correzione ovvia sbagliava dall'altra parte.** Contando solo le righe `- PIE-…` sotto
> `verifies:` il numero crollava a **36**: quel campo esiste in **due** formati — lista a blocchi e
> **inline** `verifies: [PIE-AS2, PIE-FACING, PIE-AS4a]` — e leggerne uno solo perde metà delle sedute.
> ∴ il numero buono viene da entrambi i formati con match **esatto**: `73` schedulate, `29` orfane.
>
> 🔑 **Tre metodi, tre risposte — `28`, `66`, `29` — sullo stesso identico file**, ed e' la stessa
> lezione che questo documento porta già due volte: *il criterio di misura decide il numero*. Qui non
> l'ha rivelato un confronto con la realtà, ma il **disaccordo fra due misure**: se avessi fatto solo la
> prima, `28` sarebbe finito qui dentro senza che nulla lo contraddicesse.
>
> 🔄 **Rimisurato il 2026-09-03 su `31859e13`, e i quattro numeri erano già invecchiati prima di
> questa passata.** Dicevano `200 · 98 · 68 · 30`; la base senza le modifiche di oggi misura
> `201 · 99 · 66 · 33`. Questo file è stato scritto **stamattina** su `2eb4ace2`: fra le due misure
> `main` è avanzato, ed è il difetto che il registro delle PIE documenta da undici giri — *il numero si
> ricalcola, non si aggiorna a mente*, e vale anche per un documento nato lo stesso giorno.
>
> ➕ **Il contributo di questa passata è esattamente uno**: entra `PIE-V01-SHIELD`, schedulata in `U18`
> ([#1403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1403)). Totale e aperte salgono
> di uno, le **schedulate** di uno, e le **orfane non si muovono**: 33 prima, 33 dopo. ⏱️ *Rimisurato il 2026-09-04: le orfane sono **32** — `PIE-V01-PACKAGED` è entrata in `U23`, che aveva `verifies: []` pur avendo un piano e cinque issue.*
>
> ⚠️ **Le «21 con ostacolo dichiarato» non sono state rimisurate** e restano come le trovo: contarle
> chiede di leggere il testo di ogni riga, non un marcatore, e nessuno strumento qui lo fa. Il numero
> è quello di stamattina e va trattato come tale.
>
> ✅ **Rimisurato una seconda volta dopo aver mergiato `origin/main` (8 commit, merge PULITO), e i
> numeri NON si muovono**: `202 · 100 · 67 · 33` prima e dopo. 🔑 **E si sa perché, che è diverso dal
> constatarlo**: dei tre file toccati, il registro ha ricevuto **testo e nessuna riga di voce**, e la
> seduta nuova `U44` ha `verifies: []`, quindi non sposta né le schedulate né le orfane. ⚠️ Il merge non
> ha chiesto attenzione — nessun conflitto, nessun marcatore — ed è il caso in cui l'unica cosa che
> segnalerebbe uno scarto è **rieseguire il comando**: una conferma vale la misura quanto una correzione.
>
> 🔑 **Metodo**: stesso criterio del comando canonico del registro — le **righe di tabella**, non un
> `grep` sugli ID. ⚠️ La prima stesura di questa misura usava `[A-Z0-9-]` per l'identificativo e
> perdeva **dieci** voci, quelle con una minuscola (`PIE-AS4a`, `PIE-BU2c`, `PIE-HEXPLAY-3b`): dava
> `192` dove il comando canonico dice `202`, e sarebbe passata inosservata se i due numeri non fossero
> stati confrontati.

⚠️ *«Senza ostacolo dichiarato»* **non** significa eseguibile: significa che la voce non dice di essere
bloccata. **Ventinove** di esse non sono in nessuna sequenza, ed è la condizione che
`spec-tactical-designer.md` §9 descrive come *«tende a non essere mai eseguita»*.

## 2. Le sedute, in ordine di resa

| seduta | voci aperte | `unblocked_by` | issue | verdetto |
|---|---|---|---|---|
| **U42** | **21** | `[]` | #231 #233 #1919 #2009 **CLOSED** · #2187 la convoca | 🟢 **eseguibile ora, e sono 11 Play non 21** |
| **U43** | **7** | `[]` | #151 (EPIC aperta) | 🟡 da verificare voce per voce |
| U25 | 6 | `[U21]` | #1095 aperta | 🔴 bloccata da una seduta |
| U5 | 5 | `[M6.6, M6.7]` | — | 🔴 bloccata da due checkpoint |
| **U18** | **5** | `[]` | #450 #567 #583 #551–554 | 🟢 **non attende nulla**, come U42 |
| U19 · U22 | 4 ciascuna | — | varie | 🟡 |
| **U45** | **3** | `[E12]` | — | 🟡 attende un checkpoint, non una seduta |
| **U39** | **1** | `[U21]` — già soddisfatto | #1920 aperta | 🟢 **eseguibile**: `GenerateIntoAsset` allestisce la mappa in un gesto — vedi in coda alla §4 |
| **U46** | **6** | `[]` | #1873 · #1246 — nessuna delle due la blocca | 🟢 **eseguibile ora**: quattro sono le uniche voci non verdi del gate `G9`, due chiudono altrettanti scenari orfani |
| altre 14 sedute | 1–2 ciascuna | — | varie | — |
🔑 **`U46` non ordina per resa, e va letta a parte.** Sei voci sono meno di ventuno, ma quattro sono **le
uniche non verdi del subset `RELEASE-V01`**: finché restano gialle, `G9` non è verde e la v0.1 non è
consegnabile. Le altre sedute di questa tabella producono verdetti; questa produce **il gate**.
➕ **Dal 2026-09-04 ne porta due in più, e non sono lavoro aggiunto**: `PIE-VIS-SIGHTWALL` si giudica nello
**stesso Play** del punto ② — è la voce che guarda lo scenario che quel punto usa già — e `PIE-VIS-TWOLAYERS`
costa un Play a zero turni. Chiudono i due scenari `Visual.*` che il registro dichiarava *«eseguiti, verdi, e
mai guardati da nessuno»*.
⚠️ **E le sue quattro voci compaiono due volte in `editor-sessions.yaml`**, qui e nelle sedute d'origine
(`U4`, `U6`, `U11`, `U15`): è deliberato e temporaneo — `U46` è una **coda**, non un trasferimento, e
sparisce quando i residui sono chiusi. Chi conta le voci schedulate non le conti due volte.

🔑 **`U45` non era in questa tabella e ha tre voci aperte da prima di oggi**: è il tipo di riga che
l’ordinamento per resa nasconde, ed è lo stesso motivo per cui `U39` ci è entrata solo ieri.

⚠️ **`U42` e `U43` sono cambiate, e non di poco**: `U43` dichiarava **dieci** voci aperte e ne misura
**sette**. 🔑 **`U18` esce dal gruppo delle quattro** perché questa passata le ha aggiunto
`PIE-V01-SHIELD`, e con `unblocked_by: []` è la **seconda** seduta del file che non attende nulla — un
fatto che la riga *«U18 · U19 · U22, varie»* nascondeva mettendola fra due sedute che invece attendono.

🔑 **Due sedute coprono 28 delle 73 voci schedulate.** Le altre ventuno si dividono il resto, molte con una
voce sola: l'ordine non è una preferenza, è dove una sessione produce venti verdetti invece di uno.

## 3. Come si avvia uno scenario, e la trappola che lo rende inutile

Il meccanismo esiste ed è in `PIE-TEST-CONSOLE`:

- `rt.Test.List` — elenca gli scenari registrati. ⚠️ **Il numero si rimisura, non si cita**: questa riga ha
  detto «4», poi «8», poi «9», e al 2026-08-17 ne contava **78**.
- `rt.Test.Run <ScenarioId>` — esegue lo scenario **nella partita in corso**.
- `rt.Test.DumpResult` — ristampa l'ultimo `result.json`.

🔴 **La trappola, dichiarata nel registro e non teorica:**

> *«eseguire uno scenario **sostituisce la mappa** e aggiunge unità alla partita in corso — è previsto (il
> runner riusa mappa e turn manager), ma dopo conviene riavviare con `R`»*
>
> *«due esecuzioni consecutive dello stesso scenario **non sono confrontabili** senza `R` in mezzo»*

∴ con venti verifiche in fila, **dimenticare `R` una volta contamina tutti i verdetti successivi**. La
procedura è: `rt.Test.Run <id>` → guarda → **`R`** → prossimo. Senza eccezioni, anche quando sembra che non
serva.

⚠️ E l'esito dei comandi si legge nell'**Output Log**: è il medium legittimo, deciso dall'autore il
2026-08-16. L'overlay della console in PIE scorre via.

## 4. L'ordine consigliato

### Passo 1 — U42, ventuno voci in **undici Play**, nessun prerequisito

Le voci: `PIE-VIS-ICE`, `-WETFIRE`, `-KO`, `-CHARGE`, `-ROUGH`, `-COMBO`, `-COORD`, `-FALLBACK`, `-SMOKE`,
`-PHASES`, `-LEVEL`, `-COVER`, `-DOOR`, `-HIGH`, `-HIGHCOVER`, `-GUARD`, `-BRACE`, `-AREAGUARD`, più
`PIE-ACC-GUARDBRACE` e `PIE-ACC-ENVIRONMENT`.

✅ Tutte e venti **citano il proprio scenario** nella riga di registro: nessuna richiede di indovinare
cosa allestire — verificato anche sulla ventesima, che dichiara `Visual.Environment.Acceptance` e il
percorso del suo file.

➕ **La ventesima è `PIE-ACC-ENVIRONMENT`, ed è arrivata dopo che questo file era stato scritto**: la
voce ombrello del composito d'ambiente, entrata in `main` lo stesso 2026-09-03. Il testo qui sopra diceva
*diciannove* ed elencava diciannove nomi: era corretto stamattina.

✅ **La guida è stata allineata il 2026-09-03, e il ⛔ che stava qui è superato.**
[`guida-seduta-u42-corpus-visual.md`](../../technical/runbooks/guida-seduta-u42-corpus-visual.md) dice ora
**ventuno** voci, nomina `PIE-ACC-ENVIRONMENT` e `PIE-ACC-MAP`, e porta in testa alla §3 la sezione che
cambia la resa della seduta: **tre Play coprono tredici voci**.

🔑 **La seduta non è ventuno aperture: è UNDICI.** I tre compositi di acceptance —
`Visual.Environment.Acceptance` (6 voci), `Visual.Map.Acceptance` (4) e
`Visual.Combat.GuardVsBraceUnderSmallHits` (3) — ne coprono tredici; le altre otto (`-KO`, `-CHARGE`,
`-COORD`, `-FALLBACK`, `-PHASES`, `-LEVEL`, `-HIGHCOVER`, `-AREAGUARD`) hanno un Play ciascuna.

⚠️ **Un composito toglie l'allestimento, non il giudizio**, e chiede *più* attenzione per Play non meno:
la colonna «falsificata se» della guida va letta **prima** di premere Play. Se al terzo turno non ricordi
cosa stavi cercando, hai risparmiato un riavvio e perso un verdetto.

⚠️ **Prima di aprire l'editor va scritto, per ciascuna, cosa la falsifica.** Sono verifiche visive: senza un
criterio scritto prima, venti verifiche producono venti *«sembra ok»*, che non è un verdetto.

### Passo 2 — U43, sette voci

`unblocked_by: []`, ma la sua issue #151 è un'**epic aperta**. Il titolo dichiara che *«i due compositi
allestiscono già»* le voci, quindi il lavoro potrebbe essere solo di esecuzione — ⛔ **da verificare voce per
voce prima di convocarla**, non da assumere.

### Passo 3 — le 29 orfane

Non sono lavoro da implementare: sono lavoro da **schedulare**. Ognuna va assegnata a una seduta esistente o
a una nuova, e la decisione è di `editor-sessions.yaml`, che ne è l'owner.

⛔ **Non prima di U42**: assegnare ventinove voci richiede un'analisi, mentre U42 è pronta adesso.

### In coda a `U21` — `U39`, **una** voce, e chiude una issue

✅ **L’ostacolo che rendeva questa seduta ineseguibile è caduto il 2026-09-04.** L’arena piena chiede
raggio 50, e fino a #2228 quel regime **non era allestibile senza toccare un asset**: il pennello ha
`BrushRadius` con `ClampMax = "8"`, nessuna fixture arriva a 50, e l’arena raggio 50 esisteva solo
dentro un automation test. Ora si allestisce da riga di comando, senza aprire nessun `.uasset`:

```
UnrealEditor.exe <uproject> -dpcvars=rt.Map.Source=GeneratedDemoArena,rt.Match.DemoArenaRadius=50
```

⚠️ **Resta comunque legata a `U21`, e per una ragione diversa da prima**: non è più il regime a
mancare, è la **scena illuminata**. Su un livello al buio il verdetto direbbe più sulle luci che sulle
coordinate.

✅ **Si allestisce in un gesto, e il 2026-09-04 è stato fatto.** Sull’`ARTHexMapActor` del livello:
si imposta `DemoRadius` nel pannello Details e si preme **`Generate Into Asset`**
(`UFUNCTION(CallInEditor)`, `RTHexMapActor.cpp:1736`). La funzione scrive
`HexArea(centro, DemoRadius)` nel `MapAsset` assegnato, poi `SortCells()` e `RebuildInstances()`.

Misurato con `DemoRadius = 10` su `L_DevSandbox`, via `RTDevToolset.GetCurrentMap`:

```
numCells: 331   (era 65)      graphRevision: 7994  (era 11)
layers: [0]     (era [0,1])   numTransitions: 0    (era 2)
```

✅ E le coordinate **si leggono a schermo** su quelle celle — `4,-2,0`, `2,2,0`, `1,1,0` — nel viewport
editor, dove il **pennello** esiste. 🔑 **331 celle superano il DoD**: il piano che ha creato questa voce
chiedeva *«una mappa da almeno 200 celle»*
([`2026-08-31-coordinate-cella-pavimento.md`](../../superpowers/plans/2026-08-31-coordinate-cella-pavimento.md)).
⚠️ La riga di registro chiede *«l’arena piena, raggio 50, 7 651 celle»*: è una **deriva** rispetto al
piano che l’ha generata, non un requisito che qualcuno abbia deciso.

⛔ **Non salvare l’asset dopo la generazione**: `GenerateIntoAsset` fa `MarkPackageDirty()` sul
`MapAsset` del livello, che è versionato. Per una verifica una tantum non serve, e non deve entrare in un
commit — la generazione **sostituisce** il contenuto precedente (i due layer di `Scratch_Basin` sono
diventati uno, le transizioni sono sparite).

🔴 **Questa sezione ha detto due cose sbagliate prima di questa, e vanno lasciate leggibili.**
Il 2026-09-04 diceva che #2228 aveva reso eseguibile la seduta: **meccanismo sbagliato** — il
`-dpcvars` allestisce l’arena nella copia **PIE**, dove né le coordinate né il pennello esistono. Poche
ore dopo diceva che la seduta era **impossibile**: **conclusione sbagliata**, perché nessuno aveva
cercato un generatore nell’editor. ∴ Il difetto comune non era l’ipotesi ma il metodo — **concludere
senza cercare** — e `GenerateIntoAsset` stava due righe sotto il codice che si stava leggendo.

⚠️ **Non è un passo a sé, e la prima stesura di questa sezione lo chiamava «Passo 0» mettendolo
davanti a `U42`.** Era sbagliato in due modi: `U39` dichiara `unblocked_by: [U21]` — lo **stesso**
ostacolo per cui `U25` è marcata 🔴 due righe sopra — e `U42` invece non attende nulla. Un ordine che
mette una seduta bloccata prima di una libera non è un ordine di esecuzione.

Detto questo, quando `U21` si apre `U39` costa quasi nulla e rende molto:

`PIE-HEX-COORD-COSTO` è la sua unica voce aperta, e la gemella `PIE-HEX-COORD-LEGGIBILITA` è già ✅
(2026-09-01, confermata dall'autore su `L_DevSandbox`). **Metà del criterio è già misurata** — il
conteggio dei segmenti — e resta il giudizio che nessuna sonda headless può dare: navigare l'**arena
piena** (raggio 50, 7 651 celle) e dire se il viewport regge, anche trascinando il pennello.

🔑 **Chiuderla chiude l'intera seduta e con essa**
[#1920](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1920), la cui implementazione è già
tutta in `main` — runtime, guscio editor e voci di registro. Resta aperta **solo** per questo verdetto a
schermo.

🔑 **E non chiede una propria apertura**: `shares_setup_with: [U21, U22, U25, U26]`, e di quel gruppo
è la meno esigente — nessuno strumento attivo. Si esegue **nella stessa apertura**.

⚠️ **Perché fin qui non compariva in questo file**: la tabella della §2 raggruppava le sedute da
una-tre voci in *«altre 16»*, e una seduta a **una** voce sparisce nel gruppo che l'ordinamento per resa
mette per ultimo. Ordinare per numero di verdetti nasconde chi ne produce pochi **a costo quasi nullo** —
il che resta vero, ed è il motivo per cui ha una riga propria nella tabella, non un posto in cima alla fila.

### Fuori dall'ordine per resa — `U46`, quattro voci che **sono** il gate

Le tre passate qui sopra ordinano per resa: `U42` ventuno voci, `U43` sette, poi le orfane. `U46` ne ha
**quattro** e va comunque fatta presto, perché non è una seduta come le altre: le sue quattro voci sono
l'intero residuo non verde di `G9`.

**Perché esiste.** Le quattro non erano orfane — avevano già una seduta ciascuna. Il difetto era il
simmetrico di quello di `U45`: **sedute quasi esaurite che nessuno riaprirà per una voce sola**. Misurato
il 2026-09-04 — `U4` 1 residuo su 3, `U6` 1 su 4, `U11` 1 su 1, `U15` 2 su 5. Quattro aperture per quattro
voci; `U46` ne chiede una.

**Cosa la rende eseguibile ora.** `unblocked_by: []`, e dal 2026-09-04 il residuo di ciascuna voce è **una
domanda sola**: la parte misurabile è stata chiusa headless e dal ponte MCP — la salita a `LayerHeight`
esatto per `PIE-HEXPLAY-8`, cinquanta mosse senza illegali per `PIE-V01-ROSTER`, il formato del log con il
motivo accanto per `PIE-V01-LOG`.

⌫ **Questo paragrafo diceva il contrario, e la fonte era `#1873` — riscritta il 2026-09-04 attorno al residuo
reale, con l'istruttoria originale conservata.** Sosteneva che la sola presentazione
del blocco-vista fosse un anello di `DrawCellOverlay` e che «nella vista del giocatore non c'è niente», e
ne concludeva che `PIE-HEXPLAY-6` aspettasse **codice**. 🔴 **È falso, ed è stato scritto senza
rimisurare**: `RebuildInstances` popola l'ISM persistente `Blockers` con una **lastra** `0.75 × 10 cm` per
`bBlocksLineOfSight` e una **colonna** `0.40 × 55 cm` per `bBlocksMovement` — indipendenti, senza guardia
di compilazione, pinnate da `HexMapActor.BlockerVolumesComeFromCellFlags`. Il codice è su `main` dal
**2026-08-12** (`46d9ef5f`, *«dare volume ai blocchi, e distinguere "non si passa" da "non si vede"»*),
**diciotto giorni prima** che `#1873` ne misurasse l'assenza. E `bCellOverlay` **nasce spento**: chi guarda
vede i volumi, non gli anelli — l'inferenza «overlay spento ⇒ non resta nulla» non è mai stata verificata
a schermo, né dalla issue né da questo documento.

∴ **`PIE-HEXPLAY-6` non aspetta codice**: aspetta un giudizio come le altre tre. La domanda si è però
ristretta a una sola — se la lastra, alta **10 cm**, si legga **a camera obliqua**. ⚠️ E il banco non è
`L_HexArena`: le sue celle di barriera hanno **entrambi** i flag, quindi lastra e colonna si sovrappongono
sempre. È `Visual.Map.SightWallIsWalkable`, il muro che si attraversa.
⛔ Il velo fa parte della misura: `VeilInstances` porta a scala **zero** i volumi delle celle mai viste
([D-225]), quindi una cella non osservata non mostra nulla **per progetto** — e scambiarlo per un'assenza
di resa è precisamente l'errore che questo paragrafo conteneva.

⛔ **E nessuna delle quattro si delega.** Misurato dal ponte MCP: `CaptureViewport` renderizza il mondo
dell'**editor**, non quello PIE — `find_actors` trova `BP_Unit_Gadget_C_0` a `(-519,-30,-200)` e una
cattura centrata lì mostra la cella e **non l'unità**. Mappa, celle e quote si giudicano da remoto; unità,
HUD, VFX e playback no.

## 5. Cosa questa roadmap NON dice

- ⛔ **Non dice che i venti verdetti saranno verdi.** Dice che sono *osservabili*. Uno scenario che gira
  headless prova che il gioco non si rompe, non che a schermo si veda ciò che deve vedersi — è precisamente
  la ragione per cui esiste una verifica PIE.
- ⛔ **Non copre le 21 voci con ostacolo dichiarato** — ⚠️ numero **non rimisurato**, vedi la §1: quelle
  hanno cause proprie, scritte nelle loro righe,
  e vanno lette una per una.
- ⛔ **Non tocca le sedute con `execution_lane: asset`** (U1, U8, U24, U28, U29, U30): producono `.uasset`, e
  sono un altro mestiere — Content Browser e Binary Asset Lease, non osservazione.
