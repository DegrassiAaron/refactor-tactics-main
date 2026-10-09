/** L'asse `power` del Balance Radar (#603): quanto danno l'eroe produce per turno, tenendo conto di
 *  quanto spesso puo' farlo. Non burst — un colpo enorme ogni tre turni non vale quanto uno ripetibile. */
import type { AbilityInput, HeroInput } from './parse-catalog.ts';
import { availabilityWeight, rate } from './rubric.ts';

/** Ancora assoluta: `raw 100` = «uccide un eroe medio in un turno». Non e' scelta a mano — la salute
 *  del roster e' 90/95/120/90, media 98.75, e `power_raw` e' gia' danno per turno. */
export const POWER_ANCHOR = 100;

/** Denominatore dei pesi interi di `availabilityWeight`, piu' il fattore 10 con cui `power_raw` e'
 *  espresso in "danno per turno". Vedi `rubric.ts`.
 *
 *  ⚠️ **Esportato da #2579** perche' chi legge un contributo per-abilita' deve poterlo riportare sulla
 *  scala di `power_raw` senza reinventare il denominatore: due copie di `600` in due moduli sono due
 *  numeri che possono divergere. */
export const POWER_WEIGHT_SCALE = 60 * 10;

/** Un payoff e' **condizionale** quando non arriva per il solo fatto di usare l'azione. Il catalogo
 *  lo dichiara in due sedi diverse, e la regola e' **una sola** (#557):
 *
 *  - nella cella `Effetto`, come bonus su uno stato — `+8 su bersaglio Wet`;
 *  - nella colonna `Tipo`, come azione **predittiva** — l'intero payoff di `InterceptShot` dipende
 *    dall'aver indovinato dove andra' l'avversario, quindi zero e' il danno che ci si puo' aspettare
 *    senza chiedere niente.
 *
 *  Trattarle come due meccanismi separati produrrebbe due strade che possono divergere.
 *
 *  ⚠️ **Fino al 2026-08-17 questa funzione faceva match su una FRASE IN PROSA** della tabella delle
 *  reazioni — `/trigger d'ingresso su movimento/` — e il difetto era che nessuno poteva saperlo: chi
 *  avesse riscritto quella cella per correggerne il contenuto (e c'era da correggerlo: dichiarava
 *  `InterceptShot` una reazione rinviata a E14, falso dal 2026-08-10) avrebbe **mosso un rating** senza
 *  toccare un numero. Ora legge la colonna `Tipo`, che e' un campo: correggere la prosa attorno non
 *  cambia piu' i radar. Vedi `#1080`.
 *
 *  ⚠️ **`kind` NON ha un gate come `KNOWN_EFFECTS`**: il suo vocabolario e' documentato in
 *  `parse-catalog.ts`, non validato. Un `predittivo` al posto di `predittiva` scriverebbe un tipo che
 *  nessuno rifiuta, e il payoff tornerebbe a contare per intero — lo stesso difetto di prima, spostato
 *  di una casella. Cio' che lo intercetta e' un test che **pinna il valore**, non un controllo di
 *  dominio: `parse-catalog.test.ts`, «una azione predittiva NON e una reazione». */
function isPredictive(ability: AbilityInput): boolean {
  return ability.kind === 'predittiva';
}

/** Il danno che l'azione produce **senza chiedere niente**. Zero quando l'intero payoff e' condizionale.
 *
 *  Il `damage` che arriva dal parser e' gia' il primo numero della cella, quindi il `+8` condizionale
 *  non vi rientra: quel bonus vale come **condizionalita'** in `precision`, mai come danno. */
export function guaranteedDamage(ability: AbilityInput): number {
  if (ability.damage === null) return 0;
  return isPredictive(ability) ? 0 : ability.damage;
}

/** Il contributo di UNA abilita' all'asse `power` del proprio eroe (#2579).
 *
 *  ⚠️ **`weighted` e' INTERO, e non e' un dettaglio di comodita'.** E' il termine della somma **prima**
 *  dell'unica divisione, cioe' esattamente cio' che `powerRaw` accumulava e non faceva uscire. Esporre
 *  invece il contributo gia' diviso — `weighted / POWER_WEIGHT_SCALE` — darebbe una sequenza di float la
 *  cui somma **non e' garantita** coincidere con `powerRaw`: `D-108` confronta artefatti byte a byte, e
 *  un residuo di virgola mobile lo renderebbe rosso su una macchina e verde su un'altra. Chi vuole il
 *  valore sulla scala di `power_raw` divide **una volta**, alla fine, come fa l'eroe. */
export interface AbilityContribution {
  /** `Hero.<Eroe>.<Abilita>`, la chiave stabile del catalogo. */
  id: string;
  /** La colonna `Tipo`: e' la categoria su cui si costruiscono le bande. */
  kind: string;
  /** Il danno che l'azione produce senza chiedere niente — zero se il payoff e' condizionale. */
  damage: number;
  cooldown: number;
  /** `availabilityWeight(cooldown)`, intero per costruzione. */
  availability: number;
  /** `damage * 20 * availability`. Intero: e' il termine della somma di `powerRaw`. */
  weighted: number;
}

