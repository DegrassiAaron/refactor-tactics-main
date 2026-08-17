import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, writeFileSync, rmSync } from 'node:fs';
import { parseHeroCatalog } from './parse-catalog.ts';
import { profileAxes } from './profile.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const roster = () => parseHeroCatalog(HERO, ACTION);

test('i sei assi del Profile per i quattro eroi', () => {
  const got = Object.fromEntries(roster().map((h) => [h.name, profileAxes(h)]));

  assert.deepEqual(got, {
    Gadget:    { offense: 5, durability: 7, mobility: 7, control: 2, support: 1, information: 7 },
    Wraith:  { offense: 4, durability: 7, mobility: 9, control: 4, support: 1, information: 6 },
    Riktor: { offense: 3, durability: 8, mobility: 6, control: 9, support: 2, information: 5 },
    Phase:    { offense: 1, durability: 7, mobility: 8, control: 7, support: 8, information: 5 },
  });
});

test('nessun asse e costante sui quattro eroi', () => {
  const all = roster().map(profileAxes);
  for (const asse of Object.keys(all[0]) as (keyof (typeof all)[0])[]) {
    assert.ok(new Set(all.map((a) => a[asse])).size > 1, `${asse} e costante`);
  }
});

test('ogni coppia differisce su almeno tre assi (DoD #558)', () => {
  const rs = roster();
  for (let i = 0; i < rs.length; i++)
    for (let j = i + 1; j < rs.length; j++) {
      const [a, b] = [profileAxes(rs[i]), profileAxes(rs[j])];
      const diff = Object.keys(a).filter((k) => a[k] !== b[k]).length;
      assert.ok(diff >= 3, `${rs[i].name} vs ${rs[j].name}: solo ${diff} assi distinti`);
    }
});

test('la mitigazione su di se alza durability, quella sugli alleati no', () => {
  const [flux, bastion] = ['Gadget', 'Riktor'].map((n) => roster().find((h) => h.name === n)!);
  // Gadget 90 HP + scudo 15 -> supera Phase, che ne ha 95 e nessuna mitigazione.
  assert.equal(profileAxes(flux).durability, 7);
  // `Hero.Riktor.Interposition` incassa per un ALLEATO: va in support, non in durability.
  assert.ok(profileAxes(bastion).support > 1);
});

test('uno scudo dato a un ALLEATO non alza la durability di chi lo lancia', () => {
  // Sul roster di oggi nessuna abilita' dichiara un numero di mitigazione verso un alleato —
  // `Interposition` intercetta senza cifra — quindi la regola non e' esercitata dai dati reali.
  // Questo caso la mette alla prova: senza la divisione per bersaglio, Riktor diventerebbe il piu'
  // resistente due volte.
  const conScudoAlleato = readFileSync(HERO, 'utf8').replace(
    'intercetta un attacco diretto a un alleato',
    'scudo 40 a un alleato',
  );
  const tmp = new URL('./catalogo-peel.tmp.md', import.meta.url);
  writeFileSync(tmp, conScudoAlleato);

  try {
    const bastion = parseHeroCatalog(tmp, ACTION).find((h) => h.name === 'Riktor')!;
    assert.equal(profileAxes(bastion).durability, 8, 'lo scudo altrui non deve entrare');
  } finally {
    rmSync(tmp);
  }
});
