#!/usr/bin/env python3
"""Test del modello di release del Feature Registry.

Uso:
    python scripts/test_feature_registry_releases.py       # dalla radice del repo
    python -m unittest discover -s scripts -p "test_*.py"

Pinna le tre proprieta' che il 2026-08-13 (D-136) hanno smesso di essere vere da sole quando
`RELEASE_ORDER` e' passato da `v0.4` a `v1.0`. Nessuna delle tre rompeva un gate: tutte e tre
producevano una vista **degradata in silenzio**, che e' il difetto che questo repository ha gia'
pagato piu' volte.

1. **Il parser dell'owner si fermava a `v0.\\d`.** La riga `v1.0` della tabella «Le release» non
   sarebbe stata letta, e le sue epic sarebbero risultate «senza release» — che e' il caso
   legittimo di `E37`, quindi non sospetto.
2. **`known_roadmap_refs()` leggeva solo `roadmap-v0.1.md`.** Scrivere `epic: E38` su una feature
   v0.2 era un errore del validator, quindi 30 feature restavano a `epic: null` **per
   impossibilita'**; otto compensavano con un paragrafo in `out_of_release_scope`.
3. **La tabella §2.2 della v0.1 mostrava feature di qualunque release.** `RT-FEAT-REPLAY-ARCHIVE`
   (`v0.2`, epic `E12`) compariva fra le feature della v0.1 senza che niente lo segnalasse.

I test 1-2 girano sul repository reale perche' la proprieta' e' *«l'owner e il codice concordano»*,
e su una fixture sintetica non direbbe nulla. Il test 3 usa dati costruiti a mano: serve un
disallineamento che il repository non deve avere.
"""
import os
import re
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import feature_registry as fr  # noqa: E402


class ReleaseOrderShape(unittest.TestCase):
    """Le invarianti strutturali della tupla, e il gate che le controlla."""

    def test_future_is_last(self):
        self.assertEqual(fr.RELEASE_ORDER[-1], "future")

    def test_no_duplicates(self):
        self.assertEqual(len(set(fr.RELEASE_ORDER)), len(fr.RELEASE_ORDER))

    def test_planned_releases_are_sorted(self):
        planned = [r for r in fr.RELEASE_ORDER if r != "future"]
        self.assertEqual(planned, sorted(planned))

    def test_reaches_v10(self):
        # Non e' decorazione: se qualcuno riduce la tupla, le feature gia' assegnate a v0.5-v1.0
        # diventerebbero «release non valida» e il rimedio ovvio sarebbe spostarle, non ripristinarla.
        self.assertIn("v1.0", fr.RELEASE_ORDER)

    def test_gate_accepts_the_real_repository(self):
        self.assertEqual(fr.check_release_order(), [])

    def test_gate_rejects_a_release_the_owner_does_not_declare(self):
        original = fr.RELEASE_ORDER
        try:
            fr.RELEASE_ORDER = original[:-1] + ("v9.9", "future")
            problems = fr.check_release_order()
            self.assertTrue(any("v9.9" in p for p in problems),
                            f"il gate non ha visto la release non dichiarata: {problems}")
        finally:
            fr.RELEASE_ORDER = original
        self.assertEqual(fr.check_release_order(), [], "ripristino non pulito")


class OwnerTableIsParsed(unittest.TestCase):
    """La tabella «Le release» di `roadmap-post-v0.1.md` e' letta per intero."""

    def setUp(self):
        self.epics = fr.post_v01_epics()
        self.by_release = {}
        for entry in self.epics:
            self.by_release.setdefault(entry["release"], []).append(entry["epic"])

    def test_every_planned_release_after_v01_has_at_least_one_epic(self):
        for release in fr.RELEASE_ORDER:
            if release in ("v0.1", "future"):
                continue
            self.assertIn(release, self.by_release,
                          f"nessuna epic assegnata a {release}: la riga della tabella owner "
                          f"non e' stata letta, oppure la release non ha contenuto")

    def test_v10_row_is_read(self):
        # Il caso che il regex `v0\.\d` avrebbe scartato senza dirlo.
        self.assertIn("v1.0", self.by_release)

    def test_post_v01_epics_are_assignable(self):
        known, _checkpoints, _milestones = fr.known_roadmap_refs()
        for entry in self.epics:
            self.assertIn(entry["epic"], known,
                          f"{entry['epic']} e' dichiarata dall'owner ma il validator la "
                          f"rifiuterebbe in `roadmap.epic`")


