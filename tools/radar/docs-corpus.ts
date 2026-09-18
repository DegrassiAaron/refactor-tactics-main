/** Il corpus dei documenti che i gate della documentazione guardano, definito **una volta sola**.
 *
 *  Il problema che chiude (#1405, rilievi 11 e 12 della code review di #1404): `doc-links.ts` e
 *  `doc-tables.ts` portavano la stessa definizione, copiata — `markdownFiles`, `ROOT_DOCS` e il filtro
 *  sull'archivio. Aggiungere un quarto documento di governance alla radice, o un secondo prefisso
 *  escluso, aggiornava **un** gate e restringeva l'altro senza dirlo: i due continuavano a stampare una
 *  riga di copertura formulata allo stesso modo — `in 447 documenti` — mentre guardavano insiemi diversi.
 *
 *  ⚠️ **`wiki-alt.ts` non usa questo modulo, e non e' una dimenticanza.** Il suo `markdownFiles` opera sul
 *  clone della Wiki, non su `docs/`: salta `.git`, non filtra `archive` (la' non esiste) e ordina il
 *  risultato finale invece delle entry di ogni livello. Sono requisiti suoi, e parametrizzarli qui darebbe
 *  a questo modulo tre opzioni per un solo chiamante. La ragione e' scritta anche nel suo docstring.
 *
 *  ⛔ **Cosa questo modulo NON decide**: che cosa si controlla. Dice quali file entrano nel perimetro; ogni
 *  gate resta l'autorita' della propria proprieta' — un link che risolve, una tabella che non si rompe. */
