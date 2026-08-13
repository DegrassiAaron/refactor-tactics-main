// Test delle funzioni pure del Control Center.
//
//     node --test docs/control-center/
//
// Due gruppi, e la differenza conta: i primi girano su fixture minime — dicono che la REGOLA e'
// giusta, e falliscono se la si rompe; gli ultimi girano sui file veri del repository — dicono
// che il CONTRATTO regge oggi, e falliscono se il generatore cambia forma senza avvisare.

import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

import {
  githubBase, issueUrl, docUrl, wikiRefUrl, epicCheckpointKey, roadmapContainer,
  buildIndex, brokenRefs, dependencyCycles, filterFeatures, stalenessVerdict,
} from './graph.js';

const HERE = dirname(fileURLToPath(import.meta.url));
const REPO = join(HERE, '..', '..');
const readJson = (p) => JSON.parse(readFileSync(join(REPO, p), 'utf8'));

const PROJECT = { github: { owner: 'DegrassiAaron', repository: 'refactor-tactics-main', branch: 'main' } };

const feature = (over = {}) => ({
  feature_id: 'RT-FEAT-X', title: 'X', area: 'Core', kind: 'gameplay', release: 'v0.1',
  priority: 'P0', status: 'DESIGNED', gates: {}, gates_done: 0, gates_applicable: 9,
  roadmap: { epic: null, milestone: null, checkpoints: [] },
  dependencies: [], completed_by: [], owner_specs: [], issues: [], tests: [],
  scenarios: [], scenarios_planned: [], wiki_refs: [], ...over,
});

// --- Link ---------------------------------------------------------------------------------------

test('i link si derivano dalla config, non sono scritti a mano', () => {
  assert.equal(issueUrl(PROJECT, 142),
    'https://github.com/DegrassiAaron/refactor-tactics-main/issues/142');
  assert.equal(docUrl(PROJECT, 'docs/characters/v0.1/flux.md'),
    'https://github.com/DegrassiAaron/refactor-tactics-main/blob/main/docs/characters/v0.1/flux.md');
});

test('senza config non si inventa un URL: null, cosi la UI puo dirlo', () => {
  assert.equal(githubBase({}), null);
  assert.equal(issueUrl({}, 1), null);
  assert.equal(docUrl({ github: { owner: 'a' } }, 'x.md'), null);
});