class ReleaseTableIsParsedExactly(unittest.TestCase):
    """La tabella «Le release» ha tante righe quante le release pianificate, e **nient'altro**.

    🔴 Il difetto che questi test pinnano non e' ipotetico: una tabella `| **vX.Y** | ... | ... |`
    aggiunta altrove nello stesso file produceva **tre coppie fantasma** perche' `[^|]*` include
    `\\n` e la cella andava a cercarsi oltre il ritorno a capo. `check_release_order()` contava le
    fantasma come «release dichiarate dall'owner» e taceva su `v0.2`, `v0.4` e `v0.9` — il gate di
    `D-136` reso muto da una tabella di prosa, senza che nulla diventasse rosso.

    Un'asserzione di sola presenza non l'avrebbe vista: le release c'erano, **due volte**. Serve il
    **conteggio**.
    """

    def test_one_row_per_planned_release_and_no_more(self):
        rows = fr.release_table_rows()
        planned = [r for r in fr.RELEASE_ORDER if r != "future"]
        self.assertEqual(len(rows), len(planned),
                         f"la tabella «Le release» produce {len(rows)} righe per {len(planned)} "
                         f"release: {[r[0] for r in rows]}")

    def test_no_row_has_an_empty_epic_cell(self):
        # Una cella epic vuota o di solo whitespace e' la firma della riga fantasma.
        for release, epic_cell in fr.release_table_rows():
            self.assertTrue(epic_cell.strip(),
                            f"{release} ha una cella epic vuota: il parser ha agganciato una riga "
                            f"che non appartiene alla tabella «Le release»")

    def test_rows_are_unique(self):
        seen = [r[0] for r in fr.release_table_rows()]
        self.assertEqual(len(seen), len(set(seen)),
                         f"release duplicate nella lettura della tabella: {seen}")


class PostV01CheckpointsAreAssignable(unittest.TestCase):
    """Il gemello del difetto sopra, e ne condivide la forma: `epics` aveva imparato a leggere
    l'owner post-v0.1, `checkpoints` no.

    `roadmap-post-v0.1.md` dichiara i propri checkpoint nella stessa forma tabellare della v0.1 —
    `| **23.6** | ... |` — ma `known_roadmap_refs()` li leggeva dal solo `roadmap-v0.1.md`. Una
    feature che li citasse riceveva «checkpoint inesistente», ed e' la ragione per cui le due feature
    di `E23` portavano `checkpoints: []` pur avendo `23.6` e `23.7` scritti nell'owner da `D-065`:
    per **impossibilita'**, non per scelta. Corretto da `D-138`.
    """

    def setUp(self):
        _epics, self.checkpoints, _milestones = fr.known_roadmap_refs()

    def test_e23_checkpoints_from_the_post_v01_owner_are_known(self):
        # I due che l'owner dichiara da D-065, piu' i tre del consolidamento di D-138.
        for cp in ("23.3", "23.4", "23.5", "23.6", "23.7"):
            self.assertIn(cp, self.checkpoints,
                          f"{cp} e' dichiarato da roadmap-post-v0.1.md ma il validator lo "
                          f"rifiuterebbe in `roadmap.checkpoints`")

    def test_v01_checkpoints_are_still_read(self):
        # La correzione aggiunge una sorgente, non la sostituisce.
        self.assertIn("9.3", self.checkpoints)

    def test_an_undeclared_checkpoint_is_still_rejected(self):
        # Il gate deve sapere ancora fallire: allargare la sorgente non e' accettare qualunque cosa.
        self.assertNotIn("23.9", self.checkpoints)
        self.assertNotIn("99.1", self.checkpoints)


