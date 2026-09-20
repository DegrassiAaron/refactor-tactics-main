# Intenti condivisi — il piano di squadra che attraversa la rete

> `CURRENT` · **Data**: 2026-09-20 · **Owner** di *«Intenti condivisi»* come requisito
> **Autorità**: subordinato a [`product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) e al
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md). Dove questo documento e una decisione
> divergono, vince la decisione.
> **Sorgente recepita**: [`research/prd/prd-architettura-rete-e-intenti.md`](../../research/prd/prd-architettura-rete-e-intenti.md)
> §*«Intenti condivisi — PRD di feature completo»*, recepito da
> [#577](https://github.com/DegrassiAaron/refactor-tactics-main/issues/577).
> **Cosa non è**: non è un piano di esecuzione, non assegna lavoro e non apre checkpoint. Dice **quali
> requisiti esistono, con quale priorità originale, e a quale checkpoint di M10 appartengono** — così che
> `M10.1`–`M10.3` partano da requisiti con un ID invece che da trenta pagine di PRD.

---

## 0. La precondizione, misurata: in v0.1 la feature non ha soggetto

⛔ **Non è una scelta di scheduling: è una proprietà del formato.**

```
Source/RefactorTactics/Combat/RTCombatLibrary.h:446
  Format.Skirmish2v2 → UnitsPerPlayer = 2 su UnitsPerTeam = 2
  «un posto per squadra e un gruppo solo»
```

In v0.1 **il giocatore controlla entrambe le unità della propria squadra**, e l'avversario è un bot. Non
esiste un compagno umano a cui *condividere* un intento: il dato alleato è già interamente del giocatore,
e nessun byte deve attraversare la rete per arrivargli. È il fatto misurato da
[#3115](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3115), che ne possiede la
conseguenza sull'HUD — *«l'anello dice quale unità stai pianificando, il suo piano no»*.

🔑 **Perciò tutto ciò che segue è post-v0.1 per costruzione**, e l'unica cosa che vale già oggi è
l'**invariante di privacy**: nessun intento avversario in `GameState`, in `PlayerState`, su Actor
`AlwaysRelevant`, né nel log pubblico prima del momento autorizzato. Quell'invariante non aspetta M10 — è
presidiata offline da `Privacy.ServerOnlyTypesAreNotReplicated`
([#589](https://github.com/DegrassiAaron/refactor-tactics-main/issues/589)) e dalle primitive già in
codice, `URTIntentPrivacyLibrary::FilterForTeam` → `FRTIntentView`.

---

## 1. I requisiti, con la loro priorità originale

Il PRD assegna a ciascun `FR-*` una **priorità** e una **stima**. Sono dati della sorgente, riportati qui
senza rinegoziarli: servono a sapere cosa il capitolato considerava indispensabile, non a pianificare.

### `M10.2` — Piani team-only

> *«I piani viaggiano in DTO filtrati per squadra: nessuna replica globale con occultamento grafico»*
> — [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) §M10

| `FR-*` | Requisito | Priorità | Stima |
|---|---|---|---|
| `FR-TRAJ` | Traiettorie: spline o sequenza di celle, cambi di livello, punti di transizione, destinazione | `P0` | 3–5 gg |
| `FR-AOE` | Aree d'effetto dalla posizione **prevista**: cerchio, cono, linea, anello, forma custom | `P0` | 3–5 gg |
| `FR-TARGET` | Bersagli: personaggio, cella, oggetto, elemento della mappa | `P0` | 2–3 gg |
| `FR-LABEL` | Etichette d'intento da vocabolario **chiuso**: `Focus`, `Protezione`, `Scout`, `Escape`, `Trappola` | `P0` | 1–2 gg |
| `FR-STATE` | Stato del giocatore: `Editing`, `Ready`, `Locked`, `Disconnected`, `AFK/Timeout` | `P0` | 2–3 gg |
| `FR-GHOST` | Ghost dell'azione prevista — posizione, orientamento, stance, volume — **senza muovere l'attore** | `P0` | 5–8 gg |
| `FR-DRAW` | Disegno sulla mappa: tratti e frecce temporanei team-only, con limiti, timeout e mute | `P1` | 4–6 gg |

### `M10.1` — Autorità server

> *«Ogni decisione di gameplay è calcolata sul server; il client produce solo preview»*

| `FR-*` | Requisito | Priorità | Stima |
|---|---|---|---|
| `FR-CONFIRM` | Conferma, annullamento prima del lock, blocco immutabile allo scadere | `P0` | 3–5 gg |
| `FR-ROLLBACK` | Ripristino di una revisione precedente **generando una nuova revisione canonica** | `P1` | 4–6 gg |
| `FR-RECONNECT` | Snapshot completo degli intenti visibili e dello stato del turno dopo reconnect | `P1` | 3–5 gg |

⚠️ Nessuno dei tre è UI: il server assegna la revisione canonica, blocca al timeout e ricostruisce lo stato.

### `M10.3` — Canary anti-leak

| `FR-*` | Requisito | Priorità | Stima |
|---|---|---|---|
| `FR-CONFLICT` | Collisioni, fuoco amico, celle contese, risorse duplicate, azioni incompatibili | `P0` | 8–12 gg |

⚠️ È la stima singola **più alta** del capitolato, e il §2 spiega perché non è solo un vincolo.

### Fuori da M10 — presentazione locale

| `FR-*` | Requisito | Priorità | Stima | Perché fuori |
|---|---|---|---|---|
| `FR-NOTIFY` | Notifiche di cambio bersaglio, annullamento, conferma, rollback — senza spam | `P1` | 2–4 gg | non attraversa la rete: è una reazione locale a un dato già ricevuto |
| `FR-HISTORY` | Cronologia delle revisioni e timeline sintetica dei cambiamenti | `P1` | 3–5 gg | idem — il dato lo produce `FR-ROLLBACK`, la sua **vista** è locale |
| `FR-FILTER` | Filtri e accessibilità: nascondere overlay, intensità, pattern oltre al colore, color blindness | `P1` | 3–5 gg | idem, ed è un requisito di **resa**, non di protocollo |

### Fuori scope

| `FR-*` | Requisito | Priorità |
|---|---|---|
| `FR-MOD` | Etichette, renderer e tipi d'intento data-driven, con schema versionato | `P2`, e il PRD stesso lo marca *«ultima feature»* |

---

## 2. Tre correzioni alla mappatura proposta

La tabella di #577 era dichiarata *«proposta, da validare»*. Validata, regge — con tre correzioni, tutte
nella stessa direzione: **tre `FR-*` hanno già un owner per metà**, e assegnarli interi a M10 aprirebbe un
secondo owner su qualcosa di già deciso.

### `FR-GHOST` è già posseduto, ma solo per il ghost **proprio**

[`brief-planning-visuale.md`](brief-planning-visuale.md) possiede *Action Ghost* e *Ghost Timeline* — come
il giocatore guarda **il proprio** piano prima di confermarlo — ed è destinato all'epic `E11`, `CP 11.5` e
`CP 11.6`. ∴ **M10.2 non possiede il ghost**: possiede il ghost costruito da un **DTO filtrato**, cioè il
caso in cui i dati da cui il ghost nasce sono arrivati dalla rete e appartengono a un altro giocatore. Il
renderer è lo stesso; la **sorgente del dato** no, ed è l'unica metà che M10 deve validare.

### `FR-STATE` è già posseduto per `Ready` / `Editing`, e quella metà è atterrata

[#2193](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2193) possiede Ready, Unready,
countdown, soggetto del Ready e quorum, ed è **chiusa `COMPLETED` il 2026-09-09**. ✅ Il confine d'autorità
è deciso: il countdown vive nella **presentazione** — `ReadyCountdownSeconds` sta fra i *Tempi UX* di
[`spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md) §11 — e
#2193 è atterrata **senza countdown in snapshot, `TurnLog` o `StateHash`**.

