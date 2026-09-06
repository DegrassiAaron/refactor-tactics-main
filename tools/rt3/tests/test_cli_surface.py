"""La superficie della CLI: che i comandi esistano, e con gli argomenti giusti.

Questo file esiste per una lacuna misurata. Il comando `roadmaps list` era stato scritto
nel posto sbagliato dell'albero dei sottocomandi, e a scoprirlo non e' stata la suite -
verde - ma lo smoke multi-workspace, cioe' il gate piu' lento e quello che richiede tre
checkout allineati. Un errore nella forma della CLI deve cadere qui, in un decimo di
secondo, non la' dopo dieci minuti.

⚠️ Non prova il COMPORTAMENTO dei comandi: quello sta negli altri file, che parlano con
un daemon vero. Qui si prova che il comando esista, che accetti gli argomenti che la
documentazione promette e che sia legato a una funzione. E' poco, ed e' esattamente il
poco che mancava.
"""

import unittest

from rt3.cli import build_parser
from rt3.model import CANDIDATE_STATUSES, ITEM_PROGRESS_STATES

#: Ogni riga e' una invocazione che DEVE essere accettata dal parser. Gli argomenti
#: obbligatori ci sono tutti: un comando che li perdesse fallirebbe qui.
ACCEPTED = [
    ["version"],
    ["status"],
    ["daemon", "start"],
    ["daemon", "status"],
    ["daemon", "stop"],
    ["daemon", "restart"],
    ["session", "start", "--id", "DEV-1", "--role", "DEV", "--lane", "DEV",
     "--workspace-group", "DEV"],
    ["session", "status"],
    ["session", "set", "--task", "2272"],
    ["session", "stop"],
    ["sessions", "list"],
    ["event", "publish", "--type", "TASK_READY"],
    ["events", "list"],
    ["inbox", "list"],
    ["inbox", "show", "ev_1"],
    ["inbox", "ack", "ev_1"],
    ["task", "show", "2272"],
    ["tasks", "list"],
    ["candidate", "create"],
    ["candidate", "create", "--roadmap", "r", "--item", "E/A"],
    ["candidate", "status", "cand_1", "--status", "PASSED"],
    ["candidates", "list"],
    # -- roadmap orchestration
    ["roadmaps", "list"],
    ["roadmap", "validate", "--file", "r.yaml"],
    ["roadmap", "load", "--file", "r.yaml"],
    ["roadmap", "load", "--file", "r.yaml", "--reset-state"],
    ["roadmap", "show"],
    ["roadmap", "graph"],
    ["roadmap", "critical-path"],
    ["roadmap", "ready"],
    ["roadmap", "ready", "--state", "READY"],
    ["roadmap", "plan"],
    ["roadmap", "state", "list"],
    ["roadmap", "state", "set", "EPIC-B/B2", "--progress", "VALIDATED"],
    ["roadmap", "state", "set", "B2", "--progress", "DONE", "--candidate", "cand_1"],
]

#: Invocazioni che devono essere RIFIUTATE. Un parser che le accettasse fallirebbe piu'
#: tardi e con un messaggio peggiore.
REJECTED = [
    ["roadmap", "list"],                     # l'elenco sta nel plurale top-level
    ["roadmap", "states"],                   # e' `roadmap state list`
    ["roadmap", "validate"],                 # --file e' obbligatorio
    ["roadmap", "state", "set", "B2"],       # --progress e' obbligatorio
    ["roadmap", "state", "set", "B2", "--progress", "READY"],   # READY e' derivato
    ["candidate", "status", "cand_1", "--status", "FORSE"],
    ["candidate", "status", "--status", "PASSED"],  # manca l'id
    ["session", "start", "--id", "X", "--role", "QA", "--lane", "DEV",
     "--workspace-group", "DEV"],            # ruolo inesistente
]


class SurfaceTest(unittest.TestCase):
    def setUp(self):
        self.parser = build_parser()

    def test_ogni_comando_documentato_esiste(self):
        for argv in ACCEPTED:
            with self.subTest(comando=" ".join(argv)):
                args = self.parser.parse_args(argv)
                self.assertTrue(
                    callable(getattr(args, "func", None)),
                    "`rt3 {}` non e' legato a nessuna funzione".format(" ".join(argv)),
                )

    def test_le_invocazioni_sbagliate_sono_rifiutate(self):
        for argv in REJECTED:
            with self.subTest(comando=" ".join(argv)):
                with self.assertRaises(SystemExit):
                    self.parser.parse_args(argv)

    def test_gli_elenchi_stanno_tutti_nel_plurale_top_level(self):
        """La convenzione della CLI, resa verificabile.

        Senza, l'eccezione si aggiunge da sola alla prossima entita' e chi usa lo
        strumento deve ricordare quale elenco sta dove.
        """
        for plural in ("sessions", "events", "tasks", "candidates", "roadmaps"):
            with self.subTest(entita=plural):
                args = self.parser.parse_args([plural, "list"])
                self.assertTrue(callable(getattr(args, "func", None)))

    def test_gli_stati_offerti_sono_quelli_del_modello(self):
        """Le scelte della CLI non si scrivono a mano due volte.

        Se `ITEM_PROGRESS_STATES` guadagnasse uno stato e la CLI no, il comando
        rifiuterebbe un valore legittimo - e il messaggio direbbe che e' invalido.
        """
        for state in ITEM_PROGRESS_STATES:
            with self.subTest(progress=state):
                self.parser.parse_args(
                    ["roadmap", "state", "set", "E/A", "--progress", state]
                )
        for status in CANDIDATE_STATUSES:
            with self.subTest(candidate=status):
                self.parser.parse_args(
                    ["candidate", "status", "cand_1", "--status", status]
                )


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
