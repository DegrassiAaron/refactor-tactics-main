"""Attivare una Epic: quali terminali servono, quali ci sono, e quando ne serve un altro.

Due proprieta' che questo file difende:

    REQUIRED non e' ACTIVE. Il planner puo' dire che serve un secondo DEV; finche'
    nessuno apre quel terminale la sessione non esiste. Marcarla attiva perche' e' stata
    suggerita renderebbe `epic check` una fotografia dei desideri.

    Il comando stampato deve ESISTERE. Un comando inventato e' peggio che nessun
    comando: chi lo copia scopre l'errore dopo, e non sa se sbagliava lui o il piano.
"""

import argparse
import unittest

from rt3.bootstrap import (
    additional_dev_required,
    writer_owner,
    check,
    command_for,
    render_additional_dev,
    render_check,
    render_plan,
    terminal_plan,
)
from rt3.cli import build_parser
from rt3.graph import build
from rt3.roadmap import normalize
from rt3.yamlmini import parse

SORGENTE = """
roadmapSchemaVersion: 1
id: boot
resources:
  workspaces:
    DEV:
      writerCapacity: 1
    DESIGNER:
      writerCapacity: 1
  temporaryWorktrees:
    capacity: 1
wip:
  perWorkspace: 4
  global: 8
epics:
  - id: EPIC-GRID
    homeWork: DEV
    issues:
      - id: B1
      - id: B3
        requires: [B1]
      - id: B4
        requires: [B1]
  - id: EPIC-ELEMENTS
    homeWork: DESIGNER
    issues:
      - id: C1
"""


def sessione(sid, ruolo, lane, stato="ACTIVE"):
    return {"sessionId": sid, "role": ruolo, "lane": lane, "workspaceGroup": lane,
            "worktreePath": r"D:/alberi/" + lane.lower(), "writeMode": "READ_ONLY",
            "status": stato}


class BootstrapTestCase(unittest.TestCase):
    def setUp(self):
        roadmap, problemi = normalize(parse(SORGENTE))
        self.assertIsNotNone(roadmap, [p.code for p in problemi])
        self.roadmap = roadmap
        self.graph = build(roadmap)

    def piano(self, states=None, modes=None, runtime=None, epic="EPIC-GRID"):
        return terminal_plan(self.roadmap, self.graph, states, modes,
                             runtime=runtime, epic_id=epic)

    def _dopo_b1(self):
        return {"EPIC-GRID/B1": "VALIDATED"}

    #: Stessa Epic, ma con spazio per DUE lavorazioni insieme: serve a distinguere il
    #: requisito di chi tiene gia' il writer da quello che vuole un temporaneo.
    #:
    #: I 4 spazi sono la chiave della sostituzione: `writerCapacity: 1` contiene
    #: "capacity: 1" come sottostringa, ma preceduta da "writer", non da spazi.
    SORGENTE_DUE = SORGENTE.replace("    capacity: 1", "    capacity: 2")

    RUNTIME_WRITER_OCCUPATO = {
        "writers": {
            "d:/alberi/dev": {
                "ownerSessionId": "DEV-GRID-LEAD",
                "resourceKey": "d:/alberi/dev",
                "workspaceGroup": "DEV",
                "stale": False,
            }
        },
        "unreal": {},
        "sessions": [sessione("DEV-GRID-LEAD", "DEV", "DEV")],
    }

    def piano_due(self, runtime):
        roadmap, problemi = normalize(parse(self.SORGENTE_DUE))
        self.assertIsNotNone(roadmap, [p.code for p in problemi])
        self.assertEqual(roadmap.temporary_worktree_capacity, 2, "la replace ha colpito")
        self.assertEqual(roadmap.writer_capacity("DEV"), 1, "e NON writerCapacity")
        return terminal_plan(roadmap, build(roadmap), self._dopo_b1(), None,
                             runtime=runtime, epic_id="EPIC-GRID")



