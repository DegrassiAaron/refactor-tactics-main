# tools/editor-sessions/seed_wiring.py
"""Semina la bozza del cablaggio dal vecchio registro delle sedute.

Meccanica e fallibile per costruzione: deduce l'allestimento dagli indizi che
le sedute lasciano nella prosa. Cio' che NON sa dedurre lo dichiara fra gli
orfani, e non lo scrive: `registry.load` rifiuta un setup sconosciuto, quindi
una riga seminata male non puo' passare inosservata.

    python3 tools/editor-sessions/seed_wiring.py --out build/wiring-seminato.yaml
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import yaml

VECCHIO = Path("docs/roadmap/editor-sessions.yaml")

# Dal piu' specifico al piu' generico: la prima corrispondenza vince.
MAPPA_SETUP: tuple[tuple[str, str], ...] = (
    ("rt.Match.Autobattle", "SET-HEX-BOT"),
    ("L_GrayKitPlayground", "SET-GRAYKIT"),
    ("L_Frontend", "SET-FRONTEND"),
    ("rt.Test.Scenario", "SET-SCEN"),
    # L'arena GENERATA a runtime non e' `L_HexArena` e non e' uno scenario: porta con se'
    # esagono r=4, ostacoli, muro che blocca la vista, fango a costo 3, piattaforma sul
    # layer 1 e una transizione. U37 dichiara di condividerla con U2..U6.
    ("GeneratedTestArena", "SET-GEN-ARENA"),
    ("L_DevSandbox", "SET-SANDBOX"),
    ("L_HexArena", "SET-HEX-MATCH"),
)

CAMPI_PROSA = ("title", "produces", "steps", "notes", "done_when")


class InnestoError(Exception):
    """L'innesto perderebbe qualcosa. Si rifiuta, non si sovrascrive."""


INTESTAZIONE_WIRING = """wiring:
  # Prodotto da `python tools/editor-sessions/seed_wiring.py --into docs/roadmap/sedute-mattoni.yaml`.
  # Il seme e' l'euristica sulla prosa PIU' due tabelle di giudizio dichiarato che vivono nel
  # sorgente del seminatore, non qui:
  #   ALLESTIMENTO_DICHIARATO  le sedute la cui prosa non dice dove si guarda
  #   CABLAGGIO_DICHIARATO     i check che due sedute rivendicano con allestimenti DIVERSI
  # ⚠️ Correggere `check`, `setup` o `issue` QUI e' inutile: la decisione sta nelle tabelle, e il
  #    prossimo seme ricalcola.
  # 🔑 I `requires` invece si scrivono QUI, e il seme li PRESERVA: sono giudizio d'autore su un
  #    prerequisito, non una deduzione dalla prosa. Ognuno ha la propria riga di prova nel verbale
  #    `plans/sedute-componibili-rassegna-requires-2026-09-12.md`, e `compare_rassegna.py` lo verifica.
"""


# Le sedute la cui prosa NON dichiara l'allestimento, e a cui e' stato assegnato per
# giudizio il 2026-09-11. Non e' una deduzione: e' una decisione, e sta qui perche' la
# semina resti riproducibile invece di diventare 81 righe scritte a mano una volta sola.
# Due principi, entrambi decisi esplicitamente:
#   - una seduta che condivide l'allestimento di U2..U6 va su `SET-GEN-ARENA`;
#   - una seduta che dice «una partita» senza nominare la mappa cade su `SET-HEX-MATCH`.
ALLESTIMENTO_DICHIARATO: dict[str, str] = {
    # coda del blocco 2: stesso `MapSource = GeneratedTestArena` di U2, che la prosa
    # di queste tre non ripete
    "U3": "SET-GEN-ARENA",
    "U4": "SET-GEN-ARENA",
    "U5": "SET-GEN-ARENA",
    # corsia asset: si costruisce nel sandbox
    "U7": "SET-SANDBOX",
    "U9": "SET-SANDBOX",
    "U13": "SET-SANDBOX",
    # il frontend: producono i widget di `/Game/RT/UI/Framework/`
    "U28": "SET-FRONTEND",
    "U29": "SET-FRONTEND",
    "U30": "SET-FRONTEND",
    # «una partita» senza mappa nominata
    "U14": "SET-HEX-MATCH",
    "U15": "SET-HEX-MATCH",
    "U16": "SET-HEX-MATCH",
    "U19": "SET-HEX-MATCH",
    "U35": "SET-HEX-MATCH",
    "U51": "SET-HEX-MATCH",
    # serve un turno gia' pianificato: la timeline ha senso solo durante il planning
    "U52": "SET-HEX-TURN",
    # lo dicono da se': «ognuna ha gia' il proprio scenario in Scenarios/Visual/»
    "U42": "SET-SCEN",
    "U43": "SET-SCEN",
    "U44": "SET-SCEN",
    "U48": "SET-SCEN",
    "U53": "SET-SCEN",
}

