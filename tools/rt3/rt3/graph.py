"""Grafo delle dipendenze di una roadmap: cicli, ordine topologico, critical path.

Modulo PURO, come `routing.py`: nessun database, nessuna rete, nessun ambiente. Le
domande a cui risponde si provano su un dizionario, e un test del critical path non ha
bisogno di un daemon acceso.

    build()            archi, predecessori, successori
    cycle_problems()   i cicli, con la catena per esteso
    topological()      un ordine di esecuzione ammissibile
    schedule()         earliest/latest start, slack, critical path

Il critical path e' calcolato col metodo del cammino critico su pesi di NODO
(`estimate`), non di arco: in una roadmap la durata sta nel lavoro, e la dipendenza e'
istantanea. La conseguenza pratica e' che il cammino critico non e' il piu' lungo per
numero di issue, ma per somma di stime - ed e' quello che si vuole sapere.

⚠️ Il critical path e' una proprieta' del PLAN, non dello SCHEDULE: NON tiene conto
delle capacita' (un writer per workspace, un lease Unreal). E' voluto, ed e' la
definizione standard: il cammino critico dice qual e' il limite inferiore imposto dalle
DIPENDENZE. Il limite imposto dalle RISORSE lo calcola `planner.py`, e i due numeri
diversi sono un'informazione, non una contraddizione.
"""

import collections

from .roadmap import Problem


class Graph(object):
    """Vista di adiacenza di una roadmap. `order` conserva l'ordine di dichiarazione."""

    __slots__ = ("roadmap", "order", "predecessors", "successors", "gates")

    def __init__(self, roadmap, order, predecessors, successors, gates):
        self.roadmap = roadmap
        self.order = order
        self.predecessors = predecessors
        self.successors = successors
        self.gates = gates

    def estimate(self, key):
        item = self.roadmap.get(key)
        return int(item.estimate or 0) if item else 0

    def as_dict(self):
        return {
            "nodes": [
                {
                    "key": key,
                    "estimate": self.estimate(key),
                    "requires": [
                        {"item": p, "gate": self.gates[(p, key)]}
                        for p in self.predecessors[key]
                    ],
                    "unlocks": list(self.successors[key]),
                }
                for key in self.order
            ],
            "edges": [
                {"from": p, "to": s, "gate": self.gates[(p, s)]}
                for s in self.order
                for p in self.predecessors[s]
            ],
        }


def build(roadmap):
    """Costruisce il grafo. Assume i riferimenti gia' risolti da `roadmap.normalize`."""
    order = list(roadmap.items.keys())
    predecessors = collections.OrderedDict((k, []) for k in order)
    successors = collections.OrderedDict((k, []) for k in order)
    gates = {}

    for key in order:
        for dep in roadmap.items[key].requires:
            target = dep["item"]
            if target not in predecessors:
                continue  # gia' segnalato come riferimento ignoto
            if target not in predecessors[key]:
                predecessors[key].append(target)
                successors[target].append(key)
            gates[(target, key)] = dep["gate"]

    return Graph(roadmap, order, predecessors, successors, gates)


# ---------------------------------------------------------------------------
# Cicli
# ---------------------------------------------------------------------------


def find_cycles(graph):
    """Elenca i cicli come catene di chiavi, ognuna chiusa sul primo elemento.

    DFS iterativa con marcatura a tre colori. Ricorsiva sarebbe piu' corta e finirebbe
    per sbattere contro il limite di ricorsione su una roadmap profonda; e il limite si
    manifesterebbe come `RecursionError` dentro un comando di validazione, che e'
    l'ultimo posto in cui si vuole un traceback.
    """
    WHITE, GREY, BLACK = 0, 1, 2
    color = {k: WHITE for k in graph.order}
    cycles = []
    seen_signature = set()

    for root in graph.order:
        if color[root] != WHITE:
            continue
        stack = [(root, iter(graph.successors[root]))]
        path = [root]
        color[root] = GREY
        while stack:
            node, children = stack[-1]
            advanced = False
            for child in children:
                if color[child] == GREY:
                    start = path.index(child)
                    chain = path[start:] + [child]
                    signature = tuple(sorted(chain[:-1]))
                    if signature not in seen_signature:
                        seen_signature.add(signature)
                        cycles.append(chain)
                    continue
                if color[child] == WHITE:
                    color[child] = GREY
                    path.append(child)
                    stack.append((child, iter(graph.successors[child])))
                    advanced = True
                    break
            if not advanced:
                color[node] = BLACK
                stack.pop()
                path.pop()
    return cycles


def cycle_problems(graph):
    """I cicli come `Problem` di livello ERROR."""
    problems = []
    for chain in find_cycles(graph):
        problems.append(
            Problem(
                "ERROR",
                "ROADMAP_DEPENDENCY_CYCLE",
                chain[0],
                "ciclo di dipendenze: {}. Nessuna delle issue coinvolte potrebbe mai "
                "diventare READY.".format(" -> ".join(chain)),
            )
        )
    return problems


