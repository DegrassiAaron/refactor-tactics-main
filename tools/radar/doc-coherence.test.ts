/** ⚠️ **Ogni asserzione ha qui il proprio controllo positivo**, e non e' una formalita': il difetto
 *  che #3371 registra e' un'asserzione negativa creduta senza mai averla vista trovare qualcosa —
 *  *«nessuna issue nomina `U6`»*, fondata su una ricerca che dava **0 anche per `U5`**, che un
 *  convocatore ce l'ha. Un test che dimostra solo il verde dimostra una cecita' indistinguibile da una
 *  conferma.
 *
 *  Percio' per ognuna delle cinque asserzioni ci sono **due** test: il caso pulito che non produce
 *  nulla, e il caso col difetto dentro, che lo produce **nominandolo**. */
import { test } from 'node:test';
import assert from 'node:assert/strict';

import {
  registroPie,
  totaliPie,
  contraddizioniPie,
  nonDichiaraStato,
  daRigiudicare,
  sedute,
  seduteOrfane,
  nominaSeduta,
  cardinalita,
  divergenze,
} from './doc-coherence.ts';

// ---------------------------------------------------------------------------------------------
// Il registro PIE si legge dalla penultima cella, come il comando canonico
// ---------------------------------------------------------------------------------------------

test('la cella di stato e il penultimo campo, anche quando la riga ne porta piu di sette', () => {
  // `PIE-B` porta un `|` in una cella PRECEDENTE — innocuo, dice il registro, perche' `$(NF-1)` non si
  // sposta. Una lettura per indice fisso leggerebbe il campo sbagliato.
  const md = [
    '| **PIE-A** | che cosa | come | atteso | ✅ **2026-01-01** — fatto |',
    '| **PIE-B** | che cosa | `a|b` | atteso | ❌ **2026-01-02** — rotto, [#7](https://github.com/o/r/issues/7) |',
    'una riga che non e una voce',
  ].join('\n');

  const voci = registroPie(md);

  assert.deepEqual(voci.map((v) => v.id), ['PIE-A', 'PIE-B']);
  assert.deepEqual(voci.map((v) => v.glifo), ['✅', '❌']);
  assert.deepEqual(voci[1]!.issues, [7]);
});

// ---------------------------------------------------------------------------------------------
// A1 — i totali dichiarati contro il ricalcolo
// ---------------------------------------------------------------------------------------------

const REGISTRO_MINIMO = [
  '**2 voci**: ✅ **1 verdi** · 🟡 **1 parziali** · ❌ **0 fallite** · ⏳ **0 aperte**.',
  '',
  '| **PIE-A** | x | y | z | ✅ **2026-01-01** |',
  '| **PIE-B** | x | y | z | 🟡 **2026-01-02** |',
].join('\n');

test('A1 — i totali che coincidono col ricalcolo non producono niente', () => {
  const t = totaliPie(REGISTRO_MINIMO);
  assert.deepEqual(t.dichiarati, t.contati);
  assert.deepEqual(t.senzaMarcatore, []);
});

test('A1 controllo positivo — una voce aggiunta senza toccare la riga dei totali viene vista', () => {
  // E' il difetto misurato sul registro vero: ogni passata aggiunge il proprio blocco e la riga dei
  // totali resta indietro. Il 2026-09-23 lo scarto accumulato era di 23 voci.
  const mutato = REGISTRO_MINIMO + '\n| **PIE-C** | x | y | z | ⏳ |';
  const t = totaliPie(mutato);
  assert.equal(t.contati.voci, 3);
  assert.equal(t.dichiarati!.voci, 2);
  assert.notDeepEqual(t.dichiarati, t.contati);
});

test('A1 controllo positivo — una cella di stato senza marcatore viene nominata', () => {
  const mutato = REGISTRO_MINIMO + '\n| **PIE-C** | x | y | z | da decidere |';
  assert.deepEqual(totaliPie(mutato).senzaMarcatore, ['PIE-C']);
});

// ---------------------------------------------------------------------------------------------
// A3 — una issue attribuisce a una voce uno stato che il registro non le da'
// ---------------------------------------------------------------------------------------------

const REGISTRO = (voce: string) => (voce === 'PIE-HEXPLAY-6' || voce === 'PIE-HEXPLAY-8' ? '✅' : undefined);

