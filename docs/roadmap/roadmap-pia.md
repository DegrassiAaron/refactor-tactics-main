# PIA — Playable Integration & Acceptance

> **PIA non è una terza copia dello stato della release.**
>
> Lo stato delle epic resta in [`roadmap-v0.1.md`](roadmap-v0.1.md).
> La vista di esecuzione generale resta in [`roadmap-checkpoint.md`](roadmap-checkpoint.md).
> **La matrice dei gate di release resta in [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) §3
> — `G1`–`G14`**, che ha già le colonne criterio, evidenza e stato datato.
> Il registro delle verifiche manuali resta in [`test-manuali-pie.md`](../technical/test-manuali-pie.md).
>
> **PIA registra soltanto il progresso dei propri gate di integrazione e accettazione.**

Questo documento non duplica conteggi globali di issue, test o feature: per quelli valgono i documenti sopra.

**Issue roadmap**: `#2623` · **Milestone**: `PIA` (`#18`) · **Baseline**: `008edd1c`, 2026-09-06

---

## 1. A che serve

Portare il vertical slice v0.1 da «le feature esistono» a:

> il gioco integrato è **giocabile**, **comprensibile**, **deterministicamente verificato** e **provato in una
> build packaged**.

PIA non cambia l'ownership di nessuna feature. Ogni gate PIA **referenzia** un owner esistente; dove manca una
**schedulazione**, e solo lì, PIA apre una leaf.

---

## 2. Perché la matrice non è qui

Il mandato originale chiedeva a PIA «una matrice unica» di accettazione, e insieme vietava di creare
«una nuova source of truth dello stato della release». Le due richieste si contraddicono, perché la matrice
esiste già.

Misurato il **2026-09-06** su `008edd1c`: [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) §3 porta
`G1`–`G14` con criterio, evidenza e stato datato, e l'epic `#26` (E12 — Determinismo, QA e release) ne
aggrega già il progresso.

Quindi PIA **mappa**, non duplica. Ogni `PASS` PIA viene riportato sulla riga `G-n` corrispondente: un verde che
vivesse solo nella milestone `PIA` sarebbe la quarta tabella di stato che PIA esiste per non creare.

⚠️ **Due gate PIA non hanno un `G-n` corrispondente** — `PIA-0`, che produce questa mappa, e `PIA-2`, che
verifica la leggibilità del combattimento. Per non lasciare §11 insoddisfacibile, il loro esito si riporta su
**`G14`** (*documentazione allineata*), che è la riga dove la v0.1 registra la coerenza fra ciò che i documenti
promettono e ciò che il gioco fa. Non è un ripiego: se le cue di combattimento non sono leggibili, il canone
che le descrive è disallineato dal prodotto, ed è esattamente ciò che `G14` misura.

---

## 3. Mappa `PIA-x` verso `G-n`

| Gate PIA | Issue | Gate DoD | Owner esistenti |
|---|---|---|---|
| **PIA-0** Baseline e mappa | `#2615` | `G14` *(riporto)* | `#26` |
| **PIA-1** Showcase e scenari | `#2616` | `G4` | `#153` `#152` `#2149` `#2462` `#2588` |
| **PIA-2** Combat feedback | `#2617` | `G14` *(riporto)* | `#2453` `#2454` `#2456` `#2457` `#2505` · `#2455` **chiusa 2026-09-06** |
| **PIA-3** HUD, pointer, privacy | `#2618` | `G8` `G9` | `#25` `#217` · sedute `U15` `U43` · `U46` **spesa** |
| **PIA-4** Scene readability e mappa | `#2619` | `G10` | `#286` `#324` `#288` `#289` `#1758` · seduta `U9` |
| **PIA-5** Replay e packaged | `#2620` | `G4` `G12` `G13` | `#26` `#816` |
| **PIA-6** Final acceptance | `#2621` | `G14`, più il riporto su `G1`–`G14` | `#26` |
| *leaf* — tre voci PIE senza seduta | `#2622` | `G9` *(solo `PIE-V01-GHOSTS`)* | `#25` `#2454` `#2505` |

