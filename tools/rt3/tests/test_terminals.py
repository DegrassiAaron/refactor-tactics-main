"""Terminali gestiti: identita' di processo, tempo, e cio' che RT3 non deve chiudere.

Il test che regge questo file e' `PidReuseTest`. Tutto il resto e' comodita'; quello e'
sicurezza: un PID riusato che venisse scambiato per il nostro farebbe chiudere a RT3 il
programma di qualcun altro.
"""

import os
import unittest

from rt3.errors import TerminalAlreadyOpen, TerminalNotFound
from rt3.launcher import (
    SHELL_PREFERITE,
    ps_quote,
    render_script,
    resolve_shell,
    rt_terminal_snippet,
)
from rt3.model import now_iso
from rt3.terminals import (
    TERMINAL_STATES,
    check_terminal_state,
    elapsed_ms,
    format_elapsed,
    format_short_elapsed,
    identity_matches,
    process_started_at,
    verify,
    window_title,
)
from tests.harness import LocalDaemon, Rt3TestCase, client, start_session

T0 = "2026-09-07T10:52:14Z"


def term(**kw):
    base = {
        "terminal_id": "term_1", "session_id": "DEV-1", "ownership": "RT3_MANAGED",
        "shell_type": "powershell", "process_id": 4242,
        "process_started_at": T0, "state": "ACTIVE",
    }
    base.update(kw)
    return base


class PidReuseTest(unittest.TestCase):
    """🔴 §22, §36: il PID da solo non e' identita'.

    Windows riusa i numeri. Un terminale registrato con PID 4242 e un sistema in cui
    4242 e' ora un altro programma devono dare `LOST`, mai una terminazione.
    """

    def test_stesso_pid_ma_avviato_dopo_e_un_ALTRO_processo(self):
        stato, motivo = verify(term(), ora_di_avvio="2026-09-07T18:00:00Z")
        self.assertEqual(stato, "LOST")
        self.assertIn("riusato", motivo)

    def test_stesso_pid_stesso_avvio_e_il_nostro(self):
        stato, motivo = verify(term(), ora_di_avvio=T0)
        self.assertEqual(stato, "ACTIVE")
        self.assertIsNone(motivo)

    def test_processo_sparito(self):
        stato, motivo = verify(term(), ora_di_avvio=None)
        self.assertEqual(stato, "LOST")
        self.assertIn("non esiste", motivo)

    def test_senza_pid_non_si_identifica_niente(self):
        stato, motivo = verify(term(process_id=None))
        self.assertEqual(stato, "LOST")

    def test_identita_ha_TRE_esiti_non_due(self):
        """`None` non e' `False`: «non lo so» e «e' un altro» portano entrambi a non
        toccare niente, ma per ragioni diverse, e il messaggio le distingue."""
        self.assertIs(identity_matches(T0, T0), True)
        self.assertIs(identity_matches(T0, "2026-01-01T00:00:00Z"), False)
        self.assertIsNone(identity_matches(T0, None))
        self.assertIsNone(identity_matches(None, T0))

    def test_un_terminale_gia_chiuso_non_si_riverifica(self):
        for stato in ("CLOSED", "LOST"):
            self.assertEqual(verify(term(state=stato))[0], stato)


class ProcessIdentityTest(unittest.TestCase):
    """La lettura vera dal sistema operativo."""

    def test_il_processo_corrente_risulta_vivo(self):
        self.assertIsNotNone(process_started_at(os.getpid()))

    def test_un_pid_impossibile_non_risulta_vivo(self):
        self.assertIsNone(process_started_at(999999999))

    def test_pid_nullo(self):
        self.assertIsNone(process_started_at(0))
        self.assertIsNone(process_started_at(None))


