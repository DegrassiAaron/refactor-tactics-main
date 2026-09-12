# tools/editor-sessions/seed_wiring_test.py
"""La semina deduce l'allestimento dalla prosa, e dichiara cio' che non sa dedurre."""
from __future__ import annotations

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

import yaml

import seed_wiring


class DeduzioneTest(unittest.TestCase):
    def test_riconosce_il_frontend(self):
        self.assertEqual(seed_wiring.deduci_setup("si parte da L_Frontend e si preme PLAY"), "SET-FRONTEND")

    def test_riconosce_lo_scenario(self):
        self.assertEqual(seed_wiring.deduci_setup("lancia rt.Test.Scenario Visual.Map.X"), "SET-SCEN")

    def test_riconosce_l_autobattle(self):
        self.assertEqual(
            seed_wiring.deduci_setup("partita con rt.Match.Autobattle=1 su L_HexArena"), "SET-HEX-BOT"
        )

    def test_il_piu_specifico_vince_sull_arena(self):
        self.assertEqual(
            seed_wiring.deduci_setup("apri L_GrayKitPlayground, poi torna su L_HexArena"), "SET-GRAYKIT"
        )

    def test_riconosce_l_arena_generata(self):
        self.assertEqual(
            seed_wiring.deduci_setup("il terreno lo fornisce `MapSource = GeneratedTestArena`"),
            "SET-GEN-ARENA",
        )

    def test_l_arena_generata_vince_sull_arena_versionata(self):
        self.assertEqual(
            seed_wiring.deduci_setup("GeneratedTestArena, e non L_HexArena"), "SET-GEN-ARENA"
        )

    def test_uno_scenario_esplicito_vince_sull_arena_generata(self):
        self.assertEqual(
            seed_wiring.deduci_setup("rt.Test.Scenario Visual.X su GeneratedTestArena"), "SET-SCEN"
        )

    def test_senza_indizi_non_inventa(self):
        self.assertIsNone(seed_wiring.deduci_setup("una seduta descritta senza nominare mappe"))


class SeminaTest(unittest.TestCase):
    def test_una_riga_per_check_col_setup_dedotto(self):
        sessioni = [
            {"id": "U49", "verifies": ["PIE-V01-SCREENHUD"], "issues": [613],
             "steps": "aprire L_Frontend e premere PLAY"},
        ]
        righe, orfani, _ = seed_wiring.semina(sessioni)
        self.assertEqual(orfani, {})
        self.assertEqual(
            righe,
            [{"check": "PIE-V01-SCREENHUD", "setup": "SET-FRONTEND", "issue": 613}],
        )

    def test_due_sedute_che_concordano_producono_una_riga_sola(self):
        sessioni = [
            {"id": "U901", "verifies": ["PIE-X"], "steps": "rt.Test.Scenario A"},
            {"id": "U902", "verifies": ["PIE-X"], "steps": "rt.Test.Scenario B"},
        ]
        righe, _, conflitti = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-X"])
        self.assertEqual(conflitti, {})

    def test_due_sedute_in_disaccordo_sono_un_conflitto_non_una_scelta(self):
        sessioni = [
            {"id": "U901", "verifies": ["PIE-X"], "steps": "GeneratedTestArena"},
            {"id": "U902", "verifies": ["PIE-X"], "steps": "rt.Test.Scenario B"},
        ]
        righe, _, conflitti = seed_wiring.semina(sessioni)
        self.assertEqual(righe, [])
        self.assertEqual(
            conflitti, {"PIE-X": {"U901": "SET-GEN-ARENA", "U902": "SET-SCEN"}}
        )

    def test_un_conflitto_risolto_a_mano_produce_la_riga_e_sparisce(self):
        sessioni = [
            {"id": "U901", "verifies": ["PIE-HEXPLAY-6"], "steps": "GeneratedTestArena"},
            {"id": "U902", "verifies": ["PIE-HEXPLAY-6"], "steps": "rt.Test.Scenario B"},
        ]
        righe, _, conflitti = seed_wiring.semina(sessioni)
        self.assertEqual(conflitti, {})
        self.assertEqual(righe[0]["setup"], "SET-SCEN")

    def test_cio_che_non_sa_dedurre_resta_raggruppato_per_seduta(self):
        # ID inventati di proposito: un ID reale finirebbe in ALLESTIMENTO_DICHIARATO e il
        # test misurerebbe i dati invece del comportamento.
        sessioni = [
            {"id": "U997", "verifies": ["PIE-MISTERO", "PIE-ALTRO"], "steps": "nessun indizio"},
            {"id": "U998", "verifies": ["PIE-TERZO"], "steps": "nemmeno qui"},
        ]
        righe, orfani, _ = seed_wiring.semina(sessioni)
        self.assertEqual(righe, [])
        self.assertEqual(orfani, {"U997": ["PIE-ALTRO", "PIE-MISTERO"], "U998": ["PIE-TERZO"]})

    def test_una_seduta_muta_non_produce_orfani_se_un_altra_cabla_lo_stesso_check(self):
        sessioni = [
            {"id": "U1", "verifies": ["PIE-X"], "steps": "L_HexArena"},
            {"id": "U17", "verifies": ["PIE-X"], "steps": "nessun indizio"},
        ]
        righe, orfani, _ = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-X"])
        self.assertEqual(orfani, {})

    def test_un_allestimento_dichiarato_copre_una_seduta_muta(self):
        sessioni = [{"id": "U14", "verifies": ["PIE-X"], "steps": "nessun indizio"}]
        righe, orfani, _ = seed_wiring.semina(sessioni)
        self.assertEqual(orfani, {})
        self.assertEqual(righe, [{"check": "PIE-X", "setup": "SET-HEX-MATCH"}])

    def test_il_dichiarato_vince_sull_euristica(self):
        sessioni = [{"id": "U4", "verifies": ["PIE-X"], "steps": "una partita su L_HexArena"}]
        righe, _, _ = seed_wiring.semina(sessioni)
        self.assertEqual(righe[0]["setup"], "SET-GEN-ARENA")

    def test_una_seduta_non_dichiarata_resta_orfana(self):
        sessioni = [{"id": "U999", "verifies": ["PIE-X"], "steps": "nessun indizio"}]
        _, orfani, _ = seed_wiring.semina(sessioni)
        self.assertEqual(orfani, {"U999": ["PIE-X"]})

    def test_l_uscita_e_ordinata_per_check(self):
        sessioni = [{"id": "U1", "verifies": ["PIE-Z", "PIE-A"], "steps": "L_HexArena"}]
        righe, _, _ = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-A", "PIE-Z"])


