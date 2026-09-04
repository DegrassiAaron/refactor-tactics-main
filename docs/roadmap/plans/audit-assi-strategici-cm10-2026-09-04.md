# Assi strategici fra i turni — audit di `CM-10`, 2026-09-04

> `SNAPSHOT` · **Misurato**: 2026-09-04 · **Base**: `origin/main` · **Owner della domanda**: epic
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
| **Energia (risorsa firma)** | ✅ | `GainEnergy(Energy, EnergyPerTurn, MaxEnergy)` nel Cleanup (`:1881`) | 🔴 **no** |
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

E il carburante manca:

```bash
git grep -n "EnergyCost" -- Source/RefactorTactics/Ability/RTCatalogLibrary.cpp \
                            Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp   # -> 0
```

> **Nessuna azione dei cataloghi dichiara un costo in energia.** `EnergyCost` è `0` ovunque, quindi
> `IsAbilityUsable` è sempre vera sul lato energia e il contatore sale a **+25 per turno fino a 100** senza
> che niente lo consumi.

⚠️ **E i valori sono quelli dell'MVP quadrato**: il canone di
[`../../balance/RT_HeroCatalog_v0.1.md`](../../balance/RT_HeroCatalog_v0.1.md) §5.1 dichiara una risorsa
**per eroe** con nome proprio — `Carica Conduttiva` · `Riserva Idrica` · `Integrità Strutturale` · `Slancio`
— **cap 4** e **ricarica 1** sul trigger d'affinità. Il codice dice `100 / 25 / 15`.

✅ **Ha già un owner, e non serve aprirne un altro**:
[#610](https://github.com/DegrassiAaron/refactor-tactics-main/issues/610) — *«La risorsa firma ha tre valori
in tre posti, e il costo non è a catalogo»* — che è `AE-4` in
[`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). ⚠️ Il suo corpo cita `RTUnit.h:114`: la classe si è
spostata in `Source/RefactorTactics/Unit/`, e oggi le righe sono `220`–`231`.

### 2. Il punteggio si accumula, e non chiude la partita

`FRTMatchRules::ScoreToWin` è **zero**, cioè *via disattivata* — dichiarato nel commento a
`RTTurnManager.cpp:1943`. Un punto per Cleanup controllato viene contato, entra nel confronto finale al
`RoundLimit`, ma **non produce nessuna pressione**: nessuna soglia avvicina la vittoria mentre la partita è
in corso.

Misurato di conseguenza da
[`../../../Scenarios/Spec/Objective/HeldAgainstAnAdjacentEnemy.json`](../../../Scenarios/Spec/Objective/HeldAgainstAnAdjacentEnemy.json):
chi tiene il Relay segna a ogni Cleanup **senza fare niente**, e chi gli sta a un passo non toglie nulla.

## Conclusione

**`CM-10` non chiede un asse nuovo: i due che darebbero pressione di lungo periodo esistono già e sono
spenti.**

- L'**energia** ha campo, ricarica, costo, controllo e determinismo — e zero costi a catalogo.
- Il **punteggio** ha conteggio e confronto — e soglia zero.

Accendere l'uno o l'altro è una **decisione di design con owner** (#610 per la risorsa; la soglia è un dato
di `FRTMatchRules`), non un vuoto da riempire inventando. ⛔ Il divieto del work order —
*«non aggiungere nuovi assi strategici per colmare `CM-10`: prima auditare quelli esistenti»* — è quindi
rispettato **e** spiegato: l'audit dice che il vuoto non c'è, c'è un interruttore.

⚠️ **Ciò che questo audit non dice**: se la profondità dei tre assi pieni *basti*. Quella è una domanda di
playtest, non di codice, e appartiene alla seduta — non a questo file.

## Nota di metodo

🔴 **Tre misure di questo audit sono state sbagliate prima di essere giuste, sempre nello stesso modo**:
cercare un nome o un percorso e concludere l'assenza. `Risorsa firma` non esiste in codice **col suo nome
canonico** — si chiama `Energy`; `Energy` non è in `Characters/` — la classe vive in `Unit/`. Un grep che
torna zero misura **il termine cercato**, non il fatto. Vale come promemoria per il prossimo audit:
prima di scrivere *«non esiste»*, cercare almeno un sinonimo e un percorso alternativo.