class ElapsedTest(unittest.TestCase):
    """§12, §39: la durata e' derivata, e il tempo che passa non e' uno stato."""

    def test_il_caso_della_specification(self):
        """T0 + 3726 secondi = 01:02:06."""
        ms = elapsed_ms("2026-09-07T10:00:00Z", "2026-09-07T11:02:06Z")
        self.assertEqual(ms, 3726 * 1000)
        self.assertEqual(format_elapsed(ms), "01:02:06")

    def test_oltre_le_ventiquattro_ore(self):
        ms = elapsed_ms("2026-09-06T10:00:00Z", "2026-09-07T13:17:09Z")
        self.assertEqual(format_elapsed(ms), "1d 03:17:09")

    def test_forma_breve_per_il_titolo(self):
        self.assertEqual(format_short_elapsed(3726 * 1000), "+01:02")
        self.assertEqual(format_short_elapsed(0), "+00:00")

    def test_mai_negativa(self):
        """Un orologio che va all'indietro non deve produrre `-00:00:01`."""
        self.assertEqual(elapsed_ms("2026-09-07T11:00:00Z", "2026-09-07T10:00:00Z"), 0)

    def test_istante_mancante_o_invalido(self):
        self.assertIsNone(elapsed_ms(None, now_iso()))
        self.assertIsNone(elapsed_ms("non-una-data", now_iso()))
        self.assertEqual(format_elapsed(None), "--:--:--")

    def test_il_tempo_che_passa_NON_cambia_la_revisione(self):
        """§12: due letture a distanza di un'ora hanno la stessa `stateRevision`.

        Vive qui e non solo in `test_status.py` perche' e' la proprieta' che rende
        sopportabile un titolo che mostra `+00:38`: il clock e' presentazione.
        """
        from rt3.status import build_snapshot

        v = {"protocolVersion": 3, "schemaVersion": 5, "roadmapSchemaVersion": 1}
        sess = {"session_id": "DEV-1", "role": "DEV", "lane": "MAIN",
                "workspace_group": "MAIN", "status": "ACTIVE", "write_mode": "WRITER"}
        a = build_snapshot(session=sess, versions=v, generated_at="2026-09-07T10:00:00Z")
        b = build_snapshot(session=sess, versions=v, generated_at="2026-09-07T11:00:00Z")
        self.assertNotEqual(a["generatedAt"], b["generatedAt"])
        self.assertEqual(a["stateRevision"], b["stateRevision"])


class WindowTitleTest(unittest.TestCase):
    """§10: il titolo viene dal runtime, e non contiene niente di pericoloso."""

    def test_dev_mostra_task_e_modalita(self):
        t = window_title({"session_id": "DEV-MAIN-1937-1", "role": "DEV",
                          "task_id": "1936", "write_mode": "WRITER"}, "+00:07")
        self.assertEqual(t, "RT3 | DEV-MAIN-1937-1 | 1936 | WRITER | +00:07")

    def test_editor_mostra_l_epic(self):
        t = window_title({"session_id": "EDITOR-MAIN", "role": "EDITOR",
                          "epic_id": "EPIC-1937"}, "+00:18")
        self.assertEqual(t, "RT3 | EDITOR-MAIN | EPIC-1937 | +00:18")

    def test_validation_senza_lavoro_dice_WAITING(self):
        t = window_title({"session_id": "VALIDATOR-1937", "role": "VALIDATION"}, "+00:12")
        self.assertEqual(t, "RT3 | VALIDATOR-1937 | WAITING | +00:12")

    def test_niente_epic_o_task_inventati(self):
        """⛔ Se il runtime non li ha, il titolo lo dice invece di riempirli."""
        t = window_title({"session_id": "DEV-1", "role": "DEV"}, "+00:00")
        self.assertIn("no task", t)

    def test_i_valori_pericolosi_vengono_ripuliti(self):
        t = window_title({"session_id": "DEV-1", "role": "DEV",
                          "task_id": "a\r\nb; rm -rf /", "write_mode": "WRITER"}, "+00:00")
        for c in "\r\n;":
            self.assertNotIn(c, t)


