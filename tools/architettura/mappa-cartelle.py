# -*- coding: utf-8 -*-
"""La mappa delle cartelle di `Source/`: dove va ogni file C++, con un tetto di file per cartella.

    Uso:  python tools/architettura/mappa-cartelle.py [--tetto N] [--json <file>] [--verifica]

Senza argomenti stampa, per ogni cartella di destinazione, quanti file C++ conterrebbe, e l'elenco dei test che
la regola automatica non sa collocare. `--json` scrive la mappa `vecchio -> nuovo` (percorsi relativi alla radice
del repository) che `sposta-cartelle.py` applica. `--verifica` esce 1 se una cartella supera il tetto.

## Perche' esiste

L'autore ha chiesto il 2026-11-11 cartelle da **al massimo 10-15 file** (la decisione e' registrata in #4061). A
quella data undici cartelle su ventisette la superavano, `Turn/` con 73 file e `Tests/` con 353. Questa mappa e' la
sorgente dello spostamento: una tabella esplicita per il codice di produzione, e una regola per i test, che
rispecchiano il codice che verificano.

## Le regole

* **Produzione: per unita'.** Un'unita' e' il nome di un file senza estensione (`RTTurnManager` = `.h` + `.cpp`), e
  si sposta intera: un `.h` e il suo `.cpp` non si separano mai. Le unita' non elencate restano dove sono.
* **Test: rispecchiano il codice.** Un file di test va nella cartella che rispecchia quella del codice che verifica:
  `Source/RefactorTactics/Tests/<cartella del codice>/`. Il soggetto si ricava dal **nome** (il prefisso di unita'
  piu' lungo: `RTHexSimTests` -> `RTHexSim`); se il nome non basta, dagli **include**; se nemmeno quelli, da
  `TEST_DIRETTI`, scritto a mano. Una cartella di test che supera il tetto si divide con `TEST_SOTTO`.
* ⛔ **Le cartelle di soli dati non contano** (`Tests/Golden`, `Tests/Fixtures`): sono corpus, non codice, e la
  regola dei file per cartella e' una regola di navigazione del codice.

## `--verifica`: il gate, e cosa NON copre

Esce 1 se una cartella di `Source/` supera il tetto, se un test sta nella radice di `Tests/` (dei due moduli), o se
un test non ha un soggetto. `--autotest` lo prova su alberi sintetici: e' la prova che sa fallire (D-188).

⚠️ **Cosa NON copre**, dichiarato perche' non venga scoperto dopo:
* **lo specchio dentro le sottocartelle**: un test messo nella sottocartella SBAGLIATA (`Tests/Map/` per un test del
  playback), o in una inventata (`Tests/Misc/`), passa finche' la cartella resta sotto il tetto. Il gate fa rispettare
  il tetto e la radice vuota, non la scelta del soggetto, che resta di chi scrive il test e di chi lo rivede;
* **i file che non sono `.h`/`.cpp`/`.inl`**: `.Build.cs`, JSON, dati;
* **la forma degli include** (radice del modulo, non relativi): nessun gate la misura;
* **la tabella `PRODUZIONE` invecchia**: un'unita' cancellata o rinominata produce un avviso, non un rosso.
"""

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

RADICE = Path(__file__).resolve().parents[2]
RT = "Source/RefactorTactics"
ED = "Source/RefactorTacticsEditor"
EST = (".h", ".cpp", ".inl")

