/** Legge i due cataloghi e stampa `HeroInput` come JSON, oppure fallisce dicendo perche'.
 *
 *  Uso:  node tools/radar/cli.ts [--quiet]
 *
 *  Non produce file: scrive su stdout. Un errore di lettura esce con codice 1 **prima** di
 *  emettere qualunque cosa — nessun artefatto parziale (#559). */
import { readCatalogs } from './parse-catalog.ts';

const HERO_CATALOG = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION_CATALOG = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);

/** Il roster della v0.1. Cresce a otto con E35: e' un numero atteso, non una scoperta. */
const EXPECT_HEROES = 4;

try {
  const { heroes, coverage } = readCatalogs(HERO_CATALOG, ACTION_CATALOG, {
    expectHeroes: EXPECT_HEROES,
  });

  // La copertura va su stderr: stdout resta il JSON, pipe-abile senza filtri.
  if (!process.argv.includes('--quiet')) console.error(`copertura: ${coverage}`);
  process.stdout.write(JSON.stringify(heroes, null, 2) + '\n');
} catch (err) {
  console.error(`errore: ${err instanceof Error ? err.message : String(err)}`);
  process.exit(1);
}
