#!/usr/bin/env python3
"""Test degli esiti del registro PIE: quale glifo vince, e cosa ne fa lo stato della seduta.

Pinna la proprieta' che il 2026-08-20 non era vera e nessun gate lo diceva: **una voce PIE fallita
non appariva fallita in nessuna vista generata**. `STATUS_GLYPHS` — l'alfabeto delle tabelle di
roadmap — non contiene `❌`, e `pie_entries()` lo usava per leggere gli esiti: non trovando il rosso
la scansione tirava avanti e restituiva il primo glifo della **prosa**. Tre voci su 150:

- `PIE-HEX-VIZ-BLOCCHI` ❌ letta **✅** (e contata fra le verdi di U18, che dichiarava 11/15
  mentre i verdi erano 10 — la voce in piu' era il difetto #1246, aperto quel giorno);
- `PIE-HEX-MODE-H` ❌ letta **🟡**, che faceva sembrare `U1` a una voce dalla fine mentre aspetta
  un fix (#931/#996);
- `PIE-TEST-CONSOLE` ❌ letta **🟡**.

Nessun gate poteva vederlo: `generate --check` confronta il generato con la propria rigenerazione,
e la cecita' era identica dalle due parti. Il difetto e' #1249.

⚠️ **La fixture di `test_primo_glifo_vince` non e' una cella pulita, e non e' un vezzo.** Una riga
`| ❌ |` senza altri glifi NON falsifica il difetto: con l'alfabeto rotto quella cella non produce
alcun glifo, quindi `state` resta vuoto, quindi la voce non viene contata verde — e un'asserzione
«non e' verde» passerebbe con il bug ancora dentro. Il caso che rompe e' una cella che **apre** con
il rosso e cita un ✅ nella nota: e' la forma di tutte e tre le righe reali, nessuna esclusa.

Uso:
    python scripts/test_feature_registry_pie_states.py       # dalla radice del repo
    python -m unittest discover -s scripts -p "test_*.py"

VERIFICA DI MUTAZIONE (2026-08-21, una rottura per volta, da rifare se questo file cambia):

1. `RESULT_GLYPHS` senza `❌`      -> cadono `test_primo_glifo_vince` e
                                          `test_seduta_tutta_rossa_non_e_da_cominciare`;
2. `PIE_EXECUTED_GLYPHS` senza `❌`    -> cadono `test_seduta_tutta_rossa_non_e_da_cominciare` e
                                          `test_ogni_esito_diverso_da_aperto_conta_come_eseguito`;
3. `PIE_EXECUTED_GLYPHS` senza `⌫`    -> cade `test_ogni_esito_diverso_da_aperto_conta_come_eseguito`.

⚠️ Questa nota e' stata riscritta **due volte sulla misura**, e vale la pena dire perche'. Diceva
prima che la mutazione (1) abbatteva anche il test della seduta: falso allora, perche' quel test
costruisce gli stati a mano e non passava dal parser. Poi il codice e' cambiato — la condizione del
🟡 non enumera piu' i glifi, li **deriva** da `RESULT_GLYPHS` — e con la derivazione la (1) lo
abbatte davvero. Una nota di mutazione scritta a memoria invecchia al primo refactoring: si rilancia,
non si ricopia.
"""
import io
import os
import re
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import feature_registry as fr  # noqa: E402


# Le celle sono scritte nella forma del registro vero: esito, data, e una nota che RACCONTA il
# passaggio citando altri glifi. E' la forma che ha prodotto il difetto.
_RIGHE = [
    "| **PIE-FIXT-ROSSA** | cosa verifica | setup | criterio | "
    "❌ **2026-08-20** — la meta' «dall'alto» non regge. ✅ La meta' «due volumi» regge, e la voce "
    "era ⏳ fino a stamattina |",
    "| **PIE-FIXT-VERDE-CHE-CITA-ROSSO** | cosa verifica | setup | criterio | "
    "✅ **2026-08-20** — riallestita dopo che il ❌ del 2026-08-18 e' caduto |",
    "| **PIE-FIXT-PARZIALE** | cosa verifica | setup | criterio | "
    "🟡 prima meta' verificata, la seconda ⏳ |",
    "| **PIE-FIXT-APERTA** | cosa verifica | setup | criterio | ⏳ |",
]


