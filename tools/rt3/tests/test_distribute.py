"""La catena di distribuzione: worktree, cloni, fallback — e i suoi oracoli.

Gira su repository **temporanei** creati qui, non sui tre workspace della macchina. E'
deliberato: un test che dipendesse da `refactor-tactict-dev` proverebbe lo stato di quel
checkout in questo momento, cambierebbe esito domani, e non potrebbe mai costruire il
caso C - nessuno rompe un workspace vero per vedere se il fallback parte.

Gli oracoli sono quelli della specifica rivista dal panel:

    D-1  fra worktree dello stesso repo la propagazione avviene SENZA trasporto
    D-2  fra cloni distinti il trasporto resta SU DISCO (fetch da path locale)
    D-3  il caso si MISURA (git-common-dir), non si deduce dal nome
    D-4  «serve un merge» e' un PREDICATO, non un giudizio
    D-5  il fallback si attiva SOLO se A e B non sono applicabili
    D-6  la copia dichiara i file ed esclude la deny-list
"""

import os
import shutil
import subprocess
import tempfile
import unittest

import distribute
from distribute import (
    CONTROLLED_COPY,
    DISTINCT_CLONES,
    FETCH_LOCAL,
    NO_TRANSPORT,
    NOT_A_REPO,
    SAME_REPO,
    DistributionError,
)

GIT_ID = [
    "-c", "user.email=smoke@rt3.local",
    "-c", "user.name=RT3 Smoke",
    "-c", "commit.gpgsign=false",
]


def run(cwd, *args):
    proc = subprocess.run(
        ["git"] + GIT_ID + list(args),
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=120,
    )
    if proc.returncode != 0:
        raise AssertionError(
            "git {} in {} -> {}: {}".format(
                " ".join(args), cwd, proc.returncode, proc.stderr.decode("utf-8", "replace")
            )
        )
    return proc.stdout.decode("utf-8", "replace").strip()


def write(root, rel, text):
    path = os.path.join(root, rel.replace("/", os.sep))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(text)
    return path


class DistributeTestCase(unittest.TestCase):
    """Un repository sorgente con un commit da propagare."""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="rt3-distr-")
        self.source = os.path.join(self.tmp, "source")
        os.makedirs(self.source)
        run(self.source, "init", "-b", "main")
        write(self.source, "tools/rt3/rt3/model.py", "VERSION = 1\n")
        write(self.source, "README.md", "radice\n")
        run(self.source, "add", "-A")
        run(self.source, "commit", "-m", "base")
        self.base = run(self.source, "rev-parse", "HEAD")

        # ⚠️ Il clone si fa ORA, quando il commit da propagare non esiste ancora.
        # Clonarlo dopo lo riempirebbe di tutti gli oggetti - e `reachable()` sarebbe
        # vero prima di qualunque distribuzione, rendendo vacuo il test del predicato.
        # E' il difetto che il banco aveva alla prima stesura.
        self.clone_path = os.path.join(self.tmp, "clone")
        run(self.tmp, "clone", "--quiet", self.source, self.clone_path)
        run(self.clone_path, "checkout", "--quiet", self.base)

        write(self.source, "tools/rt3/rt3/model.py", "VERSION = 2\n")
        write(self.source, "tools/rt3/rt3/nuovo.py", "# aggiunto\n")
        run(self.source, "add", "-A")
        run(self.source, "commit", "-m", "la modifica da propagare")
        self.commit = run(self.source, "rev-parse", "HEAD")

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    # -- costruttori di bersagli -----------------------------------------

    def make_worktree(self):
        """Un worktree dello STESSO repository: caso A."""
        path = os.path.join(self.tmp, "worktree")
        run(self.source, "worktree", "add", "--detach", path, self.base)
        return path

    def make_clone(self):
        """Il clone distinto creato in `setUp`, che NON ha ancora il commit: caso B."""
        return self.clone_path

    def make_plain_dir(self):
        """Una directory che NON e' un repository: caso C."""
        path = os.path.join(self.tmp, "non-repo")
        os.makedirs(os.path.join(path, "tools", "rt3"))
        return path