class ReleaseViewIsFilteredByRelease(unittest.TestCase):
    """La tabella §2.2 e' la vista della v0.1, e una feature di un'altra release non ci entra."""

    @staticmethod
    def _feature(feature_id, release, epic):
        return {
            "feature_id": feature_id, "title": "t", "area": "A", "release": release,
            "status": "INTEGRATED", "gates_done": 1, "gates_applicable": 2,
            "roadmap": {"epic": epic}, "completed_by": [],
        }

    def test_v01_feature_on_a_v01_epic_is_listed(self):
        block = fr.render_features_by_epic(
            {"features": [self._feature("RT-FEAT-X-A", "v0.1", "E12")]})
        self.assertIn("RT-FEAT-X-A", block)
        self.assertNotIn("appartiene a un'altra release", block)

    def test_feature_whose_epic_is_another_release_goes_to_the_mismatch_table(self):
        # La mutazione e' esattamente `RT-FEAT-REPLAY-ARCHIVE`: release v0.2, epic E12 (v0.1).
        block = fr.render_features_by_epic(
            {"features": [self._feature("RT-FEAT-X-B", "v0.2", "E12")]})
        self.assertIn("appartiene a un'altra release", block,
                      "la contraddizione non e' stata dichiarata")
        head, _, tail = block.partition("appartiene a un'altra release")
        self.assertNotIn("RT-FEAT-X-B", head,
                         "la feature v0.2 compare comunque nella tabella della v0.1")
        self.assertIn("RT-FEAT-X-B", tail)

    def test_feature_in_a_planned_release_without_epic_is_reported(self):
        block = fr.render_features_by_epic(
            {"features": [self._feature("RT-FEAT-X-C", "v0.3", None)]})
        self.assertIn("RT-FEAT-X-C", block)
        self.assertIn("senza epic", block)

    def test_future_without_epic_is_not_reported(self):
        # In `future` un'epic non e' mancante: e' priva di senso, non c'e' release che la contenga.
        block = fr.render_features_by_epic(
            {"features": [self._feature("RT-FEAT-X-D", "future", None)]})
        self.assertNotIn("RT-FEAT-X-D", block)

    def test_feature_on_an_epic_without_release_does_not_vanish(self):
        """Il buco fra i rami: epic sconosciuta, feature post-v0.1, nessuna tabella.

        Trovato in code review. Un'epic senza riga nella tabella owner e' legittima — lo e' stata
        `E37` — ma una feature che la cita non deve sparire per questo: cadeva fuori da `by_epic`,
        da `mismatched` e da `unassigned` insieme, che e' l'omissione silenziosa che questa vista
        esiste per rendere visibile.
        """
        block = fr.render_features_by_epic(
            {"features": [self._feature("RT-FEAT-X-E", "v0.4", "E999")]})
        self.assertIn("RT-FEAT-X-E", block,
                      "la feature e' sparita da tutte e quattro le tabelle")
        self.assertIn("non ha una release", block)

    def test_post_v01_feature_on_its_own_epic_is_not_in_the_v01_table(self):
        # E26 e' v0.2: una feature v0.2 che la cita e' coerente, e non appartiene alla vista v0.1.
        block = fr.render_features_by_epic(
            {"features": [self._feature("RT-FEAT-X-F", "v0.2", "E26")]})
        self.assertNotIn("RT-FEAT-X-F", block)

    def test_the_no_epic_table_shows_the_release(self):
        """Senza la colonna Release, l'errore e la riga legittima si leggono identiche.

        Trovato in code review: la prosa della tabella fa dipendere la gravita' dalla release
        (`v0.1` e' un errore, il resto e' legittimo) mentre le colonne mostravano solo la milestone,
        `—` per tutte.
        """
        block = fr.render_features_by_epic({"features": [
            self._feature("RT-FEAT-X-G", "v0.1", None),
            self._feature("RT-FEAT-X-H", "v0.2", None),
        ]})
        rows = [l for l in block.splitlines() if "RT-FEAT-X-G" in l or "RT-FEAT-X-H" in l]
        self.assertEqual(len(rows), 2)
        self.assertNotEqual(rows[0].replace("X-G", ""), rows[1].replace("X-H", ""),
                            "le due righe sono indistinguibili: manca la release")
        self.assertTrue(any("v0.1" in r for r in rows))
        self.assertTrue(any("v0.2" in r for r in rows))