TESTA = """# Sedute — i MATTONI: allestimenti e cablaggio.
#
# ⛔ DRAFT — NON OWNER.

setups:
  - id: SET-A
    map: L_Uno
"""

VECCHIO = TESTA + """
wiring:
  # un commento che il seme riscrive
  - check: PIE-A
    setup: SET-A
    requires: [ asset:WBP_RT_PauseMenu ]
  - check: PIE-B
    setup: SET-A
    issue: 613
"""


class RequiresEsistentiTest(unittest.TestCase):
    def test_legge_i_requires_per_check(self):
        self.assertEqual(
            seed_wiring.requires_esistenti(VECCHIO),
            {"PIE-A": ["asset:WBP_RT_PauseMenu"]},
        )

    def test_un_registro_senza_requires_da_una_mappa_vuota(self):
        self.assertEqual(seed_wiring.requires_esistenti(TESTA + "\nwiring: []\n"), {})


class RendiRigheTest(unittest.TestCase):
    def test_lo_stile_e_quello_del_file_e_i_requires_stanno_in_flusso(self):
        reso = seed_wiring.rendi_righe(
            [
                {"check": "PIE-A", "setup": "SET-A", "requires": ["asset:X", "mount:X#7"]},
                {"check": "PIE-B", "setup": "SET-A", "issue": 613},
            ]
        )
        self.assertEqual(
            reso,
            "  - check: PIE-A\n"
            "    setup: SET-A\n"
            "    requires: [ asset:X, mount:X#7 ]\n"
            "  - check: PIE-B\n"
            "    setup: SET-A\n"
            "    issue: 613\n",
        )

    def test_cio_che_rende_si_rilegge(self):
        """La forma scritta deve tornare dentro `yaml.safe_load` uguale a com'e' uscita."""
        righe = [{"check": "PIE-A", "setup": "SET-A", "requires": ["asset:X", "mount:X#7"]}]
        riletto = yaml.safe_load("wiring:\n" + seed_wiring.rendi_righe(righe))
        self.assertEqual(riletto["wiring"], righe)


