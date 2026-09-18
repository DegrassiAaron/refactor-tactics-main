import { test } from 'node:test';
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { mkdtempSync, mkdirSync, writeFileSync, rmSync, readFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, sep } from 'node:path';
import { fileURLToPath } from 'node:url';
import { docsCorpus, isUnderArchive, markdownFiles, DOCS_DIR, ROOT_DOCS } from './docs-corpus.ts';

const RADAR = fileURLToPath(new URL('./', import.meta.url));
const REPO = fileURLToPath(new URL('../../', import.meta.url));

/** Un albero finto, cosi' le proprieta' di `markdownFiles` si misurano senza dipendere da `docs/`. */
function scratch(build: (root: string) => void): string {
  const root = mkdtempSync(join(tmpdir(), 'rt-docs-corpus-'));
  build(root);
  return root;
}

test('il filtro dell archivio guarda il PRIMO SEGMENTO, non il prefisso della stringa', () => {
  // 🔴 Il difetto che questo pinna: `relative(DOCS_DIR, f).startsWith('archive')` e' vero anche per
  // `archived-decisions/`, che uscirebbe dalla copertura di ENTRAMBI i gate mentre la riga stampata
  // continua a dire `(docs/archive/ escluso)`. Con la mutazione al prefisso, le tre righe `false`
  // qui sotto diventano `true` e il test cade.
  assert.equal(isUnderArchive(join(DOCS_DIR, 'archive', 'vecchio.md')), true);
  assert.equal(isUnderArchive(join(DOCS_DIR, 'archive', 'sotto', 'vecchio.md')), true);

  assert.equal(isUnderArchive(join(DOCS_DIR, 'archived-decisions', 'd.md')), false);
  assert.equal(isUnderArchive(join(DOCS_DIR, 'archive-2026', 'd.md')), false);
  assert.equal(isUnderArchive(join(DOCS_DIR, 'archive-notes.md')), false);
});

test('una cartella il cui nome COMINCIA per archive resta nel corpus', () => {
  const root = scratch((r) => {
    mkdirSync(join(r, 'archive'));
    mkdirSync(join(r, 'archived-decisions'));
    writeFileSync(join(r, 'archive', 'storico.md'), '# storico\n');
    writeFileSync(join(r, 'archived-decisions', 'viva.md'), '# viva\n');
  });
  try {
    const tenuti = markdownFiles(root).filter((f) => !isUnderArchive(f, root));
    assert.deepEqual(
      tenuti.map((f) => f.slice(root.length + 1).split(sep).join('/')),
      ['archived-decisions/viva.md'],
    );
  } finally {
    rmSync(root, { recursive: true, force: true });
  }
});

test('markdownFiles scende ricorsivamente, prende solo i .md e ha un ordine stabile', () => {
  const root = scratch((r) => {
    mkdirSync(join(r, 'b'));
    mkdirSync(join(r, 'a'));
    writeFileSync(join(r, 'z.md'), '');
    writeFileSync(join(r, 'nota.txt'), '');
    writeFileSync(join(r, 'a', 'due.md'), '');
    writeFileSync(join(r, 'a', 'uno.md'), '');
    writeFileSync(join(r, 'b', 'tre.md'), '');
  });
  try {
    const rel = markdownFiles(root).map((f) => f.slice(root.length + 1).split(sep).join('/'));
    assert.deepEqual(rel, ['a/due.md', 'a/uno.md', 'b/tre.md', 'z.md']);
    // L'ordine non dipende dall'ordine in cui il filesystem restituisce le entry: due chiamate danno
    // la stessa lista, ed e' la proprieta' che rende riproducibile il referto di un gate.
    assert.deepEqual(markdownFiles(root), markdownFiles(root));
  } finally {
    rmSync(root, { recursive: true, force: true });
  }
});

test('il corpus porta i documenti di governance della radice, e docs/archive solo se richiesto', () => {
  const senza = docsCorpus();
  const con = docsCorpus({ withArchive: true });

  for (const doc of ROOT_DOCS) {
    assert.ok(senza.includes(join(REPO, doc)), `manca ${doc}`);
  }
  assert.equal(senza.some((f) => isUnderArchive(f)), false);
  assert.ok(con.length > senza.length, 'con --with-archive il corpus deve crescere');
  assert.ok(con.some((f) => isUnderArchive(f)), 'con --with-archive l archivio deve entrare');
});

test('i due gate leggono il corpus da QUI, invece di ridefinirne uno ciascuno', () => {
  // ⚠️ Il difetto di #1405 non era un comportamento sbagliato: era che la definizione stava in due posti
  // e le due copie potevano divergere in silenzio. La proprieta' da difendere e' quindi strutturale, e
  // si misura sul sorgente: reintrodurre una copia locale in uno dei due gate fa cadere questo test.
  for (const gate of ['doc-links.ts', 'doc-tables.ts']) {
    const src = readFileSync(join(RADAR, gate), 'utf8');
    assert.match(src, /from '\.\/docs-corpus\.ts'/, `${gate} non importa il corpus condiviso`);
    assert.doesNotMatch(src, /function markdownFiles\(/, `${gate} ridefinisce markdownFiles`);
    assert.doesNotMatch(src, /ROOT_DOCS = \[/, `${gate} ridefinisce ROOT_DOCS`);
    assert.doesNotMatch(src, /startsWith\('archive'\)/, `${gate} filtra l archivio per prefisso`);
  }
});

test('i due gate dichiarano di aver guardato lo STESSO numero di documenti', () => {
  // La controprova a runtime del test strutturale qui sopra: se i corpora divergessero, le due righe di
  // copertura — formulate allo stesso modo — continuerebbero a sembrare d'accordo mentre non lo sono.
  const coperti = (gate: string): number => {
    // La riga di copertura va su **stderr**, come i problemi: e' il referto del gate, non il suo output.
    const run = spawnSync(process.execPath, [join(RADAR, gate), '--check'], {
      cwd: REPO,
      encoding: 'utf8',
    });
    assert.equal(run.status, 0, `${gate}: uscito ${run.status}
${run.stderr}`);
    const m = `${run.stdout}${run.stderr}`.match(/in (\d+) documenti/);
    assert.ok(m, `${gate}: la riga di copertura non dice quanti documenti ha guardato`);
    return Number(m![1]);
  };
  assert.equal(coperti('doc-links.ts'), coperti('doc-tables.ts'));
});