test('A3 — una issue che concorda col registro non produce niente', () => {
  const body = 'Le due voci sono a posto: `PIE-HEXPLAY-6` ✅ e `PIE-HEXPLAY-8` ✅.';
  assert.deepEqual(contraddizioniPie(body, 1, REGISTRO), []);
});

test('A3 controllo positivo — il caso REALE di #38, che vive dentro un blockquote', () => {
  // ⛔ Questo e' il test che vieta di ereditare il filtro sul blockquote da `issue-refs.ts`: la «DoD
  // viva» di una issue e' un blockquote, e il difetto per cui A3 esiste sta li' dentro.
  const body =
    '> 2. **Le quattordici voci sono ✅** — 🔴 **Due non sono verdi**: `PIE-HEXPLAY-6` e\' ❌ e aspetta' +
    ' lavoro; `PIE-HEXPLAY-8` e\' 🟡 e aspetta una decisione.';

  const trovate = contraddizioniPie(body, 38, REGISTRO);

  assert.deepEqual(
    trovate.map((d) => `${d.voce} ${d.detto}/${d.registro}`),
    ['PIE-HEXPLAY-6 ❌/✅', 'PIE-HEXPLAY-8 🟡/✅'],
  );
});

test('A3 — il glifo appartiene all ultimo nome che lo precede, non a quello di quaranta caratteri prima', () => {
  // Caso reale di #2476: con una finestra a lunghezza fissa il 🟡 finirebbe attribuito a `-6`, che nel
  // registro e' ✅ come `-8`, producendo DUE segnalazioni dove ce n'e' una sola vera.
  const body = '> ∴ resta `PIE-HEXPLAY-6` — piu\' `PIE-HEXPLAY-8` 🟡, che aspetta una decisione.';

  const trovate = contraddizioniPie(body, 2476, REGISTRO);

  assert.deepEqual(trovate.map((d) => d.voce), ['PIE-HEXPLAY-8']);
});

test('A3 — una casella di DoD e un traguardo, e il testo barrato una ritrattazione', () => {
  assert.equal(nonDichiaraStato('- [ ] la voce `PIE-HEXPLAY-6` torna ✅ solo cosi'), true);
  assert.equal(nonDichiaraStato('~~`PIE-HEXPLAY-6` 🟡 · `-8` ⏳~~'), true);
  assert.equal(nonDichiaraStato('> `PIE-HEXPLAY-6` e\' ❌'), false);

  assert.deepEqual(contraddizioniPie('- [ ] `PIE-HEXPLAY-6` torna ✅', 1, REGISTRO), []);
});

test('A3 — una voce che il registro non conosce non e una contraddizione', () => {
  assert.deepEqual(contraddizioniPie('`PIE-INVENTATA` ❌', 1, REGISTRO), []);
});

// ---------------------------------------------------------------------------------------------
// A4 — un verdetto ❌ o 🟡 la cui causa e' stata chiusa
// ---------------------------------------------------------------------------------------------

/** La riga vera di `PIE-HEX-MODE-H`, ridotta: cita **#931 e poi #996**, e solo la prima e' chiusa. */
const MODE_H =
  '| **PIE-HEX-MODE-H** | snap del gizmo | allestimento | atteso | ❌ **2026-08-15** — due comportamenti.' +
  ' Aperta come [#931](https://github.com/o/r/issues/931). Il sintomo 2 esce in' +
  ' [#996](https://github.com/o/r/issues/996): la voce resta ❌ finche quello non e chiuso |';

const STATO = (n: number) => (n === 931 ? ('CLOSED' as const) : ('OPEN' as const));

test('A4 controllo positivo — il caso REALE di PIE-HEX-MODE-H', () => {
  const trovati = daRigiudicare(registroPie(MODE_H), STATO);

  assert.deepEqual(trovati.map((d) => `${d.voce} #${d.causa}`), ['PIE-HEX-MODE-H #931']);
});

test('A4 — «tutte le issue citate chiuse» NON avrebbe trovato PIE-HEX-MODE-H', () => {
  // La falsificazione del predicato alternativo, che sul registro vero ne trova 6 invece di 9 e manca
  // proprio questo. Il test lo dimostra qui, dove il caso e' leggibile.
  const voce = registroPie(MODE_H)[0]!;
  assert.deepEqual(voce.issues, [931, 996]);
  assert.equal(voce.issues.every((n) => STATO(n) === 'CLOSED'), false);
  assert.equal(STATO(voce.issues[0]!), 'CLOSED');
});

