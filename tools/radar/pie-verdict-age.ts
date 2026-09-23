/** Segnala i verdetti PIE ✅ più VECCHI dell'ultima modifica di ciò che dichiarano di osservare.
 *
 *  Uso:  node tools/radar/pie-verdict-age.ts [--check] [--doc PERCORSO]
 *
 *  ## Il problema che chiude
 *
 *  Una cella del registro PIE descrive un criterio in prosa e porta un glifo di stato. Quando il codice
 *  sotto cambia, **nessuno rilegge la cella**: il ✅ resta, e chi lo cita crede di citare una misura
 *  corrente. Il registro stesso lo dichiara — *«un verde vacuo costa più di un'assenza, perché chi lo cita
 *  non va a rileggere la cella»* — ma non aveva modo di accorgersene.
 *
 *  🔑 **Misurato due volte il 2026-09-23, per vie indipendenti e a mano:**
 *
 *  - `PIE-GEO-GHOST`: verdetto del **2026-08-20**; il **2026-08-31** `ff37340a7` si intitola *«…e il ghost
 *    **smette di promettere muri che il rilascio non produce**»* — cioè la garanzia che quella cella
 *    giudica (*«sai dove finirà il muro senza doverlo rilasciare?»*);
 *  - `PIE-HEX-MODE-S`: verdetto delle **13:08:39**; alle **13:43:16** dello stesso giorno `490746018`
 *    riscrive 79 righe dichiarando che il ciclo di Ctrl+click *«ripeteva lo stesso nulla»*.
 *
 *  Entrambi trovati perché qualcuno stava già lavorando su quella issue. Il terzo uscirà quando qualcuno
 *  ci ricapiterà sopra per caso — che è il difetto, non l'aneddoto.
 *
 *  ## Perché il legame va DICHIARATO, misurato e non supposto
 *
 *  La via ovvia — confrontare la data del verdetto con la storia dei file che la cella **cita** — copre
 *  quasi niente, e due delle tre citazioni esistenti le ha scritte la passata che ha trovato il difetto:
 *
 *  ```bash
 *  awk -F'|' '/^\| \*\*PIE-/{if ($5 ~ /\.(cpp|h)`/) c++; t++} END {printf "%d su %d\n", c, t}' \
 *    docs/technical/test-manuali-pie.md
 *  #   3 su 250
 *  ```
 *
 *  Il secondo segnale plausibile — la issue citata dalla riga — copre metà dei verdetti e **manca uno dei
 *  due casi reali**: `PIE-GEO-GHOST` non cita nessuna issue nella propria riga.
 *
 *  ∴ il pezzo mancante non è il controllo, è il **legame**. Questo file lo legge; non lo inventa.
 *
 *  ## La convenzione: `osserva:` nella cella di STATO
 *
 *  ```text
 *  | **PIE-...** | ... | ... | ... | ✅ … osserva: `percorso` ~ `regex` @ AAAA-MM-GG |
 *  ```
 *
 *  - `` `percorso` `` — uno o più, separati da virgola: i file su cui il criterio si esercita;
 *  - `` ~ `regex` `` — **facoltativo**, e quasi sempre necessario: restringe a *quali righe* di quel file
 *    contano, e si applica con `git log -G`. Più regex separate da virgola valgono in **OR**;
 *  - `` @ AAAA-MM-GG `` — **obbligatorio**: la data del verdetto. Senza, la dichiarazione è incompleta e
 *    la riga non viene controllata.
 *
 *  🔴 **La data si DICHIARA, e non si prende da `git blame`: la falsificazione ha bocciato la prima
 *  stesura proprio qui.** Datare la riga col suo ultimo `blame` sembra elegante e **nasconde** la
 *  scadenza: qualunque ritocco alla cella — una precisazione, un declassamento, la correzione di un
 *  refuso — sposta la data in avanti e il confronto col codice torna a tacere. Misurato il 2026-09-23:
 *  rimettendo `PIE-GEO-GHOST` a ✅ per la prova, il gate **taceva**, perché la riga portava il `blame` del
 *  declassamento di quel giorno invece del verdetto del 2026-08-20.
 *
 *  ⚠️ **E nemmeno «la prima data che compare nella cella» funziona**: su una cella declassata la prima
 *  data è quella della narrazione del declassamento. Misurato sulla stessa riga.
 *
 *  🔴 **Senza la regex la granularità è il FILE, e il file è troppo grosso.** Misurato sul caso reale:
 *  i tre commit successivi al verdetto di `PIE-GEO-UNDO` hanno toccato `RTHexGeometryTool.cpp` — quindi
 *  un gate a granularità di file lo avrebbe segnalato — ma **zero** righe della transazione che quel
 *  criterio giudica. Con `` ~ `FScopedTransaction`, `Modify\(` `` il gate tace, correttamente; con
 *  `` ~ `bPreviewValid`, `FColor::Green` `` trova `960c21a02` e segnala il ghost. La stessa coppia di
 *  comandi, due risposte opposte: è la granularità a fare la differenza, non lo strumento.
 *
 *  🔴 **Nella cella di stato non entra MAI un `|`, nemmeno scritto `\|`** — ed è misurato, perché la
 *  convinzione diffusa è l'opposto. Un `\|` è un escape di *rendering*: qualunque parser che spezza sul
 *  `|` grezzo lo conta lo stesso, `awk -F'|'` compreso. Verificato il 2026-09-23: una riga contenente
 *  `` `Foo\|Bar` `` passa a **8** campi.
 *
 *  ⚠️ **E l'invariante del registro non è «sette campi», come si crede.** Il comando canonico legge
 *  `$(NF-1)`, cioè la penultima cella: un pipe in una cella *precedente* è innocuo, e infatti
 *  `PIE-HEX-MODE-O`, `-P`, `-Q` e `PIE-HEX-VIZ-VELO` stanno a 8 e 9 campi da sempre con
 *  `senza-marcatore=0`. Ciò che rompe è un pipe **nella cella di stato**, perché sposta quale campo è
 *  `$(NF-1)`. `osserva:` vive lì: per questo l'alternanza si scrive con la **virgola**, e il `|` lo
 *  costruisce questo file, dove non può fare danno.
 *
 *  ## Le tre scelte conservative, e perché
 *
 *  **Si guarda solo ✅.** Un 🟡 dichiara già da sé di essere in attesa di rimisura: segnalarlo farebbe
 *  uscire 1 su `main` finché qualcuno non riesegue una seduta, e un gate che non torna mai verde viene
 *  disattivato. È la stessa prudenza che `issue-refs.ts` dichiara nel proprio docstring.
 *
 *  **Un verdetto senza `osserva:` non è un errore.** La convenzione si riempie a poco a poco, partendo
 *  dalle famiglie che hanno prodotto le scadenze. Segnalare i non dichiarati farebbe uscire 1 su oltre
 *  cento righe il primo giorno — cioè rumore, cioè disattivazione.
 *
 *  **Si segnalano CANDIDATI DA RILEGGERE, mai «scaduto».** Un file cambia anche senza che il
 *  comportamento giudicato cambi. È la trappola in cui è caduta la passata che ha trovato il difetto: su
 *  `PIE-GEO-GHOST` il primo referto diceva *«il ghost è diverso»*, il vocabolario visivo della cella era
 *  **ancora esatto**, e solo il messaggio di commit per esteso ha deciso. Un gate che dichiarasse scaduto
 *  avrebbe avuto torto sul merito pur avendo ragione sul segnale.
 *
 *  ## Senza `git`, `NOT RUN`
 *
 *  Mai un verde ottenuto perché lo strumento non c'era: stessa scelta di `issue-refs.ts` e
 *  `anchor-state.ts`.
 */