∴ di `FR-STATE` M10.2 possiede: la **propagazione agli alleati** di uno stato che esiste già, più i due
valori che oggi non hanno soggetto — `Disconnected` e `AFK/Timeout` — perché nascono con la rete.

### `FR-CONFLICT` non è solo un vincolo, ed è metà già fatta

#577 lo classifica *«vincolo, non feature»*. ⚠️ **Il capitolato gli dà `P0` e 8–12 giorni**: la stima
singola più alta di tutte. Un vincolo non costa dodici giorni. Sono **due cose** sotto un ID:

| Metà | Stato |
|---|---|
| l'**errore bloccante** — abilità senza linea di tiro, percorso fuori mappa | ✅ **posseduto e atterrato**: `CP 38.2`, [#605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/605) *«validazione del piano in Planning, con reason code deterministico»*, owner documentale [`spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md) |
| l'**avviso consultivo** — stessa cella, fuoco amico previsto, risorsa duplicata | ⏳ **non posseduto**: nessun owner, e in v0.1 non ha soggetto (§0) |

🔑 **E la parte che M10.3 verifica è una terza cosa ancora**: che l'avviso sia **calcolabile senza il piano
avversario**. Se un warning è producibile solo conoscendo la mossa nemica, il canary deve fallire — è
esattamente ciò che [`procedura-canary-anti-leak.md`](procedura-canary-anti-leak.md) esiste per misurare.
Le tre metà non si fondono: la prima è chiusa, la seconda è lavoro, la terza è un gate.