class QuotingTest(unittest.TestCase):
    """§43: path Windows con spazi, parentesi e apostrofi devono arrivare intatti."""

    CATTIVI = [
        r"D:\Progetti\Refactor Tactics\wt",
        r"D:\Progetti\O'Brien (test)\wt",
        r"D:\a'b\c(d)e f\g",
        r"D:\normale\senza\niente",
    ]

    def test_ogni_path_sopravvive_alla_citazione(self):
        for p in self.CATTIVI:
            with self.subTest(p=p):
                citato = ps_quote(p)
                self.assertTrue(citato.startswith("'") and citato.endswith("'"))
                # PowerShell: dentro '...' l'unica sequenza e' '' per un apice.
                interno = citato[1:-1].replace("''", "'")
                self.assertEqual(interno, p)

    def test_un_apostrofo_non_chiude_la_stringa(self):
        self.assertEqual(ps_quote("O'Brien"), "'O''Brien'")

    def test_lo_script_generato_contiene_i_valori_citati(self):
        sess = {"session_id": "DEV-1", "role": "DEV", "task_id": "1936",
                "write_mode": "WRITER", "started_at": T0}
        script = render_script(
            session=sess, terminal_id="term_1", title_base="RT3 | DEV-1 | 1936 | WRITER",
            python_executable=r"C:\Program Files\Python\python.exe",
            package_root=r"D:\a'b\c(d)e f\tools\rt3", started_at=T0,
        )
        self.assertIn("'D:\\a''b\\c(d)e f\\tools\\rt3'", script)
        self.assertIn("'C:\\Program Files\\Python\\python.exe'", script)

    def test_lo_script_non_avvia_nessun_agente(self):
        """§27: la finestra si apre pronta, e basta."""
        script = render_script(
            session={"session_id": "DEV-1", "role": "DEV", "started_at": T0},
            terminal_id="term_1", title_base="RT3 | DEV-1",
            python_executable="python", package_root="pkg", started_at=T0,
        )
        testo = script.lower()
        for agente in ("claude", "codex", "chatgpt", "copilot"):
            self.assertNotIn(agente, testo)

    def test_lo_script_propaga_RT3_HOME_solo_se_c_e(self):
        comune = dict(
            session={"session_id": "DEV-1", "role": "DEV", "started_at": T0},
            terminal_id="term_1", title_base="RT3 | DEV-1",
            python_executable="python", package_root="pkg", started_at=T0,
        )
        self.assertIn("$env:RT3_HOME = 'D:\\home'",
                      render_script(rt3_home=r"D:\home", **comune))
        self.assertNotIn("RT3_HOME", render_script(**comune))


class IdentitaSessioneRTTest(unittest.TestCase):
    """La finestra deve poter LAVORARE, non solo aprirsi.

    🔴 `rt-lease.ps1` concede il motore solo a una console che dichiara
    `RT_TERMINAL_OWNER_PID` e `RT_TERMINAL_OWNER_STARTED_AT`. Una finestra gestita che
    non li ha riceve `RT_SESSION_REQUIRED` e non può compilare: è successo nel pilot
    EPIC-1937, con la build fermata prima di toccare il compilatore.

    ⛔ RT3 non li imposta da sé: `scripts/rt-terminal.ps1` possiede quella logica, con le
    stesse cautele (PID più istante di avvio, fail-closed se l'istante non si legge).
    Due sedi per la stessa identità divergerebbero al primo campo aggiunto.
    """

    SESS = {"session_id": "DEV-MAIN-1937-1", "role": "DEV", "task_id": "1936",
            "write_mode": "WRITER", "started_at": T0}
    REPO = "D:/Repo/rt"

    def test_lo_script_adotta_l_identita_RT_dal_repository(self):
        s = rt_terminal_snippet(self.REPO, self.SESS)
        self.assertIn("rt-terminal.ps1", s)
        self.assertIn(". $RT3TerminalScript", s, "dot-source, non invocazione")
        self.assertIn("-Role DEV", s)
        self.assertIn("-InstanceId 'DEV-MAIN-1937-1'", s)
        self.assertIn("-TaskId '1936'", s)

    def test_il_ruolo_passa_INTATTO_per_i_tre_ammessi(self):
        for ruolo in ("DEV", "EDITOR", "VALIDATION"):
            with self.subTest(ruolo=ruolo):
                s = rt_terminal_snippet(self.REPO, dict(self.SESS, role=ruolo))
                self.assertIn("-Role " + ruolo, s)

    def test_un_ruolo_sconosciuto_non_rompe_il_dot_source(self):
        """`rt-terminal.ps1` ha un `ValidateSet`: un valore fuori lista farebbe fallire
        il dot-source, e la finestra si aprirebbe senza identità senza dirlo."""
        s = rt_terminal_snippet(self.REPO, dict(self.SESS, role="QA"))
        self.assertIn("-Role DEV", s)

    def test_senza_repo_root_non_si_inventa_un_path(self):
        self.assertEqual(rt_terminal_snippet(None, self.SESS), "")

    def test_se_lo_script_manca_la_finestra_lo_DICE(self):
        s = rt_terminal_snippet(self.REPO, self.SESS)
        self.assertIn("Test-Path", s, "verifica prima di dot-sourciare")
        self.assertIn("else", s)
        self.assertIn("niente build", s, "l'avviso dice cosa NON si potrà fare")

    def test_l_apostrofo_dell_avviso_e_citato(self):
        """`potra'` dentro una stringa PowerShell in apici singoli: se l'apice non
        raddoppiasse, chiuderebbe la stringa e lo script non parserebbe."""
        s = rt_terminal_snippet(self.REPO, self.SESS)
        riga = [r for r in s.split(chr(10)) if "Write-Host" in r][0]
        self.assertIn("potra''", riga)
        self.assertEqual(riga.count("'") % 2, 0, "apici bilanciati")

    def test_lo_script_completo_contiene_il_dot_source(self):
        script = render_script(
            session=self.SESS, terminal_id="term_1", title_base="RT3 | DEV-1",
            python_executable="python", package_root="pkg", started_at=T0,
            repo_root=self.REPO,
        )
        self.assertIn("rt-terminal.ps1", script)
        # E sta PRIMA delle definizioni RT3, così titolo e prompt di RT3 vincono su
        # quelli che `rt-terminal.ps1` imposta per conto suo.
        self.assertLess(script.index("rt-terminal.ps1"), script.index("RT3-Title"))

    def test_senza_repo_root_lo_script_si_genera_lo_stesso(self):
        """Una finestra fuori da un checkout RT deve aprirsi: serve a leggere e a usare
        `rt3`, e il segnaposto vuoto non deve lasciare sintassi rotta."""
        script = render_script(
            session=self.SESS, terminal_id="term_1", title_base="RT3 | DEV-1",
            python_executable="python", package_root="pkg", started_at=T0,
        )
        self.assertNotIn("rt-terminal.ps1", script)
        self.assertIn("RT3-Title", script)