# --- Produzione: unita' -> cartella nuova (relativa alla radice del modulo) ------------------------------------
PRODUZIONE = {
    RT: {
        # Turn/: l'orchestratore resta alla radice; il resto per responsabilita' (piano 2026-11-05 §4.7)
        "Turn/Actions": "RTActionEffectLibrary RTActionEvent RTActionFallbackLibrary RTActionQueue RTActionQueueLibrary "
                        "RTPlanValidationLibrary RTPredictiveLibrary RTDeclaredCondition RTIntentPrivacyLibrary",
        "Turn/Resolution": "RTBlastContext RTMovementResolutionContext RTMovementActionLibrary RTHexSim RTHexSimLibrary "
                           "RTFacingLibrary RTTurnRules RTResolvedEvent RTMoveRoute",
        "Turn/Reaction": "RTReactionLibrary RTReactionOpportunityTypes RTReactionPassResult RTReactionWindowView "
                         "RTReactionConditionConsole",
        "Turn/Log": "RTTurnLog RTTurnLogLibrary RTCombatLog RTMatchStateHash RTSeenUseLedger",
        "Turn/Pacing": "RTPacing RTPacingConsole RTPacingLibrary RTPacingRecorder",
        "Turn/Playback": "RTPlaybackLibrary RTPresentationBinding RTPlanPreview RTPlaybackTracer",
        "Turn/Format": "RTMatchFormatData RTMatchFormatLibrary RTMatchSetupLibrary",
        "Replay": "RTReplayRecording",
        # Map/: il modello dei dati resta alla radice
        "Map/Actor": "RTHexMapActor RTMapVisuals RTLayerPresentation RTHexLabel RTHexLabelLibrary RTOverlayArea "
                     "RTOverlayPalette RTHexOverlayConsole RTSightLines",
        "Map/Vision": "RTHexVisionLibrary RTHexCellVisibility RTHexOcclusionLibrary RTHexLosConsole "
                      "RTKnowledgeDebugConsole RTHexArcLibrary",
        "Map/Cover": "RTHexCoverLibrary RTHexCoverPlacementLibrary RTHexDoorLibrary RTHexLedgeLibrary",
        "Map/Structure": "RTStructuralBodyLibrary RTStructureIdentityLibrary RTGeometryBake RTGeometryGrammar "
                         "RTRegionBoundary",
        "Map/Authoring": "RTMapEditLibrary RTMapTemplateLibrary RTMapDependencyLibrary RTArenaCriteriaLibrary "
                         "RTArenaCriteriaConsole RTSpawnPoint",
        # UI/
        "UI/HUD": "RTHUD RTHudLayout RTHudViewModel RTHudZoneWidget RTScreenHudWidgets RTScreenHudConsole "
                  "RTGameViewportClient RTUIPalette",
        "UI/Feed": "RTActivationLine RTDamageTokenView RTPlayerEvent RTPlayerEventProjector RTSkillCalloutView "
                   "RTUnitOverlayWidget RTLayerIndicator",
        "UI/Inspect": "RTInspect RTInspectCardWidget RTGlance RTGlossary RTIconCatalogData RTIconLibrary",
        "UI/HeroProfile": "RTHeroProfileConsole RTHeroProfileLibrary RTHeroProfileView RTHeroProfileWidget "
                          "RTHeroRadarWidget",
        "UI/Reaction": "RTReactionResponseText RTReactionWindowViewModel",
        # ScenarioHarness/: sessione e runner alla radice
        "ScenarioHarness/Authoring": "RTScenarioLoader RTScenarioWriter RTScenarioDraft RTScenarioIndex "
                                     "RTScenarioAuthoring RTTestScenario",
        "ScenarioHarness/Report": "RTTestResult RTTestReportWriter RTTestConsole",
        # Replay/: archivio e registrazione alla radice
        "Replay/Viewer": "RTReplayViewerSubsystem RTReplayViewModel RTReplayScreenWidgets RTReplayPlayerLibrary "
                         "RTReplaySeekLibrary RTReplayStateLibrary RTPlaybackSpeed",
        # Ability/: i dati alla radice
        "Ability/Catalog": "RTCatalogLibrary RTHeroCatalogLibrary RTActionDescriptions RTActionReadout",
        "Ability/Lab": "RTAbilityLab RTHeroLab RTWorkbenchVariant",
        # Perception/: le regole alla radice, la presentazione del velo a parte
        "Perception/Veil": "RTKnowledgeVeilPresenter RTVeilTransition RTVisibilityBorder",
    },
    ED: {
        # Public/ e Private/ si dividono con le STESSE sottocartelle: una feature, due meta'.
        "HexMode": "RTHexEditorMode RTHexEditorModeCommands RTHexEditorModeSettings RTHexEditorModeToolkit "
                   "RTHexEditorClick RTHexHoverGate RTHexSelectionStore RTHexTransitionGlyph RTHexAnchorReadout "
                   "RTHexLosReadout RTHexProbeReadout RTHexWorkGrid RTHexWorkGridActor",
        "Launcher": "RTDevSandboxLauncherSubsystem RTLauncherScenarioBrowser RTLauncherWorkspace "
                    "SRTLauncherScenarioPanel RTScenarioPreviewActor RTScenarioPreviewSubsystem RTScenarioViewportModel "
                    "RTScenarioBoundaryConsole",
        "Lab": "RTLabCVarSnapshot RTLabPieLauncher RTLabViewModel SRTLabPanel RTPlaygroundLayout RTPlaygroundPanelLibrary",
        "Anim": "RTAnimBrowserModel SRTAnimBrowserPanel SRTAnimPreviewViewport RTAnimScan",
        "Content/Materials": "RTGrayboxMaterials RTHexVolumeMaterial RTMaterialRecipe RTUnitOutlineMaterial",
        "Content/Build": "RTBuildAnimBindingsCommandlet RTBuildGrayboxFixturesCommandlet RTBuildGrayboxMeshesCommandlet "
                         "RTBuildHexVolumeMaterialCommandlet RTBuildIconCatalogCommandlet RTBuildPlaygroundPanelCommandlet "
                         "RTBuildUnitOutlineMaterialCommandlet",
        "Content/Patch": "RTAddDamageTokenCommandlet RTRemoveEnergyBarCommandlet RTSetCellBodyFillCommandlet "
                         "RTSetCellDoorCommandlet RTSetObjectiveCellCommandlet RTSyncUnitAnimRefsCommandlet",
    },
}