---

## 3. Le assunzioni del PRD, dichiarate invece che ereditate

⛔ **Il capitolato è datato, e le sue assunzioni non sono le nostre.** Ereditarle in silenzio significa
ereditare anche la stima che ne dipende.

| Voce | Assunzione del PRD | Realtà v0.1 | Conseguenza |
|---|---|---|---|
| Modalità | PvP **4v4** | **2v2 offline** contro bot | la feature **non ha soggetto** (§0); a 4v4 gli overlay concorrenti sono il doppio, e `FR-TRAJ`/`FR-AOE` scalano con la sovrapposizione, non coi giocatori |
| Modello di rete | **dedicated server** autoritativo | nessuna rete; `M10.1` prevede **listen server** | «il server non è un client» smette di essere gratis: su listen server l'host **è** un giocatore, e il filtro per squadra deve reggere in-process |
| Durata planning | **20–30 s**, configurabile | **30 s** (`RTTurnManager.h:239`), *«da tarare sul misurato»* | compatibile per il 2v2; per il 3v3 la spec prevede **40–45 s**, e i budget di frequenza del §5 sono per-secondo, quindi non cambiano |
| Engine | UE **5.7 o 5.8** | UE **5.8.1**, versione fissata | nessuna |
| Scalabilità | otto giocatori, design compatibile con sedici | 2v2 = quattro unità, **un** giocatore umano | i budget di banda del §5 sono **per client**: restano leggibili, ma non sono mai stati misurati su questo formato |

🔴 **La stima di 58–89 giorni-persona dipende da tutte e cinque, e non va citata come nostra.** È il numero
che il capitolato ha prodotto per un 4v4 con dedicated server. Riportarlo senza le assunzioni è il modo in
cui una stima altrui diventa un impegno proprio.

---

## 4. Cosa la v0.1 scarta, e perché — elencato invece che omesso

| Scartato | Ragione |
|---|---|
| **tutti** i `FR-*` di `M10.1`/`M10.2`/`M10.3` | §0: non c'è un compagno umano, e non c'è rete. Non è un rinvio di priorità |
| `FR-NOTIFY` · `FR-HISTORY` · `FR-FILTER` | candidati a **M8 Presentazione**, e ciascuno presuppone un dato che oggi non esiste (un intento *altrui* da notificare, revisionare, filtrare) |
| `FR-MOD` | `P2` nel capitolato, *«ultima feature»* per il capitolato stesso, e il canone lo colloca fuori dal perimetro corrente |
| la **previsione delle mosse nemiche** | fuori ambito nel PRD stesso, **e** vietata dall'invariante di privacy: non è una feature rinviata, è una cosa che non si costruisce |
| il **testo libero** nelle etichette | fuori ambito nel PRD; `FR-LABEL` resta a vocabolario chiuso, che è anche ciò che rende l'etichetta traducibile e moderabile |
| la **risoluzione automatica** dei conflitti tattici | fuori ambito nel PRD, e in conflitto con la regola del §5: due unità sulla stessa cella sono una *strategia possibile* |

---

## 5. Le due regole che non stanno scritte altrove

Sono le uniche due prescrizioni del capitolato che nessun altro documento del repository porta, ed è la
ragione principale per cui #577 chiedeva un owner invece di un link al PDF.

### 1. Il rollback è monotono

**Ripristinare una revisione non riusa il vecchio numero**: ne crea uno **nuovo** che ne copia il
contenuto.

⚠️ Non è pulizia formale, è la condizione che rende sicuro il trasporto. Gli update di preview sono
`unreliable` e possono arrivare **fuori ordine**; il ricevente applica solo revisioni **maggiori** di
quella già mostrata. Se un rollback riusasse un numero già visto, un pacchetto in ritardo potrebbe
sovrascrivere il piano ripristinato e nessuno se ne accorgerebbe — il difetto sarebbe visibile solo come
*«il piano è tornato indietro da solo»*, che è irriproducibile per costruzione.

### 2. Avvisi consultivi ↔ errori bloccanti

| Caso | Trattamento | Perché |
|---|---|---|
| due unità che terminano sulla **stessa cella** | ⚠️ **si segnala** | è una *strategia possibile*, non un errore: la contesa ha già regole deterministiche |
| fuoco amico previsto, risorsa duplicata | ⚠️ si segnala | idem: può essere voluto |
| abilità **senza linea di tiro**, percorso **fuori mappa** | ⛔ **si blocca** | è formalmente invalido, e lasciarlo passare sposterebbe la decisione nel resolver |