class InitialPlanTest(BootstrapTestCase):
    """§27: una Epic con una sola issue READY vuole tre terminali, non quattro."""

    def test_tre_terminali_e_nessun_secondo_dev(self):
        p = self.piano()
        ruoli = [r["role"] for r in p["requirements"]]
        self.assertEqual(p["requiredNow"], 3, p["requirements"])
        self.assertEqual(sorted(ruoli), ["DEV", "EDITOR", "VALIDATION"])
        self.assertEqual(len([r for r in p["requirements"] if r["role"] == "DEV"]), 1)

    def test_i_nomi_sono_deterministici(self):
        """Due persone che leggono lo stesso piano devono aprire gli stessi id."""
        nomi = [r["session_id"] for r in self.piano()["requirements"]]
        self.assertEqual(nomi, self.piano()["requirements"] and nomi)
        self.assertIn("EDITOR-DEV", nomi)
        self.assertIn("VALIDATOR-GRID", nomi)
        self.assertTrue(any(n.startswith("DEV-GRID-") for n in nomi), nomi)

    def test_il_dev_prende_la_issue_READY(self):
        dev = [r for r in self.piano()["requirements"] if r["role"] == "DEV"][0]
        self.assertEqual(dev["issue"], "B1")
        self.assertEqual(dev["write_mode"], "WRITER")

    def test_senza_issue_pronte_non_servono_terminali(self):
        stati = {"EPIC-GRID/B1": "DONE", "EPIC-GRID/B3": "DONE", "EPIC-GRID/B4": "DONE"}
        p = self.piano(states=stati)
        self.assertEqual(p["requiredNow"], 0)
        self.assertTrue(p["notes"])

    def test_epic_inesistente_e_un_errore(self):
        from rt3.errors import ItemNotFound

        with self.assertRaises(ItemNotFound):
            self.piano(epic="EPIC-CHE-NON-C-E")


class CommandTest(BootstrapTestCase):
    """§14: il comando stampato deve essere accettato dalla CLI vera."""

    def _opzioni_di_session_start(self):
        """Le option string ESATTE che `rt3 session start` dichiara."""
        parser = build_parser()
        sub = [a for a in parser._actions if hasattr(a, "choices") and a.choices][0]
        start = sub.choices["session"]._actions
        sess_sub = [a for a in start if hasattr(a, "choices") and a.choices][0]
        return {
            s
            for azione in sess_sub.choices["start"]._actions
            for s in azione.option_strings
        }

    def test_ogni_comando_e_parsabile_dalla_CLI(self):
        parser = build_parser()
        for r in self.piano()["requirements"]:
            cmd = command_for(r)
            with self.subTest(cmd=cmd):
                self.assertTrue(cmd.startswith("rt3 session start"))
                args = parser.parse_args(cmd.split()[1:])
                self.assertTrue(callable(getattr(args, "func", None)))
                self.assertEqual(args.id, r["session_id"])
                self.assertEqual(args.role, r["role"])
                self.assertEqual(args.write_mode, r["write_mode"])

    def test_le_opzioni_sono_quelle_ESATTE_non_abbreviazioni(self):
        """🔴 `parse_args` da solo non basta: argparse accetta le abbreviazioni non
        ambigue, quindi un `--workspace` inventato passerebbe come `--workspace-group`.

        Misurato: mutando il comando per stampare `--workspace` l'intera suite restava
        verde. Un comando che funziona per abbreviazione e' documentazione sbagliata -
        chi lo legge impara un'opzione che non esiste.
        """
        dichiarate = self._opzioni_di_session_start()
        for r in self.piano()["requirements"]:
            for token in command_for(r).split():
                if token.startswith("--"):
                    with self.subTest(opzione=token):
                        self.assertIn(
                            token, dichiarate,
                            "{} non e' un'opzione dichiarata: {}".format(
                                token, sorted(dichiarate)
                            ),
                        )


class RequiredNotActiveTest(BootstrapTestCase):
    """§16: una sessione suggerita non e' una sessione aperta."""

    def test_senza_runtime_tutte_MISSING(self):
        for r in self.piano()["requirements"]:
            self.assertEqual(r["state"], "MISSING", r["session_id"])

    def test_diventa_ACTIVE_solo_quando_il_control_plane_la_vede(self):
        runtime = {"writers": {}, "unreal": {},
                   "sessions": [sessione("EDITOR-DEV", "EDITOR", "DEV")]}
        stati = {r["session_id"]: r["state"] for r in self.piano(runtime=runtime)["requirements"]}
        self.assertEqual(stati["EDITOR-DEV"], "ACTIVE")
        self.assertEqual(stati["VALIDATOR-GRID"], "MISSING")

    def test_una_sessione_FERMATA_non_conta_come_attiva(self):
        runtime = {"writers": {}, "unreal": {},
                   "sessions": [sessione("EDITOR-DEV", "EDITOR", "DEV", stato="STOPPED")]}
        stati = {r["session_id"]: r["state"] for r in self.piano(runtime=runtime)["requirements"]}
        self.assertEqual(stati["EDITOR-DEV"], "MISSING")


