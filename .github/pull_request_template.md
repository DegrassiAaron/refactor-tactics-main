Closes #

<!--
⚠️ La riga qui sopra e' l'UNICA che chiude la issue. `fix(605)` nel messaggio di commit NON la chiude:
GitHub lo legge come uno scope Conventional Commits, non come un riferimento. Vedi `AGENTS.md` §12.

Se questa PR non chiude nessuna issue, cancella la riga invece di lasciarla vuota.
Se ne chiude piu' d'una, ripetila: `Closes #12` a capo `Closes #34` — un elenco separato da virgole
non funziona.

⛔ Se la base di questa PR NON e' il branch di default, `Closes` non chiude niente al merge: la issue
si chiudera' solo quando quel branch arrivera' a `main`. In quel caso chiudila a mano, o dillo qui.
-->

## Cosa cambia



## Perche'

<!-- La ragione, non il riassunto del diff. Se una scelta ha alternative scartate, nominale. -->

## Verifiche

<!--
Solo cio' che e' stato ESEGUITO, con l'esito reale. Cio' che non e' stato eseguito si scrive NOT RUN.
Un `git diff` non e' una verifica.
-->

- [ ] Build `RefactorTacticsEditor Win64 Development` → `Result: ?`
- [ ] `./scripts/rt-suite.ps1` → VALIDA / NON VALIDA, `?/?` completati, `?` fallimenti
- [ ] Verifica di mutazione → cosa e' stato indebolito, quale test e' caduto, ripristinato **e ricostruito**
- [ ] Gate documentali: `doc-links --check --with-archive` · `doc-tables --check` · `issue-refs --check`
- [ ] PIE / packaged → NOT RUN, oppure la voce eseguita

## Owner aggiornati

<!-- Il documento che possiede la regola cambiata. Se nessuno cambia, scrivi perche' nessuno cambia. -->
