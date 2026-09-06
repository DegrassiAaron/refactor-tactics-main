# Guida di seduta — chiusura `#79` · rimisura di `PIE-V01-COLL` (c), turno 4 e `PIE-V01-LOG`

> `CURRENT` · **Creata**: 2026-09-06 · **Voci**: `PIE-V01-COLL` clausola (c) · `PIE-V01-LOG` ·
> **Issue**: [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79) — `CP 11.3` ·
> **Wave RT3**: [`issue-79-combat-log-blocked-move/1`](../../rt-three-terminals/waves/issue-79-combat-log-blocked-move/)
> **Owner degli esiti**: [`test-manuali-pie.md`](../test-manuali-pie.md) — questa guida dice **come**
> osservare, non **cosa è risultato**.
>
> ⛔ **Non sostituisce [`guida-seduta-u14-collisione-stallo-e-denial.md`](guida-seduta-u14-collisione-stallo-e-denial.md)**:
> quella descrive l'allestimento e le tre clausole, e resta valida parola per parola. Questa dice cosa è
> **cambiato sotto** dal 2026-09-04, e convoca una **rimisura** — non una prima misura.

## Perché una seduta di rimisura

Il 2026-09-04 la seduta `U14` ha misurato `PIE-V01-COLL`: due clausole ✅, la terza ❌. Il reperto, alla
lettera:

> al turno 4, con il varco occupato, il combat log dice *«Gadget: resta (q=-1,r=0,L=0) (Action.Move, p50)»*
> — **la riga identica al turno 3**, quando Gadget non aveva dichiarato niente.

La guida `U14` §2.3 prescriveva già cosa farne: *«se cade, il difetto non è di questa voce: è #79 —
registralo qui e rimandalo lì»*. È stato fatto. #79 ha risposto, e questa seduta chiude il giro.

## 🔴 Il passo che viene prima di tutto: ricompilare

**Misurato il 2026-09-06, e senza questo passo la seduta misura il codice sbagliato.** La `DLL`
dell'Editor era delle `09:48`; i sorgenti del fix delle `15:47`. Aprire PIE senza ricompilare avrebbe
misurato il codice **precedente** al fix — cioè avrebbe **riprodotto il difetto**, leggibile come «il fix
non funziona».

```powershell
& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development `
    -project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -waitmutex
