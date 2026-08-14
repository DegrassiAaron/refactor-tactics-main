# M6.8 — Playtest della partita hex (`#38`) — spec panel sulla Definition of Done

> `CURRENT` · **Stato**: revisione chiusa, **non applicata** (nessun corpo issue modificato da questo file)
> **Data**: 2026-08-14 · **HEAD della revisione**: `c371c9a0`
> **Oggetto revisionato**: il corpo di [`#38`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/38)
> — *CP 2.8 — Playtest della partita hex (sessione D)* — creato il **2026-08-05**, ultimo aggiornamento
> **2026-08-09**, otto criteri fra DoD, verifica, file e chiusura.
> **Scopo**: stabilire se quella DoD è **eseguibile e falsificabile oggi**, prima che qualcuno si sieda
> all'editor per una seduta che il registro descrive in modo diverso.
> **Metodo**: ogni affermazione qui sotto è misurata su questo albero coi comandi che il repository già
> pubblica. Dove la misura contraddice la issue, vince la misura — ma il comando resta scritto, così la
> prossima persona rimisura invece di credermi.

---

## 1. Il verdetto in una riga

**La issue è più vecchia del registro che pretende di chiudere, e lo è di tre giri.** `#38` è stata scritta
il 2026-08-05 e ritoccata il 2026-08-09; fra il **10** e il **12 agosto** il registro delle verifiche manuali
ha cambiato *l'unità di lavoro* (`#371`), *il contenuto della seduta* (la mappa non si costruisce più a mano),
*due voci su cui il DoD conta* (`#410`, `#426`) e *lo stato di quattro voci su nove*. Nessuno di questi
cambiamenti è arrivato nel corpo della issue.

Il risultato non è una issue vaga — è una issue **precisa e disallineata**, che è la forma peggiore: chi la
esegue alla lettera costruisce un asset che non serve, cerca un contenitore che non esiste più, e conclude
con una clausola che sblocca una issue già chiusa.

| Voce del DoD | Classe | Perché |
|---|---|---|
| 1 · mappa di prova costruita con l'editor mode | 🔴 `OBSOLETO` | il registro dichiara che non serve più: `GeneratedTestArena` la genera |
| 2 · partita 2v2 completa **fino alla vittoria**, senza crash | ⚠️ `NON FALSIFICABILE` | «vittoria» non è un esito che l'esecutore controlla; «senza crash» non ha rilevatore |
| 3 · `PIE-HEXPLAY-1..9` tutte ✅ | 🔴 `AMBIGUO` | l'insieme non coincide con la sessione D, e nel registro le righe sono **14**, non 9 |
| 4 · nessun percorso di gioco passa da `FRTGridCoord` | ⚠️ `NON DISCRIMINA` | misurato: **0** occorrenze in `Source/`. Vero per ragioni più forti di quelle che chiede |
| 5 · «Sessione D di `test-manuali-pie.md` completa» | 🔴 `RIFERIMENTO MORTO` | le sessioni A–G sono uscite da quel file il 2026-08-10 (`#371`) |
| 6 · «log della partita allegato alla PR» | ⚠️ `SOTTOSPECIFICATO` | quale file, dove, e il repository non versiona `Saved/` |
| 7 · file coinvolti: `docs/design/test-manuali-pie.md` | 🔴 `PATH ROTTO` | `docs/design/` **non esiste**; il file è in `docs/technical/` |
| 8 · «chiude `#16` e **sblocca** `#17`» | 🔴 `SCADUTO` | **`#17` è `CLOSED`** |

**Su otto voci, cinque non sono eseguibili come scritte e due non sono falsificabili.** Quella che regge
senza riserve — la sostanza — è che M6 si chiude con una partita vera e non con la somma dei checkpoint.

---

## 2. Il panel

Focus `requirements` + `testing`. Il pannello è scelto sul tipo di documento: una DoD di **verifica manuale**,
cioè un contratto fra chi scrive il criterio e chi si siede a eseguirlo.

**WIEGERS** — *«Il criterio 3 dice "tutte ✅" e non dice di quante. Ho contato le righe del registro:
sono quattordici. La notazione `1..9` non è un intervallo su questo alfabeto — fra `-3` e `-4` c'è `-3b`, e
fra `-6` e `-7` ci sono `-6b` e `-6c`. Un DoD che si chiude "quando quelle da 1 a 9 sono verdi" costringe
l'esecutore a decidere da solo se `-6b` conti, e quella decisione cambia se la seduta finisce oggi o fra
un mese.»*

