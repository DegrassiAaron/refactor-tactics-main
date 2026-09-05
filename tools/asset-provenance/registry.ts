/** Il registro di provenienza degli asset: parsing della tabella e regole di copertura.
 *
 *  Le funzioni stanno qui, separate da `check.ts`, per la stessa ragione di `tools/asset-refs/refs.ts`:
 *  l'entry point tocca `git` e il filesystem, e non e' testabile; queste sono pure e lo sono.
 *
 *  🔴 **Una riga copre un PREFISSO, non un file.** E' la decisione che riconcilia le due meta' del
 *  Definition of Done di `#1767`, che senza questa lettura si contraddicono: *«ogni famiglia ha una
 *  riga»* parla di **28** pack, *«nessun asset resta privo di riga»* di **37.482** file. Un asset e'
 *  coperto quando un prefisso registrato lo contiene, quindi una riga sola risponde per una famiglia
 *  intera e la verifica resta per-asset.
 *
 *  Il prefisso vive in due spazi di path che il confronto tratta allo stesso modo:
 *   - **package path** — `/Game/FabAsset/Paragon/ParagonGadget/`, per cio' che sta fuori dal repository;
 *   - **path di repository** — `tools/icons-downloader/Paragon_Skill_Icons/`, per cio' che e' versionato.
 *
 *  ⚠️ **Cosa NON verifica**, dichiarato perche' non venga scoperto dopo:
 *   - **la licenza**. Verifica che una **riga esista**. Un verde qui significa «registrato», mai
 *     «consentito» — ed e' il guardrail che `#1767` scrive a lettere maiuscole. Nessun controllo
 *     automatico puo' leggere un EULA;
 *   - **la verita' di una riga**. `Licenza`, `Acquisito` e `Attribuzione` sono trascrizioni: se chi
 *     scrive sbaglia il titolo, la riga resta verde. Il valore `NON VERIFICATA` esiste proprio per
 *     non costringere a inventare;
 *   - **gli asset non versionati e non referenziati**. Un pack scaricato sul disco che nessun package
 *     versionato cita e' invisibile a `git` e all'estrattore: nessuno dei due lo vede. Le righe che
 *     lo descrivono si scrivono a mano, e `findDeadRows` non le dichiara morte solo perche' hanno
 *     `Consumer` vuoto — vedi il suo docstring;
 *   - **le ancore e la sintassi Markdown** del documento: quello e' `doc-tables.ts`. */

/** Le nove colonne che `FR-ASSET-LIC-01` prescrive, piu' `Copre`, che e' quella che il gate legge.
 *  L'ordine e' vincolante: `parseRegistry` confronta l'intestazione con questo array e rifiuta la
 *  tabella se non coincide. Rinominare o spostare una colonna senza toccare questa riga spegnerebbe
 *  il gate **in silenzio**, che e' il modo noto in cui un gate smette di misurare. */
export const COLONNE = [
  'Famiglia',
  'Copre',
  'Fonte',
  'Licenza',
  'Versione',
  'Acquisito',
  'UE',
  'Attribuzione',
  'Consumer',
] as const;

/** I marcatori che delimitano la tabella dentro il documento. Sono commenti HTML: invisibili in
 *  rendering, non ambigui per il parser. Senza, il gate dovrebbe indovinare **quale** tabella del
 *  documento e' il registro, e la prima tabella aggiunta per spiegare qualcosa lo farebbe sbagliare. */
export const MARCATORE_INIZIO = '<!-- registro:inizio -->';
export const MARCATORE_FINE = '<!-- registro:fine -->';

/** Il valore riservato per l'asset la cui licenza non e' accertata. E' un esito **registrato**, non
 *  un'omissione: `#1767` lo chiede esplicitamente. Il gate non lo tratta come errore. */
export const LICENZA_NON_VERIFICATA = 'NON VERIFICATA';

export interface RegistryRow {
  famiglia: string;
  /** Il prefisso di path che questa riga copre, gia' ripulito dai backtick. */
  copre: string;
  fonte: string;
  licenza: string;
  versione: string;
  acquisito: string;
  ue: string;
  attribuzione: string;
  consumer: string;
  /** Riga 1-based nel file, per poterla nominare in un messaggio d'errore. */
  riga: number;
}

