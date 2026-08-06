# RefactorTactics — Roadmap delle sedute in editor

> **Ultimo aggiornamento**: 2026-08-06
> Terza vista del progetto, accanto a [`roadmap-checkpoint.md`](roadmap-checkpoint.md) (esecuzione) e
> [`roadmap-v0.1.md`](roadmap-v0.1.md) (release): la vista **operativa in editor**. Elenca le sedute che solo
> una persona davanti a Unreal può fare — costruire asset e verificare le implementazioni — nell'ordine in cui
> conviene farle, con la dipendenza dichiarata verso il codice.
>
> Orizzonte: fino alla release **v0.1**. M10 (rete) e M11 (production readiness) restano fuori.

## Prossima seduta

**U1 — Mappa-arena hex**, e nella stessa apertura **U2** e **U3**: al 2026-08-06 i checkpoint 6.1, 6.2 e 6.3
sono tutti 🟡 con il **codice fatto**, in attesa solo della verifica interattiva. Una sessione all'editor porta
tre checkpoint a ✅ e chiude sei voci dell'Editor Mode ferme da M9.

> Un checkpoint 🟡 **sblocca** la seduta: 🟡 significa «codice fatto, manca la verifica», ed è esattamente la
> seduta a fornirla. Non aspettare che diventi ✅ — non può, senza di te.

## Cos'è e cosa non è

**È** la sequenza delle sedute all'editor: cosa produci, cosa serve che esista già nel codice, cosa verifichi,
quando la seduta è finita, cosa sblocca.

**Non è** il registro degli esiti → l'esito atteso e lo stato ✅/⏳ delle voci `PIE-*` vivono in
[`test-manuali-pie.md`](test-manuali-pie.md). Qui si citano gli **ID**, mai il contenuto.

