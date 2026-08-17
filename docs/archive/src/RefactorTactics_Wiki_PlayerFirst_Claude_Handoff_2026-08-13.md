> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> `HISTORICAL` · **Sorgente consumato il 2026-08-13** · non autorevole.
>
> **Cosa e' entrato**: l'audit misurato del clone Wiki (§13) e' diventato
> [`../../roadmap/plans/wiki-audit-player-first-2026-08-13.md`](../../roadmap/plans/wiki-audit-player-first-2026-08-13.md);
> le wave (§15) sono le issue **#821**–**#828**; l'epic proposta (§16) **non** e' stata creata nuova —
> ha riscritto **#422**, che era l'owner equivalente gia' esistente, come lo stesso §16 prescrive.
> La riconciliazione di **#757** e' un commento con la relation, non una modifica al suo perimetro.
>
> **Cosa non e' entrato, e perche'**:
> - le **wave 1–8** (riscrittura di ~90 pagine): sono il lavoro che le issue tracciano, non di quella
>   sessione. Lo vieta il documento stesso — «non fare big bang», §15;
> - la **verifica a schermo** (§22): dovuta per le pagine *modificate*, e non ne e' stata modificata
>   nessuna;
> - le **editorial icon** (§12.2): richiedono una decisione che nessun owner ha ancora preso;
> - le **assunzioni §1** su D-076 e D-130 sono state **verificate**, non recepite: erano corrette.
>
> ⚠️ Una prescrizione del documento e' stata **disattesa consapevolmente**: il §0 punto 8 chiede di
> «implementare la ristrutturazione a ondate» nella stessa sessione. Con 91 pagine e 83 387 parole
> misurate, farlo avrebbe significato pubblicare senza review — che il §26 vieta esplicitamente.

---

# REFACTORTACTICS — HANDOFF CLAUDE CODE
## Ristrutturazione Wiki Player-First + Developer Zone + Visual/Icon Grammar

**Data handoff:** 2026-08-13  
**Obiettivo:** trasformare la Wiki pubblicata di RefactorTactics da documentazione prevalentemente tecnica/progettuale a **manuale/enciclopedia player-first**, mantenendo una **Developer Zone separata** e senza creare una seconda source of truth tecnica.

---

# 0. ISTRUZIONE PRINCIPALE

Non limitarti a proporre una nuova struttura.

Devi:

1. aggiornare il checkout principale;
2. verificare HEAD, Decision Log, ADR, owner spec e issue correnti;
3. ispezionare il clone Wiki reale;
4. fare audit pagina-per-pagina;
5. produrre una matrice di migrazione;
6. aggiornare/supersedere le issue Wiki esistenti se obsolete;
7. creare solo le issue mancanti;
8. implementare la ristrutturazione della Wiki a ondate;
9. aggiornare sidebar, pagine, link, visual e riferimenti;
10. verificare la Wiki pubblicata a schermo;
11. lasciare repo, Wiki, roadmap e tracking coerenti.

Non inventare regole di gameplay per riempire pagine mancanti.

Se una regola non è supportata da owner spec / decisione / catalogo / stato corrente del gioco:
- segnala il gap;
- marca il contenuto come `Futuro`, `Sperimentale` o `Da definire`;
- oppure non pubblicarlo ancora.

---

# 1. STATO ATTUALE DA VERIFICARE PRIMA DI MODIFICARE

Queste sono **assunzioni di handoff**, non sostituiscono la verifica su `main`.

## 1.1 Source of truth della Wiki

Decisione corrente attesa: **D-076**.

La Wiki pubblicata vive nel repository/clone:

`refactor-tactics-main.wiki`

Il clone Wiki è la **fonte unica delle pagine pubblicate**.

`docs/wiki/` nel repository principale non deve tornare a contenere una copia delle pagine. Oggi serve per asset/manifest storici e materiale non duplicato.

### Regola
NON ricreare una pipeline con due copie della stessa prosa.

