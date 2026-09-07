"""Roadmap nel control plane: persistenza, avanzamento, candidate, restart.

Qui il grafo e il planner non sono piu' funzioni pure su un dizionario: passano per
HTTP, per SQLite e per un daemon che viene spento e riacceso. E' la differenza fra «la
regola e' giusta» e «la regola sopravvive alla chiusura del terminale che ospitava
rt3d», che e' la sola forma utile su questa macchina.
"""

import json
import unittest

from rt3.errors import ItemNotFound, RoadmapAmbiguous, RoadmapNotFound
from rt3.graph import load_checked
from rt3.roadmap import dumps
from tests.harness import LocalDaemon, Rt3TestCase, client, smoke_roadmap_path, start_session

A1, A2 = "EPIC-A/A1", "EPIC-A/A2"
B1, B2, B3, B4 = "EPIC-B/B1", "EPIC-B/B2", "EPIC-B/B3", "EPIC-B/B4"
C1, C2, C3 = "EPIC-C/C1", "EPIC-C/C2", "EPIC-C/C3"


def _load_into(cli, session_id=None, reset_state=False):
    """Carica la roadmap di smoke come farebbe `rt3 roadmap load`.

    Il PARSING resta lato client, come nella CLI vera: il daemon non legge il
    filesystem del checkout, e questo test percorre la stessa strada del comando.
    """
    roadmap, _graph, _problems = load_checked(smoke_roadmap_path())
    return cli.call(
        "roadmap.save",
        roadmapId=roadmap.id,
        name=roadmap.name,
        sourcePath=smoke_roadmap_path(),
        contentHash=roadmap.content_hash,
        roadmapSchemaVersion=roadmap.schema_version,
        document=dumps(roadmap),
        sessionId=session_id,
        resetState=reset_state,
    )


class RoadmapPersistenceTest(Rt3TestCase):
    def test_carica_e_rilegge(self):
        with LocalDaemon():
            cli = client()
            saved = _load_into(cli)
            self.assertEqual(saved["roadmap_id"], "rt3-smoke-multi-epic")
            self.assertTrue(saved["content_hash"].startswith("sha256:"))

            row = cli.call("roadmap.get")
            document = json.loads(row["document"])
            self.assertEqual(len(document["items"]), 10)
            self.assertEqual(len(document["epics"]), 3)

    def test_senza_roadmap_lo_dice_e_suggerisce_il_comando(self):
        with LocalDaemon():
            with self.assertRaises(RoadmapNotFound) as ctx:
                client().call("roadmap.ready")
            self.assertIn("roadmap load", str(ctx.exception))

    def test_due_roadmap_senza_id_non_ne_sceglie_una(self):
        """«La piu' recente» sembra ragionevole e cambia bersaglio da sola."""
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            cli.call(
                "roadmap.save",
                roadmapId="altra",
                name="altra",
                sourcePath=None,
                contentHash="sha256:0000000000000000",
                roadmapSchemaVersion=1,
                document=json.dumps(
                    {"id": "altra", "epics": [], "items": [], "resources": {}, "wip": {}}
                ),
            )
            with self.assertRaises(RoadmapAmbiguous):
                cli.call("roadmap.ready")
            # Con l'id esplicito funziona.
            self.assertEqual(
                cli.call("roadmap.ready", roadmapId="rt3-smoke-multi-epic")["roadmapId"],
                "rt3-smoke-multi-epic",
            )

    def test_ricaricare_non_azzera_lo_stato(self):
        """Buttare via cinque validazioni per un refuso sarebbe un danno silenzioso."""
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            cli.call("roadmap.setState", itemKey=B1, progress="VALIDATED")
            _load_into(cli)
            states = cli.call("roadmap.states")["states"]
            self.assertEqual([s["item_key"] for s in states], [B1])
            self.assertEqual(states[0]["progress"], "VALIDATED")

    def test_reset_state_e_esplicito(self):
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            cli.call("roadmap.setState", itemKey=B1, progress="VALIDATED")
            _load_into(cli, reset_state=True)
            self.assertEqual(cli.call("roadmap.states")["states"], [])


