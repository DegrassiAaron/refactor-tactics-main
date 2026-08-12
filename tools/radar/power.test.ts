import { test } from 'node:test';
import assert from 'node:assert/strict';
import { parseHeroCatalog } from './parse-catalog.ts';
import { guaranteedDamage, powerRaw, powerRating } from './power.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const roster = () => parseHeroCatalog(HERO, ACTION);
const ability = (h: string, id: string) =>
  roster().find((x) => x.name === h)!.abilities.find((a) => a.id === id)!;

test('un bonus condizionato da uno stato non entra nel danno garantito', () => {
  // `24 danni, +8 su bersaglio Wet` -> 24, non 32.
  assert.equal(guaranteedDamage(ability('Flux', 'Flux.LinearDischarge')), 24);
});

test('un payoff condizionato da una previsione non entra affatto', () => {
  // Stessa regola, condizionante diverso: la tabella reazioni dichiara il trigger d'ingresso su
  // movimento, quindi i 16 danni valgono solo se hai indovinato dove andra' l'avversario.
  assert.equal(guaranteedDamage(ability('Vektor', 'Vektor.InterceptShot')), 0);
});

test('un danno incondizionato entra per intero', () => {
  assert.equal(guaranteedDamage(ability('Vektor', 'Vektor.PulseShot')), 21);
  assert.equal(guaranteedDamage(ability('Flux', 'Flux.ConductiveNode')), 20);
});

test('i quattro power_raw coincidono con quelli pubblicati in #603', () => {
  const atteso: Record<string, number> = { Flux: 55.2, Vektor: 31.0, Bastion: 18.0, Riva: 16.0 };
  for (const hero of roster()) {
    assert.equal(powerRaw(hero), atteso[hero.name], `${hero.name}`);
  }
});

test('i quattro rating power sono quelli pubblicati, e distinti', () => {
  const rating = Object.fromEntries(roster().map((h) => [h.name, powerRating(h)]));
  assert.deepEqual(rating, { Flux: 6, Vektor: 4, Bastion: 3, Riva: 2 });
});
