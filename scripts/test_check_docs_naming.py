"""Prove che il gate dei nomi ritirati **puo' fallire** — non che oggi e' verde.

`#1109` ha trovato il gate cieco sui `.yaml`: `--check` usciva `0` su cinque file mai
aperti, e quello zero si leggeva come «pulito». La lezione non e' «apri anche gli yaml»,
e' che **un gate provato solo sul caso pulito non distingue «non ha trovato nulla» da
«non ha guardato»**.

Da qui la forma di ogni test: si introduce un difetto, si pretende il **rosso**, si
toglie, si pretende il **verde**. E il rosso si pretende **guardando anche cosa ha
detto**, non solo l'exit code — vedi `attende_rosso` sotto.

Si esegue con `python -m pytest scripts/test_check_docs_naming.py`, oppure
`python scripts/test_check_docs_naming.py` per un run senza pytest.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
GATE = REPO / "scripts" / "check-docs-naming.py"


def _run(root: Path) -> tuple[int, str]:
    """`(exit_code, stdout)` del gate eseguito sull'albero `root`.

    🔑 **Si esegue la COPIA dentro `root`, non l'originale.** Il gate calcola il proprio
    perimetro da `Path(__file__).resolve().parents[1]`, quindi lanciare l'originale con un
    `cwd` diverso lo lascerebbe puntato al repository vero: ogni test passerebbe o
    fallirebbe in base allo stato di `docs/`, non a quello dell'albero di prova. La prima
    stesura di questo file lo faceva, e quattro test uscivano verdi contro un albero che
    non stavano guardando.
    """
    copia = root / "scripts" / "check-docs-naming.py"
    proc = subprocess.run(
        [sys.executable, str(copia), "--check"],
        cwd=str(root), capture_output=True, text=True, errors="replace",
    )
    return proc.returncode, proc.stdout


def attende_rosso(root: Path, nome: str) -> None:
    """Il gate deve uscire `1` **e** aver segnalato `nome` come prosa legacy.

    🔴 **L'exit code da solo non basta, ed e' un difetto misurato.** Python esce `1` anche
    su un'eccezione non gestita: iniettando `raise RuntimeError` in cima a `main()`, tutti
    gli otto test «pretendi il rosso» di una stesura precedente riportavano **ok** contro
    un gate che non aveva letto un solo file. Un test che accetta un crash come successo
    e' la stessa forma vacua che questo file esiste per evitare — solo piu' difficile da
    vedere, perche' il numero atteso e' quello giusto.
    """
    code, out = _run(root)
    assert code == 1, f"atteso rosso, ottenuto exit {code}. stdout:\n{out}"
    assert "ERRORE" in out, f"il gate e' uscito 1 senza il blocco d'errore:\n{out}"
    assert nome in out, f"il gate non ha segnalato «{nome}»:\n{out}"


def attende_verde(root: Path) -> None:
    """Il gate deve uscire `0` **e** aver dichiarato di aver letto degli `.yaml`.

    🔑 Serve ai test negativi. Un verde e' ambiguo: puo' voler dire «ha guardato e non ha
    trovato» oppure «non ha guardato». Misurato: togliendo `*.yaml` dal perimetro, i due
    test negativi restavano **verdi** — passavano contro un file che il gate non stava
    leggendo.

    ⚠️ Si guarda la **riga** `per estensione:`, non `".yaml" in stdout`: quella forma e'
    soddisfatta anche da un gate che stampa un percorso `.yaml` dentro un errore, o che
    echeggia l'elenco dei pattern dal ramo «perimetro vuoto» — cioe' da un output che dice
    il contrario.
    """
    code, out = _run(root)
    riga = next((l for l in out.splitlines() if "per estensione:" in l), None)
    assert riga is not None, f"il gate non dichiara il perimetro per estensione:\n{out}"
    assert ".yaml" in riga, f"nessun .yaml nel perimetro: {riga!r} — questo verde non prova nulla"
    assert code == 0, f"atteso verde, ottenuto exit {code}. stdout:\n{out}"


# --- l'albero di prova -------------------------------------------------------
#
# Non si tocca il repository vero: si costruisce un albero minimo con la stessa forma
# (`docs/`, un file di governance in root) e ci si esegue il gate. Cosi' i test non
# dipendono da quanti documenti esistono oggi, e non diventano rossi quando qualcun altro
# scrive prosa legittima.

PULITO = {
    "CLAUDE.md": "# Governance\n\nIl roster e' Gadget, Phase, Riktor, Wraith.\n",
    "docs/technical/nota.md": "Phase manipola l'acqua.\n",
    "docs/roadmap/batch.yaml": "tracks:\n  demo:\n    note: >-\n      Wraith intercetta.\n",
}


def _tree(tmp: Path, extra: dict[str, str] | None = None) -> Path:
    root = tmp / "repo"
    root.mkdir()
    files = dict(PULITO)
    if extra:
        files.update(extra)
    for rel, body in files.items():
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(body, encoding="utf-8")
    (root / "scripts").mkdir(exist_ok=True)
    shutil.copy(GATE, root / "scripts" / "check-docs-naming.py")
    return root


# --- il perimetro ------------------------------------------------------------


def test_albero_pulito_esce_zero():
    """Il caso base. Da solo non prova nulla: serve il suo opposto, sotto."""
    with tempfile.TemporaryDirectory() as tmp:
        assert _run(_tree(Path(tmp)))[0] == 0


def test_nome_legacy_in_markdown_fa_fallire():
    """Il verso che conta per i `.md`."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {"docs/technical/nota.md": "Riva manipola l'acqua.\n"})
        attende_rosso(root, "Riva")