⚠️ **`PIA-1` non è mappato su `G3`.** `G3` chiede che i dieci test nominati dal catalogo **esistano**, ed è ✅
10/10 dal 2026-08-29: non ha residuo che PIA possa far avanzare. Il gate che `#2616` cita nella propria
Definition of Done è `G4`, ed è quello giusto.

---

## 4. Gate già soddisfatti

La regola è: **se un gate è già soddisfatto, registrarlo con evidenza; non creare lavoro per simmetria.**
Applicata all'audit del 2026-09-06, ha tolto cinque voci dal lavoro previsto.

| Gate | Evidenza |
|---|---|
| Privacy degli intenti — **offline**, sul DTO | `G8` ✅ 2026-08-24, rimisurato 2026-08-29 su `bbf0d780`. Cinque test su più file, non tre: `Reactions.IntentNotVisibleToEnemy` e `UI.IntentViewFieldsAreClassified` e `UI.EnemyViewCarriesNoAllyOnlyField` in `RTIntentPrivacyTests.cpp`; `Facing.IntentIsTeamFiltered` in `RTFacingTests.cpp`; `Combat.IntentVisibleToAlliesAlwaysEnemiesOnlyIfRevealed` in `RTCombatLibraryTests.cpp`. |
| Deflect: pool commutativa, con anti-mutazione | **D-309** e **D-312** nel [Decision Log](../decisions/RT_PDR_00_Decision_Log.md) · issue `#1918` · PR `#2032` · `1ac6e734`. Test: `Combat.GuardPoolIsPermutationInvariant`, `Combat.DeflectPoolAbsorbsBeforeGuardPool`, `Combat.GuardAndDeflectAbsorbInDeclaredOrder`. |
| Determinismo, 100 ripetizioni | **`G4` verde dal 2026-08-24** — `Replay.Verifier.ResimulationIsDeterministic`. |
| Packaging Development e Shipping | **`G12` verde dal 2026-08-16** — da **ridatare** sul commit di release. |
| Emissione di `HazardDamage` | `#2460` — produttore in `Source/RefactorTactics/Turn/RTTurnManager.cpp`. Resta il solo lato presentazione, owner `#2505`. |

### 🔴 La privacy è verde **offline**, e PIA non può fermarsi lì

`G8` si chiude con una riserva che va riportata qui per intero, perché senza di essa il verde è dedotto:

> ⚠️ Il gate resta **offline**: misura il DTO, non il filo. Il canary sul traffico è `#784`, `P0` di v0.5.

`#784` (*Canary anti-leak: fallisce se un client riceve un solo byte del piano avversario*) è **aperta**, in
milestone **v0.5**. Quindi:

* per PIA, la privacy degli intenti è `PASS` **sul DTO** — e questo va registrato, non rifatto;
* il canale di rete è `N/A` per la v0.1, che è 2v2 **offline** — non `PASS`, e nemmeno un debito di PIA.

Scriverlo qui evita l'errore che questo documento condanna altrove: leggere «`G8` ✅» come «la privacy è provata»
quando l'oggetto misurato è un altro.

### La nota sul Deflect

Il mandato chiedeva metà della proprietà. `D-309` fissa che il pool è **commutativo** fra colpi; `D-312` fissa
che `Deflect` assorbe **prima** di `Guard`, e aggiunge la misura che lo rende una decisione e non un dettaglio:
l'ordine *«decideva 5 danni su 558 configurazioni raggiungibili»*. Un oracolo che verifichi solo l'invarianza
rispetto all'ordine dei colpi resta verde se qualcuno inverte i due pool. I due assi vanno pinnati entrambi, ed
entrambi lo sono.

---

## 5. Gli oracoli visuali

