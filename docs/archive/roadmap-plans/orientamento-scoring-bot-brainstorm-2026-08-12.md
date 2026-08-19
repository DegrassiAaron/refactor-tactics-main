# Brief — l'orientamento nel punteggio del bot (CP 13.5, residuo di [#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160))

> `BRIEF` · **Data**: 2026-08-12 · **Base misurata**: `main` @ `81c034d4`
> **Cosa è**: l'esito di una sessione di discovery. Fissa il *problema* e le sue misure, non l'implementazione.
> **Cosa non è**: un owner documentale. L'owner del comportamento del bot resta
> [`spec-bot-hex.md`](../../gameplay/spec-bot-hex.md); la decisione sul facing è
> [ADR-0005](../../decisions/adr-0005-orientamento.md).

`roadmap-v0.1.md` §CP 13.5 chiede: *«il bot valuta da dove è visto e da dove può essere colpito:
l'orientamento entra nel punteggio delle candidate»*, con il test `Bot.ConsidersExposedRearArc`. La voce
mancava dal corpo della issue ed è stata reintegrata il 2026-08-12; il codice non esiste.

---

## 1. Le quattro misure che cambiano il problema

Tutte su `main` @ `81c034d4`. Sono la ragione per cui il lavoro non è quello che il titolo suggerisce.

| Misura | Comando | Conseguenza |
|---|---|---|
| Il bot **non conosce** l'orientamento | `grep -niE "facing\|orientation\|arc" Source/RefactorTactics/Bot/` → **0** | Né il proprio né quello altrui: `FRTHexBotContext` non ha il campo |
| La direzionalità annulla **due** protezioni, non una | `IsInFrontalArc` in `RTHexCombatLibrary.cpp:214` (copertura) e `RTTurnManager.cpp:3930` (Guard) | Vale per chiunque stia dietro una copertura, non solo per chi è in Guard |
| Il bot **non modella nessuna delle due** | `grep -nE "Cover" Bot/RTHexBotLibrary.cpp` → **0**; il commento «la copertura protegge» in `ScorePlan` parla della **LOS binaria** | Non ha sconti da perdere: lato difensivo non c'è niente da annullare |
| La stima del danno è **lorda** | `Plan.AttackDamage = Context.AttackDamage` (`RTHexBotLibrary.cpp:190`) | Nessuna riduzione, di nessun tipo, entra nel punteggio |

**Quanto vale la posta**: `LowCoverDamageReduction = 10`, `GuardFirstHitReduction = 15`, contro un attacco
base da 20–30. Non è una rifinitura: è fino al 50% del colpo.

⚠️ **La conseguenza da tenere presente**, perché è controintuitiva: *oggi, per il bot, essere colpito di
spalle non cambia niente*. `RearHitBypassedCover` **annulla una riduzione**, e chi non ha né copertura né
Guard non ha nulla da annullare.

---

## 2. La strada scelta, e quella scartata