# --- Test: le parole del dominio, in ordine (la prima che compare nello stem vince) -----------------------------
#
# 🔑 Vengono DOPO il nome esatto dell'unita' e PRIMA degli include. Gli include da soli sbagliano per costruzione:
# quasi ogni test include `RTActionData.h` o `RTHexMapAsset.h`, e il soggetto di un test di playback non e' il
# dato d'azione che allestisce. L'ordine conta: `ReactionWindow` prima di `Reaction`, `Hud` prima di `Plan`.
PAROLE = [
    (r"Replay", "Replay"),
    (r"Pacing", "Turn/Pacing"),
    (r"Playback|ActivationLine|SkillCallout|AttackTracer|StepFacing|Pivot|PresentationBinding|PlanPreview|FxProfile|"
     r"FxField|Cue|DefeatPresentation|AttackPlayback", "Turn/Playback"),
    (r"Hud|HUD|ActionBar|ActionSlot|ActionDock|ScreenHud|UIPalette|ControlHint|IdleBar|TargetPrompt|AimOrigin|"
     r"MatchWidget|IntentLabel|PhaseWord", "UI/HUD"),
    (r"ReactionWindow|ReactionResponse", "UI/Reaction"),
    (r"Inspect|Glance|Glossary|Icon|Tooltip", "UI/Inspect"),
    (r"HeroProfile|HeroRadar|HeroCard", "UI/HeroProfile"),
    (r"PlayerEvent|Feed|DamageToken|UnitOverlay|LayerIndicator", "UI/Feed"),
    (r"TurnLog|CombatLog|StateHash|Determinism|Golden|SeenUse|LogSubject|EventSequence|Ordering", "Turn/Log"),
    (r"Reaction|Overwatch|Brace|Guard|Intercept|Defensive|Sidestep|Anchor|DecisionBoundary|Predictive|Prediction",
     "Turn/Reaction"),
    (r"Format|MatchSetup|RoundLimit|MatchEnd|Outcome|Bootstrap|Objective", "Turn/Format"),
    (r"Bot", "Bot"),
    (r"Camera|Orbit|Zoom", "Camera"),
    (r"Frontend|MainMenu|Menu|ScreenStack|Startup", "Frontend"),
    (r"Pie", "PieSession"),
    (r"Veil|Knowledge|Perception|Blind|Noise|Acoustic|Transit|EnemyArc|VisibilityBorder|Recon", "Perception"),
    (r"Sight|Vision|Los|LOS|Occlusion|Visibility|Arc|Sector|Pointing", "Map/Vision"),
    (r"Cover|Ledge|Door|Edge|Corner", "Map/Cover"),
    (r"Structur|Geometry|Region|Rubble|Collapse|Volume", "Map/Structure"),
    (r"Overlay|Board|Label|Tint|SightLine|MapActor|MapVisual", "Map/Actor"),
    (r"Arena|Template|MapEdit|Spawn|Graybox|Fixture|Yard|Criteria|Dependency", "Map/Authoring"),
    (r"HexMap|Layer|HexTests|HexCell|CellId|Occupancy|Standab", "Map"),
    (r"Terrain|Fire|Water|Electric|Environment|Hazard|Surface|Lava|Ice", "Terrain"),
    (r"Path|Pathfinding|Route|Reach", "Pathfinding"),
    (r"Combat|Damage|Mitigation|Exposed|Defense|Hit|Range|Attack|Offensive|Blast|Heal|Shield|Stun|Prone|Status|Pay",
     "Combat"),
    (r"Workbench|Lab|Variant", "Ability/Lab"),
    (r"Catalog|Description|Readout|Kit", "Ability/Catalog"),
    (r"Hero|Ability|Action|Equipment|MovementProfile|Mirror|Point", "Ability"),
    (r"Anim|Graykit|Pose|Unit|Ghost|Outline|Death|Cleanup", "Unit"),
    (r"Scenario|Showcase|Harness|Session|Runner|Authoring", "ScenarioHarness"),
    (r"Player|Pointer|Input|Click|Select|Focus|Planning|Plan|Autobattle|Interaction", "Player"),
    (r"Move|Movement|Dash|Charge|Push|Knock|Displacement|Fall|Facing|Withdraw|Sprint|MicroStep|Stall|Collision|"
     r"Swap|Contest", "Turn/Resolution"),
    (r"Match|Turn|Phase|Round|Lock|Resolution|Simultaneous|Stress|Perf|Incidence|Order", "Turn"),
]

