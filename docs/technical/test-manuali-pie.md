# Test manuali (PIE) — verifiche interattive da eseguire

> `CURRENT` · **Ultimo aggiornamento**: 2026-08-09 · **Owner**: questo file — è il **registro** delle verifiche interattive.
> Verifiche che richiedono l'editor UE (PIE, mouse, asset) e **non** sono automatizzabili headless.
> **Complementari** ai test Automation (suite integrata **bot + hex** su `main`, tutti verdi). Parte del DoD «playtest ogni incremento» (roadmap §QA).
> Regola: una voce è ✅ **solo dopo** verifica reale in PIE — non «dovrebbe funzionare».
> **Quale voce affrontare e quando** lo dice la [**EditorMap**](../roadmap/editormap.shortlist.md): questo file
> resta il **registro** (esito atteso e stato), quella è la **sequenza** — sedute, preparazione condivisa,
> artefatti da creare, dipendenze verso i checkpoint di codice. Le voci aperte sono raggruppate lì per
> preparazione, così l'editor si apre una volta per gruppo invece che una per voce.
> **Chi esegue cosa** — quali verifiche una macchina fa da sola e quali richiedono per forza una persona — lo dice
> [`scenario-map.md`](scenario-map.md), che dichiara anche il subset **`RELEASE-V01`** del gate G9 (vedi §*Subset di
> release* qui sotto).

