# -*- coding: utf-8 -*-
"""Nei documenti STORICI uno spostamento aggiorna i link, non la storia.

Un documento storico racconta il mondo del giorno in cui e' stato scritto: un percorso nella sua prosa, o nel codice
inline di un comando che ha prodotto un output, e' un fatto di quel giorno, e riscriverlo lo falsifica
(`docs/archive/README.md`, «La regola che vale qui»). Un **link** Markdown invece e' navigazione: deve portare dove
il file e' oggi, e `doc-links --with-archive` lo pretende.

Questo modulo e' la regola, condivisa da `sposta-cartelle.py`:

* `STORICI` dice quali file sono storici;
* `solo_link(testo, file, mappa)` riscrive SOLTANTO i link Markdown che puntano a un file spostato, bersaglio ed
  etichetta, e lascia intatto tutto il resto.

Trovato dalla review della PR #4062: la prima riscrittura aveva aggiornato 142 percorsi in 28 file di
`docs/archive/`, e voci datate del changelog e delle evidenze, che citavano cartelle non ancora esistenti.
"""

import os
import re

# I file la cui prosa e' storia: l'archivio, le evidenze datate, il changelog della documentazione, e il Decision
# Log. ⚠️ Il Decision Log e' anche il canone vivo, ma le sue voci sono DATATE e in questo repository si correggono con
# un'annotazione (`⌫`) o con una decisione nuova, non riscrivendole: una voce riporta spesso l'output di un comando a
# uno SHA (`git diff --name-only 7450ead64 947a783d5`), e quel percorso e' un fatto di quell'albero.
STORICI = ("docs/archive/", "docs/technical/evidence/", "docs/CHANGELOG_DOCUMENTATION.md",
           "docs/decisions/RT_PDR_00_Decision_Log.md")

LINK = re.compile(r"\[([^\]]*)\]\(([^)\s]+)\)")


# E i documenti DATATI di due cartelle: i referti di sessione di `docs/roadmap/plans/` (nel nome la data, in testa
# «HEAD misurato»: riportano trascrizioni di comandi su quell'albero) e i piani d'esecuzione di
# `docs/superpowers/plans/` («Create: …» di un giorno preciso). ⚠️ NON le spec di `docs/superpowers/specs/`: sono i
# contratti vivi delle feature, e un loro percorso deve portare al file di oggi.
CARTELLE_DATATE = ("docs/roadmap/plans/", "docs/superpowers/plans/")
DATA = re.compile(r"20\d\d-\d\d-\d\d")


def e_storico(percorso):
    if percorso.startswith(STORICI):
        return True
    return percorso.startswith(CARTELLE_DATATE) and bool(DATA.search(percorso.rsplit("/", 1)[-1]))


def solo_link(testo, file_rel, mappa, riscrivi_etichetta):
    """Riscrive i link Markdown di `testo` (scritto in `file_rel`) che puntano a un file spostato.

    `riscrivi_etichetta(s)` applica alle etichette la stessa riscrittura dei percorsi citati, cosi' un'etichetta che
    MOSTRA il percorso resta uguale al bersaglio (l'altro difetto che `doc-links` cerca).
    """
    base = os.path.dirname(file_rel)
    n = 0

    def sostituisci(m):
        nonlocal n
        etichetta, bersaglio = m.group(1), m.group(2)
        percorso, _, ancora = bersaglio.partition("#")
        if not percorso or re.match(r"^(https?:|mailto:)", percorso):
            return m.group(0)
        risolto = os.path.normpath(os.path.join(base, percorso)).replace("\\", "/")
        if risolto not in mappa:
            return m.group(0)
        nuovo = os.path.relpath(mappa[risolto], base or ".").replace("\\", "/")
        n += 1
        return "[%s](%s%s)" % (riscrivi_etichetta(etichetta), nuovo, ("#" + ancora) if ancora else "")

    return LINK.sub(sostituisci, testo), n