class EpicAndMilestoneAreComplementary(unittest.TestCase):
    """`epic` non deve cancellare `milestone`: sono due viste, non due alternative.

    Trovato in code review. `RT-FEAT-NET-AUTHORITY` dichiara `milestone: M10` **e** (dal 2026-08-13)
    `epic: E40`; con un `or`, `M10` spariva da ogni vista generata mentre `roadmap-post-v0.1.md`
    §E40 — scritto lo stesso giorno — dichiarava che M10 resta owner della vista di esecuzione.
    """

    def test_both_are_shown(self):
        ref = fr.roadmap_ref({"epic": "E40", "milestone": "M10", "checkpoints": []})
        self.assertIn("E40", ref)
        self.assertIn("M10", ref)

    def test_epic_alone_is_unchanged(self):
        self.assertEqual(fr.roadmap_ref({"epic": "E12", "checkpoints": []}), "E12")

    def test_milestone_alone_is_unchanged(self):
        self.assertEqual(fr.roadmap_ref({"milestone": "M10", "checkpoints": []}), "M10")

    def test_checkpoints_keep_the_milestone(self):
        ref = fr.roadmap_ref({"epic": "E40", "milestone": "M10", "checkpoints": ["40.1"]})
        self.assertIn("E40.1", ref)
        self.assertIn("M10", ref)


class ReleaseSortKey(unittest.TestCase):
    """`v0.10` viene dopo `v0.9`, che l'ordine lessicografico sbaglierebbe."""

    def test_two_digit_minor_sorts_after_single_digit(self):
        got = sorted(["v0.9", "v0.10", "v1.0", "v0.2"], key=fr.release_sort_key)
        self.assertEqual(got, ["v0.2", "v0.9", "v0.10", "v1.0"])

    def test_gate_accepts_a_tuple_with_two_digit_minor(self):
        # ⚠️ `v0.10` e non `v1.1`: con `v1.1` l'ordine lessicografico e quello semantico
        # coincidono, quindi il test passerebbe anche con il confronto sbagliato. La prima
        # stesura usava `v1.1` ed era cieca — trovato dalla verifica di mutazione.
        original = fr.RELEASE_ORDER
        try:
            planned = [r for r in original if r != "future"]
            planned.insert(planned.index("v1.0"), "v0.10")   # v0.9 < v0.10 < v1.0
            fr.RELEASE_ORDER = tuple(planned) + ("future",)
            self.assertNotEqual(planned, sorted(planned),
                                "la tupla di prova non discrimina i due ordinamenti")
            problems = [p for p in fr.check_release_order() if "ordine crescente" in p]
            self.assertEqual(problems, [],
                             "il gate rifiuta una tupla che e' in ordine crescente")
        finally:
            fr.RELEASE_ORDER = original


class GateFailsLoudly(unittest.TestCase):
    """Il gate deve parlare anche quando l'owner manca o dichiara piu' di quanto la tupla ammetta."""

    def test_missing_owner_is_an_error(self):
        original = fr.ROADMAP_POST_V01
        try:
            fr.ROADMAP_POST_V01 = os.path.join(os.path.dirname(original), "_non_esiste_.md")
            problems = fr.check_release_order()
            self.assertTrue(problems, "owner assente e gate verde: nessuna release e' descritta")
            self.assertTrue(any("non esiste" in p for p in problems))
        finally:
            fr.ROADMAP_POST_V01 = original
        self.assertEqual(fr.check_release_order(), [], "ripristino non pulito")

    def test_release_declared_by_the_owner_but_not_expressible_is_an_error(self):
        # Il difetto originale visto dall'altro lato: `v0.5` era descrivibile in prosa e non
        # scrivibile nel registry. Un gate a senso unico non lo avrebbe mai detto.
        original = fr.RELEASE_ORDER
        try:
            fr.RELEASE_ORDER = tuple(r for r in original if r != "v1.0")
            problems = fr.check_release_order()
            self.assertTrue(any("v1.0" in p and "non e' in RELEASE_ORDER" in p for p in problems),
                            f"il gate non vede la release dichiarata e inesprimibile: {problems}")
        finally:
            fr.RELEASE_ORDER = original


