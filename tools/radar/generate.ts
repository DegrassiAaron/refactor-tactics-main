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
import { assertKnownEffects } from './vocabulary.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const OUT = fileURLToPath(new URL('../../docs/characters/radar/', import.meta.url));

/** Il ruolo per eroe: sta in §5.1 del catalogo, in una tabella che il parser non legge. Dichiararlo
 *  qui e' un compromesso temporaneo — va in `HeroInput` quando la Wiki lo richiedera' (#563). */
/**
 * Ruolo per CHIAVE STABILE, non per nome mostrato.
 *
 * 🔴 Era chiavata sui nomi (`Flux`, `Riva`, `Bastion`, `Vektor`) e dopo la rinomina di D-120 nessuna chiave
 * corrispondeva piu': `ROLES[hero.name]` dava `undefined`, il `?? '—'` lo trasformava in un trattino, e i
 * quattro radar sarebbero stati rigenerati **senza ruolo** con il gate verde. Il difetto si e' visto solo
 * guardando il diff degli alt della Wiki: `Gadget — Controller —` era diventato `Gadget — — —`.
 */
const ROLES: Record<string, string> = {
  flux: 'Controller',
  riva: 'Support',
  bastion: 'Guardian',
  vektor: 'Striker',
};

const check = process.argv.includes('--check');
const { heroes, coverage } = readCatalogs(HERO, ACTION, { expectHeroes: 4, expectAbilities: 20 });

const diverging: string[] = [];
if (!check && !existsSync(OUT)) mkdirSync(OUT, { recursive: true });

// Un effetto fuori vocabolario ferma tutto PRIMA di scrivere: uno zero silenzioso entrerebbe nei
// radar senza che nessuno se ne accorga (#557).
heroes.forEach(assertKnownEffects);

for (const hero of heroes) {
  // Un ruolo mancante FERMA, non degrada. Il `?? '—'` che c'era prima ha lasciato passare in silenzio
  // quattro radar senza ruolo: e' la stessa disciplina di `assertKnownEffects`, che rifiuta un effetto
  // fuori vocabolario invece di pesarlo zero (#557).
  const role = ROLES[hero.key];
  if (role === undefined) {
    throw new Error(
      `${hero.name}: nessun ruolo per la chiave "${hero.key}". Aggiungilo a ROLES in generate.ts — ` +
        `un radar senza ruolo si disegna comunque, e nessuno se ne accorge.`,
    );
  }
  const views: [string, string][] = [
    ['profile', renderRadar(hero.name, role, PROFILE_AXES, profileAxes(hero), 'Profile')],
    ['balance', renderRadar(hero.name, role, BALANCE_AXES, balanceAxes(hero), 'Balance')],
  ];

  for (const [view, svg] of views) {
  // 🔴 `hero.key`, NON `hero.name`: il nome del file segue lo Stable ID, il titolo dentro l'SVG segue il
  // nome mostrato. Diceva `hero.name.toLowerCase()` fino al 2026-08-13, e dopo la rinomina di D-120 il gate
  // cercava `gadget-profile.svg` mentre sul disco c'era `flux-profile.svg`. Rinominare i file avrebbe rotto
  // le URL che la Wiki incorpora da `raw.githubusercontent.com`.
  const file = `${OUT}${hero.key}-${view}.svg`;

  if (check) {
    const current = existsSync(file) ? readFileSync(file, 'utf8') : null;
    if (current !== svg) diverging.push(`${hero.key}-${view}.svg`);
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