class ShellTest(unittest.TestCase):
    def test_la_shell_si_risolve_in_modo_deterministico(self):
        nome, exe = resolve_shell()
        self.assertIn(nome, ("powershell", "pwsh"))
        self.assertTrue(os.path.exists(exe), exe)
        self.assertEqual(resolve_shell()[1], exe, "due chiamate, stessa risposta")

    def test_pwsh_viene_PRIMA_di_powershell(self):
        """🔴 L'ordine e' un requisito, non una preferenza.

        Gli script RT usano sintassi PowerShell 7. Windows PowerShell 5.1 non li parsa -
        misurato su `scripts/rt-suite.ps1`: 30 errori con 5.1, zero con 7.6.5 - e
        `rt-lease.ps1` carica quello script come engine guard. Da una finestra 5.1 ogni
        build muore con `ENGINE_GUARD_UNAVAILABLE` prima di iniziare, ed e' successo nel
        pilot EPIC-1937.
        """
        self.assertEqual(SHELL_PREFERITE[0], "pwsh")
        self.assertIn("powershell", SHELL_PREFERITE, "il fallback resta")

    def test_il_fallback_esiste_ancora(self):
        """`pwsh` non si assume presente: se manca si ripiega, e la lista ha due voci
        proprio per questo."""
        self.assertEqual(resolve_shell("powershell")[0], "powershell")

    def test_lo_script_RT_che_ha_rivelato_il_difetto_richiede_davvero_il_7(self):
        """Controllo positivo, sulla FONTE del requisito e non sulla sua conseguenza.

        Se un domani `rt-suite.ps1` diventasse compatibile con 5.1, questo test lo
        direbbe - e la preferenza tornerebbe una scelta invece che un vincolo. Senza,
        l'ordine resterebbe cablato per una ragione che nessuno puo' piu' verificare.
        """
        import subprocess

        # tests -> rt3 -> tools -> radice del repository: quattro livelli, non tre.
        # Con tre il file non si trovava e il test si SALTAVA, cioe' non provava niente.
        radice = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
        suite = os.path.join(radice, "scripts", "rt-suite.ps1")
        if os.name != "nt" or not os.path.exists(suite):
            self.skipTest("richiede Windows e scripts/rt-suite.ps1")

        def errori_con(exe):
            ps = (
                "$e = $null; $t = $null; "
                "[System.Management.Automation.Language.Parser]::ParseFile("
                "'{}', [ref]$t, [ref]$e) | Out-Null; $e.Count".format(
                    suite.replace("'", "''"))
            )
            out = subprocess.run([exe, "-NoProfile", "-Command", ps],
                                 capture_output=True, text=True, timeout=120)
            return int((out.stdout or "0").strip() or 0)

        self.assertGreater(errori_con("powershell"), 0,
                           "se 5.1 lo parsasse, la preferenza non sarebbe un vincolo")
        self.assertEqual(errori_con("pwsh"), 0, "il 7 deve parsarlo")


