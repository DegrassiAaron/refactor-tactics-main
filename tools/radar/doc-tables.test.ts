import { test } from 'node:test';
import assert from 'node:assert/strict';
import { findBrokenRows, findOrphanRows, findUnbalancedFence } from './doc-tables.ts';

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

// ---------------------------------------------------------------------------------------------
// Cosa distingue un delimitatore da una riga che gli somiglia
// ---------------------------------------------------------------------------------------------

test('`---` non e un delimitatore: e un titolo setext o una linea orizzontale', () => {
  // Il difetto piu' grave della stesura precedente: senza una pipe, `---` veniva accettato come
  // delimitatore, promuoveva la prosa sopra a intestazione e assorbiva le righe sotto — quindi il
  // caso per cui questo controllo esiste, la voce di Decision Log staccata, tornava invisibile.
  assert.deepEqual(
    findOrphanRows(['Prosa qualsiasi', '---', '| **D-190** | orfana |'].join('\n')).map((o) => o.line),
    [3],
  );
  assert.deepEqual(
    findOrphanRows(['| A | B |', '|---|---|', '| 1 | 2 |', '---', '| **D-190** | orfana |'].join('\n'))
      .map((o) => o.line),
    [5],
  );
  // ⚠️ Il caso che gli altri due controlli NON coprono, trovato da una mutazione sopravvissuta: qui
  // l'intestazione ha una pipe e il conteggio delle celle combacia (una e una), quindi a fermare la
  // promozione di `---` a delimitatore resta solo la richiesta della pipe.
  assert.deepEqual(
    findOrphanRows(['| A |', '---', '| orfana |'].join('\n')).map((o) => o.line),
    [1, 3],
  );
});

test('intestazione e delimitatore devono avere lo stesso numero di celle', () => {
  // GFM: «The header row must match the delimiter row in the number of cells. If not, a table will
  // not be recognized.» Senza il confronto il blocco passa per tabella e il markdown lo rende come
  // testo con le pipe a vista — di nuovo il difetto che si vuole vedere.
  assert.deepEqual(findOrphanRows(['| A | B | C |', '|---|---|'].join('\n')).map((o) => o.line), [1, 2]);
  assert.deepEqual(findOrphanRows(['| A | B |', '|---|---|---|'].join('\n')).map((o) => o.line), [1, 2]);
});

test('una intestazione senza nessuna pipe non regge una tabella', () => {
  assert.deepEqual(
    findOrphanRows(['Prosa senza pipe', '| --- | --- |', '| a | b |'].join('\n')).map((o) => o.line),
    [2, 3],
  );
});

test('la tabella prosegue fino alla riga vuota, anche su righe senza pipe', () => {
  // GFM chiude una tabella con una riga VUOTA, non con la prima riga priva di pipe: quella diventa
  // una riga di una cella. Fermarsi prima segnalava come orfana una riga che sta in tabella.
  assert.deepEqual(
    findOrphanRows(['| A | B |', '|---|---|', '| 1 | 2 |', 'continuazione senza pipe', '| 3 | 4 |'].join('\n')),
    [],
  );
});

test('un backtick inline non apre un blocco di codice', () => {
  // CommonMark: la info string di un fence a backtick non puo' contenere backtick. Senza questo
  // controllo `` ```x``` | y `` veniva letto come fence aperto: il gate usciva 1 su un documento
  // valido e la maschera si disattivava per tutto il file.
  const md = ['| A | B |', '|---|---|', '```x``` | y', '', '| orfana |'].join('\n');
  assert.equal(findUnbalancedFence(md), null);
  assert.deepEqual(findOrphanRows(md).map((o) => o.line), [5]);
});

test('findUnbalancedFence riporta la riga di apertura, 1-based', () => {
  assert.equal(findUnbalancedFence(['testo', '```js', 'const x = 1;'].join('\n')), 2);
  assert.equal(findUnbalancedFence(['```js', 'x', '```', '', '```py', 'y', '```'].join('\n')), null);
  // Una tilde non chiude un blocco a backtick: resta aperto, e la riga riportata e' la prima.
  assert.equal(findUnbalancedFence(['```text', '~~~', 'x'].join('\n')), 1);
});

test('i documenti CRLF si comportano come quelli LF', () => {
  // 🔴 Difetto trovato eseguendo il gate, non dai test: in JavaScript `\r` e' un *line terminator*,
  // quindi `.` non lo matcha e un `(.*)$` fallisce su ogni riga CRLF. I documenti di questo repository
  // sono interamente CRLF — la maschera dei fence era inerte sui file veri mentre tutte le fixture,
  // scritte con `\n`, restavano verdi. Ogni fixture qui sopra vale anche in CRLF.
  const crlf = (a: string[]) => a.join('\r\n');

  assert.deepEqual(findOrphanRows(crlf(['```markdown', '| citata | non conta |', '```'])), []);
  assert.equal(findUnbalancedFence(crlf(['testo', '```js', 'const x = 1;'])), 2);
  assert.deepEqual(
    findOrphanRows(crlf(['| A | B |', '|---|---|', '| 1 | 2 |', '', '| staccata |'])).map((o) => o.line),
    [5],
  );
});