# Gli header che quasi tutti i test includono: contarli direbbe "il test usa una mappa", non cosa verifica.
CROCEVIA = set("""RTTurnManager RTUnit RTGameMode RefactorTactics RTActionData RTActionDef RTHexMapAsset RTHexCellData
    RTCellId RTHexLibrary RTHeroData RTCatalogLibrary RTHeroCatalogLibrary RTHexMapActor RTTurnRules RTTurnLog
    RTResolvedEvent RTEquipmentData RTTestScenario RTScenarioSession RTScenarioRunner""".split())

# --- Test: le eccezioni scritte a mano (stem del file -> cartella del codice) ----------------------------------
#
# `Shared`: le fixture che piu' aree includono (`RTWorldFixtures.h` da 88 file). Accanto a un solo soggetto
# sembrerebbero sue. `Meta`: i gate sui test stessi (nomi, guardie, coerenza dei gate di release).
TEST_DIRETTI = {}
for _stem in """RTWorldFixtures RTAbilityFixtures RTConsoleVariableGuardForTest RTAuthoredArenaForTest
        RTWidgetAssetTestHelpers RTReflectedFieldsForTest RTUnitClassProbeForTest RTServerOnlyGuardFixturesForTest
        RTGameModeLevelSpyForTest RTScenarioTestSupport""".split():
    TEST_DIRETTI[_stem] = "Shared"
for _stem in "RTGateCoherenceTests RTLegacyIdentityRatchetTests RTTestNamingTests RTTestGuardTests RTG7BlueprintSurfaceTests".split():
    TEST_DIRETTI[_stem] = "Meta"
for _stem in "RTVeilFadeTests RTVeilFollowsMoveTests RTVeilProbeForTest RTVeilTests RTPlanVeilTests".split():
    TEST_DIRETTI[_stem] = "Perception/Veil"
# 🔴 Le parole del dominio sbagliano quando il nome contiene la parola di un ALTRO sottosistema: `Surface` di
# `UnitBlueprintSurface` non e' il terreno, `Guard` di `HexEdgeGuard` non e' una reazione, `Format` di
# `PieSessionFormat` non e' il formato di partita. Trovati dalla review di #4062 leggendo include e nomi Automation.
TEST_DIRETTI.update({
    "RTUnitBlueprintSurfaceTests": "Unit", "RTAnimVariantTests": "Unit", "RTUnitLabelTests": "Unit",
    "RTPieSessionFormatTests": "PieSession", "RTHexEdgeGuardTests": "Map/Cover", "RTInteractionGraphTests": "Map/Cover",
    "RTHexBotReactionTests": "Bot", "RTTurnRulesTests": "Turn/Resolution", "RTPackagingConfigTests": "Meta",
    # fixture che piu' aree includono, come le altre di `Shared`
    "RTReactionWindowFixtures": "Shared", "RTOrbitProbeForTest": "Shared", "RTHudGeometryForTest": "Shared",
})

