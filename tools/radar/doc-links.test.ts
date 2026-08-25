import { test } from 'node:test';
import assert from 'node:assert/strict';
import { scanLinks, resolveTarget, deadLinks, staleLabels } from './doc-links.ts';

test('lo scan trova i link a file del repository e ignora tutto il resto', () => {
  const md = [
    '[una spec](../technical/spec-turnlog.md)',
    '[il decision log](RT_PDR_00_Decision_Log.md)',
    '[un sito](https://example.com/pagina.md)',
    '[posta](mailto:qualcuno@example.com)',
    '[una sezione di questo file](#il-titolo)',
  ].join('\n');

  const refs = scanLinks(md);

  assert.deepEqual(
    refs.map((r) => r.target),
    ['../technical/spec-turnlog.md', 'RT_PDR_00_Decision_Log.md'],
  );
});

test('il bersaglio si risolve rispetto al file che lo contiene, senza ancora ne query', () => {
  assert.equal(
    resolveTarget('../technical/spec-turnlog.md', 'docs/roadmap/roadmap-v0.1.md'),
    'docs/technical/spec-turnlog.md',
  );
  assert.equal(
    resolveTarget('RT_PDR_00_Decision_Log.md#d-182', 'docs/decisions/adr-0009.md'),
    'docs/decisions/RT_PDR_00_Decision_Log.md',
  );
});

test('un link a un file che non esiste e segnalato, uno che esiste no', () => {
  const vivi = new Set(['docs/technical/tooling/scenario-map.md']);
  const md = [
    '[viva](tooling/scenario-map.md)',
    '[morta](scenario-map.md)',
  ].join('\n');

  const morti = deadLinks(md, 'docs/technical/README.md', (p) => vivi.has(p));

  assert.equal(morti.length, 1);
  assert.equal(morti[0]!.resolved, 'docs/technical/scenario-map.md');
});

test('un etichetta che mostra un percorso vecchio e segnalata anche se il link funziona', () => {
  const vivi = new Set(['docs/technical/tooling/scenario-map.md']);
  // Il link risolve: `deadLinks` non ha niente da dire. Ma chi legge vede il percorso vecchio, lo
  // copia altrove, e il difetto si propaga senza che nessun link si rompa.
  const md = '[docs/technical/scenario-map.md](tooling/scenario-map.md)';
  const from = 'docs/technical/README.md';

  assert.equal(deadLinks(md, from, (p) => vivi.has(p)).length, 0);

  const stale = staleLabels(md, from, (p) => vivi.has(p));
  assert.equal(stale.length, 1);
  assert.equal(stale[0]!.label, 'docs/technical/scenario-map.md');
});

test('un etichetta di prosa non e un percorso e non viene giudicata', () => {
  const vivi = new Set(['docs/technical/tooling/scenario-map.md']);
  const md = '[la mappa degli scenari](tooling/scenario-map.md)';
  assert.equal(staleLabels(md, 'docs/technical/README.md', (p) => vivi.has(p)).length, 0);
});

test('un link dentro inline code o dentro un blocco e un esempio, non un riferimento', () => {
  // Casi reali: `docs/README.md` insegna il difetto dell'etichetta che mente mostrandone uno finto,
  // e un referto cita la sintassi `[testo](url)` per spiegare come contava i link. Prenderli sul
  // serio significa segnalare la documentazione che spiega il gate — il modo piu' rapido per farlo
  // disattivare.
  const md = [
    'Una etichetta puo mentire: ``[`../vecchio/x.md`](../nuovo/x.md)``',
    'il conteggio leggeva solo la sintassi `[testo](url)`',
    '',
    '```md',
    '![Una immagine](images/che-non-esiste.png)',
    '```',
    '',
    '[questo invece e vero](davvero.md)',
  ].join('\n');

  const refs = scanLinks(md);

  assert.deepEqual(refs.map((r) => r.target), ['davvero.md']);
});