class InnestaTest(unittest.TestCase):
    def test_la_testa_si_conserva_byte_per_byte(self):
        nuovo = seed_wiring.innesta(VECCHIO, [{"check": "PIE-A", "setup": "SET-A"}])
        self.assertTrue(nuovo.startswith(TESTA))
        self.assertIn("DRAFT — NON OWNER", nuovo)

    def test_i_requires_esistenti_sopravvivono_al_seme(self):
        """Il seme non sa niente dei prerequisiti: se non li preservasse, li cancellerebbe."""
        nuovo = seed_wiring.innesta(
            VECCHIO,
            [{"check": "PIE-A", "setup": "SET-B"}, {"check": "PIE-B", "setup": "SET-A"}],
        )
        riletto = {r["check"]: r for r in yaml.safe_load(nuovo)["wiring"]}
        self.assertEqual(riletto["PIE-A"]["requires"], ["asset:WBP_RT_PauseMenu"])
        self.assertEqual(riletto["PIE-A"]["setup"], "SET-B")  # il seme aggiorna il setup
        self.assertNotIn("requires", riletto["PIE-B"])

    def test_un_requires_che_perde_la_propria_riga_ferma_tutto(self):
        """Il caso pericoloso: il seme non produce piu' la riga che portava il giudizio.

        Scriverla via sarebbe una perdita silenziosa di lavoro d'autore. Si
        rifiuta col nome del check, e chi legge decide.
        """
        with self.assertRaises(seed_wiring.InnestoError) as e:
            seed_wiring.innesta(VECCHIO, [{"check": "PIE-B", "setup": "SET-A"}])
        self.assertIn("PIE-A", str(e.exception))

    def test_e_idempotente(self):
        righe = [{"check": "PIE-A", "setup": "SET-A"}, {"check": "PIE-B", "setup": "SET-A"}]
        una = seed_wiring.innesta(VECCHIO, righe)
        due = seed_wiring.innesta(una, righe)
        self.assertEqual(una, due)

    def test_un_registro_senza_sezione_wiring_si_rifiuta(self):
        with self.assertRaises(seed_wiring.InnestoError):
            seed_wiring.innesta(TESTA, [{"check": "PIE-A", "setup": "SET-A"}])


class IntoSuFileTest(unittest.TestCase):
    def test_il_giro_su_file_non_mischia_i_fine_riga_e_conserva_la_testa(self):
        """Il percorso vero passa da `read_text`/`write_text`, non da `innesta` su
        stringhe: e' quello che nessun altro test attraversa.

        Si provano ENTRAMBE le convenzioni di fine riga, non una sola: la lettura
        e la scrittura che questo test vuole coprire traducono i fine riga in base
        a `os.linesep`, quindi il caso la cui convenzione GIA' COINCIDE con quella
        della macchina non e' probante da solo -- il codice pre-fix e quello
        corretto produrrebbero lo stesso file, e il test passerebbe anche contro
        il bug. E' l'altro caso, quello OPPOSTO a `os.linesep`, a fallire contro
        il bug: qui si esercitano entrambi, cosi' che su qualunque piattaforma
        almeno uno dei due sia l'opposto e protegga davvero.
        """
        for eol in ("\r\n", "\n"):
            with self.subTest(eol=repr(eol)):
                self._verifica_il_giro_su_file(eol)

    def _verifica_il_giro_su_file(self, eol):
        eol_bytes = eol.encode("ascii")
        with tempfile.TemporaryDirectory() as tmp:
            cartella = Path(tmp)
            sessioni = cartella / "vecchio.yaml"
            sessioni.write_text("sessions: []\n", encoding="utf-8")

            registro = cartella / "registro.yaml"
            testa = eol_bytes.join([
                b"# Registro di prova.",
                b"#",
                b"# DRAFT - NON OWNER.",
                b"",
                b"setups:",
                b"  - id: SET-A",
                b"    map: L_Uno",
                b"",
            ]) + eol_bytes
            corpo = eol_bytes.join([
                b"wiring:",
                b"  # vecchio commento che il seme riscrive",
            ]) + eol_bytes
            registro.write_bytes(testa + corpo)

            out = cartella / "out.yaml"
            argv_precedente = sys.argv
            sys.argv = [
                "seed_wiring.py",
                "--sessioni", str(sessioni),
                "--out", str(out),
                "--into", str(registro),
            ]
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    esito = seed_wiring.main()
            finally:
                sys.argv = argv_precedente

            self.assertEqual(esito, 0)
            dopo = registro.read_bytes()
            # "nessun fine-riga misto" dipende dalla convenzione in prova: per CRLF,
            # ogni \n deve avere il suo \r davanti; per LF, non deve comparire
            # nessun \r (altrimenti sarebbe un CRLF infilato in un file altrimenti LF).
            if eol == "\r\n":
                self.assertEqual(dopo.count(b"\r\n"), dopo.count(b"\n"))
            else:
                self.assertNotIn(b"\r", dopo)
            self.assertTrue(dopo.startswith(testa))


if __name__ == "__main__":
    unittest.main()