**Scelta (2026-08-12, l'autore)**: un **termine direzionale nel punteggio**, alla lettera di CP 13.5, senza
toccare `Plan.AttackDamage`.

**Scartata**: sostituire la stima nominale del danno con quella effettiva (`EffectiveCoverReduction` +
Guard), da cui l'orientamento sarebbe disceso da solo. Più economica in apparenza — nessun termine nuovo, e
`WKill` sarebbe diventato onesto — ma cambia il comportamento del bot **in modo diffuso**, ben oltre il
perimetro di un checkpoint della v0.1. Resta sul tavolo come lavoro proprio: vedi §6.

---

## 3. Il termine si **deriva**, non si tara

È il nodo che ha affondato [#149](https://github.com/DegrassiAaron/refactor-tactics-main/issues/149):
*«non esiste un valore che funzioni»* per `WInRange`, perché il problema era la scala di `WThreat` (100)
rispetto al danno (`WDamage` 10 × 20–30 = 200–300).

**Un peso nuovo e libero ripeterebbe quell'errore.** Il termine direzionale non ne ha bisogno, perché ciò
che l'orientamento cambia è **una quantità di danno già nota**:

```
esposizione = WDamage × (riduzione che la direzione scavalca)
```

Con i valori canonici: `10 × 10 = 100` per una copertura bassa, `10 × 15 = 150` per Guard. Sono **nella
stessa scala** di `WDamage × danno`, cioè della grandezza con cui devono competere, e nessuno dei due numeri
è inventato qui: vengono dal catalogo di combattimento.

> Il criterio, in una riga: **un termine nuovo non porta un numero nuovo**. Se lo porta, sta modellando
> qualcosa che il gioco non misura ancora — ed è il momento di fermarsi.

---

## 4. I due versi non valgono lo stesso

| Verso | Formula | Dato che serve | Effetto **oggi** |
|---|---|---|---|
| **Offensivo** — colpisci chi è scoperto | `Score += WDamage × RiduzioneScavalcata` | facing dei nemici: **pubblico e osservabile** | 🟢 reale contro un umano in copertura o in Guard |
| **Difensivo** — non offrire il fianco | `Score -= WDamage × RiduzionePersa` | facing **proprio**, che è una previsione (§5) | 🟡 **vale 0** finché il bot non ha una protezione da perdere |

**Deciso (2026-08-12, l'autore): il verso difensivo si scrive ora**, con il test che asserisce lo zero. Non è
lavoro sprecato: è un termine che vale zero per una ragione dichiarata e si accende da sé il giorno in cui il
bot pianificherà una copertura o una Guard. Il suo test **asserisce lo zero di oggi** invece di tacere — un
residuo asserito diventa rosso quando cambia, un residuo taciuto sparisce.

⚠️ **Conseguenza da scrivere nel DoD**: alla chiusura di CP 13.5 metà della frase della roadmap sarà
implementata e **inerte**. Chi legge «il bot valuta da dove è visto» deve trovare, accanto, che quel valutare
oggi restituisce zero e perché.

---

## 4-bis. L'ordine del turno decide quando il bonus esiste

Misurato in `RTTurnManager.cpp`: la rotazione da targeting sta a **3163**, il controllo direzionale della
Guard a **3930**. Chi attacca **si gira verso il proprio bersaglio prima** che si valuti l'arco.

> ⚡ **Il bonus posteriore svanisce proprio nel duello.** Se il nemico attacca il bot, ruota verso di lui e il
> bot lo colpisce di fronte. Il bonus sopravvive quando il bersaglio è **impegnato con qualcun altro** — cioè
> nel fuoco incrociato, non nell'1v1.

Ne segue che «girare attorno al nemico» non è la domanda giusta, e che il termine costa meno di quanto
sembri: `WApproach × MinDist` penalizza la **distanza finale**, non il percorso, quindi finire a est o a
ovest dello stesso bersaglio alla stessa distanza vale identico. Il vincolo è il budget di movimento, non il
punteggio.

**Deciso (2026-08-12, l'autore)**: il bot **conta il bonus** usando il facing di inizio turno — lo stesso che
vede il giocatore umano, quindi equo per costruzione — e la sovrastima **si misura invece di stimarla**.

⚠️ **Perché la misura è un requisito e non un extra**: `WKill` vale 10000. Un bonus contato e non incassato
può spostare la scelta del *bersaglio*, non solo della cella, e far preferire un nemico che poi non muore. È
lo stesso ordine di grandezza che rende grave la stima lorda del danno (§6).

**Come si misura** — e qui il brief si era sbagliato, corretto il 2026-08-12 in fase di implementazione:

> ⚠️ **`RearHitBypassedCover` è emesso solo per la GUARD**, non per la copertura. Misurato:
> `git grep RearHitBypassedCover -- Source/` dà due righe, e l'unica che scrive è `RTTurnManager.cpp:3982`,
> dentro il ramo che decide se la guardia tiene. La **copertura** annullata non lascia traccia, perché
> `EffectiveCoverReduction` è una funzione pura senza accesso al TurnLog — e il termine implementato si basa
> proprio sulla copertura.

Conseguenza: **il tasso di realizzo non è oggi misurabile per il caso che il bot valuta.** La decisione
«conta il bonus e misura la sovrastima» resta quindi a metà, e la metà mancante è nominata invece che
sottintesa.

Cosa servirebbe: un campo su `FRTHexAttackHit` che segnali «la direzione ha annullato la copertura», e il
`TurnManager` che lo traduca in una voce — il valore d'enum **esiste già** e si chiama proprio `…Cover`, non
ne va aggiunto uno.

⚠️ **Perché non è stato fatto insieme al resto**: una voce nuova nel TurnLog cambia gli **hash** delle tracce,
e il progetto ha golden replay e test di determinismo che li confrontano. È un cambiamento da fare sapendolo,
con la sua verifica, non di contorno a un termine di punteggio.

---

## 5. Il nodo vero: quale facing avrà il bot

Per il bersaglio il facing è un dato. **Per sé è una previsione**, e il piano non la contiene:

- il bot pianifica **destinazioni, non percorsi** (`PlannedPath.Reset()`, e il commento lo dichiara);
- `FRTHexReachableCell` porta `Cell` e `Cost` — **nessun predecessore**;
- il facing lo deriva il resolver, da regole che il bot non deve riscrivere (`D-098`).

Due strade, e la seconda costa meno di quanto sembri:

1. **`FindPathForUnit` per candidata** — corretto e senza modifiche strutturali, ma è un pathfinding per ogni
   cella raggiungibile, dentro il ciclo di scoring.
2. **Un campo `FromCell` in `FRTHexReachableCell`** — per il facing serve **solo l'ultimo passo**, e la
   ricerca che produce le celle raggiungibili quel predecessore lo conosce già mentre espande. Una riga dove
   si scrive il costo, e il facing si ottiene da `URTFacingLibrary::FacingFromPath` su due celle.

⚠️ **E il caso che le due strade non coprono**: se il piano ha un bersaglio, `D-020` fa ruotare l'unità verso
di esso *prima* di risolvere — il facing d'arrivo non è quello del movimento. La stima deve seguire lo stesso
ordine del resolver, altrimenti il bot valuta un orientamento che non avrà.

**Se la stima resta incerta**, il precedente del repo dice da che parte sbagliare (CP 13.5): *«l'errore va
nella direzione sicura — il bot sottostima le occasioni invece di inventarle»*. Tradotto qui: sul verso
offensivo non contare il bonus se non sei sicuro; sul difensivo conta l'esposizione.

---

## 6. Cosa NON entra, e perché va detto

- **La stima effettiva del danno** (§2). Oggi il bot crede di uccidere un bersaglio che la copertura salva
  per 10 punti, e `WKill` vale 10000: è un falso positivo da tre ordini di grandezza, molto più grave del
  problema che questo brief risolve. Merita una issue propria, non una riga in questa.
- **Insegnare al bot la copertura sulla cella di destinazione**, che è il prerequisito del verso difensivo.
- **Il bot che pianifica Guard di proposito**: oggi usa azioni difensive solo sotto metà HP e solo se
  «rimettono in piedi».

---

## 7. Test proposti

| Test | Cosa pinna |
|---|---|
| `Bot.ConsidersExposedRearArc` | Stesso bersaglio in copertura, due `facing`: il bot sceglie di colpirlo dal lato scoperto. È il test nominato dalla roadmap |
| `Bot.RearBonusMatchesBypassedReduction` | Il termine vale **esattamente** `WDamage × riduzione`, non un numero scritto a mano: è ciò che impedisce al peso di diventare libero in una PR futura |
| `Bot.ExposureIsZeroWithoutProtection` | Il verso difensivo vale **0** oggi, e la riga lo dice: quando il bot avrà una copertura da perdere, questo test diventa rosso e chiede di essere promosso |
| `Bot.RearBonusSurvivesTargetRotation` | Il caso di §4-bis, asserito nel verso che il gioco produce davvero: un bersaglio che attacca **il bot** ruota verso di lui e il bonus **non** si realizza; uno che attacca un terzo gli resta di spalle e si realizza. Senza, il tasso di realizzo sarebbe un numero che nessun test spiega |
| Scenario `Spec.Bot.*` | Da valutare **dopo**: l'harness ora sa dichiarare unità `bot` e `facing` (PR #615), quindi il caso del fuoco incrociato è esprimibile — ma il verso difensivo che vale zero non è materia da scenario |

**Verifica di mutazione da prevedere**: azzerare il termine direzionale deve far cadere
`ConsidersExposedRearArc` e **solo** quello. Se ne cadono altri, il termine sta spostando decisioni che non
gli competono — e con `WKill` a 10000 nel modello è un rischio concreto, non teorico.

---

## 8. Le tre domande, chiuse il 2026-08-12

| Domanda | Esito |
|---|---|
| Il bot deve preferire il fianco anche se allunga il percorso? | **Riformulata dai dati** (§4-bis): il percorso non costa — `WApproach` guarda la distanza finale. La domanda vera era se contare un bonus che il bersaglio può cancellare ruotando. **Deciso**: si conta, e la sovrastima si misura sul tasso di `RearHitBypassedCover` |
| Il facing è informazione pubblica? | **Sì, il corrente** — `ARTUnit::Facing` è ciò che le regole leggono e che la mesh mostra a fine playback. È l'**intento** di rotazione a essere privato, e ADR-0005 lo pinna: `Facing.IntentIsTeamFiltered`. Il bot legge quindi ciò che vede anche l'umano, senza passare da `FRTTeamKnowledge` per questo dato |
| Il difensivo è v0.1 o slitta? | **Entra in v0.1**, valendo zero, con `Bot.ExposureIsZeroWithoutProtection` a dichiararlo |

### Cosa resta aperto davvero

1. **La soglia oltre cui la sovrastima è inaccettabile.** La misura è decisa (§4-bis), il criterio no: se il
   tasso di realizzo fosse — poniamo — il 40%, il termine va tenuto, dimezzato o tolto? Va fissato **dopo**
   aver visto il numero, non prima: è precisamente l'errore che #149 documenta.
2. **Se la misura mostrasse che il bonus sposta il BERSAGLIO** (non solo la cella), serve decidere se il
   termine debba essere escluso dal confronto fra bersagli — cioè degradato a tie-break — invece che ritarato.