class ShortlistEpicTableIsTheV01View(unittest.TestCase):
    """§1 di `roadmap.shortlist.md` dichiara «stato da roadmap-v0.1.md §2.1»: contiene solo la v0.1.

    Trovato in code review: era il difetto gemello di quello corretto in `render_features_by_epic`,
    lasciato nel renderer accanto e innescato dal dato che lo stesso commit cambiava — 15 righe
    vuote nella tabella della v0.1 e un avviso che chiedeva a §2.1 righe per epic della v0.7.
    """

    # ⚠️ Le feature servono davvero. La prima stesura di questi test passava `{"features": []}`,
    # e con l'elenco vuoto `by_epic` resta vuoto: la tabella si riduce a `epic_status()`, che legge
    # `roadmap-v0.1.md` e quindi contiene solo epic della v0.1 **anche senza il filtro**. I test
    # erano verdi con la correzione e verdi senza — l'ha detto la verifica di mutazione, non la
    # lettura.
    @staticmethod
    def _data():
        def f(fid, release, epic):
            return {"feature_id": fid, "title": "t", "area": "A", "release": release,
                    "status": "INTEGRATED", "gates_done": 1, "gates_applicable": 2,
                    "roadmap": {"epic": epic}, "completed_by": []}
        return {"features": [
            f("RT-FEAT-Y-A", "v0.1", "E12"),
            f("RT-FEAT-Y-B", "v0.5", "E40"),
            f("RT-FEAT-Y-C", "v0.2", "E26"),
        ]}

    def test_post_v01_epics_are_absent(self):
        block = fr.render_shortlist_epics(self._data())
        for epic in ("E40", "E26"):
            self.assertNotIn(f"**{epic}**", block,
                             f"{epic} non e' della v0.1 e non va in questa tabella")

    def test_v01_epics_are_present(self):
        block = fr.render_shortlist_epics(self._data())
        self.assertIn("**E12**", block)

    def test_no_permanently_red_advisory(self):
        block = fr.render_shortlist_epics(self._data())
        self.assertNotIn("Senza stato dichiarato nell'owner", block,
                         "l'avviso chiede a §2.1 righe che §2.1 non puo' avere")


