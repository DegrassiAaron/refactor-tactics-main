# Sedute componibili — il generatore

La seduta non e' un dato: e' il raggruppamento dei check che condividono un allestimento e i cui
prerequisiti sono soddisfatti. Questo strumento lo calcola.

```bash
python3 tools/decision-log/fetch_github_cache.py --also docs/roadmap/sedute-mattoni.yaml
python3 tools/editor-sessions/build_agenda.py               # -> build/ordine-del-giorno.md
python3 tools/editor-sessions/compare_legacy.py             # il gate della fetta 0
python3 tools/editor-sessions/compare_rassegna.py           # il gate del verbale (fetta 1)
python3 -m unittest discover -s tools/editor-sessions -p '*_test.py'
python3 -m unittest discover -s tools/decision-log   -p '*_test.py'
```

⚠️ `--also` non è facoltativo: senza, la cache non conosce le issue che i `requires` citano, e
`build_agenda` si **ferma** col comando da rilanciare.

Dipendenze: Python 3.12 e `pyyaml`. Nient'altro.

⚠️ **Su Windows, qualunque comando che stampi uno stato vuole
`sys.stdout.reconfigure(encoding='utf-8')`**: la console e' `cp1252` e le emoji del registro la fanno
esplodere con `UnicodeEncodeError`. I file invece si scrivono gia' con `encoding='utf-8'` esplicito.
Limite noto: oggi solo `compare_rassegna.py` lo chiama; gli altri script di questa famiglia no. E' un
difetto noto, differito di proposito — non uniformarlo per conto tuo.

## I mattoni

`docs/roadmap/sedute-mattoni.yaml` porta `setups` e `wiring`. Il `wiring` si **rigenera** con
`seed_wiring.py`, che combina un'euristica sulla prosa del vecchio registro con due tabelle di
giudizio dichiarato nel proprio sorgente:

- `ALLESTIMENTO_DICHIARATO` — le sedute la cui prosa non dice dove si guarda;
- `CABLAGGIO_DICHIARATO` — i check che due sedute rivendicano con allestimenti **diversi**.

Correggere una riga del yaml non serve: la decisione sta nelle tabelle, e il prossimo seme ricalcola.

### I prerequisiti

`wiring.requires` dichiara perché un check **non si può guardare oggi**. Un tipo, un oracolo:
`asset:<Nome>` interroga `git ls-files Content`; `mount|cue|anim|feature:<Nome>#<issue>`
interrogano la cache GitHub, e chi chiude la issue rende il bloccante soddisfatto — nessuno deve
ricordarsi di togliere la riga a mano. Ma non succede **da sé**: la cache è un'istantanea
(campo `fetched`), non un dato vivo, quindi il check torna nell'ordine del giorno solo quando
qualcuno rilancia `fetch_github_cache.py --also docs/roadmap/sedute-mattoni.yaml` e ricalcola
l'agenda. Quando i fatti veri su uno stesso nome sono due, si scrivono due righe.

I `requires` **non** si rigenerano: sono giudizio d'autore, ognuno con la propria riga di prova
in `docs/roadmap/plans/sedute-componibili-rassegna-requires-2026-09-12.md`, e
`compare_rassegna.py` rifiuta un bloccante la cui riga di verbale non lo giustifica — e una riga
lo giustifica solo se **entrambe** le colonne portano qualcosa: la decisione ripete il token, *e*
la prova non è vuota. `seed_wiring.py --into` li **preserva** attraverso il riseminio, e si
rifiuta di riseminare se una riga che ne portava uno non viene più prodotta.

⚠️ **Il confronto è per token esatto, non per sottostringa libera.** Scrivi la decisione coi
token nella forma che questo file dichiara — `mount:WBP_RT_EventLogRight#2697`, non
`` `mount:WBP_RT_EventLogRight` (#2697) `` con la issue fuori dal token, in prosa. La seconda
forma è più naturale da leggere, ma il gate non la riconosce come prova per quel `requires`, e
**fallisce di proposito**: un verbale scritto in una forma imprecisa deve fermarsi rumorosamente,
non passare zitto.

## Cosa NON fa

- **Non possiede l'esito di un check.** Quello e' di `docs/technical/test-manuali-pie.md`, e in questo
  schema manca il campo dove scriverlo.
- **Non e' l'owner delle sedute** finche' `docs/roadmap/sedute-mattoni.yaml` porta il marcatore
  `DRAFT — NON OWNER`.
- **Non committa la propria uscita.** `build/` e' ignorato: o rigeneri, o non esiste.
- **Non apre issue.** Quello arriva alla fetta 4, e solo con `--apply`.
- **Non decide un conflitto.** Due sedute in disaccordo fermano la riga e stampano il conflitto:
  «vince la prima» sarebbe una decisione presa dall'ordine del documento.
- **Non deduce un prerequisito.** La prosa del registro PIE cita `#nnn` come *riferimenti*, non
  come bloccanti, e i nomi tipo-asset che contiene sono spesso istanze (`BP_Unit_Gadget_C_0`) o
  prefissi (`WBP_RT_`). Un `requires` si dichiara dopo aver verificato, mai per euristica: un
  falso positivo toglie un check dall'ordine del giorno in silenzio.

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
