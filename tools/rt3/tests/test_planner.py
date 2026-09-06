"""Readiness e piano: cosa e' pronto, chi lo esegue, e perche' qualcosa non parte.

Due proprieta' che questo file prova e che valgono piu' del resto:

    READY e BLOCKED sono DERIVATI. Nessun test scrive «READY» da qualche parte e poi lo
    rilegge: si dichiarano avanzamenti (VALIDATED) e si osserva la readiness cambiare.
    Un test che salvasse READY proverebbe la colonna, non la regola.

    Il piano e' DETERMINISTICO. Due workspace che chiedono un piano sullo stesso stato
    devono ottenere lo stesso piano, altrimenti due sessioni prendono due decisioni
    diverse credendo entrambe di seguire il planner.
"""

import unittest

from rt3.planner import normalize_states, plan, readiness, ready_keys, summary
from rt3.roadmap import normalize
from rt3.graph import build
from rt3.yamlmini import parse
from tests.harness import load_smoke_roadmap

A1, A2, A3 = "EPIC-A/A1", "EPIC-A/A2", "EPIC-A/A3"
B1, B2, B3, B4 = "EPIC-B/B1", "EPIC-B/B2", "EPIC-B/B3", "EPIC-B/B4"
C1, C2, C3 = "EPIC-C/C1", "EPIC-C/C2", "EPIC-C/C3"


class ReadinessTest(unittest.TestCase):
    def setUp(self):
        self.roadmap, self.graph = load_smoke_roadmap()

    def _states(self, states=None):
        return {k: r.state for k, r in readiness(self.roadmap, self.graph, states).items()}

    def test_stato_iniziale(self):
        """Senza alcuno stato dichiarato: pronte solo le tre radici."""
        states = self._states()
        self.assertEqual(
            {k: v for k, v in states.items() if v == "READY"},
            {A1: "READY", B1: "READY", C1: "READY"},
        )
        for key in (A2, A3, B2, B3, B4, C2, C3):
            self.assertEqual(states[key], "BLOCKED", key)

    def test_cross_epic_unblock(self):
        """🔴 Il test centrale: validare B2 sblocca C2, che sta in un ALTRO Epic."""
        prima = self._states({B1: "VALIDATED"})
        self.assertEqual(prima[C2], "BLOCKED")
        self.assertEqual(prima[B2], "READY")

        dopo = self._states({B1: "VALIDATED", B2: "VALIDATED"})
        self.assertEqual(dopo[C2], "READY")
        self.assertEqual(dopo[B3], "READY")
        self.assertEqual(dopo[B4], "READY")
        self.assertEqual(dopo[C3], "BLOCKED", "C3 aspetta C2, non B2")

    def test_una_issue_non_sblocca_chi_non_dipende_da_lei(self):
        """Controllo positivo al contrario: se validare B2 sbloccasse tutto, il test
        precedente sarebbe verde per la ragione sbagliata."""
        dopo = self._states({B1: "VALIDATED", B2: "VALIDATED"})
        self.assertEqual(dopo[A2], "BLOCKED")
        self.assertEqual(dopo[A3], "BLOCKED")

    def test_done_soddisfa_un_gate_validated(self):
        """Pretendere il contrario bloccherebbe per sempre chi dipende da lavoro finito."""
        self.assertEqual(self._states({A1: "DONE"})[A2], "READY")

    def test_validated_non_soddisfa_un_gate_done(self):
        roadmap, _ = normalize(
            parse(
                """
roadmapSchemaVersion: 1
id: gate
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
      - id: B
        requires:
          - item: A
            gate: DONE
"""
            )
        )
        graph = build(roadmap)
        stati = readiness(roadmap, graph, {"E/A": "VALIDATED"})
        self.assertEqual(stati["E/B"].state, "BLOCKED")
        self.assertEqual(readiness(roadmap, graph, {"E/A": "DONE"})["E/B"].state, "READY")

    def test_in_progress_non_e_ne_ready_ne_blocked(self):
        self.assertEqual(self._states({A1: "IN_PROGRESS"})[A1], "IN_PROGRESS")

    def test_una_dipendenza_iniziata_non_sblocca_chi_la_aspetta(self):
        """🔴 Cominciare non e' validare, e il gate esige la validazione.

        Questo test esiste per una lacuna misurata: mutando `_GATE_SATISFIED_BY` per
        far soddisfare un gate VALIDATED anche da IN_PROGRESS, l'intera suite restava
        verde tranne un asserto accessorio - perche' nessun test usava IN_PROGRESS
        come stato di una DIPENDENZA. Senza questa riga, il difetto piu' pericoloso
        del planner (dare per fatto il lavoro appena cominciato) passerebbe.
        """
        for stato_predecessore, atteso in (
            ("PENDING", "BLOCKED"),
            ("IN_PROGRESS", "BLOCKED"),
            ("VALIDATED", "READY"),
            ("DONE", "READY"),
        ):
            with self.subTest(predecessore=stato_predecessore):
                self.assertEqual(
                    self._states({A1: stato_predecessore})[A2], atteso
                )

    def test_una_dipendenza_iniziata_non_sblocca_nemmeno_cross_epic(self):
        """La stessa regola sul confine fra Epic, dove il difetto costerebbe di piu'."""
        self.assertEqual(self._states({B1: "VALIDATED", B2: "IN_PROGRESS"})[C2], "BLOCKED")

    def test_unmet_dice_chi_manca_e_con_che_stato(self):
        result = readiness(self.roadmap, self.graph, {B1: "IN_PROGRESS"})
        self.assertEqual(
            result[B2].unmet,
            [{"item": B1, "gate": "VALIDATED", "actual": "IN_PROGRESS"}],
        )

    def test_stato_non_valido_e_rifiutato(self):
        with self.assertRaises(ValueError) as ctx:
            readiness(self.roadmap, self.graph, {B1: "READY"})
        self.assertIn("READY", str(ctx.exception))

    def test_summary_conta_per_stato(self):
        s = summary(self.roadmap, self.graph)
        self.assertEqual(s["items"], 10)
        self.assertEqual(s["epics"], 3)
        self.assertEqual(s["byState"]["READY"], 3)
        self.assertEqual(s["byState"]["BLOCKED"], 7)