class ItemStateTest(Rt3TestCase):
    def test_avanzamento_e_readiness_derivata(self):
        with LocalDaemon():
            cli = client()
            _load_into(cli)

            def state_of(key):
                items = cli.call("roadmap.ready")["items"]
                return {i["key"]: i["state"] for i in items}[key]

            self.assertEqual(state_of(B2), "BLOCKED")
            cli.call("roadmap.setState", itemKey=B1, progress="VALIDATED")
            self.assertEqual(state_of(B2), "READY")
            self.assertEqual(state_of(C2), "BLOCKED")
            cli.call("roadmap.setState", itemKey=B2, progress="VALIDATED")
            self.assertEqual(state_of(C2), "READY")

    def test_chiave_inesistente_e_rifiutata(self):
        """Un refuso salvato non avrebbe effetto sul planner, e il comando riuscirebbe."""
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            with self.assertRaises(ItemNotFound) as ctx:
                cli.call("roadmap.setState", itemKey="EPIC-B/B22", progress="VALIDATED")
            self.assertIn("B22", str(ctx.exception))

    def test_forma_breve_accettata_quando_non_ambigua(self):
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            row = cli.call("roadmap.setState", itemKey="B1", progress="VALIDATED")
            self.assertEqual(row["item_key"], B1)

    def test_stato_non_valido_e_rifiutato(self):
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            with self.assertRaises(Exception) as ctx:
                cli.call("roadmap.setState", itemKey=B1, progress="READY")
            self.assertIn("READY", str(ctx.exception))

    def test_solo_gli_stati_dichiarati_sono_persistiti(self):
        """Scrivere PENDING per tutti al load fabbricherebbe stato che nessuno ha
        dichiarato."""
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            self.assertEqual(cli.call("roadmap.states")["states"], [])
            ready = cli.call("roadmap.ready")["items"]
            self.assertTrue(all(i["progress"] == "PENDING" for i in ready))


class PlanOverHttpTest(Rt3TestCase):
    def test_piano_calcolato_dal_daemon(self):
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            cli.call("roadmap.setState", itemKey=B1, progress="VALIDATED")
            cli.call("roadmap.setState", itemKey=B2, progress="VALIDATED")
            result = cli.call("roadmap.plan")
            modes = {a["key"]: a["mode"] for a in result["assignments"]}
            self.assertEqual(modes[B3], "PERMANENT_WRITER")
            self.assertEqual(modes[B4], "TEMPORARY_WORKTREE_SUGGESTED")

    def test_tre_client_diversi_vedono_lo_stesso_piano(self):
        """🔴 La proprieta' per cui esiste un control plane unico.

        Tre sessioni con ruolo, lane e workspace diversi chiedono il piano: la risposta
        deve essere identica, byte per byte. Se dipendesse da chi chiede, i tre
        workspace prenderebbero decisioni diverse.
        """
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            cli.call("roadmap.setState", itemKey=B1, progress="VALIDATED")
            cli.call("roadmap.setState", itemKey=B2, progress="VALIDATED")

            start_session(cli, "EDITOR-MAIN", "EDITOR", "MAIN", "MAIN")
            start_session(cli, "DEV-1", "DEV", "DEV", "DEV")
            start_session(cli, "VALIDATOR-DESIGNER", "VALIDATION", "DESIGNER", "DESIGNER")

            piani = [client().call("roadmap.plan") for _ in range(3)]
            for altro in piani[1:]:
                self.assertEqual(piani[0], altro)

    def test_critical_path_via_daemon(self):
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            result = cli.call("roadmap.criticalPath")
            self.assertEqual(result["duration"], 14)
            self.assertEqual(result["path"], [B1, B2, C2, C3])

    def test_graph_via_daemon(self):
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            g = cli.call("roadmap.graph")
            self.assertEqual(len(g["nodes"]), 10)
            unlocks = {n["key"]: n["unlocks"] for n in g["nodes"]}
            self.assertEqual(sorted(unlocks[B2]), [B3, B4, C2])