# Una cartella di test che supera il tetto si divide: (cartella, sotto-cartella) -> stem dei file.
TEST_SOTTO = {
    ("UI/HUD", "ActionBar"): "RTActionBarNamedPortsTests RTActionDockPortTests RTActionSlotMockupLookTests "
                             "RTActionSlotRefusalTests RTHudAbilityBarTests RTHudSlotLinesTests RTHudEndAndSlotTests "
                             "RTIdleBarTests RTHudCountedDashTests RTControlHintTests",
    ("UI/HUD", "Intent"): "RTHudIntentLabelPlacementTests RTHudIntentMarkersTests RTHudIntentPresentationTests "
                          "RTHudVisibleIntentViewsTests RTHudPlannedRouteTests RTHudDashPreviewTests RTAimOriginTests "
                          "RTTargetPromptTests RTTargetPromptTestHelpers RTHUDMarksTests RTHudHexGeometryTests "
                          "RTHudGeometryForTest RTHudOverlayClampTests",
    ("Combat", "Damage"): "RTDamageBreakdownTests RTDamageFormulaTests RTDefenseMitigationTests RTExposedModTests "
                          "RTFacingDefenseTests RTHeroCatalogDefenseTests RTHeroDefenseFieldsTests RTHealAreaTests",
    ("Combat", "Status"): "RTStatusTests RTStunCensusTests RTStunnedMovementTests RTUnbalancedProneTests",
    ("Turn/Playback", "Reveal"): "RTAbilityFxProfileTests RTAttackPlaybackProbeForTest RTAttackTracerPlaybackTests "
                                 "RTAttackTracerTests RTDefeatPresentationProbeForTest RTDefeatPresentationTests "
                                 "RTPlaybackActivationTests RTSkillCalloutPlaybackTests RTPlaybackActionIdentityTests",
}

# I test del modulo Editor rispecchiano le sue feature (stesse cartelle di `Public/` e `Private/`).
TEST_EDITOR = {
    "HexMode": "RTHexAnchorReadoutTests RTHexGestureSelectionTests RTHexGhostVerdictTests RTHexLosReadoutTests "
               "RTHexOverlaySettingsTests RTHexProbeReadoutTests RTHexSelectionStoreTests RTHexToolPropertiesTests "
               "RTHexTransitionGlyphTests RTHexValidationGateTests RTHexWorkGridTests RTHexPaintToolBrushTests "
               "RTMapTemplateValidationHookTests",
    "Launcher": "RTDevSandboxLauncherTests RTLauncherLayoutTests RTLauncherScenarioBrowserTests "
                "RTLauncherScenarioPanelTests RTLauncherWorkspaceTests RTScenarioPerspectiveTests "
                "RTScenarioPlaybackPreviewTests RTScenarioViewportTests",
    "Lab": "RTLabCVarSnapshotTests RTLabViewModelTests RTPlaygroundLayoutTests RTPlaygroundPanelTests",
    "Anim": "RTAnimBrowserModelTests RTAnimScanTests",
    "Content": "RTGrayboxMaterialRecipeTests RTGrayboxMeshTests RTHexVolumeMaterialTests RTUnitOutlineMaterialTests "
               "RTUnitOverlayAssetTests",
    "Wiring": "RTActionDockIndexWiringTests RTActionSlotClickWiringTests RTMainMenuEntryWiringTests "
              "RTReplayScreenWiringTests",
}


AVVISI = []


def file_cpp(cartella):
    d = RADICE / cartella
    if not d.is_dir():
        return []
    return sorted(p for p in d.iterdir() if p.is_file() and p.suffix in EST)