class CapacityTest(unittest.TestCase):
    def setUp(self):
        self.roadmap, self.graph = load_smoke_roadmap()

    def _plan(self, states):
        return plan(self.roadmap, self.graph, states)

    def _assignment(self, result, key):
        for a in result["assignments"]:
            if a["key"] == key:
                return a
        return None

    def _deferred(self, result, key):
        for d in result["deferred"]:
            if d["key"] == key:
                return d
        return None

    def test_writer_permanente_poi_worktree_temporaneo(self):
        """DEV ha un writer e due issue pronte: una parte, l'altra vuole un temporaneo."""
        result = self._plan({B1: "VALIDATED", B2: "VALIDATED"})
        b3 = self._assignment(result, B3)
        b4 = self._assignment(result, B4)
        self.assertIsNotNone(b3)
        self.assertIsNotNone(b4)
        self.assertEqual(b3["mode"], "PERMANENT_WRITER")
        self.assertEqual(b3["workspace"], "DEV")
        self.assertEqual(b4["mode"], "TEMPORARY_WORKTREE_SUGGESTED")
        self.assertEqual(b4["workspace"], "DEV")
        self.assertIn("NON creato", b4["reason"])

    def test_il_temporaneo_e_esaurito_alla_terza(self):
        """Con capacita' 1 il terzo pretendente su DEV viene rimandato, non inventato."""
        roadmap, problems = normalize(
            parse(
                """
roadmapSchemaVersion: 1
id: tre
resources:
  workspaces:
    DEV:
      writerCapacity: 1
  temporaryWorktrees:
    capacity: 1
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
      - id: B
      - id: C
"""
            )
        )
        self.assertIsNotNone(roadmap, [p.code for p in problems])
        result = plan(roadmap, build(roadmap), {})
        modes = {a["key"]: a["mode"] for a in result["assignments"]}
        self.assertEqual(modes["E/A"], "PERMANENT_WRITER")
        self.assertEqual(modes["E/B"], "TEMPORARY_WORKTREE_SUGGESTED")
        self.assertEqual(len(result["deferred"]), 1)
        self.assertEqual(result["deferred"][0]["key"], "E/C")
        self.assertEqual(
            result["deferred"][0]["reason"], "TEMPORARY_WORKTREE_CAPACITY"
        )

    def test_lavoro_gia_in_corso_occupa_il_writer(self):
        """Senza questo, il planner proporrebbe un writer dove qualcuno sta scrivendo."""
        result = self._plan({B1: "VALIDATED", B2: "VALIDATED", B3: "IN_PROGRESS"})
        b4 = self._assignment(result, B4)
        self.assertEqual(
            b4["mode"],
            "TEMPORARY_WORKTREE_SUGGESTED",
            "B3 e' IN_PROGRESS e tiene il writer permanente di DEV",
        )

    def test_due_attivita_unreal_non_sono_schedulate_insieme(self):
        """L'Editor e' esclusivo per macchina: A3 e C3 lo vogliono entrambe."""
        states = {
            A1: "VALIDATED",
            A2: "VALIDATED",
            B1: "VALIDATED",
            B2: "VALIDATED",
            C2: "VALIDATED",
            C1: "DONE",
            B3: "DONE",
            B4: "DONE",
        }
        result = self._plan(states)
        ready = ready_keys(self.roadmap, self.graph, states)
        self.assertIn(A3, ready)
        self.assertIn(C3, ready)

        unreal_assigned = [
            a for a in result["assignments"] if "UNREAL_EDITOR" in a["resources"]
        ]
        self.assertEqual(
            len(unreal_assigned),
            1,
            "con leaseCapacity 1 se ne pianifica una sola: {}".format(unreal_assigned),
        )
        self.assertEqual(result["capacity"]["unrealEditor"]["used"], 1)

        rimandata = [
            d for d in result["deferred"] if d["reason"] == "UNREAL_LEASE_CAPACITY"
        ]
        self.assertEqual(len(rimandata), 1)
        self.assertIn("esclusivo per macchina", rimandata[0]["detail"])

    def test_il_lease_unreal_non_consuma_un_writer_se_non_parte(self):
        """⚠️ Verificare il lease DOPO aver preso il writer lascerebbe un writer
        occupato da una issue che non parte, e rimanderebbe la successiva per una
        capacita' che in realta' e' libera."""
        roadmap, _ = normalize(
            parse(
                """
roadmapSchemaVersion: 1
id: lease
resources:
  workspaces:
    DEV:
      writerCapacity: 1
  unrealEditor:
    leaseCapacity: 0
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: UE
        resources: [UNREAL_EDITOR]
      - id: NORMALE
"""
            )
        )
        result = plan(roadmap, build(roadmap), {})
        assigned = {a["key"] for a in result["assignments"]}
        self.assertEqual(assigned, {"E/NORMALE"})
        self.assertEqual(result["deferred"][0]["reason"], "UNREAL_LEASE_CAPACITY")

    def test_wip_per_workspace(self):
        roadmap, _ = normalize(
            parse(
                """
roadmapSchemaVersion: 1
id: wip
resources:
  workspaces:
    DEV:
      writerCapacity: 3
  temporaryWorktrees:
    capacity: 3
wip:
  perWorkspace: 2
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
      - id: B
      - id: C
"""
            )
        )
        result = plan(roadmap, build(roadmap), {})
        self.assertEqual(len(result["assignments"]), 2)
        self.assertEqual(result["deferred"][0]["reason"], "WIP_PER_WORKSPACE")

    def test_wip_globale(self):
        result = self._plan({B1: "VALIDATED", B2: "VALIDATED"})
        self.assertLessEqual(result["wip"]["global"], self.roadmap.wip["global"])
        deferred = self._deferred(result, C1)
        self.assertIsNotNone(deferred)
        self.assertEqual(deferred["reason"], "WIP_GLOBAL")

    def test_wip_zero_significa_nessun_limite(self):
        roadmap, _ = normalize(
            parse(
                """
roadmapSchemaVersion: 1
id: nolimit
resources:
  workspaces:
    DEV:
      writerCapacity: 5
wip:
  perWorkspace: 0
  global: 0
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
      - id: B
      - id: C
"""
            )
        )
        result = plan(roadmap, build(roadmap), {})
        self.assertEqual(len(result["assignments"]), 3)