**Non è** un manuale → la procedura passo-passo sta nelle guide esistenti
([`guida-animazioni-paragon.md`](guida-animazioni-paragon.md),
[`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) §5/§6/§11,
[`../guides/debug-vs-unreal.md`](../guides/debug-vs-unreal.md)), e viene linkata. I passi espliciti compaiono
solo dove nessuna guida copre ancora la cosa: oggi **U1** (costruire l'arena) e **U16** (misurare i KPI).

**Non decide lo scope** → milestone e DoD di prodotto restano nelle altre due roadmap. Se una seduta scopre che
un DoD è sbagliato, si cambia lì.

## Anatomia di una seduta

- **Sbloccata da** — i checkpoint di codice che devono avere il **codice fatto** perché la seduta abbia senso.
  Un 🟡 basta e anzi è il caso normale: gli manca proprio la verifica che porti tu.
- **Preparazione condivisa con** — le sedute che usano lo stesso allestimento: falle nella stessa apertura.
- **Produce** — ciò che esce e che qualcun altro aspetta. Due forme legittime: un **asset committato**, oppure
  un **verdetto** che chiude un checkpoint di codice.
- **Percorso critico** — se `no`, la seduta non blocca la v0.1: è lavoro che puoi tirare avanti quando vuoi un
  risultato visibile, non quando la release lo chiede.
- **Verifichi** — gli ID delle voci in `test-manuali-pie.md`.
- **Finita quando** — condizione osservabile, non impressione.
- **Sblocca** — sedute e checkpoint a valle.

Le sedute dei blocchi 5 e 6 dichiarano **meno campi**: il codice sotto non esiste ancora e inventarne il DoD
sarebbe una metrica falsa. Si completano quando la loro epic si apre.

## Stato

Lo stato di una seduta è **derivato**, mai dichiarato a mano: se non si ricava da `test-manuali-pie.md` più
`git`, è sbagliato.

| | Quando |
|---|---|
| ⏳ | prerequisito di codice non pronto, oppure pronta ma non ancora aperta |
| 🟡 | aperta, parte fatta — oppure una verifica è andata storta |
| ✅ | tutte le sue voci `PIE-*` sono ✅ **e** i suoi asset sono committati |

## Le 17 sedute a colpo d'occhio

| | Seduta | Produce | Sbloccata da | Critico | Stato |
|---|---|---|---|---|---|
| **U1** | Mappa-arena hex | `DA_HexMap_Arena` + `L_HexArena` | CP 6.0 ✅ | sì | ⏳ |
| **U2** | Partita hex, primo giro | verdetto su allestimento e movimento | CP 6.1 🟡, CP 6.2 🟡 | sì | ⏳ |
| **U3** | Input e pianificazione | verdetto su selezione, budget, anteprima | CP 6.3 🟡 | sì | ⏳ |
| **U4** | Combat e linea di tiro | verdetto su forme, LOS, knockback | CP 6.4, CP 6.5 | sì | ⏳ |
| **U5** | Bot e HUD | verdetto + pesi utility tarati su hex | CP 6.6, CP 6.7 | sì | ⏳ |
| **U6** | Multilivello e partita completa | **chiude M6 / E2** | CP 6.8 | sì | ⏳ |
| **U7** | Personaggi Paragon | `BP_Unit_Guardian`, `BP_Unit_Ranger` | già in `main` | no | ⏳ |
| **U8** | Animazioni | `ABP_*` + montaggi | U7 | no | ⏳ |
| **U9** | Leggibilità e riferimento visivo | video di riferimento (DoD di M8) | U7, U8 | no | ⏳ |
| **U10** | Data asset delle azioni | catalogo azioni come dati | CP 1.3, CP 1.4 | sì | ⏳ |
| **U11** | I 4 eroi | `DA_Hero_*` + spawn 2v2 | E6 | sì | ⏳ |
| **U12** | Loadout | varianti, gadget, moduli | E7 | no | ⏳ |
| **U13** | Arena v0.1 | arena estesa (Rough, acqua, porta, cover) | E8, E9 | sì | ⏳ |
| **U14** | Ambiente in partita | verdetto sulle regole ambientali | U13 | sì | ⏳ |
| **U15** | HUD, intenti, log, debug | verdetto su leggibilità e osservabilità | E11 | sì | ⏳ |
| **U16** | Misura dei KPI | **numeri reali** nella tabella KPI | U6 | sì | ⏳ |
| **U17** | Release v0.1 | build Development e Shipping giocate | E12 | sì | ⏳ |

---

## Blocco 1 — Il banco di prova

*Eseguibile oggi. Senza l'arena, metà delle verifiche di M6 non hanno un terreno su cui girare.*

### U1 · Mappa-arena hex ⏳

**Sbloccata da**: CP 6.0 ✅ · **Preparazione condivisa con**: U2 · **Percorso critico**: sì
**Produce**: `DA_HexMap_Arena` e `L_HexArena`, committati

**Cosa fai** — nessuna guida copre ancora questa procedura, quindi i passi stanno qui.

1. Livello nuovo in `/Game/RT/Maps/Dev/L_HexArena/`, con un `ARTHexMapActor`; l'asset mappa in
   `.../L_HexArena/Data/DA_HexMap_Arena` (stessa forma di `L_DevSandbox`, `convenzioni-contenuti-ue.md` §5).
   *Perché non estendere `DA_HexMap_Sandbox`*: il sandbox resta il banco per prove distruttive, l'arena è la
   mappa **stabile** su cui girano le verifiche — se la stessa mappa fa entrambe le cose, un esperimento
   invalida una verifica e non te ne accorgi.
2. Editor Mode **Hex Map** → tool **Paint**, `BrushRadius=4`, un click sull'origine: esagono pieno di raggio 4
   sul layer 0.
3. 2–3 celle con `bBlocksMovement` (ostacoli) e 2–3 celle con `bBlocksLineOfSight` **allineate** fra le due
   metà del campo — servono a `PIE-HEXPLAY-6`, che senza copertura non dimostra niente.
4. Una zona a costo alto (Mud o Water) con il tool **Fill**: è quella che fa mordere il budget in
   `PIE-HEXPLAY-3`.
5. Piattaforma di 3–4 celle sul layer 1 (`ActiveLayer=1`), collegata al layer 0 da **una sola** transizione,
   creata col tool **Arch**.
6. `bShowOverlay` attivo per rileggere il risultato a colori prima di salvare.

**Verifichi** — costruire l'arena esercita esattamente le voci del mode rimaste aperte:
`PIE-HEX-MODE-E`, `-F`, `-G`, `-H`, `-L`, `-N`, più `PIE-HEX-LAYER` e `PIE-HEX-TRANS`.
Le voci già ✅ (`-I`, `-J`, `-K`, `-M`) fanno da regressione.

**Finita quando**: i due asset sono committati e le otto voci sopra hanno un esito reale.

**Sblocca**: U2 · CP 6.8 (la mappa di prova è un suo DoD) · la **sessione A** di `test-manuali-pie.md` ·
il residuo editor parcheggiato in **M9 CP 9.1**.

> **Debito noto**: `FRTHexCellData` oggi non ha il campo cover; **E9 / CP 9.1** incrementa la versione del
> formato. Quest'arena andrà **migrata** in U13. Costruirla dopo E9 eviterebbe la migrazione ma lascerebbe M6
> senza banco di prova: il costo è accettato consapevolmente.

### U2 · Partita hex, primo giro ⏳

**Sbloccata da**: CP 6.1 🟡 (codice fatto), CP 6.2 🟡 (codice fatto) · **Preparazione condivisa con**: U1, U3
**Percorso critico**: sì
**Produce**: verdetto su `PIE-HEXPLAY-1/4/5` — cioè CP 6.1 e CP 6.2 da 🟡 a ✅

**Cosa fai**

1. `RTGameMode` come GameMode Override sul livello dell'arena (`../guides/debug-vs-unreal.md` §2).
2. Play. Le unità sono **cilindri**: i `BP_Unit_*` non esistono più in `Content/` e il fallback è previsto —
   non è un difetto. Se compare anche una griglia **quadrata**, nel livello c'è un `ARTGridActor` posato a
   mano: il GameMode non ne crea più, ma non rimuove quelli già presenti.
3. Due unità verso la stessa cella, lock-in con **Spazio** → contesa.
4. Ripeti con lo **scambio diretto A↔B**: è l'unico caso che i test headless non coprono.

**Verifichi**: `PIE-HEXPLAY-1`, `PIE-HEXPLAY-4`, `PIE-HEXPLAY-5`

**Finita quando**: le tre voci hanno un esito reale in `test-manuali-pie.md`.

**Sblocca**: U3 · chiusura di CP 6.1 e CP 6.2

---

## Blocco 2 — Parità hex (M6 / E2)

*Ogni seduta segue il checkpoint di codice che la abilita. L'ordine è quello del codice, non una scelta.*

### U3 · Input e pianificazione ⏳

**Sbloccata da**: CP 6.3 🟡 (codice fatto 2026-08-05, issue `#33`, suite 192/0) ·
**Preparazione condivisa con**: U1, U2 · **Percorso critico**: sì
**Produce**: verdetto su selezione, budget e anteprima del percorso

**Cosa fai**: selezioni un'unità, muovi il mouse sulla griglia, provi una cella **valida**, una **oltre il
budget**, una **bloccata** e una **occupata**. Su mappa multilivello controlli che la cella selezionata sia
quella del layer giusto (le celle sovrapposte non devono confondersi).

**Verifichi**: `PIE-HEXPLAY-2`, `PIE-HEXPLAY-3`
**Finita quando**: entrambe hanno esito reale · **Sblocca**: U4 · chiusura di CP 6.3

### U4 · Combat e linea di tiro ⏳

**Sbloccata da**: CP 6.4, CP 6.5 · **Percorso critico**: sì
**Produce**: verdetto su forme d'attacco, LOS esagonale e knockback

**Cosa fai**: attacco attraverso una cella che blocca la vista (deve essere scartato), poi da una cella di
lato (deve andare a segno); ostacolo su un **altro layer** (il tiro deve passare, regola di elevazione).
Il knockback a 6 direzioni è l'unico punto di M6 con una decisione di design dietro: guardalo, non solo
verificalo.

**Verifichi**: `PIE-HEXPLAY-6`
**Finita quando**: la voce ha esito reale · **Sblocca**: U5 · chiusura di CP 6.4 e CP 6.5

### U5 · Bot e HUD ⏳

**Sbloccata da**: CP 6.6, CP 6.7 · **Percorso critico**: sì
**Produce**: verdetto sul bot su hex **e** i pesi utility ritarati sulla scala esagonale

**Cosa fai**: partita con almeno un'unità `bIsBotControlled`; il log utility deve mostrare coordinate
**assiali** `(q,r,L)`. Poi taratura: `TurnManager` nel World Outliner → Details ▸ *Bot*, i pesi hanno effetto
**dal turno successivo senza ricompilare** (nota in `test-manuali-pie.md`, voce `PIE-BU2b`). I default vengono
dal quadrato: su hex vanno riguardati, non dati per buoni.

**Verifichi**: `PIE-HEXPLAY-7`, `PIE-HEXPLAY-9`
**Finita quando**: le due voci hanno esito reale e i pesi eventualmente modificati sono committati
**Sblocca**: U6 · chiusura di CP 6.6 e CP 6.7

### U6 · Multilivello e partita completa ⏳

**Sbloccata da**: CP 6.8 · **Percorso critico**: sì
**Produce**: **chiusura di M6 / E2** — sessione D verde

**Cosa fai**: percorso che usa la transizione fra layer 0 e 1 (l'unità deve cambiare quota, non teletrasportarsi);
rimuovi l'arco e verifica che il path **fallisca**. Poi una partita intera, dall'avvio alla vittoria.

**Verifichi**: `PIE-HEXPLAY-8` e la rilettura di `PIE-HEXPLAY-1..9` tutte insieme (sessione D)
**Finita quando**: le nove voci `PIE-HEXPLAY` sono ✅ · **Sblocca**: U16 · chiusura di CP 6.8, milestone M6, epic E2

---

## Blocco 3 — Presentazione (M8)

*Sbloccato da subito: il C++ è già in `main` (spawn `TSubclassOf` con fallback, facing, eventi di montaggio,
anello di team). **Fuori percorso critico**: nessuna di queste sedute blocca la v0.1.*

### U7 · Personaggi Paragon ⏳

**Sbloccata da**: già in `main` · **Preparazione condivisa con**: U8, U9 · **Percorso critico**: no
**Produce**: `BP_Unit_Guardian` (Gideon) e `BP_Unit_Ranger` (Sparrow), committati

**Cosa fai**: i 26 pack Paragon sono in `Content/FabAsset/Paragon/` — path `/Game/FabAsset/Paragon/<Pack>/…`,
non più `/Game/<Pack>/…` (`convenzioni-contenuti-ue.md` appendice B). ⚠️ Gideon, Sparrow e altri 3 pack sono
stati danneggiati dalla migrazione del 2026-08-06 e **vanno riscaricati da Fab** prima di usarli.
Procedura: [`guida-animazioni-paragon.md`](guida-animazioni-paragon.md) §AS.3 e §AS.4 punto 4.
Collocazione: `/Game/RT/Characters/<CharacterId>/Blueprints/` (§5); i pack di terze parti restano **fuori** da
`/Game/RT`. Assegna le classi a `GuardianUnitClass` / `RangerUnitClass` e tieni `VisualZOffset=0`.

**Verifichi**: `PIE-AS2`, `PIE-FACING`
**Finita quando**: i Blueprint sono committati e le due voci hanno esito reale sui BP nuovi
**Sblocca**: U8

### U8 · Animazioni ⏳

**Sbloccata da**: U7 · **Preparazione condivisa con**: U7, U9 · **Percorso critico**: no
**Produce**: `ABP_Gideon`, `ABP_Sparrow` e i montaggi Cast/Hit/Death

**Cosa fai**: procedura completa in [`guida-animazioni-paragon.md`](guida-animazioni-paragon.md) §AS.4a
(locomozione Idle↔Run pilotata dai delegate, **non** da `GetVelocity`) e §AS.4b (montaggi via eventi C++),
più §«Ripetere per il Ranger» per il duplicato.

**Verifichi**: `PIE-AS4a`, `PIE-AS4b`
**Finita quando**: gli asset sono committati e le due voci hanno esito reale · **Sblocca**: U9

### U9 · Leggibilità e riferimento visivo ⏳

**Sbloccata da**: U7, U8 · **Percorso critico**: no
**Produce**: il **video (o gli screenshot) di riferimento** che è il DoD di milestone di M8

**Cosa fai**: assegna `M_TeamRing` e `M_SelectionRing` — che esistono già in
`/Game/RT/Characters/Shared/Materials/` — sui Blueprint nuovi; il colore lo imposta il codice sul MID, un solo
materiale basta per entrambi gli anelli. Poi tara la camera sulla scala esagonale (`Camera Pitch`,
`Default Arm Length` sul `RTCameraPawn`, effetto immediato anche a PIE avviato) e giudica a schermo se i colori
delle superfici sono leggibili **in partita**, non solo nell'overlay dell'editor.

**Verifichi**: `PIE-AS5` e `PIE-SEL` — già ✅ sui cilindri, da **riverificare** sui personaggi skeletal
**Finita quando**: nessun cilindro in campo (salvo asset mancanti) e il riferimento visivo è nel repo
**Sblocca**: chiusura di CP 8.1, 8.2, 8.3 e della milestone M8

---

## Blocco 4 — Il contenuto diventa dati

*Grana media: il codice sotto è specificato ma non scritto. Le sedute vanno riviste all'apertura di E1 ed E6.*

### U10 · Data asset delle azioni ⏳

**Sbloccata da**: CP 1.3, CP 1.4 · **Percorso critico**: sì
**Produce**: il catalogo azioni della v0.1 come **dati**, non come codice

Oggi in `Content/` non esiste **nessun** data asset di abilità: le abilità sono di fatto hard-coded
(`roadmap-v0.1.md` §9 punto 3). Questa seduta è ciò che chiude quel buco.

**Verifichi**: il validator di CP 1.4 deve **rifiutare** un asset volutamente invalido (ID duplicato, fallback
mancante, variante senza svantaggio) e accettare i tuoi.

> **Conflitto da risolvere in CP 1.3, non qui.** `roadmap-v0.1.md` CP 1.3 chiede asset `PDA_*`;
> `convenzioni-contenuti-ue.md` §6 — documento **normativo** — assegna il prefisso `DA_` ai Data Asset e porta
> `DA_Hero_Flux` come esempio (§5). Le due fonti non concordano: va deciso in CP 1.3 e riflesso in una sola di
> esse. Anche la collocazione va fissata lì: §5 dà la regola (dati di feature vicino alla feature, cataloghi
> globali in `/Game/RT/Data/`), non l'elenco.

### U11 · I 4 eroi ⏳

**Sbloccata da**: E6 (CP 6.1–6.6 della roadmap v0.1) · **Percorso critico**: sì
**Produce**: i data asset di Flux, Riva, Bastion e Vektor, e lo spawn 2v2 che li usa

**Cosa fai**: un asset eroe per personaggio con statistiche distinte (90/95/120/100 HP, 5/5/4/6 MP), poi una
partita per vedere che il bot gestisca MP diversi senza proporre mosse illegali. Asset mancante = fallback al
cilindro, previsto.

**Verifichi**: `PIE-V01-ROSTER`
**Finita quando**: i quattro asset sono committati e la voce ha esito reale · **Sblocca**: U12

### U12 · Loadout ⏳

**Sbloccata da**: E7 · **Percorso critico**: no
**Produce**: varianti arma, gadget e moduli reazione come dati — 1 + 1 + 1 per eroe

*Provvisoria*: E7 è l'epic che la roadmap v0.1 dichiara tagliabile per prima se il tempo stringe.

---

## Blocco 5 — La mappa diventa un sistema

*Grana grossa: dipende da E8 ed E9, non ancora scritte.*

### U13 · Arena v0.1 ⏳

**Sbloccata da**: E8, E9 · **Percorso critico**: sì
**Produce**: l'arena estesa con quanto serve alle verifiche di contenuto

Estende l'artefatto di U1 con: una cella `Terrain.Rough`, una zona d'acqua adiacente a una superficie
conduttiva, una **porta** su un passaggio obbligato, una **copertura bassa** su un bordo esposto.

> Qui avviene la **migrazione di formato** annunciata in U1: `FRTHexCellData` guadagna il campo cover e la
> versione dell'asset sale. `DA_HexMap_Arena` e `DA_HexMap_Sandbox` vanno entrambi migrati, non ricostruiti.

### U14 · Ambiente in partita ⏳

**Sbloccata da**: U13 · **Percorso critico**: sì
**Produce**: verdetto sulle regole ambientali e strutturali

**Verifichi**: `PIE-V01-COLL`, `-ROUGH`, `-DASHCOVER`, `-PUSH`, `-ELEC`, `-FIREWATER`, `-LOWCOVER`,
`-INTERCEPT`, `-FF`, `-FALLBACK`, `-DOOR` — undici voci, che `test-manuali-pie.md` §sessione E apre
gradualmente man mano che E4, E5, E8 ed E9 chiudono.

---

## Blocco 6 — Chiusura della v0.1

### U15 · HUD, intenti, log e comandi debug ⏳

**Sbloccata da**: E11 · **Percorso critico**: sì
**Produce**: verdetto su leggibilità e osservabilità

Il punto che merita attenzione non è l'HUD: è che `rt.Debug.DrawIntent` **non deve** rivelare gli intenti
avversari (invariante #6, oggi banale perché offline — ma è ora che si crea l'abitudine sbagliata).

**Verifichi**: `PIE-V01-HUD`, `-INTENT`, `-LOG`, `-DEBUG`
**Finita quando**: le quattro voci hanno esito reale · **Sblocca**: chiusura di E11

### U16 · Misura dei KPI ⏳

**Sbloccata da**: U6 (serve una partita hex completa da misurare) · **Percorso critico**: sì
**Produce**: **numeri reali** nella tabella KPI di `roadmap-checkpoint.md` e `v0.1-definition-of-done.md`

Nessuna guida copre questa procedura, quindi i passi stanno qui.

1. Partita in PIE sull'arena, `stat unit` e `stat game` dalla console → FPS client (target 60).
2. Unreal Insights per path, preview e resolver (target: path mediana < 2 ms, preview < 50 ms,
   resolver < 100 ms/turno).
3. `rt.Debug.DumpTurnLog` + `rt.Debug.VerifyReplay` per replay divergence = 0 (`PIE-V01-REPLAY`).
4. I numeri vanno **registrati anche se fuori target**: un valore misurato vale più di un ⏳.

**Verifichi**: `PIE-V01-REPLAY` · **Sblocca**: CP 3.3 / CP 7.3 e CP 12.4

### U17 · Release v0.1 ⏳

**Sbloccata da**: E12 · **Percorso critico**: sì
**Produce**: build Windows **Development** e **Shipping**, e una partita completa giocata **senza editor**

L'invocazione esatta di `RunUAT BuildCookRun` si fissa alla prima esecuzione riuscita e si scrive qui: non va
inventata a tavolino.

**Finita quando**: BUILD SUCCESSFUL su entrambe le configurazioni e una partita conclusa dalla build packaged

---

## Esiti: chi scrive cosa

Tu esegui e mi passi l'esito — a voce, oppure il percorso di `Saved/Logs/*.log`, che leggo io.
Poi **una sola passata mia**: voce `PIE-*` aggiornata in `test-manuali-pie.md`, stato della seduta ricalcolato
qui, stato del checkpoint allineato in `roadmap-checkpoint.md`. Prima di ogni seduta compilo il target Editor
con l'editor **chiuso**, così non lo apri su binari vecchi.

**Quando una verifica va storta**: la voce resta ⏳ in `test-manuali-pie.md` con una **nota datata** — è la
convenzione che quel documento già usa (es. `PIE-HEX-MODE-C`: «il refresh dopo Undo richiedeva il fix
`ea51b45`»). Nessun simbolo nuovo. La seduta scende a 🟡 e la sua chiusura resta bloccata finché il fix non è
dentro; se serve apro la issue.

## Manutenzione

Una seduta nuova nasce quando un checkpoint di codice richiede l'editor e nessuna seduta lo copre.

La regola che impedisce la divergenza è una sola: **questa roadmap non ripete mai l'esito atteso di una voce
`PIE-*`**. Cita l'ID e basta. Se ti trovi a copiare qui una colonna «esito atteso», stai scrivendo nel file
sbagliato.

## Rapporto con gli altri documenti

| Documento | Ruolo |
|---|---|
| [`piano-canonico-mvp.md`](piano-canonico-mvp.md) | **Canone**: decisioni vincolanti e invarianti |
| [`roadmap-checkpoint.md`](roadmap-checkpoint.md) | **Esecuzione**: milestone M6–M11, checkpoint, DoD, stato |
| [`roadmap-v0.1.md`](roadmap-v0.1.md) | **Release v0.1**: 12 epic, 59 checkpoint |
| *questo file* | **Operativo in editor**: sedute, artefatti, ordine, dipendenze |
| [`test-manuali-pie.md`](test-manuali-pie.md) | **Registro delle verifiche**: esito atteso e stato delle voci `PIE-*` |
| [`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) | **Normativo**: dove va un asset, come si chiama, come si sposta |
| [`guida-animazioni-paragon.md`](guida-animazioni-paragon.md) | Procedura per personaggi, AnimBP e montaggi |
| [`../guides/debug-vs-unreal.md`](../guides/debug-vs-unreal.md) | Compilare, avviare, debuggare, eseguire i test |
