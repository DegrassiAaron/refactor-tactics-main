import { test } from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdtempSync, writeFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { copertura, leggiOsserva, marcatore, modificheDopo, verdettiDichiarati } from './pie-verdict-age.ts';

/** Le righe hanno la forma di `test-manuali-pie.md` — cinque colonne, stato in coda — e non una sintassi
 *  inventata: un gate provato su una forma che il documento non usa non dice niente su quel documento. */

const riga = (id: string, stato: string) =>
  `| **${id}** | che cosa verificare | precondizione | esito atteso | ${stato} |`;

const giorno = (epoch: number) => new Date(epoch * 1000).toISOString().slice(0, 10);

test('un verdetto ✅ che dichiara percorso e data viene raccolto', () => {
  const doc = riga('PIE-GEO-UNDO', '✅ osserva: `Source/Tools/RTHexGeometryTool.cpp` @ 2026-08-20');
  const trovati = verdettiDichiarati(doc);
  assert.equal(trovati.length, 1);
  assert.equal(trovati[0].id, 'PIE-GEO-UNDO');
  assert.deepEqual(trovati[0].paths, ['Source/Tools/RTHexGeometryTool.cpp']);
  assert.equal(trovati[0].pattern, null);
  assert.equal(giorno(trovati[0].quando), '2026-08-20');
});

test('🔴 senza `@ data` la dichiarazione e\' incompleta e la riga NON si controlla', () => {
  // La data si dichiara e non si prende da `git blame`: qualunque ritocco alla cella sposterebbe il
  // blame in avanti e farebbe tacere il confronto. Misurato — e' cosi' che la prima stesura falliva la
  // propria falsificazione.
  const doc = riga('PIE-X', '✅ osserva: `a/b.cpp` ~ `Foo`');
  assert.deepEqual(verdettiDichiarati(doc), []);
  assert.equal(leggiOsserva('✅ osserva: `a/b.cpp` ~ `Foo`'), null);
});

test('🔴 l\'alternanza si scrive con la VIRGOLA, e il `|` nasce solo dentro il gate', () => {
  // Un `|` nella cella di stato sposterebbe quale campo e' `$(NF-1)`, cioe' romperebbe il comando
  // canonico del registro. ⚠️ E scriverlo `\|` NON basta, misurato: e' un escape di rendering, e
  // qualunque parser che spezza sul `|` grezzo lo conta lo stesso.
  const cella = '✅ osserva: `a/b.cpp` ~ `FScopedTransaction`, `Modify\\(` @ 2026-08-20';
  const letto = leggiOsserva(cella);
  assert.ok(letto);
  assert.equal(letto.pattern, 'FScopedTransaction|Modify\\(');

  // L'invariante vera: nessun pipe grezzo nella cella di STATO. (Non «sette campi»: il registro ha da
  // sempre righe a 8 e 9 campi che il comando canonico legge correttamente, perche' usa `$(NF-1)`.)
  const campi = riga('PIE-X', cella).split('|');
  assert.equal(campi[campi.length - 2].includes('|'), false);
});

test('piu\' percorsi separati da virgola', () => {
  const letto = leggiOsserva('✅ osserva: `a/b.cpp`, `c/d.h` ~ `Foo` @ 2026-01-02');
  assert.ok(letto);
  assert.deepEqual(letto.paths, ['a/b.cpp', 'c/d.h']);
  assert.equal(letto.pattern, 'Foo');
});

test('🔴 il marcatore e\' il PRIMO glifo, non uno qualunque della prosa', () => {
  // Una cella declassata racconta il proprio giro — «RIVISTA da ✅ a 🟡» — quindi contiene un ✅ in prosa.
  // Una prima stesura cercava il glifo ovunque e contava 124 verdi dove il canonico ne conta 88.
  assert.equal(marcatore('🟡 **RIVISTA da ✅ a 🟡 il 2026-09-23**, e la ragione e\' un commit'), '🟡');
  assert.equal(marcatore('✅ **2026-08-20** poi qualcuno ha scritto ⏳ nella prosa'), '✅');
  assert.equal(marcatore('niente glifi qui'), null);

  const doc = riga('PIE-GEO-GHOST', '🟡 da ✅ a 🟡 osserva: `a.cpp` @ 2026-08-20');
  assert.deepEqual(verdettiDichiarati(doc), []);
});

test('un verdetto ✅ SENZA `osserva:` non e\' un errore: la convenzione si riempie a poco a poco', () => {
  const doc = riga('PIE-BU2', '✅ **2026-08-12** (seduta dell\'autore)');
  assert.deepEqual(verdettiDichiarati(doc), []);
  // Ma la copertura lo conta, cosi' il riempimento e' misurabile invece che opinabile.
  assert.deepEqual(copertura(doc), { conVerdetto: 1, dichiarati: 0 });
});

