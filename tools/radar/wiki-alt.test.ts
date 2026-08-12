import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { profileAxes } from './profile.ts';
import { balanceAxes } from './balance.ts';
import { parseHeroCatalog } from './parse-catalog.ts';
import { renderRadar, PROFILE_AXES, BALANCE_AXES } from './svg.ts';
import { expectedAlt, scanMarkdown, rewrite } from './wiki-alt.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const flux = () => parseHeroCatalog(HERO, ACTION).find((h) => h.name === 'Flux')!;
const RADAR = fileURLToPath(new URL('../../docs/characters/radar/', import.meta.url));

test('l alt atteso ripete il nome accessibile e vi aggiunge i valori disegnati', () => {
  const svg = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()), 'Profile');
  const alt = expectedAlt(svg);
  assert.match(alt, /^Flux — Controller — Profile Radar: /);
  for (const axis of PROFILE_AXES) assert.ok(alt.includes(axis.label), `manca l'asse ${axis.label}`);
});

test('l alt viene dall SVG, non da una tabella parallela', () => {
  // Se l'atteso fosse ricopiato da qualche parte, cambiare l'SVG non lo cambierebbe: e' esattamente
  // il drift che questo gate esiste per impedire.
  const vero = renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()), 'Profile');
  const alterato = vero.replace(
    /(class="axis"[^>]*>Offesa<\/text>\s*<text class="value"[^>]*>)\d+/,
    '$19',
  );
  assert.notEqual(alterato, vero, 'la mutazione non ha toccato l SVG: test inutile');
  assert.notEqual(expectedAlt(alterato), expectedAlt(vero));
  assert.match(expectedAlt(alterato), /Offesa 9/);
});

test('le due viste non producono lo stesso alt', () => {
  const p = expectedAlt(renderRadar('Flux', 'Controller', PROFILE_AXES, profileAxes(flux()), 'Profile'));
  const b = expectedAlt(renderRadar('Flux', 'Controller', BALANCE_AXES, balanceAxes(flux()), 'Balance'));
  assert.notEqual(p, b);
  assert.match(b, /Balance Radar: .*Precisione/);
});

test('un SVG senza title fallisce invece di inventare un alt', () => {
  assert.throws(() => expectedAlt('<svg></svg>'), /title/);
});

test('lo scan trova i radar per URL raw e per percorso relativo, e ignora il resto', () => {
  const md = [
    '![Vecchio testo](https://raw.githubusercontent.com/x/y/main/docs/characters/radar/flux-profile.svg)',
    '![Altro](../radar/riva-balance.svg)',
    '![Una card](images/roster/26_flux-card-v0.1.png)',
    '[[Un link|flux]]',
  ].join('\n');
  const refs = scanMarkdown(md);
  assert.equal(refs.length, 2);
  assert.deepEqual(
    refs.map((r) => [r.hero, r.view]),
    [['flux', 'profile'], ['riva', 'balance']],
  );
  assert.equal(refs[0]!.alt, 'Vecchio testo');
});

test('la riscrittura tocca l alt e lascia intatti URL e riga', () => {
  const url = 'https://raw.githubusercontent.com/x/y/main/docs/characters/radar/flux-profile.svg';
  const riga = `| ![vecchio](${url}) | **[[Flux\\|flux]]** |`;
  const fixes = new Map([[`![vecchio](${url})`, `![nuovo](${url})`]]);
  const out = rewrite(riga, fixes);
  assert.equal(out, `| ![nuovo](${url}) | **[[Flux\\|flux]]** |`);
  assert.ok(out.includes(url), 'l URL non deve cambiare');
});

test('gli otto SVG committati producono otto alt distinti', () => {
  const alts = new Set<string>();
  for (const hero of ['flux', 'riva', 'bastion', 'vektor']) {
    for (const view of ['profile', 'balance']) {
      alts.add(expectedAlt(readFileSync(`${RADAR}${hero}-${view}.svg`, 'utf8')));
    }
  }
  // Otto immagini indistinguibili per chi non le vede sarebbero otto etichette uguali.
  assert.equal(alts.size, 8);
});
