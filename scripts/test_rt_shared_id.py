#!/usr/bin/env python3
"""Test dell'allocatore atomico degli ID condivisi.

Uso:
    python scripts/test_rt_shared_id.py            # dalla radice del repo
    python -m unittest discover -s scripts -p "test_*.py"

Ogni test costruisce un repository Git **temporaneo**: nessuno tocca il Decision Log reale, e il lock
sotto test non e' mai quello del clone di lavoro.

⚠️ Il test di concorrenza usa `subprocess`, non `threading`, e non e' un dettaglio di stile: `flock` e
`msvcrt.locking` sono lock **per descrittore di file**, quindi due thread dello stesso processo li
attraversano entrambi. Un test a thread verificherebbe una proprieta' che l'implementazione non
promette, e passerebbe o fallirebbe per la ragione sbagliata.
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rt_shared_id as allocator  # noqa: E402

SCRIPT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "rt_shared_id.py")

HEADER = """# Decision Log di prova

## Decision Log

| ID | Decisione | Stato | Note |
|---|---|---|---|
"""


def row(decision_id, thesis, decorated=None):
    """Una riga canonica. `decorated` permette di provare le forme con grassetto/strike."""
    label = decorated or "**%s**" % decision_id
    return "| %s | **%s** Corpo della decisione. | Accettata | - |\n" % (label, thesis)


class GitRepo:
    """Repository temporaneo con un Decision Log scrivibile."""

    def __init__(self):
        self.path = tempfile.mkdtemp(prefix="rt-shared-id-test-")
        self._git("init", "-b", "main")
        self._git("config", "user.email", "test@example.invalid")
        self._git("config", "user.name", "Test")
        self._git("config", "commit.gpgsign", "false")

    def _git(self, *args):
        proc = subprocess.run(["git"] + list(args), cwd=self.path,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if proc.returncode != 0:
            raise AssertionError("git %s: %s" % (" ".join(args), proc.stderr.decode("utf-8", "replace")))
        return proc.stdout.decode("utf-8", "replace")

    def write_log(self, body):
        target = os.path.join(self.path, allocator.DECISION_LOG)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, "w", encoding="utf-8") as handle:
            handle.write(HEADER + body)

    def commit(self, message="wip"):
        self._git("add", "-A")
        self._git("commit", "-m", message)

    def branch(self, name):
        self._git("checkout", "-b", name)

    def checkout(self, name):
        self._git("checkout", name)

    def run(self, *args):
        """Esegue lo script come processo separato, come lo userebbe una sessione."""
        proc = subprocess.run([sys.executable, SCRIPT] + list(args), cwd=self.path,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return proc.returncode, proc.stdout.decode("utf-8", "replace"), proc.stderr.decode("utf-8", "replace")

    def state(self):
        path = os.path.join(self.path, ".git", allocator.STATE_DIRNAME, allocator.STATE_FILE)
        with open(path, "r", encoding="utf-8") as handle:
            return json.load(handle)

    def destroy(self):
        shutil.rmtree(self.path, ignore_errors=True)


class RepoTestCase(unittest.TestCase):
    def setUp(self):
        self.repo = GitRepo()
        self.addCleanup(self.repo.destroy)


# --- Parser --------------------------------------------------------------------------------------

class TestParser(unittest.TestCase):

    def test_riga_normale(self):
        found = allocator.parse_declarations(row("D-001", "Tesi"))
        self.assertEqual([d.id for d in found], ["D-001"])
        self.assertEqual(found[0].num, 1)

    def test_riga_senza_grassetto(self):
        found = allocator.parse_declarations("| D-002 | **Tesi** Corpo. | Accettata | - |")
        self.assertEqual([d.id for d in found], ["D-002"])

    def test_strike_through(self):
        found = allocator.parse_declarations(row("D-002", "Tesi", decorated="~~D-002~~"))
        self.assertEqual([d.id for d in found], ["D-002"])

    def test_strike_esterno_al_grassetto(self):
        """La forma di `D-044` nel Decision Log reale: `| ~~**D-044**~~ |`.

        E' il caso che un pattern scritto sugli esempi della specifica perdeva — una riga su 131, e
        proprio quella di una decisione ritirata, il cui numero resta occupato di proposito.
        """
        for label in ("~~**D-044**~~", "**~~D-044~~**"):
            found = allocator.parse_declarations(row("D-044", "Ritirata", decorated=label))
            self.assertEqual([d.id for d in found], ["D-044"], "forma non riconosciuta: %s" % label)

    def test_ignora_i_riferimenti_nel_testo(self):
        line = "| **D-050** | **Tesi.** Supera [D-123](RT_PDR_00_Decision_Log.md) e cita D-124. | - | - |"
        found = allocator.parse_declarations(line)
        self.assertEqual([d.id for d in found], ["D-050"])

    def test_ignora_le_note_fuori_tabella(self):
        """Le Note in fondo al Decision Log usano `- **D-100** (data):` — non sono dichiarazioni."""
        text = "- **D-100** (2026-08-11): rinumerata due volte, contenuto invariato.\n"
        self.assertEqual(allocator.parse_declarations(text), [])

    def test_rileva_due_dichiarazioni_dello_stesso_id(self):
        found = allocator.parse_declarations(row("D-007", "Prima") + row("D-007", "Seconda"))
        self.assertEqual([d.id for d in found], ["D-007", "D-007"])
        self.assertNotEqual(found[0].fingerprint, found[1].fingerprint)

    def test_formato_invalido(self):
        for bad in ("| **D-44** | x | - | - |", "| D_045 | x | - | - |"):
            self.assertEqual(allocator.parse_declarations(bad), [], bad)
            self.assertEqual(len(allocator.parse_malformed(bad)), 1, bad)

    def test_fingerprint_uguale_se_la_tesi_e_la_stessa(self):
        """Una decisione ereditata e poi ampliata resta la stessa decisione.

        E' il falso positivo misurato su `D-031`, dove un branch portava la stessa tesi con il corpo
        piu' lungo: un gate che lo segnalasse suonerebbe a ogni revisione.
        """
        a = allocator.parse_declarations("| **D-031** | **Le icone sono un catalogo.** Corpo breve. | - | - |")
        b = allocator.parse_declarations("| **D-031** | **Le icone sono un catalogo.** Corpo molto piu' lungo, con altri dettagli. | - | - |")
        self.assertEqual(a[0].fingerprint, b[0].fingerprint)

    def test_fingerprint_diverso_se_la_tesi_cambia(self):
        a = allocator.parse_declarations(row("D-091", "I tre valori acustici"))
        b = allocator.parse_declarations(row("D-091", "Weapon.Environmental resta fuori"))
        self.assertNotEqual(a[0].fingerprint, b[0].fingerprint)


# --- Allocatore ----------------------------------------------------------------------------------

class TestAllocator(RepoTestCase):

    def test_bootstrap_dal_massimo_canonico(self):
        self.repo.write_log(row("D-001", "Una") + row("D-131", "Ultima"))
        self.repo.commit()
        code, out, err = self.repo.run("reserve", "D")
        self.assertEqual(code, 0, err)
        self.assertEqual(out.strip(), "D-132")

    def test_due_allocazioni_danno_id_diversi(self):
        self.repo.write_log(row("D-010", "Una"))
        self.repo.commit()
        first = self.repo.run("reserve", "D")[1].strip()
        second = self.repo.run("reserve", "D")[1].strip()
        self.assertEqual((first, second), ("D-011", "D-012"))

    def test_count_produce_id_contigui(self):
        self.repo.write_log(row("D-010", "Una"))
        self.repo.commit()
        code, out, err = self.repo.run("reserve", "D", "--count", "3")
        self.assertEqual(code, 0, err)
        self.assertEqual(out.split(), ["D-011", "D-012", "D-013"])

    def test_reservation_abbandonata_non_viene_riusata(self):
        """Il contatore e' monotono: un ID prenotato e mai dichiarato resta un buco, non un ID libero."""
        self.repo.write_log(row("D-010", "Una"))
        self.repo.commit()
        abandoned = self.repo.run("reserve", "D")[1].strip()   # D-011, mai scritta nel log
        following = self.repo.run("reserve", "D")[1].strip()
        self.assertEqual(abandoned, "D-011")
        self.assertEqual(following, "D-012")
        # E resta tale anche dopo che il log e' avanzato per altra via.
        self.repo.write_log(row("D-010", "Una") + row("D-012", "Dodici"))
        self.repo.commit()
        self.assertEqual(self.repo.run("reserve", "D")[1].strip(), "D-013")

    def test_il_contatore_non_scende_mai(self):
        """Dopo un pull che porta ID da un altro clone, il massimo canonico ha la meglio.

        E' la contraddizione fra §5.1 e §6 della specifica, risolta a favore di §5.1: se il massimo si
        leggesse solo al bootstrap, un `git pull` che porta `D-140` da un altro PC farebbe riemettere
        ID gia' usati.
        """
        self.repo.write_log(row("D-010", "Una"))
        self.repo.commit()
        self.assertEqual(self.repo.run("reserve", "D")[1].strip(), "D-011")
        self.repo.write_log(row("D-010", "Una") + row("D-140", "Arrivata da un altro clone"))
        self.repo.commit()
        self.assertEqual(self.repo.run("reserve", "D")[1].strip(), "D-141")

    def test_un_branch_aperto_e_una_rivendicazione(self):
        """Un ID dichiarato su un branch non mergiato non e' disponibile.

        Misurato sul repository reale il giorno dell'introduzione: `origin/main` era fermo a `D-131` e
        due branch aperti avevano gia' preso `D-132`. Un allocatore che guardasse solo `main`
        emetterebbe proprio quel numero — diventando la terza sessione a collidere, con lo strumento
        nato per impedirlo.
        """
        self.repo.write_log(row("D-010", "Su main"))
        self.repo.commit()
        self.repo.branch("feat/gia-in-volo")
        self.repo.write_log(row("D-010", "Su main") + row("D-200", "Rivendicata da un branch aperto"))
        self.repo.commit()
        self.repo.checkout("main")
        self.assertEqual(self.repo.run("reserve", "D")[1].strip(), "D-201")

    def test_un_worktree_parallelo_non_committato_e_una_rivendicazione(self):
        """Il caso che ha morso durante l'implementazione stessa.

        Il primo `reserve` reale ha emesso `D-134` mentre un worktree accanto teneva `D-134` fra le
        **modifiche non committate**. Nessun ref lo mostrava; il file su disco si'. E' la finestra in
        cui nascono le collisioni — fra «ho scelto il numero» e «l'ho pushato» — ed e' interna al clone,
        quindi copribile.
        """
        self.repo.write_log(row("D-010", "Su main"))
        self.repo.commit()
        linked = tempfile.mkdtemp(prefix="rt-wt-")
        shutil.rmtree(linked)
        self.repo._git("worktree", "add", linked, "-b", "feat/vicina")
        self.addCleanup(shutil.rmtree, linked, True)

        # La sessione parallela scrive il proprio numero e NON committa.
        target = os.path.join(linked, allocator.DECISION_LOG)
        with open(target, "w", encoding="utf-8") as handle:
            handle.write(HEADER + row("D-010", "Su main") + row("D-300", "Scritta e non committata"))

        self.assertEqual(self.repo.run("reserve", "D")[1].strip(), "D-301")

    def test_stato_round_trip(self):
        self.repo.write_log(row("D-005", "Una"))
        self.repo.commit()
        self.repo.run("reserve", "D", "--reason", "#621 prova")
        state = self.repo.state()
        self.assertEqual(state["schema_version"], allocator.SCHEMA_VERSION)
        self.assertEqual(state["namespaces"]["D"]["last_issued"], 6)
        entry = state["reservations"][-1]
        self.assertEqual(entry["id"], "D-006")
        self.assertEqual(entry["reason"], "#621 prova")
        self.assertEqual(entry["branch"], "main")

    def test_cedere_un_id_non_lo_libera(self):
        """Cedere toglie la diagnostica, non riapre il numero.

        Il caso reale: `D-134` riservato qui e poi ceduto a una sessione che l'aveva gia' scritto nel
        proprio worktree. Senza `release`, la regola `reserved-by-other-branch` avrebbe segnalato come
        collisione l'uso legittimo che quella sessione ne faceva.
        """
        self.repo.write_log(row("D-010", "Una"))
        self.repo.commit()
        ceduto = self.repo.run("reserve", "D")[1].strip()
        self.assertEqual(ceduto, "D-011")

        code, out, err = self.repo.run("release", ceduto)
        self.assertEqual(code, 0, err)
        self.assertEqual([r["id"] for r in self.repo.state()["reservations"]], [])
        # Il contatore NON scende: il prossimo resta D-012, non D-011.
        self.assertEqual(self.repo.state()["namespaces"]["D"]["last_issued"], 11)
        self.assertEqual(self.repo.run("reserve", "D")[1].strip(), "D-012")

    def test_cedere_un_id_inesistente_e_rosso(self):
        self.repo.write_log(row("D-010", "Una"))
        self.repo.commit()
        code, out, err = self.repo.run("release", "D-999")
        self.assertEqual(code, 1)
        self.assertIn("nessuna reservation", err)

    def test_check_non_segnala_un_id_ceduto(self):
        """La regola `reserved-by-other-branch` deve tacere dopo la cessione, e parlare prima."""
        self.repo.write_log(row("D-010", "Una"))
        self.repo.commit()
        self.repo._git("remote", "add", "origin", self.repo.path)
        self.repo._git("update-ref", "refs/remotes/origin/main", "HEAD")
        ceduto = self.repo.run("reserve", "D")[1].strip()      # D-011, a nome di `main`

        self.repo.branch("feat/altra-sessione")
        self.repo.write_log(row("D-010", "Una") + row(ceduto, "Usata da un'altra sessione"))
        self.repo.commit()

        code, out, err = self.repo.run("check")                 # prima: segnalato
        self.assertEqual(code, 1)
        self.assertIn("reserved-by-other-branch", err)

        self.repo.run("release", ceduto)
        code, out, err = self.repo.run("check")                 # dopo: silenzio
        self.assertEqual(code, 0, err)

    def test_status_elenca_le_reservation(self):
        self.repo.write_log(row("D-005", "Una"))
        self.repo.commit()
        self.repo.run("reserve", "D", "--reason", "prova")
        code, out, err = self.repo.run("status")
        self.assertEqual(code, 0, err)
        self.assertIn("D-006", out)
        self.assertIn("orfana", out)      # riservata, non ancora dichiarata nel log

    def test_lo_stato_vive_nel_common_dir_condiviso(self):
        """Due worktree dello stesso clone devono vedere lo stesso contatore.

        E' l'intera ragione per cui lo stato non e' un file versionato: se stesse nel worktree, ogni
        sessione avrebbe il proprio, e il lock non servirebbe a nulla.
        """
        self.repo.write_log(row("D-020", "Una"))
        self.repo.commit()
        linked = tempfile.mkdtemp(prefix="rt-wt-")
        shutil.rmtree(linked)            # `git worktree add` vuole un percorso inesistente
        self.repo._git("worktree", "add", linked, "-b", "feat/parallela")
        self.addCleanup(shutil.rmtree, linked, True)

        first = self.repo.run("reserve", "D")[1].strip()
        proc = subprocess.run([sys.executable, SCRIPT, "reserve", "D"], cwd=linked,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        second = proc.stdout.decode("utf-8", "replace").strip()
        self.assertEqual(proc.returncode, 0, proc.stderr.decode("utf-8", "replace"))
        self.assertEqual((first, second), ("D-021", "D-022"))


# --- Concorrenza ---------------------------------------------------------------------------------

class TestConcorrenza(RepoTestCase):

    PROCESSES = 20

    def test_venti_processi_concorrenti_ottengono_id_distinti(self):
        """Il test che giustifica l'esistenza dello script.

        Venti processi partono insieme contro lo stesso git common dir. Senza sezione critica
        leggerebbero lo stesso massimo e stamperebbero lo stesso ID — che e' esattamente il modo in cui
        il repository ha collezionato quindici collisioni.
        """
        self.repo.write_log(row("D-100", "Una"))
        self.repo.commit()

        running = [
            subprocess.Popen([sys.executable, SCRIPT, "reserve", "D", "--reason", "proc-%d" % i],
                             cwd=self.repo.path, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            for i in range(self.PROCESSES)
        ]
        issued, failures = [], []
        for proc in running:
            out, err = proc.communicate(timeout=120)
            if proc.returncode != 0:
                failures.append(err.decode("utf-8", "replace"))
            else:
                issued.append(out.decode("utf-8", "replace").strip())

        self.assertEqual(failures, [], "processi falliti: %s" % failures[:3])
        self.assertEqual(len(issued), self.PROCESSES)
        self.assertEqual(len(set(issued)), self.PROCESSES, "ID duplicati: %s" % sorted(issued))

        numbers = sorted(int(x.split("-")[1]) for x in issued)
        self.assertEqual(numbers, list(range(101, 101 + self.PROCESSES)),
                         "la sequenza non e' contigua e monotona: %s" % numbers)
        self.assertEqual(self.repo.state()["namespaces"]["D"]["last_issued"], 100 + self.PROCESSES)


# --- check ---------------------------------------------------------------------------------------

class TestCheck(RepoTestCase):

    def test_log_pulito_e_verde(self):
        self.repo.write_log(row("D-001", "Una") + row("D-002", "Due"))
        self.repo.commit()
        code, out, err = self.repo.run("check")
        self.assertEqual(code, 0, err)
        self.assertIn("2 dichiarazioni", out)

    def test_duplicato_e_rosso(self):
        """Sul Decision Log reale i duplicati sono zero, quindi il gate sarebbe verde per costruzione.

        L'unico modo di dimostrare che sa fallire e' introdurne uno e pretendere exit 1: il criterio di
        accettazione «`check` rileva un duplicato reale» non era verificabile come scritto.
        """
        self.repo.write_log(row("D-007", "Prima tesi") + row("D-007", "Seconda tesi"))
        self.repo.commit()
        code, out, err = self.repo.run("check")
        self.assertEqual(code, 1)
        self.assertIn("duplicate: D-007", err)
        self.assertIn("Prima tesi", err)
        self.assertIn("Seconda tesi", err)

    def test_formato_invalido_e_rosso(self):
        self.repo.write_log(row("D-001", "Una") + "| **D-44** | **Tesi.** Corpo. | - | - |\n")
        self.repo.commit()
        code, out, err = self.repo.run("check")
        self.assertEqual(code, 1)
        self.assertIn("malformed", err)


# --- audit-refs ----------------------------------------------------------------------------------

class TestAuditRefs(RepoTestCase):

    def test_stessa_decisione_ereditata_su_due_ref_e_verde(self):
        self.repo.write_log(row("D-001", "Una") + row("D-002", "Due"))
        self.repo.commit()
        self.repo.branch("feat/altra")
        self.repo.write_log(row("D-001", "Una") + row("D-002", "Due") + "\nAltro testo fuori tabella.\n")
        self.repo.commit()
        code, out, err = self.repo.run("audit-refs")
        self.assertEqual(code, 0, err)
        self.assertIn("nessuna collisione", out)

    def test_stesso_id_con_tesi_diversa_e_rosso(self):
        self.repo.write_log(row("D-001", "Una"))
        self.repo.commit()
        self.repo.branch("feat/collide")
        self.repo.write_log(row("D-001", "Una") + row("D-002", "Il redirect si cancella"))
        self.repo.commit()
        self.repo.checkout("main")
        self.repo.write_log(row("D-001", "Una") + row("D-002", "I profili di reazione sono catalogo"))
        self.repo.commit()
        code, out, err = self.repo.run("audit-refs")
        self.assertEqual(code, 1)
        self.assertIn("COLLISION D-002", err)

    def test_due_id_diversi_sono_verdi(self):
        self.repo.write_log(row("D-001", "Una"))
        self.repo.commit()
        self.repo.branch("feat/altra")
        self.repo.write_log(row("D-001", "Una") + row("D-003", "Tesi del branch"))
        self.repo.commit()
        self.repo.checkout("main")
        self.repo.write_log(row("D-001", "Una") + row("D-002", "Tesi di main"))
        self.repo.commit()
        code, out, err = self.repo.run("audit-refs")
        self.assertEqual(code, 0, err)

    def test_duplicato_dentro_lo_stesso_ref(self):
        """Il caso misurato su `origin/docs/wv4-environmental`: D-091 dichiarata due volte nello stesso
        file, una ereditata e una nuova. Il merge non lo risolve, perche' il file e' gia' incoerente."""
        self.repo.write_log(row("D-091", "I tre valori acustici") + row("D-091", "Weapon.Environmental resta fuori"))
        self.repo.commit()
        code, out, err = self.repo.run("audit-refs")
        self.assertEqual(code, 1)
        self.assertIn("DUPLICATE D-091", err)


if __name__ == "__main__":
    unittest.main(verbosity=2)