⛔ **La UI non risolve il conflitto**: segnala e lascia decidere. La risoluzione segue le regole del gioco.

---

## 6. I requisiti non funzionali, coi numeri della sorgente

| Area | Requisito |
|---|---|
| Latenza | risposta locale **entro un frame**; aggiornamento alleato mediano **< 100 ms**, `p95` **< 200 ms** con `RTT ≤ 120 ms` |
| Banda | massimo tipico **5 KB/s** per client per la feature; picco consigliato **< 10 KB/s** |
| Frequenza | preview limitate a **10–12 aggiornamenti/s** per giocatore; rendering locale a frame rate pieno |
| Disegno | **otto** tratti per giocatore, **64** punti per tratto |
| Sicurezza | ogni payload client validato dal server: target, turno, ownership, dimensione, frequenza, coordinate |
| Privacy tattica | il server non invia intenzioni, revisioni, target o identificatori nascosti ai client nemici |
| Affidabilità | conferma, lock, rollback e snapshot convergono **anche con perdita di pacchetti** |
| Osservabilità | telemetria di latenza, dimensione payload, revisioni scartate, conflitti, fallimenti di validazione |
| Accessibilità | nessuna informazione codificata **solo** col colore: opacità, spessore, icone, pattern |
| Privacy utente | disegni e cronologia **non persistono** oltre il match, salvo replay o telemetria configurata |

⚠️ **Sono numeri del capitolato, misurati su nessuna delle nostre build.** Diventano budget quando una
sonda li misura; fino ad allora sono il bersaglio dichiarato dalla sorgente.

⛔ **Il trasporto ha un divieto esplicito, e vale già oggi**: nessun `NetMulticast` che contenga
intenzioni private. Un multicast viene eseguito su ogni macchina per cui l'Actor è rilevante, e **non è
una separazione fra squadre**.

---

## 7. Cosa questo documento NON possiede

- ⛔ **la sequenza del turno** — è [`spec-sequenza-turno.md`](../../gameplay/spec-sequenza-turno.md);
- ⛔ **Ready / Unready / countdown / quorum** — è #2193, chiusa, e il countdown vive nella presentazione;
- ⛔ **la validazione bloccante del piano** — è `CP 38.2` / #605, con owner in
  [`spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md);
- ⛔ **il ghost del proprio piano** — è [`brief-planning-visuale.md`](brief-planning-visuale.md);
- ⛔ **il coordinamento del bot alleato** — è `CP 26.4`, con
  [`spec-bot-tattico.md`](../../gameplay/spec-bot-tattico.md) §3 e `D-096`;
- ⛔ **la procedura del canary** — è [`procedura-canary-anti-leak.md`](procedura-canary-anti-leak.md);
- ⛔ **lo scope e lo stato di M10** — è [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md).

Questo documento possiede **i requisiti**: quali sono, con che priorità li ha scritti il capitolato, a
quale checkpoint appartengono, e cosa la v0.1 scarta con la sua ragione.

---

## Provenienza

Il capitolato viveva in un PDF fino al **2026-08-12**, quindi non era né citabile né `grep`-abile; `D-009`
lo ha portato in Markdown. Fino al 2026-09-20 restava **livello 9** della gerarchia delle fonti —
*visione north-star, non normativa* — e #577 chiedeva di recepirlo in un owner prima che M10 cominci,
perché a livello 9 sarebbe sparito dal radar.

⚠️ **Recepire non è promuovere la sorgente.**
[`research/prd/`](../../research/prd/prd-architettura-rete-e-intenti.md) resta livello 9 e non diventa
autorità: è questo documento a esserlo, per i requisiti che elenca. Le trenta pagine restano la
provenienza, e vanno rilette in originale per tutto ciò che qui non compare.

⚠️ **`RT-FEAT-NET-PRIVATE-PLANNING` non ha più una definizione.** #577 chiedeva che il suo `owner_specs`
puntasse a questo documento: il Feature Registry è uscito dal repository con `D-181` il 2026-08-21,
insieme a `scripts/feature_registry.py`. L'identificatore resta leggibile come **nome** e non ha un
registro dove atterrare; lo stato delle feature vive in
[`roadmap-v0.1.md`](../../roadmap/roadmap-v0.1.md) e
[`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md).

✅ **Nessun `FR-*` confligge col canone**, verificato requisito per requisito: nessuna riga va aggiunta a
[`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md). Le tre sovrapposizioni trovate (§2) non sono
conflitti — sono **owner già esistenti**, e il rimedio è linkarli, che è ciò che il §7 fa.