def mappa_produzione(rigorosa=False):
    """{percorso vecchio: percorso nuovo} per il codice di produzione, relativi alla radice del repository."""
    out = {}
    for modulo, regole in PRODUZIONE.items():
        unita = {}
        for cartella, nomi in regole.items():
            for nome in nomi.split():
                assert nome not in unita, "unita' %s assegnata due volte" % nome
                unita[nome] = cartella
        trovate = set()
        for p in sorted((RADICE / modulo).rglob("*")):
            if not p.is_file() or p.suffix not in EST:
                continue
            rel = p.relative_to(RADICE / modulo)
            if "Tests" in rel.parts:
                continue
            nome = p.stem
            if nome not in unita:
                continue
            trovate.add(nome)
            nuova = unita[nome]
            if modulo == ED:
                # Public/ o Private/ restano la prima componente: la feature si inserisce dopo.
                meta = rel.parts[0]
                destinazione = "%s/%s/%s/%s" % (modulo, meta, nuova, p.name)
            else:
                destinazione = "%s/%s/%s" % (modulo, nuova, p.name)
            vecchio = "%s/%s" % (modulo, rel.as_posix())
            if vecchio != destinazione:
                out[vecchio] = destinazione
        mancanti = set(unita) - trovate
        # ⛔ Un assert solo quando si GENERA la mappa. Nel gate un'unita' cancellata o rinominata e' un avviso: farlo
        # cadere li' sarebbe un rosso per la ragione sbagliata, del tipo che finisce spento (review di #4062).
        if mancanti:
            assert not rigorosa, "unita' elencate ma assenti in %s: %s" % (modulo, sorted(mancanti))
            AVVISI.append("unita' elencate in PRODUZIONE ma assenti in %s (la tabella va aggiornata): %s"
                          % (modulo, ", ".join(sorted(mancanti))))
    return out


def cartella_finale(percorso, mappa):
    return str(Path(mappa.get(percorso, percorso)).parent.as_posix())


def unita_produzione(mappa):
    """{stem di unita': cartella relativa al modulo, dopo lo spostamento} per il modulo runtime."""
    out = {}
    for p in sorted((RADICE / RT).rglob("*")):
        if not p.is_file() or p.suffix not in EST:
            continue
        rel = p.relative_to(RADICE / RT)
        if "Tests" in rel.parts:
            continue
        vecchio = "%s/%s" % (RT, rel.as_posix())
        nuova = Path(mappa.get(vecchio, vecchio)).relative_to(RT).parent.as_posix()
        out[p.stem] = "" if nuova == "." else nuova
    return out


RE_INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
SUFFISSI_TEST = ("IntegrationTests", "ScenarioTests", "WorldTests", "Tests", "ForTest", "Fixtures", "Fixture", "Probe")


def soggetto_dal_nome(stem, unita):
    """L'unita' di produzione il cui nome e' il prefisso piu' lungo dello stem del test."""
    base = stem
    for s in SUFFISSI_TEST:
        if base.endswith(s):
            base = base[: -len(s)]
            break
    candidati = [u for u in unita if base.startswith(u) or u.startswith(base) and len(base) >= 6]
    if not candidati:
        return None
    return max(candidati, key=lambda u: (len(os_prefix(u, base)), -len(u)))


def os_prefix(a, b):
    n = 0
    for x, y in zip(a, b):
        if x != y:
            break
        n += 1
    return a[:n]