class PieEntries(unittest.TestCase):
    def setUp(self):
        handle, self.path = tempfile.mkstemp(suffix=".md")
        with os.fdopen(handle, "w", encoding="utf-8") as out:
            out.write("# fixture\n\n" + "\n".join(_RIGHE) + "\n")
        self._original = fr.PIE_REGISTRY
        fr.PIE_REGISTRY = self.path

    def tearDown(self):
        fr.PIE_REGISTRY = self._original
        os.unlink(self.path)

    def test_primo_glifo_vince(self):
        """Il marcatore e' il primo glifo della cella, non il primo che l'alfabeto riconosce."""
        entries = fr.pie_entries()
        self.assertEqual(entries["PIE-FIXT-ROSSA"], "❌",
                         "una voce fallita la cui nota cita ✅ deve restare ❌: e' il difetto #1249")
        self.assertEqual(entries["PIE-FIXT-PARZIALE"], "🟡")
        self.assertEqual(entries["PIE-FIXT-APERTA"], "⏳")

    def test_una_voce_verde_che_cita_il_rosso_resta_verde(self):
        """Il simmetrico, e la ragione per cui `if "❌" in cella` sarebbe una riparazione peggiore.

        Nel registro vero sei celle contengono `❌` e solo tre lo sono: `PIE-HEX-VIZ-COSTO`,
        `PIE-HEX-VIZ-BORDI` e `PIE-V01-REACTCOND` lo citano nella propria nota. Cercare il glifo
        ovunque nella cella le trasformerebbe in tre falsi rossi.
        """
        self.assertEqual(fr.pie_entries()["PIE-FIXT-VERDE-CHE-CITA-ROSSO"], "✅")


class SessionState(unittest.TestCase):
    """Lo stato della seduta: ✅ solo se tutte verdi, ⏳ solo se nessuna eseguita, 🟡 il resto."""

    def _stato(self, *stati):
        session = {"id": "U0", "verifies": [f"V{i}" for i in range(len(stati))]}
        pie = {f"V{i}": s for i, s in enumerate(stati)}
        return fr.session_state(session, pie, tracked=set())

    def test_seduta_tutta_rossa_non_e_da_cominciare(self):
        """Il caso che il fix del solo alfabeto avrebbe rotto in direzione opposta.

        Prima di #1249 il glifo di una voce rossa era **quello che la sua nota citava per primo**,
        non un valore fisso: misurato sul registro con il vecchio alfabeto, `PIE-HEX-VIZ-BLOCCHI`
        risultava ✅ e `PIE-HEX-MODE-H` e `PIE-TEST-CONSOLE` 🟡. ⚠️ Questa docstring diceva «i rossi
        erano letti verdi»: vero per una riga su tre, e la generalizzazione avrebbe fatto cercare
        al prossimo lettore un difetto piu' semplice di quello che era. Una seduta di sole voci
        come `BLOCCHI` finiva in `DONE`; una di sole voci come `MODE-H` restava 🟡 in BLOCKING.

        Riparando il solo parser il caso «tutte ❌» sarebbe caduto nel `return "⏳"` finale, cioe'
        «mai cominciata»: la bugia opposta, e nessuna delle due dice che il lavoro e' stato fatto
        ed e' andato male.
        """
        self.assertEqual(self._stato("❌", "❌"), "🟡")

    def test_una_rossa_impedisce_il_verde(self):
        self.assertEqual(self._stato("✅", "✅", "❌"), "🟡")

    def test_ogni_esito_diverso_da_aperto_conta_come_eseguito(self):
        """`⌫` non e' ne' verde ne' rosso, e non deve ricadere in «mai cominciata».

        E' il difetto che la prima stesura di #1249 lasciava in piedi: riparato il rosso, la
        condizione del 🟡 enumerava ✅ 🟡 ❌ e `⌫` cadeva nel `return "⏳"` finale — «da fare» per un
        lavoro dichiarato **rimosso dal repo**. Il test gira su tutti i glifi dichiarati invece che
        su una lista scritta a mano: chi ne aggiunge uno a `RESULT_GLYPHS` senza pensare a
        `session_state()` lo vede fallire qui.
        """
        for glifo in sorted(set(fr.RESULT_GLYPHS) - {"⏳", "✅"}):
            self.assertEqual(self._stato(glifo, glifo), "🟡",
                             "una seduta di sole voci %s non e' «da cominciare»" % glifo)

    def test_tutte_verdi_resta_verde(self):
        self.assertEqual(self._stato("✅", "✅"), "✅")

    def test_nessuna_eseguita_resta_da_fare(self):
        self.assertEqual(self._stato("⏳", "⏳"), "⏳")


