# Sedute componibili — il generatore

La seduta non e' un dato: e' il raggruppamento dei check che condividono un allestimento e i cui
prerequisiti sono soddisfatti. Questo strumento lo calcola.

```bash
python3 tools/decision-log/fetch_github_cache.py            # serve `gh` autenticata
python3 tools/editor-sessions/build_agenda.py               # -> build/ordine-del-giorno.md
python3 tools/editor-sessions/compare_legacy.py             # il gate della fetta 0
python3 -m unittest discover -s tools/editor-sessions -p '*_test.py'
```

Dipendenze: Python 3.12 e `pyyaml`. Nient'altro.

⚠️ **Su Windows, qualunque comando che stampi uno stato vuole
`sys.stdout.reconfigure(encoding='utf-8')`**: la console e' `cp1252` e le emoji del registro la fanno
esplodere con `UnicodeEncodeError`. I file invece si scrivono gia' con `encoding='utf-8'` esplicito.

## I mattoni

`docs/roadmap/sedute-mattoni.yaml` porta `setups` e `wiring`. Il `wiring` si **rigenera** con
`seed_wiring.py`, che combina un'euristica sulla prosa del vecchio registro con due tabelle di
giudizio dichiarato nel proprio sorgente:

- `ALLESTIMENTO_DICHIARATO` — le sedute la cui prosa non dice dove si guarda;
- `CABLAGGIO_DICHIARATO` — i check che due sedute rivendicano con allestimenti **diversi**.

Correggere una riga del yaml non serve: la decisione sta nelle tabelle, e il prossimo seme ricalcola.

## Cosa NON fa

- **Non possiede l'esito di un check.** Quello e' di `docs/technical/test-manuali-pie.md`, e in questo
  schema manca il campo dove scriverlo.
- **Non e' l'owner delle sedute** finche' `docs/roadmap/sedute-mattoni.yaml` porta il marcatore
  `DRAFT — NON OWNER`.
- **Non committa la propria uscita.** `build/` e' ignorato: o rigeneri, o non esiste.
- **Non apre issue.** Quello arriva alla fetta 4, e solo con `--apply`.
- **Non decide un conflitto.** Due sedute in disaccordo fermano la riga e stampano il conflitto:
  «vince la prima» sarebbe una decisione presa dall'ordine del documento.

## Perche' l'uscita non si committa

`editormap.shortlist.md` era una vista generata e committata, ed e' uscita dal repository con D-181
perche' nessuno la leggeva mentre invecchiava. Un file che devi rigenerare per vedere non puo'
mentirti sulla propria eta'.

## Perche' gli errori sono duri

Un registro incoerente **ferma** il generatore, col nome del difetto. Il precedente e' D-182, dove
`--wiki-root` esce con `sys.exit(2)` invece di venire ignorato, perche' un argomento lasciato cadere
produce un verde falso.

🔑 Ha gia' pagato una volta: il 2026-09-11 l'errore *«`PIE-AS4a` e' cablato ma non esiste nel registro
PIE»* ha scoperto che il parser di stato scartava **nove righe reali** col suffisso minuscolo — un
difetto che il registro stesso documenta e che senza l'errore duro sarebbe passato per un verde.
