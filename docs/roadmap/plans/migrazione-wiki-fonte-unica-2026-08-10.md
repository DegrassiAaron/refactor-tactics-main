# Migrazione — `docs/wiki/` sparisce, il clone diventa la fonte

> `CURRENT` · **Creato**: 2026-08-10 · **Decisione**: [D-076](../../decisions/RT_PDR_00_Decision_Log.md)
> **Owner di una sola domanda**: cosa deve succedere **prima** che `docs/wiki/` si possa cancellare.
>
> ⚠️ **Non cancellare `docs/wiki/` finché la §2 non è chiusa.** Contiene 270 righe che il clone non ha.

## 1. La decisione e come ci si è arrivati

Il repository aveva **due** sorgenti della Wiki: `docs/wiki/` (versionato col codice) e il clone
`refactor-tactics-main.wiki` (pubblicato). Misurato il 2026-08-10: **37 pagine su 43 divergenti**, in
**entrambe** le direzioni.

Il motivo non era la disattenzione: `apply_wiki_deploy` sincronizza **solo i blocchi `RT_FEATURE_STATUS`**,
mai il corpo. Nessuno ha mai pubblicato la prosa, e nessun `--check` se ne accorgeva.

**Decisione dell'autore: il clone diventa la fonte unica, `docs/wiki/` sparisce.**
Decisione collegata: i link che puntano dentro il repository (Decision Log, ADR, cataloghi) **si tolgono**,
resta il nome in grassetto — la Wiki diventa autonoma, nessun link che marcisce, nessun path interno esposto
al giocatore.

## 2. Cosa va portato prima — 270 righe, 16 pagine

Misurate **dopo** aver normalizzato i link (quindi sono differenze di contenuto, non di sintassi) e dopo aver
scartato le righe che nel clone compaiono altrove:

```
TOTALE righe di sorgente assenti dal clone: 270  su 16 pagine

    17  Fazioni/conflux.md
          ## Identità
          ## Affinità sistemica
          ## Colore
    17  Fazioni/constrine.md
          ## Identità
          ## Affinità sistemica
          ## Colore
     7  Fazioni.md
    17  Fazioni/resonance.md
          ## Identità
          ## Affinità sistemica
          ## Colore
    17  Fazioni/sentinel-directorate.md
          ## Identità
          ## Affinità sistemica
          ## Colore
     2  Guida/avversario-bot.md
    35  Guida/azioni-e-movimento.md
          ## Non tutti eseguono la stessa azione allo stesso modo
     4  Guida/che-cose-refactortactics.md
     7  Guida/mappa-terreni-e-ambiente.md
    53  Guida/reazioni-overwatch-e-previsioni.md
          ### Reaction Clash
          ## La tua riserva di tempo
          ## Irrigidirsi prepara una reazione
    26  Guida/sinergie-e-combinazioni.md
          ## Tre livelli
          ### Kit del personaggio
          ### Interazione sistemica
     9  Meccaniche/acqua-e-elettricita.md
    35  Meccaniche/facing-e-direzionalita.md
          ### Mentre ti stai muovendo
     9  Meccaniche/griglia-e-geometria.md
    11  Meccaniche.md
     4  Meccaniche/topologia-dinamica.md
```

Non sono rifiniture: sono **sezioni intere**. `Reaction Clash`, `La tua riserva di tempo`, `Tre livelli`,
`Mentre ti stai muovendo`, e `Identità`/`Affinità sistemica`/`Colore` su tutte e quattro le fazioni.

> ⚠️ **Va fatto a mano, pagina per pagina.** Un tentativo di fusione allineata ha prodotto un **falso
> negativo**: i blocchi `replace` di `difflib` scartano il lato sorgente in silenzio, e la preview dichiarava
> 3 pagine da arricchire invece di 16. La tabella dei pivot di `facing-e-direzionalita` sarebbe sparita senza
> che nulla lo segnalasse.

## 3. Cosa va rifatto nel tooling, dopo

| Cosa | Perché |
|---|---|
| **69 `wiki_refs`** nel registry | puntano a `docs/wiki/...md`; vanno alla forma `wiki:<PageName>` |
| `apply_wiki_deploy` | **salta** i ref `wiki:`. Se non cambia, i blocchi di stato smettono di aggiornarsi per **tutte** le feature |
| `deploy_name` | deve risolvere la forma `wiki:` |
| comando `feature_registry.py wiki` | oggi rigenera i blocchi **dentro `docs/wiki/`**: senza quella cartella non ha più un bersaglio |
| `docs/wiki/feature-status.md` | è generato e deploya su `Stato-delle-feature.md`: resta solo la seconda |
| [`feature-registry.md`](../feature-registry.md) | owner dello schema: la riga su `wiki_refs` va riscritta |
| `check-docs-links.py` | oggi valida i link relativi dentro `docs/wiki/`: perde un'area |

## 4. Ordine, e perché conta

```
1. portare le 270 righe nel clone      <- nessuna perdita
2. pubblicare e verificare             <- la Wiki e' completa
3. rifare il tooling (§3)              <- il deploy continua a funzionare
4. cancellare docs/wiki/               <- ora e' sicuro
```

Invertire 1 e 4 perde contenuto in modo **non recuperabile dal repository**: dopo la cancellazione la sola
copia sarebbe nella storia di git — leggibile, ma nessuno andrebbe a cercarla.

## 5. Cosa NON è toccato

`docs/characters/` **resta**. Anche lui deploya verso il clone (`Personaggi/*`) e anche lui diverge — **256
righe su 10 pagine** — ma è il catalogo degli eroi, referenziato da cataloghi, ADR e test: non è una sorgente
Wiki e non sparisce. La sua divergenza col clone resta un problema aperto, e **questa migrazione non lo
risolve**.
