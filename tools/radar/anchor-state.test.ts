import { test } from 'node:test';
import assert from 'node:assert/strict';
import { mutedClosedAnchors } from './anchor-state.ts';

/** Le righe sono copiate da `capability-roadmaps.md`, non inventate: un gate provato su una sintassi
 *  che il documento non usa non dice niente su quel documento. */

test('una ancora CHIUSA in una riga di tabella, senza annotazione, viene segnalata', () => {
  const doc = [
    '| Ruolo | Issue |',
    '|---|---|',
    '| confine public/sanitized vs private audit | #1805 |',
  ].join('\n');
  const trovate = mutedClosedAnchors(doc, new Set([1805]));
  assert.deepEqual(trovate.map((c) => c.issue), [1805]);
  assert.equal(trovate[0].line, 3);
});

test('una annotazione di GRUPPO a fine elenco copre i numeri che la precedono', () => {
  // Il falso positivo che un controllo ingenuo produce: questa riga e' CORRETTA.
  const doc = '| TD-SCENARIO | #1625 · #1628 (aperte) · #1626 · #1627 · #1629 · #1630 (chiuse) |';
  const trovate = mutedClosedAnchors(doc, new Set([1626, 1627, 1629, 1630]));
  assert.deepEqual(trovate, []);
});

test('una ancora APERTA senza annotazione non si segnala: la convenzione annota solo le chiuse', () => {
  const doc = '| consumer autobattle | #952 |';
  assert.deepEqual(mutedClosedAnchors(doc, new Set([1805])), []);
});

test('la PROSA non e una ancora, e non si segnala', () => {
  // La regola che il documento dichiara: «nessuna ANCORA», non «nessuna riga». `#1754` e' chiusa e
  // compare senza annotazione, ma come riferimento di codice — non dichiara nulla sul proprio stato.
  const doc = 'nominata** come gia e nello Scenario Harness (`RTScenarioKnowledge::OmniscientTeamId`, #1754), o resti la';
  assert.deepEqual(mutedClosedAnchors(doc, new Set([1754])), []);
});

test('il gate trova le DUE derive reali del 2026-09-23, nella forma in cui erano scritte', () => {
  const doc = [
    '| BAL-METRICS — le metriche derivate | `tools/radar/{rubric,power}.ts` (`D-108`) · #2579 (solo per eroe) |',
    '| `H` showcase / video | configuro una demo e la ripeto | **#2745** (discovery) | post-v0.1 |',
  ].join('\n');
  const trovate = mutedClosedAnchors(doc, new Set([2579, 2745]));
  assert.deepEqual(trovate.map((c) => c.issue).sort(), [2579, 2745]);
});

test('una parentesi che NON e uno stato non conta come annotazione', () => {
  // `(solo per eroe)` e `(discovery)` descrivono lo SCOPE. Il gate cerca uno stato, non una parentesi:
  // altrimenti qualunque nota a margine zittirebbe la verifica.
  const doc = '| x | #2579 (solo per eroe) |';
  assert.equal(mutedClosedAnchors(doc, new Set([2579])).length, 1);
});

test('una citazione in PROSA dentro una cella non e una ancora', () => {
  // Riga reale, §4 del documento. E' una riga di tabella, ma `#472` e' un riferimento dentro una
  // frase — non dichiara lo stato di niente. Segnalarla e' il falso positivo che disattiva un gate.
  const doc = '| Shell × Replay | Main Menu e Result navigano verso lo stesso viewer di #472 |';
  assert.deepEqual(mutedClosedAnchors(doc, new Set([472])), []);
});

test('una ancora preceduta solo da separatori o markup resta una ancora', () => {
  // I tre casi reali che NON devono sfuggire mentre si toglie il rumore.
  const casi = [
    ['| confine public/sanitized vs private audit | #1805 |', 1805],
    ['| BAL-METRICS | `tools/radar/power.ts` (`D-108`) · #2579 (solo per eroe) |', 2579],
    ['| `H` showcase | configuro una demo | **#2745** (discovery) | post-v0.1 |', 2745],
  ] as const;
  for (const [riga, issue] of casi) {
    assert.equal(mutedClosedAnchors(riga, new Set([issue])).length, 1, riga);
  }
});