---

## 1.2 Nomenclatura roster

Decisione corrente attesa: **D-130**.

Nomi player-facing correnti attesi:

- Gadget
- Phase
- Riktor
- Wraith

I nomi legacy:

- Gadget
- Phase
- Riktor
- Wraith

sono da considerare legacy salvo redirect/storia esplicita.

Prima di toccare le pagine personaggio:
- verifica D-130;
- verifica `docs/technical/piano-migrazione-roster.md`;
- verifica lo stato dell’issue **#757**;
- verifica che le dipendenze precedenti siano realmente mergiate.

NON fare una seconda migrazione concorrente.

---

## 1.3 Issue Wiki legacy da riconciliare

Verificare almeno:

- **#422** — vecchio consolidamento Wiki;
- **#757** — rename schede Wiki del roster.

#422 descrive un modello precedente al passaggio a fonte unica. Non implementarla alla lettera se confligge con D-076.

Azione richiesta:
- leggere issue + decisioni successive;
- decidere se chiudere, riscrivere, supersedere o spezzare l’issue;
- non creare duplicati.

---

# 2. VISIONE EDITORIALE

La Wiki deve diventare:

> **un manuale del gioco con una Developer Zone**

e non:

> documentazione di sviluppo con qualche pagina leggibile dai giocatori.

Priorità editoriale:

```text
PLAYER
  ↓
ADVANCED PLAYER
  ↓
DEVELOPER
```

Regola mentale indicativa:

```text
Player      ~70%
Advanced    ~20%
Developer   ~10%
```

Non misurare letteralmente le percentuali. Servono a impedire che una pagina Player venga dominata dall’implementazione.

---

# 3. PRINCIPIO DI CANONICITÀ

Gerarchia:

```text
Decision Log / ADR / Owner Spec / Cataloghi / Codice
                    ↓
             Player Interpretation
                    ↓
                    Wiki
                    ↓
        Visual / Diagram / Esempio
```

Se Wiki e owner divergono:
- correggere la Wiki.

Se il canone non definisce qualcosa:
- la Wiki non lo inventa.

Se una nuova regola player-facing viene proposta:
- prima formalizzarla nel suo owner;
- poi descriverla in Wiki.

La Developer Zone della Wiki è un **explainer tecnico**, non un nuovo owner normativo.

---

# 4. INFORMATION ARCHITECTURE TARGET

Usare questa struttura come target iniziale, adattandola solo se l’audit della Wiki reale mostra motivi concreti.

