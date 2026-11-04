# La v0.1 giocabile — brainstorming del 2026-11-04

> `SNAPSHOT` · fotografia del 2026-11-04: vale finché è l'ultima misura del suo oggetto.
>
> - **Statuto**: brief di un brainstorming con l'autore (`/sc:brainstorm`). Le scelte di §3 sono sue, ognuna col
>   testo dell'opzione **come l'ha vista**. Le ondate di §5, le conseguenze di §6 e le domande di §7 sono di chi
>   scrive, e **da confermare**.
> - **Misurato su** `origin/main` = `361e2225f`. Le misure di §2 sono state rifatte su questo commit: la base
>   precedente, `f1a56d9b1`, è avanzata con le PR #3978, #3979, #3980 e #3981 (`D-572`…`D-575`).
> - **Decisioni registrate**: nessuna, ancora. §8 dice quali servono.
> - **Conferma dell'autore**: «confermo», il 2026-11-04 (§3.5, e il rapporto confermato in Appendice A).
> - **Owner di dominio**: lo scope è di [`roadmap-v0.1.md`](../roadmap-v0.1.md), i gate di
>   [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md), l'attestazione di
>   [#85](https://github.com/DegrassiAaron/refactor-tactics-main/issues/85). Le due epic che cambierebbero release,
>   E26 ([#326](https://github.com/DegrassiAaron/refactor-tactics-main/issues/326)) ed E35
>   ([#322](https://github.com/DegrassiAaron/refactor-tactics-main/issues/322)), hanno per owner
>   [`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md).
>
> ⛔ Nessuna riga di `Source/` e di `Content/`, e nessun documento owner, cambia con questo brief. Lo scope della
> v0.1 resta quello di `roadmap-v0.1.md` finché il `D-nnn` di §8 non è in `main`: *«Una capability dichiarata dal
> tracker e assente dall'owner dello scope non è scope: è un'aspettativa.»*
>
> Convenzione: niente totali che cambiano da soli. Dove un numero serve, sta accanto al comando che lo produce.

---

## 1. Da dove viene

Dopo un rapporto sullo stato della v0.1 — i gate `G1`–`G14` più `G16` ✅ sul candidate `947a783d5`, con `G9`, `G13`
e `G16` per deroga d'autore ([D-544](../../decisions/RT_PDR_00_Decision_Log.md),
[D-545](../../decisions/RT_PDR_00_Decision_Log.md), [D-546](../../decisions/RT_PDR_00_Decision_Log.md)), e la
release non dichiarata (#85, passo 5) — l'autore ha scritto, alla lettera:

> «l'attuale milestone 0.1 non ha abbastanza feature per considerarsi giocabile. dobbiamo ridefinirla. grafica,
> skill, menu e altro sono mancanti»

Il rapporto misurava la DoD, non la giocabilità. L'unico gate che nomina la giocabilità è `G13`, ed è ✅ per deroga
su una seduta **pilotata dall'assistente con input sintetico**: nessuna persona ha giocato il pacchetto di quel
candidate.

## 2. Com'è oggi

| Che cosa | Com'è | Dove |
|---|---|---|
| Tema della v0.1 | «Il turno simultaneo funziona e si vede»: slice 2v2 offline contro bot | [`roadmap-v0.1-v1.0.md`](../roadmap-v0.1-v1.0.md) §2 |
| Giocabilità nella DoD | Un solo gate ha un criterio che la nomina, `G13` (*«Partita giocabile senza editor dalla build packaged»*; `grep -E '^\| (~~)?\*\*G[0-9]+' docs/roadmap/v0.1-definition-of-done.md \| cut -d'\|' -f3 \| grep -i -c giocabil` → 1). La riserva è scritta dal 2026-08-10: *«e una build che avvia sull'arena di prova soddisfa la lettera ma non la cosa»* | `roadmap-v0.1.md`, E46 ([D-144](../../decisions/RT_PDR_00_Decision_Log.md)); [evidenza del candidate v3](../../technical/evidence/85/misure-candidate-v3-2026-10-26.md) |
| Tetto della presentazione | [D-124](../../decisions/RT_PDR_00_Decision_Log.md) lascia fuori *«VFX completo per tutti gli status, Niagara per ogni abilità, foot IK raffinato, locomotion set bespoke, cinematic death, e ogni presentation framework che nessun gate v0.1 misura»* | `roadmap-v0.1.md`, E21 |
| Ambiente | Solo asset `Graybox` (cover, porte, superfici ghiaccio e acqua) e un Blueprint di presentazione. I DataAsset di mappa stanno sotto `Maps/Dev` e `Maps/Templates` | `find Content/RT/World Content/RT/Maps -name '*.uasset' -o -name '*.umap'` |
| Personaggi | I quattro `BP_Unit_*` referenziano soli asset `Characters` dei pack Paragon, i due anelli e `ABP_RTUnitAuthored`; nessun riferimento forte a cartelle `FX` o `Sounds`. I ruoli di presentazione (Idle, Move, Cast, Attack, Hit, Death) e le varianti per azione esistono in C++; la morte visiva è differita | `grep -a -o -E '/Game/[A-Za-z0-9_/.-]+'` sui `.uasset` (solo riferimenti forti); `Source/RefactorTactics/Unit/RTUnitAnimInstance.h`; E21.2 |
| VFX e suoni nel C++ | Nessun riscontro fuori dai `Tests/`. ⚠️ Per regola VFX e audio vivono nei Blueprint e nei dati: il C++ vuoto è atteso e **non prova** l'assenza. I Blueprint non sono stati aperti (`unreal-mcp` non connesso in sessione) | `git grep -l -E 'UNiagara\|NiagaraComponent\|SpawnSystemAtLocation\|SpawnSystemAttached\|UParticleSystem\|SpawnEmitter\|USoundBase\|USoundCue\|PlaySound\|SpawnSound\|UAudioComponent\|MetaSound' -- 'Source/*.cpp' 'Source/*.h' ':(exclude)Source/RefactorTactics/Tests' ':(exclude)Source/RefactorTacticsEditor/Private/Tests'` → nessun riscontro; controllo positivo con `UAnimInstance` → riscontri |
| Pack Paragon degli eroi | Animazioni, suoni e FX in **Cascade** (`P_*`) per ciascuno; nessun Niagara (`NS_*`). Cascade è il sistema legacy di UE, deprecato a favore di Niagara: la sua resa in 5.8.1 **non è misurata** | `find Content/FabAsset/Paragon/ParagonRiktor/FX -name 'P_*.uasset'`, e gli altri tre pack |
| Pack degli eroi nuovi | Per le schede di [`docs/characters/v0.2/`](../../characters/v0.2/) il progetto ha `ParagonMurdock` e `ParagonSteel`; **non ha** pack per `aurora` e `kwang`. I quattro pack degli eroi attuali ci sono | `ls Content/FabAsset/Paragon \| grep -i -E 'aurora\|kwang\|steel\|murdock'` → `ParagonMurdock`, `ParagonSteel`; controllo positivo con `gadget\|phase\|riktor\|wraith` → i quattro |
| Combat feedback | Evento → cue: [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454), #2456 e #2457 aperte (`P2`); #2455 (numeri di danno) chiusa | `gh issue view` |
| Audio | Nessuna issue con «audio» nel titolo, aperta o chiusa; zero occorrenze in `roadmap-v0.1.md` e nella DoD | `gh issue list --search "audio in:title" --state all`; `grep -c -i -E 'audio\|suono\|sound\|musica' docs/roadmap/roadmap-v0.1.md docs/roadmap/v0.1-definition-of-done.md` → 0 e 0 |
| Frontend | `PLAY · SETTINGS · QUIT`; `SETTINGS` ammesso come pannello «coming soon»; *«Match Setup v0.1 è preconfigurato — non c'è nulla da scegliere finché esiste un solo formato e una sola mappa»*. Settings reali, Briefing e Training Lite sono v0.2; Onboarding è v1.0 | `roadmap-v0.1.md`, CP 46.3 e 46.4; `roadmap-post-v0.1.md`, «Il frontend oltre la v0.1» |
| Scelte pre-partita | Il roster si imposta da `Team0Heroes` e `Team1Heroes` del GameMode; il bootstrapper rifiuta un `HeroId` sconosciuto (`RosterHeroMissing`); un resolver sui roster è in `main` (PR #3717) con il verdetto a schermo ancora aperto (#2748). Nessuna UI di scelta | #2748 |
| Roster | I `BP_Unit_*` si chiamano Aevik, Branth, Ivrin e Muiren e usano nell'ordine i pack Gadget, Riktor, Wraith e Phase; a schermo, per [D-120](../../decisions/RT_PDR_00_Decision_Log.md), Gadget · Phase · Riktor · Wraith. La migrazione dell'identità (#2297) è differita alla v0.2 da [D-484](../../decisions/RT_PDR_00_Decision_Log.md) (2) | riferimenti dei `.uasset`; `roadmap-v0.1.md`, E21 |
| Bot | Utility scoring; la competenza per eroe e capability è dati in [`bot-competence.yaml`](../bot-competence.yaml) ([D-102](../../decisions/RT_PDR_00_Decision_Log.md)); E26 sta nella v0.2 | `roadmap-post-v0.1.md`, E26 |
| PIA | `Playable Integration & Acceptance` (#2623, PIA-1…PIA-6 aperte) porta il slice «da "le feature esistono" a giocabile», ma **accetta ciò che c'è**: non aggiunge arte, menu o audio | [`roadmap-pia.md`](../roadmap-pia.md) §1 |

## 3. Le scelte dell'autore

Tre tornate di domande, il 2026-11-04. Le citazioni sono il testo dell'opzione scelta, come l'ha vista. Le etichette
«(Recommended)» erano dell'assistente: l'autore ha seguito la raccomandata in «Forma» e in «Fonte degli asset»; in
«Contenuto» e in «Bot» ha scelto l'opzione più grande, che non portava l'etichetta.

### 3.1 Il traguardo

| Domanda | Scelta | Il testo dell'opzione | Le altre |
|---|---|---|---|
| Per chi deve essere giocabile la v0.1? | **Solo per me** | «Una build che giochi tu dall'inizio alla fine per giudicare se il gioco diverte, anche se ancora grezza. È la soglia più bassa.» | «Pochi tester», «Pubblico / showcase» |
| Quale giro deve compiere chi gioca, dall'avvio alla fine? | **Con scelte** | «Prima della partita si scelgono eroi e/o mappa: servono un Match Setup e più di una mappa (oggi non c'è nessuno dei due).» | «Preconfigurato», «Con scelte e tutorial» |

### 3.2 Grafica e skill

| Domanda | Scelta | Il testo delle opzioni scelte |
|---|---|---|
| Quali buchi grafici pesano di più? | **tutti e quattro** | «Mappa e ambiente: oggi graybox: cover, porte e superfici grigie su mappe di sviluppo. Arte d'ambiente, luci, materiali.» · «VFX e impatti: oggi fuori da D-124 («Niagara per ogni abilità»); i Blueprint delle unità non referenziano cartelle FX.» · «Personaggi e animazioni: Paragon con i ruoli di presentazione Idle/Move/Cast/Attack/Hit/Death; la morte visiva è differita (E21.2).» · «HUD e schermate: stile, icone e layout dei widget di partita e di frontend.» |
| Cosa intendi con «skill mancanti»? | **tutti e quattro** | «Più contenuto: più skill per eroe o più eroi giocabili (oggi quattro con kit a dati; il roster a otto è v0.2).» · «Si vedono e si sentono: ogni skill ha animazione, VFX e suono distinti, leggibili a colpo d'occhio.» · «Si usano da sole: scegliere, mirare, vedere l'anteprima e capire l'esito (dock, targeting, tooltip) senza spiegazioni.» · «Hanno profondità: scelte interessanti fra skill: varianti, counterplay, bilanciamento.» |

### 3.3 Che cosa si sceglie, che cosa manca ancora, da dove arriva il materiale

| Domanda | Scelta | Il testo delle opzioni scelte | Le altre |
|---|---|---|---|
| Se potessi migliorare una sola cosa prima della prossima partita, quale sceglieresti? | **Ambiente della mappa** | «Arte d'ambiente, luci e materiali al posto del graybox.» | «Skill viste e sentite», «Scelta eroi e mappa», «HUD e schermate» |
| Cosa si sceglie prima della partita? | **tutti e quattro** | «Eroi: la squadra (e se serve quella avversaria) nel vincolo del 2v2; oggi è una proprietà del GameMode, senza UI.» · «Mappa: almeno due mappe giocabili; oggi i DataAsset di mappa stanno tutti in Maps/Dev o Maps/Templates.» · «Avversario: livello o stile del bot che affronti.» · «Obiettivo e durata: condizione di vittoria e limite di round (oggi parametri del formato).» | — |
| Cos'altro manca perché sia un gioco e non un banco di prova? | **Audio** · **Partite più vere** | «Audio: musica ed effetti; oggi zero occorrenze nella roadmap e nella DoD della v0.1.» · «Partite più vere: bot più credibile, obiettivi, varietà fra una partita e l'altra.» | Lasciate fuori: «Settings reali», «Persistenza» |
| Da dove arrivano animazioni, VFX e suoni? | **Misto** | «Parti dai pack Paragon già nel progetto e sostituisci dove non reggono: il costo iniziale è collegarli alle skill, non produrli.» | «Solo Paragon», «Asset nuovi» |

### 3.4 La forma, il contenuto, il bot, l'arte della mappa

Prima di queste quattro l'autore aveva letto la tabella delle aree e quattro nodi: le collisioni con altre release, skill
e bot che si muovono insieme, le scelte pre-partita come stato canonico, l'audio che parte da zero.

| Domanda | Scelta | Il testo dell'opzione | Le altre |
|---|---|---|---|
| Che forma diamo alla nuova definizione? | **Allargare la v0.1** | «Stesso nome e stesse epic #14, #26 e #85; scope e DoD riscritti con un D-nnn; ondate verticali nell'ordine che hai scelto, ciascuna chiusa da un tuo giudizio. Il v3 (947a783d5) resta l'ultimo candidate della vecchia definizione e la rimisura si fa una volta sola, alla fine.» | «Spezzare in due release» |
| Quanto contenuto di gioco in più? | **Roster a 8 (E35)** | «Anticipa E35 dalla v0.2 (Sentinel Directorate e Resonance): l'opzione più grande, con un D-nnn di spostamento.» | «Più skill, stessi 4 eroi», «Roster a 6» |
| Fin dove arrivano «Avversario» e «Partite più vere»? | **Anticipare E26** | «Tactical Bot v1 (#326, coordinazione di squadra) entra nella v0.1: l'opzione più grande, con un D-nnn di spostamento.» | «Livelli sul bot attuale», «Bot che gioca tutte le skill» |
| Da dove arriva l'arte d'ambiente? | **Look da materiali** | «Materiali, luce e pochi mesh su griglia esagonale: poco costo di asset, più lavoro tecnico e uno stile da definire.» | «Props già nel progetto», «Kit di terzi (Fab)», «Produzione propria» |

La preview che accompagnava «Allargare la v0.1», come l'autore l'ha vista:

```
v0.1 (ridefinita) — bozza
  ondata 1  Ambiente
  ondata 2  Skill viste/sentite + audio
  ondata 3  Scelte pre-partita (+ 2ª mappa)
  ondata 4  Contenuto, bot, partite più vere
  ondata 5  HUD/schermate + giro finale
  └─ G1–G14 e G16 rimisurati sul candidate finale
     + G17: tuo verdetto sulle partite giocate
v0.2 … v1.0  invariate,
               salvo epic spostate con D-nnn
```

### 3.5 La conferma

Il rapporto con cui è finita la sessione di brainstorming conteneva «La regola è mia, confermala o toglila.» (la regola
di scorrimento, §7) e proponeva, sotto «Prossimo passo, se confermi», la PR di questo brief, l'issue di tracking, il
`D-nnn` e l'audit per area. È in **Appendice A**, alla lettera. L'autore ha risposto, alla lettera:

> «confermo»

**Lettura dell'assistente, ribaltabile** (come [D-547](../../decisions/RT_PDR_00_Decision_Log.md)):
- la risposta **autorizza i quattro passi** e **conferma la regola di scorrimento**, perché seguiva «confermala o
  toglila»;
- **accetta lo spostamento di E26 ed E35** *sapendo* le dipendenze che il rapporto nominava (§6, prima parte): non è
  la scelta fatta a occhi chiusi del §3.4;
- **non risponde** alle domande di «Da decidere tu»: la forma di `G17`, lo stile dell'ambiente, il tetto di E26 e che
  cosa esce dalla v0.1 restano **aperte**;
- **non copre** ciò che il rapporto non conteneva: il dato dei pack mancanti per `aurora` e `kwang` (§6) è stato
  misurato **dopo**, e va detto all'autore come tale.

## 4. La definizione proposta *(di chi scrive, da correggere)*

La v0.1 è una build packaged in cui l'autore sceglie eroi, mappa, avversario e obiettivo e durata, gioca un 2v2 su una
mappa con arte d'ambiente, vede e sente ogni skill, e scrive un verdetto sulle partite giocate.

- **Un gate nuovo, `G17`**: il verdetto d'autore sulla build candidate. Il primo numero libero è `G17`
  (`grep -c -E '\*\*G(1[7-9]|2[0-9])\*\*' docs/roadmap/v0.1-definition-of-done.md` → 0); `G15` è ritirato e il
  numero non si riusa. Il giudizio «si gioca bene» è dell'autore; la meccanica (build, suite, packaging,
  determinismo, privacy) la misura chi scrive, e chi ripara non firma. Com'è fatto il verdetto è aperto (§7).
- **`G13`** («partita giocabile senza editor dalla build packaged») va riscritto o assorbito: oggi si chiude con una
  seduta senza persona.
- **`G1`–`G14` e `G16`** si rimisurano sul candidate finale: le misure sul v3 non si riportano.

## 5. Le ondate *(bozza di chi scrive; l'ordine di partenza, Ambiente per primo, è dell'autore)*

Le ondate 1–5 sono quelle della preview di §3.4. Chi scrive propone di spezzare la 4 in **4a Bot** e **4b Roster a 8**:
sono le due più grandi e la seconda dipende dalla prima. La scelta degli eroi (ondata 3) nasce con i quattro che ci
sono e si allarga con la 4b, perché il roster è dato.

| Ondata | Che cosa | Riusa (Search → Reuse) | Esce con |
|---|---|---|---|
| **1 · Ambiente** | Look da materiali su griglia esagonale, una mappa; superfici riconoscibili senza console debug; la leggibilità tattica non si sacrifica | contratto di posa ([`spec-graybox-placement-contract.md`](../../technical/systems/spec-graybox-placement-contract.md)); #2492, #2330, #1095 | la mappa non si legge come graybox; verdetto breve dell'autore; `Client FPS` da rimisurare |
| **2 · Skill viste e sentite + audio** | Dorsale evento → cue; clip per azione dei quattro eroi, morte visiva inclusa; FX e suoni dai pack, Cascade da misurare in 5.8.1; audio (musica, UI, effetti) come epic nuova, senza owner | #2454, #2456, #2457; difetti di posa #1793 e #2167; i ruoli di `URTUnitAnimInstance` | un pilota su **un** eroe completo prima di replicare sugli altri tre |
| **3 · Scelte pre-partita** | Match Setup su eroi, mappa (con la seconda), avversario, obiettivo e durata; passa dal bootstrapper esistente, nessun secondo percorso d'avvio (CP 46.4); le scelte entrano nel manifest e nel replay | resolver sui roster (#2748); `RosterHeroMissing`; [`spec-manifesto-showcase.md`](../../technical/tooling/spec-manifesto-showcase.md) | da Main Menu si configura e si avvia; il replay registra le scelte |
| **4a · Bot** | Completare E13 (#151, #159, #160, #1466), poi E26 (#326) con un tetto scritto (§6) | `bot-competence.yaml` | il bot certificato sulle skill del roster di quel momento |
| **4b · Roster a 8** | E35 (#322): 35.1–35.5, identità (#2291, #2297), bilanciamento a 8, bot certificato sugli otto | l'ondata 2 come modello di collegamento dei pack | `Heroes.RosterIsBalanced` esteso a 8; presentazione per i quattro eroi nuovi |
| **5 · HUD, schermate, giro finale** | Un solo linguaggio visivo su menu, setup, HUD e result; PIA riallineato; rimisura di `G1`–`G14` e `G16`; `G17` | #1330; PIA (#2623) | il verdetto d'autore |

## 6. Che cosa porta con sé la scelta

⚠️ **Misurato dopo che l'autore ha scelto** «Roster a 8» e «Anticipare E26». Il testo che ha visto non conteneva queste
dipendenze: nominava «un D-nnn di spostamento» e «l'opzione più grande». Se cambiano il conto, la scelta si rivede;
qui sono scritte come sono.

- **E35** dichiara una sola dipendenza, *«E6 (roster 4) chiusa»*. Però:
  - il suo CP 35.5 (*Paragon naming purge*, #2291) porta dentro l'identità dei roster, che
    [D-484](../../decisions/RT_PDR_00_Decision_Log.md) (2) aveva differito alla v0.2 insieme a #2297;
  - le schede dei quattro eroi nuovi in [`docs/characters/v0.2/`](../../characters/v0.2/) si chiamano ancora `aurora`,
    `kwang`, `murdock` e `steel`, mentre E35 nomina Ward e Vigil (Sentinel Directorate), Rime e Tethra (Resonance): la
    corrispondenza non è scritta nei file letti;
  - due schede dichiarano dipendenze tecniche: `aurora` *«Dynamic terrain, ice, LOS, movement costs»*, e descrive il
    cambio di geometria dietro di sé aggiungendo *«È una specifica di design, non ancora runtime»*; `kwang`
    *«Persistent object, electricity, water, geometry»*. Che i kit si realizzino con le meccaniche di oggi **non è
    misurato**; `roadmap-v0.1.md` §1 lascia fuori *«il motore del ghiaccio»* (lo slide a catena), che è una cosa
    diversa dal terreno di ghiaccio ma va verificata;
  - ⚠️ **misurato dopo la conferma, non nel rapporto**: il progetto non ha i pack Paragon di `aurora` e `kwang` (ci
    sono `ParagonMurdock` e `ParagonSteel`, §2). Per quei due eroi «Misto» non ha un punto di partenza: la base visiva
    è un acquisto o una produzione ancora da decidere;
  - il rischio dichiarato da E35: *«il roster raddoppia la matrice di interazioni da testare — il costo non è
    lineare»*.
- **E26** dipende da *«E13 (conoscenza parziale)»*, che in v0.1 è `P2` e aperta (#151, #159, #160, #1466) ed è già stata
  allargata dal [brainstorming del 2026-10-14](fog-of-war-ripensata-brainstorm-2026-10-14.md) (resa, memoria, rumori,
  rivelare, tutto dichiarato nella v0.1). La coda di E13 entra nel percorso critico. L'elenco di E26 nomina anche
  *belief weights*, *predictive action scoring* e *stress 4v4*, che la stessa epic dice usciti in feature separate o
  che hanno epic proprie (E17 in v0.1, E32 in v0.4): il tetto di E26 nella v0.1 va scritto.
- **Skill e bot si muovono insieme.** Per [`bot-competence.yaml`](../bot-competence.yaml) ([D-102](../../decisions/RT_PDR_00_Decision_Log.md))
  una capability decisiva `FAIL` o `UNTESTED` lascia dire solo *«il bot non sa giocarla»*: una skill nuova che il bot
  non gioca non fa vedere la profondità. Vale per ogni eroe aggiunto.
- **Le scelte pre-partita sono stato canonico.** Eroi, mappa, avversario, obiettivo e durata devono passare dal
  bootstrapper (CP 46.4: nessun secondo percorso d'avvio) ed entrare nel manifest e nel replay; se il formato dello
  stato iniziale cambia, valgono snapshot, serializzazione, versione e hash.
- **L'audio parte da zero**: nessuna issue, nessun owner in v0.1, e un dominio (Audio & Feedback) che non porta
  checkpoint della release.
- **I gate si spostano.** `G11` (`Client FPS`) va rimisurato dopo l'arte e i VFX: [D-124](../../decisions/RT_PDR_00_Decision_Log.md)
  scrive già *«misurato prima delle mesh definitive misura i cilindri»*. `G9` ha le sue voci `RELEASE-V01`: ogni ondata
  ne porta di nuove. PIA (#2623) o si assorbe o si riallinea.
- **Lo spostamento fra release è una decisione.** Le milestone v0.2, v0.3 e v0.4 scrivono che nessuna loro epic si
  apre prima dei gate della v0.1; E26 ed E35 sono epic della v0.2. Il `D-nnn` deve sciogliere quel vincolo per loro,
  nel modello di [D-160](../../decisions/RT_PDR_00_Decision_Log.md) (E23, anticipata alla v0.1).
- **Cambia il tema, non solo la lista.** `roadmap-v0.1.md` §1 registra tre ampliamenti dopo la stesura (shell di
  frontend, caduta da un bordo aperto, strumenti di prova) e il brief del 2026-10-14 ne dichiarava un altro. Questa
  ridefinizione sostituisce il tema della release, «il turno simultaneo funziona e si vede».

## 7. Domande ancora aperte

Non sono state poste in questa sessione. Si chiudono pezzo per pezzo, col pezzo.

- **`G17`**: quante partite, che forma ha il verdetto, dove si scrive (il registro è
  [`test-manuali-pie.md`](../../technical/test-manuali-pie.md)); se una seconda persona deve giocare prima della
  dichiarazione, come chiedeva `G13`.
- **Regola di scorrimento** *(proposta di chi scrive)*: ogni ondata lascia una build che si gioca, e ciò che non è
  chiuso quando l'autore dichiara scorre nella v0.2 con un `D-nnn`, non in silenzio. Da confermare o togliere.
- **Ambiente**: lo stile del look da materiali (palette, atmosfera, riferimenti); quanti tipi di superficie; la seconda
  mappa.
- **Match Setup**: doppioni ammessi; se si sceglie anche la squadra avversaria; quali preset di obiettivo e durata; che
  cosa significa «livello» del bot.
- **Audio**: musica originale o libreria; mix minimo; a quale ondata.
- **E35**: la corrispondenza schede ↔ nomi RT; se i kit di `aurora` e `kwang` si realizzano con le meccaniche di oggi;
  quando si fa la purga dell'identità.
- **Eroi senza pack** (`aurora`, `kwang`): da dove arriva la base visiva; se E35 entra comunque con soli due eroi
  nuovi finché i pack non ci sono.
- **E26**: il tetto nella v0.1.
- **Che cosa esce.** Nessun taglio è stato chiesto. Il lavoro `P2` e `P3` già aperto nella milestone resta tagliabile
  per [D-377](../../decisions/RT_PDR_00_Decision_Log.md); decidere se la ridefinizione ne usa il margine.
- **Cascade** in 5.8.1: si misura prima di contare sugli FX dei pack.

## 8. Le decisioni da registrare

Nessuna è ancora nel Decision Log. Si registrano col primo merge che le implementa, col testo di §3 e le correzioni di
§6 con il loro nome. Il numero si prende su misura al momento: vale al merge, non alla misura.

1. **Il traguardo**: la v0.1 è una build giocabile dall'autore («Solo per me»), col giro «Con scelte».
2. **Lo scope per area**: grafica, skill, scelte pre-partita, audio, partite più vere. Emenda
   [D-124](../../decisions/RT_PDR_00_Decision_Log.md) (tetto della presentazione: cade per «Niagara per ogni abilità» e
   per la morte visiva, che le opzioni scelte nominano) e CP 46.4 di [D-144](../../decisions/RT_PDR_00_Decision_Log.md)
   (*«Match Setup v0.1 è preconfigurato»*). **Non** emenda CP 46.3, perché «Settings reali» non è stato scelto e
   `SETTINGS` «coming soon» resta valido; **non** tocca la riga «Fuori scope v0.1» di `roadmap-v0.1.md` §1
   (multiplayer in rete, 4v4, GAS, progressione, modding, editor di mappe a runtime), che nessuna scelta nomina.
3. **Lo spostamento di E26 (#326) e di E35 (#322)** dalla v0.2 alla v0.1, con #2291 e #2297 che rientrano (ribalta
   [D-484](../../decisions/RT_PDR_00_Decision_Log.md) (2) per E35.5).
4. **La forma**: v0.1 allargata, a ondate. Il v3 (`947a783d5`) resta l'ultimo candidate della vecchia definizione;
   [D-547](../../decisions/RT_PDR_00_Decision_Log.md) resta com'è e la release non si dichiara su di esso.
5. **`G17`** e la riscrittura di `G13`.
6. **La fonte degli asset**: pack Paragon come partenza, sostituiti dove non reggono; l'ambiente come look da
   materiali.

Da aggiornare nello stesso giro: `roadmap-v0.1.md` (§1 e §3), `v0.1-definition-of-done.md` (§3 e banner),
`roadmap-post-v0.1.md` (E26, E35 e «Il frontend oltre la v0.1»), `roadmap-v0.1-v1.0.md` (§2), le descrizioni delle
milestone v0.1 e v0.2, i corpi di #14, #26 e #85, la milestone di #326 e #322, e PIA (#2623).

## 9. Vincoli che non cambiano

- **Autorità.** La presentazione consuma gli eventi della simulazione: animazione, VFX e audio non decidono
  movimento, colpo, danno, KO o esito.
- **Determinismo.** Le scelte pre-partita configurano lo stato iniziale canonico; i bot usano le stesse regole del
  giocatore.
- **Privacy.** Il Match Setup e l'avversario non espongono pianificazione nemica.
- **Leggibilità prima del bello.** Nell'ordine delle priorità del progetto la chiarezza per il giocatore sta sopra la
  velocità di implementazione: l'arte non cancella cover, quote e nebbia.
- **Search → Reuse → Create.** Le issue di §5 si riusano; le epic nuove prendono un numero libero misurato all'atto.
- **Chi ripara non firma.** Il verdetto è dell'autore; la misura della meccanica avviene dopo, su un commit dichiarato.

## 10. Prossimi passi

Dopo la conferma di §3.5 partono, in quest'ordine: la PR di questo brief, l'issue di tracking, il `D-nnn` (numero da
misurare al merge) e l'audit per area. **Nulla si unisce senza un «unisci» dell'autore.**

1. ~~L'autore conferma o corregge §4–§7.~~ Confermato (§3.5); restano aperte le domande di §7.
2. Issue di tracking, e `D-nnn` col testo di §3 e di Appendice A.
3. Audit per area, esistente contro ondata, per mappare le issue già aperte prima di crearne di nuove.
4. **Dopo** il merge del `D-nnn`: si aggiornano i documenti di §8 e le issue su GitHub (milestone di #326 e #322,
   descrizioni delle milestone, corpi di #14, #26 e #85, PIA). Non prima: il `D-nnn` vale al merge.

## Appendice A — il rapporto che l'autore ha confermato

Testo alla lettera, nell'ordine in cui compariva. Le omissioni sono segnate con […]; le sezioni omesse sono «Verifica»,
«Limiti» e le due note di servizio.

> Il brief è scritto, ma non committato. […] Non è cambiata nessuna riga di `Source/` o `Content/` e non ho registrato
> nessun `D-nnn`. La rimisura A/B che ti avevo proposto è superata.
>
> **Cosa hai deciso** (i testi come li hai visti sono in §3 del brief)
> - La v0.1 è giocabile solo per te, con il giro «Con scelte»: eroi, mappa, avversario, obiettivo e durata.
> - Grafica e skill vanno su tutte e quattro le dimensioni, con l'Ambiente (look da materiali) per primo.
> - Audio e «partite più vere» sì, Settings reali e persistenza no.
> - Gli asset partono dai pack Paragon («Misto»).
> - La v0.1 si allarga a ondate, anticipando E35 (roster a 8) ed E26 (Tactical Bot) dalla v0.2.
> - Il nuovo gate `G17` è il tuo verdetto sulle partite giocate.
>
> **Da sapere prima di confermare** (§6)
> Hai scelto roster a 8 ed E26 prima che misurassi le loro dipendenze:
> - **E35:** trascina l'identità dei roster (`#2291` e `#2297`, che D-484 aveva differito). Due delle quattro schede, `aurora` e `kwang`, dichiarano terreno dinamico e oggetto persistente. Non ho misurato se quei kit si realizzino con le meccaniche di oggi.
> - **E26:** dipende da E13, che è `P2` e ancora aperta. Il suo elenco nomina anche cose che altrove stanno in v0.3 e v0.4, quindi serve un tetto scritto.
> - **Assi insieme:** è la combinazione più grande su più assi con la soglia di qualità più bassa. Per tenere la release chiudibile propongo una regola di scorrimento: ogni ondata lascia una build giocabile, E26 ed E35 vanno per ultime e ciò che non è chiuso scorre in v0.2 con un `D-nnn`, non in silenzio. La regola è mia, confermala o toglila.
>
> **Da decidere tu** (§7)
> - La forma di `G17`: quante partite, dove si scrive il verdetto, se serve una seconda persona.
> - Lo stile dell'ambiente.
> - Il tetto di E26.
> - Cosa esce dalla v0.1.
>
> **Prossimo passo, se confermi**
> - Commit e PR del brief verso `main`.
> - Issue di tracking su GitHub: scrivo su GitHub, quindi ti chiedo l'OK.
> - Il `D-nnn` col testo di §3.
> - Un audit per area, per mappare le issue già aperte prima di crearne di nuove.