# I check che DUE sedute rivendicano con allestimenti DIVERSI. La semina non li decide per
# ordine di documento: li elenca come conflitti, e la decisione sta qui con la sua ragione.
# Tre delle quattro le decide il registro PIE, che e' l'owner e ha gia' risposto:
#   «C — mappa con una geometria precisa | PIE-HEXPLAY-6 · -8 | non serve allestire a mano:
#    gli scenari esistono e portano con se' arena, unita' e piani»
#   «B — un turno pianificato e risolto  | PIE-HEXPLAY-4 · PIE-V01-LOG | piani impostati e
#    lock-in con Spazio; per il log serve un turno con un fallback e una modifica ambientale»
CABLAGGIO_DICHIARATO: dict[str, str] = {
    "PIE-HEXPLAY-6": "SET-SCEN",
    "PIE-HEXPLAY-8": "SET-SCEN",
    "PIE-V01-LOG": "SET-HEX-TURN",
    # U31 giudica che il pannello non sporchi il livello, e lo fa nel playground;
    # U32 giudica le tendine, che e' un'altra domanda.
    "PIE-TD-CLEAN": "SET-GRAYKIT",
}


def deduci_setup(blob: str) -> str | None:
    for indizio, setup in MAPPA_SETUP:
        if indizio in blob:
            return setup
    return None


def semina(
    sessioni: list[dict],
) -> tuple[list[dict], dict[str, list[str]], dict[str, dict[str, str]]]:
    """Le righe seminate, gli orfani PER SEDUTA, e i conflitti PER CHECK.

    Il raggruppamento degli orfani non e' cosmetico: una seduta ERA un
    allestimento per costruzione, quindi chi assegna decide una volta per seduta
    invece che una volta per check.

    I conflitti sono la parte che non si puo' sbrigare: quando due sedute
    rivendicano lo stesso check con allestimenti DIVERSI, la risposta non e'
    «vince la prima». Quella regola decide in silenzio in base all'ordine del
    documento — ed e' precisamente la classe di arbitrarieta' che questo lavoro
    esiste per rimuovere. La riga non si scrive finche' qualcuno non decide, in
    `CABLAGGIO_DICHIARATO`.
    """
    rivendica: dict[str, dict[str, str]] = {}
    issue_di: dict[str, int] = {}
    muti: dict[str, list[str]] = {}

    for s in sessioni:
        blob = " ".join(str(s.get(k) or "") for k in CAMPI_PROSA)
        # Il giudizio dichiarato vince sull'euristica: dove qualcuno ha deciso, la prosa
        # non ha voce. U4 nomina `L_HexArena` di sfuggita e appartiene all'arena generata.
        setup = ALLESTIMENTO_DICHIARATO.get(s["id"]) or deduci_setup(blob)
        issue = (s.get("issues") or [None])[0]
        for check in s.get("verifies") or []:
            if setup is None:
                muti.setdefault(s["id"], []).append(check)
                continue
            rivendica.setdefault(check, {})[s["id"]] = setup
            if issue is not None:
                issue_di.setdefault(check, issue)

    righe: dict[str, dict] = {}
    conflitti: dict[str, dict[str, str]] = {}
    for check, per_seduta in rivendica.items():
        deciso = CABLAGGIO_DICHIARATO.get(check)
        if deciso is None:
            scelte = set(per_seduta.values())
            if len(scelte) > 1:
                conflitti[check] = dict(per_seduta)
                continue
            deciso = scelte.pop()
        riga = {"check": check, "setup": deciso}
        if check in issue_di:
            riga["issue"] = issue_di[check]
        righe[check] = riga

    coperti = set(righe) | set(conflitti)
    orfani = {
        sid: sorted(set(cs) - coperti) for sid, cs in muti.items() if set(cs) - coperti
    }
    return [righe[c] for c in sorted(righe)], orfani, conflitti


def requires_esistenti(testo: str) -> dict[str, list[str]]:
    """I `requires` gia' scritti, per check.

    Legge il registro COME DATI: i commenti non contano, conta cio' che
    `registry.load` leggerebbe. Se un giudizio e' scritto in un commento, per
    questo strumento non esiste — ed e' corretto, perche' non esiste nemmeno
    per il calcolo.
    """
    raw = yaml.safe_load(testo) or {}
    return {
        r["check"]: list(r["requires"])
        for r in (raw.get("wiring") or [])
        if r.get("requires")
    }


def rendi_righe(righe: list[dict]) -> str:
    """Le righe nello stile del file: due spazi di rientro, `requires` in flusso.

    Non si usa `yaml.safe_dump`: produce sequenze non indentate, che non e' lo
    stile del registro, e trasformerebbe ogni riseminio in un diff totale
    invece che nel diff di cio' che e' cambiato davvero.
    """
    out: list[str] = []
    for r in righe:
        out.append(f"  - check: {r['check']}")
        out.append(f"    setup: {r['setup']}")
        if r.get("requires"):
            out.append("    requires: [ " + ", ".join(r["requires"]) + " ]")
        if r.get("issue") is not None:
            out.append(f"    issue: {r['issue']}")
    return "\n".join(out) + "\n"


