# Censimento delle voci PIE orfane — 2026-09-23

> **Issue**: [#2188](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2188) ·
> **Owner della schedulazione**: [`editor-sessions.yaml`](../editor-sessions.yaml) ·
> **Registro degli esiti**: [`test-manuali-pie.md`](../../technical/test-manuali-pie.md)
>
> 📌 **Misurato su `origin/main` `27349d4f`**, che era anche `HEAD` del branch di questa passata. Lo SHA
> è pinnato e non è una formalità: il registro PIE cresce di voci quasi ogni giorno, e una misura senza
> albero dichiarato è una misura di cui non si può dire se sia ancora vera.
>
> ⛔ **Questo documento non assegna nessun esito.** Scheda: dice quali voci nessuno convocava e dove
> sono finite. Lo stato di una voce resta di `test-manuali-pie.md`, che ne è l'unico owner (R-6), e
> questa passata **non ha toccato quel file**.

---

## 1 · La misura, e il criterio che la governa

Due insiemi, incrociati.

**Le voci aperte** si contano col comando canonico, che sta in testa al registro e **non si reinventa** —
classifica sul **PRIMO** marcatore della cella di stato:

```bash
awk -F'|' '/^\| \*\*PIE-/ {s=$(NF-1);
  if (match(s, /✅|🟡|❌|⏳/)) c[substr(s, RSTART, RLENGTH)]++; else c["nessuno"]++ }
  END {printf "verde=%d parziale=%d fallita=%d aperta=%d senza-marcatore=%d\n",
       c["✅"], c["🟡"], c["❌"], c["⏳"], c["nessuno"]}' \
  docs/technical/test-manuali-pie.md
```

Su `27349d4f` risponde `verde=90 parziale=26 fallita=8 aperta=121 senza-marcatore=0`, su `245` righe di
tabella (`grep -c '^| \*\*PIE-'`).

⚠️ **Perché il criterio conta, ed è la trappola che #2188 nomina per prima**: cercare `⏳` *ovunque* nella
cella invece che come primo marcatore conta due volte ogni voce già giudicata che nomini un ⏳ nel proprio
testo. La issue registra uno scarto del 27% su quella distinzione.

**Le voci convocate** sono quelle che compaiono nel campo `verifies` di una seduta. Si leggono con un
parser YAML, **mai con un grep**:

```bash
python3 -c "import yaml;d=yaml.safe_load(open('docs/roadmap/editor-sessions.yaml',encoding='utf-8'));\
print(len({c for s in d['sessions'] for c in (s.get('verifies') or [])}))"
```

⚠️ **`editor-sessions.yaml` usa DUE forme di lista** — a trattini e inline `[A, B, C]` — e circa metà delle
sedute usa la seconda. Un parser che vede solo la prima trova `74` voci convocate invece di `169`, e manda
a schedulare righe che esistono già. Non è un'ipotesi: è il difetto che `U45` porta scritto nel proprio
commento, trovato la prima volta il 2026-09-03.

## 2 · Perché sono 42 e la issue ne diceva 30

La issue misurò `200` voci, `98` aperte, `30` orfane su `2eb4ace2`. Oggi sono `245`, `121` e **42**.

**Il registro è cresciuto, non è peggiorato.** È il suo mestiere, e il registro stesso lo dichiara nel
proprio cappello: *«questo file è un catalogo, non un backlog … le voci non verdi non sono lavoro in
ritardo»*. Fra le due misure sono entrate voci nuove — la maggior parte scritte **insieme alla propria
feature**, che è il verso giusto — e nessuna di esse è stata convocata da una seduta.

🔑 **Ed è questo il punto del censimento, non il numero.** Le voci che arrivano dopo una feature nascono
orfane per costruzione: chi chiude la feature scrive la voce (la track `playtest` di `D-139` glielo
prescrive) e non ha titolo per decidere in quale seduta vada. Senza un passaggio che le raccolga,
l'orfanità è lo stato **di default** di ogni voce nuova.

## 3 · Le 42, e dove sono finite

Le tre uscite sono quelle che la issue dichiara legittime: **seduta esistente**, **seduta nuova**,
**non schedulabile con la ragione**. La quarta — tacere — non lo è.

| Voce | Sezione del registro | Destinazione |
|---|---|---|
| `PIE-ACC-HUD` | Scenari compositi di acceptance | `U43` · seduta esistente |
| `PIE-ACC-PERCEPTION` | Scenari compositi di acceptance | `U43` · seduta esistente |
| `PIE-DEBUG-LOS` | Strumenti di leggibilità | `U53` · seduta esistente |
| `PIE-FMTVER` | Checklist | **U55** · seduta nuova |
| `PIE-GEO-BORDO` | Il gesto dell'autore — tool Geometry | `U22` · seduta esistente |
| `PIE-HEX-MODE-P` | Checklist | **U55** · seduta nuova |
| `PIE-HEX-MODE-Q` | Checklist | **U55** · seduta nuova |
| `PIE-HEX-MODE-R` | Checklist | **U55** · seduta nuova |
| `PIE-HUD-CARD-ZERO` | Contenuto della v0.1 | `U49` · seduta esistente |
| `PIE-KNOW-POSA` | Checklist | `U43` · seduta esistente |
| `PIE-NET-CANARY-PACKAGED` | Rete e privacy degli intenti (M10) | ⛔ `not_schedulable` |
| `PIE-NET-LATEJOIN` | Rete e privacy degli intenti (M10) | ⛔ `not_schedulable` |
| `PIE-NET-NO-PAUSE` | Rete e privacy degli intenti (M10) | ⛔ `not_schedulable` |
| `PIE-PACING-1` | Durata, ritmo e scala | `U19` · seduta esistente |
| `PIE-STATE-01` | Stati del personaggio (E34) | ⛔ `not_schedulable` |
| `PIE-STATE-02` | Stati del personaggio (E34) | ⛔ `not_schedulable` |
| `PIE-STATE-03` | Stati del personaggio (E34) | ⛔ `not_schedulable` |
| `PIE-STATE-04` | Stati del personaggio (E34) | ⛔ `not_schedulable` |
| `PIE-STATE-05` | Stati del personaggio (E34) | ⛔ `not_schedulable` |
| `PIE-STATE-06` | Stati del personaggio (E34) | ⛔ `not_schedulable` |
| `PIE-STATE-07` | Stati del personaggio (E34) | ⛔ `not_schedulable` |
| `PIE-STATE-08` | Stati del personaggio (E34) | ⛔ `not_schedulable` |
| `PIE-STATE-09` | Stati del personaggio (E34) | ⛔ `not_schedulable` |
| `PIE-STATE-10` | Stati del personaggio (E34) | ⛔ `not_schedulable` |
| `PIE-V01-ARENADOOR` | Contenuto della v0.1 | `U45` · seduta esistente |
| `PIE-V01-FOOTPRINT` | Durata, ritmo e scala | **U58** · seduta nuova |
| `PIE-V01-FRONTEND-ERROR` | Frontend shell e ciclo di partita (E46) | **U57** · seduta nuova |
| `PIE-V01-FRONTEND-PLAY` | Frontend shell e ciclo di partita (E46) | `U49` · seduta esistente |
| `PIE-V01-FRONTEND-REPLAY` | Frontend shell e ciclo di partita (E46) | `U23` · seduta esistente |
| `PIE-V01-GHOSTS` | Contenuto della v0.1 | `U52` · seduta esistente |
| `PIE-V01-PLAYSPEED` | Durata, ritmo e scala | `U23` · seduta esistente |
| `PIE-V01-REFUSAL` | Durata, ritmo e scala | **U58** · seduta nuova |
| `PIE-V01-REPLAY-VIEWER` | Contenuto della v0.1 | **U57** · seduta nuova |
| `PIE-V01-RXBRACE` | Durata, ritmo e scala | **U56** · seduta nuova |
| `PIE-V01-RXPLAYBACK` | Durata, ritmo e scala | **U56** · seduta nuova |
| `PIE-V01-SHIELD-WRAITH` | Contenuto della v0.1 | `U51` · seduta esistente — ⌫ *era `not_schedulable`, convocata lo stesso giorno da #2381* |
| `PIE-V01-SIGHTLINE` | Durata, ritmo e scala | **U58** · seduta nuova |
| `PIE-VIS-DEFLECT` | Corpus `Visual.*` | ⛔ `not_schedulable` |
| `PIE-VIS-INTERPOSE` | Corpus `Visual.*` | ⛔ `not_schedulable` |
| `PIE-VIS-PUSH` | Corpus `Visual.*` | ⛔ `not_schedulable` |
| `PIE-VIS-WATCHPUSH` | Corpus `Visual.*` | `U47` · seduta esistente |
| `PIE-VSLICE-01` | Gate visivo end-to-end della slice | ⛔ `not_schedulable` |

**seduta esistente `14` · seduta nuova `11` · non schedulabile `17` · somma `42`** — e la somma va scritta,
perché è l'unico modo di accorgersi che una voce è caduta fra due colonne.

⌫ **Era `13 / 11 / 18` quando questo documento è nato**, poche ore prima: `PIE-V01-SHIELD-WRAITH` era
dichiarata non schedulabile con `revisit_when: «#2381 la convoca»`, e **#2381 l'ha convocata lo stesso
giorno** in `U51` — la seduta che possiede già il gesto di scrivere `Team0Heroes` a mano. La riga è uscita
dal blocco invece di cambiare, come il suo `revisit_when` prescriveva. È il ciclo che quel blocco esiste
per rendere possibile, girato una volta: **una voce dichiarata non è parcheggiata, è in attesa di un
fatto**.

### Il criterio: «con quali altre voci si allestisce insieme?»

La issue nomina il rischio prima della soluzione: assegnare le orfane a sedute create per l'occasione
produrrebbe un file formalmente completo e operativamente falso — **sedute da una voce che nessuno
convocherà**, cioè lo stesso difetto con un altro nome.

Quindi: **nessuna delle quattro sedute nuove ha una voce sola.** `U55` ne porta quattro, le altre due o
tre, e ciascuna raccoglie voci che condividono l'allestimento — non il tema.

Le assegnazioni più nette sono quelle in cui la seduta **nominava già** la voce senza convocarla:

- **`U43`** allestisce `Visual.Perception.Acceptance` e `Visual.Hud.FirstPlayable`, e i suoi `steps`
  scrivono *«le due voci ombrello restano ⏳»*. Le due voci ombrello **di quei due scenari** erano
  `PIE-ACC-PERCEPTION` e `PIE-ACC-HUD`, fuori da `verifies`. La seduta le conosceva e non le chiamava.
- **`U28`** porta scritto che `-ERROR` e `-PLAY` *«restano SENZA seduta, ed è dichiarato invece che
  nascosto»*, con la ragione per cui non entravano lì. La ragione regge — quella seduta è
  `execution_lane: asset` — e infatti non ci sono entrate: `-PLAY` è in `U49`, che apre da `L_Frontend`
  e preme `PLAY`, e `-ERROR` in `U57`. ⚠️ Quel commento era la **previsione esatta** del difetto che
  #2188 è andato a misurare, scritta un mese prima e da nessuno raccolta. È il modo in cui una voce
  resta fuori da ogni seduta: non la dimenticanza, la dichiarazione che nessuno rilegge.
- **`U22`** ha in `verifies` `PIE-GEO-CENTRO`, e la precondizione di `PIE-GEO-BORDO` nel registro è
  letteralmente *«come sopra»*. Due metà dello stesso muro, una convocata e una no.

E due voci si erano **sbloccate senza che nessuno le ripescasse** — `PIE-V01-RXBRACE` e
`PIE-V01-RXPLAYBACK`, ora in `U56`. Le loro celle lo scrivono già: *«adesso il pericolo è che qualcuno
non esegua una voce diventata eseguibile»*. È la forma più cara del difetto — non una voce dimenticata,
una voce sbloccata e lasciata dov'era.

### Le diciotto dichiarate

Stanno nel blocco `not_schedulable:` di [`editor-sessions.yaml`](../editor-sessions.yaml), ognuna con
`reason`, `oracle` e `revisit_when`. Le cause sono quattro:

| Causa | Voci | Oracolo |
|---|---|---|
| La presentazione non disegna ciò che il dato porta | `PIE-VIS-PUSH` · `PIE-VIS-DEFLECT` · `PIE-VIS-INTERPOSE` | #3261 e #2454, entrambe `OPEN`; per le ultime due la dichiarazione è **nel codice** (`RTPresentationBinding.cpp`, che le nomina) |
| Il gate non si esegue a pezzi | `PIE-VSLICE-01` | #613 e #220 `OPEN` |
| `E34` non è cominciata | le dieci `PIE-STATE-*` | gli undici `CP 34.x` e l'epic #244, tutti `OPEN` |
| La rete non esiste in produzione | le tre `PIE-NET-*` | `grep -rn "DOREPLIFETIME" Source/ \| grep -v /Tests/` → nessuna riga |

Più una che non era bloccata ma **era di qualcun altro**: `PIE-V01-SHIELD-WRAITH`, terza casella dello
scope di [#2381](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2381). ✅ **Convocata in
`U51` lo stesso giorno**, ed è uscita da questo blocco.

🔴 **E la sua `reason` portava due identità ritirate**, ricopiate dalla cella del registro senza
verificarle: diceva *«una formazione con **Wraith** in squadra 0, contro il default `Team0Heroes =
[Gadget, Phase]`»*, e nessuno dei tre nomi esiste più — `D-334` ha ritirato `Hero.Wraith` → `Hero.Ivrin`,
`#2491` `Hero.Gadget` → `Hero.Aevik` e `Hero.Phase` → `Hero.Muiren`. Il default vero è
`Team0Heroes = {Hero.Aevik, Hero.Muiren}` (`RTGameMode.h:93`). ⚠️ **La sostanza reggeva** — l'eroe è
ancora in squadra 1, quindi l'allestimento serve lo stesso — ma il nome da scrivere nel GameMode era
sbagliato, ed è l'unica parte che chi esegue avrebbe usato. Oracolo:
`RTLegacyIdentityRatchetTests.cpp`, che tiene `Hero.Wraith` a tetto `0, 0`.

Anche questa era una frase **plausibile** copiata da una fonte autorevole senza girare l'oracolo che il
repository ha già scritto apposta.

⚠️ **Una voce dichiarata NON è schedulata**, e la differenza è il punto: è dichiarata, che vale meno.
Quando il suo oracolo cade, la voce **esce da quel blocco ed entra in un `verifies`**.

## 4 · Il gate, che è la parte che non invecchia

Una tabella committata invecchia in silenzio — è la ragione per cui `editormap.shortlist.md` è uscita dal
repository con `D-181`. Ciò che resta vero è un **invariante**, e si rimisura:

```
aperte  -  convocate da una seduta  -  dichiarate in `not_schedulable`  =  0
```

```bash
python3 - <<'PY'
import re, yaml
from pathlib import Path
RIGA = re.compile(r'^\|\s*\*\*(PIE-[A-Za-z0-9.-]+)\*\*')
aperte = set()
for l in Path('docs/technical/test-manuali-pie.md').read_text(encoding='utf-8').splitlines():
    m = RIGA.match(l)
    if m:
        cella = [x.strip() for x in l.split('|')][-2]
        if next((x for x in cella if x in '✅\U0001f7e1❌⏳'), '?') == '⏳':
            aperte.add(m.group(1))
d = yaml.safe_load(Path('docs/roadmap/editor-sessions.yaml').read_text(encoding='utf-8'))
conv = {c for s in d['sessions'] for c in (s.get('verifies') or [])}
dich = {x['check'] for x in d['not_schedulable']}
resto = sorted(aperte - conv - dich)
print(f'aperte={len(aperte)} convocate={len(aperte & conv)} dichiarate={len(aperte & dich)}')
print(f'RESIDUO={len(resto)} {resto}')
PY
```

Su `27349d4f` con questa passata applicata: `aperte=121 convocate=103 dichiarate=18`, **`RESIDUO=0`**.

🔑 **Il residuo è l'unico numero che vale la pena leggere**, e non invecchia: quando entra una voce nuova
— e ne entra una quasi ogni giorno — sale a `1` e nomina la voce. Un totale di orfane, invece, sarebbe
stantio il giorno dopo.

⚠️ **Il gate va girato su ciò che si eredita, non su ciò che si è appena scritto.** Un `RESIDUO=0`
calcolato subito dopo aver assegnato le voci che il gate stesso enumera non può fallire. La misura che
vale è quella della **prossima** persona, su un albero che ha nel frattempo ricevuto voci nuove.

## 5 · Trovato di passaggio, e NON corretto qui

Quattro cose che questa misura ha attraversato e che appartengono ad altri file. Sono scritte qui perché
la prossima persona non le riscopra da zero.

1. 🔴 **`tools/editor-sessions/pie_status.py` salta una riga del registro.** Il suo regex è
   `PIE-[A-Za-z0-9-]+`, e **`PIE-CP1.4` contiene un punto**: la riga non viene troncata, viene *saltata*,
   perché il `**` di chiusura non arriva dove il regex lo aspetta. Il modulo conta `244` voci e `89`
   verdi dove l'`awk` canonico ne conta `245` e `90`. È lo **stesso difetto** che quel file documenta nel
   proprio commento per le nove righe col suffisso minuscolo, ricomparso con un altro carattere.
   ⚠️ Oggi non produce falsi sulle orfane — `PIE-CP1.4` è verde — ma `pie_status.py` alimenta
   `compare_legacy.py`, che è un gate. **Non toccato**: è di un altro perimetro.
   Oracolo: `grep -n '^| \*\*PIE-CP1\.4\*\*' docs/technical/test-manuali-pie.md`

2. ⚠️ **Il blockquote di `PIE-VSLICE-01` dice ancora «cinque dipendenze aperte».** Era vero il
   2026-08-30; oggi tre sono chiuse (#1712, #1497 il 2026-09-02, #1535 il 2026-09-10) e ne restano due.
   La conclusione **non cambia** — la voce resta non schedulabile — ma chi legge quella riga crede di
   avere davanti cinque bloccanti invece di due. `test-manuali-pie.md` ha un owner e la correzione è sua.

3. ⚠️ **La cella di `PIE-DEBUG-LOS` dice che `PIE-HEXPLAY-6` «resta 🟡».** Sul registro misurato oggi
   `PIE-HEXPLAY-6` è ✅. L'invito che quella riga contiene — *«chi esegue le guardi insieme, stessa
   fixture, stesso muro»* — resta valido come indicazione di allestimento, ed è la ragione per cui
   `PIE-DEBUG-LOS` è andata in `U53`; è il puntatore allo stato a essere scaduto.

4. ⚠️ **`RTPresentationBinding.cpp` ha un commento scaduto su `AttackFootprint`.** Una riga lo elenca
   fra i `PendingPresentation` — *«non si disegnano ancora»* — mentre poco sotto lo stesso file registra
   `FRTPresentationBinding(ERTResolvedEventType::AttackFootprint, {"AddPlaybackFootprint"})`, cioè una
   cue vera. La differenza decide se `PIE-V01-FOOTPRINT` sia giudicabile: **lo è**, ed è in `U58`.
   Oracolo: `grep -n "AttackFootprint" Source/RefactorTactics/Turn/RTPresentationBinding.cpp`

➕ **E una che non è un difetto ma va saputa**: sei voci stanno in **due** sedute
(`PIE-HEXPLAY-6`, `PIE-HEXPLAY-8`, `PIE-V01-ROSTER`, `PIE-V01-LOG` in `U46`; `PIE-TD-CLEAN` in `U31`/`U32`;
`PIE-V01-SCREENHUD` in `U49`/`U54`). Sono tutte preesistenti a questa passata e nessuna nasce da essa —
`U46` e `U54` sono riprese dichiarate. ⚠️ Vale la pena registrarle perché `U43` porta scritta la regola
opposta: *«una voce in due sedute è una voce che nessuna delle due esegue»*. Le due cose convivono solo
finché la seconda seduta è una **ripresa** e lo dichiara; non è stato verificato per tutte e sei.

## 6 · Cosa questa passata non ha fatto

- ⛔ **Nessun esito assegnato.** Nessuna voce cambia marcatore, e `test-manuali-pie.md` non è stato
  toccato: `git diff --stat` di questa passata non lo nomina.
- ⛔ **Nessuna seduta eseguita.** Le quattro nuove nascono con `verifies` pieno e nessun verdetto.
- ⛔ **Nessun numero `D-` coniato.** Dove serviva una decisione — se una voce dichiarata debba comparire
  in una vista, e come — la forma scelta è un campo di dati con il proprio oracolo, non una decisione
  nuova.
- ⛔ **Nessun gate altrui toccato**: `tools/editor-sessions/` resta com'era, e i suoi 82 test restano
  verdi (`python3 -m unittest discover -s tools/editor-sessions -p '*_test.py'`).