class ReleaseGatesReadTheirOwnAlphabet(unittest.TestCase):
    """Un gate che fallisce si legge `❌`, e la nota che cita un altro glifo non lo sposta.

    E' il gemello, sul reader dei gate, del difetto che #1249 ha tolto da `pie_entries()`:
    `release_gates()` cercava lo stato con l'alfabeto di **roadmap** — che dichiara avanzamento e
    non contiene `❌` — quindi una cella che apriva col rosso non conteneva nessun glifo
    riconosciuto, `state` restava `""` e il gate spariva dalla tabella come `—`. Un gate di release
    che fallisce e' esattamente l'informazione per cui quella vista esiste.

    Il secondo difetto stava a valle: `"✅" in state` cercava il verde **ovunque** nella cella, e
    la cella di stato di un gate porta note della stessa forma delle voci PIE — `🟡 …: il ✅ dello
    Shipping manca`. La regola e' la **posizione**, come per il registro PIE.
    """

    # La forma e' quella reale di `v0.1-definition-of-done.md` §3: quattro colonne, e la cella di
    # stato che porta glifo + data + nota. Le note citano un glifo diverso dal proprio stato perche'
    # e' la forma NORMALE di una nota onesta in questo documento, non un caso limite.
    FIXTURE = """## 3. Gate di release della v0.1

| # | Gate | Come si verifica | Stato |
|---|---|---|---|
| **G1** | Determinismo: 100 ripetizioni | `Replay.Verifier.ResimulationIsDeterministic` | ❌ **2026-08-21** (#999): checksum divergente al giro 7 — il ✅ del 2026-08-16 valeva su cinque ripetizioni |
| **G2** | Packaging Windows | `RunUAT BuildCookRun` | ✅ **2026-08-16** (#923): `BUILD SUCCESSFUL` |
| **G3** | Partita senza editor | avvio + partita verificati | 🟡 **2026-08-10**: fino alla vittoria sul pacchetto Development; il ✅ dello Shipping manca |
| **G4** | Suite automation completa verde | elenco test + esito | ⏳ |
| **G5** | Nessun intento avversario replicato | `Reactions.IntentNotVisibleToEnemy` | ❌ **2026-08-21** (#999): l'intento arriva al client avversario |
"""

    def setUp(self):
        self._original = fr.DOD_V01
        handle, self._path = tempfile.mkstemp(suffix=".md", text=True)
        os.close(handle)
        with open(self._path, "w", encoding="utf-8") as out:
            out.write(self.FIXTURE)
        fr.DOD_V01 = self._path

    def tearDown(self):
        fr.DOD_V01 = self._original
        os.unlink(self._path)

    def _glyphs(self):
        return {gate_id: glyph for gate_id, _req, _state, glyph in fr.release_gates()}

    def test_a_failed_gate_is_read_red_and_not_empty(self):
        # Il difetto originale: `state = ""` e la tabella stampava `—`, cioe' il gate USCIVA dal
        # conteggio invece di comparire fallito.
        self.assertEqual(self._glyphs()["G1"], "❌")

    def test_a_failed_gate_keeps_its_state_cell(self):
        cells = {gate_id: state for gate_id, _req, state, _glyph in fr.release_gates()}
        self.assertTrue(cells["G1"].startswith("❌"),
                        f"la cella di stato di G1 e' andata persa: {cells['G1']!r}")

    def test_a_note_citing_green_does_not_turn_the_gate_green(self):
        glyphs = self._glyphs()
        self.assertEqual(glyphs["G3"], "🟡")
        self.assertEqual(glyphs["G1"], "❌")

    def test_the_green_count_is_one_not_three(self):
        # Con la sottostringa sarebbero tre: G1, G2 e G3 contengono tutte un ✅.
        block = fr.render_shortlist_gates(None)
        self.assertIn("verdi: **1**", block,
                      f"il conteggio dei verdi non e' 1:\n{block}")

    def test_a_bare_red_gate_is_not_lost(self):
        """G5 e' rosso e la sua nota **non cita nessun altro glifo**: e' il caso che spariva.

        La distinzione conta, ed e' misurata sul codice di `main`: G1 e G5 sono entrambi falliti,
        ma G1 sopravviveva alla lettura — per il `✅` che la sua nota **cita** — e finiva contato
        *verde*, mentre G5 non conteneva nessun glifo dell'alfabeto di roadmap, quindi `state`
        restava `""` e la riga si stampava `—`. Due gate rossi, due errori opposti, stessa tabella.
        """
        self.assertEqual(self._glyphs()["G5"], "❌")

    def test_the_view_declares_the_failed_gates(self):
        block = fr.render_shortlist_gates(None)
        self.assertIn("2 ❌", block,
                      f"i gate falliti non sono dichiarati nella riga di intestazione:\n{block}")

    def test_no_gate_renders_as_a_dash(self):
        block = fr.render_shortlist_gates(None)
        rows = [line for line in block.splitlines() if line.startswith("| **G")]
        self.assertEqual(len(rows), 5)
        for row in rows:
            self.assertFalse(row.rstrip().endswith("| — |"),
                             f"un gate e' uscito dal conteggio invece di leggersi:\n{row}")

    def test_mutation_removing_red_from_the_alphabet_breaks_the_first_test(self):
        """Verifica di mutazione dichiarata dal DoD di #1251, eseguita qui e non a mano."""
        original = fr.RESULT_GLYPHS
        try:
            fr.RESULT_GLYPHS = original.replace("❌", "")
            self.assertNotEqual(self._glyphs().get("G1"), "❌",
                                "l'alfabeto mutato legge ancora il rosso: il test non discrimina")
        finally:
            fr.RESULT_GLYPHS = original
        self.assertEqual(self._glyphs()["G1"], "❌", "ripristino non pulito")