import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

/** Un verdetto che dichiara ciò che osserva. */
export interface VerdettoDichiarato {
  /** Riga del documento, 1-based: è ciò che `git blame` vuole. */
  line: number;
  id: string;
  paths: string[];
  /** La regex per `git log -G`: le alternative della cella, unite con `|` qui. `null` se assente. */
  pattern: string | null;
  /** La data DICHIARATA del verdetto, in epoch. Vedi `dataDelVerdetto` per perche' non viene da `blame`. */
  quando: number;
}

/** Copertura della convenzione, per il messaggio di riepilogo. */
export interface Copertura {
  conVerdetto: number;
  dichiarati: number;
}

const RIGA = /^\|\s*\*\*(PIE-[A-Za-z0-9.\-]+)\*\*/;

/** La cella di stato è la PENULTIMA, come per il comando canonico del registro: `$(NF-1)`. */
function cellaDiStato(riga: string): string | null {
  const campi = riga.split('|');
  return campi.length >= 3 ? campi[campi.length - 2] : null;
}

/**
 * Il marcatore di stato della cella: il **primo** glifo che vi compare, non uno qualunque.
 *
 * 🔴 **Cercare il glifo «da qualche parte» sbaglia, ed è misurato.** Una cella declassata racconta il
 * proprio giro — *«RIVISTA da ✅ a 🟡»* — quindi contiene un ✅ in **prosa**. Una prima stesura di questo
 * file usava `includes('✅')` e contava **124** verdetti verdi dove il comando canonico ne conta **88**:
 * stava leggendo le narrazioni dei declassamenti come verdetti.
 *
 * ⚠️ La regola non è nuova e non si inventa qui: è quella che il registro dichiara in testa —
 * `match(s, /✅|🟡|❌|⏳/)` prende il primo. Questo file la ripete perché deve dare la **stessa** risposta,
 * non una propria.
 */
