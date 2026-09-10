# Il corpus delle issue e degli scenari — spec panel e triage misurato

> **Referto di misura**, non owner. Nessuna regola citata qui appartiene a questo documento.
>
> **Data**: 2026-09-10 · **Base**: `origin/main` = `5ec21ed0` · **Metodo**: misura per issue e per file, non deduzione
>
> ⛔ **Nessuna riga di `Source/` toccata.** Le modifiche a GitHub sono state **eseguite** dopo che la
> sessione ha scelto le leve: l'elenco esatto e' in §7, e ogni chiusura porta il proprio commento con la
> misura che la giustifica.
>
> **Convenzione dei numeri**: ogni conteggio qui sotto e' una **misura**, e porta il comando che la
> produce. Non e' un totale di prosa: se lo si rilegge fra un mese, lo si rimisura con quel comando.
>
> 🔴 **Tre conclusioni della prima stesura sono state RITIRATE dalla misura successiva** — §3.4.1
> (BAL e Wiki PF), §4.2 (gli scenari «senza specifica») e la parte di §4.3 che riguardava #2793. Restano
> scritte, con la ragione: un referto che cancella i propri errori insegna meno di uno che li tiene.

---

## 1. Il verdetto in una riga

> **Il corpus non e' gonfio di lavoro sciatto: e' gonfio di lavoro scritto all'altezza sbagliata.**
> Quattrocento issue aperte, e centosessanta descrivono milestone che cominciano fra cinque release.
> Il tracker non riesce a rispondere a *«cosa lavoro adesso»* senza filtrare via voci che non sono
> mai state candidate.

```bash
gh issue list --state open --limit 400 --json number --jq 'length'                     # 400
gh issue list --state open --limit 400 --label post-v0.1 --json number --jq 'length'   # 160
```

⚠️ **La trappola di paginazione del referto del 2026-09-02 vale ancora**: `gh issue list` senza
`--limit` sopra il default restituisce una risposta plausibile e corta, e non avverte. Ogni comando
qui porta `--limit 400`.

---

## 2. La diagnosi: tre altezze in una lista piatta

Il difetto non e' il numero. E' che tre generi diversi di enunciato condividono lo stesso contenitore:

| Altezza | Esempio misurato | E' azionabile? |
|---|---|---|
| **Difetto** — una sessione, un PR | #2699 *«`ParseCell` pinna l'arita' e non il valore»* | ✅ si apre il file e si corregge |
| **Traguardo** — un capitolo di milestone | #814 *«CP 45.7 · Certificazione di performance sulla build di rilascio»* | ❌ e' un **titolo di capitolo** |
| **Capability** — un arco di release | #778 *«[EPIC v1.0] E45 · Un gate di produzione»* | ❌ ed e' giusto cosi': l'epic indicizza |

Una issue e' uno strumento del primo genere. Le altre due la usano come **indice**, e l'indice funziona —
ma paga il suo costo su ogni query che qualcuno fa per trovare lavoro.

∴ La riduzione non passa dal cancellare informazione. Passa dallo **spostarla di contenitore**.

🔑 **Chiudere non e' cancellare.** Il corpo di una issue chiusa resta leggibile, cercabile e linkabile
per sempre. Un `CP` chiuso e indicizzato dal suo epic conserva ogni parola della sua DoD, e smette di
comparire fra i candidati di lavoro.

---

## 3. I quattro cluster misurati

### 3.1 🔴 Stale-open verificata contro il codice — #1515

`Scenario Harness: TeamId non validato e id unita' duplicati accettati all'apertura`.
**Entrambe le parti sono su `main`**, e il codice cita la issue per nome:

```bash
grep -n "IsValidTeamId\|id unita' duplicato" Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp
```

- `RTScenarioLoader.cpp:2470` — `id unita' duplicato: '%s'` → AC4 soddisfatta;
- `RTScenarioLoader.cpp:2488` — `URTTurnRules::IsValidTeamId(Unit.TeamId)` → AC1 e AC2 soddisfatte,
  e l'intervallo e' **derivato da `URTTurnRules`**, che e' esattamente cio' che AC2 chiedeva invece di
  una costante locale;