def innesta(testo: str, righe: list[dict]) -> str:
    """Riscrive la SOLA sezione `wiring:`, conservando tutto cio' che sta sopra.

    Un round-trip yaml cancellerebbe i commenti — fra cui il marcatore
    `DRAFT — NON OWNER`, che e' l'unica cosa che tiene UN SOLO owner in ogni
    istante. Quindi la testa si conserva come STRINGA: questa funzione lavora su
    testo gia' normalizzato a \n, non sui byte del file. E' chi chiama — `main`,
    nel blocco `--into` — a leggere e riscrivere il file senza traduzione dei fine
    riga, cosi' che la convenzione (CRLF o LF) del registro sopravviva sul disco.

    I `requires` gia' presenti si riportano sulle righe nuove. Se una riga che
    ne portava uno non viene piu' prodotta dal seme, l'innesto si RIFIUTA: quel
    giudizio e' lavoro d'autore, e perderlo in silenzio e' esattamente cio' che
    questa funzione esiste per impedire.
    """
    i = testo.find("\nwiring:")
    if i < 0:
        raise InnestoError("il registro non contiene una sezione `wiring:`")

    vecchi = requires_esistenti(testo)
    prodotti = {r["check"] for r in righe}
    orfani = sorted(set(vecchi) - prodotti)
    if orfani:
        raise InnestoError(
            "questi check portano un `requires` e il seme non produce piu' la loro riga: "
            + " ".join(orfani)
            + ". Decidi prima di riseminare: o torna la riga, o il giudizio si sposta."
        )

    arricchite = []
    for r in righe:
        nuova = dict(r)
        if r["check"] in vecchi:
            nuova["requires"] = vecchi[r["check"]]
        arricchite.append(nuova)

    return testo[: i + 1] + INTESTAZIONE_WIRING + rendi_righe(arricchite)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sessioni", default=VECCHIO, type=Path, help="il vecchio registro")
    ap.add_argument("--out", default=Path("build/wiring-seminato.yaml"), type=Path)
    ap.add_argument(
        "--into",
        type=Path,
        help="innesta la sezione `wiring:` in questo registro, preservando i `requires` gia' scritti",
    )
    a = ap.parse_args()

    if not a.sessioni.exists():
        sys.exit(f"registro non trovato: {a.sessioni}")
    vecchio = yaml.safe_load(a.sessioni.read_text(encoding="utf-8")) or {}
    righe, orfani, conflitti = semina(vecchio.get("sessions") or [])

    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(
        yaml.safe_dump({"wiring": righe}, allow_unicode=True, sort_keys=False),
        encoding="utf-8",
    )
    print(f"righe seminate: {len(righe)} -> {a.out}")
    if orfani:
        quanti = sum(len(cs) for cs in orfani.values())
        print(f"SENZA ALLESTIMENTO: {quanti} check da {len(orfani)} sedute.")
        print("Decidi una volta per SEDUTA: era un allestimento per costruzione.")
        for sid in sorted(orfani):
            print(f"  {sid}: {' '.join(orfani[sid])}")
    if conflitti:
        print(f"CONFLITTI: {len(conflitti)} check rivendicati con allestimenti diversi.")
        print("Nessuna riga scritta per questi: decidi in CABLAGGIO_DICHIARATO, con la ragione.")
        for check in sorted(conflitti):
            dove = ", ".join(f"{k}={v}" for k, v in sorted(conflitti[check].items()))
            print(f"  {check}: {dove}")

    if a.into is not None:
        if not a.into.exists():
            sys.exit(f"--into: registro non trovato: {a.into}")
        # newline="" disattiva la traduzione in LETTURA: i \r\n arrivano come sono, e possiamo
        # vedere che convenzione usa il file invece di indovinarla da os.linesep — che dipende
        # dalla macchina, non dal registro. Su Linux o in WSL, senza questo, un solo --into
        # riscriverebbe TUTTO il file da CRLF a LF: il "diff totale" che rendi_righe evita.
        # `Path.read_text` non accetta `newline` prima di Python 3.13: si apre a mano.
        with a.into.open(encoding="utf-8", newline="") as f:
            prima = f.read()
        eol = "\r\n" if "\r\n" in prima else "\n"
        try:
            dopo = innesta(prima.replace("\r\n", "\n"), righe).replace("\n", eol)
        except InnestoError as e:
            sys.exit(f"innesto rifiutato: {e}")
        if dopo == prima:
            print(f"{a.into}: gia' allineato, nessuna scrittura")
        else:
            # newline="" disattiva la traduzione anche in SCRITTURA: esce esattamente `eol`.
            with a.into.open("w", encoding="utf-8", newline="") as f:
                f.write(dopo)
            print(f"innestato in {a.into}: rileggi con `git diff` prima di committare")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
