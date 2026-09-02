/** Confronta i numeri citati nei campi `_nota*` di uno scenario con ciò che il file stesso ASSERISCE.
 *
 *  Uso:  node tools/radar/scenario-notes.ts [--check] [--wide]
 *
 *  Il problema che chiude, e la ragione per cui è **prosa contro misura nello stesso file**: un `_nota`
 *  dice *«120 - 22 = 98»* e dieci righe più sotto lo stesso file asserisce **103**. Il corpus è verde —
 *  nessun `expect` è sbagliato — ma chi apre il file per capire cosa dovrebbe succedere legge il numero
 *  sbagliato, e lo copia. È il vettore misurato da `#1904`: il `_nota` di `Combat.BasicAttack` ha
 *  propagato un totale pre-`D-224` in tre documenti a valle.
 *
 *  La causa di ogni scarto trovato finora è `D-224`: lo scudo base assorbe **5 punti per turno** sul solo
 *  danno `Direct`, e non è mai entrato nelle note.
 *
 *  ⚠️ **Ordina, non decide.** Ogni riga va letta. Un numero intermedio legittimo può capitare a distanza
 *  +5 da un'attesa per coincidenza, e succede: su questo corpus **sei** delle righe che restano dopo la
 *  correzione di `#2049` sono legittime — due riferimenti incrociati a un altro scenario, una citazione
 *  storica di ciò che una versione precedente asseriva, un HP di partenza, un valore di danno e un
 *  controfattuale derivato.
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
 *  Allargare (`--wide`, ±30 senza il vincolo di multiplo) porta i candidati da 34 a **100**: il
 *  rilevatore non sa **delimitare** il problema, sa solo ordinarlo. Un verde qui non è una prova di
 *  assenza, ed è scritto perché nessuno lo prenda per tale.
 *
 *  ## ⚠️ Il rumore che si toglie, e ognuno è stato un falso positivo VERO
 *
 *  `D-074` `D-089` `D-104` (decisioni), `ADR-0007`, `#1054` (issue), `CP 16.2` (checkpoint),
 *  `2026-08-16` (data), `q=-2` / `r=1` (celle). Senza il filtro erano **3** righe su 37, e un gate che
 *  segnala una decisione come se fosse un HP viene disattivato al primo falso rosso.
 */
import { readdirSync, readFileSync, statSync } from 'node:fs';
import { join } from 'node:path';
import { pathToFileURL } from 'node:url';

export type Segnalazione = {
  file: string;
  campo: string;
  citato: number;
  atteso: number;
  delta: number;
  contesto: string;
};

/** Ciò che NON è un numero di gioco. Si cancella conservando la lunghezza, così gli offset restano veri. */
const RUMORE =
  /\bD-\d+|\bADR-\d+|#\d+|\bCP\s*\d+(?:\.\d+)*|\b\d{4}-\d{2}-\d{2}|[qrL]=-?\d+/g;
const NUMERO = /\b(\d{2,3})\b/g;

type Raccolta = { note: Array<[string, string]>; attese: number[] };

/** Percorre lo scenario raccogliendo i `_nota*` e i valori di `UnitHpEquals`. */
export function raccogli(nodo: unknown, out: Raccolta = { note: [], attese: [] }): Raccolta {
  if (Array.isArray(nodo)) {
    for (const v of nodo) raccogli(v, out);
    return out;
  }
  if (nodo === null || typeof nodo !== 'object') return out;
  for (const [k, v] of Object.entries(nodo as Record<string, unknown>)) {
    if (k.startsWith('_nota') && typeof v === 'string') {
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
      const vicini = hp
        .map((h) => ({ h, d: h - citato }))
        .filter(({ d }) => (wide ? d !== 0 && Math.abs(d) <= 30 : d > 0 && d <= 20 && d % 5 === 0));
      if (vicini.length === 0) continue;
      const scelto = vicini.reduce((a, b) => (Math.abs(b.d) < Math.abs(a.d) ? b : a));
      const i = m.index ?? 0;
      fuori.push({
        file,
        campo,
        citato,
        atteso: scelto.h,
        delta: scelto.d,
        contesto: testo.slice(Math.max(0, i - 55), i + m[1]!.length + 55).replace(/\s+/g, ' '),
      });
    }
  }
  return fuori;
}

function scenari(radice: string): string[] {
  const out: string[] = [];
  for (const nome of readdirSync(radice).sort()) {
    const p = join(radice, nome);
    if (statSync(p).isDirectory()) out.push(...scenari(p));
    else if (nome.endsWith('.json')) out.push(p);
  }
  return out;
}

function main(): void {
  const check = process.argv.includes('--check');
  const wide = process.argv.includes('--wide');
  const files = scenari('Scenarios');

  const righe: Segnalazione[] = [];
  for (const f of files) {
    let json: unknown;
    try {
      json = JSON.parse(readFileSync(f, 'utf8'));
    } catch (e) {
      console.error(`ILLEGGIBILE ${f}: ${(e as Error).message}`);
      if (check) process.exitCode = 1;
      continue;
    }
    righe.push(...segnala(f.replace(/\\/g, '/'), json, wide));
  }

  console.error(
    `note confrontate: ${files.length} scenari, finestra ${wide ? '±30 (--wide)' : '+5..20 multipla di 5'}`,
  );
  console.error(
    '⚠️ Questo controllo ORDINA, non decide: ogni riga va letta. E non vede gli intermedi, gli scarti' +
      ' non multipli di 5, i controfattuali né i numeri SOPRA l’attesa — sono nel docstring.',
  );

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