**COCKBURN, sul contenitore** — *«Chiedo chi è l'attore e cosa apre. La issue risponde "la sessione D".
Sono andato a cercarla dove la issue dice che vive, e non c'è: il registro dichiara che le sessioni A–G
sono state spostate il 2026-08-10 perché erano lo stesso concetto modellato due volte. Oggi l'unità è la
**seduta**, e le voci HEXPLAY stanno su **cinque** sedute diverse. La differenza non è di vocabolario: è
la differenza fra una apertura d'editor e cinque.»*

**ADZIC, sul primo criterio** — *«Il criterio 1 è l'unico scritto con esempi concreti — r=4, due o tre celle
bloccanti, una superficie costosa, una piattaforma di 3–4 celle con una transizione — ed è per questo che
si nota che è morto. Quegli stessi numeri sono nel registro come descrizione di ciò che `GeneratedTestArena`
**già genera da codice**. Il DoD chiede di costruire a mano l'esempio che il codice produce.»*

**CRISPIN** — *«"Senza crash" non è un criterio di test, è una speranza. Non dice dove si guarda. Il registro,
tre sezioni più in basso, ha già la prassi giusta: si incolla il percorso di `Saved/Logs/*.log` e si leggono
le righe. Il DoD deve chiedere quella cosa lì, che è verificabile, invece di una che non lo è.»*

**NYGARD** — *«"Dall'avvio alla vittoria" presuppone che la partita finisca come vuoi tu. Non finisce come
vuoi tu: il formato ha tre esiti, e la prima esecuzione reale è finita in pareggio allo scadere dei round
con una squadra in vantaggio 2 contro 1. Quel playtest ha funzionato — ha falsificato `RoundLimit 5` e ha
fatto cambiare il formato lo stesso giorno. Un DoD che avesse potuto chiamarlo "fallito" avrebbe buttato via
il suo risultato migliore.»*

**GREGORY, sulla registrazione** — *«Ho una domanda che nessuna delle otto voci copre: chi scrive il verdetto,
e in quale file. In questo lotto di sessioni parallele il file dove i verdetti si scrivono appartiene a
un'altra track. Se la risposta è "lo scrive chi esegue", allora la seduta si ferma alla prima riga che deve
salvare.»*

---

## 3. I findings, con la misura accanto

### F1 🔴 `PIE-HEXPLAY-1..9` e «sessione D» sono **due insiemi diversi** presentati come uno

Il DoD li dà per equivalenti — criterio 3 e criterio 5. Non lo sono.

```sh
# le righe di tabella PIE-HEXPLAY nel registro: sono 14, non 9
grep -c '^| \*\*PIE-HEXPLAY' docs/technical/test-manuali-pie.md          # 14
```

| Insieme | Voci | Cardinalità |
|---|---|--:|
| `1..9` letto alla lettera | 1 · 2 · 3 · 4 · 5 · 6 · 7 · 8 · 9 | 9 |
| gruppo **«Partita hex (sessione D)»** del registro | 4 · 4b · 5 · 6 · 6b · 6c · 7 · 9 · 10 | 9 |
| **intersezione** | 4 · 5 · 6 · 7 · 9 | **5** |

Due insiemi di nove elementi che ne condividono cinque. La cardinalità identica è ciò che rende il difetto
invisibile: sembrano lo stesso elenco perché contano uguale.

⚠️ **Lo stesso «nove» è scritto anche altrove**, e in nessuno dei tre posti è misurato:
`editor-sessions.yaml` (`U6.done_when`: *«le nove voci `PIE-HEXPLAY` sono verdi»*),
[`../roadmap-checkpoint.md`](../roadmap-checkpoint.md) (due righe, §M6), e il corpo di `#38`. È una formula
copiata quattro volte da quando le voci erano nove davvero.

### F2 🔴 La «sessione D» non esiste più nel file che il DoD nomina

Il registro lo dichiara esplicitamente: le sessioni A–G *«stavano qui, e sono state spostate»* il 2026-08-10
con [`#371`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/371), perché raggruppavano per
preparazione condivisa mentre le sedute `U1`–`U17` facevano la stessa cosa in un secondo file — *«lo stesso
concetto modellato due volte»*. Oggi l'owner della sequenza è [`../editor-sessions.yaml`](../editor-sessions.yaml).

