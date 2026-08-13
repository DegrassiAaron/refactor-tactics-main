# Control Center — Execution Map v1

## Scopo

Rispondere in pochi secondi a:

1. cosa può essere fatto adesso;
2. cosa aspetta CODE;
3. cosa aspetta l'utente in PIE;
4. cosa aspetta un ASSET;
5. quale Feature/Scenario/Gate è servito da quel lavoro.

Non sostituisce Roadmap/Feature/Scenario/Editor Map.

---

## Tab

Aggiungere:

`Execution Map`

Ordine suggerito:

Overview · Roadmap · Feature Map · **Execution Map** · Scenario Map · Editor Map · My Editor Queue · Diagnostica

---

## Modalità 1 — Roads (default)

Tabella, non grafo globale.

Colonne:
- Nodo
- Release
- Lane
- Domain
- Stato owner
- Readiness derivata
- Hard prerequisites
- Soft order
- Sblocca
- Feature
- Evidence

Filtri:
- Release
- Lane
- Domain
- Readiness
- Hard only
- Search

Default:
- v0.1
- incomplete/unknown
- tutti i domain
- hard + soft distinguibili

---

## Modalità 2 — Focus Graph

Si apre scegliendo un nodo.

Mostrare:
- nodo selezionato;
- upstream hard;
- downstream hard;
- capability;
- soft order;
- related opzionali;
- evidence.

Default depth = 1.
Toggle depth = 2.

Niente force graph globale.

---

## Layout

Deterministico, no dipendenze JS esterne.

X:
- profondità topologica su edge `requires`.

Y:
- lane principale CODE / PIE / ASSET;
- per CODE, sottoriga domain group.

Stile edge:
- requires = piena;
- requires_capability = piena con nodo capability;
- follows = tratteggiata;
- related = puntinata;
- implements/verifies = sottile.

Junction:
- evidenziare un nodo se riceve >1 incoming hard da domain/lane differenti.

Il junction è una resa visuale, non uno stato source.

---

## Drawer

Per execution node:

- ID
- tipo
- ref GitHub/checkpoint/session
- release
- lane
- domain
- stato + fonte
- readiness + reason già generata
- Feature
- hard requires
- soft follows
- incoming/outgoing
- capabilities
- scenario/evidence
- rationale degli edge

Per `session:U7`:
- ASSET
- artifacts
- verifies PIE
- shares setup
- implements #287
- #593 related ma NON blocking.

---

## Empty/error states

- execution source assente:
  “Execution Map non ancora generata”.
- node ref rotto:
  rosso, mai nascosto.
- readiness unknown:
  `UNKNOWN`, con source assente.
- graph stale:
  banner già esistente, esteso alle execution source.

---

## Accessibility/leggibilità

- non affidarsi solo al colore;
- scrivere il tipo edge;
- zoom browser deve reggere;
- tabella resta sempre disponibile come fallback;
- Focus Graph non può essere l'unico modo di capire la dipendenza.

---

## Thin slice visual acceptance

### #165

Deve far vedere:
- downstream hard #166 / #314 / #512;
- DecisionBoundary;
- #166→#314 come soft order, non blocker.

### #170

Deve far vedere:
- incoming hard da #512, #66, #75;
- #625 come soft;
- #649/#687 related;
- #171 come downstream.

### U7

Deve far vedere:
- lane ASSET;
- PIE-AS2 / PIE-FACING;
- U8 shared/follows;
- implements #287;
- #593 related, non blocker.