/** I contributi delle abilita' di un eroe, nell'ordine del catalogo.
 *
 *  ⛔ **Non e' una seconda derivazione**: `powerRaw` somma questi stessi termini invece di ricalcolarli.
 *  E' la disciplina che `balance.ts` applica gia' a `control`/`support`/`durability` — *«si riusano i
 *  valori gia' calcolati invece di riderivarli, o le due viste potrebbero divergere sullo stesso eroe»*. */
export function abilityContributions(hero: HeroInput): AbilityContribution[] {
  return hero.abilities.map((a) => {
    const damage = guaranteedDamage(a);
    const availability = availabilityWeight(a.cooldown);
    return {
      id: a.id,
      kind: a.kind,
      damage,
      cooldown: a.cooldown,
      availability,
      weighted: damage * 20 * availability,
    };
  });
}

/** Il contributo di un'abilita' **nella stessa unita' di `power_raw`**: danno per turno, pesato sulla
 *  disponibilita'. E' la metrica di confronto fra abilita' di eroi diversi (#2579).
 *
 *  🔑 **Non e' una scala nuova, ed e' il punto.** `power_raw` e' gia' pubblicato in questa unita' e
 *  l'ancora resta quella dell'asse: qui si divide per lo stesso denominatore, una volta, esattamente
 *  come fa `powerRaw`. Chi legge `10` legge *«dieci danni per turno, tenuto conto del cooldown»*, che
 *  e' la stessa frase che descrive un `power_raw` di 10.
 *
 *  ⛔ **E NON si applica `rate` per-abilita'.** `POWER_ANCHOR` vale *«uccide un eroe medio in un
 *  turno»*: e' tarata sull'**eroe**, e sulla singola abilita' restituisce `1`, `2` o `3` e nient'altro
 *  — venti abilita' del roster collassano in **tre** valori, cioe' la metrica smette di rispondere
 *  alla domanda per cui esiste. La misura sta in `power.test.ts`, non in questa frase. */
export function contributionPerTurn(contribution: AbilityContribution): number {
  return contribution.weighted / POWER_WEIGHT_SCALE;
}

/** Danno per turno dell'eroe. Aritmetica intera fino all'ultima divisione (vedi `rubric.ts`). */
export function powerRaw(hero: HeroInput): number {
  const scaled = abilityContributions(hero).reduce((sum, c) => sum + c.weighted, 0);
  return scaled / POWER_WEIGHT_SCALE;
}

export function powerRating(hero: HeroInput): number {
  return rate(powerRaw(hero), POWER_ANCHOR);
}

/** La banda di riferimento di una **categoria d'azione** — la colonna `Tipo` del catalogo (#2579).
 *
 *  🔑 **Non e' un numero scelto: e' un'ancora MISURATA sul roster.** `min` e `max` sono i contributi
 *  per turno realmente osservati fra le abilita' di quella categoria, e `from` li nomina. Una banda
 *  che non nomina nessuna abilita' non esiste — e' cio' che #2579 chiede quando dice *«una banda senza
 *  derivazione non entra»*.
 *
 *  ⛔ **Niente mediana, e la ragione e' aritmetica**: la mediana di un numero pari di valori chiede
 *  una divisione, e questa rubrica tiene tutto intero fino all'ultima (vedi `POWER_WEIGHT_SCALE`).
 *  L'intervallo osservato risponde alla domanda — *«dove cade questa abilita' fra le sue simili»* —
 *  senza introdurre una seconda divisione che `D-108` dovrebbe poi confrontare byte a byte.
 *
 *  ⚠️ **La banda e' DESCRITTIVA e non entra in nessun rating.** Non passa da `rate`, non tocca
 *  `powerRating`, e nessun radar pubblicato la consuma: `D-154` vieta un punteggio opaco come gate, e
 *  una banda che spostasse un asse sarebbe esattamente quello. Serve a leggere, non a giudicare.
 *
 *  ⚠️ **E si ricalcola, non si scrive.** Un'ancora misurata sul roster invecchia appena il roster
 *  cambia; questa funzione la deriva a ogni esecuzione, quindi non esiste un numerale da mantenere
 *  allineato a mano. */
export interface AbilityBand {
  /** La colonna `Tipo`: `attacco base` · `linea` · `AoE` · `cella` · `dash` · `arco` · `charge` ·
   *  `reazione` · `predittiva` · `controllo`. */
  kind: string;
  /** Il contributo per turno piu' basso osservato nella categoria. */
  min: number;
  /** Il piu' alto. */
  max: number;
  /** Le abilita' che producono la banda: l'ancora misurata, nominata. */
  from: string[];
}

/** Le bande del roster, una per categoria d'azione presente, in ordine alfabetico di categoria. */
export function abilityBands(heroes: HeroInput[]): AbilityBand[] {
  const perKind = new Map<string, AbilityContribution[]>();
  for (const hero of heroes) {
    for (const c of abilityContributions(hero)) {
      const gruppo = perKind.get(c.kind);
      if (gruppo) gruppo.push(c);
      else perKind.set(c.kind, [c]);
    }
  }

  return [...perKind.entries()]
    .sort(([a], [b]) => (a < b ? -1 : a > b ? 1 : 0))
    .map(([kind, contributi]) => {
      const valori = contributi.map(contributionPerTurn);
      return {
        kind,
        min: Math.min(...valori),
        max: Math.max(...valori),
        from: contributi.map((c) => c.id),
      };
    });
}