Misurato lì, le voci HEXPLAY stanno su **cinque** sedute:

| Seduta | Titolo | `verifies` |
|---|---|---|
| **U2** | Partita hex, primo giro | `-1` `-4` `-5` (+ `PIE-CAM-START`) |
| **U3** | Input e pianificazione | `-2` `-3` `-3b` (+ `PIE-PREVIEW-PERSIST`) |
| **U4** | Combat e linea di tiro | `-6` `-6b` `-6c` |
| **U5** | Bot e HUD | `-7` `-9` (+ i cinque `PIE-AI-*`) |
| **U6** | Multilivello e partita completa | `-8` `-10` `-4b` (+ `PIE-FACING-1`) |

Le cinque **condividono la preparazione** (`shares_setup_with`), quindi possono stare in una apertura sola —
ma è una scelta di chi esegue, non un fatto che il DoD possa dare per scontato. U6 è la seduta che *chiude*
M6: `produces: chiusura di M6 / E2 — sessione D verde`. È lei l'erede della sessione D, e ha un nome.

### F3 🔴 Il criterio 1 prescrive un lavoro che il registro dichiara superfluo

Il gruppo «Partita hex» del registro porta questa nota:

> ⚠️ **Non serve più costruire la mappa a mano**: `MapSource = GeneratedTestArena` genera già esagono r=4,
> ostacoli, **muro che blocca la vista**, fango a costo 3, piattaforma su layer 1 e **una** transizione.

E `U2.steps` la ripete: *«Non aspetta U1: il terreno lo fornisce `MapSource = GeneratedTestArena` […] Questa
è la preparazione condivisa da U2…U6: una apertura, cinque sedute»*.

Il criterio 1 elenca **esattamente** quelle proprietà. Non è che sia sbagliato: è che descrive l'output di
`MakeTestArena` come se fosse un compito manuale.

⚠️ **E costruirlo davvero costa più di quanto sembri.** L'asset che il criterio implica è
`Content/RT/Maps/Dev/L_HexArena/`, che è il prodotto di **U1 / [`#451`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/451)**,
una seduta diversa con una track diversa. In [`../parallel-batch.yaml`](../parallel-batch.yaml) la
**Binary Asset Lease** su quel percorso è dichiarata **non emessa**, e due `.uasset` non si fondono. Eseguire
il criterio 1 alla lettera significa aprire una contesa su un binario per ottenere un terreno che il codice
già fornisce.

### F4 ⚠️ «Fino alla vittoria» chiede un esito che l'esecutore non controlla

`PIE-HEXPLAY-10` — la voce che copre questa metà — ammette la conclusione **a tre vie**, e la prima
esecuzione reale (2026-08-10) è finita così: `Pareggio - allo scadere dei round (round 5/5)`, con il team 1
avanti 2 contro 1. Quel playtest ha falsificato `RoundLimit 5` contro i 10–14 fissati da **D-010**, e il
formato spedito è passato a `RoundLimit 12` / `ExpectedRounds 10` lo stesso giorno.

Con il limite nuovo la vittoria per eliminazione ridiventa possibile — **non garantita**. Un DoD che la
esige può fallire per bilanciamento invece che per difetto, e la partita da rigiocare col limite nuovo è
comunque dovuta: il ritmo misurato su 5 round non descrive più il gioco.

### F5 ⚠️ «Senza crash» e «log allegato» non dicono dove si guarda

Il registro ha già la prassi, scritta in §*La sequenza*: *«incolla il percorso di `Saved/Logs/*.log` oppure
dimmi cosa hai osservato»*. Il DoD non la nomina, e `Saved/` non è versionato: «allegato alla PR» descrive
un artefatto che non ha un posto dove atterrare. Le voci del registro, invece, citano le righe di log
testuali (`Board 2v2 esagonale avviata su N celle con 4 eroi`, `fermo: cella contesa (q=4,r=0,L=0)`) — che
sopravvivono al fatto che il file di log non sia committabile.

### F6 ⚠️ Il quarto criterio è vero, ma non discrimina più

```sh
grep -rn "FRTGridCoord" --include="*.h" --include="*.cpp" Source/ | wc -l    # 0
```

