# Consolidamento della roadmap — 2026-08-10

> `CURRENT` come **verbale datato**, non come vista di stato. Registra cosa è stato verificato, cosa era già
> a posto e le quattro correzioni applicate. Lo stato vero resta nei suoi owner:
> [`roadmap-checkpoint.md`](../roadmap-checkpoint.md) · [`roadmap-v0.1.md`](../roadmap-v0.1.md) ·
> `feature-registry.yaml` · [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md).
> **Misurato su** `main @ 918f54c`, branch `docs/consolidamento-roadmap`.

## 1. Cosa era già consolidato

Il consolidamento documentale del 2026-08-08 ha retto. Verificato, non assunto:

| Verifica | Esito |
|---|---|
| Banner di autorità sui 9 documenti di roadmap | ✅ coerenti: 5 `CURRENT`, 3 ritirati (`HISTORICAL`/`DELIVERED`), 1 `CURRENT come requisiti, non come stato` |
| Numerazione delle **epic** | ✅ nessuna collisione: `E1`–`E21` in v0.1, `E22`–`E35` in post-v0.1. Le apparenti sovrapposizioni sono citazioni incrociate, non dichiarazioni |
| I 4 puntatori orfani a `roadmap-editor.md` | ✅ riparati dalla PR #373 |
| `feature_registry.py validate` | ✅ 0 errori |
| Piani in `plans/` senza intestazione | ✅ **falso positivo**: hanno tutti un banner, con etichette diverse (`AS-BUILT`, «piano consegnato»). Il primo grep cercava solo quattro parole e ne ha mancate tre |

L'ultima riga vale come promemoria: uno strumento di scansione scritto sul momento produce difetti che non
esistono. Nove file sono stati riaperti a mano prima di dichiararli rotti, e nessuno lo era.

## 2. Difetto 1 — `CP n.m` non è risolvibile (corretto)

I due spazi di numerazione dei checkpoint **collidono su 20 numeri su 22**: `6.3` è «Input e pianificazione»
in M6 e «Phase» in E6; `7.2`, `8.1`, `9.1`, `10.1` idem. Il `feature-registry.yaml` si era già difeso — un
checkpoint di milestone non si scrive mai nel campo `checkpoints` — ma `editor-sessions.yaml`, creato il
2026-08-10, usava la forma nuda `CP 6.3` **e mescolava i due spazi senza segnalarlo**:

| Prima | Namespace reale | Dopo |
|---|---|---|
| `CP 6.0`…`CP 6.8` (U1–U6) | milestone M6, parità hex | `M6.0`…`M6.8` |
| `CP 1.3, CP 1.4` (U10) | epic E1, tipi C++ e validator | `E1.3, E1.4` |

Sette record corretti, e la convenzione è ora **scritta nel commento del campo**: `M6.3` · `E1.3` · `E8` ·
`U13`, forma nuda vietata. La vista si è rigenerata da sola (`shortlist`, idempotente alla seconda run).

**Perché non era cosmetico.** Un lettore umano disambigua dal contesto del blocco; un comando no. La
derivazione dei gruppi `BLOCKING/READY/WAITING/DONE` decisa lo stesso giorno
([`project-control-center-spec.md`](project-control-center-spec.md) §9, D-C) legge esattamente questo campo:
prima della correzione avrebbe risolto `CP 6.1` contro «`URTHeroData` e statistiche» e prodotto un
prerequisito plausibile e sbagliato.

## 3. Difetto 2 — due gate di release verdi dichiarati ⏳ (corretto)

Il 2026-08-10 il commit `16f6182` ha eseguito CP 12.5 e scritto l'evidenza in `roadmap-v0.1.md`:

```text
Packaging Development   BUILD SUCCESSFUL · 916 MB
Packaging Shipping      BUILD SUCCESSFUL · 569 MB
Partita senza editor    Vince il team 1 — round 6/12, per eliminazione, zero crash
```

**Non ha toccato `v0.1-definition-of-done.md`**, dove `G12` e `G13` continuavano a dichiarare ⏳. Il documento
che dice quando la v0.1 è consegnabile faceva sembrare non fatto il lavoro più costoso della release.

Corretto: `G12` → ✅ con l'evidenza; `G13` → 🟡 con la riserva dichiarata, perché la partita gira su
`MapSource=GeneratedTestArena` e la via a punti non è mai stata esercitata. È la stessa ragione per cui il
gate `packaged` delle feature resta chiuso: **il gioco si pacchettizza, la release non esiste ancora.**

Effetto a valle, automatico: `milestonemap.shortlist.md` è passata da «15 gate · verdi: **0**» a «verdi:
**1**» senza che nessuno scrivesse quel numero.

## 4. Difetto 3 — numeri stantii nella mappa dei documenti (corretto)

La tabella «Rapporto con gli altri documenti» di `roadmap-checkpoint.md` descriveva viste che sono cresciute:

| Diceva | È |
|---|---|
| `roadmap-v0.1.md`: «14 epic, 69 checkpoint» | **21 epic, 95 checkpoint** (l'owner lo dichiara alla propria riga 323) |
| `v0.1-definition-of-done.md`: «gate `G1`–`G14`» | **`G1`–`G15`** |
| `roadmap-post-v0.1.md` | **assente dalla tabella**, pur essendo `CURRENT` e owner di `E22`–`E35` |

Corretti tutti e tre. Il banner del DoD dichiarava «ultimo aggiornamento 2026-08-05» mentre conteneva note
datate 2026-08-09: allineato a 2026-08-10.

## 5. Difetto 4 — il Control Center non aveva un posto (corretto)

Il lavoro descritto in [`project-control-center-spec.md`](project-control-center-spec.md) viveva solo in un
piano. Ora è `RT-FEAT-TOOL-CONTROL-CENTER` nel registry: `release: future`, `status: DESIGNED` derivato da
`spec: partial`, **nessuna epic e nessuna milestone**.

La collocazione è la decisione, non un dettaglio. Non in v0.1 — competerebbe con i 15 gate. Non in
post-v0.1 — quel documento non apre lavoro e possiede gameplay. È tooling di processo, e il precedente
esatto è `RT-FEAT-UI-SCENARIO-BROWSER`, tracciato allo stesso modo. Il registry ora conta **85 feature**,
`validate` resta a **0 errori**.

## 6. Cosa resta aperto

- **La numerazione dei checkpoint resta ambigua alla fonte.** Qui è stato corretto l'unico consumatore
  automatico (`editor-sessions.yaml`); la prosa di altri documenti continua a scrivere `CP 8.2` in forma
  nuda. Non è un difetto finché nessun comando la legge — ma è la stessa trappola, e la convenzione
  prefissata esiste dal 2026-08-08.
- **`roadmap-v0.1.md` dichiara 95 checkpoint, un conteggio a regex ne trova 94.** Non è stato inseguito:
  l'owner vince e la differenza può stare nella regex. Se un giorno quel numero diventa derivato, si risolve
  da sé.
- **Nessuna CI.** `validate`, `generate --check` e `shortlist --check` sono verdi perché qualcuno li ha
  eseguiti. È la premessa di `R-4` della spec del Control Center e il motivo per cui quella pagina deve
  mostrare quanto è vecchio il dato che dipinge.
