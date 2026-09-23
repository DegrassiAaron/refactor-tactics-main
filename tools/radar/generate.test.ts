/** `generate.ts` non esporta niente: e' uno script. Si prova eseguendolo, che e' anche il modo in cui
 *  lo usa chi legge — il criterio di #2579 chiede che la derivazione sia *«leggibile senza aprire il
 *  codice»*, quindi il comando E' il deliverable e va provato come tale. */
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { readdirSync, statSync } from 'node:fs';

const SCRIPT = fileURLToPath(new URL('./generate.ts', import.meta.url));
const OUT = fileURLToPath(new URL('../../docs/characters/radar/', import.meta.url));

const run = (...args: string[]) =>
  execFileSync(process.execPath, [SCRIPT, ...args], { encoding: 'utf8' });

test('--abilities stampa il contributo di ogni abilita del roster, con la propria banda', () => {
  const out = run('--abilities');
  // Le venti abilita' ci sono tutte: un elenco che ne perde una per strada e' peggio di nessun elenco.
  const righe = out.split('\n').filter((r) => /^\s*Hero\./.test(r));
  assert.equal(righe.length, 20, 'venti abilita nel catalogo, venti righe');
  // Il confronto fra eroi diversi e' il punto: tre abilita' pesano 6000, cioe' 10 per turno.
  for (const id of ['Hero.Aevik.ConductiveNode', 'Hero.Branth.Ram', 'Hero.Ivrin.PassingBlade']) {
    const riga = righe.find((r) => r.includes(id));
    assert.ok(riga, `${id} assente`);
    assert.match(riga!, /\b10\b/, `${id}: il contributo per turno non si legge`);
  }
  // E la derivazione di ogni banda si legge nell'uscita, non nel sorgente.
  assert.match(out, /attacco base/);
  assert.match(out, /derivata da/i);
});

test('--abilities NON scrive nessun artefatto: si rigenera, non si committa', () => {
  // La lezione di D-181: una vista generata e committata invecchia senza dirlo. Qui si stampa.
  const prima = readdirSync(OUT).map((f) => `${f}:${statSync(`${OUT}${f}`).mtimeMs}`);
  run('--abilities');
  const dopo = readdirSync(OUT).map((f) => `${f}:${statSync(`${OUT}${f}`).mtimeMs}`);
  assert.deepEqual(dopo, prima, 'nessun file dei radar deve essere toccato');
});
