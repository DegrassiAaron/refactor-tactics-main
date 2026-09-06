"""Metadati Git: repository vero, creato per il test.

Nessun mock. Un finto `git` proverebbe che il parser regge l'output che il test stesso
ha scritto, e non che regge quello che git produce - che e' l'unica cosa in dubbio,
soprattutto per il detached HEAD.
"""

import os
import shutil
import subprocess
import tempfile
import unittest

from rt3.gitmeta import collect, short_head


def _git_available():
    try:
        subprocess.run(
            ["git", "--version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=10,
        )
        return True
    except (OSError, subprocess.SubprocessError):
        return False


GIT = _git_available()


@unittest.skipUnless(GIT, "git non disponibile")
class GitMetaTest(unittest.TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp(prefix="rt3-git-")
        self.repo = os.path.join(self.root, "repo")
        os.makedirs(self.repo)
        self._run(["init", "-b", "main"])
        self._run(["config", "user.email", "rt3@test.local"])
        self._run(["config", "user.name", "RT3 Test"])
        with open(os.path.join(self.repo, "file.txt"), "w", encoding="utf-8") as fh:
            fh.write("uno\n")
        self._run(["add", "."])
        self._run(["commit", "-m", "primo"])
        self.first = self._out(["rev-parse", "HEAD"])

    def tearDown(self):
        shutil.rmtree(self.root, ignore_errors=True)

    def _run(self, args, cwd=None):
        subprocess.run(
            ["git"] + args,
            cwd=cwd or self.repo,
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

    def _out(self, args, cwd=None):
        proc = subprocess.run(
            ["git"] + args, cwd=cwd or self.repo, check=True, stdout=subprocess.PIPE
        )
        return proc.stdout.decode().strip()

    def test_branch_e_head_su_un_branch_normale(self):
        meta = collect(self.repo)
        self.assertEqual(meta["branch"], "main")
        self.assertEqual(meta["head"], self.first)
        self.assertEqual(
            os.path.normcase(meta["worktreePath"]),
            os.path.normcase(os.path.realpath(self.repo)),
        )
        self.assertEqual(
            os.path.normcase(meta["repoRoot"]),
            os.path.normcase(os.path.realpath(self.repo)),
        )

    def test_detached_head_lascia_il_branch_a_none(self):
        """Il caso che la specifica chiede esplicitamente di gestire."""
        self._run(["checkout", "--detach", "HEAD"])
        meta = collect(self.repo)
        self.assertIsNone(meta["branch"])
        self.assertEqual(meta["head"], self.first)

    def test_worktree_separato_distingue_worktree_da_repo_root(self):
        """Le due colonne non sono ridondanti, e qui si vede perche'."""
        wt = os.path.join(self.root, "wt")
        self._run(["worktree", "add", "-b", "feat/x", wt])
        meta = collect(wt)
        self.assertEqual(meta["branch"], "feat/x")
        self.assertEqual(
            os.path.normcase(meta["worktreePath"]), os.path.normcase(os.path.realpath(wt))
        )
        self.assertEqual(
            os.path.normcase(meta["repoRoot"]),
            os.path.normcase(os.path.realpath(self.repo)),
        )
        self.assertNotEqual(
            os.path.normcase(meta["worktreePath"]), os.path.normcase(meta["repoRoot"])
        )

    def test_fuori_da_un_repository_tutti_i_campi_sono_none(self):
        fuori = os.path.join(self.root, "non-un-repo")
        os.makedirs(fuori)
        meta = collect(fuori)
        self.assertEqual(
            meta, {"repoRoot": None, "worktreePath": None, "branch": None, "head": None}
        )

    def test_directory_inesistente_non_solleva(self):
        meta = collect(os.path.join(self.root, "che-non-c-e"))
        self.assertIsNone(meta["head"])


class ShortHeadTest(unittest.TestCase):
    def test_abbrevia(self):
        self.assertEqual(short_head("a" * 40), "aaaaaaaa")

    def test_head_assente(self):
        self.assertEqual(short_head(None), "-")


if __name__ == "__main__":
    unittest.main()
