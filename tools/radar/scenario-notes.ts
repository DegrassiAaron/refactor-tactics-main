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
 *  ## 🔴 Le sei righe legittime si ASSOLVONO, e prima il gate era rosso su un corpus pulito
 *
 *  Fino al 2026-09-03 `--check` usciva **1** con quelle sei righe, cioè su un corpus che questo stesso
 *  docstring dichiarava pulito. **Un gate rosso quando tutto va bene non è un gate: è rumore che si
 *  impara a scavalcare** — e il difetto non è teorico. Quel giorno, controllando quali gate girassero su
 *  `main`, il rosso ha prodotto la conclusione *«nessuno ha letto queste righe»* in chi stava proprio
 *  verificando lo stato del repository. Erano documentate come lette da giorni, qui sopra.
 *
 *  ∴ `_numeri_letti` (`CAMPO_LETTI`): mappa **campo → motivo**, con il motivo **obbligatorio**. Il modello
 *  è quello delle esenzioni di `issue-refs.ts`, e ne eredita le tre difese:
 *
 *  1. **il motivo è obbligatorio** — un'assoluzione muta non assolve e si dichiara a parte;
 *  2. **l'assoluzione è per CAMPO, non per file** — un numero nuovo in `_nota_coppia` non è coperto da
 *     un'assoluzione scritta per `_nota`, cioè il gate non smette di guardare dove ha già trovato qualcosa;
 *  3. **le assoluzioni si stampano a ogni esecuzione** — un elenco che cresce in silenzio è il modo in cui
 *     un gate si svuota senza che nessuno abbia deciso di svuotarlo.
 *
 *  ⛔ **E il campo esce dalla scansione**, che è la trappola del meccanismo: un motivo spiega un numero,
 *  quindi lo **contiene**. Senza l'esclusione ogni assoluzione genererebbe il rilievo che assolve, e il
 *  gate resterebbe rosso dichiarando di essersi assolto — peggio di prima, perché sembrerebbe un difetto
 *  dei dati invece che dello strumento. Un test lo presidia (`il motivo di un'assoluzione non genera
 *  segnalazioni proprie`), e la mutazione è stata misurata: aggiunto un numero divergente in un campo non
 *  assolto, `--check` torna **1** e lo elenca fra i «da leggere», con le sei assoluzioni ancora stampate.
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
  /** Il motivo con cui lo scenario dichiara il numero gia' letto. Assente = da leggere. */
  assolto?: string;
  /** Un'assoluzione senza motivo: NON assolve, e vale come difetto a se'. */
  mutaSenzaMotivo?: boolean;
};

/** Ciò che NON è un numero di gioco. Si cancella conservando la lunghezza: gli offset restano veri. */
const RUMORE =
  /\bD-\d+|#\d+|\bCP\s*\d+(?:\.\d+)*|\b\d{4}-\d{2}-\d{2}|[qrL]=-?\d+/g;
const NUMERO = /\b(\d{2,3})\b/g;

type Raccolta = { note: Array<[string, string]>; attese: number[]; letti: Map<string, string> };

/**
 * Il campo con cui uno scenario dichiara che un suo numero **e' stato letto e giudicato legittimo**.
 *
 * Forma: `"_numeri_letti": { "_nota": "perche' quel numero non e' un difetto" }` — chiave il **campo**,
 * valore il **motivo**, obbligatorio.
 *
 * 🔴 **Deve essere escluso dalla scansione, ed e' la trappola principale del meccanismo**: `raccogli`
 * percorre ogni campo `_*` e scende negli oggetti. Un motivo spiega un numero, quindi **contiene** quel
 * numero: senza questa esclusione ogni assoluzione genererebbe il rilievo che assolve, e il gate resterebbe
 * rosso dicendo di essersi assolto.
 */
export const CAMPO_LETTI = '_numeri_letti';

/**
 * Percorre lo scenario raccogliendo **ogni** campo di prosa (`_…`) e i valori di `UnitHpEquals`.
 *
 * 🔴 Il prefisso è `_` e non `_nota`: vedi il docstring: la scelta del prefisso decide quali difetti il
 * rilevatore può vedere, e `_errata_scudo`/`_oracolo`/`_turno` ne nascondevano 19.
 */