```text
🏠 HOME

🎮 INIZIA A GIOCARE
 ├─ Cos'è RefactorTactics
 ├─ Come si gioca
 ├─ Il turno simultaneo
 ├─ Planning
 ├─ Resolution
 ├─ Le azioni fondamentali
 └─ La tua prima partita

⚔️ COMBATTIMENTO
 ├─ Movimento
 ├─ Attacco e targeting
 ├─ Dash e movimenti speciali
 ├─ Guard
 ├─ Brace
 ├─ Overwatch
 ├─ Reazioni
 ├─ Facing e direzioni
 ├─ Spinte e spostamenti forzati
 ├─ Collisioni
 └─ Azioni predittive

🗺️ CAMPO DI BATTAGLIA
 ├─ Griglia esagonale
 ├─ Coperture
 ├─ Linea di vista
 ├─ Altezza e livelli
 ├─ Porte e passaggi
 ├─ Ponti
 ├─ Tunnel
 ├─ Ostacoli e geometria
 └─ Elementi interattivi

🌊 AMBIENTE
 ├─ Come funziona l'ambiente
 ├─ Acqua
 ├─ Elettricità
 ├─ Fuoco
 ├─ Ghiaccio
 ├─ Vapore e fumo
 ├─ Terreni difficili
 ├─ Cadute e vuoto
 ├─ Interazioni ambientali
 └─ Combo ambientali

👁️ INFORMAZIONE E PERCEZIONE
 ├─ Visione
 ├─ Fog of War
 ├─ Rumore
 ├─ Ultima posizione conosciuta
 ├─ Informazione di squadra
 ├─ Confermato / Previsto / Incerto
 ├─ Stealth
 └─ Depistaggio

🧑‍🚀 PERSONAGGI
 ├─ Scegliere un personaggio
 ├─ Ruoli e stili di gioco
 ├─ Gadget
 ├─ Phase
 ├─ Riktor
 ├─ Wraith
 ├─ Sinergie
 ├─ Matchup
 └─ Personaggi futuri

🧠 STRATEGIA
 ├─ Prevedere l'avversario
 ├─ Controllare lo spazio
 ├─ Setup e payoff
 ├─ Bait e bluff
 ├─ Coordinazione di squadra
 ├─ Focus fire
 ├─ Creare percorsi obbligati
 ├─ Giocare con informazione incompleta
 └─ Errori comuni

🎯 PARTITE E OBIETTIVI
 ├─ Come si vince
 ├─ Obiettivi
 ├─ Round e durata
 ├─ Formati di partita
 ├─ Bot e allenamento
 └─ Scenari dimostrativi

🌍 MONDO
 ├─ Ambientazione
 ├─ Fazioni
 ├─ Luoghi
 └─ Glossario

────────────────────

🛠️ DEVELOPER ZONE
 ├─ Developer Home
 ├─ Architettura
 ├─ Simulazione deterministica
 ├─ TurnLog e replay
 ├─ Networking e privacy
 ├─ Mappa e pathfinding
 ├─ Gameplay systems
 ├─ Data e cataloghi
 ├─ UI architecture
 ├─ Test e QA
 ├─ Development Status
 ├─ Roadmap
 └─ Decisioni tecniche
```

## Sidebar
- profondità normale massima: 2 livelli;
- 3 livelli solo nei grandi hub;
- evitare alberi profondi stile filesystem tecnico;
- Developer Zone visivamente separata;
- `Development Status` non nel percorso Player.

---

# 5. HOME TARGET

La Home deve parlare prima al giocatore.

Hero concettuale:

```text
REFACTORTACTICS

Pianifica.
Prevedi.
Guarda i piani collidere.

Un gioco tattico a turni simultanei
in cui entrambe le squadre prendono le proprie decisioni
prima che il campo di battaglia le risolva.
```

Tre pilastri:

## PIANIFICA
Decidi movimento, azione, target, facing e coordinazione.

## PREVEDI
L’avversario pianifica contemporaneamente. L’informazione è incompleta.

## RISOLVI
Movimenti, attacchi, reazioni e ambiente si incontrano nella Resolution.

Start cards:

- Come si gioca
- Il turno simultaneo
- Scegli un personaggio
- Capisci il campo di battaglia

Non mettere nella parte alta:
- Unreal Engine;
- roadmap;
- milestone;
- Stable ID;
- RulesVersion;
- Feature ID;
- issue GitHub.

---

# 6. PAGINA “LA TUA PRIMA PARTITA”

Creare se manca.

Struttura:

```text
1. Scegli un personaggio
2. Seleziona una unità
3. Guarda dove può muoversi
4. Pianifica un'azione
5. Controlla il piano degli alleati
6. Conferma / Ready
7. Guarda la Resolution
8. Leggi perché qualcosa ha funzionato o fallito
```

Deve essere sufficiente per iniziare senza leggere l’intera Wiki.

---

# 7. TEMPLATE A — MECCANICA

Applicare a:
- Movimento
- Guard
- Brace
- Overwatch
- Facing
- Collisioni
- Spinte
- Reazioni
- Targeting
- ecc.