def test_nome_legacy_in_yaml_fa_fallire():
    """🔑 **Il test che `#1109` esiste per avere.**

    Prima di quella issue il gate non apriva i `.yaml`: questo caso usciva **0**, e lo
    zero era indistinguibile da quello del test qui sopra.
    """
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                "tracks:\n  demo:\n    note: >-\n      Riva non ha reazioni mappate.\n",
        })
        attende_rosso(root, "Riva")


def test_nome_legacy_in_commento_yaml_fa_fallire():
    """I commenti YAML sono prosa quanto i valori: `editor-sessions.yaml` e' fatto cosi'."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml": "# il checkpoint 6.3 e' «Riva» in E6\ntracks: {}\n",
        })
        attende_rosso(root, "Riva")


def test_backtick_non_fa_fallire_in_yaml():
    """La maschera markdown vale anche negli YAML — ed e' la ragione per cui il perimetro
    si e' potuto estendere senza scrivere una maschera nuova."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                "tracks:\n  demo:\n    note: >-\n      Il token `Hero.Riva` e' un identificatore.\n",
        })
        attende_verde(root)


def test_perimetro_dichiara_le_estensioni():
    """AC 3: senza il conteggio per estensione, «zero occorrenze» e «zero file letti»
    restano indistinguibili — che e' come il difetto e' passato inosservato."""
    with tempfile.TemporaryDirectory() as tmp:
        code, out = _run(_tree(Path(tmp)))
        riga = next((l for l in out.splitlines() if "per estensione:" in l), None)
        assert riga is not None, f"manca la riga del perimetro:\n{out}"
        assert ".md" in riga and ".yaml" in riga, f"perimetro incompleto: {riga!r}"
        assert code == 0


def test_ogni_estensione_protetta_entra_nel_perimetro():
    """La proprieta' e' «il perimetro raccoglie cio' che dichiara», non «ci sono N file».

    ⚠️ Una stesura precedente contava i `.yaml` del **repository vero** e pretendeva
    `>= 5`: sarebbe diventata rossa se qualcuno avesse spostato `docs/roadmap/`, per una
    ragione che non ha nulla a che vedere col comportamento del gate.
    """
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/a.yaml": "tracks: {}\n",
            "docs/roadmap/b.yml": "tracks: {}\n",
            "docs/technical/c.md": "Phase.\n",
        })
        code, out = _run(root)
        riga = next((l for l in out.splitlines() if "per estensione:" in l), None)
        assert riga is not None, f"manca la riga del perimetro:\n{out}"
        for atteso in (".md", ".yaml", ".yml"):
            assert atteso in riga, f"{atteso} non e' nel perimetro: {riga!r}"
        assert code == 0


# --- il marcatore: UNA forma sola, e il perche' -------------------------------


def test_marcatore_html_esenta_la_riga_in_markdown():
    """82 marcatori reali vivono in questa forma."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/technical/nota.md":
                "<!-- rename-exempt: la riga dichiara la mappatura -->\n"
                "D-130 rinomina Riva in Phase.\n",
        })
        assert _run(root)[0] == 0


def test_marcatore_html_esenta_dentro_una_nota_yaml():
    """Le note lunghe dei YAML sotto `docs/` sono markdown: chi ci scrive usa `<!-- -->`."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                "tracks:\n  demo:\n    note: >-\n"
                "      <!-- rename-exempt: la riga dichiara la mappatura -->\n"
                "      D-130 rinomina Riva in Phase.\n",
        })
        attende_verde(root)


