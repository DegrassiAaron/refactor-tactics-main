/** Genera gli SVG dei radar, o verifica che quelli committati siano allineati.
 *
 *  Uso:  node tools/radar/generate.ts [--check]
 *
 *  Con `--check` non scrive nulla: confronta e **fallisce** se un file diverge. Poiche' i rating
 *  vengono dai cataloghi (D-106), il gate diventa un test di regressione sui **dati competitivi** —
 *  toccare una stat lo rende rosso finche' i grafici non sono rigenerati nello stesso commit. */
import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { readCatalogs } from './parse-catalog.ts';
import { profileAxes } from './profile.ts';
import { renderRadar, PROFILE_AXES, BALANCE_AXES } from './svg.ts';
import { balanceAxes } from './balance.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const OUT = fileURLToPath(new URL('../../docs/characters/radar/', import.meta.url));

/** Il ruolo per eroe: sta in §5.1 del catalogo, in una tabella che il parser non legge. Dichiararlo
 *  qui e' un compromesso temporaneo — va in `HeroInput` quando la Wiki lo richiedera' (#563). */
const ROLES: Record<string, string> = {
  Flux: 'Controller',
  Riva: 'Support',
  Bastion: 'Guardian',
  Vektor: 'Striker',
};

const check = process.argv.includes('--check');
const { heroes, coverage } = readCatalogs(HERO, ACTION, { expectHeroes: 4, expectAbilities: 20 });

const diverging: string[] = [];
if (!check && !existsSync(OUT)) mkdirSync(OUT, { recursive: true });

for (const hero of heroes) {
  const role = ROLES[hero.name] ?? '—';
  const views: [string, string][] = [
    ['profile', renderRadar(hero.name, role, PROFILE_AXES, profileAxes(hero))],
    ['balance', renderRadar(hero.name, role, BALANCE_AXES, balanceAxes(hero))],
  ];

  for (const [view, svg] of views) {
  const file = `${OUT}${hero.name.toLowerCase()}-${view}.svg`;

  if (check) {
    const current = existsSync(file) ? readFileSync(file, 'utf8') : null;
    if (current !== svg) diverging.push(`${hero.name.toLowerCase()}-${view}.svg`);
  } else {
    writeFileSync(file, svg, { encoding: 'utf8' });
  }
  }
}

console.error(`copertura: ${coverage}`);

if (check && diverging.length > 0) {
  console.error(
    `errore: ${diverging.length} radar non allineati ai cataloghi: ${diverging.join(', ')}.\n` +
      `Rigenerali nello stesso commit: node tools/radar/generate.ts`,
  );
  process.exit(1);
}
console.error(check ? 'radar allineati ai cataloghi' : `scritti ${heroes.length * 2} radar in docs/characters/radar/`);