```markdown
# <Nome meccanica>

> <funzione tattica in una frase>

## In breve
<2–4 frasi>

## Perché conta
<problema tattico>

## Come funziona
1. ...
2. ...
3. ...

## Esempio di turno

### Prima
...

### Piano
...

### Resolution
...

### Risultato
...

## La decisione tattica
> <domanda reale del giocatore>

## Come usarla bene
...

## Come contrastarla
...

## Interazioni importanti
...

## Errori comuni
...

## Advanced
...

## Stato nel gioco
Giocabile / In sviluppo / Sperimentale / Futuro

## Vedi anche
- ...
- ...
- ...

---

## Per sviluppatori
<solo riferimenti minimali>
```

Regola:
**regola → esempio → scelta → counterplay**

---

# 8. TEMPLATE B — PERSONAGGIO

Applicare ai personaggi correnti.

```markdown
# <Nome>

## <Ruolo / archetipo>

> "<fantasia tattica in una frase>"

[Hero image]
[Radar opzionale]

## Identità
...

## Ti piacerà se...
- ...
- ...

## Potrebbe non piacerti se...
- ...
- ...

## Profilo tattico
| Aspetto | Valutazione |
|---|---|
| Offesa | ... |
| Difesa | ... |
| Mobilità | ... |
| Controllo | ... |
| Supporto | ... |
| Complessità | ... |

## Come si gioca
...

## Kit

### <Abilità>
**Tipo:** ...
**Cosa fa**
...
**Quando usarla**
...
**Attenzione**
...

## La combo fondamentale
...

## Sinergie
...

## Contro chi / cosa soffre
...

## Come affrontarlo
...

## Ambiente preferito
...

## Consigli per iniziare
...

## Advanced
...

## Varianti e personalizzazione
...

## Stato nel gioco
...

## Vedi anche
...

---

## Per sviluppatori
...
```

Ogni scheda deve poter essere riassunta con:

> “Questo personaggio vuole ______ e teme ______.”

I numeri devono provenire dalla fonte canonica corrente.

---

# 9. TEMPLATE C — AMBIENTE

Applicare a:
- Acqua
- Fuoco
- Elettricità
- Ghiaccio
- Vapore
- Fumo
- Terreni
- ecc.

```markdown
# <Elemento>

> <ruolo tattico in una frase>

## In breve
...

## Cosa cambia

### Movimento
...

### Combattimento
...

### Visione / informazione
...

### Rumore
...

## Come appare in partita
...

## Come viene creato
...

## Come scompare o cambia
...

## Interazioni

### <A> + <B>
**Risultato:** ...
**Perché conta:** ...
**Counter:** ...

## Esempio di turno
...

## Come sfruttarlo
...

## Come evitarlo o contrastarlo
...

## Chi lo usa particolarmente bene
...

## Chi deve fare attenzione
...

## Informazione e incertezza
...

## Advanced
...

## Stato nel gioco
...

## Vedi anche
...

---

## Per sviluppatori
...
```

Non duplicare la stessa interazione in 4 pagine con testi diversi.

Creare/aggiornare una pagina centrale:
`Interazioni ambientali`

con matrice canonica.

---

# 10. TEMPLATE D — STRATEGY GUIDE

Applicare a:
- Prevedere l’avversario
- Controllare lo spazio
- Setup e payoff
- Bait e bluff
- Coordinazione
- Informazione incompleta
- ecc.

```markdown
# <Concetto strategico>

> "<principio memorabile>"

## L'idea
...

## Perché funziona in RefactorTactics
...

## Il problema
...

## Esempio

### Quello che sai
...

### Quello che prevedi
...

### Quello che non sai
...

### Le tue opzioni

**A — ...**
vantaggio:
...
rischio:
...

**B — ...**
vantaggio:
...
rischio:
...

### Cosa scegli?
...

## Pattern utile
> "<massima tattica>"

## Quando NON funziona
...

## Livello successivo
...

## Esempi con i personaggi
...

## Esempi con la mappa
...

## Errori comuni
...

## Esercizio
...

## Vedi anche
...
```

Le Strategy Guide non richiedono sempre una sezione Developer.