class RegistryTest(Rt3TestCase):
    """Il registro persistente: un solo terminale vivo per sessione."""

    def setUp(self):
        Rt3TestCase.setUp(self)
        self.store = self.open_store()
        self.store.start_session(
            session_id="DEV-1", role="DEV", workspace_group="MAIN", lane="MAIN",
            worktree_path=r"D:\wt", write_mode="READ_ONLY",
        )

    def test_nasce_STARTING_e_diventa_ACTIVE_solo_con_l_identita(self):
        t = self.store.create_terminal("DEV-1", "powershell", r"D:\wt")
        self.assertEqual(t["state"], "STARTING")
        self.assertEqual(t["ownership"], "RT3_MANAGED")

        # Senza istante di avvio l'identita' e' incompleta: NON diventa attivo.
        parziale = self.store.attach_terminal_process(t["terminal_id"], 4242, None)
        self.assertEqual(parziale["state"], "STARTING")

        pieno = self.store.attach_terminal_process(t["terminal_id"], 4242, T0)
        self.assertEqual(pieno["state"], "ACTIVE")
        self.assertEqual(pieno["process_started_at"], T0)

    def test_una_sola_finestra_viva_per_sessione(self):
        self.store.create_terminal("DEV-1", "powershell", r"D:\wt")
        with self.assertRaises(TerminalAlreadyOpen):
            self.store.create_terminal("DEV-1", "powershell", r"D:\wt")

    def test_dopo_la_chiusura_se_ne_puo_aprire_un_altro(self):
        t = self.store.create_terminal("DEV-1", "powershell", r"D:\wt")
        self.store.set_terminal_state(t["terminal_id"], "CLOSED", close_reason="test")
        altro = self.store.create_terminal("DEV-1", "powershell", r"D:\wt")
        self.assertNotEqual(altro["terminal_id"], t["terminal_id"])

    def test_una_sessione_MANUALE_non_ha_riga(self):
        """🔴 §1: e' questa assenza che impedisce a RT3 di chiudere la finestra di una
        persona. Non un flag da controllare: proprio nessuna riga."""
        self.assertIsNone(self.store.get_terminal_for_session("DEV-1"))
        self.assertEqual(self.store.list_terminals(), [])

    def test_chiudere_registra_quando_e_perche(self):
        t = self.store.create_terminal("DEV-1", "powershell", r"D:\wt")
        chiuso = self.store.set_terminal_state(
            t["terminal_id"], "CLOSED", close_reason="SESSION_STOP", exit_code=0)
        self.assertEqual(chiuso["state"], "CLOSED")
        self.assertEqual(chiuso["close_reason"], "SESSION_STOP")
        self.assertIsNotNone(chiuso["closed_at"])

    def test_stato_non_ammesso_rifiutato(self):
        with self.assertRaises(ValueError):
            check_terminal_state("QUASI_CHIUSO")
        for s in TERMINAL_STATES:
            self.assertEqual(check_terminal_state(s), s)


