# #2534 — Panel di specifica sulla rigiudicazione · 2026-09-09

> **Comando**: `/issue-run 2534` → `sc:spec-panel 2534`, modalità **critique**, focus `requirements · testing · compliance`.
> **Base misurata**: `main = 5ab19332`, working tree pulito, allineato a `origin/main` dopo `fetch`.
> **Panel**: Wiegers (requisiti) · Adzic (criteri eseguibili) · Cockburn (attore e goal) · Nygard (modi di fallimento) · Crispin (cosa può giudicare un test, cosa un occhio).
> **Referto precedente sullo stesso perimetro**: [`2697-combat-log-senza-consumatori-spec-panel-2026-09-09.md`](2697-combat-log-senza-consumatori-spec-panel-2026-09-09.md).

## §1 — I fatti, prima delle opinioni

Tutto misurato su `main = 5ab19332`. Nessuna riga di questa sezione viene dal corpo della issue.

| Fatto | Evidenza |
|---|---|
| La scelta fra (a) e (b) **è presa e motivata** | [`D-340`](../../decisions/RT_PDR_00_Decision_Log.md), **Accettata**, decisione d'autore del 2026-09-06 — sceglie la (b) e dichiara ⛔ *«non si alza `RTSightSlabHeight` oltre i 10 cm»*, ⛔ *«se un giorno la lastra dovrà essere leggibile, servirà un `D-nnn` proprio»* |
| La (b) è **implementata nel TurnLog** | `Turn/RTTurnManager_Blast.cpp:1531` scrive `SightBlockerCell` via `SightBlockerForLog`; `Turn/RTTurnLogLibrary.cpp:792` compone la riga |
| Il filtro di conoscenza vive **alla scrittura** | `Turn/RTTurnLogLibrary.cpp:361`; sei casi in `Tests/RTCombatLogTests.cpp:1745-1839`; ciclo RED eseguito e documentato in issue |
| Il formato è **v13** | `Tests/RTTurnLogSerializationTests.cpp:1476` (`+12` byte); campo `Public` in `Replay/RTReplayPrivacyLibrary.cpp:41` |
| Il feed **ha un consumatore** | `UI/RTHudViewModel.cpp:496` chiama `URTPlayerEventProjector::Project` — consegnato da **#2707** (`063ab898`, mergiata `2026-09-09T05:02:04Z`) |
| L'ostacolo è **marcato nel mondo** | `UI/RTHUD.cpp:57` `ComputeBlockerMarks`, chiamata a `:929`; test `RefactorTactics.HUD.BlockerMarksOnlyNameableCells` |
| Il caso avversario è **coperto end-to-end** | `RefactorTactics.UI.PlayerEventLog.BlockedShotDoesNotRevealTheWallToTheUnauthorized` misura entrambe le metà: l'autorizzato riceve la cella, l'altro **zero eventi** |
| Il feed **testuale** non disegna ancora | `WBP_RT_EventLog.uasset` **non esiste** — fetta **F** di [#1936](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1936), confermato da `hud-v01-three-terminals-audit-2026-09-05.md` |
| `PIE-HEXPLAY-6` e `PIE-VIS-SIGHTWALL` | **❌ entrambe**, riconfermate il 2026-09-09, `runId 20260909-055417`, banco `Visual.Map.SightWallIsWalkable`, `PASS (4/4 assertion, 2 turni)` |
| La seduta che le convoca | `U46` in [`editor-sessions.yaml`](../editor-sessions.yaml) — `verifies` le contiene entrambe, `unblocked_by: []` |

∴ **Delle quattro caselle della DoD, due sono soddisfatte e nessuna delle due residue è codice.**

## §2 — I rilievi

### 🔴 F-01 · WIEGERS — la quarta casella non è un criterio di accettazione, è una dipendenza

> *«`PIE-HEXPLAY-6` torna verde, e il subset `RELEASE-V01` torna senza voci fallite»*

Questa voce non è soddisfacibile da nessun lavoro nel perimetro di #2534, e la ragione è **misurata su due lati**:

* la comprensibilità **visiva** dipende dalla via (a), che `D-340` ha escluso per decisione — e la stessa decisione dichiara che riaprirla richiede un `D-nnn` proprio;
* la comprensibilità **testuale** dipende dal feed a schermo, cioè da `WBP_RT_EventLog.uasset`, che è la fetta **F** di #1936 e non esiste.

Un criterio che il proprietario non può soddisfare non misura il suo lavoro: misura quello di qualcun altro. Va **riformulato** in ciò che questa issue controlla — la rigiudicazione avviene, è consuntivata in un posto solo, e se resta ❌ il residuo ha un owner **nominato** — e la dipendenza va scritta dove oggi è implicita.

📌 Il commento del 2026-09-09 04:14 lo dice già a metà (*«non è raggiungibile da sola»*), ma **il corpo della issue non è stato aggiornato**: un lettore che apre la issue trova ancora quattro caselle pari.

### 🔴 F-02 · NYGARD + CRISPIN — la premessa del verdetto è cambiata 47 minuti dopo il verdetto

La cronologia, in UTC, dagli oggetti Git e da GitHub:

| Ora | Evento |
|---|---|
| ~03:54 | verdetto d'autore sul banco: *«non si capisce perché non parte, non ci sono riferimenti video, solo log»* |
| 04:14 | commento su #2534 che registra la dipendenza da #2697 |
| **04:57** | ultimo commit che tocca `Source/` (`282c8a5b`) |
| **04:58** | mtime del binario `Binaries/Win64/UnrealEditor-RefactorTactics.dll` |
| **05:02** | merge di **#2707** — `ComputeBlockerMarks` entra in `main` |

∴ **Il verdetto negativo è stato emesso su una build che non conteneva il marcatore nel mondo.** *«Non ci sono riferimenti video»* era vero quando è stato scritto e **non è più una descrizione dello stato corrente**.

⚠️ Rigiudicare senza dichiarare *quale build si sta guardando* ripeterebbe esattamente l'errore che questa issue esiste per correggere: la misura del 2026-09-04 diceva *«il muro si vede»* perché guardava l'oggetto sbagliato (la colonna da 55 cm), e nessuno lo aveva scritto accanto al verdetto.

→ **La seduta deve dichiarare commit e binario**, non solo lo scenario.

### 🟡 F-03 · COCKBURN — chi apre l'Editor non parte da qui, e leggerebbe il criterio vecchio

`U46` convoca entrambe le voci in `verifies`, ma:

* `issues: [1246]` — **né #2534 né #2476 sono nominate**. È lo stesso difetto che #2476 registra per sé stessa: *«la seduta esiste, nessuna issue la convoca»*, qui nella direzione opposta;
* il passo **②** dichiara *«cosa resta davvero da giudicare: se la lastra, alta 10 cm, si legga a camera obliqua»*. Dopo `D-340` quella domanda **non ha più un esito utile**: la decisione ha già rinunciato a rendere leggibile la lastra. Chi eseguisse il passo com'è scritto giudicherebbe la cosa che la decisione ha escluso, e la registrerebbe come fallimento di qualcosa che nessuno intende correggere.

### 🟡 F-04 · ADZIC — il giudizio umano si restringe a un esempio, e metà è già headless

Il criterio *«la lastra da 10 cm è leggibile»* non è esprimibile come esempio eseguibile ed è, per decisione, **falso e destinato a restare tale**. Il criterio che conta dopo `D-340` è diverso e si scrive:

```gherkin
Dato   il banco Visual.Map.SightWallIsWalkable, alla camera del giocatore (pitch -40, arm 450)
Quando un tiro è negato dalla cella (q=0,r=0,L=0)
Allora la riga nomina quella cella
  E    quella cella — e nessun'altra — è marcata a schermo
  E    il giocatore collega la riga al segno senza sapere in anticipo cosa cercare
```

Le prime due righe hanno già un oracolo automatico (`HUD.BlockerMarksOnlyNameableCells` per la seconda). **Solo la terza richiede un occhio**, ed è un'unica domanda invece di un giudizio estetico su una geometria che non cambierà.

### 🟢 F-05 · CRISPIN — il guardrail di privacy regge, ed è stato verificato non assunto

La catena ha tre stadi e il terzo **non** riapplica il filtro:

```
SightBlockerForLog (filtro alla scrittura)
  -> Project / BuildPlayerEventFeed (IsAuthorized come PRIMO passo)
       -> ComputeBlockerMarks (nessun filtro - e non deve averne)
```

Il commento in `RTHUDMarksTests.cpp` lo dichiara: un secondo contratto di conoscenza è ciò che #1936 vieta. La domanda che il panel ha posto — *«e allora chi impedisce che il marcatore diventi un leak visivo?»* — ha una risposta misurata: `BlockedShotDoesNotRevealTheWallToTheUnauthorized` prova entrambe le metà, non solo lo zero. **Nessun rilievo.**

### 🟡 F-06 · NYGARD — il corpus golden è cieco proprio sul campo che questa issue introduce

`SightBlockerCell` è escluso da `VisitDiscriminatingFields` (`Turn/RTTurnLog.h`), quindi `CompareSerializedTraces` non lo guarda: una sua regressione **non produce un rosso**. È il fatto che il commento del 2026-09-08 ha misurato e lasciato aperto — *«ciò che resta da decidere è vostro»*. Nessuno lo possiede oggi.

⛔ Fuori scope di questa issue, ma non può restare senza owner: è la sorveglianza del campo che #2534 esiste per introdurre.

## §3 — Consenso del panel

1. **La parte di codice di #2534 è completa nel perimetro che `D-340` le assegna.** Non c'è implementazione mancante da scrivere qui.
2. **Il residuo è un giudizio umano**, e una sessione headless non lo emette. Ciò che una sessione headless può consegnare è il **banco pronto**: la seduta convocata, il criterio aggiornato alla decisione vigente, la build dichiarata.
3. **Due criteri su quattro vanno riscritti** perché oggi misurano lavoro di altri (F-01) o una domanda superata (F-03).
4. **Nessuno dei rilievi giustifica di riaprire (a).** `D-340` regge: il panel non ha trovato un fatto nuovo che la contraddica.

## §4 — Cosa questa run può firmare, e cosa no

| | |
|---|---|
| ✅ Può firmare | la coerenza documentale, la convocazione della seduta, la DoD riformulata, e l'esito dei test **headless** eseguiti su un commit dichiarato |
| ⛔ Non può firmare | il verdetto d'autore di `PIE-HEXPLAY-6` e `PIE-VIS-SIGHTWALL`: è un giudizio a schermo, e chi ha scritto la correzione non ne firma da solo la misura |
| ⛔ Non tocca | `RTSightSlabHeight`, la grammatica visiva di `#956`/`#552`, e `WBP_RT_EventLog` — che appartiene a #1936 fetta F |
