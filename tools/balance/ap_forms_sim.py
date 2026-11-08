# -*- coding: utf-8 -*-
"""Simulazione per MSP-1.n (D-584 (3), proposta in docs/roadmap/plans/e38-action-point-numeri-simulazione-2026-11-08.md):

Uso: PYTHONIOENCODING=utf-8 python tools/balance/ap_forms_sim.py
 forme di turno legali oggi (slot) contro forme legali con un pool di AP.

Una 'forma' e' il multinsieme delle classi di fase delle azioni dichiarate in un turno:
  P = Prep (principale che risolve presto), D = Dash (mobilita' speciale), B = Blast (principale),
  M = Move (camminare), R = reazione, C = Cleanup (azioni ambientali: oggi occupano la principale).
Oggi: al piu' 1 fra {D, M} (slot Movimento), al piu' 1 fra {P, B, C} (slot Principale), al piu' 1 R.
Domani (D-584): nessun vincolo di slot; somma dei costi <= budget; piu' azioni nella stessa fase ammesse.
"""
import itertools, collections

CLASSES = "PDBMRC"
# forme legali oggi: prodotto delle tre scelte di slot (vuoto incluso)
today = set()
for mv in ("", "D", "M"):
    for mn in ("", "P", "B", "C"):
        for re_ in ("", "R"):
            today.add("".join(sorted(mv + mn + re_)))
today.discard("")

def shapes_for(costs, budget, max_actions=6):
    """Tutti i multinsiemi di classi con somma costi <= budget."""
    out = set()
    for n in range(1, max_actions + 1):
        for combo in itertools.combinations_with_replacement(CLASSES, n):
            if sum(costs[c] for c in combo) <= budget:
                out.add("".join(sorted(combo)))
    return out

def evaluate(costs, budget):
    s = shapes_for(costs, budget)
    lost = today - s
    new = s - today
    maxn = max(len(x) for x in s) if s else 0
    return s, lost, new, maxn

def fmt(costs):
    return " ".join("%s%d" % (c, costs[c]) for c in CLASSES)

rows = []
for P in (1, 2):
    for D in (1, 2, 3):
        for B in (1, 2, 3):
            for M in (1, 2):
                for R in (1, 2):
                    for C in (1, 2):
                        costs = dict(P=P, D=D, B=B, M=M, R=R, C=C)
                        for budget in range(3, 9):
                            s, lost, new, maxn = evaluate(costs, budget)
                            if lost:
                                continue
                            small = sorted(x for x in new if len(x) <= 3)
                            big = [x for x in new if len(x) >= 4]
                            rows.append((len(small), len(big), budget, maxn, fmt(costs), small))

rows.sort()
print("forme legali oggi (%d):" % len(today), " ".join(sorted(today, key=lambda x: (len(x), x))))
print()
print("| nuove forme (2-3 azioni) | nuove forme (4+ azioni) | budget | max azioni | costi (P D B M R C) | le nuove forme a 2-3 azioni |")
print("|---|---|---|---|---|---|")
for n_small, n_big, budget, maxn, costs, small in rows[:14]:
    print("| %d | %d | %d | %d | `%s` | %s |" % (n_small, n_big, budget, maxn, costs, " ".join("`%s`" % x for x in small) if small else "—"))

# la specifica: P1 D2 B2 (ultimate 3) R2, M non definito; proviamo M=2 e budget 6
print()
print("== vettore della specifica, letto sulle cinque fasi ==")
for M in (1, 2):
    costs = dict(P=1, D=2, B=2, M=M, R=2, C=2)
    s, lost, new, maxn = evaluate(costs, 6)
    small = sorted(x for x in new if len(x) <= 3); big = [x for x in new if len(x) >= 4]
    print("costi %s budget 6 -> perse %s | nuove a 2-3 azioni %d: %s | nuove a 4+ azioni %d | max azioni %d" % (fmt(costs), sorted(lost) or "-", len(small), " ".join(small), len(big), maxn))
print("turno pieno Prep(1)+Dash(2)+Ultimate(3) = 6: la reazione (2) non entra:", 1 + 2 + 3 + 2 > 6)