class ClassifyTest(DistributeTestCase):
    """D-3: il caso si misura."""

    def test_worktree_dello_stesso_repo(self):
        self.assertEqual(distribute.classify(self.source, self.make_worktree()), SAME_REPO)

    def test_clone_distinto(self):
        self.assertEqual(distribute.classify(self.source, self.make_clone()), DISTINCT_CLONES)

    def test_directory_non_repository(self):
        self.assertEqual(distribute.classify(self.source, self.make_plain_dir()), NOT_A_REPO)

    def test_la_sorgente_con_se_stessa_e_lo_stesso_repo(self):
        self.assertEqual(distribute.classify(self.source, self.source), SAME_REPO)

    def test_il_confronto_normalizza_relativo_e_assoluto(self):
        """⚠️ Git ritorna `--git-common-dir` relativo dalla radice e assoluto da un
        worktree. Confrontarli come stringhe direbbe «cloni distinti» per due directory
        dello stesso repository - sbagliando la sola domanda che questa funzione pone.
        """
        worktree = self.make_worktree()
        grezzo_radice = distribute.git(self.source, "rev-parse", "--git-common-dir")[1]
        grezzo_worktree = distribute.git(worktree, "rev-parse", "--git-common-dir")[1]
        self.assertNotEqual(
            grezzo_radice,
            grezzo_worktree,
            "se git li restituisse gia' uguali, questo test non proverebbe nulla",
        )
        self.assertEqual(
            distribute.common_dir(self.source), distribute.common_dir(worktree)
        )

    def test_il_nome_della_directory_non_decide(self):
        """Una dir che CONTIENE un clone non e' un clone - il caso reale di
        `refactor-tactics-technical-designer`."""
        contenitore = os.path.join(self.tmp, "contenitore")
        os.makedirs(contenitore)
        run(self.tmp, "clone", "--quiet", self.source, os.path.join(contenitore, "dentro"))
        self.assertEqual(distribute.classify(self.source, contenitore), NOT_A_REPO)
        self.assertEqual(
            distribute.classify(self.source, os.path.join(contenitore, "dentro")),
            DISTINCT_CLONES,
        )

    def test_sorgente_non_repository_e_un_errore(self):
        with self.assertRaises(DistributionError) as ctx:
            distribute.classify(self.make_plain_dir(), self.source)
        self.assertEqual(ctx.exception.code, "SOURCE_NOT_A_REPO")


class ReachabilityTest(DistributeTestCase):
    """D-4: «serve un merge» e' un predicato."""

    def test_predicato_binario(self):
        clone = self.make_clone()
        self.assertTrue(distribute.reachable(self.source, self.commit))
        self.assertFalse(distribute.reachable(clone, self.commit))

    def test_dopo_il_fetch_diventa_vero(self):
        clone = self.make_clone()
        distribute.distribute(self.source, clone, self.commit, ["tools/rt3"])
        self.assertTrue(distribute.reachable(clone, self.commit))


class RamoA_WorktreeTest(DistributeTestCase):
    """D-1: fra worktree non c'e' niente da trasportare."""

    def test_nessun_trasporto(self):
        worktree = self.make_worktree()
        result = distribute.distribute(self.source, worktree, self.commit, ["tools/rt3"])
        self.assertEqual(result["case"], SAME_REPO)
        self.assertEqual(result["strategy"], NO_TRANSPORT)
        self.assertEqual(result["files"], [], "il ramo A non copia nulla")

    def test_il_commit_e_gia_raggiungibile_senza_fare_niente(self):
        """La proprieta' che rende inutile il giro su GitHub: gli oggetti sono condivisi."""
        worktree = self.make_worktree()
        self.assertTrue(
            distribute.reachable(worktree, self.commit),
            "un worktree vede gia' i commit del proprio repository",
        )
        result = distribute.distribute(self.source, worktree, self.commit, ["tools/rt3"])
        self.assertTrue(result["alreadyReachable"])

    def test_non_sposta_HEAD_del_worktree(self):
        """Integrare e' un gesto di chi possiede il ramo, non di chi distribuisce."""
        worktree = self.make_worktree()
        prima = run(worktree, "rev-parse", "HEAD")
        distribute.distribute(self.source, worktree, self.commit, ["tools/rt3"])
        self.assertEqual(run(worktree, "rev-parse", "HEAD"), prima)

    def test_il_file_nuovo_NON_compare_nel_worktree(self):
        """Controllo positivo al contrario: se il ramo A copiasse i file, il test
        «nessun trasporto» sarebbe verde e la semantica sbagliata. Il worktree resta
        al proprio commit finche' qualcuno non fa un merge locale."""
        worktree = self.make_worktree()
        distribute.distribute(self.source, worktree, self.commit, ["tools/rt3"])
        self.assertFalse(
            os.path.exists(os.path.join(worktree, "tools", "rt3", "rt3", "nuovo.py"))
        )


