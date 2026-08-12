import { readFileSync } from 'node:fs';

export interface AbilityInput {
  id: string;
  /** Colonna `Tipo` del catalogo: `attacco base` · `linea` · `AoE` · `cella` · `dash` · `arco` ·
   *  `charge` · `reazione` · `controllo`. Da qui si deriva la selettivita' (#603). */
  kind: string;
  /** Danno dichiarato nella cella `Effetto`. `null` quando l'abilita' non fa danno. */
  damage: number | null;
  cooldown: number;
  /** Cella `Effetto` grezza: la classificazione degli effetti la fa la rubrica, non il parser. */
  effect: string;
  /** `Action.X` quando l'abilita' DELEGA il proprio dato a un'azione core (D-115), altrimenti `null`.
   *  Non confondere con le reazioni che RIUSANO una semantica core tenendo i propri numeri. */
  delegatesTo: string | null;
}

export interface HeroInput {
  name: string;
  health: number;
  movePoints: number;
  visionRange: number;
  pushResistance: number;
  abilities: AbilityInput[];
}

/** Estrae il primo intero di una cella, ignorando unita' e note in coda.
 *  `5 MP` -> 5 · `7 — *era 6, alzata da...*` -> 7 */
function cellInteger(cell: string, field: string, hero: string): number {
  const m = cell.match(/-?\d+/);
  if (!m) {
    throw new Error(`${hero}: campo "${field}" non contiene un intero: "${cell.trim()}"`);
  }
  return Number(m[0]);
}

/** Danno dichiarato nella cella `Effetto`: `22 danni, range 4` -> 22.
 *  `null` quando l'abilita' non fa danno — che e' un fatto, non un errore. */
function parseDamage(effect: string): number | null {
  const m = effect.match(/(\d+)\s+danni/);
  return m ? Number(m[1]) : null;
}

const STAT_FIELDS: Record<string, keyof Omit<HeroInput, 'name' | 'abilities'>> = {
  Salute: 'health',
  Movimento: 'movePoints',
  'Range visivo': 'visionRange',
  'Resistenza Push': 'pushResistance',
};

/** Danno per `ActionId` dal catalogo azioni. La colonna `Effetto` porta il numero (D-115): i valori
 *  in prosa sotto la tabella non si leggono, perche' estrarli da una frase e' cio' che il parser
 *  deve rifiutare di fare. */
export function parseActionCatalog(source: URL | string): Map<string, number> {
  const damageById = new Map<string, number>();

  for (const line of readFileSync(source, 'utf8').split('\n')) {
    const cells = line.split('|');
    if (cells.length < 8) continue;
    const id = cells[1].trim().match(/^`(Action\.[A-Za-z]+)`$/)?.[1];
    if (!id) continue;
    const damage = parseDamage(cells[7]);
    if (damage !== null) damageById.set(id, damage);
  }

  return damageById;
}

export interface CatalogRead {
  heroes: HeroInput[];
  /** Quanto il parser ha letto, in chiaro. Si stampa anche quando tutto va bene: un parser che non
   *  dice quanto ha guardato non e' distinguibile da uno che non guarda. */
  coverage: string;
}

/** Punto d'ingresso: legge i due cataloghi e **dichiara la copertura**.
 *
 *  Il conteggio atteso e' il presidio contro il fallimento silenzioso: il filtro strutturale che
 *  distingue le sezioni eroe scarterebbe senza rumore una sezione riformattata, e il risultato
 *  sarebbe un roster piu' corto invece di un errore. */
export function readCatalogs(
  heroSource: URL | string,
  actionSource: URL | string,
  expect: { expectHeroes: number; expectAbilities: number },
): CatalogRead {
  const heroes = parseHeroCatalog(heroSource, actionSource);

  if (heroes.length !== expect.expectHeroes) {
    throw new Error(
      `copertura insufficiente: letti ${heroes.length} eroi, attesi ${expect.expectHeroes}. ` +
        `Una sezione eroe puo' aver perso la tabella "| Statistica | Valore |".`,
    );
  }

  const abilities = heroes.reduce((n, h) => n + h.abilities.length, 0);

  // Il conteggio degli eroi non copre questo caso: una riga-abilita' con una nota nella cella
  // dell'id sparisce, e il roster resta di quattro. Servono DUE numeri, non uno.
  if (abilities !== expect.expectAbilities) {
    throw new Error(
      `copertura insufficiente: letta ${abilities} abilita, attese ${expect.expectAbilities}. ` +
        `Una riga puo' avere una nota nella cella dell'AbilityId, che la rende invisibile.`,
    );
  }
  const delegated = heroes.reduce(
    (n, h) => n + h.abilities.filter((a) => a.delegatesTo !== null).length,
    0,
  );

  return {
    heroes,
    coverage: `${heroes.length}/${expect.expectHeroes} eroi · ${abilities} abilita · ${delegated} delega risolta`,
  };
}

