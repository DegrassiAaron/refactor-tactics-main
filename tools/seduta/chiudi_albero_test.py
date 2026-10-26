#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Test di `chiudi-albero.ps1` (#3862).

    python -m pytest -q tools/seduta/chiudi_albero_test.py

Il difetto: `giro-meccanico.ps1` chiudeva il gioco con `try { $p.Kill($true) } catch {}`. `Process.Kill(bool)`
esiste solo in .NET Core 3+ (PowerShell 7): in Windows PowerShell 5.1 il metodo non ha quell'overload,
l'eccezione veniva inghiottita dal `catch {}` e `RefactorTactics.exe` restava vivo a consumare CPU.

Questi test girano il helper in OGNI edizione di PowerShell presente sulla macchina (`powershell` = 5.1,
`pwsh` = 7): un test che passasse solo su una delle due non proverebbe il difetto, che e' proprio la
differenza fra le due. E la vittima e' un albero di due processi, perche' chiudere il padre e lasciare il
figlio e' il secondo modo di restare vivi.
"""

import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import uuid

QUI = os.path.dirname(os.path.abspath(__file__))
HELPER = os.path.join(QUI, "chiudi-albero.ps1")

EDIZIONI = [e for e in ("powershell", "pwsh") if shutil.which(e)]

# La vittima: un padre che lancia un figlio con lo stesso marcatore nella riga di comando, e dorme.
VITTIMA = "\n".join([
    "import subprocess, sys, time",
    "subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(120)', sys.argv[1] + '-figlio'])",
    "time.sleep(120)",
    "",
])


def processi_con(marcatore):
    """PID dei processi python che portano il marcatore nella riga di comando (escluso chi interroga)."""
    comando = (
        "Get-CimInstance Win32_Process | Where-Object { $_.Name -match '^python' -and "
        "$_.CommandLine -like '*" + marcatore + "*' -and $_.ProcessId -ne $PID } | "
        "ForEach-Object { $_.ProcessId }"
    )
    out = subprocess.run(["powershell", "-NoProfile", "-Command", comando], capture_output=True, text=True).stdout
    return [int(r) for r in out.split() if r.strip().isdigit()]


class ChiudiAlberoTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.mkdtemp(prefix="chiudi-albero-")
        self.vittima = os.path.join(self.dir, "vittima.py")
        with open(self.vittima, "w", encoding="utf-8", newline="\n") as f:
            f.write(VITTIMA)
        self.marcatore = "marcatore-" + uuid.uuid4().hex

    def tearDown(self):
        for pid in processi_con(self.marcatore):
            subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"], capture_output=True)
        shutil.rmtree(self.dir, ignore_errors=True)

    def esegui(self, edizione, atteso):
        """Lancia la vittima, chiama ChiudiAlbero con `atteso` come marcatore da verificare, e ne riporta l'esito."""
        script = os.path.join(self.dir, "prova.ps1")
        ps = "\n".join([
            ". '" + HELPER + "'",
            "$p = Start-Process -FilePath '" + sys.executable + "' -ArgumentList @('\"" + self.vittima + "\"', '" + self.marcatore + "') -PassThru",
            "Start-Sleep -Milliseconds 2000",
            "$r = ChiudiAlbero -Processo $p -CommandLineContiene '" + atteso + "'",
            "Write-Output ('RISULTATO=' + $r)",
            "",
        ])
        with open(script, "w", encoding="utf-8-sig", newline="\r\n") as f:
            f.write(ps)
        cp = subprocess.run([edizione, "-NoProfile", "-File", script], capture_output=True, text=True, timeout=90)
        return cp.stdout + cp.stderr

    def test_ci_sono_edizioni_da_provare(self):
        # controllo del controllo: senza nemmeno una edizione, gli altri test passerebbero a vuoto
        self.assertTrue(EDIZIONI, "nessuna edizione di PowerShell trovata")

    @unittest.skipUnless(shutil.which("powershell"), "serve Windows PowerShell 5.1")
    def test_controllo_positivo_il_vecchio_metodo_fallisce_su_5_1(self):
        # E' la CAUSA di #3862, non un test del helper: se `Kill($true)` cominciasse a funzionare su 5.1, il
        # difetto non esisterebbe piu' e il helper sarebbe un di piu'. Lo si dice qui, con la sua prova.
        script = os.path.join(self.dir, "vecchio.ps1")
        ps = "\n".join([
            "$p = Start-Process -FilePath '" + sys.executable + "' -ArgumentList @('-c', '\"import time; time.sleep(30)\"') -PassThru",
            "try { $p.Kill($true); Write-Output 'KILL_RIUSCITO' } catch { Write-Output ('KILL_ECCEZIONE=' + $_.Exception.Message) }",
            "if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }",
            "",
        ])
        with open(script, "w", encoding="utf-8-sig", newline="\r\n") as f:
            f.write(ps)
        cp = subprocess.run(["powershell", "-NoProfile", "-File", script], capture_output=True, text=True, timeout=60)
        self.assertIn("KILL_ECCEZIONE=", cp.stdout + cp.stderr, "Kill($true) non lancia piu' su 5.1: " + cp.stdout + cp.stderr)

    def test_chiude_padre_e_figlio_in_ogni_edizione(self):
        for ed in EDIZIONI:
            with self.subTest(edizione=ed):
                out = self.esegui(ed, self.marcatore)
                self.assertIn("RISULTATO=True", out, out)
                self.assertEqual(processi_con(self.marcatore), [], "restano processi vivi: " + out)

    def test_non_chiude_un_processo_che_non_porta_il_marcatore_atteso(self):
        for ed in EDIZIONI:
            with self.subTest(edizione=ed):
                out = self.esegui(ed, "marcatore-di-un-altro-processo")
                self.assertIn("RISULTATO=False", out, out)
                self.assertNotEqual(processi_con(self.marcatore), [], "il processo e' stato chiuso senza il suo marcatore: " + out)
                for pid in processi_con(self.marcatore):
                    subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"], capture_output=True)

    def test_un_processo_gia_uscito_non_e_un_errore(self):
        for ed in EDIZIONI:
            with self.subTest(edizione=ed):
                script = os.path.join(self.dir, "uscito.ps1")
                ps = "\n".join([
                    ". '" + HELPER + "'",
                    "$p = Start-Process -FilePath '" + sys.executable + "' -ArgumentList @('-c', '\"pass\"') -PassThru",
                    "$p.WaitForExit()",
                    "Write-Output ('RISULTATO=' + (ChiudiAlbero -Processo $p -CommandLineContiene 'qualsiasi'))",
                    "",
                ])
                with open(script, "w", encoding="utf-8-sig", newline="\r\n") as f:
                    f.write(ps)
                cp = subprocess.run([ed, "-NoProfile", "-File", script], capture_output=True, text=True, timeout=60)
                self.assertIn("RISULTATO=True", cp.stdout + cp.stderr, cp.stdout + cp.stderr)


if __name__ == "__main__":
    unittest.main()
