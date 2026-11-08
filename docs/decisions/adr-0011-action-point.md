# ADR-0011 — Il turno passa agli Action Point: un pool per unità, un costo per azione, più azioni nella stessa fase

> `CANONICAL` · **Stato**: Accettato per decisione d'autore ([D-584](RT_PDR_00_Decision_Log.md), 2026-11-08), **non implementato**.
> Il codice, il catalogo e la spec owner descrivono ancora il modello a slot di [D-028](RT_PDR_00_Decision_Log.md) e
> [D-114](RT_PDR_00_Decision_Log.md): finché E38 non consegna, quello è il comportamento in vigore e nessun
> `IMPLEMENTATION DRIFT` va dichiarato contro di esso. **I numeri non esistono**: budget e costi li propone E38 su
> una simulazione del catalogo, e l'autore li accetta in una voce successiva.
> **Data**: 2026-11-08 · **Decisore**: utente (dev singolo), con domande a scelta; il testo delle opzioni è riportato
> alla lettera in D-584 · **Nasce da**: [D-583](RT_PDR_00_Decision_Log.md) (2), la riapertura di D-114 chiesta dalla
> «Master Technical Specification» C# del 2026-11-07 ([referto](../roadmap/plans/master-spec-csharp-spec-panel-2026-11-07.md) §16)
> **Supera**: D-028 e D-114 nel modello di legalità; [D-190](RT_PDR_00_Decision_Log.md) nella sola frase «la legalità
> del piano resta strutturale» (la parte sui Movement Point resta intera) · **Non tocca**: le cinque macro-fasi
> (ADR-0003), le finestre di reazione (ADR-0004), il pivot (ADR-0008), [D-265](RT_PDR_00_Decision_Log.md) (nessuna
> risorsa universale: l'Overclock resta fuori), [D-117](RT_PDR_00_Decision_Log.md) (costo per cella)
> **Issue**: [#604](https://github.com/DegrassiAaron/refactor-tactics-main/issues/604) riaperta · epic **E38**
> ➕ **Emendamento 2026-11-08 — [D-592](RT_PDR_00_Decision_Log.md)**: la conseguenza 4 è decisa, le reazioni sono una lista (vedi sotto).

## Contesto

Dal 2026-08-12 il turno è *1 movimento · 1 principale · 1 reazione* ([D-028](RT_PDR_00_Decision_Log.md)), e la
legalità di un piano è **strutturale**: `Guard` e `BasicAttack` non si sommano perché occupano lo stesso slot, non
perché un totale sfori ([D-114](RT_PDR_00_Decision_Log.md), che respinse per nome la «capacità numerica» proposta
dal kit del 2026-08-12). Il peso di un'azione si paga in **drawback** — cooldown, status auto-inflitto, rinuncia alla
reazione — e [D-190](RT_PDR_00_Decision_Log.md) ha tolto da `ValidatePlan` l'ultimo residuo di sottrazione, il ramo
dei Movement Point.

La specifica del 2026-11-07 propone 6 Action Point per turno con costi di fase (Prep 1 · Dash 2 · Blast 2, Ultimate 3 ·
Reaction 2) su quattro fasi. Il referto l'ha misurata contro il contratto: il conflitto è `CONTRACT CONFLICT` con
D-114, e l'aritmetica ha un buco (`1 + 2 + 3 = 6` non lascia la reazione). L'autore ha riaperto D-114 il 2026-11-08
([D-583](RT_PDR_00_Decision_Log.md) (2)) e nello stesso giorno ha deciso il modello con quattro domande.

Il panel ha chiesto un ADR e non una sola riga di registro perché la scelta cambia il **tipo** della legalità, da
strutturale a quantitativa (Fowler, referto §16.1): tocca il validatore, il resolver, il TurnLog, l'HUD e il bot
insieme, e ognuna di queste sedi leggerà di qui che cosa deve diventare.

## Decisione

Le quattro risposte dell'autore, nell'ordine in cui sono state date. Il testo fra virgolette è quello dell'opzione
scelta, non una parafrasi.

### 1. La legalità è un pool di AP, senza vincolo di fase

> «Solo il budget conta: due attacchi nel Blast o due azioni di Prep sono legali se gli AP bastano, ordinati per
> priorità dentro la fase. È il modello più libero e il più costoso: il resolver simultaneo, il TurnLog, il replay e
> l'HUD oggi assumono una azione per unità per fase, e ogni fase dovrebbe accettare una lista.»

Scartate: «Pool AP, al più una azione per fase» (la raccomandazione dell'assistente) e «AP sopra gli slot di oggi».
Ne segue che un piano non dichiara più *un* percorso, *una* principale e *una* reazione (spec owner §2), ma una
**lista di azioni** ciascuna con la propria fase e il proprio costo, la cui somma non supera il budget dell'unità.

### 2. Il camminare costa AP per profilo; i Movement Point restano la distanza

> «Ogni profilo di movimento normale ha un costo in AP (es. Move n, Sprint n+1, da catalogo). I Movement Point
> restano l'asse della distanza dentro il profilo, col costo per cella del terreno (D-117). Due assi: gli AP dicono SE
> cammini e con che profilo, i MP fin dove. D-190 resta vera per i MP.»

Scartate: «No, camminare resta gratuito e strutturale» e «Sì, e gli AP comprano le celle: i Movement Point
spariscono».

### 3. Il modello ora, i numeri dopo, misurati

> «La D-nnn di MSP-1 fissa SOLO il modello (le risposte 1, 2 e 4). Budget e costi li propone E38 su una simulazione del
> catalogo attuale (quali combinazioni di oggi restano legali, quali nuove entrano), e li accetti in una sessione
> successiva. È la separazione «modello dai numeri» che la DoD di #604 chiede, e il primo consegnabile è l'assert, non
> il valore.»

Scartate: «6 AP e i costi della specifica, come punto di partenza» e «Budget che replica gli slot di oggi, poi si
differenzia». ⛔ **Nessun numero esiste prima della voce che lo accetta**: né 6, né i costi di fase della specifica.
Chi li scrive in un catalogo prima di allora scrive un difetto.

### 4. Cooldown e drawback restano: il costo AP è un asse in più

> «Cooldown, status auto-inflitti e rinunce restano contenuto del catalogo; il costo AP si aggiunge. «Due leggere non
> valgono una pesante» cade come REGOLA (gli AP la contraddicono per definizione), ma un'azione pesante può ancora
> pagare anche in cooldown o in status. Nessun dato del catalogo si rimuove oggi.»

Scartate: «Il costo AP sostituisce i drawback, i cooldown restano» e «Il costo AP sostituisce drawback E cooldown».

## Conseguenze — letture dell'assistente, ribaltabili

1. **Ordine dentro la fase.** Con più azioni della stessa unità nella stessa fase, il resolver ha bisogno di un ordine
   totale e deterministico che oggi non esiste: priorità del catalogo, poi un criterio stabile (ordine di dichiarazione
   nel piano, o Stable ID dell'azione). Va scritto prima del primo test, perché TurnLog, replay e hash lo leggono.
2. **Il piano cambia forma.** `FRTUnitPlan` (o l'equivalente che `ValidatePlan` legge) smette di avere una casella
   per slot e porta una lista; `ERTActionSlot` smette di decidere la legalità e sopravvive, se sopravvive, come
   **fase di risoluzione** dell'azione. Snapshot, serializzazione e versione dello schema seguono (CLAUDE.md §7).
3. **I reason code.** `InsufficientActionCapacity`, che D-114 vietò perché «nomina uno stato che non esiste», torna
   nominabile: lo stato esiste. Il nome lo decide chi implementa, dentro le famiglie di `spec-economia-del-turno.md`
   §5.
4. **La reazione.** Oggi è «indipendente da movimento e principale» e non si perde agendo. Con un costo in AP si può
   perdere per esaurimento del budget: è la conseguenza aritmetica che il referto §9 aveva segnalato, e l'autore l'ha
   accettata scegliendo il modello; il valore che la rende rara o frequente è dei numeri di E38.
   ➕ **2026-11-08, [D-592](RT_PDR_00_Decision_Log.md)**: le reazioni sono una **lista**, limite solo dal budget: un'unità ne arma quante gli AP
   consentono, ciascuna con trigger e condizione dichiarata propri; `PlannedReactionAbility` diventa una lista e l'ordine fra reazioni
   armate sullo stesso trigger segue la conseguenza 1.
5. **Il bot e l'HUD** leggono la stessa legalità del validatore (CLAUDE.md §11): il planning UX deve mostrare un
   budget e una somma, e il bot deve esplorare combinazioni, non più permutazioni di tre caselle.
6. **Il primo consegnabile è l'asserto.** Prima di ogni numero: un test che fallisce se due azioni della stessa fase
   vengono risolte in ordine non deterministico, e uno che fallisce se il validatore accetta un piano la cui somma
   supera il budget. La simulazione del catalogo di cui parla la risposta 3 è lo strumento con cui E38 propone i valori.
7. **Che cosa non cambia.** Le cinque macro-fasi e il Move dopo il Blast; le finestre di reazione; il costo per cella
   e i profili di movimento; i cooldown e gli status del catalogo; D-265 (nessuna risorsa universale: il settimo AP da
   Overclock della specifica non entra). ⚠️ Il modello della specifica sulle **quattro** fasi non è adottato: le
   azioni si distribuiscono su Prep, Dash, Blast, Move e reazione.

## Alternative considerate

| Alternativa | Esito |
|---|---|
| Pool AP, al più una azione per fase (raccomandata dall'assistente) | Scartata dall'autore: il vincolo di fase restava strutturale |
| AP sopra gli slot di oggi (due contabilità) | Scartata: «due leggere non valgono una pesante» cadeva solo a metà |
| Camminare gratuito e strutturale | Scartata: il tempo del turno non sarebbe più un solo budget |
| Gli AP comprano le celle, i MP spariscono | Scartata: è il «pool unico» che la spec owner descrive come collasso |
| 6 AP e i costi della specifica come punto di partenza | Scartata: i numeri si misurano sul catalogo, non si copiano |
| Il costo AP sostituisce drawback o cooldown | Scartata: il catalogo non perde dati oggi |

## Verifica

Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged: `N/A` per questo ADR, che non cambia codice.
Quando E38 implementa: `RefactorTactics.Simulation.GoldenCorpusMatches` cambierà per costruzione (nuovo schema di
piano) e va rigenerato **dopo** che l'asserto d'ordine dentro la fase è verde, non prima.

## Revisione

Si rilegge quando E38 propone i numeri: se la simulazione mostra che nessun budget ragionevole lascia legale la
reazione in un turno pieno, la conseguenza 4 torna all'autore come domanda, non come difetto di implementazione.
