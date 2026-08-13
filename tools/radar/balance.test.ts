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
  const b = balanceAxes(hero('Riktor'));
  const p = profileAxes(hero('Riktor'));
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
    { name: 'Gadget', values: profileAxes(hero('Gadget')) },
    { name: 'Riktor', values: profileAxes(hero('Riktor')) },
    PROFILE_AXES,
  );
  assert.match(svg, /<title id="t">Gadget vs Riktor/);
  assert.equal((svg.match(/class="shape/g) ?? []).length, 2, 'due poligoni');
  // Una legenda, o due forme sovrapposte non si leggono.
  assert.ok(svg.includes('>Gadget<') && svg.includes('>Riktor<'));
});

test('anche il confronto ha decimali fissi', () => {
  const svg = renderCompare(
    { name: 'Gadget', values: profileAxes(hero('Gadget')) },
    { name: 'Phase', values: profileAxes(hero('Phase')) },
    PROFILE_AXES,
  );
  for (const n of svg.match(/-?\d+\.\d+/g) ?? []) assert.match(n, /^-?\d+\.\d{2}$/);
});
