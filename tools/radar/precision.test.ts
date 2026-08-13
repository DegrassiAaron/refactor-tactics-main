import { test } from 'node:test';
import assert from 'node:assert/strict';
import { parseHeroCatalog } from './parse-catalog.ts';
import { unconditionality, selectivity, precisionRating } from './precision.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const hero = (n: string) => parseHeroCatalog(HERO, ACTION).find((h) => h.name === n)!;

test('incondizionalita: quota di danno garantito sul potenziale, pesata', () => {
  // Riktor e Phase non hanno nulla di condizionale: 1.00 esatto.
  assert.equal(unconditionality(hero('Riktor')), 1);
  assert.equal(unconditionality(hero('Phase')), 1);

  // Gadget paga il `+8 su Wet`; Wraith paga l'intero payoff predittivo di InterceptShot.
  assert.ok(unconditionality(hero('Gadget')) < 1);
  assert.ok(unconditionality(hero('Wraith')) < unconditionality(hero('Gadget')));
});

test('selettivita: quota di disponibilita che non rischia gli alleati', () => {
  assert.equal(selectivity(hero('Riktor')), 1);
  assert.equal(selectivity(hero('Phase')), 0);
  assert.equal(selectivity(hero('Wraith')), 0.75);
  assert.equal(selectivity(hero('Gadget')), 0.5);
});

test('i quattro rating precision sono quelli pubblicati, e distinti', () => {
  const r = Object.fromEntries(
    parseHeroCatalog(HERO, ACTION).map((h) => [h.name, precisionRating(h)]),
  );
  assert.deepEqual(r, { Gadget: 7, Wraith: 8, Riktor: 10, Phase: 4 });
});
