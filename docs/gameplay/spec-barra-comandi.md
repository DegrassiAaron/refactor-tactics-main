# Spec — La barra dei comandi: undici voci, e che cosa ciascuna lascia fare al movimento

> `CURRENT` · **Owner** della composizione della barra e della compatibilità azione↔movimento **come
> assegnazione**. **Data**: 2026-09-13.
> **Decisioni**: [`D-407`](../decisions/RT_PDR_00_Decision_Log.md) (composizione e slot) ·
> [`D-409`](../decisions/RT_PDR_00_Decision_Log.md) (`Evasion`) ·
> [`D-410`](../decisions/RT_PDR_00_Decision_Log.md) (`Reflect`) ·
> [`D-070`](../decisions/RT_PDR_00_Decision_Log.md) (Overwatch → `Withdraw`).
> **Provenienza**: specifica consolidata della skill bar del 2026-09-13, consumata dallo spec panel in
> [`../roadmap/plans/skill-bar-consolidamento-spec-panel-2026-09-13.md`](../roadmap/plans/skill-bar-consolidamento-spec-panel-2026-09-13.md).
>
> ⛔ **Questa pagina non decide il MODELLO della compatibilità: lo consuma.** Il modello è la **soglia**
> `Stability` / `MinStability` di [`spec-compatibilita-azioni-movimento.md`](spec-compatibilita-azioni-movimento.md)
> ([`D-116`](../decisions/RT_PDR_00_Decision_Log.md)), che ha **misurato e respinto** la matrice per azione —
> 124 celle contro 35 numeri. La tabella di §3 è l'**assegnazione** dei valori a quel modello, non una
> seconda verità sullo stesso vincolo.
>
> ⚠️ **Nessuna riga di codice esprime oggi la soglia**: `FRTMovementProfile::Stability` esiste
> (`#653`), `FRTActionDef::MinStability` **no**, e il confronto è [#606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/606).

## 1. Gli undici comandi

| Comando | Slot | Regola |
|---|---|---|
| **Attacco base** | principale | Uno specifico del personaggio — vedi [`spec-attacco-base-per-eroe.md`](spec-attacco-base-per-eroe.md) |
| **Guardia** | principale | Difesa comune a tutti, settore di 120° orientabile |
| **Difesa caratteristica** | `SignatureDefense` | L'abilità difensiva **del personaggio**: `Evasion`, `Reflect` o altra |
| **Overwatch** | principale | Sorveglianza che usa l'attacco base |
| **Move** | movimento | Selezione della modalità e del percorso |
| **Interact** | principale | Interazione con un oggetto |
| **Wait** | — | Nessuna azione principale e nessuno spostamento volontario |
| **Quattro skill** | principale *(in generale)* | Kit prestabilito del personaggio nel prototipo |

Sono **undici elementi logici**. ⛔ **Non è un layout**: questa pagina non prescrive la disposizione grafica
della HUD, e chi la legge come specifica di UI la legge male.

### 1.1 🔴 Lo slot si chiama `SignatureDefense`, non `Brace`

La sorgente d'autore chiama `Brace` lo slot della difesa caratteristica, e dichiara che *«non è una
riduzione di danno universale»*. **Nel repository `Action.Brace` è esattamente quello**: una delle sette
generiche di [`D-025`](../decisions/RT_PDR_00_Decision_Log.md), che applica `Braced` (−10 a ogni colpo, da
**ogni** lato) e `Root` (`Source/RefactorTactics/Ability/RTCatalogLibrary.cpp:1523`).

Due entità con lo stesso nome, di cui una è il contrario dell'altra, si pagano a ogni lettura del TurnLog.
Stesso criterio di [`D-082`](../decisions/RT_PDR_00_Decision_Log.md) e
[`D-230`](../decisions/RT_PDR_00_Decision_Log.md): il nuovo prende un nome libero, il vecchio resta ciò che è.

⛔ **E `Root` è la ragione per cui i due non potevano coincidere comunque**: `Action.Brace` blocca il
movimento volontario, mentre la difesa caratteristica **consente il `Withdraw`**. Chi si irrigidisce non
ripiega.

### 1.2 Il roster attuale non soddisfa ancora lo slot

Misurato: le difese d'eroe esistenti sono **eterogenee**. `Hero.Ivrin.PhaseGuard` e `Hero.Muiren.TideGuard`
derivano da `Action.Shield` (principale, Prep); `Hero.Ivrin.Deflection` e `Hero.Aevik.ReactiveCapacitor` sono
**reazioni**. Nessuna è una `Evasion` né un `Reflect`.

⚠️ Allineare il roster è lavoro di **kit**, non di regola: questa pagina definisce lo slot, non lo riempie.

## 2. L'azione principale

`Attacco base`, `Guardia`, l'abilità della difesa caratteristica, `Overwatch` e `Interact` occupano l'azione
principale. Le quattro skill del kit **in generale** la occupano; un'eccezione si dichiara **nella singola
skill**, mai nella regola generale — è il pattern di
[`D-014`](../decisions/RT_PDR_00_Decision_Log.md)/[`D-028`](../decisions/RT_PDR_00_Decision_Log.md).

**Si sceglie una sola azione principale per turno.**

## 3. Che cosa ciascuna azione lascia fare al movimento

| Azione scelta | Movimento successivo consentito | Come si esprime nel modello a soglia |
|---|---|---|
| Attacco base | `Move` oppure `Sneak` | `MinStability` **1** |
| Guardia | `Move` oppure `Sneak` | `MinStability` **1** |
| Difesa caratteristica (`Evasion`) | spostamento in **Dash**, poi eventuale `Withdraw` in Move | **riserva** lo slot a `Withdraw` |
| Difesa caratteristica (`Reflect`) | `Withdraw` | **riserva** lo slot a `Withdraw` |
| Overwatch | `Withdraw` | **riserva** lo slot a `Withdraw` ([`D-070`](../decisions/RT_PDR_00_Decision_Log.md)) |
| Interact | anche `Sprint`; prima o dopo il percorso | `MinStability` **0** |
| Solo movimento | `Move`, `Sneak`, `Sprint` | nessun vincolo |
| Wait | nessuno; orientamento finale consentito | — |
| Skill del kit | dichiarato **dalla skill**, eventualmente nessuno | `MinStability` proprio |

### 3.1 🔑 Due meccanismi diversi, e non vanno confusi

La colonna di destra usa **due** strumenti, e la differenza è la parte che conta.

**La soglia** risponde a *«quanto in fretta posso muovermi avendo scelto questa azione?»*: un'azione dichiara
il `MinStability` che le serve, un profilo dichiara la `Stability` che concede, e
`legale ⇔ Profilo.Stability >= Azione.MinStability`. Nega uno **spettro** di profili, dal più veloce in giù.

**La riserva** risponde a *«questa azione mi ha già impegnato il movimento?»*: l'azione nomina **un** profilo,
e lo slot è suo. Non è una soglia stretta al massimo — è un'altra domanda. Il divieto di `Dash` per chi arma
l'Overwatch **non è una regola a sé**: lo slot è già impegnato, quindi è una *conseguenza*
([`D-070`](../decisions/RT_PDR_00_Decision_Log.md)).

⚠️ **`Withdraw` non ha una `Stability` nella tabella di `D-116`**, che ne dichiara quattro — fermo `3`,
`Sneak` `2`, `Move` `1`, `Sprint` `0`. Il catalogo gli dà oggi `1` **come segnaposto dichiarato**
(`Source/RefactorTactics/Ability/RTMovementProfileLibrary.cpp:70`), ed è taratura di
[#606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/606).

### 3.2 Chi riserva lo slot è un DATO, non un `if`

La riserva si dichiara sull'azione, non si deduce dall'`ActionId`. È il criterio che rende esprimibile
`Evasion` e `Reflect` senza toccare il controller: un kit che dichiarasse un'altra azione *«questa ti inchioda
a un ripiegamento»* è coperto dallo stesso campo.

⛔ **Da non confondere con `Slot`**: quello dice **quale** slot l'azione consuma, la riserva dice a quale
profilo l'azione **costringe** uno slot che non consuma.

➕ Il campo è in arrivo con [#1410](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1410),
che lo introduce per il solo `Overwatch`. Questa pagina ne dichiara il **secondo e terzo utente**.

## 4. Che cosa la tabella NON dice

- ⛔ **Non è una matrice per azione.** [`spec-compatibilita-azioni-movimento.md`](spec-compatibilita-azioni-movimento.md)
  §3.1 si intitola *«Perché non la matrice per azione»* e la respinge con una misura: 31 azioni × 4 profili
  = **124** celle da compilare e bilanciare, contro **35** numeri con la soglia. Le righe qui sopra sono
  **valori assegnati** a quel modello: se un domani una riga non fosse esprimibile con una soglia o una
  riserva, è la riga a essere sbagliata, non il modello.
- ⛔ **Non decide i quattro valori di `Stability`.** Sono *«la parte da playtestare, non la parte da
  decidere»*, e appartengono a [#606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/606).
- ⛔ **Non introduce eccezioni per eroe.** Sono `AE-7` in [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md), e
  il pattern è fissato: un'eccezione si dichiara **nel kit**, mai nella regola generale.

## 5. Wait

`Wait` rinuncia all'azione principale **e** allo spostamento volontario. Consente l'orientamento finale.

⛔ **Non concede nulla di proprio**: cooldown e durate procedono normalmente. Non dà armatura, precisione,
furtività né reazione gratis — `Actions.Wait.AllowsFacingAndReaction` verifica che **conservi** ciò che
aveva, non che guadagni. È la chiusura di `AE-6` ([`D-329`](../decisions/RT_PDR_00_Decision_Log.md)).

✅ È il **default alla scadenza** della pianificazione senza comandi.

## Rapporto con gli altri documenti

| Documento | Rapporto |
|---|---|
| [`spec-compatibilita-azioni-movimento.md`](spec-compatibilita-azioni-movimento.md) | **Prevale sul modello**: soglia e non matrice. Questa pagina ne è l'assegnazione |
| [`spec-economia-del-turno.md`](spec-economia-del-turno.md) | Possiede gli slot, i budget e il momento in cui si pagano |
| [`spec-tassonomia-movimento.md`](spec-tassonomia-movimento.md) | Possiede le famiglie del movimento e i micro-step |
| [`spec-attacco-base-per-eroe.md`](spec-attacco-base-per-eroe.md) | Possiede la semantica dell'attacco base |
| [`../balance/RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) | Possiede i **numeri** entro queste regole |