class EpicCheckTest(BootstrapTestCase):
    """§29: 0/3, 1/3, 2/3, 3/3 — e solo alla fine EXECUTION READY."""

    def _runtime(self, *ids):
        return {"writers": {}, "unreal": {},
                "sessions": [sessione(i, "DEV", "DEV") for i in ids]}

    def test_progressione(self):
        nomi = [r["session_id"] for r in self.piano()["requirements"]]
        attesi = [(0, False), (1, False), (2, False), (3, True)]
        for quanti, pronto in attesi:
            with self.subTest(attive=quanti):
                esito = check(self.piano(runtime=self._runtime(*nomi[:quanti])))
                self.assertEqual(esito["active"], quanti)
                self.assertEqual(esito["total"], 3)
                self.assertEqual(esito["ready"], pronto)
                testo = render_check(esito)
                if pronto:
                    self.assertIn("EPIC EXECUTION READY", testo)
                else:
                    self.assertIn("NOT READY", testo)
                    self.assertIn("{}/3".format(quanti), testo)

    def test_missing_elenca_chi_manca(self):
        nomi = [r["session_id"] for r in self.piano()["requirements"]]
        esito = check(self.piano(runtime=self._runtime(nomi[0])))
        self.assertEqual(sorted(esito["missing"]), sorted(nomi[1:]))