**Zero.** Fuori da `Source/` il nome sopravvive in una riga di `AGENTS.md` — che *vieta* di reintrodurlo — e
in artefatti di build (`Binaries/`, `Intermediate/`), che non sono codice. La clausola «il tipo può esistere,
ma non nel flusso della partita» descrive uno stato intermedio superato: il tipo **non esiste**. Il criterio
non può più distinguere un albero conforme da uno che non lo è, e va o rimosso o convertito nel gate
meccanico che oggi passerebbe da solo.

### F7 🔴 La clausola di chiusura è scaduta

```sh
gh issue view 17 --json state,closedAt --template '{{.state}} {{.closedAt}}'   # CLOSED 2026-08-14T07:13:45Z
```

`#17` — *E3, Dismissione del quadrato* — è **chiusa**. «Sblocca `#17`» non descrive più un effetto: chi legge
la riga va a cercare un lavoro che non c'è. Resta vera l'altra metà — `#38` chiude l'epic `#16`, che è `OPEN`.

⚠️ **È scaduta stamattina**, non un mese fa: `closedAt` è `2026-08-14T07:13:45Z`, poche ore prima di questa
revisione. È il caso limite che rende inutile la regola «le issue vecchie vanno rilette»: questa riga era
vera all'ultima volta che qualcuno avrebbe potuto ragionevolmente controllarla. L'unica difesa è rimisurare
al momento di eseguire — che è ciò che questo referto fa, e il motivo per cui i comandi restano scritti.

### F8 🔴 Il path del file è rotto

`docs/design/` non esiste in questo albero. Il file è
[`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md).

### F9 ⚠️ Il DoD non dice chi scrive il verdetto — e in questo lotto non è scrivibile da chiunque

Il prodotto di `#38` sono gli **esiti**, e gli esiti si scrivono in `docs/technical/test-manuali-pie.md`.
In [`../parallel-batch.yaml`](../parallel-batch.yaml) quel file è nel `writable` della track
`content_editor` (`#451`), uscito da `integration_only` **per questo giro**. Una sessione che esegue `#38` e
registra gli esiti scrive un file non assegnato: per **D-139** è STOP e riallocazione, non «solo questa riga».

Il batch aveva già previsto il caso, in astratto, parlando di un'altra track: *«due track che producono
verdetti PIE si contendono l'unico posto dove un verdetto si scrive»*. `#38` è il secondo caso, ed è
concreto.

### F10 ℹ️ Tre delle nove sono già verdi, e il DoD non lo sa

Stato misurato oggi, col metodo che il registro stesso pubblica (primo marcatore **posizionale** nella
colonna di stato — non il primo che si cerca, che è l'errore che ho commesso al primo tentativo):

```sh
awk -F'|' '/^\| [*][*]PIE-HEXPLAY/ {n=$2; gsub(/[*` ]/,"",n); s=$(NF-1);
  if (match(s, /✅|🟡|⏳/)) printf "%-30s %s\n", n, substr(s, RSTART, RLENGTH)}' \
  docs/technical/test-manuali-pie.md
```

| | Voci | |
|---|---|--:|
| ✅ | `-1` `-3` `-5` | 3 |
| 🟡 | `-2` `-3b` `-7` `-8` `-10` | 5 |
| ⏳ | `-4` `-4b` `-6` `-6b` `-6c` `-9` | 6 |

**Delle nove numerate: ✅ 1·3·5 — 🟡 2·7·8 — ⏳ 4·6·9.** Tre su nove verdi, non zero.
⚠️ [`../roadmap-checkpoint.md`](../roadmap-checkpoint.md) §M6 dichiara *«`PIE-HEXPLAY-1..9`, tutte ⏳»*: era
vero fino al 2026-08-09, è falso dal 10. Non lo correggo qui — quel file è `integration_only` — ma è la
quarta copia della stessa formula non rimisurata (F1).

### F11 ℹ️ Collaterale: il gate **G9** della DoD di release è indietro

[`../v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) dichiara per G9 *«2 verdi · 7 parziali ·
8 aperte (2026-08-09)»*. Rimisurato oggi col comando che quel documento pubblica:

```sh
awk -F'|' '/^\| [*][*]PIE-[A-Za-z0-9.-]*[*][*] `RELEASE-V01`/ {s=$(NF-1);
  if (match(s, /✅|🟡|⏳/)) c[substr(s, RSTART, RLENGTH)]++; n++}
  END {printf "righe=%d verde=%d parziale=%d aperta=%d\n", n, c["✅"], c["🟡"], c["⏳"]}' \
  docs/technical/test-manuali-pie.md               # righe=17 verde=5 parziale=6 aperta=6
```