/** Un difetto **del registro stesso**: una riga che il gate non riesce a leggere come riga. */
export interface ParseIssue {
  riga: number;
  testo: string;
  perche: string;
}

export interface ParseResult {
  rows: RegistryRow[];
  issues: ParseIssue[];
}

/** Spezza una riga di tabella Markdown nelle sue celle.
 *
 *  ⚠️ **Una pipe preceduta da backslash non e' un separatore.** E' lo stesso inganno che
 *  `doc-tables.ts` documenta: `Attack \| Ability` e' testo, non due celle. Un contatore ingenuo qui
 *  produce righe della larghezza sbagliata, e il falso positivo che ne segue costa credibilita' al
 *  gate — che e' il modo noto in cui un gate viene disattivato. */
export function splitRow(line: string): string[] {
  const cells: string[] = [];
  let current = '';
  for (let i = 0; i < line.length; i++) {
    const ch = line[i];
    if (ch === '\\' && line[i + 1] === '|') {
      current += '|';
      i++;
      continue;
    }
    if (ch === '|') {
      cells.push(current);
      current = '';
      continue;
    }
    current += ch;
  }
  cells.push(current);
  // Una riga di tabella apre e chiude con `|`: la prima e l'ultima cella sono vuote per costruzione.
  return cells.slice(1, -1).map((c) => c.trim());
}

/** Vero se `copre` e' una copertura scrivibile in colonna `Copre`.
 *
 *  Due forme, e nessuna terza:
 *   - **cartella** — finisce con `/`, e copre tutto cio' che le sta sotto;
 *   - **file singolo** — ha un'estensione, e copre solo se stesso.
 *
 *  🔴 **La barra finale non e' cosmetica.** Senza, `Content/Icon` coprirebbe anche `Content/Icons/`, e
 *  un prefisso scritto per una famiglia ne assolverebbe in silenzio un'altra che condivide l'inizio
 *  del nome. E' lo stesso motivo per cui `.gitignore` scrive `/Content/FabAsset/` con la barra. */
export function isCopertura(copre: string): boolean {
  if (copre.endsWith('/')) return true;
  const ultimo = copre.slice(copre.lastIndexOf('/') + 1);
  return /\.[A-Za-z0-9]+$/.test(ultimo);
}

/** Vero se `path` cade sotto `copre`, con la semantica dichiarata da `isCopertura`. */
export function copre(copertura: string, path: string): boolean {
  return copertura.endsWith('/') ? path.startsWith(copertura) : path === copertura;
}

/** Vero per la riga separatore di una tabella Markdown — `|---|---|` con o senza `:`. */
function isSeparator(cells: string[]): boolean {
  return cells.length > 0 && cells.every((c) => /^:?-{1,}:?$/.test(c));
}

/** Legge la tabella del registro dal documento.
 *
 *  Restituisce le righe **e** i difetti del registro stesso, separati: una riga con una colonna in
 *  meno non e' «un asset scoperto», e' un registro rotto, e i due errori vogliono messaggi diversi. */
