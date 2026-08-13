import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, writeFileSync, rmSync } from 'node:fs';
import { parseHeroCatalog } from './parse-catalog.ts';
import { profileAxes } from './profile.ts';
import { assertKnownEffects } from './vocabulary.ts';

const H = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const A = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);

test('il roster di oggi usa solo effetti del vocabolario', () => {
  for (const h of parseHeroCatalog(H, A)) assertKnownEffects(h);
});

test('un effetto sconosciuto e un ERRORE, non uno zero silenzioso', () => {
  const s = readFileSync(H, 'utf8').replace('18 danni, `Interrupt` sui dispositivi', '18 danni, `Confonde` i nemici');
  const tmp = new URL('./voc.tmp.md', import.meta.url);
  writeFileSync(tmp, s);
  try {
    const flux = parseHeroCatalog(tmp, A).find((x) => x.name === 'Gadget')!;
    // `Flux.Overload` e' un AbilityId, NON il nome mostrato: D-120 ha rinominato l'eroe in `Gadget` e
    // lasciato fermi gli Stable ID. L'errore del vocabolario nomina l'abilita', quindi qui resta `Flux.`.
    assert.throws(() => assertKnownEffects(flux), /Flux\.Overload.*Confonde/);
  } finally { rmSync(tmp); }
});

test('monotonia: aumentare un input non fa scendere il rating che ne dipende', () => {
  const base = parseHeroCatalog(H, A).find((x) => x.name === 'Phase')!;
  for (const [hp, atteso] of [[95, 7], [140, 9]] as const) {
    const h = { ...base, health: hp };
    assert.equal(profileAxes(h).durability, atteso, `durability a ${hp} HP`);
  }
  // Property: su tutto l'intervallo plausibile, mai decrescente.
  let prec = 0;
  for (let hp = 60; hp <= 150; hp += 5) {
    const r = profileAxes({ ...base, health: hp }).durability;
    assert.ok(r >= prec, `durability scende da ${prec} a ${r} a ${hp} HP`);
    prec = r;
  }
});

test('sensibilita: il delta dichiarato muove l asse di almeno 1', () => {
  // La spec dichiara: 17 punti di Salute muovono `durability` di 1 (ancora 150 su nove passi).
  const base = parseHeroCatalog(H, A).find((x) => x.name === 'Phase')!;
  const a = profileAxes({ ...base, health: 95 }).durability;
  const b = profileAxes({ ...base, health: 95 + 17 }).durability;
  assert.ok(b >= a + 1, `+17 HP deve muovere durability: ${a} -> ${b}`);
});