> **Cosa il log puo' provare, e cosa no.** Per diverse voci qui sotto la logica — e da 2026-08-06 anche la
> **sequenza di interazione** (`ARTPlayerController::HandleClickOnCell` e' guidabile senza viewport) — e' coperta
> da test automatici: il loro stato dice «coperto headless» col nome del test. Per quelle voci la seduta in PIE
> **non cerca errori di logica**: guarda se cio' che il codice ha deciso *appare* correttamente. Un log puo' dire
> «la cella evidenziata e' (q,r,L)», non «la vedi»: esagono giallo, anteprima ciano, unita' centrate sui
> centri-cella, inquadratura e fluidita' del playback restano verificabili solo a schermo.

## Come eseguire

> **Prima di aprire l'editor**: molte di queste voci hanno già una copertura automatica, e alcune si
> riproducono senza toccare il mouse con uno **scenario** (`rt.Test.Scenario <Id>` + Play). Come si eseguono
> i test, come si scrive uno scenario e come si legge un report fallito:
> **[`test-e-diagnosi.md`](test-e-diagnosi.md)**.
> Il PIE serve per ciò che nessun test vede: che si **veda** a schermo e che il giocatore **capisca**.

- Apri il progetto: doppio clic su `RefactorTactics.uproject`. All'avvio l'editor può chiedere **quale versione
  dell'engine** usare: scegli **5.8** (`D:\EpicGames\UE_5.8`), la versione bloccata dal progetto. Se chiede di
  **ricompilare i moduli**, accetta — oppure compila da riga di comando (vedi `ue58-build-gotchas`).
  - **Perché lo chiede**: su questa macchina UE 5.8 è registrato come **build custom** (chiave
    `HKCU\Software\Epic Games\Unreal Engine\Builds`, GUID `{B20BD8AB-…}` → `D:/EpicGames/UE_5.8`), non come
    installazione del Launcher — l'unica registrata per nome è la 5.4. Quindi `"EngineAssociation": "5.8"` non
    risolve nulla in locale e l'editor apre il selettore.
  - Dopo la scelta l'editor **riscrive il GUID** dentro `RefactorTactics.uproject`: è corretto e va lasciato
    così in locale (niente più dialoghi). **Non committare quella modifica**: il GUID vale solo su questa
    macchina, il file versionato deve restare a `"5.8"`. Se finisce nello stage per sbaglio:
    `git checkout -- RefactorTactics.uproject`.
- **PIE**: pulsante Play (o `Alt+P`). Il `TurnManager` ha `PlanningSeconds≈30s`: premi **Spazio** per il lock-in manuale.
- I `LogRT: [RT] ...` nell'**Output Log** narrano il round (fasi, esiti).
- I livelli del demo (`L_Prototype`, `L_DevSandbox`) sono **vuoti nell'editor**: griglia, luce, unità e turn manager
  li allestisce a runtime il `RTGameMode`. Viewport nera prima del Play = normale, non un livello rotto.
  Lo **sfondo resta nero anche in gioco**: il GameMode aggiunge una luce direzionale, nessun cielo.
- Camera: **`Home`** ricentra sulla griglia (il pawn parte dall'origine, la board 10×10 si estende per 2000 uu);
  **`F`** centra sull'**unità selezionata** — lo zoom orbita attorno al pawn, quindi senza spostarlo la rotellina
  avvicina al centro della mappa e non al personaggio. Inclinazione e distanza si tarano dal Details del
  `RTCameraPawn` (`Camera Pitch`, `Default Arm Length`) con effetto immediato, anche a PIE avviato.
  Se la vista sembra bloccata e compaiono le **etichette degli actor** in viewport, hai fatto **Eject** (`F8`):
  stai guardando con la camera dell'editor, non con quella del gioco — `F8` di nuovo per rientrare nel pawn.

## Stato in numeri — 2026-08-09

**119 voci**: ✅ **28 verdi** · 🟡 **21 parziali** (regola coperta da test, resta il visivo) · ⏳ **70 aperte**.

*(Rimisurate col comando qui sotto il **2026-08-09**, dopo l'aggiunta di `PIE-MUT-BASTION-SLOW` — la prima
voce del registro che **non** è una verifica visiva: è una verifica di mutazione, headless, rimasta fuori
dall'automazione solo perché richiede una precondizione che nessuno script si garantisce da solo. Nata ⏳ e
chiusa ✅ lo stesso giorno, come la sua gemella `PIE-MUT-ACTIONS-ZERO`. `senza-marcatore` misurato: **0**.)*

*(Rimisurate col comando qui sotto il **2026-08-09** **dopo il merge**, non prima: due rami hanno toccato
questo file lo stesso giorno e ognuno aveva il **proprio** numero giusto — `114 (25/21/68)` da un lato, con le
tre voci `PIE-VIS-GUARD`, `PIE-VIS-BRACE` e `PIE-VIS-COORD`; `112 (26/21/65)` dall'altro, con
`PIE-PREVIEW-AREA` promossa a ✅ e `PIE-PREVIEW-PERSIST` aperta. **Nessuno dei due valeva più dopo l'unione.**
Poi è arrivata la chiusura di **E16** con `PIE-FACING-1`, e il numero è cambiato una **terza** volta nello
stesso giorno. È il difetto che questo documento ha già pagato quattro volte, e l'unica difesa è quella
scritta qui sotto: si misura **dopo il merge**, col comando. `senza-marcatore` resta **0**.)*

> ⚠️ **Tre scenari visivi erano nel corpus senza una voce qui, ed è la classe di buco peggiore che questo
> registro possa avere**: uno scenario eseguito, verde, e **mai guardato da nessuno** — cioè un file che
> *sembra* coperto due volte e non lo è nemmeno una. Sono `Visual.Combat.GuardReducesFirstHit`,
> `Visual.Combat.BraceReducesEveryHit` e `Visual.Combat.WaterElectricCoordinated`, arrivati **dopo** il blocco
> di 18 voci del 2026-08-08. La convenzione «ogni scenario visivo porta una voce PIE» era scritta
> ([`scenari-validazione-visiva.md`](scenari-validazione-visiva.md) §9) ma **nessun comando la verificava**.
> Ora c'è, ed è una riga:
>
> ```bash
> echo "scenari: $(find Scenarios/Visual -name '*.json' | wc -l)  \
> voci: $(grep -c '^| \*\*PIE-VIS-' docs/technical/test-manuali-pie.md)"   # devono coincidere: 21 e 21
> ```
>
> Va eseguito **quando si aggiunge uno scenario `Visual.*`**, non quando si sospetta un buco: la convenzione
> che si ricorda a mano è la convenzione che si dimentica quando si ha fretta.

*(Prima di questa passata erano 111 con 65 aperte; e prima ancora 100 con 54, dopo le **10 voci**
`PIE-STATE-*` dell'epic E34 — che nascono ⏳ per un motivo più forte del solito: **verificano un sistema che
non esiste**.)*

> ⚠️ **Il comando qui sotto era rotto, e ha mentito per settimane.** Usava `s ~ /✅/`, che cerca il simbolo
> **ovunque** nella cella di stato — non il marcatore iniziale. Una voce 🟡 il cui testo cita un ✅ nella
> propria storia («…verificato ✅ il 2026-08-06, resta il visivo») veniva contata **verde**. Sono 3 voci, e
> le quote citate fin qui — comprese quelle scritte oggi — erano **28/18** invece di **25/21**. Il totale
> non è mai stato sbagliato: solo la ripartizione. Ora il comando prende il **primo** simbolo della cella
> (`match` + `substr`) e stampa anche `senza-marcatore`, che deve restare **0**: se sale, qualcuno ha scritto
> una riga senza stato.
*(Rimisurate il 2026-08-08 dopo l'aggiunta di **5 voci bot** (`PIE-AI-01…05`) e **2 di formato/icone**
(`PIE-ICON-01`, `PIE-FMT-01`) dal consolidamento dei sorgenti `docs/src/`. Somma verificata: 27+18+38 = 83.)*

<details><summary>Conteggio precedente — 76 voci (2026-08-08, prima del consolidamento)</summary>
*(Rimisurate col comando qui sotto il 2026-08-08, dopo `PIE-SCEN-FILTER`/`PIE-SCEN-KEEP`, entrambe verdi. Il
totale precedente era giusto — 74 — ma la ripartizione citata «17/32» non lo era: il comando su quel testo
dava già **18/31**. Confermo la lezione di sotto: il numero si ricalcola, e anche le **quote** vanno lette dal
comando.)*
</details>

> **Perché questo numero era rotto** (issue #192): due sessioni parallele hanno misurato lo stesso file in
> momenti diversi — «67 voci: 22/15/30» e «65 voci: 23/16/26» — e il merge ha lasciato **entrambe** le versioni
> con i marcatori di conflitto dentro il documento. Nessuna delle due era sbagliata quando è stata scritta:
> una aveva appena aggiunto le **4** voci di durata/ritmo/scala, l'altra le **2** degli strumenti di
> leggibilità più tre riclassificazioni. Erano entrambe **parziali**, e sceglierne una avrebbe perso il lavoro
> dell'altra. Da lì sono poi arrivate `PIE-V01-MATCHEND` (CP 10.3) e `PIE-V01-COVEREDIT` (CP 9.1), e
> `PIE-V01-LOWCOVER` è passata da ⏳ a 🟡. **La lezione è quella già scritta qui sotto**: il numero si
> *ricalcola*, non si aggiorna a mente — ed è per questo che il comando è nel documento.

Misura riproducibile, così il numero non si cita a memoria:

```bash
awk -F'|' '/^\| \*\*PIE-/ {s=$(NF-1);
  if (match(s, /✅|🟡|⏳/)) c[substr(s, RSTART, RLENGTH)]++; else c["nessuno"]++ }
  END {printf "verde=%d parziale=%d aperta=%d senza-marcatore=%d\n",
       c["✅"], c["🟡"], c["⏳"], c["nessuno"]}' \
  docs/technical/test-manuali-pie.md
```

Delle 69 aperte, **50 stanno negli otto gruppi qui sotto** (`1+9+9+4+3+1+2+21 = 50`). Le **19 mancanti** sono
7 voci aggiunte il 2026-08-08 da un'altra sessione, le **10** `PIE-STATE-*` di E34 — che non entrano in una
sessione di verifica perché non sono eseguibili — più `PIE-PREVIEW-PERSIST` e `PIE-FACING-1`, aperte il
2026-08-09. (`PIE-MUT-BASTION-SLOW` è nata e chiusa lo stesso giorno, quindi non compare qui.) Le lascio
dichiarate invece di gonfiare una riga a caso, perché una somma che torna con sé stessa è il modo più facile
di sembrare verificati senza esserlo (vedi la nota qui sopra). Chi le ha scritte sa dove vanno.

**Nove delle diciannove sono nel subset `RELEASE-V01`** — `PIE-HEXPLAY-1/2/3/8`, `PIE-V01-HUD`,
`PIE-V01-LOG`, `PIE-V01-INTENT`, `PIE-V01-ROSTER`, `PIE-FACING-1` — e questa è la conseguenza che vale la pena
vedere: **il gate G9 dipende per metà da voci che nessuna seduta pianifica**. Assegnarle vale più che
eseguirne una a caso.

> ⚠️ **Nota di metodo.** Fino al 2026-08-08 questa riga diceva «le **31** aperte in sei gruppi», con
> una somma «verificata» di `2+9+9+4+4+1+2 = 31`, mentre poche righe sopra il conteggio **misurato** ne
> diceva 34. La somma tornava **con sé stessa**: è il modo più facile di sembrare verificati senza esserlo.
> La regola è che questa ripartizione deve coincidere col conteggio misurato sopra — se non coincide,
> è **questa tabella** a essere indietro, e si riconta col comando.

| Gruppo | Voci | Nota |
|---|---|---|
| **Eseguibile subito** (sessione G) | `V01-MATCHEND` | **1** — nessuna precondizione oltre a una partita avviata (CP 10.3). `TEST-CONSOLE` è uscita da qui il 2026-08-08 (🟡) e **`PREVIEW-AREA` il 2026-08-09 (✅)**: tre difetti per chiuderla, nessuno visibile a un test. Il suo residuo `PIE-PREVIEW-PERSIST` è ⏳ e **non ha ancora una seduta** |
| **Partita hex** (sessione D) | `HEXPLAY-4/4b/5/6/6b/6c/7/9/10` | **9** — è il **gate di M6** (CP 6.8). ⚠️ **Non serve più costruire la mappa a mano**: `MapSource = GeneratedTestArena` genera già esagono r=4, ostacoli, **muro che blocca la vista**, fango a costo 3, piattaforma su layer 1 e **una** transizione |
| **Editor** (sessione A) | `HEX-LAYER` `HEX-TRANS` `HEX-MODE-E/F/G/H/L/N/O` | **9** — verificano gli **strumenti**, non più un prerequisito della sessione D |
| **Durata e scala** (sessione F) | `V01-MATCHLEN` `V01-MAPSCALE` `V01-READY` `V01-OVERWATCH` | **4** — producono numeri di playtest, non superano gate. `READY` e `OVERWATCH` descrivono un comportamento **atteso** (countdown ed E14) che non esiste ancora |
| **In attesa di codice** | `V01-ELEC` `V01-FIREWATER` (E8, ora **chiusa**: il codice c'è, la verifica a schermo no) · `V01-HUD` (E11) | **3** — `V01-LOWCOVER` è uscita da qui il 2026-08-07 (CP 9.1) e `V01-DOOR` il 2026-08-08 (CP 9.3): per entrambe la regola è ora coperta headless, e sono passate a 🟡 |
| **Asset da preparare** | `V01-COVEREDIT` | **1** — editing delle coperture nel data asset mappa (CP 9.1). ⚠️ `DA_HexMap_Sandbox` è oggi **vuoto**: va ridisegnato |
| **Animazioni** | `AS4a` `AS4b` | **2** — richiedono i montage Paragon |
| **Scenari visivi** (corpus `Visual.*`) | `VIS-FIRE` `VIS-ICE` `VIS-WETFIRE` `VIS-KO` `VIS-CHARGE` `VIS-ROUGH` `VIS-COMBO` `VIS-COORD` `VIS-PUSH` `VIS-FALLBACK` `VIS-SMOKE` `VIS-PHASES` `VIS-LEVEL` `VIS-COVER` `VIS-DOOR` `VIS-HIGH` `VIS-INTERPOSE` `VIS-DEFLECT` `VIS-HIGHCOVER` `VIS-GUARD` `VIS-BRACE` | **21** — nessuna precondizione oltre a scegliere lo scenario e premere Play. Non sono gate: la regola è già coperta headless dalle assertion, qui si guarda la **leggibilità**. Catalogo: [`scenari-validazione-visiva.md`](scenari-validazione-visiva.md) · classe **B** in [`scenario-map.md`](scenario-map.md) |

## Subset di release — il marcatore `RELEASE-V01`

Il gate **G9** della [Definition of Done](../roadmap/v0.1-definition-of-done.md) chiede che «il subset
`RELEASE-V01` delle verifiche manuali sia eseguito» e dichiara che **la marcatura vive qui**. Fino al
2026-08-09 non viveva qui: `git grep -c RELEASE-V01 HEAD -- docs/` dava **sei righe in tre file** — quattro
nei sorgenti archiviati, due nella DoD stessa — e **zero in questo registro**. Un gate che nomina un insieme
vuoto non è verificabile, ed era la **seconda** volta per G9, che prima citava «le 12 verifiche `PIE-V01-*`»
quando il registro ne contava 74.

Una voce marcata `RELEASE-V01` è una voce **senza la quale la v0.1 non è consegnabile**. Il criterio non
aggiunge nulla alla DoD: sono le tre cose che nomina — *la partita completa su hex multilivello, la fine
partita a tre vie, la leggibilità minima*.

**Restano fuori, per criterio e non per comodità**: gli strumenti dell'editor (non entrano nella build di
gioco), le voci che producono **numeri di playtest** (G11 chiede di *avere* i numeri, non di centrarli), il
corpus `Visual.*` (leggibilità, non consegnabilità: la regola è già coperta dalle assertion), e tutto ciò che
appartiene a E34 o alla v0.2.

**17 voci** — si contano col comando, non a memoria:

```bash
# il marcatore compare anche nella prosa qui sopra: si conta la RIGA DI TABELLA, non la parola
grep -c '^| \*\*PIE-[A-Za-z0-9.-]*\*\* `RELEASE-V01`' docs/technical/test-manuali-pie.md    # 17

awk -F'|' '/RELEASE-V01/ && /^\| \*\*PIE-/ {s=$(NF-1);
  if (match(s, /✅|🟡|⏳/)) c[substr(s, RSTART, RLENGTH)]++ }
  END {printf "verde=%d parziale=%d aperta=%d\n", c["✅"], c["🟡"], c["⏳"]}' \
  docs/technical/test-manuali-pie.md                              # 2 / 7 / 8 al 2026-08-09
```

Motivazione voce per voce e stato aggregato: [`scenario-map.md`](scenario-map.md) §8.

**Nove delle diciassette non stanno in nessuna seduta** — `PIE-HEXPLAY-1/2/3/8`, `PIE-V01-HUD`,
`PIE-V01-LOG`, `PIE-V01-INTENT`, `PIE-V01-ROSTER`, `PIE-FACING-1` — e per la regola già scritta qui sotto
(«una voce che non sta in una seduta non viene eseguita mai») è la cosa da sistemare per prima: **assegnarle
a una seduta vale più che eseguirne una a caso**.

`PIE-FACING-1` è entrata nel subset il 2026-08-09, con la chiusura di **E16**, e non per completezza: dal
CP 16.2 l'emisfero posteriore è **scoperto**, quindi il facing decide il danno. Se l'orientamento che si vede
non è quello che il resolver ha usato, il giocatore non può pianificare una difesa direzionale — è
leggibilità minima nel senso più stretto, non presentazione.

> Spostare una voce dentro o fuori è legittimo, purché la si sposti **qui e in `scenario-map.md` insieme** e
> purché il motivo sia il criterio. Quello che non è legittimo è lasciarlo vuoto.

## Checklist

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-AS5** | Anello di team a terra | `M_TeamRing` creato + assegnato a `TeamRingMaterial` sui `BP_Unit` | Anello **blu** (team 0) / **rosso** (team 1) sotto ogni unità, visibile dall'alto; senza `M_TeamRing` nessun anello (cilindro colorato come prima) | ✅ 2026-08-05 |
| **PIE-SEL** | Anello di selezione (anche su skeletal) | `M_SelectionRing` creato + assegnato a `SelectionRingMaterial` sui `BP_Unit` | Selezionando un'unità compare un **anello giallo** a terra (cornice esterna al TeamRing), visibile anche quando il cilindro è nascosto (personaggio skeletal); deselezionando sparisce. Senza `M_SelectionRing`: nessun anello (fallback: resta solo l'ingrandimento del cilindro) | ✅ 2026-08-05 — **non serve un materiale dedicato**: basta assegnare `M_TeamRing` a `SelectionRingMaterial`, il colore (giallo) lo imposta il codice sul MID via parametro `Color`; i due anelli restano distinguibili per scala (1.6 team, 1.9 selezione) |
| **PIE-P3** | Combat log mostra i reason (TurnLog) | — (funziona anche col cilindro) | Destinazione contesa → log «fermo (cella contesa)»; attacco senza LOS → «nessuna linea di tiro» | ✅ 2026-08-05 — entrambi i reason osservati nel log: contesa in fase di risoluzione, e `BP_Unit_Ranger_C_1 coperto (nessuna linea di tiro)` **in pianificazione** (il controller valida la LOS al momento del bersagliamento, non solo al lock-in) |
| **PIE-AS2** | Personaggio skeletal appoggiato a terra | `BP_Unit_Guardian` (Gideon, `VisualZOffset=0`) → `GuardianUnitClass` | Al posto del cilindro compare il personaggio, a terra (nessun «fluttuamento») | ✅ 2026-08-05 |
| **PIE-AS4a** | Locomozione Idle↔Run | `ABP_Gideon` + bind dei delegate (guida-animazioni-paragon) | In fase **Move** Gideon passa a `Jog_Fwd`, torna `Idle` a fine risoluzione | ⏳ |
| **PIE-AS4b** | Colpi e morte (montages) | `AM_Gideon_Cast/Hit/Death` + bind `OnAttackResolved`/`OnUnitDefeated` | Nel **Blast**: attaccante gioca `Cast`, bersaglio `Hit`; morte → `Death` | ⏳ |
| **PIE-FACING** | Orientamento al movimento | `bFaceMovementDirection=true` sul `BP_Unit` | L'unità ruota (yaw) verso la direzione di corsa; `Jog_Fwd` credibile in ogni direzione | ✅ 2026-08-05 — corsa orientata correttamente. **Nota di design**: a fine movimento l'unità resta voltata verso l'ultima direzione percorsa (scelta confermata: non torna a un orientamento "avanti") | 
| **PIE-MP4** | Click → layer (multilivello) | mappa col ponte sopraelevato | Il click seleziona la cella del **layer giusto** (terra vs ponte) | 🟡 **logica coperta headless** da `RefactorTactics.Hex.WorldToCellIdRoundTripAcrossLayers` (il punto-mondo torna la cella **completa**, layer incluso, e la composizione e' la stessa usata dal click di gioco). ⏳ al PIE resta il gesto col mouse su celle sovrapposte: che cliccando il ponte si selezioni la cella del ponte e non quella sotto |
| **PIE-CP1.4** | Evidenziazione cella sotto il cursore | — | La cella sotto il mouse è evidenziata | ✅ 2026-08-05 |
| **PIE-HEX** | Griglia esagonale graybox (pivot) | `ARTHexMapActor` in un livello, `DemoRadius > 0` | Griglia di celle esagonali visibile (graybox); con `MapAsset` popolato mostra quelle celle | ✅ 2026-08-05 (con `DemoRadius=0` la griglia resta: viene dall'asset) |
| **PIE-HEX-LAYER** | Filtro layer attivo (H4) | `ARTHexMapActor` con celle su ≥2 layer (es. `GenerateIntoAsset` con `ActiveLayer=0`, poi `ActiveLayer=1`) | `LayerView=ActiveOnly` mostra **solo** le celle di `ActiveLayer`; `AllLayers` le mostra tutte, impilate per quota (`LayerHeight`) → la viz non confonde i livelli | ⏳ (H4b) |
| **PIE-HEX-TRANS** | Transizione verticale bridge/scala (H4) | due celle sovrapposte (stessi X/Y, Layer diverso), `TransitionFrom`/`TransitionTo` impostati | `AddVerticalTransition` collega i due layer (Undo/Redo ok, package dirty, validator pulito); `RemoveVerticalTransition` lo toglie | ⏳ (H4b) |
| **PIE-HEX-MODE-A** | Editor Mode hex appare e si attiva (H5a) | modulo `RefactorTacticsEditor` compilato | Nella toolbar Modes compare «Hex Map»; attivandolo il pannello si apre senza crash (nessun tool) | ✅ 2026-08-05 |
| **PIE-HEX-MODE-B** | Selezione a click nel viewport (H5b) | mode Hex Map attivo, `ARTHexMapActor` nel livello (selezionato o unico) | Tool «Select» attivo → click su una cella → esagono giallo sulla cella + `SelectedCell`/superficie/costo/blocco corretti nel pannello; cambiando `ActiveLayer` sull'actor seleziona il piano giusto (celle sovrapposte) | ✅ 2026-08-05 |
| **PIE-HEX-MODE-C** | Paint a click nel viewport (H5c) | mode Hex Map attivo, tool Paint, `ARTHexMapActor` nel livello | Con `Operation=Paint`, click su una cella → esagono verde + cella creata/aggiornata (superficie/costo/blocco del pennello); `LastCell` corretto; Undo ripristina | ✅ 2026-08-05 (il refresh dopo Undo richiedeva il fix `ea51b45`) |
| **PIE-HEX-MODE-D** | Erase a click nel viewport (H5c) | mode Hex Map attivo, tool Paint | Con `Operation=Erase`, click su una cella esistente → esagono rosso + cella rimossa dall'ISM; Undo ripristina; cambiando `ActiveLayer` agisce sul piano giusto | ✅ 2026-08-05 |
| **PIE-HEX-MODE-F** | Render transizioni nel tool Arch (H5c.2a) | mode Hex Map, tool Arch, `ARTHexMapActor` con transizioni | Le transizioni esistenti appaiono come linee colorate (per Kind) con freccia From->To | ⏳ (H5c.2a) |
| **PIE-HEX-MODE-E** | Crea transizione via gizmo (H5c.2b) | mode Hex Map, tool Arch, `ARTHexMapActor` con celle su >=2 layer | Click From → gizmo → drag su To (anche altro layer, snap a cella) → Commit crea la transizione (visibile); Undo la rimuove; ClearArch annulla il pendente | ⏳ (H5c.2b) |
| **PIE-HEX-MODE-G** | Ciclo di vita del gizmo (smoke, H5c.2b) | mode Hex Map, tool Arch, `ARTHexMapActor` nel livello | Click su una cella → compare il gizmo di traslazione; **re-click** su un'altra cella → resta **un solo** gizmo (nessun duplicato); **cambio tool** (Select) o uscita dal mode → il gizmo **sparisce** (nessun gizmo orfano in scena) | ⏳ (H5c.2b) |
| **PIE-HEX-MODE-H** | Snap del gizmo cross-layer (H5c.2b) | mode Hex Map, tool Arch, celle su >=2 layer | Trascinando il gizmo, `To` si aggancia sempre al **centro di una cella**; alzando la quota di ~`LayerHeight` il target passa al **layer superiore** (`WorldToLayer`); nessun jitter/loop durante lo snap (guardia `bSnapping`) | ⏳ (H5c.2b) |
| **PIE-HEX-MODE-I** | Drag-paint (H5c.3b) | mode Hex Map, tool Paint (`Operation=Paint`), `ARTHexMapActor` con `MapAsset` | Tenere premuto e trascinare dipinge più celle in una pennellata (dedup: ripassare non ridipinge); **un** Ctrl+Z annulla l'intera pennellata; click singolo = 1 cella (PIE-C invariato) | ✅ 2026-08-05 |
| **PIE-HEX-MODE-J** | Drag-erase (H5c.3b) | mode Hex Map, tool Paint (`Operation=Erase`) | Trascinare cancella più celle in una pennellata; un Undo le ripristina tutte; cambiare tool a metà drag non lascia transazioni aperte; **erase su celle inesistenti/vuote NON crea voci Undo né marca l'asset dirty** (transazione lazy) | ✅ 2026-08-05 |
| **PIE-HEX-MODE-K** | Pennello a raggio N (H5c.4) | mode Hex Map, tool Paint, `ARTHexMapActor` con `MapAsset` | `BrushRadius=0` → 1 cella (come prima); `BrushRadius=N>0` → un click dipinge/cancella l'esagono pieno di raggio N; drag dipinge fasce larghe (dedup); **un** Ctrl+Z annulla l'intera pennellata | ✅ 2026-08-05 |
| **PIE-HEX-MODE-L** | Rimuovi arco via tool (H5c.5b) | mode Hex Map, tool Arch, `ARTHexMapActor` con transizioni | Con `Operation=Remove`, click su un arco disegnato lo rimuove (Undo lo ripristina); click nel vuoto (nessun arco entro soglia) non fa nulla; con `Operation=Add` il flusso gizmo resta invariato | ⏳ (H5c.5b) |
| **PIE-HEX-MODE-M** | Overlay debug superfici (H5c.6) | mode Hex Map, tool Select o Paint, `ARTHexMapActor` con celle di superfici diverse | Con `bShowOverlay` attivo, ogni cella appare come esagono colorato per superficie (Water blu, Fire arancio, Mud marrone, ...); le celle bloccate hanno un esagono rosso interno; `bShowOverlay` off = nessun overlay | ✅ 2026-08-05 |
| **PIE-HEX-MODE-N** | Secchiello / flood-fill (H5c.7) | mode Hex Map, tool Fill, `ARTHexMapActor` con `MapAsset` popolato | In Fill, click su una regione la riempie col pennello corrente; un Ctrl+Z ripristina l'intera regione; click su cella vuota non fa nulla; passando a Select/Paint con overlay si vedono i nuovi colori | ⏳ (H5c.7) |
| **PIE-HEX-MODE-O** | Default `MoveCost` dal catalogo terreni (CP 8.1) | mode Hex Map, tool Paint, `ARTHexMapActor` nel livello | Cambiando `Surface` nel pannello del pennello (es. a `Rough`), `MoveCost` si aggiorna da solo al valore del catalogo (`2` per `Rough`); `bBlocksMovement` resta `false` | ⏳ |
| **PIE-BU2** | Bot: posizionamento via utility scoring | partita avviata | In pianificazione il bot sceglie la cella pesando **minaccia/kiting** (può **restare** invece di esporsi); il combat log mostra `<Bot>: utility -> (x,y,Lz) score=N`. Il kiter (Ranger) mantiene la distanza, la mischia (Guardian) chiude, nessuno corre in celle sotto tiro. Osserva se gli score hanno senso → base per il **tuning dei pesi** (BU.3) | ✅ |
| **PIE-BU2b** | Tuning pesi bot in editor | PIE attivo | Modificando `WKill/WThreat/WKiteViolation/WApproach/WDamage/WElevation` sul `TurnManager` (World Outliner → Details ▸ *Bot*) il comportamento cambia **dal turno successivo, senza ricompilare**: es. ↑`WThreat` = bot più prudente; ↓`WApproach` = mischia meno aggressiva; ↑`WElevation` = predilige le alte quote. Dettagli nella nota sotto | ✅ |
| **PIE-BU3** | Bot: utility unica posizione/attacco | **dopo** refactor BU.3b | Un'unica utility sceglie fra **{resta e attacca}** e **{muoviti per posizionarti}** (l'attacco vale solo da fermo: il Blast precede il Move). Verifica: se attaccare da fermo espone troppo il bot preferisce ripararsi invece di sparare; se l'attacco **uccide** spara sempre; guardie **support/panic/dash** intatte; log `utility -> ... attacca X score=N` oppure `... score=N (resta)` | ✅ |
| **PIE-BU3c** | Bot: dash+attacco (scatto poi colpisce) | **dopo** BU.3c | Se scattando raggiunge una cella da cui ha tiro e l'attacco conviene (utility), il bot pianifica **scatto + attacco** (log `utility -> scatto (x,y,Lz) + attacca X`): nel Blast (dopo il Dash) colpisce dalla cella post-scatto. **Nota**: se lo scatto è deviato da un conflitto di movimento simultaneo, l'attacco può mancare (log `nessuna linea di tiro`) — coerente coi turni simultanei | ✅ |

### Partita su griglia esagonale (M6 — Parità hex)

> Voci **pianificate in anticipo**, per definire *prima* cosa dovrà dimostrare lo switch, così la verifica non
> viene inventata a lavoro finito. Precondizioni comuni: un livello con `ARTHexMapActor` + `MapAsset` popolato
> (vedi «Mappa di prova» sotto) e il `RTGameMode` che allestisce la partita su quella mappa.
>
> **Tutte eseguibili da CP 6.7** (2026-08-06): allestimento, input, movimento, scatto, collisione, LOS, forme,
> spinta, bot e HUD passano dallo strato esagonale, con un'unica fonte di scala
> (`ARTTurnManager::GetHexContext`, la stessa che usa la HUD). La **8** (multilivello) richiede in più una
> mappa con due layer e un arco, cioè l'artefatto d'editor della seduta U-multilivello.

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-HEXPLAY-1** `RELEASE-V01` | Allestimento della partita su mappa hex | livello di prova + GameMode hex | All'avvio del PIE si vede la griglia **esagonale** con 4 unità (2v2) **centrate sui centri-cella** (nessun offset né compenetrazione); la camera inquadra l'arena; nessun residuo della griglia quadrata | 🟡 **coperto headless** da `RefactorTactics.MatchSetup.GameModeSpawnsOnHexMap` (board 2v2 sulle celle di partenza, nessuna sovrapposizione), `…GameModeFallsBackOnEmptyMapAsset` e `…MapSourceTestArenaWinsOverLevelAsset`. ⏳ al PIE resta cio' che solo l'occhio vede: unita' **centrate sui centri-cella** senza offset ne' compenetrazione, e nessun residuo di griglia quadrata a schermo. Nota: le unita' sono **cilindri** (i `BP_Unit_*` non esistono piu', fallback previsto); con `MapSource=GeneratedTestArena` si gioca sulla mappa di prova generata da codice |
| **PIE-CAM-START** `RELEASE-V01` | La partita si apre sulla propria squadra, da vicino | partita hex avviata | All'avvio la camera è centrata sul **punto medio delle proprie unità** (non sul centro della mappa) e più ravvicinata di `Home`: braccio a `MatchStartArmLength` (default 450) invece di `DefaultArmLength` (800). Nel log compare `Camera sulla squadra <TeamId> (<N> unità, arm=…)` e il `CameraPawn BeginPlay` successivo riporta lo **stesso** braccio — se riportasse 800 l'inquadratura sarebbe stata sovrascritta. `Home` deve continuare a mostrare l'insieme della mappa a 800 | ✅ **2026-08-06** — `Camera sulla squadra 0 (2 unita', arm=450)` seguito da `CameraPawn BeginPlay (arm=450, pitch=-40)`: applicata e non sovrascritta, riuscita al **primo** tentativo (nessun ripiego su `RecenterView`). Confermata a schermo dall'utente |
| **PIE-HEXPLAY-2** `RELEASE-V01` | Selezione e cella sotto il cursore | partita hex avviata | Click su un'unità la seleziona; l'evidenziazione (esagono **giallo**) segue la cella **esagonale** sotto il mouse; su mappa multilivello si seleziona la cella del **layer giusto** — il layer viene dalla **quota** del punto colpito, quindi cliccando il ponte si evidenzia la cella del ponte | 🟡 **2026-08-06** — selezione ✅ (`Selezionata: RTUnit_1`, `RTUnit_0`), guardia sulle avversarie ✅ (`e' avversaria: seleziona prima una tua unita'`), evidenziazione gialla sotto il cursore ✅ confermata a schermo dall'utente. ⏳ resta il **layer su mappa multilivello**: l'arena di ripiego ha un solo layer, serve la mappa di prova |
| **PIE-HEXPLAY-3** `RELEASE-V01` | Pianificazione del movimento entro budget | unità del giocatore selezionata | Una cella valida aggiunge un waypoint e mostra l'anteprima del percorso (esagoni **ciano** + segmenti fra i centri); una cella **oltre il budget**, **bloccata**, **occupata** o **fuori mappa** viene rifiutata: il piano precedente resta intatto e il log riporta il motivo. Più waypoint deviano il percorso (non prende la scorciatoia) e il budget si spende **cumulativamente**. **Click destro** (o `Backspace`) annulla l'ultimo waypoint e l'anteprima si accorcia | 🟡 **logica e interazione coperte headless 2026-08-06** — la sequenza col mouse e' guidata dai test: `RefactorTactics.PlayerInput.WaypointClicksBuildAndRejectPlans` (click validi che allungano il piano; cella **bloccata** e cella **fuori mappa** rifiutate col piano precedente **intatto**; budget esaurito) e `…PlayerInput.UndoShortensThePlan` (annullare accorcia, annullare tutto torna a «resto fermo»). Budget cumulativo e rifiuti verificati anche in partita (`costo 1/5 → 5/5`). ⏳ al PIE resta **solo il visivo**: che l'anteprima ciano e i marker dei waypoint si **vedano** e coincidano col percorso poi eseguito |
| **PIE-HEXPLAY-4** `RELEASE-V01` | Risoluzione e playback del movimento | piani impostati, lock-in con **Spazio** | Le unità scorrono di cella in cella lungo il percorso risolto; a fine playback ogni unità è **esattamente** sul centro della cella finale e la posizione visiva coincide con la cella logica (nessuna deriva accumulata) | ⏳ **eseguibile da CP 6.2**. Coperto headless da `RefactorTactics.HexMove.UnitReachesPlannedCell`; il PIE aggiunge cio' che il test non vede: la fluidita' dello scorrimento |
| **PIE-HEXPLAY-3b** | Il rifiuto del bersaglio dice il motivo **giusto** | unità propria selezionata, un nemico **fuori portata** e uno **dietro una copertura** | Cliccando il nemico **fuori portata** il log dice «fuori portata (max N)» — **mai** «coperto», nemmeno su un'arena senza un solo muro. Cliccando quello dietro una cella con `bBlocksLineOfSight` dice «coperto (nessuna linea di tiro)». Con l'abilità in ricarica o senza energia dice «non pronta», prima di ogni altra verifica. Nel Blast, un attacco fermato dalla copertura compare nel combat log come reason code con coordinate assiali | 🟡 **2026-08-06** — metà **fuori portata** ✅ dal log: `RTUnit_3 fuori portata (max 3)` e `(max 7)` su un'arena **senza un solo muro**, dove prima usciva «coperto». Anche il caso positivo osservato: `Piano: RTUnit_0 usa Colpo preciso su RTUnit_3`. ⏳ resta la metà **coperto**: serve una cella con `bBlocksLineOfSight`, che l'arena di ripiego non ha. Coperto headless da `Combat.HexTargetingReasonDistinguishesRangeFromCover` e `HexCombat.NoMapFailClosed` |
| **PIE-HEXPLAY-4b** | Scatto (fase Dash) su hex | unità con abilità di scatto pronta, destinazione entro la sua portata | Lo scatto si risolve **prima** del Blast e porta l'unità sulla cella scelta anche in direzione obliqua (dove il budget quadrato l'avrebbe rifiutata); oltre la portata, su cella occupata o bloccata lo scatto **non avviene** (l'unità resta, la ricarica scatta comunque); scatto + movimento nello stesso turno restano compatibili | ⏳ **eseguibile da CP 6.5**. Coperto headless da `RefactorTactics.HexMove.DashReachesCellOnHex` / `DashRejectsOutOfBudget` |
| **PIE-HEXPLAY-5** `RELEASE-V01` | Collisione simultanea su hex | due unità pianificate verso la **stessa** cella | Entrambe restano ferme (o si fermano prima), **nessuna sovrapposizione**; il combat log riporta il reason «cella contesa»; ripetendo con lo scambio diretto A↔B lo scambio **riesce** | ⏳ **eseguibile da CP 6.2**. La contesa e' coperta headless da `RefactorTactics.HexMove.ContestedCellStopsBoth` (due esiti `BlockedContested` nel TurnLog). ⚠️ **La seconda metà di questa voce è FALSA** (verificato il 2026-08-08): lo scambio diretto A↔B **non riesce**, e non perché il resolver lo vieti — perché la **pianificazione** lo rifiuta prima (`FindPathForUnit`: goal occupato → `NoPath`). Cliccando la cella di un nemico adiacente non si ottiene alcun percorso, quindi lo scambio non è nemmeno pianificabile. Comportamento fissato da `Scenario.RunnerSwapRejectedByPlanning`; il conflitto con `HexSim.ResolveSwapAllowed` è descritto in `Scenarios/Movement/SwapRejectedByPlanning.json`. **Al PIE resta da confermare che il log dica il motivo** invece di lasciare il giocatore a chiedersi perché l'unità non si muove |
| **PIE-HEXPLAY-6** `RELEASE-V01` | Copertura: LOS esagonale | una cella con `bBlocksLineOfSight` fra attaccante e bersaglio | L'attacco pianificato attraverso il muro viene scartato con «nessuna linea di tiro»; spostandosi di una cella di lato il tiro va a segno. Con un ostacolo su un **altro layer** il tiro passa (regola di elevazione) | ⏳ **eseguibile da CP 6.4**. Coperto headless da `RefactorTactics.HexBlast.NoLineOfSightOnHexMap`; il PIE aggiunge ciò che il test non vede: che il giocatore **capisca** dal log perché il colpo non parte, e la prova sul multilivello |
| **PIE-HEXPLAY-6b** | Forme d'attacco su esagoni | **Flux** (`LinearDischarge` = linea, `Overload` = area r1) e **Riva** (`PressureJet` = linea, `CircularTide` = area r1) con più nemici in zona | La **linea** colpisce anche chi sta sulla traiettoria prima del bersaglio; l'**area** colpisce il bersaglio e i suoi 6 vicini. `CircularTide` **cura gli alleati e bagna i nemici**: il fuoco amico va giudicato su `Overload`, non su di essa. Il combat log elenca un colpo per bersaglio. ⚠️ **Il cono non è verificabile in partita**: `HexCone` esiste e ha i suoi test, ma **nessuna abilità del roster v0.1 lo usa** (0 occorrenze di `ERTAbilityShape::Cone`) — la voce citava «Guardian (Spazzata)», archetipo quadrato rimosso al CP 3.2 | ⏳ **eseguibile da CP 6.4**. Forme coperte headless (`RefactorTactics.HexCombat.Shape*`); l'**anteprima della zona esiste dal 2026-08-07** (contorno **rosso** sulle celle colpite, **arancione** sugli alleati), col wiring verificato da `Preview.HitCellsMatchCombatShape` — l'anteprima riceve le celle da `HexHitCells`, quindi non può divergere dall'esito. Il PIE verifica che quella zona sia **leggibile a colpo d'occhio**: si capisce dove finirà il cono *prima* di lanciarlo? |
| **PIE-HEXPLAY-6c** | Spinta del Guardian su hex | Guardian che colpisce con Spazzata (knockback 2) | Il bersaglio è respinto lungo una delle **6 direzioni esagonali** (quella del colpo), non lungo un asse cardinale; si ferma davanti a un ostacolo, a un'altra unità o al bordo della mappa; due spinte opposte sullo stesso bersaglio si **annullano** e due bersagli spinti verso la stessa cella restano entrambi fermi | ⏳ **eseguibile da CP 6.5**. Coperto headless (`HexCombat.Knockback*`, `HexBlast.KnockbackOnHexGrid`); il PIE verifica lo **scivolamento animato** lungo il percorso |
| **PIE-HEXPLAY-7** | Bot su hex | almeno un'unità con `bIsBotControlled` | Il bot propone **solo mosse legali** (mai celle occupate o fuori budget), preferisce le celle al riparo, il kiter mantiene la distanza e la mischia chiude; il log utility mostra celle in coordinate **assiali** `(q,r,L)` | ⏳ **eseguibile da CP 6.6**. Coperto headless da `RefactorTactics.HexBotPlay.*` (mosse legali, panico, supporto, tuning, scatto prudente); il PIE aggiunge il **giudizio sul comportamento**: gli score hanno senso guardando la partita? |
| **PIE-HEXPLAY-8** `RELEASE-V01` | Multilivello: movimento via arco | mappa con due layer collegati da una transizione | Un percorso che usa scala/ponte **cambia layer**; il playback porta l'unità alla quota giusta (`LayerHeight`); rimuovendo l'arco i due layer tornano irraggiungibili (il path fallisce, non «teletrasporta») | 🟡 **coperto headless**: da `RefactorTactics.HexMove.ClimbsOnlyThroughTransition` (2026-08-06) e, con **CP 9.4** (2026-08-08), da `Structures.Bridge.RemovalBreaksPath` — il percorso fra i due layer **fallisce** (`NoPath`, nessuna cella restituita) e la cella oltre esiste ancora, quindi manca il collegamento e non la destinazione — e da `Structures.Bridge.NoTeleportOnRemoval`, che lo verifica su un **turno vero**: il ponte cade nel Blast e chi lo stava attraversando resta dov'era. Al PIE resta il **playback**: che l'unità salga alla quota giusta (`LayerHeight`) invece di scivolare sul piano, e che il crollo del ponte si veda |
| **PIE-HEXPLAY-9** `RELEASE-V01` | HUD e anteprima piani su hex | partita hex avviata | Barre HP/scudo/energia, timer, fase e combat log invariati; l'anteprima dei piani (ciano), i marker dei waypoint, la preview dello scatto (magenta) e la traccia post-lock (grigia) seguono i **centri esagonali** e coincidono col percorso realmente eseguito; il combat log riporta i reason code con coordinate **assiali** `(q=..,r=..,L=..)`; **`Home`** ricentra la camera sulla mappa esagonale | ⏳ **eseguibile da CP 6.7** |
| **PIE-FACING-1** `RELEASE-V01` | L'orientamento logico e quello che si vede sono lo stesso | partita hex avviata, un'unita' che si muove e una che attacca in un'altra direzione | A fine playback la mesh **guarda dove guarda la regola**: chi si e' mosso e' orientato lungo l'ultimo passo, chi ha attaccato verso il bersaglio. Premendo **Spazio** per saltare il playback l'esito e' lo **stesso** (lo snap passa da `FinishPlayback`, che il salto attraversa). Il combat log mostra le voci di orientamento con la direzione (`si orienta a NE (movimento)`), e quando un colpo arriva da dietro dice **perche'** la copertura non ha protetto | ⏳ **CP 16.1/16.2 (2026-08-09)**. Coperto headless da 13 test `Facing.*` e 5 scenari `Spec.Facing.*`, che pero' leggono il facing **logico**: al PIE resta l'unica cosa che nessun test vede, cioe' se la figura a schermo e il valore che decide le regole coincidono davvero. ⚠️ Manca un **indicatore dell'arco frontale** in HUD: senza, il giocatore non puo' sapere da dove e' scoperto prima di essere colpito — e' il motivo per cui il gate `ui_wiki` della feature resta `partial` |
| **PIE-HEXPLAY-10** `RELEASE-V01` | Partita completa fino alla vittoria | partita hex avviata, almeno un'unità per squadra col bot | La partita arriva a una **conclusione**: una squadra viene eliminata, compare «PARTITA FINITA» e `R` riavvia. Durante i turni: nessuna unità sovrapposta o fuori mappa, nessun blocco della pianificazione. **Dato di riferimento** (misurato headless il 2026-08-06): bot contro bot la partita si decide al **turno 10**, dentro il limite di 12 del catalogo. Era **25** finché lo scudo di supporto non scadeva e si accumulava (issue `#96`, risolta) | ⏳ **eseguibile da CP 6.7**. Coperta headless da `RefactorTactics.HexMatch.PlaysToCompletion` (tenuta e invarianti); il PIE aggiunge il **giudizio sul ritmo**, che nessun test può dare |

### Contenuto della v0.1 (catalogo azioni, eroi, ambiente, strutture)

> Voci pianificate in anticipo per la release **v0.1** ([`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md)). Le prime dodici
> traducono la matrice di test manuali del catalogo di bilanciamento (§14); le ultime cinque coprono roster,
> HUD e strumenti di debug.
> Precondizioni comuni: partita hex avviata (sessione D verde) e catalogo v0.1 caricato dai data asset.
> Riferimento issue in [`v0.1-issue-plan.md`](../roadmap/v0.1-issue-plan.md).
>
> ⚠️ **Riallineamento 2026-08-07** — la frase «*il codice non esiste ancora*» che apriva questa sezione **era
> falsa**: le epic E1, E4, E5 ed E6 sono chiuse (**352** test misurati a fine giornata; erano 324 quando
> questa nota è stata scritta la mattina — le reazioni d'eroe di CP 5.5/6.7 sono arrivate nel frattempo).
> Riclassificazione verificata voce per voce, non a stima:
>
> | Esito | Voci |
> |---|---|
> | ⏳ → 🟡 **coperte headless** | `PUSH` · `INTERCEPT` · `FF` · `ROSTER` · `INTENT` (metà) · `LOG` (parte) |
> | ⏳ **restano aperte** | `ELEC` · `FIREWATER` (E8 CP 8.3/8.4) · `LOWCOVER` (E9) · `DOOR` (E9) · `HUD` (E11) · `DEBUG` (E11) |
>
> Le sei che restano ⏳ **coincidono esattamente** con le epic senza un solo test (`Environment.*`, `Cover.*`,
> `Structures.*`, `UI.*`, `Debug.*`): non è una coincidenza, è la conferma che la riclassificazione è corretta.
> Una voce 🟡 **non è verde**: il test copre la regola, il PIE copre ciò che il test non può vedere — che si
> veda a schermo e che il giocatore lo capisca.

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-V01-COLL** | Collisione sulla stessa cella | due unità pianificate verso la stessa cella, stessa priorità | Entrambe si fermano nella cella precedente; il log riporta il reason; ripetendo con una `Charge` contro un `Move`, la Charge entra e l'altra resta indietro | 🟡 **coperto headless** da `RefactorTactics.HexMove.ContestedCellStopsBoth` e `RefactorTactics.HexSim.ResolveContestedDestination` (destinazione contesa → entrambe ferme, esito indipendente dall'ordine). Al PIE resta da vedere che **a schermo** non si sovrappongano e che il combat log riporti il reason |
| **PIE-V01-ROUGH** | Costo del terreno accidentato | mappa con celle `Terrain.Rough` | Attraversare una cella accidentata consuma **2 MP** invece di 1; con 5 MP il raggio raggiungibile si accorcia di conseguenza; `Dash` e `Charge` **non** la attraversano | 🟡 **coperto headless** da `RefactorTactics.HexSim.ReachableRespectsTerrainCost` e `RefactorTactics.MatchSetup.TestArenaHasTheFeaturesItPromises` (attraversare il fango costa più dei passi percorsi). Al PIE resta da vedere che il **budget mostrato** si riduca di conseguenza |
| **PIE-V01-DASHCOVER** | Dash contro copertura alta | copertura alta sulla traiettoria del Dash | Il Dash si ferma prima della copertura oppure è invalidato in pianificazione (nessun attraversamento, nessun crash) | 🟡 **coperto headless 2026-08-06** — lo scatto e' ora **lineare** (`#46`, CP 4.5 parziale): `RefactorTactics.HexSim.DashIsLinear` verifica che un ostacolo sulla traiettoria **annulli** lo scatto (non lo aggira e non ci si ferma prima), che una cella non allineata alle sei direzioni sia rifiutata e che un layer diverso non sia mai «in linea». `HexMove.DashRefusesBlockedDestination` copre la destinazione bloccata: in RISOLUZIONE vale `Fallback.Stop` (ci si ferma nell'ultima cella libera della traiettoria), mentre in PIANIFICAZIONE la destinazione bloccata è rifiutata — le due cose non si contraddicono, la prima serve a quando il movimento simultaneo altrui chiude una traiettoria che era libera al lock-in (`#142`). `HexMatch.HeroDashResolvesLinearly` copre lo stesso sugli EROI, cioè sul catalogo che il gioco schiera. Al PIE resta da vedere **a schermo** che lo scatto non si pianifichi, invece di partire e fermarsi a metà |
| **PIE-V01-PUSH** | Push verso cella occupata | bersaglio con una cella occupata alle spalle | Nessuno spostamento illegale: la spinta si annulla e l'unità resta dov'è; il log spiega il motivo | 🟡 **coperto headless 2026-08-07** da `RefactorTactics.Actions.Push.InvalidDestination` e `Actions.Pull`. Al PIE resta da vedere che **a schermo** l'unità non si sposti affatto (nessun sobbalzo) e che il log dia il motivo |
| **PIE-V01-ELEC** | Acqua elettrificata | acqua creata da Riva/Sprinkler + `Electrify` di Flux | La propagazione segue le celle conduttive, si ferma a **3 celle**, colpisce ogni unità **una sola volta**; ripetendo la stessa configurazione l'esito è identico | ⏳ |
| **PIE-V01-FIREWATER** | Acqua spegne il fuoco | cella in fiamme + acqua sopra | La cella di fuoco è rimossa e `Burning` cancellato dalle unità coinvolte; il fuoco non si propaga oltre | ⏳ |
| **PIE-V01-LOWCOVER** | Copertura bassa direzionale | copertura bassa su un bordo fra attaccante e bersaglio (si aggiunge dal pannello proprietà del data asset mappa: `Covers` → `Edge`) | L'attacco dal lato protetto infligge **10 danni in meno**; girando attorno e colpendo da un altro lato il danno è pieno | 🟡 **coperto headless 2026-08-07** (CP 9.1) da `Cover.DirectionalDamageReduction`, `Cover.LowCover.WrongSideNoReduction` (bordo opposto **e** i due adiacenti), `Cover.LowCover.AoESameSide`, `Cover.LowCover.NeverHealsTarget`. Al PIE resta la **leggibilità**: che si capisca a schermo da quale lato si è riparati, visto che oggi nessuna mesh rappresenta il riparo |
| **PIE-V01-COVEREDIT** | Coperture nell'asset mappa e migrazione del formato | `DA_HexMap_Sandbox` aperto nell'editor (formato v3 da CP 9.1) | L'asset si apre senza errori; `Cells → Covers` accetta una voce per bordo (`Edge`, `Type = Low`, `Integrity = 30`); **risalvando** l'asset la versione 3 si materializza su disco (finché non lo si risalva la migrazione avviene in memoria a ogni caricamento). Nessun dato preesistente cambia | ⏳ **la logica è coperta headless** da `HexMap.FormatMigrationPreservesCells` e dalla verifica sulla serializzazione reale (`spec-copertura-cp91.md` §7.3: 26 celle, digest identico attraverso v2 → v3). Al PIE resta ciò che il test non vede: **l'editing a mano** e il risalvataggio. ⚠️ `DA_HexMap_Sandbox` è oggi **vuoto (0 celle)**: va ridisegnato prima di usarlo |
| **PIE-V01-INTERCEPT** | Intercept protegge l'alleato | alleato entro 2 celle con `Intercept` preparato | L'intercettore **diventa** il bersaglio dell'attacco diretto; con un AoE o un hazard l'intercetto **non** scatta | 🟡 **coperto headless 2026-08-07** da `Reactions.Intercept`, `InterceptRejectsAoE`, `InterceptRejectsHazard`, `InterceptOnlyNearbyAllies`, `InterceptRequiresCompatibleTrajectory`, `InterceptResolvesBeforeOtherReactions`. **Dal 2026-08-07 è anche un'abilità d'eroe reale** (CP 5.5/6.7): `Heroes.BastionInterpositionRedirectsDirectHit` e `…UsesReactionSlot`, più `Heroes.FluxReactiveCapacitorShieldsAndCounters` e `Heroes.VektorDeflectionReducesDirectHit`. Al PIE resta la **leggibilità**: che si capisca a schermo *chi* ha incassato il colpo al posto di chi |
| **PIE-V01-FF** | Friendly fire su AoE | `CircularAoE` centrato dove c'è anche un alleato | Il danno è applicato **anche** all'alleato; l'HUD/preview lo segnala prima del lock-in | 🟡 **coperto headless 2026-08-07** da `Actions.AoE.FriendlyFire` (il danno **arriva**) e da `Preview.AllyInAreaIsFlagged` (la cella dell'alleato è marcata nell'anteprima). **L'anteprima ora esiste**: la cella dell'alleato dentro l'area si disegna in **arancione** invece che in rosso. Al PIE resta il giudizio: quell'arancione si **nota** prima di premere Spazio, o passa inosservato? |
| **PIE-V01-FALLBACK** | Fallback su bersaglio che si sposta | attacco diretto su un bersaglio che si muove nello stesso turno | Si applica il fallback dichiarato (`Cancel` per gli attacchi diretti): nessun colpo «inseguente», il log riporta il fallback applicato | 🟡 **coperto headless** da `RefactorTactics.Actions.Fallback.*` (8 test: `Stop`, `Wait`, `AttackCell`, `Cancel`, `ValidationReasons`, `LoggedOutcome`, `CancelIsLoggedInMatch`, `NoRandomTargeting`) — il fallback si applica e **compare nel log**, e non esiste bersagliamento automatico casuale. ⏳ al PIE resta da vedere che chi gioca **capisca** cosa e' successo leggendo il combat log, senza dedurlo dal comportamento |
| **PIE-V01-DOOR** | Porta chiusa durante il turno | percorso che attraversa una porta chiusa da un'azione nello stesso turno | Il grafo è ricostruito: l'unità **si ferma** davanti alla porta (`Fallback.Stop`), nessun path fantasma attraverso la porta chiusa | 🟡 **coperto headless 2026-08-08** da `RefactorTactics.Structures.Door.ClosingStopsMovement`, che gira un **turno vero** (porta chiusa nel Blast, percorso già pianificato, unità ferma davanti al varco e `BlockedByTopology` nel TurnLog). Al PIE resta il visivo: che l'unità si **fermi a schermo** senza attraversare l'anta e che il combat log dica il motivo. ⚠️ Serve una mappa con una porta: nessun `.uasset` ne disegna ancora una (limite dichiarato di CP 9.3) |
| **PIE-V01-REPLAY** | Replay dello stesso turno | `rt.Debug.DumpTurnLog` + `rt.Debug.VerifyReplay` | Rieseguendo lo stesso turno con lo stesso seed, TurnLog e checksum sono **identici**; il comando non segnala divergenze | 🟡 **coperto headless** da `RefactorTactics.HexSim.ReplayDivergenceZero` (stesso snapshot → stesso TurnLog e stesso hash). ⏳ al PIE resta il giro con i comandi `rt.Debug.DumpTurnLog` / `rt.Debug.VerifyReplay`, che non esistono ancora (CP 11.4) |
| **PIE-V01-ROSTER** `RELEASE-V01` | Roster dei 4 eroi | `URTHeroData` per Flux, Riva, Bastion, Vektor | Le 4 unità in campo hanno statistiche distinte (90/95/120/100 HP, 5/5/4/6 MP); il bot gestisce MP diversi senza proporre mosse illegali; asset mancante = fallback al cilindro | 🟡 **coperto headless 2026-08-07** da `Heroes.StatsFromData`, `Heroes.SpawnFromData`, `Heroes.SpawnFailsClosedWithoutData` (fallback), `Heroes.RosterIsBalanced` e i quattro `Heroes.<Eroe>.MatchesCatalog`. ⏳ al PIE resta il **giudizio in partita**: che i quattro si sentano diversi da giocare, e che il bot con 4 MP non proponga mosse da 6 |
| **PIE-V01-HUD** `RELEASE-V01` | HUD di partita completo | partita v0.1 avviata | Barre HP/scudo/energia, timer, fase, **round corrente su `RoundLimit`** (il limite letto dal formato, non scritto a mano nel widget), slot occupati (movimento/principale/reazione) e cooldown residui, tutti a schermo e coerenti col simulatore | ⏳ |
| **PIE-V01-INTENT** `RELEASE-V01` | Intenti alleati e certezza | due unità alleate in pianificazione | Gli intenti alleati mostrano i tre livelli **confermato / previsto / incerto**; **nessun** intento avversario è visibile in alcuna forma | 🟡 **metà coperta 2026-08-07**: la **privacy** sì — `Reactions.IntentNotVisibleToEnemy` e `IntentViewSkipsDeadAndKeepsOrder` (nessun intento avversario, ordine stabile). ⏳ resta l'altra metà, i **tre livelli di certezza**, che appartengono a E11 CP 11.2 e non esistono ancora |
| **PIE-V01-LOG** `RELEASE-V01` | Combat log con reason code | un turno con un fallback e una modifica ambientale | Ogni voce riporta `ActionId`, priorità, coordinate assiali `(q,r,L)` e `ValidationResult`; i fallback e le modifiche ambientali sono espliciti | 🟡 **parzialmente coperto 2026-08-07** da `Actions.Fallback.LoggedOutcome`, `Fallback.CancelIsLoggedInMatch`, `Reactions.NotTriggeredIsLogged`, `Terrain.Status.LogMatchesState` (ciò che accade **finisce** nel log). ⏳ resta il **formato**: `ActionId`, priorità e coordinate assiali nella voce, e le modifiche ambientali — che dipendono da E8 CP 8.3/8.4, assenti |
| **PIE-V01-DEBUG** | Comandi `rt.Debug.*` | build Development o PIE | Gli 8 comandi rispondono; le celle mostrano `CellId`/`TerrainId`/`TraversalCost`/`OccupantId`/`HazardTags`/`CoverEdges`/`ChunkRevision`; **`DrawIntent` non rivela gli intenti avversari** | 🟡 **1 comando su 8 esiste**: `rt.Debug.DrawCells` (`Map/RTHexOverlayConsole.cpp`), verificato in PIE il 2026-08-07 → voce **PIE-DEBUG-CELLS**. La roadmap dichiarava l'area «⏳ *verificato assente*»: era **falso**. Restano `DrawGrid`, `DrawPaths`, `DrawCover`, `DrawIntent`, `DrawResolution`, `DumpSnapshot`, `DumpTurnLog`, `VerifyReplay` (CP 11.4) |

### Strumenti di leggibilità (aggiunti il 2026-08-07)

> Due strumenti che **non esistevano** quando le voci sopra sono state scritte, e che ne rendono verificabili
> diverse: senza di essi la mappa non comunica le proprie regole e un playtest non può distinguere un difetto
> del gioco da una regola non mostrata. Precondizione comune: partita hex avviata.

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-DEBUG-CELLS** | Overlay di leggibilità della mappa | partita avviata, console `rt.Debug.DrawCells 1` | Ogni cella mostra un contorno del **colore della superficie** (fango marrone, acqua blu, ghiaccio azzurro, fumo grigio-bluastro); **rosso interno** dove blocca il movimento, **giallo interno** dove blocca la vista. `rt.Debug.DrawCells 0` spegne tutto; senza argomento fa da interruttore | ✅ **2026-08-07** — verificato dall'utente su `GeneratedTestArena`. Giallo e rosso comparivano già; il **fango no**: il contorno superficie era disegnato a `z=2.0` mentre la faccia del disco-cella sta a `2.5` (cilindro engine, mezza-altezza 50 uu × `FlatScale` 0.05) e restava **sepolto nella mesh**. Corretto in `069b616`: le quote derivano ora da `RTCellTopZ` e sommano `Cell.Height` |
| **PIE-PREVIEW-AREA** `RELEASE-V01` | Anteprima dell'area d'attacco e del fuoco amico | unità selezionata, un'abilità ad **area** o **cono** pianificata su un bersaglio | Le celle colpite si evidenziano in **rosso**; se un **alleato** è dentro l'area la sua cella è **arancione** — e si vede **prima** del lock-in. Le celle raggiungibili col budget corrente sono in **verde tenue** (il fango accorcia il raggio a vista d'occhio). Premendo Spazio l'anteprima **sparisce** | ✅ **2026-08-09** — verificato dall'utente: «vedo zone arancioni», e alla domanda di giudizio della voce — capisci di stare per colpire un alleato senza doverci ragionare? — «**sì, si capisce**». È la risposta che chiude questa voce: non «si vede», ma «si capisce». **Tre difetti** per arrivarci, nessuno visibile a un test automatico. **(a)** L'arancione era disegnato con `DepthPriority=SDPG_World` a 2,5 unità dal piano della cella: il cilindro lo copriva, e siccome l'arancione esiste **solo** su celle occupate da un alleato era l'unico contorno sistematicamente nascosto. **(b)** Si vedeva ma non rispondeva alla domanda: l'anteprima parla di **celle**, chi guarda chiede «questo cilindro lo prendo?» — aggiunti il nome marcato sopra la testa (`! Riva` arancione) e la riga `TIRO: N celle - M ALLEATO NELLA ZONA`. **(c)** Non compariva affatto: la sessione «selezionava» con `HandleClickOnUnit`, che **non seleziona** (presuppone una selezione e tratta l'argomento come bersaglio), quindi usciva in silenzio mentre il log dichiarava successo — estratta `SelectUnit`. Ricetta ripetibile: `Scenario To Run = Combat.FriendlyFire`, `Scenario Turn Pause Seconds = 30`, Play: Flux si seleziona da solo con `Overload` su Bastion. ⚠️ Difetto residuo tracciato come `PIE-PREVIEW-PERSIST` |

### Scenario Test Harness (aggiunte il 2026-08-07)

> L'harness esegue scenari `.json` attraverso il **percorso di gioco reale** ed è **interamente coperto
> headless** (30 test `RefactorTactics.Scenario.*` + `RefactorTactics.ScenarioIndex.*`, misurati il
> 2026-08-08). Restano cinque cose che nessun test automatico può vedere, perché riguardano ciò che accade
> **a schermo** e ciò che l'utente **non deve** dover fare.
> Guida d'uso: [`test-e-diagnosi.md`](test-e-diagnosi.md) · modello di classificazione:
> [`scenario-index-e-tag.md`](scenario-index-e-tag.md).

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-TEST-AUTORUN** | «Premo Play e parte da solo» | **`BP_GameMode` → Class Defaults → *RefactorTactics\|Test* → `ScenarioToRun` = `Movement.Basic` (**menu a tendina**, non testo libero)**, Compile + Save, poi **Play**. *(In alternativa, per una volta sola: console `rt.Test.Scenario Movement.Basic`, che prevale sulla proprietà.)* | Compaiono **due unità** (Flux team 0, Bastion team 1) su un'arena esagonale, **senza toccare mouse o tastiera**. Nell'Output Log: `[RT-Test] AUTO-RUN Movement.Basic -> PASS (2/2 assertion, 1 turni) · report: …`. La partita normale **non** viene allestita (nessun timer di pianificazione, nessun bot che gioca). Rimettendo `rt.Test.Scenario` a vuoto e ripremendo Play, torna la partita normale | ✅ **2026-08-08** — verificato dall'utente in PIE. Log: `[RT-Test] AUTO-RUN Movement.Basic -> PASS (2/2 assertion, 1 turni) · report: Saved/RTTests/Movement.Basic/20260808-075237`. È il requisito centrale del documento di specifica («devo poter premere Play e osservare») e l'unico pezzo che l'automazione non poteva coprire, perché headless non esiste un mondo di gioco |
| **PIE-TEST-VISUAL** | Il movimento si **vede**, non si teletrasporta | come sopra, dopo l'AUTO-RUN | L'unità `A1` si sposta da `(-2,0,0)` a `(-1,0,0)` **scorrendo** lungo il percorso, non saltando. A fine turno è esattamente sul centro della cella. ⚠️ Se invece appare già arrivata, il runner sta risolvendo più rapidamente di quanto il playback riesca a mostrare: **non è un difetto di simulazione** (i test headless verificano già l'esito) ma di presentazione, e va annotato qui — sarà la modalità *Visual* del harness a doverlo gestire | ✅ **2026-08-08** — verificato dall'utente dopo il **runner latente** (`8c8365e`). I timestamp lo provano: `AUTO-RUN 09:15:44.830` → `FINITO 09:15:48.772`, **3,9 secondi**, con i turni a `:44`, `:46`, `:48`. Nessun turno dopo il `FINITO`. **Storia**: il primo tentativo diede un falso positivo — il movimento visto erano **turni fantasma** (piani appesi + timer che li ririsolveva, `4e6c2e0`); lo scenario vero si risolveva dentro `BeginPlay` e finiva prima del primo fotogramma. La sessione ora avanza **un passo per frame**, e la stessa macchina a stati serve headless e in gioco |
| **PIE-PREVIEW-PERSIST** | L'avviso di fuoco amico **sopravvive** al cambio di selezione | come `PIE-PREVIEW-AREA`, con l'attacco di Flux gia' pianificato su Bastion | Selezionando un'ALTRA unita' (es. Riva, per muoverla) il marcatore `! Riva` sopra la testa e la zona arancione **restano visibili**: il piano di Flux esiste ancora, e l'avviso serve fino al lock-in | ⏳ **difetto noto, segnalato il 2026-08-09**: oggi spariscono. Il marcatore legge `IsPreviewAllyHitCell`, cioe' l'anteprima dell'unita' **selezionata**, non i piani; cambiando selezione l'anteprima si ricalcola per la nuova unita' e il fuoco amico dell'altra svanisce. Va contro l'intenzione dichiarata nel codice — «un alleato dentro l'area va visto PRIMA del lock-in, non dedotto dai danni dopo» — perche' svanisce **proprio mentre** si finisce il turno. Correzione probabile: calcolare il marcatore dai PIANI delle proprie unita' (`PlannedAttackTarget` + `HexHitCells`) invece che dallo stato dell'anteprima, lasciando la zona a terra legata alla selezione |
| **PIE-TEST-CONSOLE** | I comandi rispondono durante una partita | partita avviata (anche normale) | `rt.Test.List` elenca **8** scenari (i 6 `Movement.*` e i 2 `Combat.*`); `rt.Test.Run Movement.BasicFailsOnPurpose` stampa `FAIL` **con atteso e ottenuto** e il percorso del report; `rt.Test.DumpResult` ristampa l'ultimo `result.json`. ⚠️ Attenzione: eseguire uno scenario **sostituisce la mappa** e aggiunge unità alla partita in corso — è previsto (il runner riusa mappa e turn manager), ma dopo conviene riavviare con `R` | 🟡 **2026-08-08 — metà eseguita.** `rt.Test.List` ha risposto a schermo: l'utente ha visto i **9** scenari (i 6 `Movement.*`, i 2 `Combat.*` e `RT_Showcase_Relay_v01`). Il `FAIL` di `rt.Test.Run Movement.BasicFailsOnPurpose` è stato **prodotto** — `FAIL (0/1 assertion, 1 turni)`, `FALLITA UnitAtCell(A1): atteso (q=3,r=0,L=0), ottenuto (q=-2,r=0,L=0)`, più il percorso del report — ma **l'utente non l'ha visto a schermo**: l'overlay della console in PIE mostra poche righe e scorre via. ⚠️ **Questa voce chiede «i comandi rispondono», e una risposta che non si legge non risponde**: era stata segnata ✅ sulla prova nei log, ed è stata corretta a 🟡 su obiezione dell'utente — la prova nei log dice che il *codice* funziona, che è ciò che i test headless già coprono (`Scenario.RunnerDiagnosesFailure`, `ReportIsSelfSufficient`). Per chiudere serve che l'esito sia **leggibile senza aprire l'Output Log**: o si porta a schermo una riga di sintesi, o si dichiara qui che il medium legittimo è l'Output Log e la voce cambia richiesta. Il conteggio in questa riga era «4», fermo a prima degli scenari `Combat.*` |
| **PIE-SCEN-FILTER** | I filtri restringono la tendina, **e la tendina si aggiorna** | `BP_GameMode` → Class Defaults → *RefactorTactics\|Test* | Con i filtri vuoti, `Scenario To Run` elenca tutti e 8 gli scenari. Impostando `Scenario Filter A = movement` la tendina scende a **6**; aggiungendo `Scenario Filter B = core` scende a **3** (`Movement.Basic`, `Movement.Collision`, `Movement.SwapRejectedByPlanning`). Il vocabolario dei due filtri contiene solo tag esistenti, con la voce vuota in testa | ✅ **2026-08-08** — verificato dall'utente in Editor: «la selezione filtra i possibili scenario to run». **Il rischio tecnico non si è materializzato**: `GetOptions` rivaluta l'elenco al cambio di un'altra property dello stesso actor, quindi il Details Panel ridisegna la tendina da solo. Nessuna `PostEditChangeProperty`, nessuna dipendenza `PropertyEditor` |
| **PIE-SCEN-KEEP** | Filtrare **non perde** lo scenario selezionato | come sopra, con `Scenario To Run = Combat.BasicAttack` già salvato | Impostando `Scenario Filter A = movement` — che esclude `Combat.BasicAttack` dalla vista — la property **resta** su `Combat.BasicAttack`, e premendo Play parte quello. Il filtro è una vista, non un vincolo | ✅ **2026-08-08** — verificato dall'utente in Editor: «se cambio scelta, lo scenario to run non si resetta». Il combo box tiene il valore corrente anche quando i filtri lo escludono dalla vista, quindi la configurazione salvata nel `.uasset` non sembra né è andata persa. ⚠️ **Al termine svuota `Scenario To Run`** (prima voce della tendina) e Save: la property sopravvive alla sessione, e al Play successivo il GameMode esegue lo scenario e **non allestisce la partita normale** (`RTGameMode.cpp:136` fa `return`). Sintomo: schermo quasi nero, nessuna unità tua, **nessuna barra abilità** — perché la barra si disegna solo con un'unità selezionata. Diagnosi in un colpo: cerca `AUTO-RUN` nell'Output Log, la riga dice anche **da dove** viene lo scenario |

### Verifiche di mutazione — rimaste fuori dall'headless per contesa sul binario

> Queste voci **sarebbero** automatizzabili: non chiedono l'editor, il mouse o un asset. Stanno qui perché
> richiedono una condizione che nessuno script può garantirsi da solo — che **nessun altro processo UE tenga
> la DLL** — e quindi serve una persona che scelga il momento. Non sono verifiche visive: si eseguono da riga
> di comando, e l'esito è un test rosso.
>
> **Perché esistono.** Un assert nuovo che nasce verde non ha ancora dimostrato di saper diventare rosso.
> La regola del progetto è rompere il codice **una mutazione per volta** e controllare che cadano
> *esattamente* i test attesi. La trappola è nota e documentata: una mutazione che non fa cadere niente può
> voler dire «assert vacuo», ma anche **«la build non è arrivata al binario»** — e i due casi si distinguono
> solo confrontando il timestamp della DLL con quello del sorgente.

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-V01-ARENA** | La v0.1 gira su una **mappa d'autore**, non sull'arena generata | Seduta **U1** di [`../roadmap/roadmap-editor.md`](../roadmap/roadmap-editor.md): produce `DA_HexMap_Arena` + `L_HexArena`. Sbloccata da CP 6.0 ✅ | Disegnare l'arena con gli strumenti hex. **Il Blueprint non va piu' toccato**: dal 2026-08-10 `rt.Map.Source` scavalca la proprieta' da riga di comando, quindi la verifica si fa con `RefactorTactics.exe -dpcvars=rt.Map.Source=LevelAsset` (⚠️ `-dpcvars`, **non** `-ExecCmds`, che gira troppo tardi e fallisce in silenzio). Resta una scelta di progetto se cambiare anche il default del Blueprint. Verifica: il pacchettizzato deve loggare la mappa d'autore invece di `MapSource=GeneratedTestArena: uso la mappa di PROVA` | ⏳ **aperta il 2026-08-10** — e' l'**ultimo** ostacolo a CP 12.5, che per il resto e' verificato: Development e Shipping pacchettizzano, e una partita gira dall'avvio alla fine sul formato `Format.Skirmish2v2`. Non e' automatizzabile: disegnare una mappa richiede l'editor e il giudizio di una persona |
| **PIE-MUT-BASTION-SLOW** | L'assert su `Status.Slow` di `Bastion.ImpactShot` **sa diventare rosso** | Nessun `UnrealEditor`/`UnrealEditor-Cmd` in esecuzione (`Get-Process UnrealEditor-Cmd`): finché uno tiene `Binaries/Win64/UnrealEditor-RefactorTactics.dll`, il link fallisce con `LNK1104` e il test gira contro il binario vecchio. ⚠️ **Il processo che blocca non è l'Editor**: è il runner headless delle automation, visibile solo in Task Manager — «ho chiuso l'editor» non basta | Togliere la riga `FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Slow, 1)` da `RTHeroCatalogLibrary.cpp` (indice 0 di Bastion) · ricostruire · **verificare che `LastWriteTime` della DLL sia più recente del sorgente** — è questo il passo che rende la prova valida, non la scritta `Result: Succeeded` · eseguire `Automation RunTests RefactorTactics.Heroes`. Deve cadere **esattamente** `RefactorTactics.Heroes.Bastion.MatchesCatalog`, sull'assert «ImpactShot: applica Status.Slow», e **nient'altro**. Poi ripristinare la riga e ricostruire | ✅ **2026-08-09** — eseguita. DLL `20:35:57` contro sorgente `20:35:38` (mutazione **nel binario**, non solo su disco), poi `29 tests performed` con **un solo** rosso: `Heroes.Bastion.MatchesCatalog`, `Expected 'ImpactShot: applica Status.Slow' to be true`. Nient'altro è caduto: l'assert non è vacuo. Ripristinata e ricostruita, albero identico al commit. **Storia utile a chi ripete la prova**: due tentativi precedenti erano risultati «verdi» ed erano falsi negativi — nel primo la DLL era più vecchia del sorgente (`LNK1104`, file tenuto da un runner headless), nel secondo l'automation non partiva affatto (vedi la nota sotto) |
| **PIE-MUT-ACTIONS-ZERO** | La convenzione `Actions[0] = attacco base` **sa diventare rossa** | Come sopra: nessun processo UE che tenga la DLL, e runner lanciato **in diretta** | Aggiungere `Bastion->Actions.Swap(0, 3);` in coda a `MakeBastion` — cosi' l'indice 0 non e' piu' l'attacco base — - ricostruire, **verificando il timestamp della DLL** - `Automation RunTests RefactorTactics.Heroes`. Deve cadere `RefactorTactics.Heroes.BasicAttackIsIndexZeroForEveryHero`. ⚠️ **Cadono anche altri due test** (`Bastion.MatchesCatalog`, `BastionInterpositionUsesReactionSlot`) ed e' **corretto**: uno scambio di indici rompe tutto cio' che dipende dalla posizione, non solo la convenzione. Non e' rumore da sopprimere: e' la misura di quanto il roster dipenda da quegli indici — cioe' l'argomento a favore di un campo di ruolo, il giorno in cui un consumer runtime lo giustifichera' | ✅ **2026-08-09** — eseguita. DLL `21:58:37` contro sorgente `21:58:07`; `30 tests performed`, 3 rossi fra cui quello atteso, con `Expected 'ImpactShot: 8 danni' to be 8, but it was 20`. Ripristinata: sorgente identico a `origin/main` |

> ⚠️ **Come lanciare il runner, e perché conta.** Un processo avviato con `Start-Process` (staccato, senza
> console) viene **throttlato da UE come applicazione in background**: il motore gira a ~0,6 fps invece di
> ~113, e l'automation controller non arriva mai a completare la discovery dei test — resta fermo dopo
> `Ready to start automation`, senza errori, per un tempo indefinito. Sembra un blocco del message bus e non
> lo è. Il sintomo si legge nel **contatore di frame** del log (`[2026.08.09-18.12.29:110][ 44]`): se avanza
> di poche unità al secondo, è quello.
>
> Lanciare il runner **in diretta** (`& "…/UnrealEditor-Cmd.exe" …`), non con `Start-Process`.

### Durata, ritmo e scala — verifiche di *game feel*

> Voci aperte il **2026-08-07** con [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md).
> Sono l'unica parte di quella spec che un test automatico **non può** chiudere: i numeri sono target di
> playtest, e ciò che si valuta è se il ritmo *si sente* giusto. Nessuna di queste voci è un gate della v0.1
> (§3 della DoD): servono a **produrre i numeri**, non a superarli.
>
> ⚠️ Il formato a cui puntano i target (**3v3 Standard**) **non esiste in v0.1**. Qui si misura il **2v2** e lo
> si registra come 2v2 — non si dichiara «fuori target» un numero confrontato con la banda di un altro formato.

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-V01-MATCHLEN** | Durata della partita e numero di round | partita 2v2 completa giocata da un umano contro i bot, cronometro alla mano (o `rt.Debug.Pacing` per i tempi di turno) | La partita chiude entro la banda **10–14 round** del formato 2v2 e **il ritmo non annoia**: nessuna sequenza di round in cui non succede nulla di significativo. Si annota: round giocati, durata a cronometro, round del **primo contatto** col nemico, e se la fine è arrivata per eliminazione o per limite. **Un numero fuori banda si registra, non si nasconde** | ⏳ |
| **PIE-V01-MATCHEND** `RELEASE-V01` | Fine partita a tre vie, a schermo | partita 2v2 avviata; per la via «obiettivo» serve CP 10.2, per il formato d'autore un `URTMatchFormatData` creato in editor sotto `/Game/RT/Match/` e assegnato al `RTGameMode` | A partita conclusa l'HUD mostra **esito e via** («Vince il team 0 (blu) — per eliminazione» / «— allo scadere dei round» / «— per obiettivo»), il contatore in alto dice **`Turno n/RoundLimit`**, e **`R` riavvia in tutte e tre le vie**. Senza asset assegnato l'Output Log deve dire a chiare lettere che è in uso il **formato di ripiego** `Format.Fallback`: se il ripiego non si nota, i numeri di `PIE-V01-MATCHLEN` finiscono attribuiti a un formato che non era in vigore | ⏳ **eseguibile da CP 10.3** (2026-08-07). Logica coperta headless da `Match.TurnLimitEndsAPlayedMatch`, `Match.ObjectiveEndsAPlayedMatch`, `MatchFormat.FallbackIsObservable`; il PIE verifica **ciò che il test non vede**: il testo a schermo e il riavvio |
| **PIE-V01-READY** | Ready anticipato e countdown annullabile | partita avviata | Chiudendo la pianificazione prima dello scadere del timer parte un **countdown di 3 s** prima del commit; premendo **Unready** durante il countdown si torna alla pianificazione **senza aver perso il piano**; il countdown **non** sostituisce il timer massimo. Si annota a che secondo si è dichiarato Ready nei round tipici | ⏳ **il countdown non esiste ancora**: oggi Spazio fa lock-in immediato e irreversibile. La voce documenta il comportamento atteso, non uno da verificare adesso |
| **PIE-V01-OVERWATCH** | Finestra Fast Reaction da 3 s | E14 (CP 14.5/14.6) atterrata; un'unità con Overwatch armato | La finestra `FIRE`/`HOLD` compare **solo** al proprietario (l'alleato la vede in sola lettura, l'avversario **nulla**), dura **3 s** con countdown visibile, e allo scadere applica **HOLD** senza consumare la charge. **Tre secondi bastano** a decidere senza rileggere tutta la situazione? Se serve rileggere, il problema è la finestra, non la durata. La slow-motion è solo presentazione: ripetendo lo stesso turno l'esito non cambia | ⏳ **E14** (CP 14.6) |
| **PIE-V01-MAPSCALE** | Scala della mappa in Move, non in celle | mappa di prova con almeno **due rotte** distinte fra gli spawn | Dallo spawn, **attraversare tutta la mappa** costa un numero di Move coerente con la classe dichiarata (**Skirmish** ~3–4, **Standard** ~5–7); **entro 1–2 round** è possibile contestare una zona rilevante o entrare in contatto; le due rotte offrono un **trade-off reale** (più rapida vs più coperta), non due strade equivalenti | ⏳ dipende dalla mappa di prova (vedi sotto) |

### Bot — leggibilità delle decisioni

> Voci aperte il **2026-08-08** consolidando [`../archive/src/handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md`](../archive/src/handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md) §24.1.
> Il bot ha già copertura headless ampia (utility scoring, copertura, minaccia da dash). Queste voci **non
> ricontrollano la regola**: guardano se un umano che osserva la partita capisce *perché* il bot ha fatto
> quella mossa. Un test può dire che lo score di una cella era il più alto; non può dire che la scelta
> sembrasse sensata a chi guarda.

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-AI-01** | Il bot produce solo Intent legali | scenario base, nessun input umano, bot su entrambi i team | Nessun intent invalido accettato, nessun bypass della validazione. L'Output Log non contiene rifiuti silenziosi: se un intent è scartato, il motivo è scritto | ⏳ — logica coperta headless; il PIE guarda il **log narrato** durante una partita intera |
| **PIE-AI-02** | Objective awareness senza contatto | nessun nemico visibile, un obiettivo disponibile | Il bot avanza o contesta secondo il profilo, invece di restare fermo o di muoversi verso informazione che **non ha** (cercare uno stato nascosto sarebbe barare) | ⏳ |
| **PIE-AI-03** | Lethal contro posizione | bersaglio legalmente eliminabile + alternativa di movimento mediocre | Il bot preferisce il KO, salvo che l'obiettivo domini la scelta. Lo score breakdown a schermo mostra **perché** | ⏳ |
| **PIE-AI-04** | Cover contro esposizione | due celle raggiungibili a costo simile, una con cover valida | Il bot sceglie la cella tatticamente migliore e il breakdown lo spiega con la voce «copertura», non con un totale opaco | ⏳ |
| **PIE-AI-05** | Hazard avoidance | il path più breve attraversa un hazard **noto** al bot | Il bot devia, oppure attraversa **dichiarando** che il costo vale il guadagno. Attraversare senza che il breakdown lo giustifichi è un difetto | ⏳ |

### Formato e icone

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-ICON-01** | Le icone si distinguono a schermo | CP 20.2 atterrato, catalogo popolato per Identity/Action/Phase/Status/Certainty | Ogni icona è riconoscibile alla dimensione reale dell'HUD e **due stati diversi non si confondono**. Nessuna informazione affidata al **solo colore** (§12.4 del sorgente muri/porte, stessa regola per l'HUD). Una chiave non risolta si vede come errore, non come spazio vuoto | ⏳ **E20** (CP 20.3) |
| **PIE-FMT-01** | Formato e mappa concordano | CP 19.1 atterrato | Avviando una mappa `Skirmish` con un formato `Standard` il gioco **rifiuta e lo dice**; con formato e mappa coerenti parte normalmente. Il caso di errore è quello da guardare: se fallisce in silenzio, la classe non sta proteggendo niente | ⏳ **E19** (CP 19.1) |

### Stati del personaggio (E34, post-v0.1)

Tutte ⏳ e **tutte dipendenti da un sistema che non esiste**: nascono qui perché il ciclo
docs → epic → scenario → PIE resti chiuso, non perché siano eseguibili oggi. Epic
[#244](https://github.com/DegrassiAaron/refactor-tactics-main/issues/244) · owner
[`../gameplay/brief-stati-personaggio-e-trasformazioni.md`](../gameplay/brief-stati-personaggio-e-trasformazioni.md).

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-STATE-01** | Lo stato attivo si riconosce senza ricordarlo | CP 34.6 · catalogo icone popolato | Guardando l'unità si capisce **in quale configurazione è**, senza aprire un tooltip. Due stati diversi non si confondono, e nessuna informazione è affidata al solo colore | ⏳ **E34** |
| **PIE-STATE-02** | La transizione è dichiarata in Planning e risolve in `Prep` | CP 34.2 | Lo stato **non** cambia al momento del click: cambia in `Prep`, durante la risoluzione. Se cambia prima, il planning simultaneo sta leakando | ⏳ **E34** |
| **PIE-STATE-03** | Il ghost mostra la configurazione **prevista** | CP 34.5 | Pianificando la transizione, il ghost mostra range, abilità e posizione **dello stato nuovo**, non di quello corrente. Le azioni rese indisponibili si vedono tali **prima** del commit | ⏳ **E34** |
| **PIE-STATE-04** | Il toggle gratuito non esiste | CP 34.2 | Provare `A → B → A` in turni consecutivi: la seconda transizione è rifiutata o costa. Se è gratis, il sistema è micro-ottimizzazione — è il fallimento di design che il brief §5.2 vuole evitare | ⏳ **E34** |
| **PIE-STATE-05** | Una forma a durata fissa scade nel `Cleanup` | CP 34.2 | Lo stato termina nella stessa fase in cui Bastion recupera `Integrità Strutturale`, non a inizio turno successivo | ⏳ **E34** |
| **PIE-STATE-06** | Gli override di abilità si vedono nella barra | CP 34.3 | Entrando in stato, le abilità sostituite cambiano nell'HUD; quelle non disponibili sono disabilitate e **dicono perché** | ⏳ **E34** |
| **PIE-STATE-07** | Il Move resta l'ultima fase volontaria | CP 34.4 | Con uno stato che riduce o disabilita il movimento, l'ordine delle macro-fasi **non cambia**: nessuno stato anticipa il Move o lo sposta prima del Blast | ⏳ **E34** |
| **PIE-STATE-08** | Il trigger ambientale è leggibile | CP 34.7 · scenario `State.Flux.Charged` | Entrando nella cella elettrificata lo stato si attiva e **si vede perché**: la causa ambientale è distinguibile da un'attivazione volontaria | ⏳ **E34** |
| **PIE-STATE-09** | Il default di una Fast Reaction non trasforma | CP 34.8 · **E14** | Lasciando scadere la finestra di 3,0 s, `Timeout → HOLD`: l'unità **non** entra in stato. Il default non deve mai essere la scelta più forte | ⏳ **E34** |
| **PIE-STATE-10** | Il TurnLog spiega la transizione | CP 34.10 | Nel log compaiono stato precedente, stato nuovo, trigger, fase e motivo. Una transizione **rifiutata** compare quanto una riuscita: se il rifiuto è silenzioso, il playtest non è diagnosticabile | ⏳ **E34** |

> Nessuna di queste è un gate della v0.1: sono la copertura di **CP 34.11**, e la loro esistenza serve a
> impedire che la Matrix 5 di [`../characters/matrici-stati-personaggio.md`](../characters/matrici-stati-personaggio.md)
> resti con le colonne di validazione vuote quando gli stati diventeranno `PROTOTYPE`.

## La sequenza: quale voce affrontare, e quando

Questo file e' il **registro**: dice cosa verificare e com'e' andata. **Non** dice in che ordine, con quale
preparazione condivisa, ne' quali asset creare prima — quella e' la
[**EditorMap**](../roadmap/editormap.shortlist.md), generata da
[`editor-sessions.yaml`](../roadmap/editor-sessions.yaml).

> **Le sessioni A-G stavano qui, e sono state spostate** il 2026-08-10 (issue
> [#371](https://github.com/DegrassiAaron/refactor-tactics-main/issues/371)). Raggruppavano le voci per
> **preparazione condivisa** — meta' del lavoro di una seduta — mentre le sedute `U1`-`U17` della ritirata
> `roadmap-editor.md` facevano l'altra meta' in un secondo file. Erano **lo stesso concetto modellato due
> volte**, ed e' il modo in cui la divergenza e' gia' nata una volta. Ora sono un elenco solo, e i loro
> conteggi non sono piu' scritti a mano: la sessione A dichiarava «13 voci» ed erano diciotto.

**Cosa serve da me**: non posso eseguire il PIE, che richiede l'editor interattivo. Posso compilare il target
prima di ogni seduta, prepararti la sequenza esatta dei passi, e **leggere i log** dopo — incolla il percorso
di `Saved/Logs/*.log` oppure dimmi cosa hai osservato, e aggiorno le voci con l'esito (aprendo un fix se
emerge un difetto).

## Note operative sulle singole voci

> **PIE-CP1.4**: codice fatto (`c06ef51`), resta solo la verifica interattiva (evidenziazione cella-cursore).
> Le altre voci hanno il **codice pronto**; manca solo la verifica interattiva (e, per AS.2/AS.4/AS.5, gli asset in editor).
> **Nota (PIE-HEX-MODE-E, undo)**: dopo Commit, verifica che ripetuti Undo/Redo rimuovano/ripristinino la transizione
> senza lasciare gizmo/transform orfani (l'interleaving delle transazioni del gizmo con la `FScopedTransaction` del
> Commit va osservato; l'asset resta integro perché il proxy è `Transient`).

> **PIE-BU2 · tuning pesi**: i pesi dell'utility scoring sono ora `UPROPERTY` sul `TurnManager`
> (categoria *Refactor Tactics ▸ Bot*): `WKill / WDamage / WThreat / WKiteViolation / WApproach`
> (default invariati = comportamento BU.2). Il `TurnManager` è **spawnato a runtime** dal `RTGameMode`:
> per calibrare **durante il PIE**, selezionalo nel **World Outliner** e modifica i pesi nel **Details** →
> hanno effetto **dal turno successivo** (`PlanBots` li rilegge ad ogni pianificazione), **senza ricompilare**.
> In alternativa, piazza un `RT Turn Manager` nel livello e imposta i pesi sull'istanza (il GameMode riusa
> quello esistente invece di spawnarne uno nuovo).

> **Playtest 2026-08-04** (`Saved/Logs/RefactorTactics_2.log`, partita T1→T6, **vince il team 1/bot**):
> **PIE-BU2/BU3/BU3c ✅**. Dash+attacco a segno — T1 `RTUnit_2: scatto (4,6)+attacca RTUnit_1`,
> T4 `RTUnit_3: scatto (4,3)+attacca RTUnit_0`, T5 `RTUnit_2: scatto (3,6)+attacca RTUnit_0`; nessun
> `nessuna linea di tiro`. Resta+attacca e posizionamento (`score=-140 (resta)`) osservati.
> **Non ancora esercitati** (restano da verificare): **PIE-BU2b** (tuning pesi non modificato in partita),
> il fattore **quota** (partita interamente a Layer 0, ponte non usato), e le guardie **panic/support**.

> **Playtest 2026-08-04 #2** (tuning + panic): con `WThreat=100` il Guardian resta (`(6,4) score=-140 (resta)`);
> con `WThreat=18` avanza e ingaggia (`(6,5) score=-48` → `scatto (6,6) + attacca score=344`) → **PIE-BU2b ✅**.
> Osservato anche il **panic** del kiter (`RTUnit_2: scatto difensivo (schiva)`) → **panic ✅**. Resta solo il
> **support** (Barriera del Guardian, non ancora emerso).

> **Automatizzati** (2026-08-04): **tuning** (WThreat), **panic** e **support** del bot sono ora coperti da
> **test d'integrazione headless** — `Source/RefactorTactics/Tests/RTBotPlanningTests.cpp` (smoke + panic +
> support + tuning): costruiscono un mondo 2v2, invocano `PlanBotsForTest()` e verificano le decisioni via i
> campi `Planned*`, **senza PIE**. Quindi queste tre guardie **non richiedono più verifica manuale**.
> *(Il dash-avvicinamento ora è pesato da `WThreat`: il bot rinuncia allo scatto se la cella è troppo esposta — test `PlanningDashRespectsThreat`.)*

> **PIE-PACING-1 — il cablaggio degli input** ⏳: in PIE, con `bRecordPacing = true` sul `RT Turn Manager`,
> giocare un turno selezionando due volte un'unità, impartendo un ordine, annullando un waypoint e chiudendo
> con Spazio. Poi `rt.Debug.Pacing`: il sommario deve riportare **1 turno**, `SelectionCount` ≥ 2,
> `OrderCount` ≥ 1, `UndoCount` = 1 e **nessun taglio** (il lock-in è stato manuale). Verificare che
> `Saved/RT/pacing_*.csv` esista, abbia l'intestazione e **una riga per turno giocato**.
> *(I contatori in sé sono già coperti headless da `RefactorTactics.Pacing.RecordsDecisionComposition`:
> qui si verifica solo che il controller li alimenti davvero.)*

> **PIE-V01-ROSTER — il roster a quattro eroi in partita** ⏳: in PIE su una mappa con almeno quattro celle
> percorribili, verificare che entrino in campo **quattro unità distinte** e non più due Ranger e due Guardian.
> Nel log di avvio deve comparire `[RT] Board 2v2 esagonale avviata su N celle con 4 eroi`.
> Attese: **team 0** = Flux (90 HP, 5 MP) + Riva (95 HP, 5 MP); **team 1** = Bastion (120 HP, **4 MP**) +
> Vektor (100 HP, **6 MP**). Selezionando le unità del giocatore, il **raggio di movimento mostrato deve
> differire** fra eroi con MP diversi — è la parte che i test headless non possono mostrare (verificano i dati
> sull'Actor, non l'anteprima a schermo).
> *(Le statistiche in sé sono già coperte da `RefactorTactics.Heroes.SpawnFromData`, che invoca
> `ARTGameMode::SetupHexMatch` in un `UWorld` vero: qui si verifica solo ciò che si vede — mesh, anteprima
> del movimento, e che il fallback al cilindro non sia regredito quando `HeroUnitClasses` è vuota.)*

## Scenari di validazione visiva — corpus `Visual.*`

> Aggiunte il **2026-08-08** con il corpus omonimo. Owner del catalogo:
> [`scenari-validazione-visiva.md`](scenari-validazione-visiva.md).
>
> **Precondizione comune a tutte**: `Scenario Filter A = animation` nel `BP_GameMode`, poi
> `Scenario To Run = <id>` e Play — oppure `rt.Test.Scenario <Id>`. Nessun'altra preparazione: gli scenari
> portano con sé arena, unità e piani.
>
> **Queste voci non verificano che lo scenario passi.** Quella parte la dicono già le assertion, headless e
> senza aprire l'editor: se la logica devia, lo scenario è rosso prima che qualcuno prema Play. Qui si
> verifica **ciò che il test non può vedere** — che l'effetto sia leggibile, distinguibile da quelli vicini, e
> che non suggerisca una regola diversa da quella che il resolver ha applicato.
>
> ⚠️ Tre di queste hanno un numero che il **primo run** deve confermare (`CHARGE` le celle finali, `COMBO`
> il bonus Wet, `COVER` l'entità della riduzione). Se una esce rossa alla prima esecuzione, sospettare lo
> **scenario** prima del gioco: sono valori derivati dal catalogo, non misurati.

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-VIS-FIRE** | Danno da terreno in due momenti | `Visual.Environment.FireOnEnter` | All'ingresso in `(0,-2)` una reazione immediata (10 danni), e nel **Cleanup** una seconda, distinta, per `Burning` (8). Se le due usano la stessa animazione, una regola sparisce | ⏳ |
| **PIE-VIS-ICE** | La scivolata non è un passo | `Visual.Environment.IceSlide` | Flux fa **due** passi voluti e **uno** subìto: il terzo, fino a `(-2,1)`, deve leggersi come perdita di controllo — non come un passo identico agli altri | ⏳ |
| **PIE-VIS-WETFIRE** | L'acqua spegne le fiamme | `Visual.Environment.WetExtinguishesFire` | Al turno 2 il getto di Riva **spegne visibilmente** il fuoco addosso a Vektor. È un'assenza (i danni del Cleanup non arrivano): se le fiamme restano accese, il giocatore non sa che la regola è scattata | ⏳ |
| **PIE-VIS-KO** | Eliminazione di un'unità | `Visual.Combat.Defeat` | Due colpi per turno, barra che scende due volte, e al **quarto** turno la rimozione. L'unità non deve sparire prima che il colpo sia arrivato. ⚠️ Erano **due** turni fino ad [ADR-0007](../decisions/adr-0007-attacco-base-per-eroe.md) (2026-08-09): `Bastion.ImpactShot` è passato da 24 a 8, la somma per turno da 45 a 29, e i 90 HP di Flux non finiscono più in due. La seduta dura quindi il doppio, ed è la stessa cosa da guardare | ⏳ |
| **PIE-VIS-CHARGE** | La carica si distingue dal passo | `Visual.Movement.Charge` | Accelerazione, impatto e arresto **addosso** a Flux, più la spinta di 1. Confronto diretto con `Movement.LongWalk`: se si vedono uguali, la differenza fra Dash e Move non arriva | ⏳ |
| **PIE-VIS-ROUGH** | Un rifiuto è muto | `Visual.Movement.RoughRefusesCharge` | La carica **non parte**: nessuno slancio accennato, nessun impatto mancato, nessuno si muove. Un rifiuto che sembra un'animazione interrotta suggerisce un bug dove c'è una regola | ⏳ |
| **PIE-VIS-COMBO** | Acqua + elettricità dal **terreno** | `Visual.Combat.WaterElectric` | Vektor attraversa l'acqua in fase Dash e incassa la scarica potenziata. Si deve capire che il bonus viene dal **terreno bagnato**, non da chi ha bagnato (D-029): nessuna enfasi su un'unità che non c'è | ⏳ |
| **PIE-VIS-COORD** | Acqua + elettricità, e l'ordine fra due eroi | `Visual.Combat.WaterElectricCoordinated` | Dentro lo stesso Blast: prima il getto di Riva (prio 50), **poi** la scarica di Flux potenziata (prio 55). Si deve capire che il secondo colpo è più forte **perché** il primo è arrivato prima — se i due colpi sembrano simultanei, la regola di [D-036](../decisions/RT_PDR_00_Decision_Log.md) resta invisibile e la combo firma si legge come un caso fortunato | ⏳ |
| **PIE-VIS-PUSH** | Spinta assorbita vs subìta | `Visual.Combat.PushResistance` | Bastion incassa e **non arretra**, Vektor arretra. Se l'impatto su Bastion mostra comunque un contraccolpo, si legge una spinta riuscita dove è stata assorbita | ⏳ |
| **PIE-VIS-FALLBACK** | Il piano viene rivalidato | `Visual.Combat.FallbackTargetMoved` | Bastion lascia la cella nel Dash e la scarica di Flux arriva **sulla cella vuota**. Deve leggersi come un piano che ha trovato il bersaglio altrove, non come un colpo a caso | ⏳ |
| **PIE-VIS-SMOKE** | Il fumo accorcia, non acceca | `Visual.Combat.SmokeCapsTargeting` | Bastion **si vede** e non si può colpire. Diverso da `Combat.BlockedByWall`, dove la vista è negata: se il fumo si comporta come un muro, si impara la regola sbagliata | ⏳ |
| **PIE-VIS-PHASES** | L'ordine delle fasi | `Visual.Core.PhaseOrder` | Tre azioni in tre momenti **separati e riconoscibili**: carica (Dash), colpo (Blast), camminata (Move). Se accadono insieme, la spina dorsale del turno resta invisibile | ⏳ |
| **PIE-VIS-LEVEL** | Salita di livello | `Visual.Map.MultiLevel` | Flux sale sulla piattaforma attraverso l'unica transizione. Si deve capire **dove è finito**: è il banco di prova della camera su due layer | ⏳ |
| **PIE-VIS-COVER** | La copertura è di un bordo | `Visual.Map.LowCoverEdge` | Due colpi simultanei sullo stesso bersaglio con **entità diverse**, e il bordo riparato distinguibile dagli altri cinque **prima** di sparare | ⏳ |
| **PIE-VIS-DOOR** | La porta è di un bordo | `Visual.Map.ClosedDoor` | Riva arriva a `(1,1)` **girando**. Il percorso deve raccontare da sé perché è lungo: se la porta chiusa non si vede, il giro sembra un difetto del pathfinding | ⏳ |
| **PIE-VIS-HIGH** | L'altura non dà bonus | `Visual.Map.HighGroundNoBonus` | Due colpi **identici** dalla cresta e dal piano. La presentazione non deve enfatizzare il tiro dall'alto: suggerirebbe un vantaggio numerico che in v0.1 non esiste (D-024) | ⏳ |
| **PIE-VIS-HIGHCOVER** | La barriera alta nega, non riduce | `Visual.Map.HighCoverBlocks` (fixture `CoverYard`) | Il colpo **non parte** e il percorso **gira**: due negazioni dallo stesso bordo. La barriera dev'essere distinguibile a colpo d'occhio dalla copertura **bassa** una riga più sotto — se si somigliano, il giocatore prova a sparare attraverso un muro | ⏳ |
| **PIE-VIS-INTERPOSE** | Il colpo cambia destinatario | `Visual.Reaction.Interposition` | Il proiettile parte verso Vektor e finisce su Bastion. **Entrambe le metà devono vedersi**: se si vede solo l'arrivo, la scena si legge come «Flux ha sbagliato mira» invece che come una reazione | ⏳ |
| **PIE-VIS-DEFLECT** | La parata riduce | `Visual.Reaction.Deflection` | 22 diventano 2. Se la parata non si vede, si legge un attacco debole invece di una difesa riuscita — e la prossima volta il giocatore non arma la reazione | ⏳ |
| **PIE-VIS-GUARD** | `Guard` protegge **una volta** | `Visual.Combat.GuardReducesFirstHit` | Bastion 120 → 92: il **primo** colpo è ridotto di 15, il secondo arriva pieno. La differenza fra i due impatti deve vedersi *nel momento in cui la guardia finisce* — se i due colpi si somigliano, il giocatore impara che `Guard` vale per tutto il turno | ⏳ |
| **PIE-VIS-BRACE** | `Brace` protegge **sempre**, ma meno | `Visual.Combat.BraceReducesEveryHit` | Bastion 120 → 97: **ogni** colpo perde 10 e la riduzione non finisce mai. Confronto diretto con `PIE-VIS-GUARD`: le due grammatiche difensive devono leggersi **diverse** — una si consuma, l'altra dura. Se la presentazione è la stessa, la scelta fra le due diventa arbitraria | ⏳ |
