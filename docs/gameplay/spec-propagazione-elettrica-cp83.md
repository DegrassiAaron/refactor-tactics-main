# Spec — Propagazione elettrica (E8, CP 8.3)

> 📌 **Stato di implementazione storico al 2026-08-07 (CP 8.3).** Numeri, conteggi di test ed esiti qui sotto fotografano
> la chiusura del checkpoint. Lo **stato corrente** è posseduto da
> [`../roadmap/roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md): questa spec non compete con la roadmap come
> fonte di stato.

> **Issue**: `#66` · **Epic**: `#22` (E8) · **Dipende da**: `#65` (CP 8.2, chiusa) · **Data**: 2026-08-07
> **Branch**: `feat/66-propagazione-elettrica` (worktree isolato) · **Baseline misurata**: 352 test in 52 file
> Fonti: [`RT_TerrainCatalog_v0.1.md`](../balance/RT_TerrainCatalog_v0.1.md) §2 ·
> [`RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) §6 · [`spec-terreni-e8.md`](spec-terreni-e8.md) ·
> [`spec-stati-temporanei-cp82.md`](spec-stati-temporanei-cp82.md) §4 (ordine del Cleanup)

## 1. Obiettivo

La combo firma del gioco — **acqua + elettricità** — con un limite e un ordine espliciti. «Propagazione senza
limite» è un errore **dichiarato** dal catalogo §17: su una mappa d'acqua colpirebbe tutti e renderebbe il
turno impredicibile, cioè l'opposto del pilastro di prodotto «predizione, non RNG opaco».

## 2. Stato verificato (2026-08-07, sul codice)

| Pezzo | Stato prima di CP 8.3 | Nota |
|---|---|---|
| `bConductsElectricity` sul terreno | ✅ esiste, vero per `ShallowWater` e `Conductive` | nessun lettore |
| `PropagationLimit` su `FRTActionDef` | ✅ esiste, validator rifiuta i negativi | **nessuna azione lo valorizzava** |
| `Action.Electrify` | ❌ assente dal catalogo core **al momento del CP** | compariva solo in `RTCatalogTests.cpp:157` come azione sintetica · ✅ **oggi esiste** in `RTCatalogLibrary.cpp` come azione di fase `Environment`; **nessun eroe la usa** (verificato 2026-08-08) |
| `Status.Electrified` | 🟡 tag definito | nessun consumatore |

Il difetto era quindi *strutturale*: la regola esisteva nei documenti e i suoi ingredienti nel codice, ma
**nessuna azione poteva innescarla**. Costruire la propagazione senza il suo innesco avrebbe prodotto il
quinto «motore che nessuno consuma» dopo E5 e i quattro casi di CP 8.2.

## 3. Decisioni

### D1 — Il percorso è il grafo dell'acqua, non un raggio *(chiude un'ambiguità della DoD)*

La DoD diceva «massimo 3 celle» senza dire *3 misurate come*. Su esagoni, distanza esagonale e passi lungo la
pozza divergono appena l'acqua gira attorno a un ostacolo: una cella a distanza 2 può essere a 4 passi d'acqua.

**Decisione**: **visita in ampiezza (BFS) sulle celle che dichiarano `bConductsElectricity`**, al massimo
`PropagationLimit` passi. La distanza esagonale non entra nel calcolo.

*Perché*: «l'elettricità segue l'acqua» è una regola che il giocatore può prevedere **guardando la mappa**;
«l'elettricità colpisce tutto entro 3 celle purché ci sia acqua da qualche parte» no. La leggibilità tattica è
un pilastro di prodotto, e qui costava una scelta di algoritmo, non una feature.

### D2 — La conduzione è della CELLA, mai dello stato dell'unità

Il modello ha due portatori possibili: la cella (`bConductsElectricity`) e l'unità (`Status.Wet`, che
`Hero.Phase.PressureJet` applica anche all'asciutto). **La propagazione guarda solo le celle.**

*Perché*: due modelli di conduzione paralleli sarebbero la duplicazione che il canone vieta, e renderebbero la
regola impredicibile (chi è bagnato non si vede sulla mappa quanto l'acqua). `Status.Wet` resta ciò che è già:
il moltiplicatore di `Hero.Gadget.LinearDischarge` (+8), verificato da `Heroes.Hero.Gadget.WetBonus`.

Verificato da un test (`StopsAtNonConductive`): un'unità sull'asciutto **non è un ponte**, nemmeno se adiacente.

### D3 — La sorgente è la cella del bersaglio *(limite dichiarato)*

`Action.Electrify` colpisce un'unità; la scarica entra nel terreno **sotto di lei** e da lì si propaga.

*Limite*: il catalogo prevede anche «colpisce una cella conduttiva» senza unità sopra. La pianificazione non
ha ancora un bersaglio-cella per le azioni (`ARTUnit::PlannedAttackTarget` è un'unità, `PlannedDashCell` serve
allo scatto): arriverà col targeting per cella dell'HUD (**E11**). Il caso tattico utile — colpire un nemico
nell'acqua — è coperto.

### D4 — Ordine nel Cleanup: la scarica precede `Burning` *(motivata)*

CP 8.2 ha fissato il Cleanup in cinque passi. La propagazione entra come **passo 0**, prima del danno di
`Status.Burning`.

*Perché è una decisione e non un dettaglio*: un'unità a 10 HP, che brucia e sta nell'acqua elettrificata, muore
in entrambi gli ordini — ma il TurnLog registra **due reason code diversi**, e il golden replay di CP 12.6 li
fisserà. Ordine scelto: la scarica è un evento **istantaneo** dell'ambiente, il bruciore è un danno **a tempo**
che matura a fine turno.

### D5 — Il danno propagato è una costante di calcolo, non un `Effects`

`Action.Electrify` dichiara `Damage 20` negli `Effects`; i **12** della propagazione vivono in
`URTCombatLibrary::PropagatedElectricDamage`.

*Perché*: gli `Effects` sono ciò che l'azione fa **al bersaglio che qualcuno ha mirato**. Il danno propagato
arriva a chi nessuno ha mirato: è il valore che l'ambiente trasporta, della stessa natura del danno di
`Burning` e del −20 di `Guard`. Metterlo negli `Effects` avrebbe richiesto un secondo campo «a chi si applica
questo effetto», cioè il campo che CP 5.5 ha evitato con la regola per tipo.

### D6 — `Status.Electrified` **non** viene applicato alle unità

Il tag esiste, il catalogo lo dichiara «istantaneo (una sola volta per evento)». Applicarlo produrrebbe uno
stato senza durata e senza consumatore — il difetto ricorrente di questo repository, già contato quattro volte
al CP 8.2. **Non si applica**; l'unicità per evento è garantita dal BFS (ogni cella visitata una volta).

> ✅ **Emendato il 2026-09-03 — [D-315](../decisions/RT_PDR_00_Decision_Log.md), [#1324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1324).**
> Questa riga resta **vera per l'unità** e va letta come dice: il tag non entra in `StatusTurns`, non ha una
> scadenza, e nessuno lo revoca. Ma «non si applica» era stato letto anche come *«non si registra»*, ed è la
> ragione per cui il tag è rimasto **inerte** — dichiarato, con un'icona obbligatoria, e mai nominato da
> nessuno — anche dopo che questo checkpoint è atterrato.
>
> Da oggi la propagazione scrive per ogni unità raggiunta una voce `ERTLogCategory::Status` con esito
> `AppliedInstantly`, nel Cleanup e sulla cella in cui l'ha raggiunta. 🔴 **Il consumatore è il TurnLog, non
> `ApplyStatus`** — e non poteva essere `ApplyStatus`, che rappresenta `N` turni oppure il legame alla cella e
> per `Turns <= 0` ritorna in silenzio. Un evento istantaneo non è nessuna delle due forme.
>
> ⚠️ Il costo dichiarato: cambiano gli **hash** dei turni in cui una scarica si propaga. Il formato **non**
> cambia versione (valore in coda a un enum che viaggia in un `uint8` già presente).

### D7 — `Gadget.Insulator` esce dalla DoD *(spostato a CP 7.2, deciso con l'autore)*

La quinta riga della DoD della issue richiedeva l'immunità del gadget, che appartiene a **E7 CP 7.2** (`#61`,
P2, non costruita). Un DoD non spuntabile senza un'altra epic non chiude un checkpoint, e implementare qui un
flag d'immunità che nessuno valorizza sarebbe un altro dato senza consumatore. La riga è stata **spostata**
sulla issue `#61` con il suo test (`Equipment.Insulator.ImmuneToOnePropagation`).

## 4. Test

| Test | Cosa fisserebbe se cadesse |
|---|---|
| `Environment.WaterElectricPropagation` *(nome vincolante, catalogo §15)* | la combo non funziona **in partita**: danni, propagazione, voci di TurnLog |
| `Environment.Propagation.HitsUnitOnce` | una pozza larga trasforma una scarica in un'esecuzione |
| `Environment.Propagation.DeterministicOrder` | l'ordine dipende dall'input → replay divergente |
| `Environment.Propagation.RespectsRangeLimit` | il limite del catalogo non è rispettato |
| `Environment.Propagation.StopsAtNonConductive` *(aggiunto)* | la catena non si interrompe: il caso **negativo** che la DoD non copriva |
| `Environment.Propagation.InitialAndPropagatedDamageDiffer` *(aggiunto)* | 20 e 12 diventano lo stesso numero |
| `Environment.Propagation.NoMapFailsClosed` *(aggiunto)* | senza mappa si inventa una topologia |
| `Environment.Propagation.ElectrifiedIsLoggedAsInstantLabel` *(aggiunto, [D-315](../decisions/RT_PDR_00_Decision_Log.md))* | **due difetti opposti insieme**: se cade la prima metà la scarica è tornata muta in un replay; se cade la seconda qualcuno ha inventato una durata al tag per farlo sembrare vivo |

I tre aggiunti vengono dalla review di panel della specifica: la DoD originale non aveva nessun test sul caso
negativo né sulla differenza fra i due danni.

Il quarto viene da [#1324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1324) ed è l'unico che
asserisce **due metà in tensione**: un test che verificasse solo l'esistenza della voce sarebbe passato anche
davanti a un `ApplyStatus(Tag, 1)`, cioè davanti alla soluzione che §D6 esiste per vietare.

### 4.1 Verifiche di mutazione eseguite

| Mutazione | Test caduti | Atteso |
|---|---|---|
| conduttività ignorata **+** limite +2 **+** nessuna memoria delle celle visitate *(mutazioni disgiunte, un giro solo)* | `StopsAtNonConductive`, `RespectsRangeLimit`, `HitsUnitOnce`, `DeterministicOrder`, `WaterElectricPropagation` | ✅ |
| danno propagato = danno iniziale (nel `TurnManager`) | `WaterElectricPropagation` | ✅ — **non** `InitialAndPropagatedDamageDiffer`, che passa i numeri esplicitamente alla funzione pura: quella differenza la protegge **solo** il test d'integrazione, ed è la ragione per cui esiste |
| `ResolveCombat` torna a consumare le azioni di fase `Environment` | `WaterElectricPropagation` | ✅ |

> ⚠️ **Incidente di processo, registrato perché non si ripeta**: durante il secondo giro ho usato
> `git checkout <file>` per «togliere la mutazione» da un file la cui implementazione **non era ancora
> committata** — cancellando la funzione appena scritta. La build è poi fallita (`LNK2019`) e i test hanno
> girato sul binario precedente, mostrando gli stessi fallimenti del giro prima. Regola: **committare
> l'implementazione prima di iniziare le mutazioni**, e verificare `Result: Succeeded` sull'output completo
> della build a ogni giro.

**Suite**: **359 test unici in 53 file** (da 352 in 52), 0 fallimenti, build `RefactorTactics` e
`RefactorTacticsEditor` verdi.

## 5. Fuori scope dichiarato

- **Interazioni fuoco/acqua** (`Wet` spegne `Burning`, acqua che congela): **CP 8.4** (`#67`).
- **`Hero.Gadget.ConductiveNode`**: «rende conduttiva una cella per 2 turni» richiede **terreno dinamico** (una cella
  che cambia superficie a runtime), che il modello non ha — la mappa è un asset statico. Resta senza effetti,
  come dichiarato dal catalogo eroi; la sua abilitazione naturale è CP 8.4/E9.
- **Targeting per cella** di `Action.Electrify` (colpire una pozza vuota): E11, vedi D3.
- **Immunità alla propagazione** (`Gadget.Insulator`): CP 7.2, vedi D7.
- **Chi ha `Action.Electrify` nel kit**: nessun eroe del roster v0.1 la dichiara, come per `Action.Heal`,
  `Action.CreateWater` e `Action.Ignite`. Le azioni core sono il **vocabolario** da cui eroi ed equipaggiamento
  attingono; il consumatore che questo checkpoint richiede è il **resolver**, ed è verificato in partita.

## 6. File coinvolti

| File | Modifica |
|---|---|
| `Terrain/RTTerrainData.h` | `FRTPropagationHit` (unità, passi, danno, cella) |
| `Terrain/RTTerrainLibrary.{h,cpp}` | `CollectElectricPropagation`: BFS puro, ordine totale |
| `Ability/RTCatalogLibrary.cpp` | `Action.Electrify` nel catalogo core, `PropagationLimit = 3` |
| `Combat/RTCombatLibrary.h` | `PropagatedElectricDamage = 12` |
| `Turn/RTTurnManager.{h,cpp}` | `ResolveEnvironment` nel Cleanup; le azioni ambientali non vengono più consumate dal Blast |
| `Tests/RTElectricPropagationTests.cpp` (nuovo) | i sette test |

## 7. Rischi

- **Il golden hash cambierà**: il TurnLog guadagna voci di categoria `Combat` in fase `Cleanup` con
  `ActionId = Action.Electrify`. È atteso (E15 §rischi lo prevede per ogni epic che atterra), ma il corpus di
  CP 12.6 va generato **dopo** E8.
- **Nessun eroe usa `Action.Electrify`**: finché E7 (gadget) o una variante d'eroe non la mette in un kit, in
  partita la propagazione si vede solo negli scenari che la pianificano esplicitamente. È dichiarato in §5, non
  nascosto.