class DynamicSecondDevTest(BootstrapTestCase):
    """§17, §28: il secondo DEV compare quando serve, e NON prima."""

    def test_con_due_issue_pronte_serve_un_secondo_dev(self):
        p = self.piano(states=self._dopo_b1())
        dev = [r for r in p["requirements"] if r["role"] == "DEV"]
        self.assertEqual(len(dev), 2, [r["session_id"] for r in p["requirements"]])
        self.assertEqual([d["issue"] for d in dev], ["B3", "B4"])

    def test_il_secondo_dev_chiede_un_worktree_TEMPORANEO(self):
        p = self.piano(states=self._dopo_b1())
        extra = additional_dev_required(p)
        self.assertEqual(len(extra), 1)
        r = extra[0]
        self.assertEqual(r["issue"], "B4")
        self.assertEqual(r["reason"], "TEMPORARY_WORKTREE_REQUIRED")
        self.assertIn("temporaneo", r["detail"].lower())

    def test_il_secondo_dev_e_REQUIRED_non_ACTIVE(self):
        """🔴 La distinzione che tiene onesto il check."""
        extra = additional_dev_required(self.piano(states=self._dopo_b1()))[0]
        self.assertEqual(extra["state"], "MISSING")
        self.assertNotEqual(extra["state"], "ACTIVE")

    def test_il_messaggio_e_ACTION_REQUIRED_e_dice_di_creare_il_worktree(self):
        extra = additional_dev_required(self.piano(states=self._dopo_b1()))[0]
        testo = render_additional_dev(extra, occupato_da="DEV-GRID-1")
        for pezzo in ("[RT3 ACTION_REQUIRED]", "Additional DEV session required",
                      "Capability: GIT_WRITER", "occupied by DEV-GRID-1",
                      "TEMPORARY_WORKTREE_REQUIRED", "Manual worktree setup required."):
            self.assertIn(pezzo, testo)

    def test_chi_TIENE_il_writer_non_puo_chiedere_un_temporaneo(self):
        """Contraddizione trovata dallo smoke, non da un test: il piano stampava
        `DEV-B-1 [ACTIVE]` e insieme «richiede un worktree TEMPORANEO» - ma quella
        sessione E' la proprietaria del writer permanente.

        La causa: il planner vede il writer occupato e marca l'assegnazione TEMPORARY,
        non sapendo che a occuparlo e' la stessa sessione a cui la sta assegnando. Il
        dato che gli manca (`ownerSessionId`) e' pero' nello snapshot runtime, quindi
        qui non si indovina: si legge.
        """
        p = self.piano_due(self.RUNTIME_WRITER_OCCUPATO)
        dev = [r for r in p["requirements"] if r["role"] == "DEV"]
        self.assertEqual(len(dev), 2, [r["session_id"] for r in p["requirements"]])

        primo = dev[0]
        self.assertEqual(primo["session_id"], "DEV-GRID-LEAD",
                         "eredita l'id di chi tiene il writer, non il canonico")
        self.assertEqual(primo["state"], "ACTIVE", "la sessione esiste gia'")
        self.assertIsNone(
            primo["detail"],
            "chi tiene il writer permanente non puo' richiedere un temporaneo",
        )
        self.assertNotEqual(primo["reason"], "TEMPORARY_WORKTREE_REQUIRED")

    def test_il_temporaneo_dice_CHI_occupa_il_writer(self):
        """17: «occupied by another session» non dice a chi chiedere."""
        extra = additional_dev_required(self.piano_due(self.RUNTIME_WRITER_OCCUPATO))
        self.assertEqual(len(extra), 1)
        self.assertEqual(extra[0]["state"], "MISSING")
        self.assertIn("DEV-GRID-LEAD", extra[0]["detail"])

    def test_senza_writer_attivo_la_numerazione_resta_da_1(self):
        """Controllo positivo: senza proprietario nulla eredita nulla, altrimenti il
        test sopra sarebbe verde per una ragione diversa da quella scritta."""
        dev = [r for r in self.piano_due({"writers": {}, "unreal": {}, "sessions": []})
               ["requirements"] if r["role"] == "DEV"]
        self.assertEqual([d["session_id"] for d in dev], ["DEV-GRID-1", "DEV-GRID-2"])
        self.assertEqual([d["state"] for d in dev], ["MISSING", "MISSING"])

    def test_il_messaggio_FINALE_nomina_il_proprietario_non_another_session(self):
        """Il difetto gemello: `render_additional_dev` accetta `occupato_da`, ma chi lo
        chiamava non glielo passava, e il messaggio diceva «occupied by another
        session» - vero, e inutile: non dice a chi chiedere.

        Qui si verifica la CATENA (piano -> proprietario -> testo), non il renderer da
        solo: il renderer col parametro giusto era gia' verde mentre l'output reale era
        sbagliato.
        """
        piano = self.piano_due(self.RUNTIME_WRITER_OCCUPATO)
        req = additional_dev_required(piano)[0]
        self.assertEqual(writer_owner(piano, req), "DEV-GRID-LEAD")
        testo = render_additional_dev(req, occupato_da=writer_owner(piano, req))
        self.assertIn("occupied by DEV-GRID-LEAD", testo)
        self.assertNotIn("another session", testo)

    def test_senza_proprietario_il_piano_non_ne_inventa_uno(self):
        piano = self.piano_due({"writers": {}, "unreal": {}, "sessions": []})
        self.assertEqual(piano["writerOwners"], {})
        req = additional_dev_required(piano)[0]
        self.assertIsNone(writer_owner(piano, req))

    def test_prima_dell_avanzamento_NON_c_e_nessun_secondo_dev(self):
        """Controllo positivo: se comparisse sempre, il test sopra non proverebbe nulla."""
        self.assertEqual(additional_dev_required(self.piano()), [])

    def test_il_piano_indica_il_worktree_manuale_nel_render(self):
        testo = render_plan(self.piano(states=self._dopo_b1()))
        self.assertIn("Manual worktree setup required.", testo)


class LaneIsolationTest(BootstrapTestCase):
    """§30: il piano di una Epic non trascina dentro l'altra lane."""

    def test_il_piano_di_EPIC_GRID_non_nomina_DESIGNER(self):
        p = self.piano(epic="EPIC-GRID")
        lane = {r["lane"] for r in p["requirements"]}
        self.assertEqual(lane, {"DEV"})
        self.assertNotIn("C1", [r["issue"] for r in p["requirements"]])

    def test_il_piano_di_EPIC_ELEMENTS_riguarda_solo_DESIGNER(self):
        p = self.piano(epic="EPIC-ELEMENTS")
        self.assertEqual({r["lane"] for r in p["requirements"]}, {"DESIGNER"})
        self.assertIn("DESIGN-DEV-ELEMENTS-1",
                      [r["session_id"] for r in p["requirements"]])

    def test_senza_epic_il_piano_copre_tutta_la_roadmap(self):
        p = terminal_plan(self.roadmap, self.graph, None, None, epic_id=None)
        self.assertEqual({r["lane"] for r in p["requirements"]}, {"DEV", "DESIGNER"})