# ---------------------------------------------------------------------------
# Ordine topologico
# ---------------------------------------------------------------------------


def topological(graph):
    """Ordine di esecuzione ammissibile. Kahn, con coda ordinata per stabilita'.

    Il tie-break e' l'ordine di DICHIARAZIONE, non l'alfabetico: e' l'unica priorita'
    che qualcuno ha scritto davvero. Se il grafo ha un ciclo, l'ordine ritornato e'
    parziale - i nodi del ciclo mancano - e chi chiama deve aver gia' controllato.
    """
    position = {key: i for i, key in enumerate(graph.order)}
    remaining = {k: len(graph.predecessors[k]) for k in graph.order}
    ready = sorted([k for k, n in remaining.items() if n == 0], key=position.get)
    result = []
    while ready:
        node = ready.pop(0)
        result.append(node)
        for successor in graph.successors[node]:
            remaining[successor] -= 1
            if remaining[successor] == 0:
                ready.append(successor)
                ready.sort(key=position.get)
    return result


# ---------------------------------------------------------------------------
# Critical path
# ---------------------------------------------------------------------------

ScheduleRow = collections.namedtuple(
    "ScheduleRow", "key estimate earliest_start earliest_finish latest_start "
    "latest_finish slack critical"
)


def schedule(graph):
    """Earliest/latest start, slack e criticita' per ogni nodo.

    Ritorna `(rows, duration)` con `rows` un OrderedDict chiave -> ScheduleRow. Su un
    grafo ciclico ritorna `(OrderedDict(), 0)`: calcolare un cammino critico su un ciclo
    darebbe un numero, e un numero sbagliato e' peggio di nessun numero.
    """
    order = topological(graph)
    if len(order) != len(graph.order):
        return collections.OrderedDict(), 0

    earliest_start, earliest_finish = {}, {}
    for key in order:
        start = 0
        for pred in graph.predecessors[key]:
            start = max(start, earliest_finish[pred])
        earliest_start[key] = start
        earliest_finish[key] = start + graph.estimate(key)

    duration = max(earliest_finish.values()) if earliest_finish else 0

    latest_finish, latest_start = {}, {}
    for key in reversed(order):
        if not graph.successors[key]:
            finish = duration
        else:
            finish = min(latest_start[s] for s in graph.successors[key])
        latest_finish[key] = finish
        latest_start[key] = finish - graph.estimate(key)

    rows = collections.OrderedDict()
    for key in graph.order:
        slack = latest_start[key] - earliest_start[key]
        rows[key] = ScheduleRow(
            key=key,
            estimate=graph.estimate(key),
            earliest_start=earliest_start[key],
            earliest_finish=earliest_finish[key],
            latest_start=latest_start[key],
            latest_finish=latest_finish[key],
            slack=slack,
            critical=(slack == 0),
        )
    return rows, duration


def critical_path(graph):
    """La catena critica come lista di chiavi, piu' la durata del progetto.

    Ci sono roadmap con PIU' cammini critici di pari durata. Questa funzione ne ritorna
    UNO, scelto in modo deterministico: fra i nodi critici si segue sempre il primo in
    ordine di dichiarazione. `rows` porta il flag `critical` per tutti, quindi
    l'informazione completa non va persa.
    """
    rows, duration = schedule(graph)
    if not rows:
        return [], 0

    position = {key: i for i, key in enumerate(graph.order)}
    critical_nodes = [k for k, r in rows.items() if r.critical]
    if not critical_nodes:
        return [], duration

    # Partenza: nodo critico senza predecessori critici, primo per dichiarazione.
    starts = [
        k
        for k in critical_nodes
        if not any(rows[p].critical for p in graph.predecessors[k])
    ]
    if not starts:
        return [], duration
    node = sorted(starts, key=position.get)[0]

    path = [node]
    while True:
        nxt = [
            s
            for s in graph.successors[node]
            if rows[s].critical and rows[s].earliest_start == rows[node].earliest_finish
        ]
        if not nxt:
            break
        node = sorted(nxt, key=position.get)[0]
        path.append(node)
    return path, duration


# ---------------------------------------------------------------------------
# Ingresso unico
# ---------------------------------------------------------------------------


def load_checked(path):
    """Carica, normalizza e verifica i cicli. Un solo punto d'ingresso.

    Esiste perche' la validazione ha DUE meta': `roadmap.normalize` prova la forma e
    risolve i riferimenti, `find_cycles` prova che il grafo sia percorribile. Due
    chiamate separate lasciate ai chiamanti producono, prima o poi, un chiamante che ne
    fa una sola - e carica una roadmap con un ciclo.
    """
    from .roadmap import RoadmapError, load

    roadmap, problems = load(path)
    graph = build(roadmap)
    cycles = cycle_problems(graph)
    if cycles:
        problems = list(problems) + cycles
        raise RoadmapError(
            "roadmap {} non valida: {} ciclo/i di dipendenze.".format(path, len(cycles)),
            problems=problems,
        )
    return roadmap, graph, problems