/** Le due fonti sono ENTRAMBE obbligatorie (D-115): il catalogo eroi non e' autosufficiente, e un
 *  parametro opzionale reintroduce il percorso in cui una delega non risolta vale `null` in silenzio. */
export function parseHeroCatalog(source: URL | string, actionSource: URL | string): HeroInput[] {
  const text = readFileSync(source, 'utf8');
  const actionDamage = parseActionCatalog(actionSource);
  const heroes: HeroInput[] = [];

  // Una sezione eroe apre con `## <n>. <Nome> — <sottotitolo>`, ma lo stesso livello ospita anche
  // sezioni non-eroe (`## 5. Confronto rapido`). Il discriminante e' STRUTTURALE — la tabella delle
  // statistiche — non il nome: un titolo puo' cambiare, la tabella e' cio' che il parser consuma.
  // Una sezione eroe che perdesse quella tabella sparirebbe in silenzio: e' il conteggio finale a
  // renderlo visibile, non questo filtro.
  const sections = text
    .split(/^## \d+\. /m)
    .slice(1)
    .filter((s) => /\|\s*Statistica\s*\|\s*Valore\s*\|/.test(s));

  for (const section of sections) {
    const name = section.slice(0, section.indexOf('\n')).split('—')[0].trim();
    const stats: Partial<HeroInput> = { name };

    const abilities: AbilityInput[] = [];

    for (const line of section.split('\n')) {
      const cells = line.split('|');
      if (cells.length < 3) continue;

      const field = STAT_FIELDS[cells[1].trim()];
      if (field) {
        stats[field] = cellInteger(cells[2], cells[1].trim(), name);
        continue;
      }

      // Riga della tabella abilita': `| \`Hero.Ability\` | Nome | Tipo | Effetto | CD |`
      const id = cells[1].trim().match(/^`([A-Za-z]+\.[A-Za-z]+)`$/)?.[1];
      if (!id || cells.length < 6) continue;

      const effect = cells[4].trim();

      // D-115: SOLO la forma `è \`Action.X\`` delega il dato. La tabella delle reazioni cita anch'essa
      // azioni core (`Action.Counter`, `Action.Intercept`, `Action.Deflect`) ma quelle riusano la
      // SEMANTICA e tengono i propri numeri inline: risolverle darebbe il valore sbagliato.
      // Niente `\b` prima di `è`: in JS il confine di parola e' definito su [A-Za-z0-9_], e `è` non
      // ne fa parte — dopo un `*` (grassetto markdown) non c'e' confine e il match fallirebbe sempre.
      const delegatesTo = effect.match(/(?:^|[^\p{L}])è\s+\*{0,2}`(Action\.[A-Za-z]+)`/u)?.[1] ?? null;

      let damage = parseDamage(effect);
      if (delegatesTo !== null) {
        const delegated = actionDamage.get(delegatesTo);
        if (delegated === undefined) {
          throw new Error(
            `${id}: delega a "${delegatesTo}", che non ha un danno leggibile nel catalogo azioni. ` +
              `Passa il catalogo azioni al parser, o porta il numero in colonna (D-115).`,
          );
        }
        damage = delegated;
      }

      abilities.push({
        id,
        kind: cells[3].trim(),
        damage,
        cooldown: cellInteger(cells[5], `${id} CD`, name),
        effect,
        delegatesTo,
      });
    }

    for (const [label, field] of Object.entries(STAT_FIELDS)) {
      if (stats[field] === undefined) {
        throw new Error(`${name}: statistica "${label}" assente dal catalogo`);
      }
    }
    heroes.push({ ...(stats as Omit<HeroInput, 'abilities'>), abilities });
  }

  return heroes;
}