**17 righe · 5 verdi · 6 parziali · 6 aperte.** Il conteggio delle righe regge (17, come dichiarato); lo
stato no. Segnalato e non corretto: `v0.1-definition-of-done.md` è `integration_only`.

---

## 4. La DoD riscritta

Pronta da incollare nel corpo di `#38`. Ogni voce è falsificabile da chi si siede all'editor, e nessuna
chiede un lavoro che il codice già fa.

> ### Definition of Done
>
> - [ ] **Terreno**: la seduta gira su `MapSource = GeneratedTestArena` — esagono r=4, ostacoli, muro che
>   blocca la vista, fango a costo 3, piattaforma su layer 1 e una transizione, generati da `MakeTestArena`.
>   ⚠️ Nessun asset mappa va costruito o salvato qui: `L_HexArena` è il prodotto di **U1** (`#451`) e la sua
>   Binary Asset Lease non è emessa per questa track.
> - [ ] **Le quattordici voci `PIE-HEXPLAY` del registro hanno un esito reale**, non un'attesa. Verdi le
>   nove che compongono il verdetto di M6 — `-1 -2 -3 -4 -5 -6 -7 -8 -9`; per `-3b -4b -6b -6c -10`
>   è ammesso 🟡 **con la ragione scritta accanto** (già oggi `-6b` dichiara che il cono non è verificabile:
>   nessuna abilità del roster v0.1 usa `ERTAbilityShape::Cone`).
> - [ ] **Sedute**: `U2 → U3 → U4 → U5 → U6` di `editor-sessions.yaml`, che condividono la preparazione e
>   possono stare in una apertura sola. Il `done_when` di **U6** è la chiusura: le voci rilette *tutte
>   insieme*, non voce per voce.
> - [ ] **Una partita 2v2 intera raggiunge una conclusione dichiarata** con `RoundLimit 12`, senza
>   sovrapposizioni né unità fuori mappa. Almeno una run chiude **per eliminazione**; se dopo tre run chiude
>   sempre per scadenza, non è un fallimento della seduta — è un numero per G11 e una issue di bilanciamento.
> - [ ] **Nessun `ensure`, `check` o crash** nel log della seduta. Evidenza: le righe pertinenti di
>   `Saved/Logs/*.log` incollate negli esiti delle voci — non il file, che non è versionato.
> - [ ] ~~Nessun percorso di gioco passa più da `FRTGridCoord`~~ → **soddisfatto e verificabile a comando**:
>   `grep -rn "FRTGridCoord" --include="*.h" --include="*.cpp" Source/ | wc -l` = **0**. Resta come gate, non
>   come lavoro.
>
> ### Test / verifica
> Gli esiti si scrivono in [`docs/technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md) —
> **non** `docs/design/`, che non esiste. ⚠️ In questo batch quel file è nel `writable` della track
> `content_editor`: l'assegnazione va risolta **prima** della seduta, non a verifica fatta.
>
> ### Chiusura
> Chiude l'epic `#16`. ~~Sblocca `#17`~~ — **`#17` è chiusa dal 2026-08-14 `07:13:45Z`**.

---

## 5. Cosa questo referto **non** fa

- **Non modifica il corpo di `#38`.** La riscrittura di §4 è una proposta; applicarla è un atto separato,
  e chi lo fa dichiara nella issue che viene da qui.
- **Non tocca i tre file che propagano la formula «nove»** — `roadmap-checkpoint.md`,
  `v0.1-definition-of-done.md`, `editor-sessions.yaml`: sono `integration_only`, si riconciliano in
  integrazione. F1, F10 e F11 esistono perché quella riconciliazione abbia i numeri già misurati.
- **Non risolve F9.** Chi possiede `test-manuali-pie.md` in questo giro è una decisione di batch, non di
  questa sessione: `parallel-batch.yaml` è `integration_only` e una track nuova per M6.8 non è dichiarata.
  È la cosa da decidere **prima** di aprire l'editor, perché decide se la seduta ha dove scrivere.
- **Non esegue la seduta.** Il PIE richiede l'editor interattivo. Quello che si può preparare senza —
  la sequenza esatta dei passi per le cinque sedute, e la lettura dei log dopo — non è in questo referto
  perché non è stato chiesto: è il passo successivo naturale.