---

# 11. VISUAL GRAMMAR

La Wiki deve riusare il DNA visuale dell’HUD, ma con densità più bassa.

Concetto:

```text
HUD = operational interface
Wiki = tactical field manual
```

## 11.1 Callout consentiti

Limitare a:

- `In breve`
- `Consiglio`
- `Attenzione`
- `Regola importante`
- `Advanced`
- `Futuro`

Non creare decine di varianti.

---

## 11.2 Certainty Grammar

Riutilizzare la grammatica del gioco:

### Confermato
- linea piena;
- marker solido;
- alta opacità.

### Previsto
- linea tratteggiata;
- marker hollow;
- ghost translucido.

### Incerto
- linea puntinata/fading;
- `?`;
- area di incertezza.

La Wiki deve insegnare a leggere l’HUD.

---

## 11.3 Palette

Riutilizzare i token visuali correnti dove applicabili:

- `RT_UI_BG_Deep`
- `RT_UI_BG_Panel`
- `RT_UI_BG_Raised`
- `RT_UI_Cyan`
- `RT_UI_Violet`
- `RT_UI_Amber`
- `RT_UI_Red`
- `RT_UI_White`

Non affidare il significato al colore.

---

## 11.4 Diagrammi canonici

Supportare soprattutto 4 famiglie:

1. Stato singolo
2. Before / After
3. Planning / Resolution
4. Decision tree

Ogni diagramma deve avere caption.

---

## 11.5 Screenshot vs diagramma

Screenshot:
- “come appare”

Diagramma:
- “perché succede”

---

# 12. ICON GRAMMAR

## 12.1 NON creare un secondo sistema di gameplay icon

La Wiki deve riusare le icone del gioco per:

- Phase
- Action
- Status
- Environment
- Identity
- Certainty
- Tactical
- Warning
- Reaction

Le gameplay icon devono mantenere le chiavi/categorie correnti.

Verificare il catalogo attuale:
`URTIconLibrary::RequiredIconIds()`

Non fidarsi di liste scritte a mano.

---

## 12.2 Gameplay Icons vs Editorial Icons

### Gameplay Icons
Riutilizzate da HUD/game.

### Editorial Icons
Esistono solo per la documentazione e NON entrano nel catalogo runtime.

Set editoriale massimo consigliato:

- Summary
- Rule
- Example
- Decision
- Tip
- Counter
- Advanced
- Future
- Developer
- Related

---

## 12.3 Regole accessibilità icone

Ogni nuova icona:

- leggibile a 48 px;
- leggibile a 32 px;
- test a 24 px;
- test grayscale;
- test protanopia;
- test deuteranopia;
- test tritanopia;
- distinguibile su fondo chiaro;
- distinguibile su fondo scuro;
- non dipende dal solo colore;
- confrontata con le silhouette più simili.

La silhouette è il primo canale.
Il colore è il secondo.

---

# 13. AUDIT DELLA WIKI REALE

Prima di riscrivere la sidebar, inventariare il clone reale.

Per ogni pagina rilevare:

- path/nome;
- titolo;
- categoria attuale;
- link in ingresso;
- link in uscita;
- immagini;
- eventuali nomi legacy;
- riferimenti tecnici;
- stato della feature;
- contenuto duplicato;
- eventuale owner normativo;
- qualità Player;
- qualità Advanced;
- quantità di implementation noise.

Assegnare una classificazione:

- `KEEP`
- `PLAYER-REWRITE`
- `SPLIT`
- `MERGE`
- `MOVE-DEV`
- `STALE-AUDIT`
- `HIDE`
- `REMOVE/ARCHIVE`
- `NEW`

Produrre:

`wiki-audit-player-first-2026-08-13.md`

nel posto appropriato di tracking/documentazione del repository principale.

---

# 14. BASELINE DI MIGRAZIONE DA VERIFICARE

