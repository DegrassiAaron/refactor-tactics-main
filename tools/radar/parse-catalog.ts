import { readFileSync } from 'node:fs';

export interface AbilityInput {
  id: string;
  /** Colonna `Tipo` del catalogo: `attacco base` · `linea` · `AoE` · `cella` · `dash` · `arco` ·
   *  `charge` · `reazione` · `predittiva` · `controllo`. Da qui si deriva la selettivita' (#603), e
   *  da `predittiva` la condizionalita' del payoff in `power.ts` (#1080). */
  kind: string;
  /** Danno dichiarato nella cella `Effetto`. `null` quando l'abilita' non fa danno. */
  damage: number | null;
  cooldown: number;
  /** Cella `Effetto` grezza: la classificazione degli effetti la fa la rubrica, non il parser. */
  effect: string;
  /** `Action.X` quando l'abilita' DELEGA il proprio dato a un'azione core (D-115), altrimenti `null`.
   *  Non confondere con le reazioni che RIUSANO una semantica core tenendo i propri numeri. */
  delegatesTo: string | null;
  /** Bonus di danno **condizionato da uno stato**, dichiarato nella cella `Effetto` come `+N su X`.
   *  `null` quando non c'e' — non `0`, che si confonderebbe con «bonus dichiarato e nullo».
   *  Il parser estrae il numero; che significhi «fuori da `power`, dentro `precision`» lo decide la
   *  rubrica (#557). */
  conditionalBonus: number | null;
  /** La riga della tabella reazioni, **grezza**, quando l'abilita' vi compare. La rubrica decide
   *  cosa significhi: qui non si classifica, si riporta. */
  reaction: ReactionInput | null;
}

export interface ReactionInput {
  /** L'azione core di cui riusa la semantica — NON una delega di dati (D-115). `null` se assente. */
  coreSemantics: string | null;
  /** `ready` = implementata in partita · `deferred` = rinviata a E14. Il catalogo lo marca con
   *  ✅/⏳, ed e' un dato competitivo: #557 ha DECISO che le `deferred` contano nei rating, quindi
   *  la rubrica deve poter esprimere anche la scelta opposta. */
  status: 'ready' | 'deferred';
  /** Cella `Stato` senza il marcatore, grezza. Per `InterceptShot` e' l'unica sede in cui il
   *  catalogo dichiara il trigger predittivo. */
  note: string;
}

export interface HeroInput {
  /**
   * Nome MOSTRATO, dall'intestazione del catalogo. Cambia: D-120 ha rinominato il roster
   * (`Flux` -> `Gadget`, `Riva` -> `Phase`, `Bastion` -> `Riktor`, `Vektor` -> `Wraith`).
   * Va nel titolo del radar, mai in un nome di file o in una URL.
   */
  name: string;

