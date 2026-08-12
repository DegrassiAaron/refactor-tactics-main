/** Allinea (o verifica) il testo alternativo dei radar pubblicati sulla Wiki.
 *
 *  Uso:  node tools/radar/wiki-alt.ts --wiki-root <clone> [--check|--write]
 *
 *  Il problema che chiude: gli `alt` sono **prosa scritta a mano** nel clone della Wiki, mentre i
 *  radar si **rigenerano dai cataloghi** (D-106). Cambiare la salute di un eroe cambia il grafico e
 *  lascia l'`alt` indietro, in silenzio: chi vede il radar legge il numero nuovo, chi usa uno screen
 *  reader sente quello vecchio. `generate.ts --check` non se ne accorge, perche' guarda gli SVG.
 *
 *  La catena diventa: cataloghi -> SVG (gate di `generate.ts`) -> alt (gate di questo file). L'atteso
 *  si ricava **dall'SVG**, mai da una tabella parallela: una seconda copia dei valori sarebbe
 *  esattamente la cosa che stiamo togliendo. */
import { readFileSync, existsSync, writeFileSync, readdirSync, statSync } from 'node:fs';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { join } from 'node:path';

const RADAR_DIR = fileURLToPath(new URL('../../docs/characters/radar/', import.meta.url));

/** Un riferimento a un radar dentro una pagina Markdown. */
export interface AltRef {
  /** Testo alternativo cosi' com'e' scritto oggi. */
  alt: string;
  /** `flux`, `riva`, … */
  hero: string;
  /** `profile` | `balance` */
  view: string;
  /** L'intera immagine Markdown, per poterla sostituire senza ambiguita'. */
  match: string;
}

/** Immagine Markdown che punta a un SVG dei radar, per URL raw o per percorso relativo.
 *
 *  L'`alt` non puo' contenere `]`, e non serve che possa: e' una frase. */
const IMAGE = /!\[([^\]]*)\]\(([^)]*radar\/([a-z]+)-(profile|balance)\.svg)\)/g;

export function scanMarkdown(text: string): AltRef[] {
  const out: AltRef[] = [];
  for (const m of text.matchAll(IMAGE)) {
    out.push({ alt: m[1]!, hero: m[3]!, view: m[4]!, match: m[0]! });
  }
  return out;
}

/** Il testo alternativo che l'SVG **si merita**, ricavato dall'SVG stesso.
 *
 *  Il `<title>` e' gia' il nome accessibile dell'immagine (`aria-labelledby`), quindi l'`alt` lo
 *  ripete e vi aggiunge i valori: chi non vede la forma deve ricevere gli stessi numeri che sono
 *  disegnati sui raggi, non un'etichetta generica come «Profile Radar di Flux». */
export function expectedAlt(svg: string): string {
  const title = svg.match(/<title[^>]*>([^<]*)<\/title>/)?.[1];
  if (!title) throw new Error('SVG senza <title>: non ha un nome accessibile da cui partire');

  const pairs = [...svg.matchAll(/class="axis"[^>]*>([^<]*)<[\s\S]*?class="value"[^>]*>([^<]*)</g)].map(
    (m) => `${m[1]} ${m[2]}`,
  );
  if (pairs.length === 0) throw new Error(`${title}: nessuna coppia asse/valore leggibile dall'SVG`);

  return `${title}: ${pairs.join(', ')}`;
}

/** Sostituisce gli `alt` divergenti, lasciando intatto tutto il resto della riga. */
export function rewrite(text: string, fixes: Map<string, string>): string {
  let out = text;
  for (const [oldImage, newImage] of fixes) out = out.split(oldImage).join(newImage);
  return out;
}

function markdownFiles(root: string): string[] {
  const out: string[] = [];
  const walk = (dir: string) => {
    for (const name of readdirSync(dir)) {
      if (name === '.git') continue;
      const p = join(dir, name);
      if (statSync(p).isDirectory()) walk(p);
      else if (name.endsWith('.md')) out.push(p);
    }
  };
  walk(root);
  return out.sort();
}

function main(): void {
  const argv = process.argv;
  const rootIdx = argv.indexOf('--wiki-root');
  if (rootIdx === -1 || !argv[rootIdx + 1]) {
    console.error('uso: node tools/radar/wiki-alt.ts --wiki-root <clone> [--check|--write]');
    process.exit(2);
  }
  const root = argv[rootIdx + 1]!;
  const check = argv.includes('--check');
  const write = argv.includes('--write');

  const diverging: string[] = [];
  let seen = 0;

  for (const file of markdownFiles(root)) {
    const text = readFileSync(file, 'utf8');
    const refs = scanMarkdown(text);
    if (refs.length === 0) continue;

    const fixes = new Map<string, string>();
    for (const ref of refs) {
      seen++;
      const svgPath = `${RADAR_DIR}${ref.hero}-${ref.view}.svg`;
      if (!existsSync(svgPath)) {
        console.error(`errore: ${file} cita ${ref.hero}-${ref.view}.svg, che non esiste`);
        process.exit(1);
      }
      const want = expectedAlt(readFileSync(svgPath, 'utf8'));
      if (ref.alt === want) continue;

      diverging.push(`${file.slice(root.length + 1)} [${ref.hero}-${ref.view}]`);
      if (!check) fixes.set(ref.match, ref.match.replace(`![${ref.alt}]`, `![${want}]`));
    }

    if (write && fixes.size > 0) {
      writeFileSync(file, rewrite(text, fixes), { encoding: 'utf8' });
    }
  }

  console.error(`riferimenti ai radar esaminati: ${seen}`);

  if (diverging.length === 0) {
    console.error('alt allineati agli SVG');
    return;
  }
  if (check) {
    console.error(
      `errore: ${diverging.length} alt non allineati agli SVG:\n  ${diverging.join('\n  ')}\n` +
        `Correggili: node tools/radar/wiki-alt.ts --wiki-root <clone> --write`,
    );
    process.exit(1);
  }
  console.error(
    write
      ? `riscritti ${diverging.length} alt`
      : `${diverging.length} alt da riscrivere (nessuna modifica: passa --write)`,
  );
}

// Eseguito come script, non quando i test importano le funzioni pure. `pathToFileURL` e non una
// stringa `file://` costruita a mano: su Windows il percorso ha la lettera di unita' e le backslash,
// e il confronto fallirebbe in silenzio lasciando la CLI muta.
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