Questa è una guida iniziale. Verificare contro il clone reale prima di applicare.

## Home / onboarding

- `Home.md` → PLAYER-REWRITE
- `che-cose-refactortactics` → PLAYER-REWRITE
- `struttura-del-round` → RENAME/REWRITE → `Il turno simultaneo`
- `Wiki-Guide` → MERGE/REMOVE
- `La tua prima partita` → NEW

## Planning / combat

- `azioni-e-movimento` → SPLIT
- `planning-e-coordinazione` → SPLIT
- `reazioni-overwatch-e-previsioni` → SPLIT
- `facing-e-direzionalita` → PLAYER-REWRITE
- Guard → NEW/EXTRACT
- Brace → NEW/EXTRACT
- Collisioni → NEW
- Spinte → NEW
- Targeting → NEW
- Dash → NEW
- Azioni predittive → NEW

## Battlefield

- griglia/geometria → PLAYER-REWRITE
- mappa/terreni/ambiente → SPLIT
- topologia dinamica → PLAYER-REWRITE/RENAME
- Coperture → NEW/EXTRACT
- LOS → NEW/EXTRACT
- Altezza → NEW/EXTRACT
- Porte/passaggi → NEW/EXTRACT
- Tunnel → NEW

## Environment

- acqua-e-elettricita → SPLIT
- Acqua → NEW
- Elettricità → NEW
- Fuoco → NEW
- Ghiaccio → NEW
- Vapore/fumo → NEW
- Terreni difficili → NEW
- Cadute e vuoto → NEW
- Interazioni ambientali → NEW

## Information

- visibilita-rumore-e-informazione → SPLIT
- Visione
- Fog of War
- Rumore
- Last Contact
- Confermato/Previsto/Incerto
- Stealth
- Depistaggio

## Characters

- `Personaggi.md` → PLAYER-REWRITE
- legacy roster pages → migrazione verso Gadget/Phase/Riktor/Wraith solo dopo verifica D-130/#757
- Paragon pages → HIDE/MOVE-DEV salvo nuova decisione esplicita
- radar → REUSE solo da dati correnti

## Factions

- tutte → STALE-AUDIT prima della riscrittura

## Strategy

Creare almeno:

P0:
- Prevedere l’avversario
- Controllare lo spazio
- Setup e payoff
- Coordinazione di squadra
- Giocare con informazione incompleta

P1:
- Bait e bluff
- Creare percorsi obbligati
- Focus fire
- Errori comuni

## TurnLog/determinism

- parte Player → “Perché è successo?” / Combat Log / Resolution
- parte tecnica → Developer Zone

## Feature status

- `Stato-delle-feature` → MOVE-DEV

---

# 15. WAVES DI IMPLEMENTAZIONE

Non fare big bang.

## Wave 0 — Audit
Solo inventario, conflitti, dipendenze, issue.

## Wave 1 — Fondamenta Player
- Home
- Cos’è RefactorTactics
- Come si gioca
- Il turno simultaneo
- La tua prima partita
- Sidebar

## Wave 2 — Core Gameplay
- Planning
- Movimento
- Attacco
- Guard
- Brace
- Overwatch
- Reazioni
- Facing

## Wave 3 — Battlefield
- Griglia
- Cover
- LOS
- Altezza
- Porte/passaggi
- Ambiente hub

## Wave 4 — Environment + Information
- Acqua
- Elettricità
- Fuoco
- Ghiaccio
- Interazioni
- Visione
- Rumore
- Fog of War
- Last Contact

## Wave 5 — Characters
Solo dopo compatibilità con D-130/#757.

## Wave 6 — Strategy
P0 prima, P1 dopo.

## Wave 7 — Developer Zone
Spostare/riscrivere contenuto tecnico.

## Wave 8 — Cleanup
- link;
- redirect;
- orfani;
- sidebar finale;
- visual;
- accessibility;
- broken links;
- mobile check.

---

# 16. ISSUE / EPIC / ROADMAP

