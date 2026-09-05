# `GOV-4` — il contratto «Dual Roadmap» come owner: decisione e emendamento

> `CURRENT` · **Stato**: decisione presa e registrata (`D-336`) · **Data**: 2026-09-05
> **HEAD della misura**: `origin/main` = `009ef817`. ⚠️ Durante la sessione `origin/main` si è mosso **tre
> volte** (`f335b6d8` → `5ee69775` → `009ef817`) e il branch del checkout condiviso è cambiato **due volte**
> sotto la run: la scrittura è avvenuta in un worktree isolato su `D:/rt-gov4`, mai nel working tree condiviso.
> **Oggetto**: chiudere `GOV-4` di [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) — *«il contratto `A/B`
> "Dual Roadmap" diventa un owner del repository, o resta un formato d'ingresso che ogni volta si revisiona?»*
> **Innesco dichiarato dalla voce**: *«il quarto work order della stessa forma, oppure una decisione d'autore
> sul canale con cui i kit entrano»*. Entrambi si sono verificati lo stesso giorno.
> 🔑 **Nessun numero qui è ricordato.** Ogni conteggio porta il selettore che l'ha prodotto, in §2.

---

## 1. Il verdetto in una riga

> **L'uscita è la *(c)*, che la voce non elencava: `AGENTS.md` §8 acquista una sottosezione invece che il
> repository un terzo contratto — e la ragione per cui *(c)* è legittima è che [`D-330`](../../decisions/RT_PDR_00_Decision_Log.md)
> aveva già stabilito che le fasi `A0…A6`/`B0…B7` *sono* «`AGENTS.md` §8–§10 con altri nomi».**

Registrata come [`D-336`](../../decisions/RT_PDR_00_Decision_Log.md).

---

## 2. Come è stata misurata

La voce dichiarava: *«ciò che nessuna misura può dire è quanti kit arriveranno ancora — con pochi vince *(a)*,
con molti vince *(b)*, e il numero è una scelta d'autore»*.

**Vero per il futuro. Falso per il ritmo del passato**, che è un dato del repository.

```bash
grep -rli "work order\|brief esterno\|mandato esterno\|kit d'autore\|handoff esterno\|\
fornito in chat\|consegna effimera\|brief operativo" docs/roadmap/plans/
```

| | Valore |
|---|---|
| Referti che consumano una consegna **esterna** | **17** |
| Referti totali in `docs/roadmap/plans/` | **136** |
| Il mandato che ha istruito questa decisione | il **18°** |

Distribuzione, dalla data nel nome del file:

| Periodo | Giorni | Kit | Ritmo |
|---|---|---|---|
| 2026-08-26 → 2026-09-01 | 7 | 9 | **1,3 / giorno** |
| 2026-09-04 → 2026-09-05 | 2 | 9 *(incluso quello di oggi)* | **4,5 / giorno** |

🔑 **Il ritmo è triplicato in undici giorni.** È il dato che sposta il peso da *(a)* — dove il costo della
revisione si ripete a ogni consegna — senza però giustificare *(b)*, per la ragione di §3.

### Due premesse corrette

🔴 **Una è mia.** Il report che ha innescato questa decisione affermava che il mandato del 2026-09-05 fosse
*«il quarto work order della stessa forma»*. **È falso in senso stretto.** Il contratto `A/B` compare in tre
file; quel mandato non lo usa — ha una struttura propria, `§1…§14`. È il quarto **genere**, non la quarta
**forma**. La correzione è scritta anche in `D-336`, perché cambia cosa la decisione governa: il **genere**.

⚠️ **Una è della voce stessa.** *«Tre istanze in due giorni»* sottoconta perché guarda il solo contratto `A/B`:

```bash
ls docs/roadmap/plans/ | grep -i "dual.roadmap"                      # 5
grep -rl "A0…A6\|B0…B7\|Dual Roadmap\|Dual-Roadmap" docs/roadmap/plans/  # 3
```

**Cinque** file portano `dual-roadmap` nel **nome**, **tre** contengono il contratto. L'etichetta si è
propagata più in fretta della regola — ed è di per sé un argomento sul canale d'ingresso.

---

## 3. L'istruttoria

### Perché non *(b)*

Sarebbe il **terzo** contratto di processo accanto ad `AGENTS.md` e `CLAUDE.md`. È il rilievo `R1` di due
referti consecutivi e il difetto che [`D-181`](../../decisions/RT_PDR_00_Decision_Log.md) ha pagato per
rimuovere sull'asse delle viste. La voce lo dichiarava già, e la misura non lo smentisce.

### Perché non *(a)*

Al ritmo di §2 la revisione si paga ~4,5 volte al giorno, e ogni volta riscopre la stessa cosa.

### Perché *(c)* non è un'invenzione

Il repository l'ha già usata **il giorno prima**: `D-330` ha emendato `AGENTS.md` §9 con la sottosezione
*«Authoring e acceptance»* invece di creare un documento. E `D-330` stessa aveva stabilito che le fasi `A/B`
*sono* «§8–§10 con altri nomi»: se sono già quelle sezioni, il posto dove scrivere la regola è quella
sezione.

### La misura che decide non è il ritmo

