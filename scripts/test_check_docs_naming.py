"""Prove che il gate dei nomi ritirati **puo' fallire** — non che oggi e' verde.

`#1109` ha trovato il gate cieco sui `.yaml`: `--check` usciva `0` su cinque file mai
aperti, e quello zero si leggeva come «pulito». La lezione non e' «apri anche gli
yaml», e' che **un gate provato solo sul caso pulito non distingue «non ha trovato
nulla» da «non ha guardato»**.

Da qui la forma di ogni test qui dentro: si introduce un difetto, si pretende il
**rosso**, si toglie, si pretende il **verde**. Un test che verificasse solo il verde
sarebbe soddisfatto da uno script che non legge niente — che e' precisamente il difetto
che stiamo chiudendo.

Si esegue con `python -m pytest scripts/test_check_docs_naming.py`, oppure
`python scripts/test_check_docs_naming.py` per un run senza pytest.
"""

from __future__ import annotations

import importlib.util
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
GATE = REPO / "scripts" / "check-docs-naming.py"


def _load_gate():
    """Il gate ha un trattino nel nome: `import` non basta, serve lo spec loader."""
    spec = importlib.util.spec_from_file_location("check_docs_naming", GATE)
    module = importlib.util.module_from_spec(spec)
    sys.modules["check_docs_naming"] = module
    spec.loader.exec_module(module)
    return module


gate = _load_gate()


def run_check(root: Path) -> int:
    """Esegue `--check` sull'albero `root` e ritorna l'exit code.

    ⚠️ Si legge l'**exit code**, non la coda dell'output: un gate puo' stampare righe
    rassicuranti e uscire `1`, ed e' successo davvero durante `#1109`.

    🔑 **Si esegue la COPIA dentro `root`, non l'originale.** Il gate calcola il proprio
    perimetro da `Path(__file__).resolve().parents[1]`, quindi lanciare l'originale con
    un `cwd` diverso lo lascerebbe puntato al repository vero: ogni test passerebbe o
    fallirebbe in base allo stato di `docs/`, non a quello dell'albero di prova. La
    prima stesura di questo file lo faceva, e i quattro test di mutazione sono usciti
    verdi contro un albero che non stavano guardando.
    """
    return _run(root)[0]


def _run(root: Path) -> tuple[int, str]:
    """`(exit_code, stdout)` — serve a chi deve provare *anche* cosa e' stato letto."""
    copia = root / "scripts" / "check-docs-naming.py"
    proc = subprocess.run(
        [sys.executable, str(copia), "--check"],
        cwd=str(root),
        capture_output=True, text=True, errors="replace",
    )
    return proc.returncode, proc.stdout


def run_check_su_yaml_letto(root: Path) -> int:
    """Come `run_check`, ma **fallisce se il gate non ha aperto nessun `.yaml`**.

    🔑 Serve ai test NEGATIVI — quelli che pretendono il **verde**. Un verde e' un
    risultato ambiguo: puo' voler dire «ha guardato e non ha trovato» oppure «non ha
    guardato». Misurato: togliendo `*.yaml` dal perimetro, `test_backtick_...` e
    `test_marcatore_yaml_esenta_...` restavano **verdi** — passavano contro un file che
    il gate non stava leggendo. E' la forma vacua che [[test-di-invarianza]] descrive:
    «X non produce Y» e' soddisfatto da una X che non viene nemmeno eseguita.
    """
    code, out = _run(root)
    assert ".yaml" in out, (
        "il gate non dichiara di aver letto .yaml: questo verde non prova nulla"
    )
    return code


# --- l'albero di prova -------------------------------------------------------
#
# Non si tocca il repository vero: si costruisce un albero minimo con la stessa forma
# (`docs/`, un file di governance in root) e ci si esegue il gate. Cosi' il test non
# dipende da quanti documenti esistono oggi, e non diventa rosso quando qualcun altro
# scrive prosa legittima.


def make_tree(root: Path, files: dict[str, str]) -> None:
    for rel, body in files.items():
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(body, encoding="utf-8")


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
    make_tree(root, files)
    shutil.copy(GATE, root / "scripts" / "check-docs-naming.py") if (root / "scripts").exists() else None
    (root / "scripts").mkdir(exist_ok=True)
    shutil.copy(GATE, root / "scripts" / "check-docs-naming.py")
    return root


# --- i test ------------------------------------------------------------------


