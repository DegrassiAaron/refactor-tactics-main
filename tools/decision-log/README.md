# Vista del Decision Log

Genera una **pagina unica** con tutte le decisioni `D-nnn`, a partire dall'owner canonico
[`docs/decisions/RT_PDR_00_Decision_Log.md`](../../docs/decisions/RT_PDR_00_Decision_Log.md).

```bash
python3 tools/decision-log/build_decision_view.py --out build/decision-log.html
```

Nessuna dipendenza oltre a Python 3.11. L'HTML prodotto è autonomo (i dati sono incorporati) e
**non va committato**: `build/` è in `.gitignore`, si rigenera dal Markdown.

## Cosa mostra

- una riga di registro per decisione: ID, data, stato, frase guida, testo completo, impatto e le note
  della sezione `## Note` agganciate alla decisione che nominano;
- **ogni riferimento è cliccabile**: `#nnnn` apre l'issue su GitHub, `D-nnn` salta alla decisione nella
  stessa pagina, i link relativi puntano al file su `main`;
- per ogni decisione, chi cita (`Cita`) e chi la cita (`Citata da`), ricavati dai rimandi nel testo;
- una seconda vista **Issue collegate**: ogni issue citata dal registro con le decisioni che la nominano;
- ricerca full-text, filtri di stato, ordinamento, `#d-134` come deep link, `/` per cercare.

## Limiti noti

- Lo **stato** è classificato per parole chiave sulla colonna `Stato` (`consolidata`, `accettata`,
  `proposta`, `aperta`, `superata`): è una lettura del testo libero, non un campo strutturato.
- Il renderer Markdown è minimale — code span, link, grassetto, corsivo, barrato, blockquote nelle note.
  Un `**` spaiato nella sorgente resta visibile: è un difetto del Markdown, non della vista.
- La pagina legge solo il Decision Log. `docs/OPEN_DECISIONS.md` e `docs/DOC_CONFLICT_MATRIX.md`
  restano fuori.