- `FRTScenarioCorpusRunsTest` (`RTScenarioCorpusTests.cpp:90`) itera `URTScenarioIndex::ListIds` → AC3
  e' asserita dal corpus, non osservata a mano.

⚠️ **Le caselle della DoD di #1515 sono tutte vuote.** Il referto del 2026-09-02 aveva gia' misurato che
*«lo stato delle caselle di DoD non e' un segnale»*: qui lo conferma nella terza forma — implementato
e mai spuntato.

∴ **#1515 e' chiudibile misurando.** Resta da verificare la sola AC5 (verifica di mutazione).

### 3.2 Duplicato vero — #2657 / #2658

Stesso test `RefactorTactics.Reactions.Counter.TwoCountersKeepTheirOwnOrigin`, stesso fenomeno
(intermittenza sul solo campo autore), stessa conclusione (*«non e' il refactor di #2587»*).

```bash
gh issue view 2657 --json createdAt --jq .createdAt   # 2026-09-07T09:42:26Z
gh issue view 2658 --json createdAt --jq .createdAt   # 2026-09-07T09:43:04Z
```

**Trentotto secondi.** Non e' una divergenza di analisi: e' una doppia apertura. Le due misure — «una su
tre» e «una su sei» — sono due campioni dello **stesso** flaky, e insieme valgono piu' che separate.

∴ Una sopravvive e assorbe la tabella dell'altra. L'altra si chiude come duplicato, con il link.

### 3.3 Cluster di un solo file, una sola review — #2698 · #2699 · #2701 · #2702

Le quattro sono nate dalla stessa code review di #2546, misurate sullo **stesso commit** `975882af`,
e vivono nello **stesso file** `RTScenarioLoader.cpp`. Sono la stessa classe di difetto, che #2546 ha
gia' nominato: *«un campo presente e malformato non e' mai un campo assente»*.

Verificato che il cluster e' **ancora reale** — `ParseCell` non e' stato toccato:

```bash
sed -n '61,82p' Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp
```

controlla `Arr->Num() != 3` e poi chiama `AsNumber()` senza guardia: un elemento non numerico diventa
`0`, un frazionario si tronca, e il cast `double → int32` fuori scala resta UB.

⛔ **Non e' una proposta di chiuderle come risolte.** Sono aperte e valide. La proposta e' che
**quattro branch, quattro PR e quattro review** per quattro guardie nello stesso file sono un costo di
processo che il lavoro non giustifica. Una issue con quattro sezioni nominate ha lo stesso contenuto e
un solo giro.

⚠️ **#1515 non entra in questo cluster** pur essendo lo stesso file: e' gia' fatta (§3.1).

### 3.4 🔴 Il livello sbagliato — le milestone da `v0.5` a `v1.0`

```bash
gh issue list --state open --limit 400 --json number,milestone \
  --jq '[.[] | select((.milestone.title // "") | test("v0[.][5-9]|v1[.]0"))] | length'   # 46
```

Quarantasei issue, di cui sei epic e quaranta capitoli. Nessuna di esse e' un candidato di lavoro
mentre `v0.1` ha ancora il proprio perimetro aperto:

```bash
gh issue list --state open --limit 400 --milestone "v0.1 — Offline Vertical Slice" \
  --json number --jq 'length'   # 89
```

L'epic di ciascuna milestone **esiste gia'** ed e' il contenitore giusto: i capitoli vivono nel suo corpo
come elenco che linka la issue chiusa, e tornano aperti quando la loro milestone comincia.

⚠️ **E l'epic non lo subisce: lo chiede.** #778 porta gia' la frase *«Non nasce una issue per questo, e
non e' una svista»*. Verificato che ne' #773, ne' #776, ne' #778 dichiarano una regola contraria.

### 3.4.1 ⌫ La stessa leva applicata a BAL e Wiki PF — RITIRATA

La prima stesura estendeva il ragionamento a due cluster fuori milestone, entrambi aperti in blocco da un
solo PR di roadmap: la scala **BAL** (`#2566` … `#2575`, epic #2565) e le onde **Wiki PF** (`#821` …
`#828`, epic #422). Sembravano la stessa forma: un epic, N capitoli sequenziali, nessuno in lavorazione.

🔴 **Entrambi gli epic vietano esplicitamente il collasso**, e la verifica e' arrivata prima
dell'esecuzione:

| Epic | La regola, testuale |
|---|---|
| **#422** | *«Ogni wave e' una issue indipendente. Nessun big bang: e' una regola dell'handoff, non una preferenza.»* |
| **#2565** | *«Ogni stadio `BAL 0.1 … BAL 1.0` ha un'issue di stadio che **nomina il proprio owner** — esistente o dichiarato assente.»* |

E la misura conferma che non sono lavoro remoto: **#2566** (`BAL 0.1`) dichiara *«Dipende da: nulla. E'
la base della scala»* con il proprio dato gia' consegnato da #1950, e **#821** (`Wave 1`) porta un
perimetro di pagine misurato. Sono azionabili **adesso**.

∴ **Nessuna delle diciotto e' stata toccata.** L'indice che era gia' stato appeso a #2565 e #422 e' stato
**rimosso**, perche' annunciava una chiusura che non avviene — e #422 aveva gia' una sezione *«Stato
delle wave»* che quell'indice duplicava.

🔑 **La lezione, che vale oltre questo caso**: due cluster con la stessa *forma* non hanno la stessa
*natura*. La differenza non stava nella struttura — stava in una riga scritta nell'epic, e si vedeva solo
aprendolo. Una leva strutturale non e' autorizzata dalla struttura.

---

## 4. Gli scenari

```bash
find Scenarios -name '*.json' | wc -l    # 133
```

### 4.1 Nessuno e' morto — e questo corregge un'euristica

Il primo pass ha cercato quali file di `Scenarios/` nessuno nomina, e ha concluso *«orfani»*. **La
conclusione era sbagliata.** `FRTScenarioCorpusRunsTest` itera `URTScenarioIndex::ListIds(...)`, che
risale da `IFileManager::FindFilesRecursive` (`RTScenarioIndex.cpp:129`): **ogni** file del corpus gira,
nominato o no.

🔑 La misura giusta non e' *«questo file e' eseguito?»* — lo e'. E' *«qualcosa dice cosa difende?»*.

### 4.2 ⌫ RITIRATA — «sei scenari senza specifica» era un artefatto della misura

**La prima stesura di questa sezione affermava un difetto che non esiste, e va ritirata per intero.**

La misura era:

```bash
git grep -l "<NomeScenario>" -- ':!Scenarios'
```

e restituiva `0` per sei file — `AreaGuardFromImpactCenterKeepsPool`, `BaseShieldStopsDirectNotHazard`,
`PhaseGuardShieldExpiresBeforeTheNextTurn`, `InteractWithoutDoorChangesNothing`,
`AllySpottingLetsYouShoot`, `StayingKeepsContact` — e `1` per `AreaGuardFromImpactCenterIsBypassed`.
Da li' la conclusione: *«girano e nessun documento dice a cosa servono»*.

🔴 **Il difetto era nel `-- ':!Scenarios'`.** Escludendo la cartella, la misura escludeva **esattamente
il posto in cui la specifica vive**. Aperti i file, ciascuno porta la propria motivazione in campi
dedicati:

| Campo | Cosa contiene | Esempio misurato |
|---|---|---|
| `_nota` | la regola difesa | *«lo scudo base ferma il danno DIRETTO e NON il danno da terreno»* (`BaseShieldStopsDirectNotHazard`) |
| `_nota_perche_esiste` | perche' il file non e' ridondante | *«senza questo, un'implementazione che rifiuta SEMPRE gli attacchi passerebbe l'altro scenario»* (`AllySpottingLetsYouShoot`) |
| `_nota_come_si_falsifica` | la mutazione dichiarata **prima** di eseguirla | *«in `ARTUnit::ExpireTemporaryShield()` COMMENTARE le due righe del corpo. ⚠️ Non un `return` anticipato»* |
| `_nota_verifica` | l'esecuzione, con la sua evidenza | *«ESEGUITO il 2026-08-30 … PASS, 6/6 assertion, `stateHash 1c898784`»* |
| `_nota_limite_dichiarato` | cio' che il file **non** prova | *«nessun oracolo del formato legge lo stato di un bordo»* (`InteractWithoutDoorChangesNothing`) |

🔑 **E i sei non sono un residuo: sono gemelli di controllo.** Ciascuno esiste perche' il proprio gemello
positivo non dimostri meno di quanto sembra — `AllySpottingLetsYouShoot` sta a
`CannotShootWhatYouCannotSee` come `StayingKeepsContact` sta a `MovingBreaksContact`. Sono la difesa
contro la vacuita', cioe' il contrario del difetto che questa sezione credeva di aver trovato.

∴ **Nessuna azione.** Un documento esterno che li nominasse sarebbe una **seconda verita'** da tenere
allineata a mano — la stessa classe di difetto che `RTScenarioLoader.cpp` argomenta contro quando rifiuta
una costante locale `0..1` al posto di `URTTurnRules`.

⚠️ **Perche' la trappola vale la pena di restare scritta**: una misura che esclude una cartella per
igiene sta anche decidendo dove la verita' non puo' stare. Qui la risposta giusta era **dentro**
l'esclusione. E' la stessa forma dei due errori gia' registrati dal referto del 2026-09-02 — contare
occorrenze invece di produttori, e fidarsi di un limite di paginazione.

### 4.3 Lo scenario orfano che falsifica il titolo di una issue

`PhaseGuardShieldExpiresBeforeTheNextTurn.json` non e' nato per caso:

```bash
git log --oneline -- Scenarios/Spec/Combat/PhaseGuardShieldExpiresBeforeTheNextTurn.json
# 5f84e06e test(2381): lo scudo proattivo di Wraith protegge un turno, non due
```

Il titolo di **#2381** dice: *«non ha scenario, ne' voce PIE, ne' seduta»*. **La prima delle tre e' ora
falsa**, e la casella corrispondente nel corpo e' gia' `[x]`.

∴ #2381 non e' chiudibile — restano la voce PIE e la seduta — ma il suo **titolo mente a chi lo legge in
una lista**, che e' il luogo in cui una issue viene scelta o scartata. Va ri-titolata al residuo.

⚠️ **E il titolo aveva una seconda affermazione falsa**, che la prima stesura di questo referto non aveva
visto: nomina `Hero.Wraith.PhaseGuard`, e quell'identita' e' stata ritirata da #2491.

```bash
git grep -oh "Hero\.\(Ivrin\|Wraith\)\.PhaseGuard" -- Source Data Scenarios | sort | uniq -c
# 18 Hero.Ivrin.PhaseGuard
```

Zero occorrenze del nome che il titolo porta. Un titolo che nomina un'identita' ritirata manda chi cerca
a grep-are una stringa che non esiste.

🔑 Questa e' una **classe**, non un caso — ma e' piu' stretta di quanto sembrasse:

- ✅ **#2826** appartiene alla classe. Il corpo dichiara *«la premessa principale di questa issue e'
  caduta: la porta esiste»*, e il titolo continua a dire *«`SelectAbilityForCurrent` e' privata e non
  `UFUNCTION`»*. Ri-titolata al residuo reale, che e' **leggibilita' del dock** (scope 3, 4, 6, 7) e non
  raggiungibilita' dell'azione.
- ⌫ **#2793 NON vi appartiene**, e la prima stesura di questo referto sbagliava a includerla. Il suo
  corpo dice *«il blocco non e' caduto, si e' spostato su BLIND-2 → OBS-1»*: a spostarsi e' il
  **bloccante**, non la premessa. Il difetto che il titolo enuncia e' ancora vero —
  `URTHexSimLibrary::BlockedCellsFor` (`Turn/RTHexSimLibrary.cpp:31`) scorre `Snapshot.Occupancy` per
  intero, e l'unico criterio di esclusione resta *«non sono io»*. **Nessuna modifica.**

∴ Un titolo si riscrive quando **enuncia** un fatto che la misura ha smentito. Non quando cambia cio' che
lo blocca: quello e' lavoro di dipendenze, e vive nel corpo.

---

## 5. I nove sospetti, misurati uno per uno — e nessuno si chiude

L'incrocio branch↔issue produce candidate con un `feat(N)`/`fix(N)` mergiato **col titolo che
corrisponde**. Il referto del 2026-09-02 aveva gia' avvertito che quello e' un **sospetto**, non un
verdetto. Misurate:

| Issue | Il codice c'e'? | Cosa resta | Chiudibile? |
|---|---|---|---|
| #2875 | ✅ `RTVeilTransition` + `feat(2875)` mergiato | il corpo lo dichiara da solo: *«nessun blocco resta sul codice. L'unico residuo e' la **seduta Editor**»*, piu' una voce PIE nuova accanto a `PIE-HEX-VIZ-VELO` | ❌ |
| #2402 | ✅ `ERTMoveOutcome::Fell` in `RTTurnManager_Blast.cpp`, `Fall.OutcomeIsFellNotDisplaced` esiste | la DoD chiude con **«suite VALIDA»** e una verifica di mutazione via `tools/mutation/caduta-gate.py` | ❌ |
| #2742 | ✅ `feat(2742)` mergiato | la DoD esige **#1941 chiusa** (che e' aperta) e la voce PIE scritta *con* la feature | ❌ |
| #2596 | ✅ `feat(2596)` mergiato | da rimisurare contro la DoD | ❌ non concluso |
| #2697 | ✅ montato in PR #2834, con oracolo verde e rosso sull'asset precedente | il corpo lo dichiara: *«Resta la sola voce `HUMAN-SIGNED`: la seduta PIE»*, e ⛔ *«chi ha scritto la correzione non ne firma da solo il verdetto»* | ❌ |
| #2849 | ✅ `fix(2849)` mergiato, caselle `[x]` | 🔴 *«Il punto aperto, che e' una **decisione** e non un fix: cosa rimpiazza il guardiano»* | ❌ |
| #2501 | ✅ entrambi i test nominati esistono | la DoD chiude con **«suite VALIDA»** e il corpus golden da rigenerare se l'esito cambia | ❌ |
| #2341 | ⚠️ `UsedByBlast` presente, il perimetro allargato da verificare | anti-vacuita' dichiarata, corpus golden | ❌ non concluso |
| #2826 | ✅ la porta esiste (`RTPlayerController.h:743`) | ⚠️ `NOT RUN`: i cinque test `HudViewModel.*` sono in una corsia che non compila. Piu' gli scope 3, 4, 6, 7 | ❌ — **ri-titolata**, §4.3 |

> **Il verdetto**: **nessuna delle nove si chiude misurando il sorgente.** Ogni residuo e' una di tre
> cose — una **seduta Editor/PIE**, una **decisione**, o una **suite da eseguire**. E' esattamente la
> proporzione che il referto del 2026-09-02 aveva misurato (*«quarantatre su cinquantasei chiedevano di
> guardare uno schermo»*), ritrovata su un campione diverso.

🔑 **∴ La leva «stale-open» e' molto piu' piccola di come si presenta.** Un `feat(N)` mergiato col titolo
giusto **somiglia** a una chiusura e quasi mai lo e'. L'unica chiusura di questo passaggio — #1515 — non
veniva da questo incrocio: veniva dal codice, che la citava per nome.

## 6. Cosa resta `NOT RUN`

- **Compile / Automation / PIE / packaged.** Nessun gate eseguito: il passaggio non tocca `Source/`.
  ⚠️ Al momento della misura il motore era **libero** (nessun processo `UnrealEditor*` o `LiveCoding*`),
  quindi il `NOT RUN` e' una scelta di perimetro, non un blocco.
- **La verifica di mutazione di #1515**, dichiarata nel commento di chiusura invece che presunta.
- **#2596 e #2341**: sospetti la cui DoD non e' stata percorsa voce per voce.
- **Il contenuto dei 133 scenari.** Misurata l'esecuzione (§4.1) e l'auto-documentazione (§4.2), non
  la correttezza di ciascuna assertion.

---

## 7. Cio' che e' stato eseguito

Ogni chiusura porta un commento con la misura che la giustifica. Nessun corpo e' stato cancellato.

| Azione | Issue | Ragione |
|---|---|---|
| **Chiusa** `completed` | #1515 | §3.1 — il codice la soddisfa e la cita per nome |
| **Chiusa** duplicato | #2658 | §3.2 — trentotto secondi da #2657; la sua misura e' stata **trasferita** nel corpo di #2657 |
| **Commentata** | #2657 | assorbe i tre dati che solo #2658 aveva |
| **Consolidata + ri-titolata** | #2698 | §3.3 — assorbe #2699, #2701, #2702 in quattro sezioni nominate |
| **Chiuse** confluite | #2699 · #2701 · #2702 | §3.3 — ⛔ i difetti **non** sono risolti: e' un consolidamento di contenitore, e `ParseCell` e' stato ri-verificato invariato prima di chiudere |
| **Chiusi** come capitoli | i quaranta di `v0.5`→`v1.0` (#779-#785, #786-#791, #792-#796 + #1621, #797-#802, #803-#806 + #1604, #807-#816) | §3.4 |
| **Indicizzati** | gli epic #773 · #774 · #775 · #776 · #777 · #778 | ciascuno porta ora l'elenco dei propri capitoli chiusi, col comando per riaprirli |
| **Ri-titolata** | #2381 | §4.3 — due affermazioni del titolo erano false: lo scenario esiste, e l'identita' e' `Ivrin` |
| **Ri-titolata** | #2826 | §4.3 — il titolo asseriva la premessa che il corpo dichiara caduta |
| ⌫ **Annullata** | l'indice appeso a #2565 e #422 | §3.4.1 — rimosso: annunciava una chiusura che non avviene |
| ⛔ **Nessuna azione** | #2793 · i dieci `BAL` · le otto `Wave` · i sei scenari | §3.4.1, §4.2, §4.3 — tre conclusioni ritirate dalla misura |

## 8. Verification

- Compile: `N/A` — nessuna riga di `Source/` toccata
- Tests: `N/A`
- Determinism: `N/A`
- Replay: `N/A`
- Privacy: `N/A`
- PIE: `N/A`
- Packaged: `N/A`
- Misura del corpus issue: `PASS` — comandi in §1, §3.4
- Misura dell'esecuzione del corpus scenari: `PASS` — §4.1
- Misura di #1515 contro il codice: `PASS` — §3.1
- Ri-verifica di `ParseCell` prima di consolidare il cluster: `PASS` — §3.3
- Verifica delle regole degli epic prima di collassarne i capitoli: `PASS` — §3.4, §3.4.1
- Misura dei nove sospetti contro la loro DoD: `PASS` per sette (#2875, #2402, #2742, #2697, #2849, #2501, #2826), `NOT RUN` per due (#2596, #2341) — §5
- Verifica di mutazione di #1515: `NOT RUN` — dichiarata nel commento di chiusura
- Auto-documentazione degli scenari (§4.2): `PASS` — e **ritira** la conclusione della prima stesura

### La misura che si e' corretta da sola

Tre conclusioni sono state ritirate **dopo** essere state scritte, e due di esse **prima** che l'azione
corrispondente venisse eseguita:

| Ritirata | Sbagliava perche' | Fermata prima dell'azione? |
|---|---|---|
| §3.4.1 — collassare `BAL` e `Wiki PF` | due cluster con la stessa **forma** non hanno la stessa **natura**: entrambi gli epic vietano il collasso per iscritto | ✅ si', ma l'indice era gia' stato appeso ed e' stato rimosso |
| §4.2 — «sei scenari senza specifica» | la misura escludeva `Scenarios/`, cioe' **il posto in cui la specifica vive** | ✅ si' |
| §4.3 (parte) — ri-titolare #2793 | a spostarsi era il **bloccante**, non la premessa: il titolo e' vero | ✅ si' |

⚠️ **La correzione di §4.2 e' arrivata solo perche' la leva andava *eseguita*.** Finche' la conclusione
restava scritta in un referto, reggeva; ad aprire i file per nominarli, e' caduta in un minuto. Una
conclusione che nessuno prova a usare non viene falsificata da niente.