class ModeAndIdempotenceTest(unittest.TestCase):
    """🔴 D-7/D-8: il piano deve ricordare le proprie decisioni.

    Il difetto che questi test chiudono, misurato il 2026-09-06: il planner suggeriva
    `TEMPORARY_WORKTREE_SUGGESTED`, qualcuno eseguiva il suggerimento, e al giro
    successivo il planner rileggeva soltanto `IN_PROGRESS` - contando quell'item come
    writer PERMANENTE. Con `writerCapacity: 1` si arrivava a `used 2 / capacity 1`:
    il vincolo sforato eseguendo il piano che quel vincolo doveva rispettare, e nessuno
    che lo dicesse.
    """

    SOURCE = """
roadmapSchemaVersion: 1
id: modi
resources:
  workspaces:
    DEV:
      writerCapacity: 1
  temporaryWorktrees:
    capacity: 2
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
      - id: B
      - id: C
"""

    def setUp(self):
        roadmap, problems = normalize(parse(self.SOURCE))
        self.assertIsNotNone(roadmap, [p.code for p in problems])
        self.roadmap = roadmap
        self.graph = build(roadmap)

    def _occupazione(self, states, modes):
        """Risorse occupate dal lavoro GIA' in corso, prima di pianificare.

        ⚠️ Misura `_Capacity` e non l'esito di `plan()`: nel piano finito `used` somma
        l'occupazione preesistente e le assegnazioni appena decise, e i due addendi non
        si distinguono piu'. Un test che leggesse quel totale proverebbe qualcosa di
        diverso da quello che dice - ed e' l'errore che ho fatto scrivendolo.
        """
        from rt3.planner import _Capacity

        capacity = _Capacity(self.roadmap)
        capacity.occupy_existing(self.roadmap, normalize_states(self.roadmap, states), modes)
        return capacity

    def test_il_temporaneo_dichiarato_non_consuma_il_writer_permanente(self):
        c = self._occupazione(
            {"E/A": "IN_PROGRESS", "E/B": "IN_PROGRESS"}, {"E/B": "TEMPORARY_WORKTREE"}
        )
        self.assertEqual(c.writers["DEV"], 1, "solo A tiene il writer permanente")
        self.assertEqual(c.temporary, 1, "B sta su un worktree temporaneo")

    def test_senza_modalita_si_assume_il_permanente(self):
        """Conservativo: assumere il temporaneo libererebbe un writer forse occupato."""
        c = self._occupazione({"E/A": "IN_PROGRESS"}, {})
        self.assertEqual(c.writers["DEV"], 1)
        self.assertEqual(c.temporary, 0)

    def test_la_modalita_conta_solo_per_cio_che_e_in_corso(self):
        """Una issue VALIDATED non occupa nulla, qualunque modalita' porti scritta."""
        c = self._occupazione(
            {"E/A": "VALIDATED", "E/B": "DONE"},
            {"E/A": "TEMPORARY_WORKTREE", "E/B": "PERMANENT_WRITER"},
        )
        self.assertEqual(c.writers["DEV"], 0)
        self.assertEqual(c.temporary, 0)

    def test_eseguire_il_piano_non_sfora_la_capacita(self):
        """L'invariante di D-7, provato percorrendo il ciclo intero.

        Si prende il piano, si ESEGUE (ogni assegnazione diventa IN_PROGRESS con la
        modalita' che il planner ha scelto), e si ripianifica. Nessuna risorsa deve
        risultare usata piu' della propria capacita', a nessun giro.
        """
        states, modes = {}, {}
        for giro in range(4):
            result = plan(self.roadmap, self.graph, states, modes)
            with self.subTest(giro=giro):
                for group, v in result["capacity"]["writers"].items():
                    self.assertLessEqual(
                        v["used"], v["capacity"], "writer {} sforato: {}".format(group, v)
                    )
                for res in ("temporaryWorktrees", "unrealEditor"):
                    v = result["capacity"][res]
                    self.assertLessEqual(
                        v["used"], v["capacity"], "{} sforato: {}".format(res, v)
                    )
                self.assertEqual(
                    result["overCommitted"],
                    [],
                    "il piano non deve sforare per conto proprio: {}".format(
                        result["overCommitted"]
                    ),
                )
            if not result["assignments"]:
                break
            for a in result["assignments"]:
                states[a["key"]] = "IN_PROGRESS"
                modes[a["key"]] = (
                    "TEMPORARY_WORKTREE"
                    if a["mode"] == "TEMPORARY_WORKTREE_SUGGESTED"
                    else "PERMANENT_WRITER"
                )

    def test_il_ciclo_completo_prende_tutte_le_issue(self):
        """Controllo positivo: se il piano non sforasse perche' non assegna nulla,
        il test precedente sarebbe verde per la ragione sbagliata."""
        states, modes = {}, {}
        presi = set()
        for _ in range(5):
            result = plan(self.roadmap, self.graph, states, modes)
            if not result["assignments"]:
                break
            for a in result["assignments"]:
                presi.add(a["key"])
                states[a["key"]] = "IN_PROGRESS"
                modes[a["key"]] = (
                    "TEMPORARY_WORKTREE"
                    if a["mode"] == "TEMPORARY_WORKTREE_SUGGESTED"
                    else "PERMANENT_WRITER"
                )
        self.assertEqual(presi, {"E/A", "E/B", "E/C"})

    def test_uno_stato_che_sfora_viene_DICHIARATO_non_nascosto(self):
        """Due issue in corso sullo stesso gruppo, nessuna su un temporaneo.

        Il planner non lo ha prodotto - qualcuno le ha dichiarate a mano. Nasconderlo
        sarebbe peggio che esporlo: chi legge crederebbe che il vincolo regga.
        """
        result = plan(
            self.roadmap,
            self.graph,
            {"E/A": "IN_PROGRESS", "E/B": "IN_PROGRESS"},
            {},
        )
        over = result["overCommitted"]
        self.assertEqual(len(over), 1, over)
        self.assertEqual(over[0]["resource"], "writer:DEV")
        self.assertEqual((over[0]["used"], over[0]["capacity"]), (2, 1))
        self.assertIn("TEMPORARY_WORKTREE", over[0]["detail"])

    def test_over_committed_e_sempre_presente_anche_vuoto(self):
        """Un campo che compare solo nei guai costringe a distinguere assente da nessuno."""
        self.assertEqual(plan(self.roadmap, self.graph, {}, {})["overCommitted"], [])

    def test_troppi_temporanei_sono_dichiarati(self):
        result = plan(
            self.roadmap,
            self.graph,
            {"E/A": "IN_PROGRESS", "E/B": "IN_PROGRESS", "E/C": "IN_PROGRESS"},
            {
                "E/A": "TEMPORARY_WORKTREE",
                "E/B": "TEMPORARY_WORKTREE",
                "E/C": "TEMPORARY_WORKTREE",
            },
        )
        risorse = {o["resource"] for o in result["overCommitted"]}
        self.assertIn("temporaryWorktrees", risorse)

    def test_il_wip_conta_sul_gruppo_anche_per_il_temporaneo(self):
        """Un worktree temporaneo non e' un quarto workspace: e' un secondo posto dove
        lo stesso gruppo lavora, e il lavoro in corso resta suo."""
        c = self._occupazione({"E/A": "IN_PROGRESS"}, {"E/A": "TEMPORARY_WORKTREE"})
        self.assertEqual(c.wip_workspace["DEV"], 1)
        self.assertEqual(c.wip_global, 1)