È la **ripartizione del valore dentro un kit**. Delle quattordici sezioni del mandato del 2026-09-05, quelle
procedurali erano già canone:

| Sezione del kit | Dove era già scritta | Nota |
|---|---|---|
| §2 ricognizione | `CLAUDE.md` §1 *Context protocol* | — |
| §3 anti-duplicazione | `CLAUDE.md` §7 — *SEARCH → REUSE / UPDATE → CREATE* | — |
| §13 formato del report | `CLAUDE.md` §9 | ✅ la versione del repository è **migliore**: impone `NOT RUN` |
| §8 «crea dieci epic con questi titoli» | `CLAUDE.md` §7 | ⛔ **in conflitto**: vieta di assegnare contatori condivisi dalla memoria |

Il valore stava **interamente** nelle 50 candidate tecniche. ∴ un kit non si accetta né si rifiuta in blocco:
se ne prende il contenuto e se ne ignora il preambolo. **Questa è la regola scritta in §8.**

### Il precedente di specie

[`roadmap-issues-v01-v10-spec-panel-2026-08-29.md`](roadmap-issues-v01-v10-spec-panel-2026-08-29.md) consumò
un work order della stessa specie — misura lo stato, riconcilia GitHub, aggiorna gli owner — e il verdetto fu
*«rigorosa nel metodo e scaduta nei fatti»*. Il 2026-09-05 si è ripetuto su premesse diverse: `S9 Deflect` era
chiusa da `D-309`/`D-312`, il *«magic number 20»* era già `static constexpr DeflectDamageReduction`, e il
*«conflitto Anchor/CoverSelection»* non esiste — `D-302`/`D-303` hanno risolto sei voci `COV-*` su otto.

Da qui la prima clausola della sottosezione: **misura le premesse, decadono in ore**.

---

## 4. Cosa è stato scritto

| File | Cosa |
|---|---|
| [`AGENTS.md`](../../../AGENTS.md) §8 | nuova sottosezione *«Un work order esterno»* — 28 righe, in coda a *Prima/Durante/Dopo* |
| [`RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) | `D-336`, più la nota di assegnazione del numero |
| [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | `GOV-4` **barrata** con l'esito; l'istruttoria originale è **conservata** dietro *«Istruttoria conservata»*, come il documento impone |

### `D-336` — il numero, e la collisione che ha reso `D-335` inutilizzabile

**Primo tentativo: `D-335`.** Verificato nei tre posti che la nota su `D-304` prescrive, su `origin/main` =
`009ef817`: registro a `D-334`; **nessuna** delle PR aperte tocca il file; **nessuno** dei **10** branch
remoti, diffati contro `origin/main` sul solo registro, rivendica un `D-3nn` che `main` non abbia già.

🔴 **Alla riverifica prima del merge la collisione c'era.** `origin/docs/gov-5-busta-payload` rivendica
`D-335` per `GOV-5`, con un commit delle **18:25:13**. Il mio è delle **18:23:53** — ottanta secondi prima —
e **nessuna delle due sessioni poteva vedere l'altra**.

🔑 **Il metodo non era difettoso: il branch non esisteva ancora al momento del controllo.** Rieseguito adesso,
lo stesso `git diff origin/main..<branch>` sul solo registro lo trova. Il controllo misura un **istante**.

⚠️ **Ed è la lezione che vale più del numero.** Fra la misura a tre posti e il merge c'è una finestra in cui
una seconda sessione può rivendicare lo stesso ID senza che nessuna delle due se ne accorga — e ottanta
secondi bastano. La riverifica prima del merge non è prudenza: è **l'unico** momento in cui quella finestra si
chiude. Il precedente di `D-323`, che al merge si trovò **79 commit** più avanti, diceva già questo; qui la
finestra si è chiusa su un intervallo di un minuto e venti.

✅ **Ceduto `D-335`, preso `D-336`** — verificato libero su `origin/main` e su **tutti** i branch remoti alla
stessa misura, insieme a `D-337`…`D-340`. La cessione non è un giudizio sul merito: è l'opzione che non chiede
niente all'altra sessione, ed è più economica di un coordinamento che nessun canale rende possibile.

⚠️ **Va riverificato di nuovo prima del merge**, per la ragione appena scritta.

---

## 5. Limiti, e ciò che non è stato fatto

⛔ `NOT RUN`: build, `rt-suite`, Automation Test, PIE, packaged. Il write-set è di **soli documenti** e non
tocca `Source/`: `AGENTS.md` §9 non chiede un build a un write-set senza sorgenti.

⚠️ **La decisione non predice il futuro.** Se i kit cessassero domani, la sottosezione resterebbe vera e
inerte — costa quaranta righe in una sezione che già esiste. È il motivo per cui *(c)* è difendibile anche
nell'ipotesi che favoriva *(a)*, e va detto perché è il punto debole del ragionamento di §2: il ritmo misurato
è un dato del **passato**.

⛔ **Non è stato pubblicato un template a cui i kit debbano conformarsi.** Sarebbe *(b)* per un'altra via: chi
scrive un work order resta libero della forma, ed è il repository a dichiarare cosa ne legge.

⚠️ **Il kit del 2026-09-05 non è versionato**, coerentemente con la clausola appena scritta: resta citabile
solo qui e nel report della sessione che lo ha consumato.
