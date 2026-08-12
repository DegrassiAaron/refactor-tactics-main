import { test } from 'node:test';
import assert from 'node:assert/strict';
import { parseHeroCatalog } from './parse-catalog.ts';
import { unconditionality, selectivity, precisionRating } from './precision.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const hero = (n: string) => parseHeroCatalog(HERO, ACTION).find((h) => h.name === n)!;

test('incondizionalita: quota di danno garantito sul potenziale, pesata', () => {
  // Bastion e Riva non hanno nulla di condizionale: 1.00 esatto.
  assert.equal(unconditionality(hero('Bastion')), 1);
  assert.equal(unconditionality(hero('Riva')), 1);

  // Flux paga il `+8 su Wet`; Vektor paga l'intero payoff predittivo di InterceptShot.
  assert.ok(unconditionality(hero('Flux')) < 1);
  assert.ok(unconditionality(hero('Vektor')) < unconditionality(hero('Flux')));
});

test('selettivita: quota di disponibilita che non rischia gli alleati', () => {
  assert.equal(selectivity(hero('Bastion')), 1);
  assert.equal(selectivity(hero('Riva')), 0);
  assert.equal(selectivity(hero('Vektor')), 0.75);
  assert.equal(selectivity(hero('Flux')), 0.5);
});

test('i quattro rating precision sono quelli pubblicati, e distinti', () => {
  const r = Object.fromEntries(
    parseHeroCatalog(HERO, ACTION).map((h) => [h.name, precisionRating(h)]),
  );
  assert.deepEqual(r, { Flux: 7, Vektor: 8, Bastion: 10, Riva: 4 });
});
