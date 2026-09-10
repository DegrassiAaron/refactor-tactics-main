import { test } from 'node:test';
import assert from 'node:assert/strict';
import { parseHeroCatalog } from './parse-catalog.ts';
import { unconditionality, selectivity, precisionRating } from './precision.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const hero = (n: string) => parseHeroCatalog(HERO, ACTION).find((h) => h.name === n)!;

test('incondizionalita: quota di danno garantito sul potenziale, pesata', () => {
  // Branth e Phase non hanno nulla di condizionale: 1.00 esatto.
  assert.equal(unconditionality(hero('Branth')), 1);
  assert.equal(unconditionality(hero('Phase')), 1);

  // Aevik paga il `+8 su Wet`; Ivrin paga l'intero payoff predittivo di InterceptShot.
  assert.ok(unconditionality(hero('Aevik')) < 1);
  assert.ok(unconditionality(hero('Ivrin')) < unconditionality(hero('Aevik')));
});

test('selettivita: quota di disponibilita che non rischia gli alleati', () => {
  assert.equal(selectivity(hero('Branth')), 1);
  assert.equal(selectivity(hero('Phase')), 0);
  assert.equal(selectivity(hero('Ivrin')), 0.75);
  assert.equal(selectivity(hero('Aevik')), 0.5);
});

test('i quattro rating precision sono quelli pubblicati, e distinti', () => {
  const r = Object.fromEntries(
    parseHeroCatalog(HERO, ACTION).map((h) => [h.name, precisionRating(h)]),
  );
  assert.deepEqual(r, { Aevik: 7, Ivrin: 8, Branth: 10, Phase: 4 });
});