class EveryGateConsumerAgrees(unittest.TestCase):
    """Quattro lettori, **una** regola — e sul repository reale danno lo stesso numero.

    🔴 Non e' un'invariante teorica: il 2026-08-21, su `main`, i quattro consumatori di
    `release_gates()` applicavano quattro regole diverse allo stesso fatto, e due numeri
    **pubblicati** si contraddicevano.

    | consumatore | regola | verdi |
    |---|---|---|
    | `render_shortlist_gates` | `"✅" in state` (sottostringa) | 1 |
    | `render_control_center_page` (Wiki) | `state == "✅"` (uguaglianza) | **0** |
    | `docs/control-center/index.html` | `state.startsWith('✅')` | 1 |
    | `release_gates` | alfabeto senza `❌` | — |

    `milestonemap.shortlist.md` stampava «15 gate · verdi: **1**», la pagina Wiki
    `Stato-del-progetto.md` «**0** gate di release verdi su 15»: la cella di G12 e' `✅ **2026-08-16**
    (#923): …`, quindi non e' mai **uguale** a `"✅"`. Il glifo si calcola una volta sola e i lettori
    lo leggono; questo test e' il gate che tiene chiusa la porta.
    """

    @classmethod
    def setUpClass(cls):
        # Il percorso reale, non un payload sintetico: la proprieta' e' *«le viste pubblicate
        # concordano»*, e su dati costruiti a mano non direbbe nulla.
        registry = fr.load_registry()
        cls.data = fr.build_json(registry)
        cls.graph = fr.build_graph(registry)

    def setUp(self):
        self.gates = fr.release_gates()
        self.graph_gates = self.graph["release_gates"]

    def test_the_real_definition_of_done_is_readable(self):
        self.assertTrue(self.gates, "nessun gate letto da v0.1-definition-of-done.md §3")

    def test_no_gate_lost_its_glyph(self):
        senza = [gid for gid, _req, _state, glyph in self.gates if not glyph]
        self.assertEqual(senza, [], f"gate senza glifo riconosciuto: {senza}")

    def test_shortlist_and_wiki_publish_the_same_number(self):
        green = sum(1 for _gid, _req, _state, glyph in self.gates if glyph == "✅")
        shortlist = fr.render_shortlist_gates(None)
        self.assertIn(f"verdi: **{green}**", shortlist)
        wiki = fr.render_control_center_page(self.data, self.graph)
        self.assertIn(f"**{green} gate di release verdi su {len(self.gates)}**", wiki,
                      "la pagina Wiki pubblica un numero diverso dalla shortlist")

    def test_the_graph_carries_the_glyph_so_no_reader_re_derives_it(self):
        # Il JS del Control Center legge lo stesso campo invece di riapplicare una quarta regola.
        self.assertTrue(self.graph_gates, "il grafo non porta nessun gate")
        for entry in self.graph_gates:
            self.assertIn("glyph", entry)
        js = open(os.path.join(fr.REPO, "docs", "control-center", "index.html"),
                  encoding="utf-8").read()
        self.assertNotIn("g.state.startsWith('✅')", js,
                         "il Control Center deriva ancora il verde per conto proprio")
        self.assertIn("g.glyph === '✅'", js)