test('A4 — una causa ancora aperta, una voce verde e una senza issue non si segnalano', () => {
  const md = [
    '| **PIE-A** | x | y | z | ❌ aperta come [#5](https://github.com/o/r/issues/5) |',
    '| **PIE-B** | x | y | z | ✅ chiusa da [#931](https://github.com/o/r/issues/931) |',
    '| **PIE-C** | x | y | z | 🟡 nessuna issue citata |',
  ].join('\n');

  assert.deepEqual(daRigiudicare(registroPie(md), STATO), []);
});

/** La stessa riga, con il marcatore che dichiara la voce gia' istruita. */
const MODE_H_DICHIARATA = MODE_H.replace(
  '❌ **2026-08-15**',
  '❌ ⛔ **DICHIARATA NON RIGIUDICABILE il 2026-09-28**: il bloccante non e caduto, e stato instradato. **2026-08-15**',
);

test('A4 — una voce DICHIARATA non rigiudicabile esce, e la sua gemella non dichiarata resta', () => {
  // Le due righe differiscono per il solo marcatore: e la mutazione che prova che a farle uscire e
  // quello, non un'altra differenza del testo.
  assert.deepEqual(daRigiudicare(registroPie(MODE_H), STATO).map((d) => d.voce), ['PIE-HEX-MODE-H']);
  assert.deepEqual(daRigiudicare(registroPie(MODE_H_DICHIARATA), STATO), []);
});

test('A4 — il marcatore non copre una voce la cui causa e ancora APERTA: non e un interruttore', () => {
  // Senza questo, «dichiarata» diventerebbe un modo per far tacere il gate su qualunque voce. Qui la
  // causa e aperta, quindi la voce non era segnalata nemmeno prima: il marcatore non cambia nulla, ed e
  // esattamente cio' che deve fare.
  const aperta =
    '| **PIE-Z** | x | y | z | ❌ ⛔ **DICHIARATA NON RIGIUDICABILE** — aperta come [#5](https://github.com/o/r/issues/5) |';
  assert.deepEqual(daRigiudicare(registroPie(aperta), STATO), []);
  const senza = aperta.replace('⛔ **DICHIARATA NON RIGIUDICABILE** — ', '');
  assert.deepEqual(daRigiudicare(registroPie(senza), STATO), []);
});

test('A4 — una voce DIFFERITA oltre la release esce, e la sua gemella non differita resta', () => {
  const differita = MODE_H.replace(
    '❌ **2026-08-15**',
    '❌ ⛔ **DIFFERITA OLTRE LA v0.1**: la meta residua non e nel subset di release. **2026-08-15**',
  );
  assert.deepEqual(daRigiudicare(registroPie(MODE_H), STATO).map((d) => d.voce), ['PIE-HEX-MODE-H']);
  assert.deepEqual(daRigiudicare(registroPie(differita), STATO), []);
});

test('A4 — una voce del subset RELEASE-V01 non puo dichiararsi differita', () => {
  // La guardia che impedisce al marcatore di diventare un interruttore sul perimetro di consegna: se
  // una voce blocca `G9`, differirla e' una decisione di scope e non una nota in una cella.
  const nelSubset =
    // ⚠️ Il tag sta nella STESSA cella del nome: e' la forma che il comando canonico di `G9` conta.
    '| **PIE-X** `RELEASE-V01` | y | z | ❌ ⛔ **DIFFERITA OLTRE LA v0.1** — aperta come [#931](https://github.com/o/r/issues/931) |';
  const trovati = daRigiudicare(registroPie(nelSubset), STATO);
  assert.deepEqual(trovati.map((d) => d.voce), ['PIE-X']);
});