class DeterminismTest(unittest.TestCase):
    def setUp(self):
        self.roadmap, self.graph = load_smoke_roadmap()

    def test_lo_stesso_stato_produce_lo_stesso_piano(self):
        states = {B1: "VALIDATED", B2: "VALIDATED"}
        piani = [plan(self.roadmap, self.graph, states) for _ in range(5)]
        for altro in piani[1:]:
            self.assertEqual(piani[0], altro)

    def test_l_ordine_delle_chiavi_di_stato_non_cambia_il_piano(self):
        """Uno stato costruito in ordine inverso e' lo stesso stato.

        Se il piano cambiasse, dipenderebbe dall'ordine di iterazione di un dizionario -
        cioe' due workspace otterrebbero piani diversi dallo stesso database.
        """
        diretto = {B1: "VALIDATED", B2: "VALIDATED", C1: "IN_PROGRESS"}
        inverso = dict(reversed(list(diretto.items())))
        self.assertEqual(
            plan(self.roadmap, self.graph, diretto),
            plan(self.roadmap, self.graph, inverso),
        )

    def test_il_cammino_critico_ha_precedenza_sulle_altre(self):
        """C2 sta sul cammino critico: fra le pronte, va assegnata per prima."""
        result = plan(self.roadmap, self.graph, {B1: "VALIDATED", B2: "VALIDATED"})
        self.assertEqual(result["assignments"][0]["key"], C2)


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
