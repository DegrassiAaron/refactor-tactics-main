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


# La vittima ORFANA: il padre lancia il figlio e ESCE. E' il caso in cui `taskkill /T` sul padre non basta, perche'
# il figlio non ha piu' un padre vivo da cui essere raggiunto.
VITTIMA_ORFANA = "\n".join([
    "import subprocess, sys",
    "subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(120)', sys.argv[1] + '-figlio'])",
    "",
])

# Attende che il FIGLIO esista, invece di dormire a tempo: su una macchina carica il padre puo' essere chiuso prima
# che il figlio sia nato, e il test passerebbe senza aver provato il figlio.
ATTENDI_FIGLIO = [
    "for ($i = 0; $i -lt 100; $i++) {",
    "  if (@(Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*MARCATORE-figlio*' -and $_.ProcessId -ne $PID }).Count -gt 0) { break }",
    "  Start-Sleep -Milliseconds 100",
    "}",
]


def processi_con(marcatore):
    """PID dei processi che portano il marcatore nella riga di comando (escluso chi interroga)."""
    comando = (
        "Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*" + marcatore + "*' -and "
        "$_.ProcessId -ne $PID } | ForEach-Object { $_.ProcessId }"
    )
    out = subprocess.run([EDIZIONI[0], "-NoProfile", "-Command", comando], capture_output=True, text=True).stdout
    return [int(r) for r in out.split() if r.strip().isdigit()]


class ChiudiAlberoTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.mkdtemp(prefix="chiudi-albero-")
        self.vittima = os.path.join(self.dir, "vittima.py")
        with open(self.vittima, "w", encoding="utf-8", newline="\n") as f:
            f.write(VITTIMA)
        self.orfana = os.path.join(self.dir, "vittima_orfana.py")
        with open(self.orfana, "w", encoding="utf-8", newline="\n") as f:
            f.write(VITTIMA_ORFANA)
        self.marcatore = "marcatore-" + uuid.uuid4().hex

    def tearDown(self):
        for pid in processi_con(self.marcatore):
            subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"], capture_output=True)
        shutil.rmtree(self.dir, ignore_errors=True)

    def esegui(self, edizione, atteso, vittima=None, attendi_padre_uscito=False):
        """Lancia la vittima, chiama ChiudiAlbero con `atteso` come marcatore da verificare, e ne riporta l'esito."""
        script = os.path.join(self.dir, "prova.ps1")
        attesa = [r.replace("MARCATORE", self.marcatore) for r in ATTENDI_FIGLIO]
        ps = "\n".join([
            ". '" + HELPER + "'",
            "$p = Start-Process -FilePath '" + sys.executable + "' -ArgumentList @('\"" + (vittima or self.vittima) + "\"', '" + self.marcatore + "') -PassThru",
        ] + ["$p.WaitForExit()"] * attendi_padre_uscito + attesa + [
            "Write-Output ('FIGLIO_PRESENTE=' + (@(Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*" + self.marcatore + "-figlio*' -and $_.ProcessId -ne $PID }).Count -gt 0))",
            "$r = ChiudiAlbero -Processo $p -CommandLineContiene '" + atteso + "'",
            "Write-Output ('RISULTATO=' + $r)",
            "",
        ])
        with open(script, "w", encoding="utf-8-sig", newline="\r\n") as f:
            f.write(ps)
        cp = subprocess.run([edizione, "-NoProfile", "-File", script], capture_output=True, text=True, timeout=90)
        return cp.stdout + cp.stderr

    def test_ci_sono_edizioni_da_provare(self):
        # controllo del controllo: senza nemmeno una edizione, gli altri test passerebbero a vuoto. E il difetto e'
        # SPECIFICO di Windows PowerShell 5.1: provare solo `pwsh` lo lascerebbe non provato senza che nulla lo dica.
        self.assertIn("powershell", EDIZIONI, "serve Windows PowerShell 5.1: e' l'edizione del difetto")

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
                self.assertIn("FIGLIO_PRESENTE=True", out, "il figlio non e' nato: il test non proverebbe nulla. " + out)
                self.assertIn("RISULTATO=True", out, out)
                self.assertEqual(processi_con(self.marcatore), [], "restano processi vivi: " + out)

    def test_chiude_un_figlio_orfano_che_porta_il_marcatore(self):
        # il padre e' gia' uscito, il figlio no: chiudere "il processo" ritorna subito, e il gioco resta vivo
        for ed in EDIZIONI:
            with self.subTest(edizione=ed):
                out = self.esegui(ed, self.marcatore, vittima=self.orfana, attendi_padre_uscito=True)
                self.assertIn("FIGLIO_PRESENTE=True", out, "il figlio non e' nato: il test non proverebbe nulla. " + out)
                self.assertIn("RISULTATO=True", out, out)
                self.assertEqual(processi_con(self.marcatore), [], "resta un orfano vivo: " + out)

    def test_non_tocca_un_processo_estraneo_che_cita_il_marcatore(self):
        # Il marcatore e' il percorso del log della run: una shell che lo legge (`Get-Content -Wait`), un editor
        # aperto sul log o una query di diagnostica lo hanno nella riga di comando, e NON sono il gioco. Chiudere
        # "tutto cio' che cita il marcatore" li ucciderebbe: si chiude solo cio' che e' lo STESSO eseguibile.
        for ed in EDIZIONI:
            with self.subTest(edizione=ed):
                estraneo = subprocess.Popen([ed, "-NoProfile", "-Command", "Start-Sleep 60 # " + self.marcatore])
                try:
                    out = self.esegui(ed, self.marcatore)
                    self.assertIn("RISULTATO=True", out, out)
                    self.assertIsNone(estraneo.poll(), "un processo estraneo che citava il marcatore e' stato chiuso: " + out)
                finally:
                    subprocess.run(["taskkill", "/PID", str(estraneo.pid), "/T", "/F"], capture_output=True)

    def test_non_chiude_un_processo_che_non_porta_il_marcatore_atteso(self):
        for ed in EDIZIONI:
            with self.subTest(edizione=ed):
                out = self.esegui(ed, "marcatore-di-un-altro-processo")
                self.assertIn("FIGLIO_PRESENTE=True", out, out)
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