Dieci gate di `#2617` e uno di `#2619` giudicano ciò che il giocatore **vede**. Formulazioni come «si vede
chiaramente» o «sono distinguibili» non sono falsificabili: un revisore non può scrivere `FAIL` contro di esse,
e un gate su cui non si può fallire non è evidenza. Il registro PIE porta già questa lezione — un criterio
scritto su uno stato **già acceso** risulta verde senza aver provato niente.

Forma richiesta:

> Il fotogramma **A** (istante nominato) e il fotogramma **B** (istante nominato) differiscono in un elemento
> nominato. Se A e B sono indistinguibili, il verdetto è `FAIL`.

Due fotogrammi nominati sono anche l'evidenza allegabile che `#2621` richiede.

---

## 6. L'Editor è la risorsa che ordina il lavoro

Il parallelismo fra `#2617`, `#2618` e `#2619` non è governato dalla contesa sui file, ma dalla **mutua
esclusione dell'Editor** (`CLAUDE.md` §10). I loro gate sono tutti PIE: eseguirli come verifiche indipendenti
significherebbe un lease per voce.

Il repository ha già la soluzione: le **sedute** di [`editor-sessions.yaml`](editor-sessions.yaml), che
raggruppano più voci in una sola apertura. PIA usa quelle, non ne inventa altre.

⚠️ **`U46` è già stata spesa.** La seduta *«i quattro residui del gate `G9`, in una sola apertura»* è stata
eseguita il 2026-09-05/06 (`#2476`, PR `#2541`, commit `6a677671` delle 10:01) — **sei ore prima** del baseline
`008edd1c` di questo documento. Riprogrammarla sarebbe un lease speso per rigiudicare verdetti già scritti.

| Voce | Seduta | Stato al baseline |
|---|---|---|
| `PIE-V01-HUD` | `U15` | ✅ 2026-08-24 |
| `PIE-V01-INTENT` | `U15` | ✅ 2026-08-24 |
| `PIE-DEBUG-CELLS` | `U15` | ✅ 2026-08-07 |
| `PIE-V01-DEBUG` | `U15` | 🟡 — **l'unica voce di `U15` realmente aperta** |
| `PIE-V01-LOG` | `U15`, `U46` | ✅ **eseguita 2026-09-05 in `U46`** |
| `PIE-V01-POINTER` | `U43` | ⏳ |
| `PIE-V01-ROSTER` | `U46` | ✅ 2026-09-06 |
| `PIE-HEXPLAY-6` | `U46` | ❌ **rigiudicata 2026-09-06: la risposta si inverte, non è leggibile** (`#2534`) |
| `PIE-HEXPLAY-8` | `U46` | ⏳ — il residuo superstite di `U46` |
| `PIE-ICON-01` | `U9` | ⏳ — owner `#217` (E20) |
| `PIE-VIS-DEFLECT`, `PIE-VIS-INTERPOSE`, `PIE-V01-GHOSTS` | 🔴 **nessuna** | ⏳ — da cui `#2622` |

### 🔴 `PIE-HEXPLAY-6` è un `FAIL` su una voce `RELEASE-V01`

Non è un dettaglio di seduta: `PIE-HEXPLAY-6` appartiene al subset `RELEASE-V01`, quindi **blocca `G9`**, che
`PIA-3` mappa. Il documento lo registra invece di ereditarlo in silenzio: `PIA-3` non può essere `PASS` finché
`#2534` non è risolta o la voce non è esplicitamente riclassificata.

### Il disallineamento di `PIE-ICON-01`

La voce si esegue nella seduta `U9`, che questo documento assegna a **`PIA-4`**; il suo owner `#217` (E20) è
elencato fra quelli di **`PIA-3`**. È voluto — la verifica sta dove si apre l'Editor, il lavoro sta dove vive
l'epic — ma va detto, perché altrimenti il primo dei due gate a chiudersi registra un verdetto sullo scope
dell'altro. `PIE-ICON-01` si conta in `PIA-4`.