test('A4 — una seduta che ha EMESSO il verdetto non e la causa del difetto', () => {
  // Il caso reale del 2026-09-28: chiudendo #3378 tre voci appena giudicate sono rientrate in A4,
  // perche' il loro verdetto la citava per prima. La seduta non e' un bloccante caduto.
  const conSeduta =
    '| **PIE-Y** | x | y | z | ❌ **RIGIUDICATA il 2026-09-28** (seduta [#3378](https://github.com/o/r/issues/3378)): il residuo e altro |';
  const chiusa = (n: number) => (n === 3378 || n === 931 ? ('CLOSED' as const) : ('OPEN' as const));
  assert.deepEqual(daRigiudicare(registroPie(conSeduta), chiusa), []);

  // ⛔ E la MUTAZIONE che lo giustifica: la stessa riga senza la parola «seduta» torna a essere
  //    segnalata. E' quella parola a fare la differenza, non un'altra proprieta' del testo.
  const senzaParola = conSeduta.replace('(seduta [#3378]', '([#3378]');
  assert.deepEqual(daRigiudicare(registroPie(senzaParola), chiusa).map((d) => d.voce), ['PIE-Y']);
});

test('A4 — una seduta citata in un esito SUPERATO non mette a tacere il verdetto corrente', () => {
  // Il limite dichiarato: si guarda solo cio' che sta prima del primo marcatore di cronaca.
  const cronaca =
    '| **PIE-W** | x | y | z | ❌ aperta come [#931](https://github.com/o/r/issues/931) ⏻ *Esito precedente:* giudicata in (seduta [#3378](https://github.com/o/r/issues/3378)) |';
  const chiusa = (n: number) => (n === 3378 || n === 931 ? ('CLOSED' as const) : ('OPEN' as const));
  assert.deepEqual(daRigiudicare(registroPie(cronaca), chiusa).map((d) => d.voce), ['PIE-W']);
});

// ---------------------------------------------------------------------------------------------
// A2 — una seduta critica che nessuno convoca e che nessuno ha eseguito
// ---------------------------------------------------------------------------------------------

const YAML = [
  'sessions:',
  '  - id: U5',
  '    title: bot e HUD',
  '    critical: true',
  '    issues: []',
  '    verifies:',
  '      - PIE-A',
  '      - PIE-B',
  '    artifacts: []',
  '  - id: U6',
  '    title: nessuno la chiama',
  '    critical: true',
  '    issues: []',
  '    verifies:',
  '      - PIE-B',
  '    artifacts: []',
  '  - id: U7',
  '    title: convocata dal proprio campo',
  '    critical: true',
  '    issues: [1719]',
  '    verifies: []',
  '    artifacts: []',
  '  - id: U8',
  '    title: non critica',
  '    critical: false',
  '    issues: []',
  '    verifies: []',
  '    artifacts: []',
].join('\n');

test('il parser minimale legge id, critical, issues, verifies e artifacts nelle due forme', () => {
  const s = sedute(YAML);
  assert.deepEqual(s.map((x) => x.id), ['U5', 'U6', 'U7', 'U8']);
  assert.deepEqual(s.map((x) => x.critical), [true, true, true, false]);
  assert.deepEqual(s[0]!.verifies, ['PIE-A', 'PIE-B']);
  assert.deepEqual(s[2]!.issues, [1719]);
});

test('A2 controllo positivo — il predicato NON e cieco: U5 e nominata, U6 no', () => {
  // ⛔ E' il controllo che il referto che apre #3371 non aveva fatto: una ricerca che risponde 0 su U6
  // non dice niente finche non si e visto che risponde >0 su un caso dove il convocatore ESISTE.
  const corpi = ['Seduta U5 — Bot e HUD: sei residui', 'un altro testo che non nomina nessuna seduta'];

  assert.equal(corpi.some((t) => nominaSeduta(t, 'U5')), true);
  assert.equal(corpi.some((t) => nominaSeduta(t, 'U6')), false);
  // e la forma con i backtick, che e quella usata nei corpi
  assert.equal(nominaSeduta('la seduta `U6` produce il verdetto', 'U6'), true);
  // ⚠️ non basta la sottostringa: `U6` non e nominata da `U60`
  assert.equal(nominaSeduta('seduta U60 e un altra cosa', 'U6'), false);
});

test('A2 — solo la seduta che nessuno nomina e che nessuno ha eseguito viene segnalata', () => {
  const verdetti = new Map([['PIE-A', '✅'], ['PIE-B', '⏳']]);
  const orfane = seduteOrfane(
    sedute(YAML),
    (id) => nominaSeduta('Seduta U5 — bot e HUD', id),
    (v) => verdetti.get(v),
    () => false,
  );

  // U5 e nominata da una issue; U7 ha il proprio `issues:`; U8 non e critica.
  assert.deepEqual(orfane.map((o) => o.id), ['U6']);
});