```

Cercare `Result: Succeeded`. ⚠️ **Con l'Editor chiuso**, e prima di aprirlo.

Poi verificare di essere sul commit giusto: il fix vive dal **`3fff65a6`** in poi, sul branch
`fix/79-blocked-move-turnlog`. Su `main` non c'è ancora.

## Banco A — `Movement.CollisionChoke`: la clausola (c) e il turno 4

Allestimento, tendina e procedura: **identici** a
[`guida-seduta-u14…`](guida-seduta-u14-collisione-stallo-e-denial.md) §1. Non si costruisce niente, si
sceglie `Movement.CollisionChoke` da `Scenario To Run` e si preme Play.

⚠️ Alza `ScenarioTurnPauseSeconds` (default `1.5`) — quattro turni scorrono in fretta e qui si deve
**leggere** — e **annota a quale valore**: un verdetto preso a velocità diversa è un verdetto su un'altra
cosa.

### Cosa deve essere cambiato

Un solo Play copre **due** voci, perché sono la stessa osservazione. Guarda il combat log ai turni **3** e
**4**, che sono il controllo e il caso:

| Turno | Situazione | Riga attesa |
|---|---|---|
| **T3** | `A1` non dichiara niente | `Gadget: resta (q=-1,r=0,L=0) (Action.Move, p50)` |
| **T4** | `A1` prova il varco, `B1` lo occupa **stando fermo** | `Gadget: fermo: cella occupata (q=-1,r=0,L=0) (Action.Move, p50)` |

- ✅ **passa** se le due righe sono **diverse**, e quella del T4 dice che il passaggio è stato negato;
- ❌ **cade** se sono di nuovo **la stessa stringa** — è il difetto del 2026-09-04, non chiuso.

🔑 **La domanda non è «c'è una riga diversa»: è se chi legge capisce che ha tentato ed è stato negato.**
La clausola (c) ammette tre vie — *«al click, al commit o nel log»* — e questa wave ha lavorato **sul log**.
Se la riga c'è ma non si capisce, è ❌ con la ragione scritta, non ✅ con una riserva.

⚠️ **Guarda il combat log a schermo, non l'Output Log.** In headless la riga è già stata misurata
conforme il 2026-09-06 (`OBSERVED`, evidenza in
[`evidence/collisionchoke-turnlog-b5badb79.log`](../../rt-three-terminals/waves/issue-79-combat-log-blocked-move/evidence/)).
Ciò che **nessun oracolo ha dato** è che quella riga arrivi al **widget** che il giocatore guarda: è l'unica
cosa che questa seduta aggiunge, ed è il motivo per cui `COMBAT LOG` è rimasto `OBSERVED` invece di `PASS`.

### Le altre due clausole di `PIE-V01-COLL`

(a) e (b) erano ✅ e **non si rimisurano**: il fix non tocca la contesa. ⚠️ Se però il conteggio delle righe
`fermo: cella contesa` ai T1-T2 fosse cambiato — devono restare **4**, due unità × due turni — è una
**regressione** e va registrata, non ignorata.

## Banco B — `PIE-V01-LOG`: che il log si legga

Banco **diverso**, e non è pignoleria: `editor-sessions.yaml` lo dichiara misurato —

> ⚠️ La partita normale su `L_HexArena` **NON** serve a questa voce: dodici turni senza un solo fallback né
> una modifica ambientale. È un fatto sulla mappa, non sul codice.

Procedura: `Scenario To Run` → **`Visual.Environment.Acceptance`** → Play. Cinque turni con i cinque
fenomeni di superficie, fra cui una **carica rifiutata dal rough**, che è il fallback che la voce chiede.
Si legge in `Window → Output Log`, filtro `LogRT`.

**La domanda della voce**, che non è cambiata: *chi apre quel log **senza sapere cosa cercare** capisce
perché un'azione è stata sostituita?*

➕ **Cosa aggiunge #79 a questa voce.** Un fallback in più da leggere — «percorso bloccato → fermo» — che è
il **primo dei due esempi** che la DoD di `CP 11.3` nomina e che fino a ieri non esisteva. Se hai già fatto
il Banco A, hai già visto quella riga: dichiara se **da sola**, senza il contesto della seduta, si capisce.

⛔ **`CollisionChoke` non sostituisce `Visual.Environment.Acceptance`.** Copre un fallback su cinque; la
voce chiede la leggibilità del log **in generale**. Usare il primo per chiudere la seconda darebbe un ✅ su
un campione di uno.

## Cosa NON rifare

Il gioco non si rompe, ed è già provato altrove:

- `Movement.CollisionChoke` gira **PASS 6/6**, con `BlockedByUnit = 1` — l'assertion che il 2026-09-04
  dava `0`;
- sei Automation Test coprono i casi A/B/C, il confine del budget e il terzo produttore;
- la superficie di replica è invariata (delta `0`).

⛔ Questa seduta **non** verifica che il diniego funzioni: verifica che il giocatore **se ne accorga**. Se ti
trovi a controllare che l'unità resti ferma, stai rifacendo un test che è già verde.

## Se vuoi provare «al click»

Non è richiesto da nessuna delle due voci, ed è la parte che nessuno ha ancora guardato: la clausola (c)
ammette il segnale **al click**, e oggi non esiste.

⚠️ **Prima di concluderne qualcosa, sappi cosa ho misurato nel codice**: un click che colpisce la **mesh**
di un'unità va a `OnClickUnit` — carica, attacco, o niente — e **non** produce un waypoint di movimento.
Solo un click che prende il **terreno** della cella occupata arriva a `HandleClickOnCell` e genera il
diniego. Quindi un «non succede niente» al click sul nemico **non è** il difetto di #79: è la porta
d'ingresso che non passa di lì.

Se lo provi, registralo come **osservazione separata** — non come esito di (c), che si giudica sul log.

## Come registrare l'esito

Gli esiti vanno in [`test-manuali-pie.md`](../test-manuali-pie.md), colonna **Stato** della voce —
**non qui**, e non in `editor-sessions.yaml` (regola `R-6`).

Per ciascuna voce: ✅ o ❌, la data, e per un ❌ **cosa hai visto**, non cosa manca.

| Campo | Perché |
|---|---|
| commit misurato | il fix vive da `3fff65a6`; un esito senza SHA non è riverificabile |
| `Result: Succeeded` della build | senza, l'esito può essere del codice precedente al fix |
| `ScenarioTurnPauseSeconds` usato | un verdetto a velocità diversa è un verdetto su un'altra cosa |
| le due righe T3 / T4, **verbatim** | sono il confronto: la loro identità era il difetto |
| (c) e turno 4 **separate**, se le giudichi separate | un verdetto aggregato non dice quale metà ha ceduto |

⚠️ Se apri anche `PIE-V01-COLL` (a) e (b) per controllo, dillo: sono già ✅ e una loro seconda misura è
informazione, non un requisito.

## Dopo

Con (c) ✅, `PIE-V01-COLL` esce dal 🟡 in cui è dal 2026-09-04 e la voce `#79` perde il suo ultimo residuo:
il DoD di codice era già dichiarato completo da [#419](https://github.com/DegrassiAaron/refactor-tactics-main/pull/419),
e restava **solo** la verifica PIE.

⚠️ Resta aperto, e **non** blocca la chiusura di #79:
[#2627](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2627) — la riga dice *che* il
passaggio è negato, non **quale**: la destinazione richiesta è nel TurnLog ma non nel testo reso. Se durante
la seduta ti accorgi che ti manca quell'informazione, è il posto dove dirlo.
