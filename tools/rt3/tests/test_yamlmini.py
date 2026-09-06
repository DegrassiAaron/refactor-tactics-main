"""Il parser YAML del sottoinsieme: cosa legge, e soprattutto cosa RIFIUTA.

Un parser scritto in casa ha un modo caratteristico di fallire: non alza un errore,
legge una cosa diversa da quella scritta. Per questo la meta' di questi test verifica i
rifiuti, e per questo esiste il cross-check con PyYAML - quando c'e' - sugli stessi
documenti: due implementazioni indipendenti che concordano sono una prova che una sola
non puo' dare.
"""

import unittest

from rt3.yamlmini import YamlError, parse
from tests.harness import smoke_roadmap_path

try:  # pragma: no cover - dipende dalla macchina
    import yaml as _pyyaml
except ImportError:  # pragma: no cover
    _pyyaml = None


class SubsetTest(unittest.TestCase):
    def test_mapping_e_scalari(self):
        got = parse(
            "id: alfa\n"
            "numero: 12\n"
            "decimale: 1.5\n"
            "vero: true\n"
            "falso: false\n"
            "niente: null\n"
            "tilde: ~\n"
            "vuoto:\n"
        )
        self.assertEqual(
            got,
            {
                "id": "alfa",
                "numero": 12,
                "decimale": 1.5,
                "vero": True,
                "falso": False,
                "niente": None,
                "tilde": None,
                "vuoto": None,
            },
        )

    def test_lista_di_mapping_annidati(self):
        got = parse(
            "epics:\n"
            "  - id: E1\n"
            "    issues:\n"
            "      - id: A\n"
            "        estimate: 2\n"
            "      - id: B\n"
            "  - id: E2\n"
            "    issues:\n"
            "      - id: C\n"
        )
        self.assertEqual(
            got,
            {
                "epics": [
                    {"id": "E1", "issues": [{"id": "A", "estimate": 2}, {"id": "B"}]},
                    {"id": "E2", "issues": [{"id": "C"}]},
                ]
            },
        )

    def test_lista_allo_stesso_indent_della_chiave(self):
        """Forma legittima e comune: `- ` alla stessa colonna della chiave."""
        got = parse("requires:\n- uno\n- due\n")
        self.assertEqual(got, {"requires": ["uno", "due"]})

    def test_flow_inline(self):
        got = parse("a: [1, 2, tre]\nb: {x: 1, y: due}\nc: []\nd: {}\n")
        self.assertEqual(
            got, {"a": [1, 2, "tre"], "b": {"x": 1, "y": "due"}, "c": [], "d": {}}
        )

    def test_flow_annidato(self):
        got = parse("r: [{item: A, gate: VALIDATED}, {item: B}]\n")
        self.assertEqual(
            got, {"r": [{"item": "A", "gate": "VALIDATED"}, {"item": "B"}]}
        )

    def test_commento_in_coda_e_cancelletto_dentro_una_stringa(self):
        """⚠️ Un `split('#')` romperebbe la seconda riga, che e' la forma normale di
        citare una issue di RefactorTactics."""
        got = parse('a: 1   # commento\nnote: "vedi #2272"\ncolore: "#fff"\n')
        self.assertEqual(got, {"a": 1, "note": "vedi #2272", "colore": "#fff"})

    def test_stringa_quotata_conserva_i_due_punti(self):
        got = parse("t: 'a: b'\nu: \"c: d\"\n")
        self.assertEqual(got, {"t": "a: b", "u": "c: d"})

    def test_numero_quotato_resta_stringa(self):
        got = parse("versione: '1'\naltra: 1\n")
        self.assertEqual(got, {"versione": "1", "altra": 1})

    def test_due_punti_senza_spazio_non_e_una_coppia(self):
        got = parse("a: nome:valore\n")
        self.assertEqual(got, {"a": "nome:valore"})

    def test_documento_vuoto(self):
        self.assertIsNone(parse("# solo commenti\n\n"))