class RamoB_CloniTest(DistributeTestCase):
    """D-2: fra cloni il trasporto resta su disco."""

    def test_fetch_e_materializzazione(self):
        clone = self.make_clone()
        result = distribute.distribute(self.source, clone, self.commit, ["tools/rt3"])
        self.assertEqual(result["case"], DISTINCT_CLONES)
        self.assertEqual(result["strategy"], FETCH_LOCAL)
        self.assertTrue(result["reachableAfter"])
        self.assertIn("tools/rt3/rt3/nuovo.py", result["files"])
        self.assertTrue(
            os.path.exists(os.path.join(clone, "tools", "rt3", "rt3", "nuovo.py"))
        )
        with open(os.path.join(clone, "tools", "rt3", "rt3", "model.py"), encoding="utf-8") as h:
            self.assertEqual(h.read().strip(), "VERSION = 2")

    def test_il_trasporto_non_passa_da_un_URL(self):
        """🔴 L'oracolo che la regola difende. Il `remote origin` del clone punta alla
        sorgente su disco; qui si prova che la distribuzione funziona anche **senza**
        alcun remote configurato, cioe' che il path e' la sorgente e non il remote.
        """
        clone = self.make_clone()
        run(clone, "remote", "remove", "origin")
        self.assertEqual(run(clone, "remote"), "", "il clone non ha piu' remote")
        result = distribute.distribute(self.source, clone, self.commit, ["tools/rt3"])
        self.assertTrue(result["reachableAfter"])

    def test_non_sposta_HEAD_ne_tocca_l_indice(self):
        clone = self.make_clone()
        prima_head = run(clone, "rev-parse", "HEAD")
        prima_index = run(clone, "write-tree")
        distribute.distribute(self.source, clone, self.commit, ["tools/rt3"])
        self.assertEqual(run(clone, "rev-parse", "HEAD"), prima_head)
        self.assertEqual(
            run(clone, "write-tree"),
            prima_index,
            "git archive scrive il working tree, non l'indice: un commit di un'altra "
            "sessione non deve assorbire questi file",
        )

    def test_dry_run_non_scrive(self):
        clone = self.make_clone()
        result = distribute.distribute(
            self.source, clone, self.commit, ["tools/rt3"], dry_run=True
        )
        self.assertIn("tools/rt3/rt3/nuovo.py", result["files"])
        self.assertFalse(
            os.path.exists(os.path.join(clone, "tools", "rt3", "rt3", "nuovo.py"))
        )

    def test_commit_inesistente_nella_sorgente(self):
        clone = self.make_clone()
        with self.assertRaises(DistributionError) as ctx:
            distribute.distribute(self.source, clone, "0" * 40, ["tools/rt3"])
        self.assertEqual(ctx.exception.code, "COMMIT_NOT_IN_SOURCE")


