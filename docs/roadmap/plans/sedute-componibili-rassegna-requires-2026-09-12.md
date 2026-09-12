# Rassegna dei `requires` — verbale, 2026-09-12

> `ESITO DI PASSAGGIO` · Fetta **1** di
> [`sedute-componibili-design-2026-09-11.md`](sedute-componibili-design-2026-09-11.md) ·
> Piano: [`sedute-componibili-piano-fetta-1-2026-09-12.md`](sedute-componibili-piano-fetta-1-2026-09-12.md)
>
> ⛔ **Questo documento non possiede niente.** Non è un registro e non va tenuto aggiornato.
> Dice una cosa sola: **al 2026-09-12 questi check erano stati esaminati, e con quale esito.**
> Lo stato di un check appartiene a `docs/technical/test-manuali-pie.md`; il cablaggio a
> `docs/roadmap/sedute-mattoni.yaml`; le sedute, fino alla fetta 5, a `docs/roadmap/editor-sessions.yaml`.
>
> 🔑 **Convenzione sui numeri.** Ogni conteggio è un esito di questo passaggio e porta il comando
> che lo produce. Chi rilegge **rimisura**.

## Cos'è stato esaminato

Il bacino: i check **cablati** e **non verdi** — un check verde è finito, un check non cablato
appartiene alla coda scoperta, che è un'altra misura.

```bash
python - <<'PY'
import sys; sys.path.insert(0, 'tools/editor-sessions')
import registry, pie_status
_, w = registry.load(); s = pie_status.load()
print(len([x for x in w if x.check in s and s[x.check]["stato"] != pie_status.VERDE]))
PY
```

→ **118** al 2026-09-12, su `c8a2116a`.

Questa fetta esamina **9** di quei check — il primo lotto del Task 6, gli allestimenti
`SET-FRONTEND`, `SET-GRAYKIT`, `SET-HEX-TURN`. I lotti successivi coprono il resto (vedi la
riga in coda alla tabella).

## Il criterio

Una domanda sola, per ogni check: **qualcuno che apre l'Editor in quell'allestimento, oggi, può
arrivare a un verdetto?** Se sì, nessun prerequisito — l'assenza della riga *è* la dichiarazione.
Se no, il prerequisito si dichiara col tipo del proprio oracolo, e con l'owner che può toglierlo.

Un `requires` non verificato non si scrive: un falso positivo toglie un check dall'ordine del
giorno **in silenzio**, un falso negativo si vede aprendo l'Editor. Gli errori non costano uguale.

## Il verbale

| check | allestimento | decisione | prova |
|---|---|---|---|
| `PIE-V01-FRONTEND-MAIN` | `SET-FRONTEND` | — | `WBP_RT_MainMenu` tracciato (PR #1178, `git ls-files Content` conferma) e cablato (`PLAY→StartMatch`, `SETTINGS→PushScreen`, `QUIT→QuitGame`); già **ESEGUITA il 2026-09-04** con verdetto — la nota che diceva «manca l'asset» era superata |
| `PIE-V01-FRONTEND-NAV` | `SET-FRONTEND` | — | i cinque `WBP_RT_*` della precondizione (seduta `U24`: `WBP_RT_FallbackBanner`, `-ErrorModal`, `-LoadingScreen`, `-FrontendRoot`, `-ModalLayer`) sono tutti tracciati (`oracles.asset_tracciati()`); la catena `BeginPlay → StartFrontendForThisGame → StartFrontend → InitializeFrontend` esiste in codice ed è coperta dal test `FrontendGameModeStartsTheFrontend` — misurato chiudendo #938 |
| `PIE-V01-FRONTEND-PAUSE` | `SET-FRONTEND` | `asset:WBP_RT_PauseMenu` | non tracciato in `Content/` (`oracles.asset_tracciati()` → `False`); la riga di registro e la seduta `U30` lo nominano `*** NON TRACCIATO ***` |
| `PIE-V01-FRONTEND-RESULT` | `SET-FRONTEND` | `asset:WBP_RT_ResultScreen` | non tracciato in `Content/` (`oracles.asset_tracciati()` → `False`); la riga di registro e la seduta `U29` lo nominano `*** NON TRACCIATO ***` |
| `PIE-V01-SCREENHUD` | `SET-FRONTEND` | — | già **ESEGUITA il 2026-09-11 su `43e7126b`** con verdetto reale (quattro criteri verdi, uno non eseguibile, uno fallito, uno bloccato dal fallito) — la voce è dimostrabilmente raggiungibile. `WBP_RT_EventLogRight` non è un package: compare solo in un commento di test (`RTMatchWidgetAssetTests.cpp:987`) come nome di **nodo** dentro l'albero del widget, e il mount report della seduta lo conferma costruito (`[ok] EventLog: 1`) dalla classe tracciata `WBP_RT_EventLog`. I due difetti aperti — #2963 (catalogo icone, blocca il criterio Dock) e #2964 (`Il feed e' montato e il turno produce eventi, ma a schermo non compare nessuna riga`, blocca Feed) — sono già registrati come tali nel registro PIE stesso, non impediscono l'accesso al check. #2697 (`gh issue view 2697`, aperta) parla di zero consumatori del feed: non è più il caso qui, dove un consumatore (`WBP_RT_EventLogRight`) risulta montato |
| `PIE-TD-CLEAN` | `SET-GRAYKIT` | — | nessun artifact nelle sedute che lo convocano (`U31`, `U32`: `artifacts: []`); il crash che aveva interrotto `U31` (#2115) è `CLOSED`, corretto lo stesso giorno; `U32` l'ha già confermato in parte il 2026-08-30 |
| `PIE-TD-DOCK` | `SET-GRAYKIT` | — | nessun artifact (`U31: artifacts: []`); il difetto che lo rendeva rosso (#2168) è `CLOSED` dal 2026-09-03, mai riverificato ma non bloccato — la precondizione (`Load Layout → Default Editor Layout`) è procedurale, non un prerequisito mancante |
| `PIE-TD-PRESENT` | `SET-GRAYKIT` | — | nessun artifact (`U31: artifacts: []`); `L_DevSandbox` tracciato; la parte confermata il 2026-09-02 resta valida, il resto fu interrotto dallo stesso #2115, `CLOSED` lo stesso giorno |
| `PIE-PREVIEW-GHOST` | `SET-HEX-TURN` | — | nessun artifact (`U52: artifacts: []`); il codice è in `main` da #2941, coperto headless da sette `Preview.*`; la precondizione è CVar da console + un piano costruito a mano, nessun asset o widget mancante — non ancora eseguita perché nessuna sessione precedente aveva uno schermo da guardare, non perché manchi un prerequisito |

<!-- lotti successivi: SET-SANDBOX, SET-GEN-ARENA, SET-HEX-MATCH, SET-HEX-BOT, SET-SCEN -->

## Cosa questa rassegna non ha deciso

- `SET-TD` è dichiarato e nessun check lo usa. È una domanda sull'**allestimento**, non sui
  prerequisiti: resta il follow-up che §12 della spec ha aperto.
- La **coda scoperta** — i check che nessuna riga di `wiring` cabla — non è entrata nel bacino.
  Fra essi `PIE-VIS-DEFLECT` e `PIE-VIS-INTERPOSE`, che la spec cita come casi `cue` canonici:
  non si può dichiarare il prerequisito di un check che non ha ancora un allestimento.
- I restanti check del bacino (`SET-SANDBOX`, `SET-GEN-ARENA`, `SET-HEX-MATCH`, `SET-HEX-BOT`,
  `SET-SCEN`) appartengono ai lotti successivi di questo stesso Task 6, non a questa fetta.