class CliOutputTest(BootstrapTestCase):
    """Quello che l'utente LEGGE davvero da `rt3 epic activate`.

    Serve perche' i test sui moduli puri non vedono la riga che li chiama. Il difetto
    misurato era esattamente li': `render_additional_dev` accettava `occupato_da`, i
    suoi test passavano il parametro, e la CLI no - quindi l'output reale diceva
    «occupied by another session» mentre la suite era verde.
    """

    class _ClientFinto(object):
        def __init__(self, piano):
            self.piano = piano
            self.chiamate = []

        def call(self, op, **kw):
            self.chiamate.append((op, kw))
            return self.piano

    def _esegui(self, piano):
        from rt3 import cli

        righe = []
        client = self._ClientFinto(piano)
        vecchio_client, vecchio_out = cli._client, cli.out
        cli._client = lambda args: client
        cli.out = lambda testo="": righe.append(testo)
        try:
            args = argparse.Namespace(id="boot", epic="EPIC-GRID", json_out=False,
                                      json=False)
            codice = cli.cmd_epic_activate(args)
        finally:
            cli._client, cli.out = vecchio_client, vecchio_out
        return codice, "\n".join(righe), client

    def test_activate_stampa_CHI_occupa_il_writer(self):
        piano = self.piano_due(self.RUNTIME_WRITER_OCCUPATO)
        codice, testo, client = self._esegui(piano)
        self.assertEqual(codice, 0)
        self.assertEqual(client.chiamate[0][0], "epic.activate")
        self.assertIn("[RT3 ACTION_REQUIRED]", testo)
        self.assertIn("occupied by DEV-GRID-LEAD", testo)
        self.assertNotIn(
            "occupied by another session", testo,
            "la CLI deve passare il proprietario, non lasciare il generico",
        )

    def test_activate_non_contraddice_chi_tiene_il_writer(self):
        """Il testo che ha rivelato il difetto nello smoke: una sessione ACTIVE a cui
        si chiedeva un worktree temporaneo."""
        piano = self.piano_due(self.RUNTIME_WRITER_OCCUPATO)
        _, testo, _ = self._esegui(piano)
        # Il blocco DI QUEL requisito, non il primo del piano: i DEV vengono dopo
        # EDITOR e VALIDATION, e tagliare sul numero sbagliato misura un altro testo.
        SEP = 2 * chr(10)   # separatore fra i blocchi di render_plan
        blocchi = [b for b in testo.split(SEP)
                   if "DEV-GRID-LEAD   [" in b]   # il requisito, non l'ACTION_REQUIRED
        self.assertEqual(len(blocchi), 1, testo)
        self.assertIn("DEV-GRID-LEAD   [ACTIVE]", blocchi[0])
        self.assertNotIn("TEMPORANEO", blocchi[0].upper())
        self.assertNotIn("Manual worktree setup required.", blocchi[0])


class AsciiOutputTest(BootstrapTestCase):
    """Ogni riga stampata deve sopravvivere a QUALUNQUE codepage della console.

    Difetto misurato: `render_plan` scriveva il titolo con un em dash. Su cp1252 usciva
    un byte illeggibile; su cp850 e cp437 - le codepage tipiche di `cmd.exe` - Python
    sollevava UnicodeEncodeError e il comando terminava con exit 1. Uno status che si
    stampa dopo ogni azione non puo' fallire per un carattere decorativo.
    """

    CODEPAGE = ("ascii", "cp437", "cp850", "cp1252")

    def _ascii(self, etichetta, testo):
        for enc in self.CODEPAGE:
            try:
                testo.encode(enc)
            except UnicodeEncodeError as e:
                self.fail("{} non e' stampabile in {}: {!r}".format(
                    etichetta, enc, testo[max(0, e.start - 20):e.end + 20]))

    def test_tutti_i_render_del_bootstrap_sono_stampabili(self):
        piano = self.piano_due(self.RUNTIME_WRITER_OCCUPATO)
        self._ascii("render_plan", render_plan(piano))
        self._ascii("render_check", render_check(check(piano)))
        for r in piano["requirements"]:
            self._ascii("command_for", command_for(r))
        for r in additional_dev_required(piano):
            self._ascii("render_additional_dev",
                        render_additional_dev(r, occupato_da=writer_owner(piano, r)))

    def test_anche_il_piano_VUOTO_e_stampabile(self):
        """Il ramo delle note, che ha un testo diverso."""
        piano = self.piano(states={}, epic="EPIC-ELEMENTS")
        self._ascii("render_plan/note", render_plan(piano))


