/** Confronta i numeri citati nella PROSA di uno scenario con ciò che il file stesso ASSERISCE.
 *
 *  Uso:  node tools/radar/scenario-notes.ts [--check] [--wide]
 *
 *  Il problema che chiude, ed è **prosa contro misura nello stesso file**: un `_nota` dice
 *  *«120 - 22 = 98»* e dieci righe più sotto lo stesso file asserisce **103**. Il corpus è verde —
 *  nessun `expect` è sbagliato — ma chi apre il file per capire cosa dovrebbe succedere legge il numero
 *  sbagliato, e lo copia. È il vettore misurato da `#1904`: il `_nota` di `Combat.BasicAttack` ha
 *  propagato un totale pre-`D-224` in tre documenti a valle.
 *
 *  La causa di ogni scarto trovato finora è `D-224`: lo scudo base assorbe **5 punti per turno** sul solo
 *  danno `Direct`, e non era mai entrato nella prosa.
 *
 *  ⚠️ **Ordina, non decide.** Ogni riga va letta. Sul corpus di oggi le **sei** righe che restano dopo
 *  `#2049` sono tutte legittime: due riferimenti incrociati a un altro scenario, una citazione storica di
 *  ciò che una versione precedente asseriva, un HP di partenza, un valore di danno e un controfattuale.
 *
 *  ## 🔴 Perché guarda OGNI campo che inizia con `_`, e non solo `_nota*`
 *
 *  La prima stesura leggeva solo `_nota*`. **Misurato il 2026-09-02**: con quel filtro il corpus dava 6
 *  righe; leggendo tutti i campi `_` ne dava **25**. Le altre 19 stavano in `_errata_scudo` (15 file),
 *  `_oracolo` e `_turno` — cioè proprio nei campi che descrivono *cosa il file deve produrre*. Un
 *  rilevatore che sceglie i campi per prefisso decide anche quali difetti può vedere.
 *
 *  ## 🔴 Cosa NON vede, misurato e non dedotto
 *
 *  La firma è «un numero **sotto** un'attesa, a distanza multipla di 5 fino a 20». Quindi:
 *
 *  - **I valori intermedi**, che stanno *sopra* il totale finale. `Spec/Cover/TemporaryCoverExpires.json`
 *    aveva **quattro** numeri sbagliati (`99 · 88 · 77 · 56`) e questo controllo ne vedeva **uno**.
 *  - **Gli scarti che non sono multipli di 5**. `Visual/Combat/PushResistance.json` diceva `120-1=119`
 *    contro un'attesa di `120`: Δ1, invisibile. `Visual/Reaction/Deflection.json` diceva `88` contro `90`:
 *    Δ2, invisibile — ed era il caso peggiore, perché da `D-224` il colpo parato non fa **più niente** e
 *    lo scenario aveva smesso di mostrare ciò che gli dà il nome.
 *  - **I controfattuali** con delta arbitrario: *«48 sarebbe il valore se…»* → `53`, Δ13.
 *  - **I numeri sopra l'attesa**: *«se dice 79…»* → `84`.
 *
 *  ⚠️ E la finestra arriva a **20** e non a 5 per una ragione misurata: in
 *  `Spec/ActionEconomy/CooldownBlocksWithSlotFree.json` il bersaglio incassa **due** colpi, quindi il
 *  delta è 10 — e la passata del 2026-08-30, che cercava solo un delta di 5, quel file l'aveva mancato.
 *
 *  Allargare (`--wide`, ±30 senza il vincolo di multiplo) porta i candidati a **un centinaio**: il
 *  rilevatore non sa **delimitare** il problema, sa solo ordinarlo. Un verde qui non è una prova di
 *  assenza, ed è scritto perché nessuno lo prenda per tale.
 *
 *  ## ⚠️ Il rumore che si toglie, e quanto ne toglie DAVVERO
 *
 *  🔑 **Misurato togliendo un ramo alla volta e rieseguendo**: sul corpus di oggi solo `D-nnn` cambia il
 *  risultato (6 → 9 righe, cioè tutti e tre i falsi positivi accertati vengono da lì: `D-074`, `D-089`,
 *  `D-104`). Gli altri quattro **non sopprimono niente oggi** e restano perché la prosa cambia: un `#42`
 *  o un `CP 9.5` è a un commit di distanza. ⚠️ Sono dichiarati **non misurati**, che è il contrario di
 *  elencarli come falsi positivi veri — ma ciascuno ha il suo test, quindi non possono marcire in verde.
 *
 *  ⛔ **Un quinto ramo, `ADR-\d+`, è stato TOLTO perché non poteva firare**: `NUMERO` cerca 2-3 cifre e
 *  ogni ADR del corpus ne ha quattro (`ADR-0003`…`ADR-0008`). Un ramo che non può cambiare niente non è
 *  una difesa, è una riga che qualcuno manterrà per sempre credendola tale.
 */
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { join } from 'node:path';
import { pathToFileURL } from 'node:url';
import { filesUnder } from './files.ts';

export type Segnalazione = {
  file: string;
  campo: string;
  citato: number;
  atteso: number;
  delta: number;
  contesto: string;
};

/** Ciò che NON è un numero di gioco. Si cancella conservando la lunghezza: gli offset restano veri. */
const RUMORE =
  /\bD-\d+|#\d+|\bCP\s*\d+(?:\.\d+)*|\b\d{4}-\d{2}-\d{2}|[qrL]=-?\d+/g;
const NUMERO = /\b(\d{2,3})\b/g;