Prima di creare issue nuove:

1. cerca issue Wiki/documentation aperte;
2. controlla #422 e #757;
3. controlla roadmap e piani correnti;
4. controlla lavori paralleli;
5. controlla branch/PR attivi se rilevanti.

Poi:

- aggiorna issue obsolete;
- chiudi/supersedi quelle non più valide;
- crea solo gap reali;
- collega ogni issue alla wave corretta;
- evita una mega-issue ingestibile se esistono slice indipendenti.

Possibile epic:
`Wiki Player-First Restructure`

ma creala solo se non esiste già un owner equivalente.

Tracking da allineare, se realmente usati dal progetto:
- roadmap;
- feature map;
- scenario map;
- eventuale product map;
- issue/epic;
- documentazione owner del processo Wiki.

Non aggiornare tracking morto/archiviato solo per “completezza”.

---

# 17. REGOLE DI SCRITTURA PLAYER

Ogni paragrafo Player deve rispondere ad almeno una domanda:

- cosa posso fare?
- cosa può succedere?
- cosa devo osservare?
- quale decisione devo prendere?
- come lo contrasto?
- perché questa cosa è importante?

Togliere dal corpo Player:

- `USTRUCT`;
- `UObject`;
- RPC;
- classi C++;
- `.h/.cpp`;
- Feature ID;
- issue GitHub;
- hash;
- Automation Test;
- formati serializzati;
- dependency graph software;
- API Unreal.

Eccezione:
sezione `Per sviluppatori` o Developer Zone.

---

# 18. REGOLE SUI NUMERI

Danno, range, cooldown, movement, durata, AoE ecc.:

- mostrare se utili al giocatore;
- leggere dalla fonte corrente;
- non copiare vecchi PDR/hand-off senza verifica;
- non promuovere valori `PROPOSED FOR PLAYTEST` a regole definitive.

---

# 19. STATO PLAYER

Nelle pagine Player usare solo:

- `Giocabile`
- `In sviluppo`
- `Sperimentale`
- `Futuro`

Niente gate tecnici nel corpo Player.

La certainty tattica è un altro asse:

- Confermato
- Previsto
- Incerto

NON confondere i due sistemi.

---

# 20. VISUAL REQUIREMENTS PER PAGINA

Per ogni pagina dell’audit aggiungere:

```text
GAMEPLAY ICONS REUSED:
- ...

EDITORIAL ICONS:
- ...

NEW ICON REQUIRED:
YES / NO

VISUAL:
- hero?
- tactical diagram?
- before/after?
- screenshot?
- radar?
- matrix?
```

Non creare immagini decorative senza funzione.

---

# 21. GIT SAFETY

Repository principale e Wiki sono separati.

## Main repo
- aggiornare `main`;
- branch dedicato;
- commit focalizzati;
- non includere file non correlati.

## Wiki clone
- fetch/pull prima di lavorare;
- branch dedicato se supportato dal workflow corrente;
- NON `git add -A`;
- stage per percorso;
- evitare di raccogliere lavoro di altre sessioni;
- verificare diff prima del commit.

Dopo pubblicazione:
- controllare la Wiki vera;
- non assumere che “commit riuscito” significhi “pagina corretta”.

---

# 22. VERIFICA VISIVA OBBLIGATORIA

Per le pagine modificate:

- desktop;
- mobile/narrow layout;
- immagini realmente caricate;
- caption corrette;
- icone leggibili;
- sidebar navigabile;
- heading coerenti;
- nessun link rotto;
- nessuna immagine con testo legacy incorporato;
- nessuna pagina con markup rotto.

Per le schede personaggio:
la verifica deve includere le immagini, non solo `grep`.

---

# 23. DEFINITION OF DONE

La ristrutturazione non è Done finché:

- [ ] clone Wiki auditato;
- [ ] matrice completa KEEP/REWRITE/SPLIT/MERGE/MOVE-DEV/STALE/HIDE/REMOVE/NEW;
- [ ] sidebar Player-first;
- [ ] Developer Zone separata;
- [ ] Home riscritta;
- [ ] percorso Start Here completo;
- [ ] quattro template applicati;
- [ ] visual grammar applicata;
- [ ] gameplay icon riusate invece di duplicate;
- [ ] editorial icon separate dal runtime;
- [ ] nessuna regola inventata;
- [ ] numeri verificati contro owner corrente;
- [ ] nessun nome legacy player-facing salvo sezione storica esplicita;
- [ ] #422 riconciliata;
- [ ] #757 rispettata/non conflittuale;
- [ ] nessuna feature futura presentata come giocabile;
- [ ] max 3 link “Vedi anche” per pagina;
- [ ] pagina Player non dominata da implementation details;
- [ ] broken link check eseguito;
- [ ] visual check desktop/mobile;
- [ ] Wiki pubblicata verificata a schermo;
- [ ] tracking/roadmap aggiornati solo dove necessario;
- [ ] commit separati e leggibili.

---

# 24. OUTPUT RICHIESTI A CLAUDE

Alla fine produrre un report con:

## A. Audit
- numero pagine totali;
- classificazione per tipo;
- legacy trovati;
- duplicate;
- pagine orfane;
- pagine tecniche nel percorso Player;
- visual mancanti.

## B. Decisioni prese
- issue aggiornate/chiuse/supersedute;
- nuove issue create;
- eventuali gap di canone.

## C. Implementazione
- pagine create;
- pagine riscritte;
- pagine splittate;
- pagine spostate Developer;
- sidebar modificata;
- asset visuali modificati.

## D. Verifica
- link check;
- visual check;
- grep legacy;
- stato issue;
- stato Wiki pubblicata.

## E. Commit
Elenco commit main repo + wiki clone.

## F. Follow-up
Solo ciò che resta realmente aperto.

---

# 25. COMMIT STRATEGY SUGGERITA

Esempio:

```text
docs(wiki): audit current player-facing information architecture
docs(wiki): rebuild player-first home and navigation
docs(wiki): split core combat mechanics into player guides
docs(wiki): restructure battlefield and environment guides
docs(wiki): add perception and incomplete-information guides
docs(wiki): migrate character pages to player template
docs(wiki): add core strategy guides
docs(wiki): isolate developer documentation section
docs(wiki): normalize icons visuals and cross-links
docs(wiki): finalize player-first migration checks
```

Adattare ai branch e alle issue reali.

---

# 26. NON FARE

- non ricreare pagine in `docs/wiki/` come copia della Wiki;
- non fare search/replace globale del roster senza verificare D-130;
- non implementare #422 alla lettera se superata;
- non usare vecchi PDR come fonte definitiva per numeri/nomenclatura;
- non creare un secondo icon system gameplay per la Wiki;
- non nascondere contenuto tecnico sbagliato solo cambiando sidebar;
- non cancellare asset storici non duplicati senza hash/verifica;
- non pubblicare 30 pagine in una sola commit senza review;
- non usare `git add -A` nel clone Wiki;
- non dichiarare “Done” senza verifica a schermo.

---

# 27. CRITERIO FINALE

Quando hai finito, un nuovo giocatore deve poter aprire la Wiki e capire:

1. che cos’è RefactorTactics;
2. come funziona un turno;
3. cosa può fare durante il Planning;
4. cosa succede durante la Resolution;
5. come leggere mappa, ambiente e informazione incompleta;
6. quale personaggio potrebbe piacergli;
7. quali decisioni tattiche rendono il gioco interessante;

senza dover leggere:
- classi C++;
- resolver internals;
- networking;
- hash;
- issue;
- roadmap.

Lo sviluppatore deve comunque poter trovare tutto questo nella **Developer Zone**, collegato alle fonti normative corrette.

**Questo è il risultato da ottenere.**