test('A2 — una seduta eseguita non e orfana, anche se nessuno la nomina', () => {
  const verdetti = new Map([['PIE-A', '✅'], ['PIE-B', '❌']]);
  const orfane = seduteOrfane(sedute(YAML), () => false, (v) => verdetti.get(v), () => false);
  assert.deepEqual(orfane.map((o) => o.id), []);
});

test('A2 — un artefatto tracciato vale come marcatore di esecuzione', () => {
  const yaml = YAML.replace('  - id: U6\n    title: nessuno la chiama\n    critical: true\n    issues: []\n    verifies:\n      - PIE-B\n    artifacts: []',
    '  - id: U6\n    title: nessuno la chiama\n    critical: true\n    issues: []\n    verifies:\n      - PIE-B\n    artifacts:\n      - docs/x.md');
  const orfane = seduteOrfane(sedute(yaml), () => false, () => '⏳', (p) => p === 'docs/x.md');
  assert.equal(orfane.some((o) => o.id === 'U6'), false);
});

// ---------------------------------------------------------------------------------------------
// A5 — le tre cardinalita' della roadmap
// ---------------------------------------------------------------------------------------------

const ROADMAP_PULITA = [
  '## 3. Epic della v0.1',
  '',
  '| Epic | Titolo | Priorità | CP | Perché |',
  '|---|---|---|---|---|',
  '| **E1** | uno | **P0** | 2 | perche |',
  '| **E21** | ventuno | P1 | 1 | perche |',
  '',
  '**Totale: 2 epic, 3 checkpoint**',
  '',
  '## 5. Epic in dettaglio',
  '',
  '| **1.1** | obiettivo | dod | test |',
  '| **1.2** | obiettivo | dod | test |',
  '| **E21.1** | obiettivo | dod | test |',
].join('\n');

test('A5 — le due forme di id contano come lo stesso oggetto, e le tre cardinalita coincidono', () => {
  const c = cardinalita(ROADMAP_PULITA);
  assert.equal(c.somma, 3);
  assert.equal(c.unici, 3);
  assert.deepEqual(c.totale, { epic: 2, cp: 3 });
  assert.deepEqual(divergenze(c), []);
});

test('A5 controllo positivo — un checkpoint aggiunto in §5 e non in §3 viene nominato', () => {
  const mutata = ROADMAP_PULITA + '\n| **1.3** | obiettivo | dod | test |';
  const c = cardinalita(mutata);
  assert.equal(c.unici, 4);
  assert.deepEqual(divergenze(c), [{ epic: 'E1', dichiarati: 2, trovati: 3 }]);
});

test('A5 controllo positivo — una riga che RIUSA un id gia contato non lo conta due volte', () => {
  // E' il caso reale di E47, che porta una seconda tabella — l'allocazione delle track — le cui righe
  // riaprono con `| **E47.n** |`. Contando le RIGHE se ne leggono 13 dove i checkpoint sono 7.
  const conSecondaTabella = ROADMAP_PULITA + '\n| **E21.1** | track | stato | nota |';
  const c = cardinalita(conSecondaTabella);
  assert.equal(c.unici, 3);
  assert.deepEqual(divergenze(c), []);
});

test('A5 controllo positivo — leggendo solo `| **N.M** |`, E21 risulterebbe a zero', () => {
  // La falsificazione della forma proposta nel corpo di #3371: il prefisso e obbligatorio da §4.1, e
  // un criterio che non lo accetta segnala come vuote le epic nate dopo.
  const soloNonPrefissati = ROADMAP_PULITA.split('\n').filter((l) => !l.startsWith('| **E21.1**')).join('\n');
  const c = cardinalita(soloNonPrefissati);
  assert.deepEqual(divergenze(c), [{ epic: 'E21', dichiarati: 1, trovati: 0 }]);
});

test('A5 — il totale scritto che non segue la somma viene nominato', () => {
  const mutata = ROADMAP_PULITA.replace('**Totale: 2 epic, 3 checkpoint**', '**Totale: 2 epic, 9 checkpoint**');
  const c = cardinalita(mutata);
  assert.deepEqual(c.totale, { epic: 2, cp: 9 });
  assert.notEqual(c.totale!.cp, c.somma);
});