type Raccolta = { note: Array<[string, string]>; attese: number[] };

/**
 * Percorre lo scenario raccogliendo **ogni** campo di prosa (`_…`) e i valori di `UnitHpEquals`.
 *
 * 🔴 Il prefisso è `_` e non `_nota`: vedi il docstring: la scelta del prefisso decide quali difetti il
 * rilevatore può vedere, e `_errata_scudo`/`_oracolo`/`_turno` ne nascondevano 19.
 */
export function raccogli(nodo: unknown, out: Raccolta = { note: [], attese: [] }): Raccolta {
  if (Array.isArray(nodo)) {
    for (const v of nodo) raccogli(v, out);
    return out;
  }
  if (nodo === null || typeof nodo !== 'object') return out;
  for (const [k, v] of Object.entries(nodo as Record<string, unknown>)) {
    if (k.startsWith('_') && typeof v === 'string') {
      out.note.push([k, v]);
    } else if (k === 'expect' && Array.isArray(v)) {
      for (const e of v) {
        const voce = e as { type?: string; value?: unknown };
        if (voce?.type === 'UnitHpEquals' && typeof voce.value === 'number') {
          out.attese.push(voce.value);
        }
      }
    } else {
      raccogli(v, out);
    }
  }
  return out;
}

/** Taglia il contesto per PUNTI DI CODICE, non per unità UTF-16: la prosa porta emoji fuori dal BMP. */
function finestra(testo: string, da: number, a: number): string {
  return [...testo].slice(da, a).join('').replace(/\s+/g, ' ');
}

/** Le righe da LEGGERE per un singolo scenario. `wide` allarga la finestra e toglie il vincolo di 5. */
export function segnala(file: string, json: unknown, wide = false): Segnalazione[] {
  const { note, attese } = raccogli(json);
  const hp = [...new Set(attese)].sort((a, b) => a - b);
  if (hp.length === 0) return [];

  const fuori: Segnalazione[] = [];
  for (const [campo, testo] of note) {
    const pulito = testo.replace(RUMORE, (m) => ' '.repeat(m.length));
    for (const m of pulito.matchAll(NUMERO)) {
      const citato = Number(m[1]);
      if (hp.includes(citato)) continue; // coincide con un'attesa: niente da dire
      // `citato` non è in `hp`, quindi `d` non può essere 0: non serve escluderlo.
      const vicini = hp
        .map((h) => ({ h, d: h - citato }))
        .filter(({ d }) => (wide ? Math.abs(d) <= 30 : d > 0 && d <= 20 && d % 5 === 0));
      if (vicini.length === 0) continue;
      const scelto = vicini.reduce((a, b) => (Math.abs(b.d) < Math.abs(a.d) ? b : a));
      // Gli offset sono su `pulito`, che ha la STESSA lunghezza di `testo`: il rumore è stato sostituito
      // con spazi, non tolto. È l'unica ragione per cui si può tagliare `testo` con questi indici.
      const cp = [...pulito.slice(0, m.index ?? 0)].length;
      fuori.push({
        file,
        campo,
        citato,
        atteso: scelto.h,
        delta: scelto.d,
        contesto: finestra(testo, Math.max(0, cp - 55), cp + String(citato).length + 55),
      });
    }
  }
  return fuori;
}

/** La radice del repository, come fanno `doc-links`, `issue-refs` e `catalog-code`: non la cwd. */
const RADICE = fileURLToPath(new URL('../../', import.meta.url));

function main(): void {
  const check = process.argv.includes('--check');
  const wide = process.argv.includes('--wide');
  const files = filesUnder(join(RADICE, 'Scenarios'), '.json');

  const righe: Segnalazione[] = [];
  let letti = 0;
  let illeggibili = 0;
  for (const f of files) {
    const relativo = f.slice(RADICE.length).replace(/\\/g, '/');
    // `_redirects.json` non è uno scenario: non ha `expect`, e contarlo gonfierebbe la copertura.
    if (relativo.endsWith('/_redirects.json')) continue;
    let json: unknown;
    try {
      json = JSON.parse(readFileSync(f, 'utf8'));
    } catch (e) {
      console.error(`ILLEGGIBILE ${relativo}: ${(e as Error).message}`);
      illeggibili += 1;
      continue;
    }
    letti += 1;
    righe.push(...segnala(relativo, json, wide));
  }

  // 🔑 Si conta cio' che si e' LETTO, non cio' che si e' elencato: un file che non si apre non e'
  // copertura, e stampare il totale dei file direbbe di aver guardato dove non si e' guardato.
  console.error(
    `scenari letti: ${letti}${illeggibili > 0 ? ` (${illeggibili} ILLEGGIBILI)` : ''}` +
      `, finestra ${wide ? '±30 (--wide)' : '+5..20 multipla di 5'}`,
  );
  console.error(
    '⚠️ Questo controllo ORDINA, non decide: ogni riga va letta. E non vede gli intermedi, gli scarti' +
      ' non multipli di 5, i controfattuali né i numeri SOPRA l’attesa — sono nel docstring.',
  );
  if (illeggibili > 0 && check) process.exitCode = 1;

  if (righe.length === 0) {
    console.error('nessun numero citato diverge da un’attesa dello stesso file');
    return;
  }
  console.error(`\n${righe.length} numeri da leggere:`);
  for (const r of righe) {
    console.error(`  ${r.file} · ${r.campo}: ${r.citato} → ${r.atteso} (${r.delta > 0 ? '+' : ''}${r.delta})`);
    console.error(`      …${r.contesto}…`);
  }
  if (check) process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
