# Assi strategici fra i turni — audit di `CM-10`, 2026-09-04

> `SNAPSHOT` · **Misurato**: 2026-09-04 · 🔴 **CORRETTO IL 2026-09-04**: due misure di questo file erano **false**, entrambe per lo stesso difetto di perimetro, ed è il difetto che la nota di metodo in fondo già denunciava. Le correzioni sono in linea, con il verso originale conservato · **Base**: `origin/main` · **Owner della domanda**: epic
> [#2276](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2276), finding `CM-10`
> **Cosa è**: la misura di quali assi attraversano davvero il turno, chiesta dal work order della review —
> *«auditare e rendere esplicita l'economia strategica realmente posseduta … evitare di inventarne uno per
> riempire il vuoto»*.
> **Cosa non è**: una proposta. Non aggiunge assi, non tara valori, non apre decisioni.
> **Owner dei fatti**: [`../../gameplay/spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md)
> per i budget del turno, [#610](https://github.com/DegrassiAaron/refactor-tactics-main/issues/610) per la
> risorsa firma, `FRTMatchRules` per la soglia di vittoria.

## La domanda

`CM-10` teme una partita **strategicamente piatta**: turni tatticamente profondi in cui però nulla si porta
avanti, perché *«una buona scelta ora»* e *«una buona scelta pensando a tre turni»* coincidono. La domanda
non è se il gioco abbia profondità tattica — ce l'ha — ma **quali grandezze sopravvivono al Cleanup e sono
oggetto di una scelta**.

## Il criterio

Un asse è **strategico** solo se soddisfa **tutte e tre**:

1. **persiste** oltre il Cleanup;
2. il giocatore lo **spende o lo accumula** con una decisione;
3. la decisione ha **conseguenze** su un turno successivo.

Un contatore che sale da solo soddisfa 1 e non 2. Un budget che si ripristina ogni turno soddisfa 2 e non 1.

## La misura

| Asse | Persiste | Meccanismo misurato | Decisione? |
|---|:---:|---|:---:|
| **Posizione** | ✅ | `FRTCellId` sull'unità | ✅ |
| **Cooldown** | ✅ | `Unit->TickCooldowns()` nel Cleanup (`RTTurnManager.cpp:1898`) | ✅ |
| **Carica di reazione** | ✅ | `bCharged`: `HOLD` la conserva, solo `FIRE` la consuma | ✅ |
| **HP** | ✅ | — | ⚠️ |
| **Energia (risorsa firma)** | ✅ | `GainEnergy(Energy, EnergyPerTurn, MaxEnergy)` nel Cleanup (`:1881`) | 🔴 **no** | <!-- superata: vedi nota in coda -->
| **Punteggio obiettivo** | ✅ | un punto per Cleanup controllato | 🔴 **no** |
| Slot · Movement Point · Pivot | ❌ | ripristinati ogni turno | tattici |
| Status | ~ | durate di **1–2** turni | orizzonte breve |

### I tre assi pieni

**Posizione · Cooldown · Carica di reazione.** Sono gli unici che soddisfano tutti e tre i criteri, ed è su
questi tre che la profondità fra i turni si regge oggi. Non sono pochi — il cooldown da solo produce la
domanda *«spendo `Ram` ora o lo tengo per il turno in cui servirà?»* — ma sono tutti a orizzonte **corto**:
due turni di ricarica, una carica per finestra.

### ⚠️ HP: persiste, ma l'attrition è attenuata per costruzione

`RechargeBaseShield()` gira nel Cleanup (`RTTurnManager.cpp:1885`): lo scudo base di **5** punti
([`D-224`](../../decisions/RT_PDR_00_Decision_Log.md)) **si ricarica ogni turno**. Un colpo da 8 ne
consegna 3; due colpi da 8 in due turni ne consegnano 6, non 11. L'usura di lungo periodo esiste, ma è
**sottratta di 5 per unità per turno** — ed è una scelta, non un difetto: va conosciuta da chi si chiede
perché la partita non converga per attrito.

## 🔴 I due assi che esistono come macchina e non come decisione

### 1. L'energia sale, e nessuno la spende

Il meccanismo è **completo**, non abbozzato:

| Pezzo | Dove |
|---|---|
| il campo | `ARTUnit::Energy` (`Unit/RTUnit.h:223`), `MaxEnergy = 100` |
| la ricarica | `EnergyPerTurn = 25` · `EnergyOnHit = 15`, applicata nel Cleanup (`RTTurnManager.cpp:1881`) |
| il costo | `URTActionData::EnergyCost` (`Ability/RTActionData.h:137`) |
| il controllo | `ARTUnit::CanUseAbility` → `URTCombatLibrary::IsAbilityUsable(Cooldown, Energy, EnergyCost)` |
| il determinismo | l'energia entra in `RTMatchStateHash` |

🔴 **Qui questo file diceva il falso, e la correzione è più interessante dell'errore.** La misura era:

```bash
git grep -n "EnergyCost" -- Source/RefactorTactics/Ability/RTCatalogLibrary.cpp \
                            Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp   # -> 0
```

— e da quello zero si concludeva *«nessuna azione dichiara un costo in energia, `EnergyCost` è `0` ovunque»*.
**Falso**: il costo esiste, e non sta nei cataloghi.

```
RTUnit.cpp:1111   MakeAbility("Ultimate", ..., /*EnergyCost=*/ MaxEnergy, TAG_Status_Slow, 2)
```

`ARTUnit::EnsureDefaultAbilities` dà all'Ultimate degli **archetipi legacy** un costo pieno. Il perimetro del
grep escludeva l'unico file che lo assegna. ⚠️ Lo aveva già misurato
[`D-284`](../../decisions/RT_PDR_00_Decision_Log.md) **cinque giorni prima**, e questo audit non l'ha letta.

✅ **Ciò che resta vero, ed è il fatto che conta**: il consumo esiste su **un percorso solo**. Il roster canonico
prende `Abilities = Hero->Actions`, dove nessuna azione dichiara un costo — quindi **per i quattro eroi spediti**
`IsAbilityUsable` è sempre vera sul lato energia e il contatore sale a +25 per turno fino a 100 senza abilitare
né negare nulla. 🔑 E la differenza fra le due economie — archetipi legacy con un Ultimate a costo, eroi
senza — **non è dichiarata da nessun documento**: è il difetto che `D-284` aveva trovato senza poterlo sciogliere.

⚠️ **E i valori sono quelli dell'MVP quadrato**: il canone di
[`../../balance/RT_HeroCatalog_v0.1.md`](../../balance/RT_HeroCatalog_v0.1.md) §5.1 dichiara una risorsa
**per eroe** con nome proprio — `Carica Conduttiva` · `Riserva Idrica` · `Integrità Strutturale` · `Slancio`
— **cap 4** e **ricarica 1** sul trigger d'affinità. Il codice dice `100 / 25 / 15`.

🔴 **E il modello che quei nomi descrivono è stato SCARTATO**, il che questo audit non sapeva: `AE-4` non
è aperta — è chiusa dal 2026-08-30 da [`D-265`](../../decisions/RT_PDR_00_Decision_Log.md), che decide che **non
esiste una risorsa firma universale** e che l'economia comune è slot, cooldown e drawback. La risorsa per eroe
con cap 4 è esplicitamente fra le *alternative scartate*.

✅ **Deciso il 2026-09-04**: [`D-324`](../../decisions/RT_PDR_00_Decision_Log.md) — **`Energy` esce dal
gameplay**, e l'uscita dal digest avviene nello stesso commit che toglie i campi, esercitando la condizione *(a)*
che [`D-284`](../../decisions/RT_PDR_00_Decision_Log.md) aveva dichiarato in anticipo.
[#610](https://github.com/DegrassiAaron/refactor-tactics-main/issues/610) resta l'**owner
dell'implementazione**. ⚠️ Il suo corpo cita `RTUnit.h:114`: la classe si è spostata in
`Source/RefactorTactics/Unit/`, e oggi le righe sono `220`–`231`.

### ~~2. Il punteggio si accumula, e non chiude la partita~~ — 🔴 **FALSO, ed è l'errore peggiore dei due**

Questo file diceva: *«`FRTMatchRules::ScoreToWin` è **zero**, cioè via disattivata — dichiarato nel commento a
`RTTurnManager.cpp:1943`»*. **Il formato spedito ha la soglia a CINQUE**, e da prima che questo audit
fosse scritto:

```
RTMatchFormatLibrary.cpp:232   Format->ScoreToWin = 5;   ← D-247, 2026-08-30
```

🔑 **Lo zero letto era il default della struct**, non il valore spedito — e non è stato letto nemmeno lì: è
stato **copiato da un commento stantio**. `RTTurnManager.cpp:1943` dice *«oggi ZERO»*, e
[`D-247`](../../decisions/RT_PDR_00_Decision_Log.md) sapeva di quel commento — *«il commento accanto allo zero
prometteva esattamente questo»* — senza aggiornarlo. Un avverbio senza data è sopravvissuto cinque giorni alla
propria smentita.

✅ **Il cinque non è un numero qualunque**: `D-247` lo deriva dalla **geometria** dell'arena — l'obiettivo dista
4 celle da uno spawn e 7 dall'altro, quindi chi parte a ovest ha un punto di vantaggio strutturale, e a cinque
quel punto vale il 20%, *«conta senza decidere»*. Chiude al turno 5-6 nel controllo ininterrotto, lasciando ~7
turni perché la contesa la ribalti.

⚠️ **Negli scenari resta davvero zero, e per un altro motivo**: lo `ScenarioHarness` **non imposta
`FRTMatchRules`** — zero riferimenti in `Source/RefactorTactics/ScenarioHarness/` — quindi vale il default della
struct. La misura di
[`../../../Scenarios/Spec/Objective/HeldAgainstAnAdjacentEnemy.json`](../../../Scenarios/Spec/Objective/HeldAgainstAnAdjacentEnemy.json):
chi tiene il Relay segna a ogni Cleanup **senza fare niente**, e chi gli sta a un passo non toglie nulla.

## Conclusione — 🔴 **riscritta il 2026-09-04, perché reggeva su due misure false**

Questo file concludeva che *«i due assi che darebbero pressione di lungo periodo esistono già e sono spenti»*.
Con le misure corrette **restano quattro assi pieni, non tre, e l'interruttore è uno solo**:

- ✅ **Il punteggio non è spento: è acceso a cinque** dal 2026-08-30 ([`D-247`](../../decisions/RT_PDR_00_Decision_Log.md)),
  con una via di vittoria che chiude prima del `RoundLimit`. È un **quarto asse strategico pieno** — persiste,
  si contende con una decisione, e ha conseguenze su ogni turno successivo — e questo audit lo aveva contato
  fra quelli morti.
- 🔴 **L'energia è l'unico spento, e non va accesa**: [`D-265`](../../decisions/RT_PDR_00_Decision_Log.md) ha
  scartato il modello che la giustificava, e [`D-324`](../../decisions/RT_PDR_00_Decision_Log.md) decide che
  **esce dal gameplay**. Non era un interruttore da premere: era la macchina di una regola che non c'è.

⛔ Il divieto del work order — *«non aggiungere nuovi assi strategici per colmare `CM-10`: prima auditare
quelli esistenti»* — resta rispettato, e la ragione è più forte di quella scritta prima: non solo il vuoto
non c'era, ma **l'asse che si temeva mancasse era già stato acceso da qualcun altro cinque giorni prima**.

⚠️ **Ciò che questo audit non dice**: se la profondità dei tre assi pieni *basti*. Quella è una domanda di
playtest, non di codice, e appartiene alla seduta — non a questo file.

## Nota di metodo

🔴 **Cinque misure sbagliate, sempre nello stesso modo — e due sono sopravvissute alla pubblicazione.**
Le prime tre furono corrette prima di scrivere; le altre due sono state pubblicate e corrette il giorno dopo,
**nonostante questa nota fosse già in fondo al file**. Scrivere la regola non basta a rispettarla: il costo
in energia stava in `RTUnit.cpp` e il grep guardava i due cataloghi; la soglia obiettivo è stata **copiata da
un commento** invece che letta dal formato. 🔑 **La forma più pericolosa non è il grep a zero — è il commento
che risponde alla domanda**: sembra una misura, e nessuno lo rimisura. Le tre originali:
cercare un nome o un percorso e concludere l'assenza. `Risorsa firma` non esiste in codice **col suo nome
canonico** — si chiama `Energy`; `Energy` non è in `Characters/` — la classe vive in `Unit/`. Un grep che
torna zero misura **il termine cercato**, non il fatto. Vale come promemoria per il prossimo audit:
prima di scrivere *«non esiste»*, cercare almeno un sinonimo e un percorso alternativo.

---

## ⛔ Nota di supersessione — 2026-09-04, più tardi nello stesso giorno

> Questo referto ha misurato l'**Energia (risorsa firma)** come uno degli assi accesi, e ha concluso che è
> *«una macchina e non una decisione»*. La conclusione ha avuto un effetto: la stessa sera
> [`D-324`](../../decisions/RT_PDR_00_Decision_Log.md) ha deciso che `Energy` **esce dal gameplay**, e
> [#610](https://github.com/DegrassiAaron/refactor-tactics-main/issues/610) l'ha rimossa dal codice e dal
> digest nello stesso commit.
>
> ∴ **La riga «Energia» di questa tabella non descrive più il runtime.** Il referto resta com'è — è una
> fotografia datata, e riscriverlo cancellerebbe la misura che ha prodotto la decisione — ma chi lo legge
> per sapere *quanti assi strategici il gioco possiede oggi* ne tolga uno: dei due interruttori che `CM-10`
> aveva trovato spenti, questo non è stato acceso, è stato **rimosso**. Resta il punteggio obiettivo
> (`ScoreToWin = 0`).