class ReadOnlyNotBlockedTest(Rt3TestCase):
    """🔴 P0 trovato dal pilot EPIC-1937, non dalla suite.

    EDITOR e VALIDATION, entrambe READ_ONLY sullo stesso albero del DEV, mostravano
    `[RT3 BLOCKED] / RESOLVE_WRITER_CONFLICT`. Era falso e induceva l'azione sbagliata:
    il rimedio suggerito era fermare il DEV che lavorava legittimamente.

    Il modello e' `WriterCount(worktree) <= 1`, non `SessionCount <= 1`. Leggere lo
    stesso albero mentre un altro scrive e' il caso NORMALE - e' il motivo per cui
    EDITOR e VALIDATION esistono.
    """

    WT = os.path.join(os.sep, "alberi", "condiviso")

    def _snapshot(self, cli, sid):
        return cli.call("status.snapshot", sessionId=sid,
                        git={"worktreePath": self.WT, "repoRoot": self.WT,
                             "branch": "b", "head": "h" * 40})

    def test_le_sessioni_READ_ONLY_non_sono_bloccate_dal_writer_altrui(self):
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "MAIN", worktreePath=self.WT,
                          writeMode="WRITER")
            for sid, ruolo in (("EDITOR-1", "EDITOR"), ("VALIDATOR-1", "VALIDATION")):
                start_session(cli, sid, ruolo, "MAIN", worktreePath=self.WT,
                              writeMode="READ_ONLY")

            for sid in ("EDITOR-1", "VALIDATOR-1"):
                with self.subTest(sessione=sid):
                    s = self._snapshot(cli, sid)
                    self.assertIsNone(
                        s["blockingReason"],
                        "una sessione READ_ONLY non e' bloccata da un writer altrui",
                    )
                    self.assertNotEqual(s["requiredAction"], "RESOLVE_WRITER_CONFLICT")
                    # Il lease si VEDE - serve a sapere chi scrive - ma non blocca.
                    self.assertEqual(s["writerLeaseOwner"], "DEV-1")

    def test_il_writer_che_TIENE_il_lease_non_e_bloccato(self):
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "MAIN", worktreePath=self.WT,
                          writeMode="WRITER")
            s = self._snapshot(cli, "DEV-1")
            self.assertIsNone(s["blockingReason"])

    def test_ma_un_WRITER_senza_il_lease_resta_bloccato(self):
        """Controllo positivo: la condizione e' stata RISTRETTA, non tolta.

        Il caso «writer con il lease di un altro» non si puo' costruire - l'indice unico
        lo impedisce a monte - quindi si prova l'altro ramo, che la correzione non deve
        aver toccato: una sessione che si dichiara WRITER e il lease non ce l'ha.
        """
        with LocalDaemon() as d:
            cli = client()
            start_session(cli, "DEV-1", "DEV", "MAIN", worktreePath=self.WT,
                          writeMode="WRITER")
            prima = self._snapshot(cli, "DEV-1")
            self.assertIsNone(prima["blockingReason"], "col lease non e' bloccata")

            # Il lease sparisce senza che la sessione lo sappia: e' lo stato che si
            # eredita da un database scritto prima dell'enforcement.
            lease = [l for l in d.store.list_leases()
                     if l["owner_session_id"] == "DEV-1"][0]
            d.store.release_lease(lease["resource_type"], lease["resource_key"],
                                  session_id="DEV-1")

            dopo = self._snapshot(cli, "DEV-1")
            self.assertEqual(dopo["blockingReason"], "RESOURCE_WRITER")
            self.assertEqual(dopo["requiredAction"], "RESOLVE_WRITER_CONFLICT")

class SpawnFailureTest(Rt3TestCase):
    """§34: se la finestra non si apre, non deve restare NIENTE.

    Il caso peggiore non e' l'errore: e' una sessione ATTIVA che tiene il writer di un
    albero senza che nessuna finestra esista. Il lease resterebbe preso, e la prossima
    sessione verrebbe respinta per un conflitto con un fantasma.
    """

    WT = os.path.join(os.sep, "alberi", "spawn")

    def test_il_rollback_non_lascia_ne_sessione_ne_lease(self):
        with LocalDaemon() as d:
            cli = client()
            riserva = cli.call("terminal.reserve", shellType="powershell", session={
                "sessionId": "DEV-SPAWN", "role": "DEV", "lane": "MAIN",
                "workspaceGroup": "MAIN", "worktreePath": self.WT,
                "writeMode": "WRITER",
            })
            # Dopo `reserve` tutto esiste: sessione, lease e riga STARTING.
            self.assertEqual(riserva["terminal"]["state"], "STARTING")
            self.assertEqual(len(d.store.list_leases()), 1)

            cli.call("terminal.rollback", terminalId=riserva["terminal"]["terminal_id"],
                     reason="spawn fallito: prova")

            self.assertEqual(d.store.list_leases(), [], "nessun lease appeso")
            sessione = d.store.get_session("DEV-SPAWN")
            self.assertEqual(sessione["status"], "STOPPED", "nessuna sessione fantasma")
            self.assertEqual(cli.call("terminals.list"), [], "nessun terminale vivo")

    def test_dopo_un_rollback_lo_stesso_id_si_puo_riusare(self):
        """Se il rollback lasciasse rifiuti, il secondo tentativo fallirebbe."""
        with LocalDaemon() as d:
            cli = client()
            spec = {"sessionId": "DEV-SPAWN", "role": "DEV", "lane": "MAIN",
                    "workspaceGroup": "MAIN", "worktreePath": self.WT,
                    "writeMode": "WRITER"}
            primo = cli.call("terminal.reserve", shellType="powershell", session=spec)
            cli.call("terminal.rollback", terminalId=primo["terminal"]["terminal_id"],
                     reason="prova")
            secondo = cli.call("terminal.reserve", shellType="powershell", session=spec)
            self.assertEqual(secondo["terminal"]["state"], "STARTING")
            self.assertNotEqual(secondo["terminal"]["terminal_id"],
                                primo["terminal"]["terminal_id"])

    def test_il_writer_altrui_ferma_la_riserva_PRIMA_dello_spawn(self):
        """§7: `terminal launch` non aggira l'enforcement."""
        from rt3.errors import Rt3Error

        with LocalDaemon() as d:
            cli = client()
            start_session(cli, "DEV-1", "DEV", "MAIN", worktreePath=self.WT,
                          writeMode="WRITER")
            with self.assertRaises(Rt3Error) as ctx:
                cli.call("terminal.reserve", shellType="powershell", session={
                    "sessionId": "DEV-2", "role": "DEV", "lane": "MAIN",
                    "workspaceGroup": "MAIN", "worktreePath": self.WT,
                    "writeMode": "WRITER",
                })
            self.assertIn("ALREADY_OWNED", str(ctx.exception))
            self.assertEqual(cli.call("terminals.list"), [], "nessuna finestra riservata")


