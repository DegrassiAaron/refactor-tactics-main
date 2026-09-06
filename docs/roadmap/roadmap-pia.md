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

**Issue roadmap**: `#2623` · **Milestone**: `PIA` (`#18`)

---

## 1. A che serve

Portare il vertical slice v0.1 da «le feature esistono» a:

> il gioco integrato è **giocabile**, **comprensibile**, **deterministicamente verificato** e **provato in una
> build packaged**.

PIA non cambia l'ownership di nessuna feature. Ogni gate PIA **referenzia** un owner esistente; dove un owner
non esiste, e solo lì, PIA apre una leaf.

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

---

## 3. Mappa `PIA-x` verso `G-n`

| Gate PIA | Issue | Gate DoD | Owner esistenti |
|---|---|---|---|
| **PIA-0** Baseline e mappa | `#2615` | — *(produce questa mappa)* | `#26` |
| **PIA-1** Showcase e scenari | `#2616` | `G3` | `#153` `#152` `#2149` `#2462` `#2588` |
| **PIA-2** Combat feedback | `#2617` | — | `#2453` `#2454` `#2455` `#2456` `#2457` `#2505` |
| **PIA-3** HUD, pointer, privacy | `#2618` | `G8` `G9` | `#25` `#217` · sedute `U15` `U43` `U46` |
| **PIA-4** Scene readability e mappa | `#2619` | `G10` | `#286` `#324` `#288` `#289` `#1758` · seduta `U9` |
| **PIA-5** Replay e packaged | `#2620` | `G4` `G12` `G13` | `#26` `#816` |
| **PIA-6** Final acceptance | `#2621` | `G14`, più il riporto su `G1`–`G14` | `#26` |
| *leaf* — tre voci PIE senza seduta | `#2622` | — | *nessuno: residuo genuino* |

---

## 4. Gate già soddisfatti

La regola è: **se un gate è già soddisfatto, registrarlo con evidenza; non creare lavoro per simmetria.**
Applicata all'audit del 2026-09-06, ha tolto cinque voci dal lavoro previsto.

| Gate | Evidenza |
|---|---|
| Privacy degli intenti — l'oracolo automatico sul view sanitizzato | `Reactions.IntentNotVisibleToEnemy`, `UI.IntentViewFieldsAreClassified`, `UI.EnemyViewCarriesNoAllyOnly…` in `Source/RefactorTactics/Tests/RTIntentPrivacyTests.cpp`. Sono i test con cui **`G8` è verde**. |
| Deflect: pool commutativa, con anti-mutazione | **D-309** e **D-312** nel [Decision Log](../decisions/RT_PDR_00_Decision_Log.md) · issue `#1918` · PR `#2032` · `1ac6e734`. Test: `Combat.GuardPoolIsPermutationInvariant`, `Combat.DeflectPoolAbsorbsBeforeGuardPool`, `Combat.GuardAndDeflectAbsorbInDeclaredOrder`. |
| Determinismo, 100 ripetizioni | **`G4` verde dal 2026-08-24** — `Replay.Verifier.ResimulationIsDeterministic`. |
| Packaging Development e Shipping | **`G12` verde dal 2026-08-16** — da **ridatare** sul commit di release. |
| Emissione di `HazardDamage` | `#2460` — produttore in `Source/RefactorTactics/Turn/RTTurnManager.cpp`. Resta il solo lato presentazione, owner `#2505`. |

⚠️ **Il caso Deflect merita una nota**, perché il mandato chiedeva metà della proprietà. `D-309` fissa che il
pool è **commutativo** fra colpi; `D-312` fissa che `Deflect` assorbe **prima** di `Guard`. Un oracolo che
verifichi solo l'invarianza rispetto all'ordine dei colpi resta verde se qualcuno inverte l'ordine dei due
pool. I due assi vanno pinnati entrambi, ed entrambi lo sono.

---

## 5. Gli oracoli visuali

Nove gate di `#2617` e uno di `#2619` giudicano ciò che il giocatore **vede**. Formulazioni come «si vede
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

Il repository ha già la soluzione: le **sedute** di [`editor-sessions.yaml`](editor-sessions.yaml), dove `U46`
raccoglie «i quattro residui del gate `G9`, in una sola apertura». PIA usa quelle, non ne inventa altre.

| Voci | Seduta esistente |
|---|---|
| `PIE-V01-HUD`, `PIE-V01-INTENT`, `PIE-V01-LOG`, `PIE-V01-DEBUG` | `U15` |
| `PIE-V01-POINTER` | `U43` |
| i quattro residui di `G9` | `U46` |
| `PIE-ICON-01`, leggibilità della scena | `U9` |
| `PIE-VIS-DEFLECT`, `PIE-VIS-INTERPOSE`, `PIE-V01-GHOSTS` | 🔴 **nessuna**, da cui `#2622` |

Solo `#2616` è realmente parallelizzabile: è interamente headless.

---

## 7. Sequenza

```text
PIA-0 #2615  (baseline — prerequisito di tutti)
   |
   +--> PIA-1 #2616  headless, non contende l'Editor ----------+
   |                                                           |
   +--> PIA-2 #2617  ---+                                      |
   |                    |  contendono l'Editor:                +--> PIA-5 #2620
   +--> PIA-3 #2618  ---+  si eseguono in sedute               |
   |                    |                                      |
   +--> PIA-4 #2619  ---+                                      |
                                                               v
                                                        PIA-6 #2621
```

---

## 8. Regole di verdetto

* `PASS` **solo dopo esecuzione reale**.
* Feature implementata **non equivale a** test `PASS`.
* Issue chiusa **non equivale a** gate `PASS`.
* `BLOCKED` deve **nominare** l'owner che blocca.
* `NOT RUN` è meglio di un verde dedotto.
* Nessun «sembra funzionare».

Un `NOT RUN` residuo alla chiusura è accettabile solo in due forme: `NOT RUN — motivo`, oppure
`OUT OF PIA — owner/release`. Il silenzio non è una terza forma.

---

## 9. Priorità

**P0** — T8 e showcase completo · Deflect discriminante · `HazardDamage` end-to-end · combat feedback che rende
giudicabili Deflect e Interpose · HUD e interazione essenziali · privacy degli intenti · leggibilità della scena
(E21, seduta `U9`) · determinismo · packaged smoke · **multilayer smoke**.

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
`G-n` corrispondente** di [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) §3.
