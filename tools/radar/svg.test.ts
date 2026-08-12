import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { parseHeroCatalog } from './parse-catalog.ts';
import { profileAxes } from './profile.ts';
import { balanceAxes } from './balance.ts';
import { renderRadar, renderCompare, PROFILE_AXES, BALANCE_AXES } from './svg.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const flux = () => parseHeroCatalog(HERO, ACTION).find((h) => h.name === 'Flux')!;

test('le coordinate hanno decimali fissi: sin/cos non lasciano code variabili', () => {
  const svg = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()), 'Profile');
  for (const n of svg.match(/-?\d+\.\d+/g) ?? []) {
    assert.match(n, /^-?\d+\.\d{2}$/, `coordinata con decimali non fissi: ${n}`);
  }
});

test('la generazione e deterministica: stesso input, stesso byte', () => {
  const a = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()), 'Profile');
  const b = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()), 'Profile');
  assert.equal(createHash('sha256').update(a).digest('hex'), createHash('sha256').update(b).digest('hex'));
});

test('i valori sono leggibili anche testualmente, non solo come forma', () => {
  const svg = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()), 'Profile');
  assert.match(svg, /<title id="t">Flux/); // id richiesto da aria-labelledby
  for (const axis of PROFILE_AXES) assert.ok(svg.includes(axis.label), `manca l'etichetta ${axis.label}`);
  // information 7 e' il tratto di Flux: deve comparire come testo, non solo come vertice.
  assert.match(svg, /Informazione[^<]*<\/text>\s*<text[^>]*>7</);
});

test('un asse TBD fa fallire la generazione nominando eroe e asse', () => {
  const rotti = { ...profileAxes(flux()), control: undefined as unknown as number };
  assert.throws(() => renderRadar('Flux', 'Controller', PROFILE_AXES, rotti, 'Profile'), /Flux.*control/);
});

test('l ordine dei raggi e normativo e non cambia', () => {
  assert.deepEqual(
    PROFILE_AXES.map((a) => a.key),
    ['offense', 'durability', 'mobility', 'control', 'support', 'information'],
  );
});

test('l SVG dichiara una dimensione propria, non solo una proporzione', () => {
  // Senza width/height la misura la decide il contenitore: sulla Wiki lo stesso radar veniva reso
  // 896x896 da solo e 47x47 dentro una tabella, dove le etichette diventano illeggibili.
  for (const svg of [
    renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()), 'Profile'),
    renderCompare(
      { name: 'Flux', values: profileAxes(flux()) },
      { name: 'Riva', values: profileAxes(flux()) },
      PROFILE_AXES,
    ),
  ]) {
    const root = svg.slice(0, svg.indexOf('>') + 1);
    assert.match(root, /width="400"/, 'manca width sulla radice');
    assert.match(root, /height="400"/, 'manca height sulla radice');
    // La proporzione deve restare coerente con la dimensione, o l'immagine si deforma.
    assert.match(root, /viewBox="0 0 400 400"/);
  }
});

test('il titolo dichiara la vista: un Balance non si annuncia come Profile', () => {
  const values = { ...balanceAxes(flux()) };
  const profile = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()), 'Profile');
  const balance = renderRadar('Flux', 'Controller', BALANCE_AXES, values, 'Balance');
  assert.match(profile, /<title id="t">Flux — Controller — Profile Radar<\/title>/);
  assert.match(balance, /<title id="t">Flux — Controller — Balance Radar<\/title>/);
  // Il titolo e' il nome accessibile: due immagini indistinguibili non possono chiamarsi uguale.
  assert.notEqual(
    profile.match(/<title[^>]*>([^<]*)</)![1],
    balance.match(/<title[^>]*>([^<]*)</)![1],
  );
});

test('il testo resta leggibile su tema scuro, non solo su fondo chiaro', () => {
  // Le etichette sono #333 e #111 su fondo trasparente: senza una regola per il tema scuro
  // spariscono, e i valori che il §10 dell'owner vuole «leggibili come testo» non lo sono.
  for (const svg of [
    renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()), 'Profile'),
    renderCompare(
      { name: 'Flux', values: profileAxes(flux()) },
      { name: 'Flux', values: profileAxes(flux()) },
      PROFILE_AXES,
    ),
  ]) {
    const dark = svg.match(/@media \(prefers-color-scheme: dark\) \{([\s\S]*?)\n    \}/);
    assert.ok(dark, 'manca il blocco per il tema scuro');
    for (const cls of ['.axis', '.value']) {
      assert.match(dark[1], new RegExp(`\\${cls} \\{[^}]*fill:`), `il tema scuro non ridefinisce ${cls}`);
    }
  }
});

test('la legenda del confronto prende il colore da una classe, non da un attributo', () => {
  // Un `fill=` inline sopravviverebbe alla media query solo per caso: le due etichette devono
  // poter cambiare colore col tema, e restare comunque distinte fra loro.
  const svg = renderCompare(
    { name: 'Flux', values: profileAxes(flux()) },
    { name: 'Riva', values: profileAxes(flux()) },
    PROFILE_AXES,
  );
  assert.doesNotMatch(svg, /<text class="legend[^"]*"[^>]*fill=/);
  assert.match(svg, /<text class="legend a-ink"/);
  assert.match(svg, /<text class="legend b-ink"/);
});
