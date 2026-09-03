import { test } from 'node:test';
import assert from 'node:assert/strict';
import { segnala, raccogli, CAMPO_LETTI } from './scenario-notes.ts';

const conAttesa = (nota: string, valore: number, campo = '_nota') => ({
  [campo]: nota,
  turns: [{ expect: [{ type: 'UnitHpEquals', unit: 'B1', value: valore }] }],
});

/** Il caso reale che ha aperto `#2049`: la nota dice 98, il file asserisce 103. */
test('una nota che cita un totale diverso da cio che il file asserisce viene segnalata', () => {
  const righe = segnala('X.json', conAttesa("la riduzione non vale: 120 - 22 = 98, non 113.", 103));

  assert.equal(righe.length, 1);
  assert.equal(righe[0]!.citato, 98);
  assert.equal(righe[0]!.atteso, 103);
  assert.equal(righe[0]!.delta, 5);
});

/** 🔑 Anti-vacuita': con la nota CORRETTA non deve segnalare niente, o segnalerebbe sempre. */
test('la stessa nota, corretta, non produce nessuna segnalazione', () => {
  assert.equal(segnala('X.json', conAttesa("120 - (22 - 5 di scudo base) = 103.", 103)).length, 0);
});

/**
 * 🔴 **Ogni campo `_` e' prosa, non solo `_nota*`** — ed e' il difetto che ha reso questo strumento
 * cieco su 19 righe: stavano in `_errata_scudo`, `_oracolo` e `_turno`, cioe' nei campi che dicono
 * *cosa il file deve produrre*.
 */
test('la prosa e ogni campo che inizia con _, non solo _nota', () => {
  for (const campo of ['_errata_scudo', '_oracolo', '_turno', '_assertion', '_intento']) {
    const righe = segnala('X.json', conAttesa('il conto vero e 98.', 103, campo));
    assert.equal(righe.length, 1, `${campo} non e' stato letto`);
    assert.equal(righe[0]!.campo, campo);
  }
});

/**
 * 🔴 Il rumore, **un ramo alla volta**: il titolo precedente prometteva cinque categorie e ne misurava
 * una sola, quindi cinque sesti del filtro potevano marcire in verde.
 */
for (const [nome, testo] of [
  ['decisione', 'non e raggiungibile (D-098, uscita B)'],
  ['issue', 'il flag non era copiato (#98)'],
  ['checkpoint', 'da CP 98.2 la guardia copre il davanti'],
  ['data', 'misurato il 2026-08-98 su un altro checkout'],
  ['cella', 'il bersaglio arretra su (q=98,r=1)'],
] as const) {
  test(`un numero di ${nome} non e un HP`, () => {
    assert.deepEqual(segnala('X.json', conAttesa(testo, 103)), []);
  });
}

/** Un numero che COINCIDE con un'attesa e' la prosa che fa il suo mestiere: silenzio. */
test('un numero uguale a un attesa non viene segnalato', () => {
  assert.equal(segnala('X.json', conAttesa('Wraith resta a 90 pieni.', 90)).length, 0);
});

/**
 * ⚠️ **Il buco dichiarato, misurato invece che promesso.** Un valore INTERMEDIO sta sopra il totale
 * finale, quindi la finestra stretta non lo vede — ed e' cosi' che
 * `Spec/Cover/TemporaryCoverExpires.json` teneva nascosti tre numeri sbagliati su quattro.
 */
test('un valore intermedio sopra l attesa e invisibile alla finestra stretta e visibile a --wide', () => {
  const scenario = conAttesa('T1 colpo pieno (21) -> 99.', 76);

  assert.equal(segnala('X.json', scenario, /*wide=*/ false).length, 0);
  const larga = segnala('X.json', scenario, /*wide=*/ true);
  assert.equal(larga.length, 1);
  assert.equal(larga[0]!.citato, 99);
  assert.equal(larga[0]!.delta, -23);
});

/**
 * 🔑 **La firma stretta e' `multiplo di 5` E `fino a 20`, e sono due vincoli distinti.** Senza questo
 * test si potevano togliere entrambi senza che niente diventasse rosso.
 */
