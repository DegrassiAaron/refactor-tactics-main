import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { parseHeroCatalog } from './parse-catalog.ts';
import { profileAxes } from './profile.ts';
import { renderRadar, PROFILE_AXES } from './svg.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const flux = () => parseHeroCatalog(HERO, ACTION).find((h) => h.name === 'Flux')!;

test('le coordinate hanno decimali fissi: sin/cos non lasciano code variabili', () => {
  const svg = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()));
  for (const n of svg.match(/-?\d+\.\d+/g) ?? []) {
    assert.match(n, /^-?\d+\.\d{2}$/, `coordinata con decimali non fissi: ${n}`);
  }
});

test('la generazione e deterministica: stesso input, stesso byte', () => {
  const a = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()));
  const b = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()));
  assert.equal(createHash('sha256').update(a).digest('hex'), createHash('sha256').update(b).digest('hex'));
});

test('i valori sono leggibili anche testualmente, non solo come forma', () => {
  const svg = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()));
  assert.match(svg, /<title id="t">Flux/); // id richiesto da aria-labelledby
  for (const axis of PROFILE_AXES) assert.ok(svg.includes(axis.label), `manca l'etichetta ${axis.label}`);
  // information 7 e' il tratto di Flux: deve comparire come testo, non solo come vertice.
  assert.match(svg, /Informazione[^<]*<\/text>\s*<text[^>]*>7</);
});

test('un asse TBD fa fallire la generazione nominando eroe e asse', () => {
  const rotti = { ...profileAxes(flux()), control: undefined as unknown as number };
  assert.throws(() => renderRadar('Flux', 'Controller', PROFILE_AXES, rotti), /Flux.*control/);
});

test('l ordine dei raggi e normativo e non cambia', () => {
  assert.deepEqual(
    PROFILE_AXES.map((a) => a.key),
    ['offense', 'durability', 'mobility', 'control', 'support', 'information'],
  );
});