def test_marcatore_html_esenta_dentro_un_commento_yaml():
    """🔑 **Il caso che rende superflua una seconda sintassi.**

    Dentro un commento YAML, `# <!-- rename-exempt: ... -->` e' semplicemente testo del
    commento: il marcatore funziona senza che il gate debba sapere cosa sia un commento
    YAML. E' la ragione per cui la forma `#` e' stata ritirata invece che corretta.
    """
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                "# <!-- rename-exempt: la riga dichiara la mappatura -->\n"
                "# D-130 rinomina Riva in Phase\n"
                "tracks: {}\n",
        })
        attende_verde(root)


def test_marcatore_html_esenta_la_riga_stessa_in_fondo():
    """La forma in-tabella: obbligatoria dentro una tabella markdown, utile negli YAML."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                'tracks:\n  demo:\n'
                '    note: "Riva e Phase"  <!-- rename-exempt: dichiara la mappatura -->\n',
        })
        attende_verde(root)


def test_la_ragione_del_marcatore_non_e_prosa():
    """La motivazione piu' naturale nomina la mappatura, e non deve far fallire il gate.

    Non serve codice: il marcatore **e'** un commento HTML, e `mask()` li azzera da sempre.
    """
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/technical/nota.md":
                "<!-- rename-exempt: D-130 dichiara la mappatura Flux -> Gadget -->\n"
                "La riga nomina Flux come personaggio.\n",
        })
        assert _run(root)[0] == 0


def test_marcatore_stantio_fa_fallire():
    """L'altra meta': un'esenzione che sopravvive al proprio motivo e' una zona cieca."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/technical/nota.md":
                "<!-- rename-exempt: motivo che non vale piu' -->\n"
                "Questa riga non nomina nessun eroe ritirato.\n",
        })
        code, out = _run(root)
        assert code == 1, f"un marcatore su una riga senza nomi ritirati deve uscire 1:\n{out}"
        assert "stantio" in out.lower() or "marcator" in out.lower(), out


# --- i tre falsi negativi che hanno fatto ritirare la forma `#` ---------------
#
# Sono i casi che una seconda sintassi `# rename-exempt:` apriva, tutti raggiungibili da
# testo scritto in buona fede. Restano qui come **regressione**: se qualcuno reintroduce
# quella forma, questi tre diventano rossi.


def test_cancelletto_con_ragione_di_un_solo_spazio_non_esenta():
    """Il piu' insidioso: un carattere invisibile ribaltava il gate da rosso a verde."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml": "# rename-exempt: \n# Riva gioca\ntracks: {}\n",
        })
        attende_rosso(root, "Riva")


def test_cancelletto_dentro_un_block_scalar_non_esenta():
    """Un titolo markdown dentro `note: |` e' **contenuto**, non un commento.

    Ed e' precisamente il pericolo che vietava la forma `#` nei `.md`: le note YAML *sono*
    markdown, quindi il divieto doveva valere anche li'.
    """
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                "tracks:\n  demo:\n    note: |\n"
                "      # rename-exempt: la sezione spiega il marcatore\n"
                "      Riva gioca nel team del bot.\n",
        })
        attende_rosso(root, "Riva")


def test_cancelletto_dentro_una_stringa_non_esenta():
    """Un `#` dentro uno scalare quotato e' contenuto: il valore che *documenta* il
    marcatore non deve diventarne uno."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                'tracks:\n  demo:\n'
                '    note: "per esentare scrivi # rename-exempt: motivo"\n'
                '    altro: "Riva gioca nel team"\n',
        })
        attende_rosso(root, "Riva")


def test_cancelletto_nei_markdown_non_esenta():
    """`#` a inizio riga in un `.md` e' un **titolo**: non puo' esentare la riga dopo."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            # ⚠️ Nessuna riga vuota fra i due: il marcatore esenta la riga IMMEDIATAMENTE
            # successiva, quindi con un vuoto in mezzo l'esenzione cadrebbe li' e il test
            # passerebbe anche con la porta aperta. Misurato.
            "docs/technical/nota.md":
                "# rename-exempt: sembra un marcatore ma e' un titolo\n"
                "Riva manipola l'acqua.\n",
        })
        attende_rosso(root, "Riva")


if __name__ == "__main__":
    fallimenti = 0
    for nome, fn in sorted(globals().items()):
        if not nome.startswith("test_") or not callable(fn):
            continue
        try:
            fn()
            print(f"  ok   {nome}")
        except Exception as exc:  # noqa: BLE001 — un errore inatteso e' un fallimento
            fallimenti += 1
            prima = str(exc).splitlines()[0] if str(exc) else type(exc).__name__
            print(f"  FAIL {nome}: {prima}")
    print()
    print(f"{fallimenti} fallimenti" if fallimenti else "tutti i test passano")
    raise SystemExit(1 if fallimenti else 0)