import { existsSync, readdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { join, relative, sep } from 'node:path';

export const REPO_ROOT = fileURLToPath(new URL('../../', import.meta.url));
export const DOCS_DIR = fileURLToPath(new URL('../../docs/', import.meta.url));

/** I contratti operativi in `.claude/`: portano tabelle e link come i documenti di `docs/`, e #3166 ha
 *  misurato che **due** delle tre sintesi mai confrontate divergevano dal proprietario. Un cancello non
 *  vede una divergenza semantica — quella richiede il confronto — ma vede cio' per cui esiste: una
 *  tabella che si rompe e un percorso che non arriva.
 *
 *  ⚠️ **Entrati nel perimetro il 2026-09-18 a superficie verde**: `0` tabelle rotte, `0` link morti e `0`
 *  etichette stantie su nove documenti, misurati PRIMA di accendere il gate. Accendere un cancello quando
 *  e' gia' rosso e' il modo noto di farlo disattivare — e' la stessa ragione per cui `ROOT_DOCS` e' una
 *  lista e non un glob.
 *
 *  ⛔ Il perimetro era stato dichiarato assente **quattro volte** (#3159, #3165, #3166, #3169) sulla base
 *  di `DOCS_DIR` letto senza `main()`: `AGENTS.md` era gia' coperto da `ROOT_DOCS`, `.claude/` no. */
export const CLAUDE_DIR = fileURLToPath(new URL('../../.claude/', import.meta.url));

/** I tre documenti di governance della radice sono i piu' letti del repository: restarne fuori vorrebbe
 *  dire non vedere un percorso morto proprio dove costa di piu'.
 *
 *  ⚠️ Elencati, non presi con un glob su `*.md`, e la ragione e' cambiata il 2026-08-30.
 *
 *  Prima: la radice conteneva anche materiale **importato** — handoff datati e
 *  `RefactorTactics_Wiki_Lore.md`, che linkava `images/…` relativo al clone della Wiki e non a qui — e un
 *  glob li avrebbe segnalati come rotti: quattro falsi positivi al primo giro, che e' il modo noto di far
 *  disattivare un gate. Quei file sono stati consumati e la radice ora contiene esattamente i tre qui
 *  sotto, quindi oggi lista e glob darebbero lo stesso insieme.
 *
 *  La lista resta perche' la radice e' la casella di posta dell'autore: un kit ci viene lasciato,
 *  consumato e rimosso, e nell'intervallo linka percorsi di un altro repository. Un glob renderebbe il
 *  gate rosso proprio durante quel lavoro — cioe' nella condizione in cui serve di piu' che sia verde per
 *  il resto. 🔴 **Il punto cieco che questo si sceglie**: un Markdown nuovo lasciato in radice non lo
 *  controlla nessuno, e ci puo' restare settimane. E' successo: **sei** documenti, entrati fra il
 *  2026-08-08 e il 2026-08-28 e usciti tutti il 2026-08-30 — e i gate, che esistono dal 2026-08-25, non ne
 *  avrebbero visto nessuno. */
export const ROOT_DOCS = ['AGENTS.md', 'CLAUDE.md', 'README.md'];

/** Tutti i `.md` sotto una radice, in ordine stabile.
 *
 *  ⚠️ **Il tipo di ogni entry si legge dalla directory** (`withFileTypes`), non con una `statSync` per
 *  elemento: sotto `docs/` ci sono **935** entry in 77 directory, quindi la versione precedente spendeva
 *  935 syscall per gate e 1870 per la coppia di controlli, che gira sempre insieme.
 *
 *  🔑 **Una differenza c'e' ed e' dichiarata**: `Dirent.isDirectory()` non segue i symlink, `statSync` si'.
 *  Un symlink a directory sotto `docs/` verrebbe ora ignorato invece che attraversato. Misurato prima di
 *  cambiare — `git ls-files -s docs .claude | awk '$1=="120000"'` non restituisce niente: nessun symlink
 *  e' tracciato, quindi il corpus non cambia. Il giorno in cui ne entrasse uno, questa riga dice cosa
 *  aspettarsi.
 *
 *  L'ordine e' quello di `readdirSync(dir).sort()` di prima: confronto fra code unit, non `localeCompare`,
 *  che darebbe un ordine diverso su accenti e maiuscole. */
export function markdownFiles(root: string): string[] {
  const out: string[] = [];
  const walk = (dir: string) => {
    const entries = readdirSync(dir, { withFileTypes: true })
      .sort((a, b) => (a.name < b.name ? -1 : a.name > b.name ? 1 : 0));
    for (const entry of entries) {
      const full = join(dir, entry.name);
      if (entry.isDirectory()) walk(full);
      else if (entry.name.endsWith('.md')) out.push(full);
    }
  };
  walk(root);
  return out;
}

/** `true` se il file sta sotto `docs/archive/` — il criterio e' il **primo segmento** del percorso.
 *
 *  🔴 **Era `relative(DOCS_DIR, f).startsWith('archive')`**, cioe' un prefisso di stringa: vero anche per
 *  `archive-notes.md`, `archived/` e `archive-2026/`. Il giorno in cui nasce `docs/archived-decisions/`,
 *  quel sottoalbero esce dalla copertura di **entrambi** i gate — e la riga stampata continua a dire
 *  `(docs/archive/ escluso: passa --with-archive)`, nominando una directory che non e' quella saltata.
 *
 *  ✅ **Latente, e va detto**: `ls docs/ | grep -i '^archiv'` restituisce solo `archive/`, tanto alla
 *  misura del 2026-09-06 quanto a quella del 2026-09-18. Nessuna copertura e' persa in questo momento; e'
 *  un difetto che aspettava un nome di cartella, non uno che stesse gia' mordendo. */
export function isUnderArchive(file: string, docsDir: string = DOCS_DIR): boolean {
  return relative(docsDir, file).split(sep)[0] === 'archive';
}

export interface CorpusOptions {
  /** Include anche `docs/archive/`, che per difetto resta fuori: e' storico, e si ripara solo se
   *  qualcuno decide di farlo. */
  withArchive?: boolean;
}

/** I file che un gate della documentazione deve guardare: i tre documenti di governance della radice,
 *  `docs/` (meno l'archivio) e `.claude/`.
 *
 *  ⚠️ L'ordine e' significativo e viene ripetuto qui perche' i gate stampano i problemi nell'ordine in cui
 *  li incontrano: radice, `docs/`, `.claude/`. Cambiarlo cambierebbe l'ordine dei referti a parita' di
 *  difetti. */
export function docsCorpus({ withArchive = false }: CorpusOptions = {}): string[] {
  return [
    ...ROOT_DOCS.map((e) => join(REPO_ROOT, e)).filter((p) => existsSync(p)),
    ...markdownFiles(DOCS_DIR).filter((f) => withArchive || !isUnderArchive(f)),
    ...(existsSync(CLAUDE_DIR) ? markdownFiles(CLAUDE_DIR) : []),
  ];
}