export function parseRegistry(md: string): ParseResult {
  const lines = md.split(/\r?\n/);
  const start = lines.findIndex((l) => l.trim() === MARCATORE_INIZIO);
  const end = lines.findIndex((l) => l.trim() === MARCATORE_FINE);

  if (start < 0 || end < 0 || end < start) {
    return {
      rows: [],
      issues: [{
        riga: 0,
        testo: '',
        perche: `marcatori assenti o invertiti: servono ${MARCATORE_INIZIO} e ${MARCATORE_FINE}`,
      }],
    };
  }

  const rows: RegistryRow[] = [];
  const issues: ParseIssue[] = [];
  let headerSeen = false;

  for (let i = start + 1; i < end; i++) {
    const raw = lines[i];
    const trimmed = raw.trim();
    if (!trimmed.startsWith('|')) continue;

    const cells = splitRow(trimmed);
    if (isSeparator(cells)) continue;

    if (!headerSeen) {
      headerSeen = true;
      const atteso = COLONNE.join(' | ');
      const trovato = cells.join(' | ');
      if (trovato !== atteso) {
        issues.push({
          riga: i + 1,
          testo: trimmed,
          perche: `intestazione inattesa.\n  attesa:  ${atteso}\n  trovata: ${trovato}`,
        });
        // Senza intestazione valida le colonne non hanno significato: si smette di leggere.
        return { rows, issues };
      }
      continue;
    }

    if (cells.length !== COLONNE.length) {
      issues.push({
        riga: i + 1,
        testo: trimmed,
        perche: `${cells.length} celle invece di ${COLONNE.length}`,
      });
      continue;
    }

    const copre = cells[1];
    const m = /^`([^`]+)`$/.exec(copre);
    if (!m) {
      issues.push({
        riga: i + 1,
        testo: trimmed,
        perche: `la colonna Copre deve essere un prefisso fra backtick, trovato: ${copre || '(vuoto)'}`,
      });
      continue;
    }

    const prefisso = m[1].trim();
    if (!isCopertura(prefisso)) {
      issues.push({
        riga: i + 1,
        testo: trimmed,
        perche:
          `«${prefisso}» non e' una copertura valida: serve una cartella che finisce con '/' `
          + `oppure un file singolo con estensione`,
      });
      continue;
    }

    rows.push({
      famiglia: cells[0],
      copre: prefisso,
      fonte: cells[2],
      licenza: cells[3],
      versione: cells[4],
      acquisito: cells[5],
      ue: cells[6],
      attribuzione: cells[7],
      consumer: cells[8],
      riga: i + 1,
    });
  }

  if (!headerSeen) {
    issues.push({ riga: start + 1, testo: '', perche: 'nessuna tabella fra i marcatori' });
  }

  const visti = new Map<string, number>();
  for (const r of rows) {
    const gia = visti.get(r.copre);
    if (gia !== undefined) {
      issues.push({
        riga: r.riga,
        testo: r.copre,
        perche: `prefisso duplicato, gia' dichiarato a riga ${gia}`,
      });
    } else {
      visti.set(r.copre, r.riga);
    }
  }

  return { rows, issues };
}

/** La riga che copre `path`, o `null`.
 *
 *  🔴 **Vince il prefisso piu' lungo**, non il primo. Due righe possono sovrapporsi di proposito — una
 *  famiglia e una sua sotto-cartella con licenza diversa — e la risposta giusta e' la piu' specifica.
 *  Prendere la prima renderebbe la risposta dipendente dall'ordine delle righe nel documento, cioe'
 *  cambiabile da un riordino che nessuno legge come un cambio di semantica. */
export function coveringRow(path: string, rows: RegistryRow[]): RegistryRow | null {
  let best: RegistryRow | null = null;
  for (const r of rows) {
    if (!copre(r.copre, path)) continue;
    if (best === null || r.copre.length > best.copre.length) best = r;
  }
  return best;
}

export interface Uncovered {
  path: string;
  /** Da dove viene il path: un file versionato, o un riferimento dentro un package versionato. */
  origine: string;
}

/** I path che nessuna riga copre. */
export function findUncovered(
  paths: { path: string; origine: string }[],
  rows: RegistryRow[],
): Uncovered[] {
  return paths.filter((p) => coveringRow(p.path, rows) === null);
}

/** Le righe **morte**: un prefisso che non copre piu' niente.
 *
 *  E' il gemello di `findStaleExceptions` in `tools/asset-refs/refs.ts`, e la ragione e' la stessa: una
 *  riga scaduta e una viva sono indistinguibili a vista, quindi il registro si riempie di famiglie che
 *  il progetto non ha piu' e chi legge non sa quali. Un registro che cresce e non cala smette di
 *  descrivere cio' che c'e'.
 *
 *  ⚠️ **`paths` deve contenere tutte e tre le popolazioni** — versionata, referenziata **e** quella
 *  presente solo sul disco. Passarne due su tre dichiarerebbe morte le righe dei 24 pack che stanno
 *  sotto `Content/FabAsset/` senza che nessun package versionato li citi: sono presenti e non cotti,
 *  che e' esattamente lo stato che il registro deve saper scrivere. `check.ts` glielo passa. */
export function findDeadRows(rows: RegistryRow[], paths: string[]): RegistryRow[] {
  return rows.filter((r) => !paths.some((p) => copre(r.copre, p)));
}