def mappa_test(mappa, tetto):
    unita = unita_produzione(mappa)
    tests = RADICE / RT / "Tests"
    out = {}
    collocati = defaultdict(list)
    ignoti = []
    for p in file_cpp(tests):
        stem = p.stem
        if stem in TEST_DIRETTI:
            cartella = TEST_DIRETTI[stem]
            via = "a mano"
        else:
            s = soggetto_dal_nome(stem, unita)
            parola = next((c for pat, c in PAROLE if re.search(pat, stem[2:])), None)
            if s is not None and len(os_prefix(s, stem)) >= 8 and s not in CROCEVIA:
                cartella, via = unita[s], "nome:%s" % s
            elif parola is not None:
                cartella, via = parola, "parola"
            else:
                inc = RE_INCLUDE.findall(p.read_text(encoding="utf-8", errors="replace"))
                conta = defaultdict(int)
                for i in inc:
                    nome = Path(i).stem
                    if nome in unita and nome not in CROCEVIA:
                        conta[unita[nome]] += 1
                if conta:
                    cartella = max(sorted(conta), key=lambda c: conta[c])
                    via = "include"
                else:
                    ignoti.append(stem)
                    continue
        collocati[cartella].append((stem, via, p.name))
    for cartella, voci in collocati.items():
        for stem, via, nome in voci:
            sotto = next((s for (c, s), stems in TEST_SOTTO.items() if c == cartella and stem in stems.split()), None)
            dest = "Tests" + ("/" + cartella if cartella else "") + ("/" + sotto if sotto else "")
            out["%s/Tests/%s" % (RT, nome)] = "%s/%s/%s" % (RT, dest, nome)

    # I test dell'Editor: tabella esplicita, e ogni file deve comparirvi.
    per_stem = {s: c for c, stems in TEST_EDITOR.items() for s in stems.split()}
    for p in file_cpp(RADICE / ED / "Private/Tests"):
        if p.stem not in per_stem:
            ignoti.append("Editor:" + p.stem)
            continue
        out["%s/Private/Tests/%s" % (ED, p.name)] = "%s/Private/Tests/%s/%s" % (ED, per_stem[p.stem], p.name)
    return out, collocati, ignoti


def conteggi(mappa):
    """File C++ per cartella DOPO lo spostamento, su tutto `Source/` (le cartelle di soli dati non hanno file C++)."""
    c = defaultdict(int)
    for p in (RADICE / "Source").rglob("*"):
        if p.is_file() and p.suffix in EST:
            rel = p.relative_to(RADICE).as_posix()
            c[cartella_finale(rel, mappa)] += 1
    return c


def esito(tetto, rigorosa=False):
    """La misura e il verdetto del gate su `RADICE`. Il verdetto e' 1 se una cartella supera il tetto, se un test non
    ha soggetto, o se un test sta nella radice di `Tests/`; 0 altrimenti."""
    del AVVISI[:]
    mappa = mappa_produzione(rigorosa)
    mt, collocati, ignoti = mappa_test(mappa, tetto)
    mappa.update(mt)
    c = conteggi(mappa)
    sopra = {k: v for k, v in c.items() if v > tetto}
    rc = 1 if (sopra or ignoti or mt) else 0
    return {"mappa": mappa, "mt": mt, "collocati": collocati, "ignoti": ignoti, "conteggi": c, "sopra": sopra, "rc": rc}