class TerminalStopSuLostTest(Rt3TestCase):
    """🔴 `terminal stop` deve funzionare anche quando la finestra è già sparita.

    Il caso reale, capitato nel pilot EPIC-1937: chiudi la finestra con la X, RT3 marca
    il terminale `LOST` — corretto, non riconosce più quel processo — e poi
    `terminal stop` risponde `TERMINAL_NOT_FOUND`, **lasciando la sessione ATTIVA e il
    writer lease preso**.

    Chi ci finisce dentro è bloccato senza una via ovvia: la finestra non c'è più, il
    comando che dovrebbe ripulire dice che non c'è nulla da ripulire, e l'albero resta
    occupato. La sessione è il soggetto di `terminal stop`, non la finestra: quella è
    ciò che si chiude *in più*, quando c'è ancora.
    """

    WT = os.path.join(os.sep, "alberi", "perso")

    def _riserva(self, cli, sid="DEV-LOST"):
        return cli.call("terminal.reserve", shellType="powershell", session={
            "sessionId": sid, "role": "DEV", "lane": "MAIN",
            "workspaceGroup": "MAIN", "worktreePath": self.WT, "writeMode": "WRITER",
        })

    def test_stop_su_terminale_LOST_ferma_comunque_la_sessione(self):
        with LocalDaemon() as d:
            cli = client()
            r = self._riserva(cli)
            tid = r["terminal"]["terminal_id"]
            # Il processo esiste e viene registrato, poi "muore": si simula marcando il
            # terminale LOST, che è esattamente ciò che `_terminal_view` fa da solo
            # quando l'identità non torna più.
            cli.call("terminal.attach", terminalId=tid, processId=os.getpid(),
                     processStartedAt="2000-01-01T00:00:00Z")   # istante che non torna
            self.assertEqual(len(d.store.list_leases()), 1, "il lease c'è")

            # 🔴 Il passo che rende il test fedele al caso reale. Il terminale
            # diventa LOST NEL DATABASE quando qualcuno lo osserva - `terminal list`
            # lo fa - e da quel momento non e' piu' fra i vivi. Senza questa riga il
            # test passava anche col difetto: la prima osservazione avveniva dentro
            # `close` stesso, che quindi lo trovava ancora.
            cli.call("terminals.list")
            self.assertEqual(d.store.get_terminal(tid)["state"], "LOST")

            esito = cli.call("terminal.close", sessionId="DEV-LOST")

            self.assertEqual(d.store.get_session("DEV-LOST")["status"], "STOPPED",
                             "la sessione deve fermarsi anche senza finestra")
            self.assertEqual(d.store.list_leases(), [], "il lease deve essere rilasciato")
            self.assertFalse(esito["closed"], "nessuna finestra è stata chiusa")
            self.assertIn("detail", esito)

    def test_stop_di_una_sessione_SENZA_terminale_resta_un_errore(self):
        """Controllo positivo: la tolleranza vale per un terminale che c'è stato, non
        per una sessione manuale — quella non ha mai avuto una finestra di RT3, e
        `terminal stop` su di lei è un comando sbagliato, non un caso da assorbire."""
        from rt3.errors import Rt3Error

        with LocalDaemon():
            cli = client()
            start_session(cli, "MANUALE", "DEV", "MAIN", worktreePath=self.WT,
                          writeMode="READ_ONLY")
            with self.assertRaises(Rt3Error) as ctx:
                cli.call("terminal.close", sessionId="MANUALE")
            self.assertIn("TERMINAL_NOT_FOUND", str(ctx.exception))

    def test_stop_ripetuto_non_esplode(self):
        """Chi non è sicuro di aver già ripulito deve poter richiamare il comando."""
        with LocalDaemon() as d:
            cli = client()
            tid = self._riserva(cli)["terminal"]["terminal_id"]
            cli.call("terminal.attach", terminalId=tid, processId=os.getpid(),
                     processStartedAt="2000-01-01T00:00:00Z")
            cli.call("terminal.close", sessionId="DEV-LOST")
            secondo = cli.call("terminal.close", sessionId="DEV-LOST")
            self.assertFalse(secondo["closed"])
            self.assertEqual(d.store.list_leases(), [])


