import { test } from 'node:test';
import assert from 'node:assert/strict';
import { parseHeroCatalog } from './parse-catalog.ts';
import { balanceAxes } from './balance.ts';
import { renderRadar, renderCompare, BALANCE_AXES, PROFILE_AXES } from './svg.ts';
import { profileAxes } from './profile.ts';

const H = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const A = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const roster = () => parseHeroCatalog(H, A);
const hero = (n: string) => roster().find((h) => h.name === n)!;

test('l ordine dei cinque assi Balance e normativo (owner §2.2)', () => {
  assert.deepEqual(BALANCE_AXES.map((a) => a.key), ['precision', 'power', 'control', 'support', 'durability']);
});

test('i cinque assi Balance riusano i valori gia calcolati, senza ricalcolarli', () => {
  const b = balanceAxes(hero('Bastion'));
  const p = profileAxes(hero('Bastion'));
  // I tre condivisi devono coincidere: due viste, un solo calcolo.
  assert.equal(b.control, p.control);
  assert.equal(b.support, p.support);
  assert.equal(b.durability, p.durability);
  assert.equal(b.power, 3);
  assert.equal(b.precision, 10);
});

test('il Balance SVG si genera per i quattro eroi', () => {
  for (const h of roster()) {
    const svg = renderRadar(h.name, 'Balance', BALANCE_AXES, balanceAxes(h), 'Balance');
    assert.match(svg, /<svg/);
    for (const a of BALANCE_AXES) assert.ok(svg.includes(a.label));
  }
});

test('il radar di confronto sovrappone due eroi e li distingue', () => {
  const svg = renderCompare(
    { name: 'Flux', values: profileAxes(hero('Flux')) },
    { name: 'Bastion', values: profileAxes(hero('Bastion')) },
    PROFILE_AXES,
  );
  assert.match(svg, /<title id="t">Flux vs Bastion/);
  assert.equal((svg.match(/class="shape/g) ?? []).length, 2, 'due poligoni');
  // Una legenda, o due forme sovrapposte non si leggono.
  assert.ok(svg.includes('>Flux<') && svg.includes('>Bastion<'));
});

test('anche il confronto ha decimali fissi', () => {
  const svg = renderCompare(
    { name: 'Flux', values: profileAxes(hero('Flux')) },
    { name: 'Riva', values: profileAxes(hero('Riva')) },
    PROFILE_AXES,
  );
  for (const n of svg.match(/-?\d+\.\d+/g) ?? []) assert.match(n, /^-?\d+\.\d{2}$/);
});