class RamoC_FallbackTest(DistributeTestCase):
    """D-5 e D-6: il fallback e' un'ultima risorsa, e dichiara cosa fa."""

    def test_la_copia_e_RIFIUTATA_se_il_bersaglio_e_un_repository(self):
        """🔴 Il controllo negativo. Senza, «copia come fallback» passerebbe sempre e
        nessun test distinguerebbe l'ultima risorsa dall'abitudine."""
        clone = self.make_clone()
        result = distribute.distribute(
            self.source, clone, self.commit, ["tools/rt3"], allow_copy=True
        )
        self.assertEqual(
            result["strategy"],
            FETCH_LOCAL,
            "--allow-copy non deve degradare un caso che Git sa gestire",
        )

    def test_senza_autorizzazione_la_copia_non_parte(self):
        target = self.make_plain_dir()
        with self.assertRaises(DistributionError) as ctx:
            distribute.distribute(self.source, target, self.commit, ["tools/rt3"])
        self.assertEqual(ctx.exception.code, "COPY_NOT_AUTHORIZED")
        self.assertIn("ultima risorsa", str(ctx.exception))

    def test_copia_autorizzata_su_directory_non_repository(self):
        target = self.make_plain_dir()
        result = distribute.distribute(
            self.source, target, self.commit, ["tools/rt3"], allow_copy=True
        )
        self.assertEqual(result["case"], NOT_A_REPO)
        self.assertEqual(result["strategy"], CONTROLLED_COPY)
        self.assertTrue(
            os.path.exists(os.path.join(target, "tools", "rt3", "rt3", "nuovo.py"))
        )

    def test_la_copia_dichiara_i_file_trasferiti(self):
        target = self.make_plain_dir()
        result = distribute.distribute(
            self.source, target, self.commit, ["tools/rt3"], allow_copy=True
        )
        for rel in result["files"]:
            self.assertTrue(
                os.path.exists(os.path.join(target, rel.replace("/", os.sep))),
                "dichiarato ma non copiato: {}".format(rel),
            )

    def test_la_copia_esclude_la_deny_list(self):
        """D-6: `.git` sovrascriverebbe la storia, `runtime.db` e' lo stato di UNA
        macchina. Due modi diversi di distruggere qualcosa che non ci appartiene."""
        write(self.source, "tools/rt3/Saved/log.txt", "rumore\n")
        write(self.source, "tools/rt3/__pycache__/x.pyc", "binario\n")
        write(self.source, "tools/rt3/.rt3/runtime.db", "stato\n")
        write(self.source, "tools/rt3/rt3/vero.py", "ok\n")
        target = self.make_plain_dir()
        result = distribute.distribute(
            self.source, target, self.commit, ["tools/rt3"], allow_copy=True
        )
        copiati = " ".join(result["files"])
        self.assertIn("tools/rt3/rt3/vero.py", copiati)
        for escluso in ("Saved", "__pycache__", ".rt3"):
            self.assertNotIn(escluso, copiati, "{} non doveva essere copiato".format(escluso))
            self.assertFalse(
                os.path.exists(os.path.join(target, "tools", "rt3", escluso)),
                "{} e' finito sul disco del bersaglio".format(escluso),
            )

    def test_dry_run_del_fallback_non_scrive(self):
        target = self.make_plain_dir()
        result = distribute.distribute(
            self.source, target, self.commit, ["tools/rt3"], allow_copy=True, dry_run=True
        )
        self.assertTrue(result["files"])
        self.assertFalse(
            os.path.exists(os.path.join(target, "tools", "rt3", "rt3", "nuovo.py"))
        )


class OrderOfTheChainTest(DistributeTestCase):
    """La catena e' un ORDINE, non tre alternative: A prima di B, B prima di C."""

    def test_la_strategia_dipende_solo_dal_caso_misurato(self):
        atteso = {
            SAME_REPO: NO_TRANSPORT,
            DISTINCT_CLONES: FETCH_LOCAL,
            NOT_A_REPO: CONTROLLED_COPY,
        }
        for target in (self.make_worktree(), self.make_clone(), self.make_plain_dir()):
            with self.subTest(target=os.path.basename(target)):
                caso = distribute.classify(self.source, target)
                result = distribute.distribute(
                    self.source, target, self.commit, ["tools/rt3"], allow_copy=True
                )
                self.assertEqual(result["strategy"], atteso[caso])

    def test_ogni_esito_dichiara_sempre_caso_strategia_e_perche(self):
        """Una distribuzione che non dice cosa ha fatto non e' verificabile."""
        for target in (self.make_worktree(), self.make_clone(), self.make_plain_dir()):
            with self.subTest(target=os.path.basename(target)):
                result = distribute.distribute(
                    self.source, target, self.commit, ["tools/rt3"], allow_copy=True
                )
                for chiave in ("case", "strategy", "reason", "files", "commit"):
                    self.assertIn(chiave, result)
                self.assertTrue(result["reason"].strip())


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
