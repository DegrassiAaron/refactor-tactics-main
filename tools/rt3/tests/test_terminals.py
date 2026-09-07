"""Terminali gestiti: identita' di processo, tempo, e cio' che RT3 non deve chiudere.

Il test che regge questo file e' `PidReuseTest`. Tutto il resto e' comodita'; quello e'
sicurezza: un PID riusato che venisse scambiato per il nostro farebbe chiudere a RT3 il
programma di qualcun altro.
"""

import os
import unittest

from rt3.errors import TerminalAlreadyOpen, TerminalNotFound
from rt3.launcher import ps_quote, render_script, resolve_shell
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


class ShellTest(unittest.TestCase):
    def test_la_shell_si_risolve_in_modo_deterministico(self):
        nome, exe = resolve_shell()
        self.assertIn(nome, ("powershell", "pwsh"))
        self.assertTrue(os.path.exists(exe), exe)
        self.assertEqual(resolve_shell()[1], exe, "due chiamate, stessa risposta")


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


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
