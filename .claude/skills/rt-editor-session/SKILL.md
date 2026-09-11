---
name: rt-editor-session
description: Avvia, gestisce, verifica e chiude una seduta controllata di Unreal Editor per Refactor Tactics. Usare solo quando l'utente invoca esplicitamente una sessione Editor, PIE o Automation.
argument-hint: 'mode=<editor|pie|automation> scope=<area> intent=<inspect|author|verify|reproduce> goal="<obiettivo>" [issue=<ref>] [map=<asset>] [scenario=<id>] [evidence=<tipi>]'
disable-model-invocation: true
---

# Refactor Tactics — Editor Session

Gestisci una singola seduta Unreal dalla worktree corrente, dall'ingresso al cleanup. Gli argomenti ricevuti sono:

```text
$ARGUMENTS
```

## Contratto della seduta

Interpreta gli argomenti come coppie `chiave=valore`; i valori tra virgolette possono contenere spazi. Se sono presenti istruzioni libere, trattale come parte di `goal`.

Prima di occupare l'Editor, dichiara un contratto breve con:

- `goal`: risultato osservabile richiesto;
- `mode`: `editor`, `pie` o `automation`;
- `scope`: area funzionale, per esempio `umg`, `hud`, `targeting`, `gameplay`, `map`, `scenario` o `replay`;
- `intent`: `inspect`, `author`, `verify` o `reproduce`;
- issue, mappa, scenario, write-set ed evidenze, quando applicabili;
- condizione di completamento e condizioni di blocco.

`goal` è obbligatorio. Chiedi chiarimenti solo se un dato mancante cambia materialmente l'azione o il rischio. Non inventare mappe, scenari, comandi, asset o risultati.

## Preflight

1. Opera nella directory da cui la sessione Claude è stata avviata. Non cercare né selezionare un'altra worktree e non cambiare branch.
2. Identifica root Git, branch, SHA, file `.uproject`, modifiche preesistenti e documentazione operativa applicabile.
3. Non eseguire stash, reset, checkout distruttivi, merge, rebase, commit o push salvo richiesta esplicita.
4. Scopri i launcher, i test, l'MCP e le procedure Unreal realmente disponibili nel repository. Non ricostruire comandi da memoria.
5. Controlla istanze Unreal esistenti, progetto aperto e ownership. Riusa un Editor solo se l'ownership è certa e compatibile con la seduta.
6. Stabilisci l'ownership leggendo la `CommandLine` dei processi Unreal — porta il `.uproject`, quindi **quale clone**, e `-abslog`, quindi **quale sessione** (`AGENTS.md` §11). Se esiste un Editor incompatibile e l'ownership non è stabilibile, termina con `BLOCKED`.
   ⛔ **Non cercare un lock o un lease**: sono stati rimossi da [`D-347`](../../../docs/decisions/RT_PDR_00_Decision_Log.md) e [`D-362`](../../../docs/decisions/RT_PDR_00_Decision_Log.md) ha rifiutato di reintrodurli — *«niente `scripts/`, niente file di lock, niente marker»*, perché un file di lock resta stantio dopo un crash mentre **un processo che non c'è non dichiara niente**.

## 🔴 Dove puoi scrivere un asset: nel clone principale, e si misura

Il punto 1 dice di restare dove sei, e vale per **tutto tranne una cosa**: creare, modificare, rinominare, spostare, cancellare, importare o **salvare** un asset Unreal via MCP appartiene al **clone principale**, quello che ospita il bridge (`CLAUDE.md` §10, `AGENTS.md` §11, [`D-347`](../../../docs/decisions/RT_PDR_00_Decision_Log.md)).

```powershell
git rev-parse --git-dir            # nel clone principale: .git
git rev-parse --git-common-dir     # se DIFFERISCE, sei in un worktree
```

⚠️ **La ragione è tecnica e silenziosa, ed è per questo che va scritta accanto alla regola**: un worktree **non ha i file gitignorati**, quindi i riferimenti duri di un asset vi leggono `None` e **salvarlo li azzera** — senza errore, e ce ne si accorge dopo. Il bridge MCP è inoltre **uno solo**: usarlo da un altro checkout muta gli asset del principale mentre leggi il `git status` del tuo.

| `intent` | Dove può avvenire |
|---|---|
| `inspect`, `verify`, `reproduce` — lettura, PIE, automation | **qualunque** checkout |
| `author` su `.uasset`/`.umap` via MCP | **solo** il clone che ospita il bridge |

⛔ Se i due `git-dir` differiscono e l'intent è `author` su binari, termina con `BLOCKED` nominando il clone che ospita il bridge. Non aggirarlo, e non dedurre da dove sei dal nome della directory né dal prompt.

Non invocare, ripristinare o dipendere da RT3, RTI o relativi processi e comandi legacy.

## Confini di scrittura

- Un solo writer Unreal può modificare `.uasset` e `.umap` alla volta.
- Per `intent=author`, dichiara il write-set prima della prima modifica binaria. Puoi derivarlo dal goal quando è inequivocabile; altrimenti chiedi conferma.
- Non salvare asset caricati o dirty che non appartengono al write-set.
- Mantieni intatte le modifiche preesistenti dell'utente e degli altri processi.
- MCP è preferibile per stato Editor-only e asset binari quando disponibile, ma una risposta MCP riuscita non dimostra che il comportamento sia corretto.

## Routing per modalità

- `mode=editor`: leggi [references/editor.md](references/editor.md) e applicalo.
- `mode=pie`: leggi [references/pie.md](references/pie.md) e applicalo.
- `mode=automation`: leggi [references/automation.md](references/automation.md) e applicalo.

Leggi sempre [references/evidence-cleanup.md](references/evidence-cleanup.md). Non caricare le reference delle modalità non richieste.

## Regole di esecuzione

- Avvia Unreal solo quando il contratto e il preflight sono completi e l'Editor è realmente necessario.
- Preferisci il setup e il controllo minimi che possono provare o smentire il goal.
- Non ampliare la seduta per correggere problemi estranei: registrali come finding.
- Conserva la stessa identità di prova: worktree, branch, SHA, configurazione, binario e stato motore devono rimanere tracciabili.
- Distingui ciò che hai osservato da ciò che hai inferito. Non dichiarare `PASS` per assenza di errori, avvio riuscito o semplice acknowledgement di un tool.
- Se una capability necessaria non è disponibile, non sostituirla silenziosamente con un test più debole. Usa `BLOCKED` o `NOT RUN` e spiega cosa manca.

## Chiusura della seduta

Il cleanup è obbligatorio anche in caso di errore:

1. termina PIE o la run attiva;
2. salva soltanto le modifiche intenzionali autorizzate;
3. chiudi l'Editor avviato da questa seduta;
4. non chiudere un Editor preesistente senza ownership esplicita;
5. verifica la terminazione dei soli processi posseduti;
6. segnala processi residui o zombie senza eseguire kill indiscriminati;
7. riporta lo stato Git finale e ogni file modificato.

Concludi usando il report definito in `references/evidence-cleanup.md`.