class MessaggiCheSuggerisconoComandiTest(Rt3TestCase):
    """⛔ Un messaggio che suggerisce un comando inesistente è peggio di uno che non ne
    suggerisce nessuno: chi lo copia scopre l'errore dopo, e non sa se sbagliava il
    comando o lo stato che il comando descriveva.

    Successo davvero: `RT3_SESSION_EXISTS` proponeva `rt3 session stop --id <X>`, che il
    parser non accetta — la forma vera è `rt3 --session <X> session stop`.
    """

    def _comandi_suggeriti(self, testo):
        """Ogni `rt3 ...` fra backtick dentro il messaggio."""
        import re
        return [m.group(1) for m in re.finditer(r"`(rt3 [^`]+)`", testo)]

    def test_il_messaggio_di_roadmap_assente_suggerisce_un_comando_VERO(self):
        """Lo stesso difetto, un'altra occorrenza: proponeva `rt3 roadmap list`, ma il
        sottocomando e' al PLURALE. Due messaggi sbagliati nello stesso modo non sono
        una coincidenza: e' il motivo per cui questa classe esiste."""
        from rt3.cli import build_parser
        from rt3.errors import Rt3Error

        with LocalDaemon():
            cli = client()
            try:
                cli.call("roadmap.get", roadmapId="non-esiste")
                self.fail("una roadmap inesistente deve essere respinta")
            except Rt3Error as exc:
                testo = str(exc)

        suggeriti = self._comandi_suggeriti(testo)
        self.assertTrue(suggeriti, "il messaggio deve dire come trovarne una vera")
        parser = build_parser()
        for comando in suggeriti:
            with self.subTest(comando=comando):
                try:
                    parser.parse_args(comando.split()[1:])
                except SystemExit:
                    self.fail("il messaggio suggerisce `{}`, che il parser rifiuta"
                              .format(comando))

    def test_il_messaggio_di_sessione_gia_attiva_suggerisce_comandi_VERI(self):
        from rt3.cli import build_parser
        from rt3.errors import SessionExists

        with LocalDaemon():
            cli = client()
            start_session(cli, "DOPPIA", "DEV", "MAIN")
            try:
                start_session(cli, "DOPPIA", "DEV", "MAIN")
                self.fail("una sessione già attiva deve essere respinta")
            except Exception as exc:
                testo = str(exc)

        self.assertIn("RT3_SESSION_EXISTS", testo)
        suggeriti = self._comandi_suggeriti(testo)
        self.assertTrue(suggeriti, "il messaggio deve dire cosa fare")

        parser = build_parser()
        for comando in suggeriti:
            with self.subTest(comando=comando):
                argv = [a for a in comando.split()[1:] if not a.startswith("<")]
                argv = [a.replace("<id>", "X") for a in argv]
                try:
                    parser.parse_args(argv)
                except SystemExit:
                    self.fail("il messaggio suggerisce `{}`, che il parser rifiuta"
                              .format(comando))

if __name__ == "__main__":  # pragma: no cover
    unittest.main()
