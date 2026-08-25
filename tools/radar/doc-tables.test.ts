import { test } from 'node:test';
import assert from 'node:assert/strict';
import { findBrokenRows, findOrphanRows } from './doc-tables.ts';

test('una riga con una cella in meno rompe la tabella e viene segnalata', () => {
  // Caso reale: `OPEN_DECISIONS.md` BAS-5 fondeva domanda e risposta perche' mancava una pipe.
  const md = [
    '| ID | Domanda | Risposta |',
    '|---|---|---|',
    '| `A-1` | prima domanda | prima risposta |',
    '| `A-2` | seconda domanda e la risposta tutta attaccata |',
  ].join('\n');

  const broken = findBrokenRows(md);

  assert.equal(broken.length, 1);
  assert.equal(broken[0]!.line, 4);
  assert.equal(broken[0]!.cells, 2);      // `A-2` piu' la cella fusa
  assert.equal(broken[0]!.expected, 3);   // ID, Domanda, Risposta
});

test('una pipe ESCAPATA non e un separatore, e la riga non e rotta', () => {
  // `Attack \| Ability \| Overwatch` in `ECO-1`: il markdown le rende come testo. Un contatore che le
  // conta come separatori produce un falso positivo — ed e' l'errore che questo test pinna.
  const md = [
    '| ID | Domanda | Risposta |',
    '|---|---|---|',
    '| `B-1` | copre `Attack \\| Ability \\| Overwatch` | e non dice altro |',
  ].join('\n');

  assert.deepEqual(findBrokenRows(md), []);
});

// ---------------------------------------------------------------------------------------------
// Righe orfane: un blocco di righe `|` che NON e' una tabella
// ---------------------------------------------------------------------------------------------

test('una riga di tabella ISOLATA e orfana: da sola non e una tabella', () => {
  // Caso reale: una voce del Decision Log inserita DOPO la riga vuota che chiude la tabella. Rende
  // come testo letterale con le pipe a vista, e il controllo di larghezza non la vede — un blocco di
  // una riga non ha sorelle con cui confrontarsi.
  const md = [
    '| **D-189** | prima decisione | Accettata |',
    '',
    '| **D-190** | seconda decisione | Accettata |',
    '',
    '## Note',
  ].join('\n');

  const orphans = findOrphanRows(md);

  assert.equal(orphans.length, 2);
  assert.deepEqual(orphans.map((o) => o.line), [1, 3]);
});

test('un header staccato dal suo delimitatore da una riga vuota e orfano', () => {
  // La forma in cui il difetto compare nei documenti importati: fra intestazione e `|---|` si e'
  // infilata una riga vuota, e il markdown smette di vedere una tabella.
  const md = [
    '| Reazione | Trigger | Effetto |',
    '',
    '|---|---|---|',
    '',
    '| **Estinzione** | fuoco vicino | congela |',
  ].join('\n');

  assert.equal(findOrphanRows(md).length, 3);
});

test('una tabella senza righe di dati e valida: header piu delimitatore bastano', () => {
  // Non e' un difetto: GFM rende una tabella vuota. Segnalarla sarebbe il falso positivo che
  // toglie credibilita' al gate.
  const md = ['| Colonna | Altra |', '|---|---|'].join('\n');

  assert.deepEqual(findOrphanRows(md), []);
});

test('una tabella dentro un code fence non e codice da controllare', () => {
  // Un documento che CITA markdown non sta dichiarando una tabella. Senza questa esclusione il
  // controllo nasce con falsi positivi su ogni esempio citato — cinque, misurati in `docs/`.
  const md = [
    'Il frammento da correggere:',
    '',
    '```markdown',
    '| `MaxPromptsPerReaction` | **3** | §5 sorgente; in v0.1 costante',
    '```',
    '',
    'e il resto del discorso.',
  ].join('\n');

  assert.deepEqual(findOrphanRows(md), []);
  assert.deepEqual(findBrokenRows(md), []);
});

// ---------------------------------------------------------------------------------------------
// Code fence: la maschera deve CHIUDERSI, e non deve mai spegnere il controllo
// ---------------------------------------------------------------------------------------------

test('un fence NON chiuso non spegne i controlli fino a fine file', () => {
  // Regressione trovata in code review: mascherare da un fence aperto fino a EOF toglieva a
  // `findBrokenRows` una riga che su `main` vedeva. Un backtick perso disattivava il gate, con
  // `--check` a zero — esattamente il difetto che questo file esiste per impedire.
  const md = [
    '```js',
    'const x = 1;',
    '',
    '| A | B | C |',
    '|---|---|---|',
    '| 1 | 2 |',
    '',
    '| voce isolata |',
  ].join('\n');

  assert.deepEqual(findBrokenRows(md).map((b) => b.line), [6]);
  assert.deepEqual(findOrphanRows(md).map((o) => o.line), [8]);
});

test('una riga isolata DOPO un fence chiuso viene vista', () => {
  // Il buco nella prima verifica di mutazione: la fixture non aveva righe `|` dopo la chiusura,
  // quindi `open = true` (maschera che non si chiude mai) passava tutti i test.
  const md = ['```markdown', '| citata | non conta |', '```', '', '| orfana vera |'].join('\n');

  assert.deepEqual(findOrphanRows(md).map((o) => o.line), [5]);
});

test('un marcatore diverso non chiude il fence', () => {
  // Caso reale nel repository: `~~~~ water ~~~~` dentro un blocco ```text invertiva la maschera,
  // e da li' in poi il file veniva letto al contrario — contenuto saltato, esempi controllati.
  const md = ['```text', '~~~~ water ~~~~', '| A | B |', '```', '', '| X | Y |', '|---|---|'].join('\n');

  assert.deepEqual(findOrphanRows(md), []);
});

// ---------------------------------------------------------------------------------------------
// GFM: le pipe ai bordi sono opzionali
// ---------------------------------------------------------------------------------------------

test('una tabella senza pipe ai bordi e valida e non va segnalata', () => {
  // GFM rende tutte e tre. Segnalarle sarebbe il falso positivo che toglie credibilita' al gate:
  // l'euristica «solo righe che cominciano con |» produceva falsi NEGATIVI, innocui; trasformarla
  // in falsi allarmi e' un peggioramento.
  for (const rows of [
    ['| A | B |', '--- | ---', '| 1 | 2 |'],
    ['A | B', '|---|---|', '| 1 | 2 |'],
    ['| A | B |', '|---|---|', '1 | 2', '| 3 | 4 |'],
  ]) {
    assert.deepEqual(findOrphanRows(rows.join('\n')), [], rows[1]);
  }
});

test('un delimitatore ha almeno un trattino per cella, e le celle vuote non bastano', () => {
  // `| - | - |` E' valido per GFM: la spec chiede celle il cui unico contenuto siano trattini e
  // due punti, senza un minimo di tre. `|   |   |` invece non lo e', e va trattato da riga di dati.
  assert.deepEqual(findOrphanRows(['| **D-1** | x |', '| - | - |'].join('\n')), []);
  assert.deepEqual(
    findOrphanRows(['| **D-1** | x |', '|   |   |'].join('\n')).map((o) => o.line),
    [1, 2],
  );
});