test('il branch dichiarato entra nel link', () => {
  const onDev = { github: { ...PROJECT.github, branch: 'dev' } };
  assert.match(docUrl(onDev, 'docs/x.md'), /\/blob\/dev\//);
});

// Gli esempi usano i due casi che esistono davvero dopo D-076: le pagine di gioco vivono solo nel
// clone (`wiki:`), il catalogo eroi resta nel repository. Il vecchio esempio citava
// `docs/wiki/meccaniche/overwatch.md` e `wiki:Meccanica-overwatch`: il primo e' un percorso
// cancellato, il secondo un nome col prefisso che le pagine non hanno piu' dal 2026-08-10.
test('`wiki:Page` va alla Wiki, un path va al blob: non sono la stessa cosa', () => {
  assert.equal(wikiRefUrl(PROJECT, 'wiki:overwatch'),
    'https://github.com/DegrassiAaron/refactor-tactics-main/wiki/overwatch');
  assert.match(wikiRefUrl(PROJECT, 'docs/characters/v0.1/flux.md'),
    /\/blob\/main\/docs\/characters\/v0\.1\/flux\.md$/);
});

// --- Chiavi di checkpoint -------------------------------------------------------------------------

test('il checkpoint si prefissa dal proprio numero, non dal contenitore', () => {
  // `RT-FEAT-CORE-TURN` sta in E2 e cita anche `4.1`, che e' di E4: prefissare con l'epic
  // dichiarata produrrebbe `E2.4.1` oppure, peggio, `E2.1` — un checkpoint che esiste ed e' altro.
  assert.equal(epicCheckpointKey('4.1'), 'E4.1');
  assert.equal(epicCheckpointKey('2.2'), 'E2.2');
  assert.equal(epicCheckpointKey('M6.3'), 'M6.3');
});

test('il contenitore e epic oppure milestone, mai entrambi inventati', () => {
  assert.equal(roadmapContainer(feature({ roadmap: { epic: 'E12', checkpoints: [] } })), 'E12');
  assert.equal(roadmapContainer(feature({ roadmap: { milestone: 'M9', checkpoints: [] } })), 'M9');
  assert.equal(roadmapContainer(feature()), null);
});

// --- Relazioni inverse ---------------------------------------------------------------------------

test('le relazioni inverse esistono nei due versi', () => {
  const registry = { features: [
    feature({ feature_id: 'RT-FEAT-A', scenarios: ['Scen.One'], issues: [7] }),
    feature({ feature_id: 'RT-FEAT-B', dependencies: ['RT-FEAT-A'], scenarios: ['Scen.One'] }),
    feature({ feature_id: 'RT-FEAT-C', completed_by: ['RT-FEAT-A'] }),
  ] };
  const graph = { editor_sessions: [], scenarios: [{ id: 'Scen.One', requires: [], path: 'x' }],
                  checkpoints: {}, epics: {}, milestones: {} };
  const index = buildIndex(registry, graph);
  assert.deepEqual(index.featuresByScenario.get('Scen.One'), ['RT-FEAT-A', 'RT-FEAT-B']);
  assert.deepEqual(index.dependents.get('RT-FEAT-A'), ['RT-FEAT-B']);
  assert.deepEqual(index.completes.get('RT-FEAT-A'), ['RT-FEAT-C']);
  assert.deepEqual(index.featuresByIssue.get(7), ['RT-FEAT-A']);
});

test('seduta e feature si toccano attraverso il checkpoint, non per magia', () => {
  const registry = { features: [
    feature({ feature_id: 'RT-FEAT-A', roadmap: { epic: 'E1', checkpoints: ['1.4'] } }),
    feature({ feature_id: 'RT-FEAT-Z', roadmap: { epic: 'E9', checkpoints: ['9.9'] } }),
  ] };
  const graph = { scenarios: [], checkpoints: {}, epics: {}, milestones: {}, editor_sessions: [
    { id: 'U10', unblocks: [], unblocked_by: [{ ref: 'E1.4', kind: 'checkpoint' }] },
  ] };
  const index = buildIndex(registry, graph);
  assert.deepEqual(index.featuresBySession.get('U10'), ['RT-FEAT-A']);
});

// --- Riferimenti rotti ----------------------------------------------------------------------------

test('un riferimento a qualcosa che non esiste viene mostrato, non nascosto', () => {
  const registry = { features: [
    feature({ feature_id: 'RT-FEAT-A', dependencies: ['RT-FEAT-FANTASMA'],
              scenarios: ['Scen.Assente'], roadmap: { epic: 'E99', checkpoints: ['99.1'] } }),
  ] };
  const graph = { editor_sessions: [
    { id: 'U1', unblocks: [], unblocked_by: [{ ref: 'CP 6.3', kind: null }] },
  ], scenarios: [], checkpoints: {}, epics: {}, milestones: {} };
  const broken = brokenRefs(registry, graph, buildIndex(registry, graph));
  const kinds = broken.map((b) => b.kind).sort();
  assert.deepEqual(kinds, ['checkpoint', 'dependency', 'prerequisito', 'roadmap', 'scenario']);
  assert.ok(broken.some((b) => b.ref === 'CP 6.3'),
    'la forma nuda non risolve e la pagina deve dirlo');
});

test('niente falsi positivi: quel che esiste non finisce fra i rotti', () => {
  const registry = { features: [
    feature({ feature_id: 'RT-FEAT-A' }),
    feature({ feature_id: 'RT-FEAT-B', dependencies: ['RT-FEAT-A'], scenarios: ['Scen.One'],
              roadmap: { epic: 'E1', checkpoints: ['1.4'] } }),
  ] };
  const graph = { editor_sessions: [], scenarios: [{ id: 'Scen.One', requires: [], path: 'x' }],
                  checkpoints: { 'E1.4': '✅' }, epics: { E1: '✅' }, milestones: {} };
  assert.deepEqual(brokenRefs(registry, graph, buildIndex(registry, graph)), []);
});

// --- Cicli ------------------------------------------------------------------------------------------

test('un ciclo di dipendenze viene trovato, e uno lineare no', () => {
  const cyclic = { features: [
    feature({ feature_id: 'A', dependencies: ['B'] }),
    feature({ feature_id: 'B', dependencies: ['C'] }),
    feature({ feature_id: 'C', dependencies: ['A'] }),
  ] };
  assert.equal(dependencyCycles(cyclic).length > 0, true);
  const linear = { features: [
    feature({ feature_id: 'A', dependencies: ['B'] }),
    feature({ feature_id: 'B', dependencies: [] }),
  ] };
  assert.deepEqual(dependencyCycles(linear), []);
});

// --- Filtri -------------------------------------------------------------------------------------------

test('i filtri mordono sui campi che esistono nel dato', () => {
  const list = [
    feature({ feature_id: 'RT-FEAT-A', release: 'v0.1', area: 'Core', status: 'DONE',
              gates: { spec: 'done', packaged: 'todo' } }),
    feature({ feature_id: 'RT-FEAT-B', release: 'future', area: 'Tools', status: 'BLOCKED',
              gates: { spec: 'todo', packaged: 'done' }, issues: [142] }),
  ];
  assert.deepEqual(filterFeatures(list, { release: 'v0.1' }).map((f) => f.feature_id), ['RT-FEAT-A']);
  assert.deepEqual(filterFeatures(list, { area: 'Tools' }).map((f) => f.feature_id), ['RT-FEAT-B']);
  assert.deepEqual(filterFeatures(list, { blocked: 'yes' }).map((f) => f.feature_id), ['RT-FEAT-B']);
  // `gate` filtra per gate MANCANTE: chi ce l'ha `done` esce.
  assert.deepEqual(filterFeatures(list, { gate: 'packaged' }).map((f) => f.feature_id), ['RT-FEAT-A']);
  assert.deepEqual(filterFeatures(list, { text: '#142' }).map((f) => f.feature_id), ['RT-FEAT-B']);
  assert.equal(filterFeatures(list, {}).length, 2);
});

// --- Staleness ------------------------------------------------------------------------------------------

test('il grafo piu vecchio della sorgente e stale; il contrario no', () => {
  const older = { sha: 'aaa', date: '2026-08-09T10:00:00Z' };
  const newer = { sha: 'bbb', date: '2026-08-10T10:00:00Z' };
  assert.equal(stalenessVerdict(newer, older).stale, true);
  assert.equal(stalenessVerdict(older, newer).stale, false);
  assert.equal(stalenessVerdict(null, newer).known, false);
});

// --- Contratto: i file veri --------------------------------------------------------------------------------

test('CONTRATTO — i due artefatti hanno la forma che la pagina assume', () => {
  const registry = readJson('docs/roadmap/feature-registry.json');
  const graph = readJson('docs/roadmap/project-graph.json');
  for (const key of ['project', 'count', 'features']) assert.ok(key in registry, `manca ${key}`);
  for (const key of ['project', 'sources', 'diagnostics', 'release_gates', 'epics', 'milestones',
                     'checkpoints', 'editor_sessions', 'editor_queue', 'pie_entries', 'scenarios']) {
    assert.ok(key in graph, `manca ${key} nel grafo`);
  }
  assert.ok(githubBase(registry.project), 'la config GitHub deve reggere la derivazione dei link');
  assert.equal(registry.count, registry.features.length);
});

test('CONTRATTO — nessun feature_id duplicato', () => {
  const registry = readJson('docs/roadmap/feature-registry.json');
  const ids = registry.features.map((f) => f.feature_id);
  assert.equal(new Set(ids).size, ids.length);
});

test('CONTRATTO — il progresso e N/M coi soli gate applicabili, mai una percentuale', () => {
  const registry = readJson('docs/roadmap/feature-registry.json');
  for (const f of registry.features) {
    const applicable = Object.values(f.gates).filter((g) => g !== 'na').length;
    const done = Object.values(f.gates).filter((g) => g === 'done').length;
    assert.equal(f.gates_applicable, applicable, `${f.feature_id}: denominatore`);
    assert.equal(f.gates_done, done, `${f.feature_id}: numeratore`);
    assert.ok(f.gates_done <= f.gates_applicable, `${f.feature_id}: N > M`);
  }
});

test('CONTRATTO — sui dati veri non ci sono riferimenti rotti ne cicli', () => {
  const registry = readJson('docs/roadmap/feature-registry.json');
  const graph = readJson('docs/roadmap/project-graph.json');
  const broken = brokenRefs(registry, graph, buildIndex(registry, graph));
  assert.deepEqual(broken, [], 'riferimenti che non risolvono');
  assert.deepEqual(dependencyCycles(registry), [], 'cicli fra dipendenze');
});

test('CONTRATTO — ogni seduta in coda ha un gruppo, e i gruppi coprono le sedute con stato', () => {
  const graph = readJson('docs/roadmap/project-graph.json');
  const inQueue = Object.values(graph.editor_queue).flat();
  const withState = graph.editor_sessions.filter((s) => s.state !== '—').map((s) => s.id);
  assert.deepEqual([...inQueue].sort(), [...withState].sort());
  for (const s of graph.editor_sessions) {
    assert.equal(s.queue_group === null, s.state === '—',
      `${s.id}: senza stato derivabile non sta in coda, e viceversa`);
  }
});

test('CONTRATTO — ogni seduta dichiara una lane fra pie e asset, e il default e pie', () => {
  const graph = readJson('docs/roadmap/project-graph.json');
  const lanes = new Set(graph.editor_sessions.map((s) => s.execution_lane));
  assert.deepEqual([...lanes].sort(), ['asset', 'pie'],
    'il generatore serializza solo le due lane note');
  for (const s of graph.editor_sessions) {
    assert.ok(s.execution_lane, `${s.id}: lane assente — il default non e stato applicato`);
  }
  // Il campo e' facoltativo nel YAML: se la serializzazione lo perdesse, TUTTE le sedute
  // uscirebbero `pie` e il test sopra passerebbe lo stesso. Serve una seduta che il default NON
  // spiega — e' la sola forma in cui questo contratto sa fallire.
  const asset = graph.editor_sessions.filter((s) => s.execution_lane === 'asset').map((s) => s.id);
  assert.ok(asset.length > 0, 'nessuna seduta ASSET: il campo non arriva dal YAML');
  assert.deepEqual(asset.sort(), ['U1', 'U7', 'U8'],
    'le sedute il cui output primario e un asset committato');
});

test('CONTRATTO — la release di ogni feature sta fra quelle che la roadmap dichiara', () => {
  const registry = readJson('docs/roadmap/feature-registry.json');
  const known = new Set(['v0.1', 'v0.2', 'v0.3', 'v0.4', 'future']);
  for (const f of registry.features) {
    assert.ok(known.has(f.release), `${f.feature_id}: release ${f.release} sconosciuta`);
  }
  // v0.3 esiste nei DATI, non solo nell'enum: prima del 2026-08-13 il registry non sapeva
  // esprimerla e cinque feature stavano in `future` per assenza di un valore, non di una
  // decisione. L'insieme e' pinnato per esteso e non con un `some(...)`: con `some` bastava una
  // sola feature per far passare il test, e riportarne quattro in `future` non avrebbe rotto
  // niente — cioe' il test avrebbe smesso di verificare proprio la migrazione che esiste per
  // proteggere. Quando la roadmap owner assegnera' altre feature a v0.3, questa lista cresce
  // insieme a lei: e' il punto in cui il cambiamento si dichiara.
  const v03 = registry.features.filter((f) => f.release === 'v0.3').map((f) => f.feature_id);
  assert.deepEqual(v03.sort(), [
    'RT-FEAT-ACTION-DELAYED',    // E29 · #329
    'RT-FEAT-ACTION-TRAPS',      // E29 · #329
    'RT-FEAT-BOT-BELIEF',        // E27 · #327
    'RT-FEAT-BOT-PREDICTIVE',    // E28 · #328
    'RT-FEAT-INTENT-CONDITIONAL', // E33 · #330
  ], 'le feature che le issue epic v0.3 nominano per nome');
  // Le Feature `RT-FEAT-PERCEPTION-*` restano v0.1 anche se E27 le cita: E27 le **estende**, e
  // un'epic che estende una capacita' non ne riscrive retroattivamente la release.
  assert.ok(registry.features.filter((f) => f.feature_id.startsWith('RT-FEAT-PERCEPTION-'))
    .every((f) => f.release === 'v0.1'), 'la percezione base e nata in v0.1 e ci resta');
});