class SessionCounts(unittest.TestCase):
    """`N/M voci verdi · K ❌`, e il secondo termine solo quando esiste."""

    def _conta(self, *stati):
        session = {"id": "U0", "verifies": [f"V{i}" for i in range(len(stati))]}
        return fr.session_counts(session, {f"V{i}": s for i, s in enumerate(stati)})

    def test_le_rosse_escono_dai_verdi_e_si_contano_a_parte(self):
        self.assertEqual(self._conta("✅", "✅", "❌", "⏳"), ("2/4", 1))

    def test_senza_rosse_il_secondo_termine_e_zero(self):
        """La riga di una seduta pulita non deve cambiare forma: e' la decisione del 2026-08-21."""
        self.assertEqual(self._conta("✅", "⏳"), ("1/2", 0))

    def test_seduta_senza_voci(self):
        self.assertEqual(fr.session_counts({"id": "U0"}, {}), ("—", 0))


class RegistroReale(unittest.TestCase):
    """Gira sul repository, perche' la proprieta' e' «oggi le viste non mentono».

    ⚠️ **Asserisce la REGOLA, non i valori di oggi.** La prima stesura pinnava per nome i tre rossi
    (`BLOCCHI`, `MODE-H`, `TEST-CONSOLE`) e i tre verdi che li citano: un test cosi' diventa rosso
    quando #1246 chiude e `BLOCCHI` passa a ✅ — cioe' **quando il progetto va bene** — e il
    fallimento accuserebbe il parser mentre e' il dato ad essere cambiato. E' il difetto che questo
    repository conosce come *«un DoD con numeri letterali invecchia da solo»*, applicato a un test.

    La proprieta' vera e' che per **ogni** voce lo stato coincide col primo glifo della sua cella,
    qualunque esso sia oggi.
    """

    def _celle(self):
        """(id, ultima cella) letti dal registro, senza passare da `pie_entries()`."""
        celle = {}
        with io.open(fr.PIE_REGISTRY, encoding="utf-8") as fh:
            for line in fh:
                if not line.startswith("| **PIE-"):
                    continue
                cells = [c.strip() for c in line.strip().strip("|").split("|")]
                if len(cells) < 2:
                    continue
                m = re.match(r"\*\*(PIE-[A-Za-z0-9.\-]+)\*\*", cells[0])
                if m:
                    celle[m.group(1)] = cells[-1]
        return celle

    def test_ogni_voce_vale_il_primo_glifo_della_propria_cella(self):
        entries = fr.pie_entries()
        for voce, cella in self._celle().items():
            atteso = next((c for c in cella if c in fr.RESULT_GLYPHS), "")
            self.assertEqual(entries.get(voce), atteso,
                             "%s: la cella apre con %r" % (voce, atteso))

    def test_nessuna_voce_e_rossa_per_via_di_una_citazione(self):
        """Il simmetrico sul repository: una cella che CITA ❌ senza aprirci non e' rossa.

        Non nomina le voci: le trova. Oggi sono tre e domani possono essere altre, ma la relazione
        — cita il glifo, non lo dichiara — non cambia.
        """
        entries = fr.pie_entries()
        citanti = [v for v, cella in self._celle().items()
                   if "❌" in cella and not cella.lstrip().startswith("❌")]
        self.assertTrue(citanti, "nessuna cella cita ❌: la proprieta' non e' esercitata")
        for voce in citanti:
            self.assertNotEqual(entries.get(voce), "❌",
                                "%s cita ❌ nella nota ma non lo dichiara" % voce)

    def test_ogni_voce_ha_un_glifo(self):
        """Nessuna voce muta: una cella senza marcatore riconosciuto e' un esito che sparisce."""
        senza = sorted(k for k, v in fr.pie_entries().items() if not v)
        self.assertEqual(senza, [], "voci PIE senza glifo di stato riconosciuto")


if __name__ == "__main__":
    unittest.main(verbosity=2)