Solo `#2616` è realmente parallelizzabile: è interamente headless.

---

## 7. Sequenza

Le frecce sono dipendenze: un nodo non chiude finché quelli a monte non hanno un esito.

```text
                       PIA-0 #2615  (baseline: prerequisito di tutti)
                             |
        +--------------------+--------------------+--------------------+
        |                    |                    |                    |
        v                    v                    v                    v
   PIA-1 #2616          PIA-2 #2617          PIA-3 #2618          PIA-4 #2619
   headless             \                    |                    /
   parallelizzabile      \___ contendono l'Editor: si eseguono __/
        |                     in sedute, non in parallelo
        |                                   |
        +-----------------+-----------------+
                          |
                          v
                    PIA-5 #2620   replay + packaged
                          |
                          v
                    PIA-6 #2621   final acceptance
```

`PIA-6` non chiude finché **tutti** i gate a monte hanno un esito registrato: `PASS`, `FAIL` o `NOT RUN` con
motivo. Nessun ramo salta `PIA-6`.

---

## 8. Regole di verdetto

Gli stati sono i quattro di `CLAUDE.md` §6, e non ce ne sono altri:

| Stato | Quando |
|---|---|
| `PASS` | il gate è stato **eseguito** e ha avuto esito positivo |
| `FAIL` | eseguito, esito negativo — come `PIE-HEXPLAY-6` oggi |
| `NOT RUN` | non eseguito. **Deve portare il motivo**, e se il motivo è un blocco deve **nominare** l'issue che blocca |
| `N/A` | il gate non si applica a questa release — come il canale di rete per una v0.1 offline |

Le formule del mandato si traducono così, per non introdurre un quinto stato:

* *«BLOCKED — owner»* → `NOT RUN — bloccato da #nnn`
* *«OUT OF PIA — owner/release»* → `N/A — owner/release`

Regole invarianti:

* `PASS` **solo dopo esecuzione reale**.
* Feature implementata **non equivale a** test `PASS`.
* Issue chiusa **non equivale a** gate `PASS`.
* `NOT RUN` è meglio di un verde dedotto.
* Il silenzio non è uno stato.

---

## 9. Priorità

**P0** — T8 e showcase completo · Deflect discriminante · `HazardDamage` end-to-end · combat feedback che rende
giudicabili Deflect e Interpose · HUD e interazione essenziali · privacy degli intenti · leggibilità della scena
(E21, seduta `U9`) · determinismo · packaged smoke · **multilayer smoke** · **`#2534`**, il `FAIL` di
`PIE-HEXPLAY-6` che blocca `G9`.

⚠️ Il multilayer smoke è stato **promosso da `P1` a `P0`**: `G10` chiede una partita completa 2v2 su mappa
**multilivello** e non è verde, quindi non è un extra ma parte di un gate di release aperto.

**P1** — Ghost Timeline · combat log visual · debug tools · door smoke · diagnostica avanzata.

`P1` non significa facoltativo: se il DoD corrente lo richiede, resta gate prima della chiusura della release.

---

## 10. Cosa PIA non crea

Epic `E<n>` · simulator · TurnLog · targeting · LOS · pathfinder · sistema di Facing · sistema di status ·
tassonomia di cue · una nuova source of truth dello stato della release · duplicati di `#2149` o
`#2453`–`#2457` · le Station 02–08 del [Gray Kit Playground](roadmap-gray-kit-playground.md) · networking, GAS
o modding · nuove regole di gameplay per rendere verde uno showcase.

---

## 11. Chiusura

La milestone `PIA` si chiude con `#2621`, e solo dopo che ogni gate PIA chiuso è stato **riportato sulla riga
`G-n` corrispondente** di [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) §3 — con `G14` come
destinazione dichiarata per `PIA-0` e `PIA-2`, che non hanno una riga propria.
