# #2742 — Panel di specifica sull'overlay delle linee di tiro · 2026-09-09

> **Comando**: `/issue-run 2742` → `sc:spec-panel 2742`, modalità **critique**, focus **architecture**.
> **Panel**: Fowler (lead) · Newman · Hohpe · Nygard.
> **Domanda operativa della run**: *quanto #1941 blocca davvero?* — non *se* sia scritto che blocca.
> **Base misurata**: `main = f339f537`.

## §1 — La misura della dipendenza

`#1941` (OVL-01) ha **11 caselle di DoD, zero chiuse**. Ma non tutte sono precondizioni di questa issue. Lette una per una:

| Casella di #1941 | Precondizione di #2742? |
|---|---|
| un **tipo** che porta `Meaning · Source · Certainty · Priority · Layer` | ✅ **sì** — è il modello che l'overlay consumerebbe |
| la **palette in una sede** configurabile | ✅ **sì** — altrimenti il colore della LOS nasce letterale |
| le **tre collisioni** (arancione · ciano · oro) chiuse da un `D-nnn` | ✅ **sì**, e una è **esattamente la LOS**: la spec v0.2 le assegna il ciano, che qui è già la traccia del percorso |
| `Certainty` cambia solo l'opacità · `Source` non cambia il colore | ⚪ no — regole interne al modello |
| gate di distinguibilità 7 tinte × 9 superfici | ⚪ no — lo eredita chi disegna |
| nessun Actor per cella · costo di ridisegno · glifo leggibile · voce PIE · suite | ⚪ no |

∴ **il blocco è reale ma circoscritto: 3 caselle su 11**, e la terza riguarda proprio il colore che questa issue userebbe.

⚠️ **Difetto trovato in #1941, e non è di questa run**: l'ultima casella chiede *«`./scripts/rt-suite.ps1` verde»*. Quello script è **uscito dal repository** il 2026-09-08 con `D-346`/`D-347`. Un criterio che nessuno può eseguire non tiene aperta una issue: va riscritto o tolto. Segnalato lì, non corretto qui.

## §2 — Il rilievo capitale

### 🔴 FOWLER — la issue confonde **il dato** con **la resa**, e solo la seconda dipende da #1941

`#2742` è scritta come un'unica cosa: *«le linee di tiro non si vedono»*. Ma sono due responsabilità, e il repository ha già il pattern che le separa:

```cpp
// UI/RTHUD.h — entrambe STATICHE e PURE, con il disegno altrove
static void ComputePlannedHitMarks(const TArray<ARTUnit*>&, int32 PlayerTeamId, TSet<FRTCellId>&, TSet<FRTCellId>&);
static void ComputeBlockerMarks(const TArray<FRTPlayerEventLineView>&, TSet<FRTCellId>&);
```

Nove riferimenti nei test le esercitano **senza aprire un viewport**. Il commento accanto a `ComputeBlockerMarks` dichiara anche perché stanno lì e non in `DrawHUD`: *«`DrawHUD` non ha copertura headless»*.

∴ una `ComputeLineOfSightViews` — *«date le unità e l'osservatore, quali linee esistono, quali sono interrotte e dove»* — **non tocca la palette, non tocca `DrawPlanningPreview`, e non ha bisogno di #1941**. È il colore che ne ha bisogno, non il dato.

📊 **Impatto**: la parte bloccata passa da 100% a ~la resa. La parte testabile headless si sblocca subito.

### 🔴 NYGARD — ma un produttore senza consumatore è **codice morto che invecchia**

⛔ Obiezione alla conclusione di Fowler, e regge: costruire `ComputeLineOfSightViews` **oggi** e disegnarla quando #1941 chiude significa consegnare un produttore che nessuno chiama. È l'anti-pattern che questo repository conosce già — *«dati senza consumatore»* — e il costo non è teorico: un dato inerte non viene esercitato, quindi le sue assunzioni marciscono in silenzio finché qualcuno non prova a usarlo.

🔑 **La risoluzione non è rinunciare: è trovargli un consumatore vero, adesso.** Ed esiste: **#2741** — il rifiuto di bersaglio — ha bisogno di **una** linea, quella del click, con lo stesso identico contenuto (passa / non passa / dove si interrompe / di che tipo è il blocco).

∴ **lo stesso produttore serve entrambe**, con cardinalità diversa: una linea per #2741, N per #2742.

### 🟡 NEWMAN — il confine della conoscenza, e la famiglia in cui questa query deve stare

**#2596** (*Enemy Tactical Query*) sta costruendo esattamente la forma *«query autorizzata che parte dall'osservatore»*, con il canary di privacy e l'ordinamento `StableLess` già prescritti nella sua DoD.

⚠️ Se #2742 ne apre una seconda, nascono **due contratti di conoscenza** — che è il difetto che `#1936` vieta e che `ComputeBlockerMarks` evita non rifiltrando.

📝 **Raccomandazione**: la query delle linee appartiene a quella famiglia. O la consuma, o ne segue la forma dichiarandolo.

### 🟡 HOHPE — due meccanismi di consegna, e le linee somigliano al secondo

Oggi il dato di presentazione arriva a schermo in due modi:

| | Meccanismo | Autorizzazione |
|---|---|---|
| `DrawPlanningPreview` | legge lo **stato degli Actor** | applicata dentro, per famiglia |
| il feed del giocatore | `Project` → `BuildPlayerEventFeed` | **a monte**, come primo passo |

Le linee di tiro portano informazione sulla **conoscenza** (ciò che vedo ora contro ciò che ricordo — è l'argomento che `RTHexLosConsole.cpp` usa per dire che la LOS *«ha una certezza vera mentre le altre cinque no»*). ∴ appartengono al secondo modello, non al primo.

## §3 — Consenso del panel

1. **Il blocco di #1941 è reale, e circoscritto a tre caselle** — fra cui la collisione del ciano, che è proprio il colore della LOS. Non è procedurale.
2. **Ma blocca la resa, non il dato.** Il repository ha già il pattern che li separa, con nove test che lo esercitano.
3. ⛔ **Il dato non va però costruito da solo**: senza consumatore è inerte, e l'unico consumatore disponibile prima di #1941 è **#2741**.
4. ∴ **#2742 resta `Blocked` per la resa**, e la sua parte utile si realizza **dentro #2741**, che ne diventa il primo chiamante.
5. **La query deve stare nella famiglia di #2596**, non accanto: due contratti di conoscenza sono il difetto, non la ridondanza.

## §4 — Cosa raccomanda il panel

**Non aprire lavoro su #2742 adesso.** La run si chiude in `Blocked`, con la dipendenza **misurata** invece che citata.

Ciò che #2742 avrebbe consegnato di utile e non bloccato — il produttore delle linee autorizzate — va **specificato dentro #2741**, che ha un consumatore reale e nessuna dipendenza. Quando #1941 chiude, #2742 aggiunge la resa a un dato già esercitato dai test e già usato in partita.

⛔ **Cosa il panel continua a sconsigliare**: il sesto `FColor` letterale in `DrawPlanningPreview`. È la scorciatoia che chiuderebbe #2742 in un pomeriggio e allargherebbe il difetto che #1941 esiste per chiudere.