export function marcatore(cella: string): string | null {
  const m = /[✅\u{1F7E1}❌⏳]/u.exec(cella);
  return m ? m[0] : null;
}

/**
 * Estrae `osserva:` da una cella di stato.
 *
 * ⚠️ La `~ `regex`` si ferma alla fine della cella: una seconda clausola `osserva:` nella stessa cella
 * non è prevista dalla convenzione e non si cerca di indovinarla.
 */
export function leggiOsserva(
  cella: string,
): { paths: string[]; pattern: string | null; quando: number } | null {
  const m =
    /osserva:\s*((?:`[^`]+`\s*,\s*)*`[^`]+`)(?:\s*~\s*((?:`[^`]+`\s*,\s*)*`[^`]+`))?\s*@\s*(\d{4}-\d{2}-\d{2})/
      .exec(cella);
  if (!m) {
    return null;
  }
  const backtickati = (s: string) =>
    [...s.matchAll(/`([^`]+)`/g)].map((p) => p[1].trim()).filter((p) => p.length > 0);

  const paths = backtickati(m[1]);
  if (paths.length === 0) {
    return null;
  }

  // 🔑 L'alternanza si scrive con la VIRGOLA nel documento e diventa `|` QUI: un `|` nella cella di
  // stato sposterebbe quale campo e' `$(NF-1)`, cioe' romperebbe il comando canonico del registro.
  const alternative = m[2] ? backtickati(m[2]) : [];
  const pattern = alternative.length > 0 ? alternative.join('|') : null;

  // Mezzogiorno UTC: la data dichiarata è un giorno, non un istante, e confrontarla con un timestamp
  // di commit alla mezzanotte farebbe dipendere l'esito dal fuso di chi ha committato.
  const quando = Date.parse(`${m[3]}T12:00:00Z`) / 1000;
  return { paths, pattern, quando };
}

/** I verdetti ✅ che dichiarano ciò che osservano. Un 🟡 dichiara già da sé di essere in attesa. */
export function verdettiDichiarati(doc: string): VerdettoDichiarato[] {
  const fuori: VerdettoDichiarato[] = [];
  doc.split('\n').forEach((riga, i) => {
    const m = RIGA.exec(riga);
    if (!m) {
      return;
    }
    const stato = cellaDiStato(riga);
    if (stato === null || marcatore(stato) !== '✅') {
      return;
    }
    const osserva = leggiOsserva(stato);
    if (osserva === null) {
      return;
    }
    fuori.push({ line: i + 1, id: m[1], paths: osserva.paths, pattern: osserva.pattern, quando: osserva.quando });
  });
  return fuori;
}

/** Quante righe portano un verdetto ✅ e quante di quelle dichiarano `osserva:`. */
export function copertura(doc: string): Copertura {
  let conVerdetto = 0;
  let dichiarati = 0;
  for (const riga of doc.split('\n')) {
    if (!RIGA.test(riga)) {
      continue;
    }
    const stato = cellaDiStato(riga);
    if (stato === null || marcatore(stato) !== '✅') {
      continue;
    }
    conVerdetto += 1;
    if (leggiOsserva(stato) !== null) {
      dichiarati += 1;
    }
  }
  return { conVerdetto, dichiarati };
}

/**
 * Quando quella RIGA del documento è stata scritta l'ultima volta, in epoch.
 *
 * 🔴 **`git blame`, mai `git log -L`.** Misurato il 2026-09-23: su una riga di tabella di questo
 * documento `git log -L '/regex/',+1` riportava il commit che aveva creato la SEZIONE, saltando la
 * modifica successiva che `blame` vede. Un gate costruito su quello daterebbe i verdetti indietro di
 * settimane, cioè non segnalerebbe mai niente.
 */
export function dataDelVerdetto(docPath: string, line: number): number | null {
  try {
    const out = execFileSync('git', ['blame', '-L', `${line},${line}`, '--porcelain', '--', docPath], {
      encoding: 'utf8',
      stdio: ['ignore', 'pipe', 'ignore'],
    });
    const m = /^committer-time (\d+)$/m.exec(out);
    return m ? Number(m[1]) : null;
  } catch {
    return null;
  }
}

/**
 * Quando l'ultima modifica a `path` ha toccato righe che corrispondono a `pattern`, in epoch.
 *
 * `null` quando nessun commit corrisponde — che è il caso buono: significa che ciò che il verdetto
 * osserva non è cambiato.
 */
export function ultimaModifica(path: string, pattern: string | null): number | null {
  const args = ['log', '-1', '--format=%ct'];
  if (pattern !== null) {
    // `-G` cerca nel DIFF: commit che hanno aggiunto o tolto righe corrispondenti. È esattamente la
    // domanda «questo commit ha toccato ciò che il criterio guarda?», e non dipende dal tracciamento
    // delle righe come `-L`.
    args.push(`-G${pattern}`);
  }
  args.push('--', path);
  try {
    const out = execFileSync('git', args, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] }).trim();
    return out.length > 0 ? Number(out) : null;
  } catch {
    return null;
  }
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) {
  const { readFileSync } = await import('node:fs');
  const { fileURLToPath } = await import('node:url');

  const arg = (name: string, fallback: string) => {
    const i = process.argv.indexOf(name);
    return i !== -1 && process.argv[i + 1] ? process.argv[i + 1] : fallback;
  };
  const docPath = arg('--doc', fileURLToPath(new URL('../../docs/technical/test-manuali-pie.md', import.meta.url)));

  let doc: string;
  try {
    doc = readFileSync(docPath, 'utf8');
  } catch {
    console.error(`NOT RUN: documento non leggibile (${docPath}).`);
    process.exit(0);
  }

  // Senza `git` non si misura niente, e non si dichiara verde per averlo constatato.
  if (dataDelVerdetto(docPath, 1) === null) {
    console.error('NOT RUN: `git blame` non risponde (git assente, o percorso fuori da un repository).');
    process.exit(0);
  }

  const cop = copertura(doc);
  const dichiarati = verdettiDichiarati(doc);

  const sospetti: string[] = [];
  for (const v of dichiarati) {
    const quando = v.quando;
    for (const p of v.paths) {
      const tocco = ultimaModifica(p, v.pattern);
      if (tocco !== null && tocco > quando) {
        const g = (t: number) => new Date(t * 1000).toISOString().slice(0, 10);
        sospetti.push(
          `  L${v.line}  ${v.id}  verdetto ${g(quando)} < ultima modifica ${g(tocco)}  ${p}` +
            (v.pattern ? `  ~ ${v.pattern}` : ''),
        );
      }
    }
  }

  console.error(
    `copertura: ${cop.dichiarati} verdetti ✅ su ${cop.conVerdetto} dichiarano cosa osservano ` +
      `· ${sospetti.length} da rileggere`,
  );

  if (sospetti.length > 0) {
    console.error(
      `errore: ${sospetti.length} verdetti ✅ sono PIU' VECCHI dell'ultima modifica di cio' che osservano.\n` +
        `Non e' una dichiarazione di scadenza: e' un invito a rileggere la cella, perche' un file cambia\n` +
        `anche senza che il comportamento giudicato cambi. Chi rilegge decide, e lo scrive.\n` +
        sospetti.join('\n'),
    );
    process.exit(1);
  }
  console.error('nessun verdetto piu\' vecchio di cio\' che osserva');
}