export function raccogli(
  nodo: unknown,
  out: Raccolta = { note: [], attese: [], letti: new Map() },
): Raccolta {
  if (Array.isArray(nodo)) {
    for (const v of nodo) raccogli(v, out);
    return out;
  }
  if (nodo === null || typeof nodo !== 'object') return out;
  for (const [k, v] of Object.entries(nodo as Record<string, unknown>)) {
    if (k === CAMPO_LETTI) {
      // ⛔ Si raccoglie e NON si ricorre: vedi `CAMPO_LETTI`. Un motivo che spiega un numero lo contiene,
      // e scenderci dentro farebbe segnalare l'assoluzione stessa.
      if (v !== null && typeof v === 'object' && !Array.isArray(v)) {
        for (const [campo, motivo] of Object.entries(v as Record<string, unknown>)) {
          // ⚠️ Un'assoluzione MUTA si registra come stringa vuota, non si scarta: `segnala` la fa fallire.
          // Scartarla qui la renderebbe indistinguibile da un'assoluzione assente, cioe' silenziosa.
          out.letti.set(campo, typeof motivo === 'string' ? motivo.trim() : '');
        }
      }
    } else if (k.startsWith('_') && typeof v === 'string') {
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
  const { note, attese, letti } = raccogli(json);
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
      // 🔑 L'assoluzione e' per CAMPO, non per file: un numero nuovo in `_nota_coppia` non e' coperto da
      // un'assoluzione scritta per `_nota`. Un'esenzione che valesse per l'intero scenario sarebbe il modo
      // in cui il gate smette di guardare proprio dove ha gia' trovato qualcosa una volta.
      const motivo = letti.get(campo);
      fuori.push({
        file,
        campo,
        citato,
        atteso: scelto.h,
        delta: scelto.d,
        contesto: finestra(testo, Math.max(0, cp - 55), cp + String(citato).length + 55),
        ...(motivo ? { assolto: motivo } : {}),
        ...(motivo === '' ? { mutaSenzaMotivo: true } : {}),
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

  const mute = righe.filter((r) => r.mutaSenzaMotivo);
  const assolte = righe.filter((r) => r.assolto);
  const daLeggere = righe.filter((r) => !r.assolto && !r.mutaSenzaMotivo);

  // 🔑 **Le assoluzioni si stampano SEMPRE**, come `issue-refs` fa con le proprie esenzioni: un elenco che
  // cresce in silenzio è il modo in cui un gate si svuota senza che nessuno abbia deciso di svuotarlo.
  if (assolte.length > 0) {
    console.error(`\n${assolte.length} già letti e dichiarati legittimi (${CAMPO_LETTI}):`);
    for (const r of assolte) {
      console.error(`  ${r.file} · ${r.campo}: ${r.citato} → ${r.atteso} — ${r.assolto}`);
    }
  }

  // ⛔ Un'assoluzione SENZA MOTIVO non assolve e non tace: è l'unica difesa contro un meccanismo che
  // diventi un interruttore. `issue-refs` pretende il motivo per la stessa ragione.
  if (mute.length > 0) {
    console.error(
      `\n${mute.length} assoluzioni SENZA MOTIVO: ${CAMPO_LETTI} lo pretende, e non è una formalità.`,
    );
    for (const r of mute) {
      console.error(`  ${r.file} · ${r.campo}: ${r.citato} → ${r.atteso} — motivo mancante`);
    }
  }

  if (daLeggere.length === 0 && mute.length === 0) {
    console.error(
      assolte.length > 0
        ? `nessun numero NUOVO diverge da un'attesa dello stesso file (${assolte.length} già letti)`
        : 'nessun numero citato diverge da un’attesa dello stesso file',
    );
    return;
  }

  if (daLeggere.length > 0) {
    console.error(`\n${daLeggere.length} numeri da leggere:`);
    for (const r of daLeggere) {
      console.error(
        `  ${r.file} · ${r.campo}: ${r.citato} → ${r.atteso} (${r.delta > 0 ? '+' : ''}${r.delta})`,
      );
      console.error(`      …${r.contesto}…`);
    }
  }
  if (check) process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
