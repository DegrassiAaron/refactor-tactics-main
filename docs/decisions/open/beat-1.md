# `BEAT-1` — Quale meccanismo tiene il ritmo osservato indipendente dal tempo di risposta altrui?

> `OPEN` · **Stato**: aperta · **Ultimo aggiornamento**: 2026-09-20 (aperta, e scorporata alla nascita: il corpo ha superato la soglia) · **Owner**: `PDR-00`
> **Corpo scorporato** da [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) il 2026-09-20: la riga di tabella resta lì come **puntatore**, l'istruttoria vive qui. È la prima voce che la soglia del §«La soglia» intercetta da quando è stata dichiarata — e l'ha intercettata **alla nascita**, non dopo una crescita.
> ⛔ **Non è una decisione presa**, ed è la cartella a dirlo: `open/` tiene ciò che aspetta ancora qualcuno. Quando si chiude, la voce diventa una `D-0xx` nel [Decision Log](../RT_PDR_00_Decision_Log.md).
> **Origine**: la rimisurazione di [#759](https://github.com/DegrassiAaron/refactor-tactics-main/issues/759), misurata su `main` `a5b119d5`. **Owner documentale**: [`adr-0004-finestre-di-reazione.md`](../adr-0004-finestre-di-reazione.md) §7-bis ([`D-021`](../RT_PDR_00_Decision_Log.md)).

## La domanda

**Quale meccanismo tiene il ritmo osservato indipendente dal tempo di risposta altrui, e con quale tolleranza numerica — sapendo che la sospensione di finestra, già prescritta, è il canale che quel meccanismo deve neutralizzare?**

## Perché non si deduce

🔴 **§7-bis elenca tre alternative e non ne sceglie nessuna**: *«Buffering, pacing, *fixed resolution beat* o meccanismo equivalente»*. Un requisito con tre risposte accettabili è un requisito che ogni implementazione futura può dichiarare soddisfatto scegliendo la più comoda — e la DoD di #759 chiede infatti *«il meccanismo è dichiarato e implementato […] come scelto, non come elenco di alternative»*.

`git grep -rni "fixed resolution beat" origin/main` trova la formula in **quattro** righe: §7-bis, la riga di `D-021`, e due copie d'archivio dello stesso elenco (`docs/archive/src/`). Nessuna la sceglie, e in `Source/` non compare: `git grep -rni "fixed resolution beat" origin/main -- Source/` → nessuna riga.

🔴 **E la tolleranza non è un numero da nessuna parte**: la DoD la chiede esplicitamente (*«un numero scritto, non "impercettibile"»*). Senza quel numero la voce 4 della DoD — *«le due timeline devono essere indistinguibili entro la tolleranza dichiarata»* — non è scrivibile come asserzione: manca il termine di paragone.

## 🔑 Ciò che rende la voce urgente invece che teorica: il canale esiste già, ed è prescritto

Il meccanismo che produce la pausa vietata **esiste**, ed è **dovuto** da decisioni accettate.

Durante una finestra aperta il playback **si ferma**, e la sospensione è esplicita:

```cpp
// ARTTurnManager, nel tick del playback
if (bPlaybackHeldByWindow)
{
    return;                                    // ← esce PRIMA di applicare la velocità
}
const float Dt = DeltaSeconds * URTPlaybackLibrary::EffectivePlaybackSpeed(ViewerPlaybackSpeed);
```

⚠️ **Questa è una precisione che costa, e va tenuta**: non è *«la resolution rallenta»*, è **si arresta** — per esattamente il tempo che il difensore impiega a rispondere. È ancora più letteralmente ciò che §7-bis vieta, che parla proprio di *«pausa variabile correlata alla scelta privata»*. Il flag è alzato in `FinishPlayback` dentro `if (IsResolutionSuspended())` e azzerato in `BeginPlayback`, ed è tenuto **separato** da `bPlaybackPaused` di proposito — *«due pause con lo stesso flag si annullerebbero a vicenda al primo `ResumePlayback`»*.

Accanto, e distinta, c'è la **slow-motion**: `ARTTurnManager::UpdateReactionSlowMotion` allinea `ViewerPlaybackSpeed` alla presenza di una finestra, scrivendo `ReactionWindowPlaybackSpeed` mentre attende e `1.f` quando non attende più. ⚠️ **Il suo valore è una manopola di pacing, non una costante di regola**: il docstring del campo lo dichiara — sta accanto a `PhaseBeatSeconds` e `AttackShowSeconds`, e il default è *«un punto di partenza dichiarato, non misurato»*. Pin: `RefactorTactics.Reactions.SlowMotionRestoresOnBothExits`.

⛔ **Nessuno dei due è un difetto sfuggito.** [`D-355`](../RT_PDR_00_Decision_Log.md) stabilisce che la finestra umana si apra **durante** il playback; [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) dichiara la slow-motion *«sola presentazione»*, e [`D-350`](../RT_PDR_00_Decision_Log.md) ne registra la seconda voce come **implementata**; `D-350` stessa toglie al giocatore il controllo manuale della velocità proprio dove una finestra può aprirsi — e il codice la rispetta, tornando a `1.f` invece che a una preferenza. Le tre decisioni sono coerenti fra loro e incompatibili con §7-bis **nel momento in cui esiste un secondo osservatore**. Nessun documento le metteva nella stessa frase.

✅ **Oggi non è un leak, e la misura è questa**: nessuno di questi stati raggiunge un avversario perché **niente** è replicato — `grep -rn "DOREPLIFETIME" Source/ | grep -v /Tests/` non restituisce righe. Il canale è **programmato, non aperto**: si aprirà con la prima superficie di rete, e chi la scriverà lo troverà già lì. È precisamente il caso che §7-bis aveva previsto — *«si documenta l'architettura e si apre la issue»* — con la differenza che l'architettura da documentare ha ora **un nome**.

## Le uscite, e i loro costi

**(a) Lo stato di playback diventa per-osservatore**, e l'avversario non riceve né l'arresto né il rallentamento.
⛔ **Da sola non basta, ed è il difetto della prima stesura di questa voce**: rendere `ViewerPlaybackSpeed` per-osservatore lascia intatto `bPlaybackHeldByWindow`, che è la pausa **vera** e agisce prima. Vanno per-osservatore entrambi, o non si è chiuso niente.
⚠️ E allora i due schermi mostrano la stessa resolution a ritmi diversi: va deciso cosa significhi per un replay che deve riprodurli entrambi.

**(b) Fixed resolution beat**: il ritmo è una costante di formato e la finestra vive dentro il beat, mai oltre.
⛔ Vincola la durata massima della finestra al beat, cioè tocca `CP 14.5`/`CP 14.6` e il campione `p50`/`p90` di [`D-351`](../RT_PDR_00_Decision_Log.md).

**(c) Buffering**: l'avversario osserva con un ritardo fisso ≥ timeout della finestra.
⛔ Il costo è quel ritardo su **ogni** resolution, anche quando nessuna finestra si apre, ed è il prezzo che la tolleranza numerica deve pesare.

## Cosa questa voce non è

⚠️ **Da non confondere con la porzione che resta di M10**: latenza reale, riconnessione e «client lento» come entità di rete non hanno oggi un soggetto e restano `post-v0.1`. Questa voce riguarda la **forma** del meccanismo, che è decidibile ora.

⛔ **E non autorizza a spostare #759 in v0.1**: [#2462](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2462) lo esclude in tre punti del proprio corpo — *«owner separato, non spostare»*, e due volte *«non assorbita»*. Una sua quarta menzione di #759 è una casella di DoD che assegna l'onere altrove senza vietare lo spostamento: va letta per quello che dice.

**Innesco**: la prima issue che implementi una superficie di rete, oppure la prima modifica a `UpdateReactionSlowMotion` o a `bPlaybackHeldByWindow`.