  /**
   * Chiave STABILE, in minuscolo: il prefisso degli `AbilityId` dell'eroe (`Flux.ArcPulse` -> `flux`).
   *
   * 🔴 **Esiste perche' il nome del file non puo' seguire il nome mostrato.** Fino al 2026-08-13 il
   * generatore scriveva `${hero.name.toLowerCase()}-profile.svg`: dopo la rinomina di D-120 cercava
   * `gadget-profile.svg`, sul disco c'era `flux-profile.svg`, e il gate era rosso — con in piu' otto file
   * che sarebbero rimasti orfani rigenerando.
   *
   * Rinominare i file avrebbe rotto le **URL pubblicate**: la Wiki incorpora i radar via
   * `raw.githubusercontent.com/.../docs/characters/radar/flux-profile.svg`, e `wiki-alt.ts` rifiuta un
   * riferimento a un file che non esiste. Un nome di file derivato da un'etichetta rinominabile e' una URL
   * che si rompe a ogni rinomina — ed e' esattamente cio' che D-120 evitava tenendo fermi gli Stable ID.
   *
   * Il prefisso delle abilita' e' l'unico identificatore stabile che la sezione di catalogo contiene.
   */
  key: string;
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
function parseConditionalBonus(effect: string): number | null {
  const m = effect.match(/\+(\d+)\s+su\b/);
  return m ? Number(m[1]) : null;
}

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
/** La tabella delle reazioni, che sta **prima** delle sezioni eroe e ha tre colonne invece di cinque.
 *  Dichiara le stesse abilita' da un altro lato: semantica core riusata e stato di implementazione. */
export function parseReactionTable(text: string): Map<string, ReactionInput> {
  const byId = new Map<string, ReactionInput>();

  for (const line of text.split('\n')) {
    const cells = line.split('|');
    // Tre colonne dati: `| Reazione | Semantica core | Stato |` -> 5 elementi con i bordi vuoti.
    if (cells.length !== 5) continue;

    const id = cells[1].trim().match(/^`([A-Za-z]+(?:\.[A-Za-z]+)+)`$/)?.[1];
    if (!id) continue;

    const raw = cells[3].trim();
    const status = raw.startsWith('✅') ? 'ready' : raw.startsWith('⏳') ? 'deferred' : null;
    if (status === null) continue; // non e' la tabella delle reazioni

    byId.set(id, {
      coreSemantics: cells[2].trim().match(/^`(Action\.[A-Za-z]+)`$/)?.[1] ?? null,
      status,
      note: raw.replace(/^[✅⏳]\s*/, '').trim(),
    });
  }

  return byId;
}

export function parseHeroCatalog(source: URL | string, actionSource: URL | string): HeroInput[] {
  const text = readFileSync(source, 'utf8');
  const actionDamage = parseActionCatalog(actionSource);
  // Le reazioni si UNISCONO alle abilita', non si aggiungono: `ReactiveCapacitor` e' dichiarato in
  // entrambe le tabelle e deve restare una voce sola.
  const reactions = parseReactionTable(text);
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
      const id = cells[1].trim().match(/^`([A-Za-z]+(?:\.[A-Za-z]+)+)`$/)?.[1];
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
        conditionalBonus: parseConditionalBonus(effect),
        reaction: reactions.get(id) ?? null,
      });
    }

    for (const [label, field] of Object.entries(STAT_FIELDS)) {
      if (stats[field] === undefined) {
        throw new Error(`${name}: statistica "${label}" assente dal catalogo`);
      }
    }

    // La chiave STABILE: il segmento dell'EROE dentro l'`AbilityId`. Si esige che sia UNO SOLO per eroe
    // invece di prendere il primo che capita — un kit con due chiavi significa che la sezione ne mescola
    // due, e sceglierne una in silenzio scriverebbe il radar di un eroe nel file di un altro.
    //
    // 🔴 **È il PENULTIMO segmento, non il primo, e il cambio è di `#755`.** Gli `AbilityId` sono passati
    // da `Flux.ArcPulse` a `Hero.Gadget.ArcPulse` (D-130, `#754`): col primo segmento la chiave sarebbe
    // diventata `hero` per **tutti e quattro** gli eroi, e i quattro radar si sarebbero scritti sullo
    // stesso file. ⚠️ Il guard qui sotto **non l'avrebbe fermato**: con `Hero.` il prefisso resta uno solo
    // per eroe, quindi `prefixes.length === 1` era soddisfatto. A fermare tutto è stato il pattern degli
    // id, che accettava due soli segmenti e non ne ha riconosciuto nessuno — un caso in cui il difetto è
    // stato preso dalla parte sbagliata del codice, e per fortuna.
    //
    // Il penultimo funziona per entrambe le forme: `Hero.Gadget.ArcPulse` → `gadget`, e `Flux.ArcPulse`
    // → `flux` se un catalogo restasse indietro.
    const heroKey = (id: string) => {
      const parts = id.split('.');
      return parts.length >= 2 ? parts[parts.length - 2] : parts[0];
    };
    const prefixes = [...new Set(abilities.map((a) => heroKey(a.id)))];
    if (prefixes.length !== 1) {
      throw new Error(
        `${name}: gli AbilityId devono condividere un solo segmento d'eroe (trovati: ${prefixes.join(', ') || 'nessuno'}). ` +
          `E' la chiave stabile con cui si nomina il file del radar.`,
      );
    }

    heroes.push({
      ...(stats as Omit<HeroInput, 'abilities' | 'key'>),
      key: prefixes[0].toLowerCase(),
      abilities,
    });
  }

  return heroes;
}