def autotest():
    """Il gate provato su alberi sintetici: dimostra che SA fallire (D-188), e che non cade per la ragione sbagliata.

    ⚠️ Prova il VERDETTO e la selezione dei file, non la mappa di oggi: che la tabella `PRODUZIONE` descriva bene il
    repository lo dice il comando senza argomenti. Gli alberi non contengono nessuna unita' di `PRODUZIONE`, quindi
    ogni prova attraversa anche il ramo «unita' elencate ma assenti», che nel gate deve essere un avviso.
    """
    import shutil
    import tempfile
    global RADICE
    vera = RADICE

    def albero(file):
        d = Path(tempfile.mkdtemp(prefix="rt-mappa-autotest-"))
        for rel in file:
            (d / rel).parent.mkdir(parents=True, exist_ok=True)
            (d / rel).write_text("// sintetico\n", encoding="utf-8")
        return d

    def n_file(cartella, n, est=".h"):
        return ["%s/RTSint%02d%s" % (cartella, i, est) for i in range(n)]

    casi = [
        ("una cartella esattamente al tetto", n_file(RT + "/Turn/Alpha", 15), 0),
        # 🔑 il caso per cui il gate esiste
        ("una cartella che supera il tetto di uno", n_file(RT + "/Turn/Alpha", 16), 1),
        ("il tetto conta .h, .cpp e .inl insieme", n_file(RT + "/Map", 8, ".h") + n_file(RT + "/Map", 8, ".cpp"), 1),
        # ⛔ i file che non sono codice non contano: le cartelle di soli dati restano fuori
        ("i file di dati non contano", n_file(RT + "/Tests/Golden", 40, ".rttl"), 0),
        ("un test nella cartella del codice che verifica", [RT + "/Tests/Turn/RTQualcosaTests.cpp"], 0),
        # 🔴 lo specchio: un test lasciato nella radice di `Tests/`, con o senza un soggetto riconoscibile
        ("un test nella radice di Tests/ con un soggetto", [RT + "/Tests/RTHudQualcosaTests.cpp"], 1),
        ("un test nella radice di Tests/ senza soggetto", [RT + "/Tests/RTXyzTests.cpp"], 1),
        ("un test dell'Editor nella radice dei suoi Tests/", [ED + "/Private/Tests/RTQualcosaTests.cpp"], 1),
    ]
    falliti = []
    try:
        for nome, file, atteso in casi:
            d = albero(file)
            try:
                RADICE = d
                avuto = esito(15)["rc"]
            finally:
                RADICE = vera
                shutil.rmtree(str(d), ignore_errors=True)
            ok = avuto == atteso
            print("  %s %s: atteso %d, avuto %d" % ("✅" if ok else "❌", nome, atteso, avuto))
            if not ok:
                falliti.append(nome)
        # Il ramo delle unita' assenti: nel gate avviso, nella generazione della mappa errore.
        d = albero(n_file(RT + "/Turn", 3))
        try:
            RADICE = d
            esito(15)
            avviso = any("assenti" in x for x in AVVISI)
            try:
                esito(15, rigorosa=True)
                rigoroso = False
            except AssertionError:
                rigoroso = True
        finally:
            RADICE = vera
            shutil.rmtree(str(d), ignore_errors=True)
        for nome, ok in [("un'unita' assente e' un avviso nel gate", avviso),
                         ("e un errore quando si genera la mappa", rigoroso)]:
            print("  %s %s" % ("✅" if ok else "❌", nome))
            if not ok:
                falliti.append(nome)
    finally:
        RADICE = vera
    if falliti:
        print("❌ autotest rosso: %s" % ", ".join(falliti))
        return 1
    print("✅ autotest verde — %d casi, e il gate sa tornare 1." % (len(casi) + 2))
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tetto", type=int, default=15)
    ap.add_argument("--json")
    ap.add_argument("--verifica", action="store_true")
    ap.add_argument("--autotest", action="store_true", help="prova il gate su alberi sintetici: sa fallire?")
    ap.add_argument("--dettaglio", action="store_true", help="stampa come e' stato collocato ogni test")
    a = ap.parse_args()
    for f in (sys.stdout, sys.stderr):
        if hasattr(f, "reconfigure"):
            f.reconfigure(errors="replace")
    if a.autotest:
        return autotest()

    e = esito(a.tetto, rigorosa=bool(a.json))
    mappa, mt, collocati, ignoti, c, sopra = e["mappa"], e["mt"], e["collocati"], e["ignoti"], e["conteggi"], e["sopra"]
    for x in AVVISI:
        print("⚠️ " + x)
    print("file da spostare: %d · cartelle: %d · sopra il tetto (%d): %d" % (len(mappa), len(c), a.tetto, len(sopra)))
    for k in sorted(c, key=lambda k: (-c[k], k)):
        if c[k] > a.tetto or a.dettaglio:
            print("  %3d %s%s" % (c[k], k, "   <-- SOPRA" if c[k] > a.tetto else ""))
    if a.dettaglio:
        for cartella in sorted(collocati):
            print("== Tests/%s" % cartella)
            for stem, via, _ in sorted(collocati[cartella]):
                print("     %-48s %s" % (stem, via))
    if ignoti:
        print("test senza soggetto (vanno in TEST_DIRETTI): %s" % ", ".join(ignoti))
    if a.json:
        Path(a.json).write_text(json.dumps(mappa, indent=1, sort_keys=True), encoding="utf-8")
        print("mappa: %s" % a.json)
    # 🔑 Dopo la riorganizzazione un test che la regola vorrebbe spostare e' un test lasciato nella radice di
    # `Tests/`: rompe lo specchio in silenzio, ed e' l'erosione che questo gate esiste per fermare (#4061).
    if mt:
        print("test nella radice di Tests/, che vanno nella cartella del codice che verificano: %s"
              % ", ".join(sorted(Path(v).name for v in mt)))
    if a.verifica:
        return e["rc"]
    return 0


if __name__ == "__main__":
    sys.exit(main())
