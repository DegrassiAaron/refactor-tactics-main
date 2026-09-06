# Apply RT Three Terminal Roles update

Questo bundle sostituisce/aggiunge file sotto `docs/rt-three-terminals/`.

## Modifiche principali

- `rt-three-terminals` = 3 ruoli, N terminali.
- Più istanze DEV supportate nello stesso checkout.
- Identità istanza `[ROLE:PID]`.
- VALIDATION: più terminali possono esistere, ma un solo job Unreal attivo alla volta.
- EDITOR: normalmente una sola sessione Unreal e un solo writer `.uasset/.umap`.
- Regole Git esplicite per shared working tree.
- Fix installer: i template VS Code versionati vivono in `payload/vscode/`, perché `.vscode/` è ignorata.
- Preflight installer: niente installazione parziale se manca un file.
- Nuovi task VS Code per aprire singoli ruoli ripetutamente.
- Alias `RT: Open 3 terminals` mantenuto per compatibilità.

## Livello wave (nuovo)

Separato dal livello ruolo. I prompt di ruolo dicono cosa un terminale può occupare; i prompt di wave dicono come si esegue e si consegna un lavoro attraverso i ruoli.

- `prompts/RT3_CONTRACT.md` — contratto condiviso: preflight fail-closed, precondizioni repo, verdetti tipizzati, matrice canonica, scoping dal write-set, schema e persistenza handoff, propagazione `BLOCKED`, defect policy con terminazione.
- `prompts/WAVE_DEV_LEAD.md` — prompt di wave DEV-LEAD: produce l'handoff di ingresso `RT3-DEVLEAD-<sha7>.md` che `WAVE_EDITOR.md` e `WAVE_VALIDATION.md` richiedono come input.
- `prompts/WAVE_DEV_MAIN.md` — prompt di wave DEV-MAIN: implementazione dentro lo scope assegnato, contributo in `contrib/`.
- `prompts/WAVE_DEV_TEST.md` — prompt di wave DEV-TEST: authoring di test, scenari e validator; l'esecuzione resta a VALIDATION.
- `prompts/WAVE_EDITOR.md` — prompt di wave EDITOR.
- `prompts/WAVE_VALIDATION.md` — prompt di wave VALIDATION.
- `prompts/RT3_EXAMPLE.md` — handoff di esempio compilato, con `FAIL`, `BLOCKED` e `USER_REQUIRED`.
- `waves/` — handoff persistiti, contributi DEV in `contrib/` e artefatti di evidenza.

Punti che cambiano il comportamento:

- EDITOR e VALIDATION sono **file separati**: dichiarano identità mutuamente esclusive e non vanno incollati insieme;
- input mancante o placeholder non risolto ⇒ `BLOCKED` prima di ispezionare il repository;
- `PASS` richiede `EVIDENCE_REF` ad artefatto; `N/A` richiede `REASON`; un verdetto malformato si legge `BLOCKED`;
- nuovo verdetto `OBSERVED` per i sistemi che un ruolo può guardare ma non provare — `PRIVACY` in EDITOR è `OBSERVED`, mai `PASS`;
- handoff a tre punti fissi: `BASE_SHA`, `PRODUCED_SHA`, `PARENT_BRANCH`;
- handoff ed evidenza persistiti su disco, non nella conversazione;
- il seed PIE è un input dichiarato prima dell'esecuzione: `SEED_SOURCE: generated` non ammette `PASS` su determinismo;
- lifecycle Editor e chiusura su errore rinviati a `CLAUDE.md` §5, non riformulati;
- reintegrate le regole perse: oracolo positivo MCP, risposta vuota ≠ capability assente, `performed = 0` ≠ `PASS`;
- defect loop con `ATTEMPT`: al terzo ciclo sullo stesso `FINDING_ID` si escala.

## Revisione 2026-09-06 — workspace, lease e policy MCP

- identità di workspace `MAIN | DEV | TECHNICAL_DESIGNER`, registrata per macchina e **non** dedotta
  dal nome della cartella; `-WorkspaceId` è obbligatorio nell'installer;
- `MAIN` è un workspace, non il branch `main`: l'authoring asset resta su un branch di task;
- lease del motore unico per macchina sotto `%LOCALAPPDATA%\RefactorTactics\RT3\`, acquisito **just-in-time**: aprire un
  terminale non occupa Unreal;
- `rtmode` declassato a informativo. Il suo file era per-checkout mentre il motore è per-macchina
  (finding `parsecell-arity/1-F13`);
- `rt-suite-safe.ps1` chiede un lease vivo e posseduto, non più il mode locale;
- `rt-mcp-guard.ps1`: preflight della policy «asset MCP solo da MAIN», con codici di rifiuto stabili;
- log JSONL di lease e side effect accanto al lease, senza segreti;
- `.mcp.json` **esce dal versionamento** e lo genera l'installer: era un blob identico nei tre
  checkout e non poteva descrivere una configurazione di macchina;
- task VS Code per stato/acquisizione/rilascio del lease e per il preflight MCP;
- profili e task passano a `pwsh`: `scripts/rt-suite.ps1` è UTF-8 senza BOM e Windows PowerShell 5.1
  lo legge come Windows-1252 — 26 errori di parsing, misurati.

⛔ **Il preflight autorizza, non intercetta.** Il trasporto MCP è HTTP diretto: chi lo salta
raggiunge il bridge lo stesso. Vedi «Enforcement reale» nel README.

## Applicazione manuale

Copia la cartella `docs/rt-three-terminals/` sopra quella del repository.

Poi, dalla root del repository:

```powershell
.\docs\rt-three-terminals\install-rt-terminals.ps1 -RepoRoot (Get-Location).Path
```

## Verifiche consigliate

1. Apri due volte `RT: Open DEV terminal`.
2. Verifica che i prompt mostrino ID differenti.
3. `rtstatus` in entrambi.
4. Apri VALIDATION e EDITOR.
5. Verifica che `rtsuite` sia bloccato da DEV/EDITOR.
6. Cambia `rtmode VALIDATION` e verifica il gate dal terminale VALIDATION.
7. Non avviare Unreal durante questi smoke test.
