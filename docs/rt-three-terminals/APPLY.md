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