class PlannerConsistencyTest(BootstrapTestCase):
    """Una sola verita': planner, bootstrap e UX devono dire la stessa cosa.

    🔴 Prima di questo gate, `bootstrap.py` conteneva logica che trasformava un
    `TEMPORARY_WORKTREE_SUGGESTED` del planner in un permanente della UX. Correggeva un
    difetto vero, ma nel posto sbagliato: due verita' su una sola decisione, e chi
    leggeva `roadmap plan` vedeva l'opposto di chi leggeva `epic activate`.

    Ora il planner decide e bootstrap rende. Questi test lo verificano invece di
    prometterlo.
    """

    def _confronta(self, states, runtime):
        from rt3.planner import plan as build_plan

        roadmap, problemi = normalize(parse(self.SORGENTE_DUE))
        self.assertIsNotNone(roadmap, [p.code for p in problemi])
        graph = build(roadmap)
        piano = build_plan(roadmap, graph, states, None, runtime=runtime)
        setup = terminal_plan(roadmap, graph, states, None, runtime=runtime,
                              epic_id="EPIC-GRID")
        return piano, setup

    def test_ogni_requisito_DEV_porta_la_modalita_DEL_PLANNER(self):
        piano, setup = self._confronta(self._dopo_b1(), self.RUNTIME_WRITER_OCCUPATO)
        # Solo l'Epic attivata: il piano copre tutta la roadmap, il setup una lane.
        dal_planner = {a["key"].split("/")[-1]: a["mode"]
                       for a in piano["assignments"] if a["key"].startswith("EPIC-GRID/")}
        self.assertTrue(dal_planner, "lo scenario deve produrre assegnazioni")
        dev = [r for r in setup["requirements"] if r["role"] == "DEV"]
        self.assertEqual(len(dev), len(dal_planner))
        for r in dev:
            self.assertEqual(r["resource_mode"], dal_planner[r["issue"]], r["session_id"])

    def test_e_anche_il_proprietario_e_quello_del_planner(self):
        piano, setup = self._confronta(self._dopo_b1(), self.RUNTIME_WRITER_OCCUPATO)
        owner = {a["key"].split("/")[-1]: a["ownerSessionId"]
                 for a in piano["assignments"] if a["key"].startswith("EPIC-GRID/")}
        for r in [x for x in setup["requirements"] if x["role"] == "DEV"]:
            atteso = owner[r["issue"]]
            if atteso:
                self.assertEqual(r["session_id"], atteso,
                                 "l'id lo porta il piano, non la convenzione")

    def test_chi_ha_il_permanente_non_chiede_MAI_un_temporaneo(self):
        """L'invariante che il difetto violava, ora espressa sui due lati insieme."""
        _, setup = self._confronta(self._dopo_b1(), self.RUNTIME_WRITER_OCCUPATO)
        for r in setup["requirements"]:
            if r.get("resource_mode") == "PERMANENT_WRITER":
                self.assertNotEqual(r["reason"], "TEMPORARY_WORKTREE_REQUIRED")
                self.assertIsNone(r["detail"])

    def test_la_coerenza_vale_anche_SENZA_runtime(self):
        piano, setup = self._confronta(self._dopo_b1(), None)
        dal_planner = {a["key"].split("/")[-1]: a["mode"]
                       for a in piano["assignments"] if a["key"].startswith("EPIC-GRID/")}
        for r in [x for x in setup["requirements"] if x["role"] == "DEV"]:
            self.assertEqual(r["resource_mode"], dal_planner[r["issue"]])

if __name__ == "__main__":  # pragma: no cover
    unittest.main()