test('la copertura distingue i dichiarati dal totale dei verdetti', () => {
  const doc = [
    riga('PIE-A', '✅ osserva: `a.cpp` @ 2026-01-01'),
    riga('PIE-B', '✅ nessuna dichiarazione'),
    riga('PIE-C', '🟡 osserva: `c.cpp` @ 2026-01-01'),
    riga('PIE-D', '⏳'),
    'testo che non e\' una riga di tabella, con la parola osserva: `finto.cpp` @ 2026-01-01',
  ].join('\n');
  assert.deepEqual(copertura(doc), { conVerdetto: 2, dichiarati: 1 });
  // Controllo NEGATIVO: la prosa fuori tabella non entra, o il gate segnalerebbe i propri esempi.
  assert.deepEqual(verdettiDichiarati(doc).map((v) => v.id), ['PIE-A']);
});

test('una cella senza backtick dopo `osserva:` non produce un percorso vuoto', () => {
  // Guardia contro il percorso `''`, che `git log -- ''` interpreterebbe come «tutto il repository».
  assert.equal(leggiOsserva('✅ osserva: niente di utile @ 2026-01-01'), null);
  assert.equal(leggiOsserva('✅ osserva: `` @ 2026-01-01'), null);
});

test('🔑 `@ <sha>` è un riferimento esatto: nessuna data, nessun fuso', () => {
  const letto = leggiOsserva('✅ osserva: `a/b.cpp` ~ `Foo` @ 5e0e5c67c');
  assert.ok(letto);
  assert.equal(letto.sha, '5e0e5c67c');
  assert.equal(letto.quando, null);
  assert.equal(letto.pattern, 'Foo');

  // La forma con la data resta com'era, e non dichiara alcuno sha.
  const vecchio = leggiOsserva('✅ osserva: `a/b.cpp` @ 2026-08-20');
  assert.ok(vecchio);
  assert.equal(vecchio.sha, null);
  assert.equal(giorno(vecchio.quando as number), '2026-08-20');

  // Controllo NEGATIVO: una parola qualunque, o un esadecimale troppo corto, dopo `@` non è uno sha.
  assert.equal(leggiOsserva('✅ osserva: `a.cpp` @ domani'), null);
  assert.equal(leggiOsserva('✅ osserva: `a.cpp` @ abc12'), null);
});

test('un verdetto ✅ con `@ sha` viene raccolto', () => {
  const doc = riga('PIE-CAM-X', '✅ osserva: `a/b.cpp` @ 5e0e5c67c');
  const trovati = verdettiDichiarati(doc);
  assert.equal(trovati.length, 1);
  assert.equal(trovati[0].sha, '5e0e5c67c');
});

test('🔴 `modificheDopo` guarda SOLO ciò che viene dopo lo sha, nei due versi', () => {
  // Il caso di #3799: verdetto della sera, codice modificato nel pomeriggio dello stesso giorno. Con la
  // data a mezzogiorno UTC il codice del pomeriggio risultava più nuovo del verdetto; con lo sha sta
  // PRIMA del commit misurato, e il confronto è esatto.
  const dir = mkdtempSync(join(tmpdir(), 'pie-age-'));
  try {
    const git = (data: string, ...a: string[]) =>
      execFileSync(
        'git',
        ['-C', dir, '-c', 'user.name=t', '-c', 'user.email=t@t', '-c', 'commit.gpgsign=false', ...a],
        { encoding: 'utf8', env: { ...process.env, GIT_AUTHOR_DATE: data, GIT_COMMITTER_DATE: data } },
      ).trim();
    const pomeriggio = '2026-10-15T15:00:00Z';
    const sera = '2026-10-15T21:00:00Z';

    git(pomeriggio, 'init', '-q');
    writeFileSync(join(dir, 'a.cpp'), 'int Foo = 1;\n');
    git(pomeriggio, 'add', '.');
    git(pomeriggio, 'commit', '-q', '-m', 'pomeriggio: Foo');
    git(sera, 'commit', '-q', '--allow-empty', '-m', 'sera: il verdetto è dato qui');
    const misurato = git(sera, 'rev-parse', 'HEAD');

    // Nessun commit dopo lo sha: il codice del pomeriggio non è "più nuovo" del verdetto.
    assert.equal(modificheDopo(misurato, 'a.cpp', 'Foo', dir), null);

    // Un commit DOPO che tocca Foo viene trovato; uno che tocca altro no.
    writeFileSync(join(dir, 'a.cpp'), 'int Foo = 2;\n');
    git(sera, 'commit', '-q', '-am', 'dopo: Foo cambia');
    assert.ok(modificheDopo(misurato, 'a.cpp', 'Foo', dir));
    assert.equal(modificheDopo(misurato, 'a.cpp', 'Bar', dir), null);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test('🔴 uno sha che non si risolve NON è un verde: `modificheDopo` lo dichiara', () => {
  // Mai un verde ottenuto perché lo strumento non poteva rispondere.
  assert.equal(modificheDopo('0000000000000000000000000000000000000000', 'a.cpp', null), 'SHA-NON-RISOLTO');
});