class ReleaseGatesParseTheirRowHonestly(unittest.TestCase):
    """Tre difetti del parser trovati in code review su PR #1253, verificati con una probe.

    Erano **preesistenti**, ma questa PR li rendeva peggiori invece di lasciarli fermi: dichiara
    «il glifo si calcola una volta sola», quindi da qui in poi una lettura sbagliata non resta in
    una vista — si propaga a tutte e quattro.
    """

    # ⚠️ Raw string: la cella di G18 contiene un backslash **letterale**, come nel documento vero.
    # Senza `r`, Python lo interpreta come sequenza di escape e la fixture smette di riprodurre il
    # caso che il test esiste per coprire.
    FIXTURE = r"""## 3. Gate di release della v0.1

| # | Gate | Come si verifica | Stato |
|---|---|---|---|
| **G16** | Cella di stato vuota | vedi il ✅ di G12, stesso comando |  |
| **G17** | Glifo fuori alfabeto | comando | ⬛ sconosciuto |
| **G18** | Nota che cita una pipe | comando | ❌ **2026-08-21**: `grep -c 'a \| b'` trova 3 occorrenze |
| **G19** | Verde vero | comando | ✅ **2026-08-16** (#923): `BUILD SUCCESSFUL` |
"""

    def setUp(self):
        self._original = fr.DOD_V01
        handle, self._path = tempfile.mkstemp(suffix=".md")
        with os.fdopen(handle, "w", encoding="utf-8") as out:
            out.write(self.FIXTURE)
        self.addCleanup(os.unlink, self._path)
        self.addCleanup(setattr, fr, "DOD_V01", self._original)
        fr.DOD_V01 = self._path

    def _gates(self):
        return {gid: (state, glyph) for gid, _req, state, glyph in fr.release_gates()}

    def test_an_empty_status_cell_is_not_green(self):
        """G16 non ha ancora un verdetto, e veniva contato **verde**.

        `cells = [c for c in cells if c]` buttava via la cella vuota, poi la scansione a ritroso
        ripiegava sulla cella dell'**evidenza** — che cita il `✅` di G12 — e la pubblicava come se
        fosse l'esito. La cella di stato e' l'ULTIMA: se e' vuota, l'informazione e' che manca.
        """
        state, glyph = self._gates()["G16"]
        self.assertEqual(glyph, "", f"un gate senza verdetto e' letto {glyph!r}")
        self.assertEqual(state, "", f"lo stato ha ripiegato sull'evidenza: {state!r}")

    def test_an_empty_status_cell_renders_as_a_dash_not_as_the_evidence(self):
        block = fr.render_shortlist_gates(None)
        row = next(l for l in block.splitlines() if l.startswith("| **G16**"))
        self.assertNotIn("vedi il ✅ di G12", row,
                         f"la colonna Stato pubblica l'evidenza come esito:\n{row}")
        self.assertTrue(row.rstrip().endswith("| — |"), row)

    def test_an_escaped_pipe_in_the_note_does_not_truncate_the_state(self):
        r"""Lo split sulla `|` grezza tagliava la nota a meta'.

        ⚠️ Non e' un caso di laboratorio: la cella di **G9** contiene gia' `\|` dentro un comando
        `grep`, e una nota rossa che cita un comando e' la forma normale. `\|` rende il markdown, ma
        non cambia il carattere su cui lo split lavora.
        """
        state, glyph = self._gates()["G18"]
        self.assertEqual(glyph, "❌")
        self.assertIn("trova 3 occorrenze", state,
                      f"la nota e' stata troncata sulla pipe escapata: {state!r}")

    def test_the_rendered_row_keeps_its_three_columns(self):
        block = fr.render_shortlist_gates(None)
        for line in block.splitlines():
            if not line.startswith("| **G"):
                continue
            # `| a | b | c |` -> cinque segmenti, coi due estremi vuoti.
            cols = re.split(r"(?<!\\)\|", line)
            self.assertEqual(len(cols), 5,
                             f"riga a {len(cols) - 2} colonne dentro una tabella a 3:\n{line}")

    def test_the_failed_count_is_rendered_by_the_shared_helper(self):
        """`failed_suffix()` esiste per tenere questa resa in **un** posto, e la sua docstring lo dice.

        La prima stesura di questa PR ne aveva scritta una terza copia a mano, in grassetto: gia'
        divergente dalle sedute (`· 2 ❌` contro `· **2 ❌**`) nel commit che dichiarava di renderle
        uguali. Il test asserisce la resa **condivisa**, non una sottostringa che le accetta entrambe.
        """
        block = fr.render_shortlist_gates(None)
        self.assertIn(f"verdi: **1**{fr.failed_suffix(1)}", block, block.splitlines()[2])


if __name__ == "__main__":
    unittest.main(verbosity=2)