def test_albero_pulito_esce_zero():
    """Il caso base. Da solo non prova nulla: serve il suo opposto, sotto."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp))
        assert run_check(root) == 0, "un albero senza nomi ritirati deve uscire 0"


def test_nome_legacy_in_markdown_fa_fallire():
    """Il verso che conta per i `.md`."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {"docs/technical/nota.md": "Riva manipola l'acqua.\n"})
        assert run_check(root) == 1, "un nome ritirato in prosa markdown deve uscire 1"


def test_nome_legacy_in_yaml_fa_fallire():
    """🔑 **Il test che `#1109` esiste per avere.**

    Prima di questa issue il gate non apriva i `.yaml`: questo caso usciva **0**, e lo
    zero era indistinguibile da quello del test qui sopra.
    """
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                "tracks:\n  demo:\n    note: >-\n      Riva non ha reazioni mappate.\n",
        })
        assert run_check(root) == 1, "un nome ritirato in prosa YAML deve uscire 1"


def test_nome_legacy_in_commento_yaml_fa_fallire():
    """I commenti YAML sono prosa quanto i valori: `editor-sessions.yaml` e' fatto cosi'."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml": "# il checkpoint 6.3 e' «Riva» in E6\ntracks: {}\n",
        })
        assert run_check(root) == 1, "un nome ritirato in un commento YAML deve uscire 1"


def test_backtick_non_fa_fallire_in_yaml():
    """La maschera markdown vale anche negli YAML — ed e' la ragione per cui il
    perimetro si e' potuto estendere senza scrivere una maschera nuova."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                "tracks:\n  demo:\n    note: >-\n      Il token `Hero.Riva` e' un identificatore.\n",
        })
        assert run_check_su_yaml_letto(root) == 0, (
            "un identificatore in backtick non e' prosa player-facing"
        )


def test_marcatore_yaml_esenta_la_riga():
    """La forma `# rename-exempt:` — un commento HTML in un YAML sarebbe testo inerte."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                "# rename-exempt: la riga dichiara la mappatura\n"
                "# D-130 rinomina Riva in Phase\n"
                "tracks: {}\n",
        })
        assert run_check_su_yaml_letto(root) == 0, (
            "il marcatore YAML deve esentare la riga successiva"
        )


def test_marcatore_yaml_stantio_fa_fallire():
    """L'altra meta': un'esenzione che sopravvive al proprio motivo e' una zona cieca."""
    with tempfile.TemporaryDirectory() as tmp:
        root = _tree(Path(tmp), {
            "docs/roadmap/batch.yaml":
                "# rename-exempt: motivo che non vale piu'\n"
                "# questa riga non nomina nessun eroe ritirato\n"
                "tracks: {}\n",
        })
        assert run_check(root) == 1, "un marcatore su una riga senza nomi ritirati deve uscire 1"


def test_perimetro_dichiara_le_estensioni():
    """AC 3: senza il conteggio per estensione, «zero occorrenze» e «zero file letti»
    restano indistinguibili — che e' come il difetto e' passato inosservato."""
    proc = subprocess.run(
        [sys.executable, str(GATE), "--check"],
        cwd=str(REPO), capture_output=True, text=True, errors="replace",
    )
    assert ".yaml" in proc.stdout, "il perimetro deve dichiarare quanti .yaml ha letto"
    assert ".md" in proc.stdout, "il perimetro deve dichiarare quanti .md ha letto"


def test_i_cinque_yaml_del_repository_sono_nel_perimetro():
    """Pinna il fatto misurato in `#1109`: cinque `.yaml` sotto `docs/`, zero letti prima."""
    files = gate.docs_files()
    yaml_files = [f for f in files if f.suffix == ".yaml"]
    assert len(yaml_files) >= 5, (
        f"attesi almeno 5 .yaml nel perimetro, trovati {len(yaml_files)}"
    )


if __name__ == "__main__":
    fallimenti = 0
    for nome, fn in sorted(globals().items()):
        if not nome.startswith("test_") or not callable(fn):
            continue
        try:
            fn()
            print(f"  ok   {nome}")
        except AssertionError as exc:
            fallimenti += 1
            print(f"  FAIL {nome}: {exc}")
    print()
    print(f"{fallimenti} fallimenti" if fallimenti else "tutti i test passano")
    raise SystemExit(1 if fallimenti else 0)
