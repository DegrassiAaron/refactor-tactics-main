# RT — Catalogo eroi v0.1

> **Fonte**: catalogo di bilanciamento v0.1 §§7–9 · PDR-12 — oggi in
> [`prd-personaggi-azioni-e-bilanciamento.md`](../src/prd/prd-personaggi-azioni-e-bilanciamento.md) e
> [`RT_PDR_v0.1_consolidato.md`](../archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md)
> **Decisione abilitante**: [`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md) · **Checkpoint**: CP 1.2 (issue `#28`)
> **Implementazione**: epic **E6** (`#54`–`#59`).

## Stato dell'implementazione (2026-08-06)

**Aggiornato al 2026-08-06 (epic E6 completata)**: i quattro eroi esistono come dati
(`URTHeroCatalogLibrary::MakeGadget/MakePhase/MakeRiktor/MakeWraith`) e `ARTGameMode` allestisce il 2v2 con loro
— formazione di default **Gadget + Phase** contro **Riktor + Wraith**. I due archetipi (`ERTArchetype`) non
partecipano più allo spawn di partita; restano come helper nei test d'integrazione.

**Aggiornamento 2026-08-08.** Di quel «non ancora costruito» resta solo una voce: **E8** (terreni dinamici,
superfici attive) ed **E9** (coperture bassa e alta, strutture, distruzione) sono **completate**; le abilità
che dipendevano da loro hanno ora un sistema che le consuma. Manca **E7** (gadget). Ogni caso residuo è
dichiarato nella PR del checkpoint e nei commenti del codice, mai nascosto dietro un effetto segnaposto.

### Overwatch, per ogni eroe

`Overwatch` **non è la skill di nessuno**: è un'azione universale che ogni eroe possiede
([D-012](../decisions/RT_PDR_00_Decision_Log.md) · [D-014](../decisions/RT_PDR_00_Decision_Log.md)), e compete
con l'azione offensiva principale. Ciò che **cambia per eroe** è il **profilo**: cosa fa scattare la reazione e
con quale effetto, secondo kit ed equipaggiamento.

I quattro profili **non esistono ancora** — né nel codice né in una fonte corrente — e questo documento **non
li inventa**. Si definiscono in **E14**, insieme alla finestra di decisione.

### Quota elevata

Nessun eroe riceve danno o vista in più per il fatto di stare in alto
([D-018](../decisions/RT_PDR_00_Decision_Log.md) · [D-024](../decisions/RT_PDR_00_Decision_Log.md)): l'altura
vale per geometria. Un eroe **può** dichiarare un bonus legato alla quota, ma allora è **suo**, scritto nel suo
kit — non una regola implicita del terreno.

### Reazioni — aggiornamento del 2026-08-07 (CP 6.7, `#155`)

Le reazioni **non** sono più in quella lista: il motore di E5 le regge (CP 5.5, `#154`) e tre delle quattro
sono cablate e verificate in partita.

| Reazione | Semantica core riusata | Stato |
|---|---|---|
| `Hero.Gadget.ReactiveCapacitor` | `Action.Counter` | ✅ scudo 15 **e** 10 danni all'attaccante |
| `Hero.Riktor.Interposition` | `Action.Intercept` | ✅ incassa il colpo diretto a un alleato entro 2 celle |
| `Hero.Wraith.Deflection` | `Action.Deflect` | ✅ −20 sul colpo che l'ha innescata |
| `Hero.Phase.FlowReaction` | — | ⏳ **E14**: produce movimento dentro un boundary di risoluzione |

La rinviata lo dichiara **nei dati** (slot `None`, nessun trigger), non solo nei commenti: con lo slot
`Reaction` il pass del turno la raccoglierebbe e registrerebbe un'attivazione che non produce nulla.

> ➖ **`Hero.Wraith.InterceptShot` è uscita da questa tabella il 2026-08-17, e il denominatore è calato con
> lei: erano cinque.** Non è una reazione — dal 2026-08-10 è una **Predictive Action** consegnata
> (E18 CP 18.2, [D-016](../decisions/RT_PDR_00_Decision_Log.md)), con slot `Main`,
> `PredictiveTargeting = LockCell` e `PredictionBoundary = MovementEntry`. Non attende un innesco: si
> dichiara in pianificazione e si risolve a un boundary deterministico.
> Il suo **Tipo** nella tabella delle abilità di Wraith è ora `predittiva`, ed è **lì** che la rubrica dei
> radar legge la condizionalità del payoff — non più da questa cella di prosa.

Identità, cooldown ed effetti restano dell'eroe; fase, priorità, slot e trigger vengono dall'azione core.

## Struttura di un eroe

**Fisso**: identità · ruolo · attacco base · **quattro abilità fondamentali** · affinità ambientale · debolezza ·
statistiche base.
**Configurabile**: variante arma · gadget · modulo di reazione · **variante di una** abilità (una sola per eroe
nel vertical slice).

---

## 1. Gadget — tecnico della conduzione

**Ruolo**: attacco · controllo · combo elettrica · disattivazione dispositivi.

| Statistica | Valore |
|---|---:|
| Salute | 90 |
| Movimento | 5 MP |
| Range visivo | 7 — *era 6, alzata da [D-073](../decisions/RT_PDR_00_Decision_Log.md) (`#131`): è l'unico del roster che vede oltre il raggio 6* |
| Resistenza Push | 0 |
| Affinità | elettricità |
| Debolezza | acqua (`Affinity.Water`) — decisa in CP 6.2, non nel PDF: stesso identificatore dell'affinità di Phase |

| AbilityId | Abilità | Tipo | Effetto | CD |
|---|---|---|---|---:|
| `Hero.Gadget.ArcPulse` | Impulso ad arco | attacco base | 22 danni, range 4 | 0 |
| `Hero.Gadget.LinearDischarge` | Scarica lineare | linea | 24 danni, **+8 su bersaglio `Wet`** | 2 |
| `Hero.Gadget.ConductiveNode` | Nodo conduttore | cella | **è `Action.Electrify`**: scarica sul grafo conduttivo, range 4, propagazione 3 ([D-064](../decisions/RT_PDR_00_Decision_Log.md)) | 2 |
| `Hero.Gadget.Overload` | Sovraccarico | AoE | 18 danni, `Interrupt` sui dispositivi | 3 |
| `Hero.Gadget.ReactiveCapacitor` | Capacitore reattivo | reazione | scudo 15 e 10 danni all'attaccante | 3 |

> **Ownership del bonus `Wet`** ([D-029](../decisions/RT_PDR_00_Decision_Log.md) ·
> [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md)). Il `+8` di `Hero.Gadget.LinearDischarge` è una
> condizione **dell'abilità di Gadget** su uno **stato del sistema**: dipende da `Status.Wet` sul bersaglio, non
> dall'eroe che ha applicato `Wet`. Phase è oggi la sorgente più comune, ma **non** è un requisito: qualsiasi
> sorgente di `Wet` autorizzata dalle regole (`Gadget.Sprinkler`, acqua bassa del terreno, una futura abilità)
> abilita lo stesso payoff. Le etichette storiche **`Water-Electric Combo`** (Signature secondaria di Gadget) e
> «combo elettrica» qui sopra nominano quell'**interazione sistemica**, non una coppia Gadget + Phase: restano
> invariate perché sono dati canonici e un rename richiede migrazione, non una PR documentale.

**Variante di `LinearDischarge`**
- *Scarica concentrata*: **+6 danni**, ma **non si propaga**.
- *Scarica ramificata*: **bersaglio aggiuntivo**, ma **−6 danni per bersaglio**.

---

## 2. Phase — manipolatrice dell'acqua

**Ruolo**: supporto · controllo del terreno · setup di combo · riposizionamento.

| Statistica | Valore |
|---|---:|
| Salute | 95 |
| Movimento | 5 MP |
| Range visivo | 5 |
| Resistenza Push | 0 |
| Affinità | acqua |
| Debolezza | elettricità (`Affinity.Electricity`) — decisa in CP 6.3, non nel PDF: simmetrica a Gadget |

| AbilityId | Abilità | Tipo | Effetto | CD |
|---|---|---|---|---:|
| `Hero.Phase.PressureJet` | Getto pressurizzato | linea | 16 danni, applica `Wet`, `Push 1` | 0 |
| `Hero.Phase.CircularTide` | Marea circolare | AoE | cura 18 agli alleati, `Wet` ai nemici | 2 |
| `Hero.Phase.FluidTrail` | Scia fluida | dash | `Dash 3` e crea acqua lungo il percorso | 2 |
| `Hero.Phase.MistVeil` | Velo di nebbia | AoE | crea fumo raggio 1 | 3 |
| `Hero.Phase.FlowReaction` | Flusso reattivo | reazione | `Reposition 1` dopo un attacco | 3 |

**Variante di `CircularTide`**
- *Marea curativa*: cura **24**, ma **non applica `Wet`** ai nemici (niente setup elettrico).
- *Marea d'urto*: cura **10**, ma applica **`Push 1`** ai nemici.

---

## 3. Riktor — architetto del campo

**Ruolo**: difesa · controllo dello spazio · modifica degli archi · protezione degli alleati.

| Statistica | Valore |
|---|---:|
| Salute | 120 |
| Movimento | 4 MP |
| Range visivo | 5 |
| Resistenza Push | 0 — *era 1, l'unica del roster, azzerata da [D-075](../decisions/RT_PDR_00_Decision_Log.md) (`#402`): a soglia 1 era immunità totale, non stabilità — vedi §5* |
| Affinità | strutture |
| Debolezza | movimento (`Affinity.Movement`) — decisa in CP 6.4, non nel PDF: simmetrica a Wraith |

| AbilityId | Abilità | Tipo | Effetto | CD |
|---|---|---|---|---:|
| `Hero.Riktor.ImpactShot` | Colpo cinetico | attacco base | 8 danni, range 3, applica `Slow` | 0 |
| `Hero.Riktor.KineticPanel` | Pannello cinetico | arco | crea una copertura da 30 HP | 2 |
| `Hero.Riktor.Reconfigure` | Riconfigurazione | arco | sposta o ruota una copertura | 2 |
| `Hero.Riktor.Ram` | Ariete | charge | 20 danni e `Push 1` | 2 |
| `Hero.Riktor.Interposition` | Interposizione | reazione | intercetta un attacco diretto a un alleato | 3 |

**Variante di `KineticPanel`**
- *Pannello rinforzato*: integrità **45**, ma durata **1 turno**.
- *Pannello adattivo*: integrità **25**, ma **ruotabile gratuitamente una volta**.

---

## 4. Wraith — duellante predittivo

**Ruolo**: assalto · interruzione · punizione del movimento · duello.

| Statistica | Valore |
|---|---:|
| Salute | 90 — *era 100, abbassata da [D-069](../decisions/RT_PDR_00_Decision_Log.md) (`#131`)* |
| Movimento | 6 MP |
| Range visivo | 6 |
| Resistenza Push | 0 |
| Affinità | movimento |
| Debolezza | strutture (`Affinity.Structures`) — decisa in CP 6.5, non nel PDF: simmetrica a Riktor |

| AbilityId | Abilità | Tipo | Effetto | CD |
|---|---|---|---|---:|
| `Hero.Wraith.PulseShot` | Tiro a impulsi | attacco base | 21 danni, range 4 | 0 |
| `Hero.Wraith.InterceptShot` | Tiro d'intercetto | predittiva | 16 danni e **stop del movimento** | 2 |
| `Hero.Wraith.PassingBlade` | Lama di passaggio | dash | `Dash 3`, 20 danni attraversando | 2 |
| `Hero.Wraith.Deflection` | Deviazione | reazione | riduce il danno di 20 | 2 |
| `Hero.Wraith.Feint` | Finta | controllo | marca una cella e ottiene `Reposition` | 2 |

> ℹ️ **`InterceptShot` è di tipo `predittiva`, e il trigger non sta nella cella `Effetto`** — quella cella ha
> un **vocabolario chiuso**, verificato da `Radar.Vocabulary`, e scriverci prosa la fa fallire. Il trigger è
> **d'ingresso su movimento**, non «sono stato colpito»: la cella si dichiara in pianificazione, si blocca
> (`PredictiveTargeting = LockCell`) e si risolve al passaggio dell'avversario
> (`PredictionBoundary = MovementEntry`), senza input durante la Resolution.
> È da `Tipo = predittiva` che la rubrica dei radar deduce che il payoff è **condizionale a una previsione**
> e non entra nel danno garantito (#557, #1080).

**Variante di `InterceptShot`**
- *Intercetto preciso*: **20 danni**, ma controlla **una sola cella**.
- *Intercetto esteso*: **14 danni**, ma controlla **una linea di 3 celle**.

---

## 5. Confronto rapido

| Eroe | HP | MP | Vista | Push res. | Affinità | Identità in una riga |
|---|---:|---:|---:|---:|---|---|
| Gadget | 90 | 5 | 7 | 0 | elettricità | fragile, trasforma l'acqua altrui in danno, e **vede più lontano di tutti** |
| Phase | 95 | 5 | 5 | 0 | acqua | prepara il terreno agli altri e cura |
| Riktor | 120 | 4 | 5 | 0 | strutture | cambia la forma della mappa, lento |
| Wraith | 90 | 6 | 6 | 0 | movimento | punisce chi si muove, il più mobile |

### 5.1 Percezione e risorsa firma — consolidato il 2026-08-07

La colonna **Vista** smette di essere inerte: con lo slice di conoscenza parziale
([`brief-conoscenza-parziale.md`](../gameplay/brief-conoscenza-parziale.md)) decide **cosa la squadra può bersagliare**.
Nessuna azione del roster supera la vista di chi la usa (max `range` 5 su vista minima 5), quindi il vincolo
non riduce la gittata di nessuno: la vista lunga vale **anticipo d'informazione**, non danno.

| Eroe | Vista | Ruolo | Risorsa firma | Ricarica su | Cap |
|---|---:|---|---|---|---:|
| Gadget | 7 | Controller | Carica Conduttiva | interazione elettrica | 4 |
| Phase | 5 | Support | Riserva Idrica | interazione con acqua | 4 |
| Riktor | 5 | Guardian | Integrità Strutturale | Cleanup | 4 |
| Wraith | 6 | Striker | Slancio | movimento eseguito | 4 |

I restanti parametri di percezione (`Detection`, `Identification`, `Stealth`, `Tracking`, firme) sono
inizializzati **al valore più basso disponibile, uguale per tutti** (Detection 48, Identification 45,
Stealth 2, Tracking 1): si parte piatti e si differenzia col playtest.

> ⚠️ **Corretto il 2026-08-08.** Questo paragrafo diceva: «*nessuno di essi entra nella v0.1 — lo slice è
> binario (in vista / fuori vista)*». **Non è più vero**, e i numeri qui sopra venivano dal workbook, che è
> `RESEARCH` ([D-023](../decisions/RT_PDR_00_Decision_Log.md)).
>
> La percezione della v0.1 **non è binaria**: è **conoscenza parziale** a tre livelli — `Nascosto`,
> `ContattoIncerto`, `Rilevato` — più `UltimoContatto`, unita per squadra (**Team Knowledge**, epic **E13**).
> Il **rumore** è un secondo canale, propagato con interi sul grafo. Il **facing** aggiunge un cono frontale
> più la consapevolezza ravvicinata a 360° entro 2 celle
> ([ADR-0005](../decisions/adr-0005-orientamento.md) §4b, epic **E16**).
>
> Quali di questi parametri diventino statistiche per eroe, e con quali valori, si decide in **E13**: qui non
> si scrive un numero che nessun sistema legge.

Riktor compra HP con **movimento** e vista; Wraith compra mobilità con **salute**; Phase sta in
mezzo; Gadget ha il danno combo più alto.

> ✅ **Aggiornato il 2026-08-10 ([D-069](../decisions/RT_PDR_00_Decision_Log.md), `#131`): Wraith 100 → 90.**
> La frase qui sopra diceva che Wraith «compra mobilità con l'assenza di difese» mentre sulle quattro
> statistiche base **non comprava nulla**: a 100/6/6/0 era migliore o pari ovunque rispetto a Gadget (90/5/6/0)
> *e* a Phase (95/5/5/0), e strettamente migliore in salute **e** movimento. Adesso il costo è un numero.
>
> ✅ **Chiusa il 2026-08-10 con la seconda leva: Gadget 6 → 7 di vista ([D-073](../decisions/RT_PDR_00_Decision_Log.md)).**
> Il calo di Wraith aveva tolto la dominanza su Phase e lasciato quella su Gadget, dove a parità di salute e
> vista Wraith restava avanti di un punto movimento. Con la vista 7 Gadget ha qualcosa di strettamente
> migliore, e **nessun eroe domina più nessun altro** sulle quattro statistiche base.
>
> **Perché la vista e non il movimento**, misurato e non intuito: dare 6 MP a Gadget — o toglierne uno a
> Wraith — renderebbe i due profili **identici**, e `RosterIsBalanced` verifica che nessuna coppia li
> condivida; si sarebbe rotto un test per ripararne un altro. E una `PushResistance` negativa per Wraith
> sarebbe stata un numero **senza effetto osservabile**: è una soglia, e le spinte del catalogo valgono
> almeno 1.
>
> Il test non asserisce più due coppie scelte a mano: verifica la non-dominanza su **ogni** coppia del
> roster. È la forma che regge quando il roster crescerà a otto (E35) — un eroe nuovo che dominasse qualcuno
> fa cadere il test da solo, senza che nessuno debba ricordarsi di aggiungere una riga.
>
> La compensazione nelle **abilità** resta com'era e non era in discussione: Gadget ha il bonus combo più alto
> del roster (+8 su `Wet`), Phase la cura ad area.

> ✅ **Allineato il 2026-08-12 ([D-075](../decisions/RT_PDR_00_Decision_Log.md), `#402`): Riktor 1 → 0 di
> resistenza alla spinta.** La decisione è del **2026-08-10** e `RTHeroCatalogLibrary.cpp` scrive `0` da
> allora: questo documento era rimasto indietro, e la frase qui sopra vendeva come prezzo pagato — «compra HP
> **e resistenza**» — una statistica che l'eroe non ha.
>
> Perché quell'`1` non era stabilità: la resistenza è una **soglia**
> ([D-038](../decisions/RT_PDR_00_Decision_Log.md)) e ogni spinta del gioco vale `1`
> ([D-074](../decisions/RT_PDR_00_Decision_Log.md)), quindi valeva **immunità a ogni spostamento, sempre,
> senza spendere un'azione** — e svuotava metà di `Guard` e `Brace` proprio sull'eroe su cui la scelta
> difensiva dovrebbe pesare di più. Il prezzo resta reale su **120 HP e 4 MP**: non dipendeva da questo campo.
>
> ⚠️ **La colonna «Push res.» è ora interamente `0`, e `PushResistance` è una meccanica dormiente**: modello e
> resolver la implementano (ramo `ERTActionEffect::Push`), **nessun contenuto la esercita**. È dichiarato, non
> dimenticato.
>
> 🔴 **Ma il modo in cui si risveglia era scritto al contrario, ed è stato corretto il 2026-08-16.** Questa
> riga diceva *«si risveglia da sé se una v0.2 introduce una spinta `≥ 2`»*. È falso in **entrambi** i sensi.
> `PushResistance` è una **soglia**, non una sottrazione (D-038): il resolver annulla lo spostamento solo se
> `Amount <= PushResistance` (`RTTurnManager.cpp:4214`). Con la colonna a `0`, **nessuna** spinta viene mai
> annullata — e una spinta più **forte** rende la soglia più difficile da raggiungere, non più facile: contro
> uno spostamento di 2 servirebbe `PushResistance ≥ 2`. Ciò che risveglia la meccanica è **un eroe con
> `PushResistance > 0`**, cioè una decisione di contenuto sul roster, non una spinta nuova.
>
> ⚠️ E la spinta forte è comunque **già arrivata**, il che rende la vecchia formulazione doppiamente
> ingannevole: `Weapon.Impact` porta `Hero.Phase.PressureJet` a **2** ([D-085](../decisions/RT_PDR_00_Decision_Log.md)),
> default di Phase ([D-089](../decisions/RT_PDR_00_Decision_Log.md)) — e la colonna è rimasta dormiente
> esattamente come prima, che è la prova di quanto sopra.
>
> L'esito è pinnato dallo scenario
> `Spec.Combat.RiktorIsPushedLikeAnyone`, che manda Riktor e Wraith a incassare lo stesso `Hero.Phase.PressureJet`
> e li fa arretrare **entrambi**; la regola della soglia resta pinnata da
> `RefactorTactics.Actions.PushResistanceIsAThreshold`, che il valore se lo costruisce da solo.
>
> ⚠️ Conseguenza per chi deriva viste dai cataloghi: `Resistenza Push` è **costante sul roster**, quindi non
> discrimina. Un asse che la somma alla `Salute` ricade sulla sola salute — e lì Gadget e Wraith sono **entrambi
> a 90**.

**Debolezza dichiarata**: il PDF elenca «debolezza» fra gli elementi fissi di ogni eroe ma **non la esplicita**
per nessuno dei quattro. Va fissata in E6 e scritta qui: senza, l'identità resta metà. **Gadget**: fissata in
CP 6.2, acqua (`Affinity.Water`) — vedi §1. **Phase**: fissata in CP 6.3, elettricità (`Affinity.Electricity`),
simmetrica a Gadget — vedi §2. **Riktor**: fissata in CP 6.4, movimento (`Affinity.Movement`), simmetrica a
Wraith — vedi §3. **Wraith**: fissata in CP 6.5, strutture (`Affinity.Structures`) — vedi §4.

Il roster chiude in **due coppie simmetriche**: Gadget↔Phase sull'acqua/elettricità, Riktor↔Wraith sullo
spazio/movimento. La debolezza di ogni eroe è l'affinità di un altro — verificato da
`RefactorTactics.Heroes.RosterIsBalanced`.

---

## 6. Loadout iniziali consigliati

| Eroe | Variante d'abilità | Gadget | Modulo di reazione |
|---|---|---|---|
| Gadget | Scarica ramificata | `Gadget.Insulator` | `Reaction.ReactiveShield` |
| Phase | Marea curativa | `Gadget.Sprinkler` | `Reaction.HazardEscape` |
| Riktor | Pannello adattivo | `Gadget.PortableCover` | `Reaction.AllyIntercept` |
| Wraith | Intercetto esteso | `Gadget.Sensor` | `Reaction.EmergencyDash` |

---

## 7. Divergenze rispetto al PDF (dichiarate)

| # | PDF | Qui | Motivo |
|---|---|---|---|
| 1 | Tabelle delle abilità con nomi, effetti e cooldown sfalsati nell'estrazione | Ricostruite accoppiando `AbilityId` → effetto per posizione e per coerenza semantica (es. `Hero.Phase.MistVeil` → fumo, non «cura alleati») | L'accoppiamento letterale produceva abilità incoerenti col nome e col ruolo |
| 2 | «Debolezza» dichiarata fra gli elementi fissi | ~~Assente~~ → **fissata in E6** per tutti e quattro (CP 6.2–6.5), in due coppie simmetriche | Non è stata inventata: decisa esplicitamente eroe per eroe |
| 3 | 4 eroi | ~~In codice esistono 2 archetipi~~ → **risolto in E6**: i quattro eroi sono in codice e in partita | Lo stato aggiornato è dichiarato in testa |
| 4 | Cooldown di `Hero.Phase.PressureJet` non leggibile nella colonna | Assunto **0** (è l'attacco base per la sua colonna «Tipo: linea» a costo 0) | Coerente con gli altri attacchi base, tutti a CD 0 — assunzione **marcata** |
| 5 | `Bastion.ImpactShot`: 24 danni | **8 danni + `Slow` 1 turno**, range 3 invariato ([ADR-0007](../decisions/adr-0007-attacco-base-per-eroe.md), 2026-08-09) | A 24 era l'attacco base **più forte del roster**, mentre il ruolo dichiarato di Riktor è Utility/Emergency: la contraddizione stava nei numeri, non nel ruolo. 8 è la metà esatta di `Riva.PressureJet` (16), che sta un gradino sopra. Lo `Slow` è l'unica delle utility candidate insieme esprimibile e coerente — `ERTStructureOp` non danneggia coperture, e uno `Status` si applica al bersaglio, quindi «genera Guard su di sé» non è rappresentabile | <!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->

**Non specificato nel PDF** (da fissare in E6): debolezza di ciascun eroe (**tutte fissate**: Gadget CP 6.2, Phase
CP 6.3, Riktor CP 6.4, Wraith CP 6.5) ·
range di `Hero.Gadget.Overload` (fissato in CP 6.2: **3**, coerente con `ConductiveNode`) e `Hero.Phase.CircularTide`
(fissato in CP 6.3: **4**, come `Hero.Gadget.Overload`) · durata di `Status.Wet` (fissata in CP 6.3: **1 turno**, come
`Guard`/`Exposed`/`Marked` — finestra di combo stretta) · durata di `Hero.Wraith.Feint` (fissata in CP 6.5: **1 turno**, come `Wet`/`Marked`) · se le reazioni
degli eroi occupino lo stesso slot dei moduli di reazione dell'equipaggiamento (probabile, ma il PDF elenca
entrambi senza dirlo).