class CandidateLifecycleTest(Rt3TestCase):
    def test_due_candidate_sulla_stessa_issue_hanno_esiti_opposti(self):
        """⚠️ L'esito sta sul CANDIDATE, non sulla issue.

        Due candidate sullo stesso branch puntano a due commit diversi: una colonna
        sulla issue li conflaterebbe nell'ultimo che passa, e «il validator lavora sulla
        candidate» non sarebbe verificabile.
        """
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            start_session(cli, "DEV-1", "DEV", "DEV", "DEV", branch="feat/b1")

            primo = cli.call(
                "candidate.create",
                sessionId="DEV-1",
                branch="feat/b1",
                head="a" * 40,
                roadmapId="rt3-smoke-multi-epic",
                itemKey=B1,
            )
            secondo = cli.call(
                "candidate.create",
                sessionId="DEV-1",
                branch="feat/b1",
                head="b" * 40,
                roadmapId="rt3-smoke-multi-epic",
                itemKey=B1,
            )
            self.assertNotEqual(primo["candidate_id"], secondo["candidate_id"])
            self.assertEqual(primo["status"], "PENDING")

            start_session(cli, "VALIDATOR-DEV", "VALIDATION", "DEV", "DEV")
            fallito = cli.call(
                "candidate.setStatus",
                candidateId=primo["candidate_id"],
                status="FAILED",
                sessionId="VALIDATOR-DEV",
                note="regressione sul turno",
            )
            passato = cli.call(
                "candidate.setStatus",
                candidateId=secondo["candidate_id"],
                status="PASSED",
                sessionId="VALIDATOR-DEV",
            )
            self.assertEqual(fallito["status"], "FAILED")
            self.assertEqual(passato["status"], "PASSED")
            self.assertEqual(fallito["branch"], passato["branch"])
            self.assertNotEqual(fallito["head"], passato["head"])

            # Solo il candidate passato avanza la issue, e resta scritto QUALE.
            row = cli.call(
                "roadmap.setState",
                itemKey=B1,
                progress="VALIDATED",
                candidateId=secondo["candidate_id"],
            )
            self.assertEqual(row["candidate_id"], secondo["candidate_id"])

            stato_finale = cli.call("candidate.get", candidateId=primo["candidate_id"])
            self.assertEqual(
                stato_finale["status"],
                "FAILED",
                "il candidate fallito NON diventa valido perche' la issue e' avanzata",
            )

    def test_stato_candidate_non_valido_e_rifiutato(self):
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "DEV", "DEV")
            cand = cli.call("candidate.create", sessionId="DEV-1")
            with self.assertRaises(Exception) as ctx:
                cli.call(
                    "candidate.setStatus",
                    candidateId=cand["candidate_id"],
                    status="FORSE",
                )
            self.assertIn("FORSE", str(ctx.exception))


class RestartTest(Rt3TestCase):
    def test_roadmap_stati_e_candidate_sopravvivono_al_riavvio(self):
        """Il terminale che ospita rt3d viene chiuso: non deve perdersi niente."""
        with LocalDaemon():
            cli = client()
            _load_into(cli)
            start_session(cli, "DEV-1", "DEV", "DEV", "DEV")
            cli.call("roadmap.setState", itemKey=B1, progress="VALIDATED")
            cand = cli.call(
                "candidate.create",
                sessionId="DEV-1",
                roadmapId="rt3-smoke-multi-epic",
                itemKey=B1,
            )
            cli.call(
                "candidate.setStatus", candidateId=cand["candidate_id"], status="PASSED"
            )
            prima = cli.call("roadmap.plan")

        # rt3d spento. Nuovo daemon sullo stesso RT3_HOME.
        with LocalDaemon():
            cli = client()
            self.assertEqual(
                [r["roadmap_id"] for r in cli.call("roadmaps.list")],
                ["rt3-smoke-multi-epic"],
            )
            states = cli.call("roadmap.states")["states"]
            self.assertEqual(states[0]["item_key"], B1)
            self.assertEqual(states[0]["progress"], "VALIDATED")
            self.assertEqual(
                cli.call("candidate.get", candidateId=cand["candidate_id"])["status"],
                "PASSED",
            )
            self.assertEqual(cli.call("roadmap.plan"), prima)

    def test_lo_schema_migrato_resta_migrato(self):
        from rt3 import SCHEMA_VERSION

        with LocalDaemon():
            self.assertEqual(client().health()["dbSchemaVersion"], SCHEMA_VERSION)
        with LocalDaemon():
            self.assertEqual(client().health()["dbSchemaVersion"], SCHEMA_VERSION)


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