test('la finestra stretta pretende un multiplo di 5 e non va oltre 20', () => {
  // Δ3: non e' multiplo di 5 -> muta (ed e' il caso `Deflection`, Δ2, che il corpus aveva nascosto).
  assert.equal(segnala('X.json', conAttesa('scende a 100.', 103)).length, 0);
  // Δ25: multiplo di 5 ma oltre la finestra -> muta.
  assert.equal(segnala('X.json', conAttesa('scende a 78.', 103)).length, 0);
  // Δ10: dentro entrambi -> parla. E' il caso a DUE colpi che la passata del 2026-08-30 mancava.
  assert.equal(segnala('X.json', conAttesa('scende a 93.', 103)).length, 1);
});

/**
 * 🔑 **Il rumore si cancella conservando la lunghezza**, e questa e' la sola ragione per cui gli offset
 * calcolati su `pulito` possono tagliare `testo`. Sostituirlo con stringa vuota sfaserebbe ogni
 * contesto stampato — il report esiste per essere letto, quindi il contesto sbagliato lo svuota.
 */
test('il contesto stampato viene dal testo VERO e resta allineato al numero', () => {
  // 🔑 **Il rumore sta LONTANO dal numero**, e non e' un dettaglio della fixture: con il rumore
  // adiacente, toglierlo sposta la finestra di quanto la finestra stessa e' larga, e il numero ci resta
  // dentro per caso — misurato, la prima stesura di questo test passava anche con la mutazione. Fra i
  // due c'e' un riempimento piu' lungo della semifinestra (55), quindi lo sfasamento porta il taglio
  // fuori bersaglio.
  const rumore = '(D-074) e (D-089) e (D-104) e (D-111) e (D-222) e (D-333) e (D-444) e (D-555)';
  const riempimento = 'riga di riempimento che non dice niente e serve solo a distanziare, ripetuta due volte per superare la semifinestra da cinquantacinque caratteri, ancora, e ancora';
  // E un rumore CORTO vicino al numero, per l'altra meta': dimostra che il contesto viene dal testo
  // vero e non da quello con il rumore cancellato. Le due mutazioni sono diverse e vogliono due
  // ancore diverse — quella lontana per lo sfasamento, quella vicina per la sorgente.
  const nota = `${rumore} ${riempimento} poi (D-999) il conto giusto era 98 e non altro. ${riempimento}`;
  const righe = segnala('X.json', conAttesa(nota, 103));

  assert.equal(righe.length, 1);
  // La finestra e' centrata sul numero segnalato: se gli offset si sfasassero, `98` uscirebbe.
  assert.ok(righe[0]!.contesto.includes('era 98 e non altro'), righe[0]!.contesto);
  // E viene dal testo VERO, non da quello con il rumore cancellato.
  assert.ok(righe[0]!.contesto.includes('D-999'), righe[0]!.contesto);
});

/** Uno scenario senza attese di HP non ha un metro: non si inventa. */
test('senza UnitHpEquals non si segnala niente', () => {
  assert.deepEqual(
    segnala('X.json', { _nota: 'Riktor scende a 110.', turns: [{ expect: [{ type: 'TurnsCompleted', value: 3 }] }] }),
    [],
  );
});

/** Le note e le attese si raccolgono a qualunque profondita': gli scenari annidano turni e squadre. */
test('la raccolta scende nell albero invece di fermarsi al primo livello', () => {
  const { note, attese } = raccogli({
    a: { b: [{ _nota_x: 'testo', expect: [{ type: 'UnitHpEquals', value: 7 }] }] },
  });

  assert.deepEqual(note, [['_nota_x', 'testo']]);
  assert.deepEqual(attese, [7]);
});

// ---------------------------------------------------------------- ASSOLUZIONI (#2161)
//
// 🔴 Il difetto che il meccanismo chiude: `--check` usciva **1** su un corpus che il docstring di questo
// stesso file dichiarava pulito. Un gate rosso quando tutto va bene non è un gate — è rumore che si impara
// a scavalcare, e il 2026-09-03 ha prodotto una conclusione sbagliata sullo stato del repository in chi
// stava proprio verificando quali gate girassero.

/** Il caso base: un numero dichiarato già letto porta il motivo con sé e smette di essere «da leggere». */
test('un numero assolto con motivo resta segnalato ma marcato, non sparisce', () => {
  const righe = segnala('X.json', {
    ...conAttesa('Riktor scende a 115 nello scenario gemello.', 120),
    [CAMPO_LETTI]: { _nota: '115 è del gemello, non di questo file' },
  });

  assert.equal(righe.length, 1);
  assert.equal(righe[0]!.assolto, '115 è del gemello, non di questo file');
  assert.equal(righe[0]!.mutaSenzaMotivo, undefined);
});