class RejectTest(unittest.TestCase):
    """Ogni costrutto fuori dal sottoinsieme alza, e nomina la riga.

    Il rifiuto e' la caratteristica di sicurezza del modulo: un'anchor ignorata in
    silenzio produrrebbe una roadmap valida e con dipendenze mancanti.
    """

    def _reject(self, source, needle):
        with self.assertRaises(YamlError) as ctx:
            parse(source)
        message = str(ctx.exception)
        self.assertIn(needle, message)
        self.assertTrue(
            message.startswith("riga "),
            "l'errore deve nominare la riga, trovato: {}".format(message),
        )
        return message

    def test_anchor(self):
        self._reject("base: &ancora\n", "anchor")

    def test_alias(self):
        self._reject("a: 1\nb: *ancora\n", "alias")

    def test_tag(self):
        self._reject("a: !!str 1\n", "tag")

    def test_block_scalar(self):
        self._reject("testo: |\n  riga\n", "block scalar")
        self._reject("testo: >\n  riga\n", "block scalar")

    def test_merge_key(self):
        self._reject("a: 1\n<<: *base\n", "merge key")

    def test_documento_multiplo(self):
        self._reject("---\na: 1\n", "documento")

    def test_tab_nell_indentazione(self):
        self._reject("a:\n\tb: 1\n", "TAB")

    def test_chiave_duplicata(self):
        """Una chiave ripetuta in YAML vero vince l'ultima, in silenzio. Qui no."""
        self._reject("id: uno\nid: due\n", "duplicata")

    def test_chiave_duplicata_nel_flow(self):
        self._reject("a: {x: 1, x: 2}\n", "duplicata")

    def test_flow_non_chiuso(self):
        self._reject("a: [1, 2\n", "non chiusa")

    def test_stringa_non_chiusa_nel_flow(self):
        self._reject("a: [\"uno, due]\n", "non chiusa")

    def test_indentazione_incoerente(self):
        self._reject("a:\n  b: 1\n   c: 2\n", "indentazione")

    def test_documento_che_comincia_indentato(self):
        with self.assertRaises(YamlError) as ctx:
            parse("  a: 1\n")
        self.assertIn("indentato", str(ctx.exception))


@unittest.skipIf(_pyyaml is None, "PyYAML non installato su questa macchina")
class CrossCheckTest(unittest.TestCase):
    """Due implementazioni indipendenti sullo stesso documento devono concordare.

    ⚠️ Il test SALTA dove PyYAML manca, e questo e' il suo limite: non e' un gate che
    protegge le macchine senza PyYAML. E' un controllo di correttezza che gira dove
    puo', ed e' il motivo per cui il pacchetto non DIPENDE da PyYAML - se ci dipendesse,
    non ci sarebbe niente da confrontare.
    """

    DOCUMENTS = [
        "id: alfa\nn: 12\nf: 1.5\nt: true\nz: null\n",
        "epics:\n  - id: E1\n    issues:\n      - id: A\n        estimate: 2\n",
        "a: [1, 2, tre]\nb: {x: 1, y: due}\n",
        "r:\n  - item: A\n    gate: VALIDATED\n  - item: B\n    gate: DONE\n",
        "note: \"vedi #2272\"\nq: 'a: b'\n",
        "vuoto:\nlista:\n- uno\n- due\n",
    ]

    def test_documenti_sintetici(self):
        for index, source in enumerate(self.DOCUMENTS):
            with self.subTest(documento=index):
                self.assertEqual(parse(source), _pyyaml.safe_load(source))

    def test_roadmap_di_smoke_reale(self):
        """Il documento vero, non uno inventato per il test."""
        path = smoke_roadmap_path()
        with open(path, "r", encoding="utf-8") as handle:
            source = handle.read()
        self.assertEqual(parse(source), _pyyaml.safe_load(source))


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