/**
 * 🔑 **La trappola principale del meccanismo, e la ragione per cui `CAMPO_LETTI` esce dalla scansione.**
 *
 * Un motivo spiega un numero, quindi **contiene** quel numero. Senza l'esclusione ogni assoluzione
 * genererebbe il rilievo che assolve, e il gate resterebbe rosso dichiarando di essersi assolto — il che
 * è peggio del rosso di prima, perché sembrerebbe un difetto dei dati invece che dello strumento.
 */
test('il motivo di un assoluzione non genera segnalazioni proprie', () => {
  const righe = segnala('X.json', {
    ...conAttesa('Riktor scende a 115 nello scenario gemello.', 120),
    [CAMPO_LETTI]: { _nota: 'il gemello asserisce 115, e anche 110 e 105 sono suoi' },
  });

  // Una sola riga: quella della nota. I tre numeri del motivo non ne producono nessuna.
  assert.equal(righe.length, 1);
  assert.equal(righe[0]!.citato, 115);
});

/** ⛔ Un'assoluzione MUTA non assolve: senza il motivo obbligatorio il meccanismo sarebbe un interruttore. */
test('un assoluzione senza motivo non assolve e si dichiara', () => {
  const righe = segnala('X.json', {
    ...conAttesa('Riktor scende a 115 nello scenario gemello.', 120),
    [CAMPO_LETTI]: { _nota: '   ' },
  });

  assert.equal(righe.length, 1);
  assert.equal(righe[0]!.assolto, undefined);
  assert.equal(righe[0]!.mutaSenzaMotivo, true);
});

/** Lo stesso vale per un motivo che non è nemmeno una stringa: `null`, un numero, un oggetto. */
test('un motivo che non e una stringa vale come muto', () => {
  const righe = segnala('X.json', {
    ...conAttesa('Riktor scende a 115 nello scenario gemello.', 120),
    [CAMPO_LETTI]: { _nota: 42 },
  });

  assert.equal(righe[0]!.mutaSenzaMotivo, true);
});

/**
 * 🔑 **L'assoluzione è per CAMPO, non per file.** Un'esenzione che valesse per l'intero scenario sarebbe il
 * modo in cui il gate smette di guardare proprio dove ha già trovato qualcosa una volta.
 */
test('assolvere un campo non assolve gli altri campi dello stesso file', () => {
  const righe = segnala('X.json', {
    _nota: 'Riktor scende a 115 nello scenario gemello.',
    _nota_coppia: 'e nel confronto si legge 110.',
    turns: [{ expect: [{ type: 'UnitHpEquals', unit: 'B1', value: 120 }] }],
    [CAMPO_LETTI]: { _nota: '115 è del gemello' },
  });

  assert.equal(righe.length, 2);
  assert.equal(righe.find((r) => r.campo === '_nota')!.assolto, '115 è del gemello');
  assert.equal(righe.find((r) => r.campo === '_nota_coppia')!.assolto, undefined);
});

/** Anti-vacuità: senza il campo di assoluzione nulla cambia rispetto al comportamento storico. */
test('senza il campo di assoluzione la segnalazione resta nuda', () => {
  const righe = segnala('X.json', conAttesa('Riktor scende a 115 nello scenario gemello.', 120));

  assert.equal(righe.length, 1);
  assert.equal(righe[0]!.assolto, undefined);
  assert.equal(righe[0]!.mutaSenzaMotivo, undefined);
});

/** La raccolta legge le assoluzioni a qualunque profondità, come fa con note e attese. */
test('le assoluzioni si raccolgono senza entrare nella scansione delle note', () => {
  const { note, letti } = raccogli({
    _nota: 'testo con 115',
    [CAMPO_LETTI]: { _nota: 'motivo con 115 e 120' },
    turns: [{ expect: [{ type: 'UnitHpEquals', value: 120 }] }],
  });

  assert.deepEqual(note, [['_nota', 'testo con 115']]);
  assert.deepEqual([...letti], [['_nota', 'motivo con 115 e 120']]);
});
