"""Da PLAN a SCHEDULE e ASSIGNMENT: cosa e' eseguibile adesso, e chi lo prende.

Modulo PURO: prende una roadmap, un grafo e un dizionario di stati, e ritorna numeri.
Non apre il database, non parla col daemon, non crea worktree e non lancia Unreal. E'
la ragione per cui il planner si prova senza avviare niente.

## I due gradini

    readiness()   cosa e' pronto        dipende SOLO da dipendenze + stato
    plan()        chi lo esegue         dipende ANCHE dalle capacita'

Sono separati perche' rispondono a due domande che si confondono facilmente. Una issue
puo' essere READY e non essere pianificata: significa che nulla la blocca, ma la
risorsa che le serve e' occupata. Chiamare «BLOCKED» quel caso nasconderebbe che il
problema e' la capacita', non il piano - e la correzione e' diversa.

## Determinismo

L'ordine e' l'invariante di questo modulo, non un dettaglio: due workspace che
chiedono un piano sullo stesso stato devono ottenere lo STESSO piano, altrimenti due
sessioni prendono due decisioni diverse credendo entrambe di seguire il planner.

Priorita', in ordine, tutte totali:

    1. slack crescente          il cammino critico per primo
    2. earliest start crescente cio' che puo' partire prima
    3. ordine di dichiarazione  l'unica priorita' che qualcuno ha scritto

Non c'e' alcun tie-break casuale, nessun `set` iterato, nessun `dict` non ordinato.

## Capacita'

    writerCapacity        scrittori PERMANENTI per workspace group
    temporaryWorktrees    worktree temporanei, quando il permanente e' occupato
    unrealEditor          lease dell'Editor, esclusivo per macchina
    wip                   lavoro in corso, per workspace e globale (0 = nessun limite)

⛔ Il planner SUGGERISCE il worktree temporaneo, non lo crea. La creazione e' data
plane, e questo modulo non tocca il data plane: la milestone corrente verifica che la
decisione sia giusta, e chi la esegue e' un'altra questione.
"""

import collections

from .model import ITEM_PROGRESS_STATES

#: Stati DERIVATI. Non sono persistiti da nessuna parte (vedi `model.ITEM_PROGRESS_STATES`).
READINESS_STATES = ("READY", "BLOCKED", "IN_PROGRESS", "VALIDATED", "DONE")

#: Quale stato di avanzamento soddisfa quale gate. `DONE` soddisfa anche un gate
#: `VALIDATED`: una issue chiusa e' stata validata, e pretendere il contrario
#: bloccherebbe per sempre chi dipende da lavoro gia' finito.
_GATE_SATISFIED_BY = {
    "VALIDATED": ("VALIDATED", "DONE"),
    "DONE": ("DONE",),
}

#: Modi con cui una issue puo' occupare un workspace.
ASSIGNMENT_MODES = ("PERMANENT_WRITER", "TEMPORARY_WORKTREE_SUGGESTED")

#: Perche' una issue READY non e' stata pianificata. Codici stabili: la CLI li stampa e
#: uno script puo' confrontarli senza leggere la prosa.
DEFER_REASONS = (
    "WRITER_CAPACITY",
    "TEMPORARY_WORKTREE_CAPACITY",
    "UNREAL_LEASE_CAPACITY",
    "WIP_PER_WORKSPACE",
    "WIP_GLOBAL",
    "NO_EXECUTION_WORKSPACE",
)


Readiness = collections.namedtuple(
    "Readiness", "key state progress blocked_by unmet"
)

#: `ownerSessionId` e' popolato SOLO quando la risorsa assegnata e' gia' posseduta da
#: una sessione viva: dice che quel lavoro va a QUELLA sessione, nel suo albero. Vuoto
#: significa «serve una sessione nuova», non «non si sa».
Assignment = collections.namedtuple(
    "Assignment", "key workspace mode resources reason ownerSessionId"
)

Deferred = collections.namedtuple("Deferred", "key workspace reason detail")


# ---------------------------------------------------------------------------
# Readiness
# ---------------------------------------------------------------------------


def normalize_states(roadmap, states):
    """Completa il dizionario degli stati: ogni item della roadmap ha un valore.

    Un item assente vale `PENDING`. E' il default giusto e non «sconosciuto»: una
    roadmap appena caricata non ha ancora nessuna riga di stato, e senza questo default
    la prima query fallirebbe su ogni item.
    """
    result = collections.OrderedDict()
    for key in roadmap.items:
        value = (states or {}).get(key, "PENDING")
        if value not in ITEM_PROGRESS_STATES:
            raise ValueError(
                "stato di avanzamento non valido per {}: {!r}. Ammessi: {}.".format(
                    key, value, ", ".join(ITEM_PROGRESS_STATES)
                )
            )
        result[key] = value
    return result


def readiness(roadmap, graph, states=None):
    """Stato derivato di ogni item. Ritorna un OrderedDict chiave -> Readiness."""
    progress = normalize_states(roadmap, states)
    result = collections.OrderedDict()

    for key in roadmap.items:
        current = progress[key]
        if current in ("DONE", "VALIDATED", "IN_PROGRESS"):
            result[key] = Readiness(
                key=key, state=current, progress=current, blocked_by=[], unmet=[]
            )
            continue

        unmet = []
        for pred in graph.predecessors[key]:
            gate = graph.gates[(pred, key)]
            if progress[pred] not in _GATE_SATISFIED_BY[gate]:
                unmet.append(
                    {"item": pred, "gate": gate, "actual": progress[pred]}
                )

        result[key] = Readiness(
            key=key,
            state="BLOCKED" if unmet else "READY",
            progress=current,
            blocked_by=[u["item"] for u in unmet],
            unmet=unmet,
        )
    return result


def ready_keys(roadmap, graph, states=None):
    return [k for k, r in readiness(roadmap, graph, states).items() if r.state == "READY"]


# ---------------------------------------------------------------------------
# Priorita'
# ---------------------------------------------------------------------------


def priority_index(roadmap, graph):
    """Chiave di ordinamento per ogni item: `(slack, earliest_start, posizione)`."""
    from .graph import schedule

    rows, _duration = schedule(graph)
    position = {key: i for i, key in enumerate(graph.order)}
    index = {}
    for key in roadmap.items:
        row = rows.get(key)
        if row is None:
            index[key] = (10**9, 10**9, position.get(key, 0))
        else:
            index[key] = (row.slack, row.earliest_start, position.get(key, 0))
    return index


# ---------------------------------------------------------------------------
# Piano
# ---------------------------------------------------------------------------


class _Capacity(object):
    """Contatore delle risorse consumate mentre il piano si costruisce."""

    def __init__(self, roadmap):
        self.roadmap = roadmap
        self.writers = collections.Counter()
        self.temporary = 0
        self.unreal = 0
        self.wip_workspace = collections.Counter()
        self.wip_global = 0
        #: Chi tiene le risorse secondo il RUNTIME. Serve ai messaggi: un rimando che
        #: dice «occupato» senza dire DA CHI lascia l'operatore a cercarlo a mano.
        self.runtime_writers = []
        self.runtime_unreal = []
        #: Le due fonti dei writer permanenti, tenute separate finche' `consolidate()`
        #: non le fonde col massimo. Sommarle conterebbe due volte lo stesso lavoro.
        self.runtime_count = collections.Counter()
        self.declared_count = collections.Counter()
        #: 🔴 I writer permanenti gia' posseduti da una sessione DISPONIBILE, per
        #: gruppo. Non sono capacita' consumata: sono capacita' con un nome sopra.
        #: Tenere un lease dice CHI puo' scrivere in quell'albero, non che una unita'
        #: di lavoro concorrente sia gia' schedulata la' dentro.
        self.permanent_owners = collections.defaultdict(list)
        #: Chi tiene il writer PERMANENTE di ogni gruppo: il proprietario di un lease
        #: non riutilizzabile, oppure la sessione a cui il piano lo assegna adesso.
        #: Non e' «tutti i writer del gruppo»: una lane puo' avere anche scrittori in
        #: alberi temporanei, e nominarli come occupanti del permanente produce un
        #: messaggio che si contraddice - misurato: a DEV-B-2 veniva detto che il
        #: writer era occupato da DEV-B-2.
        self.permanent_holder = {}

    def occupy_runtime(self, runtime, progress=None):
        """Occupa le risorse che il RUNTIME dichiara possedute adesso.

        🔴 Chiude il difetto che l'audit ha misurato: il planner leggeva solo gli item
        `IN_PROGRESS` della roadmap e riportava `writer DEV 0/1` mentre due sessioni
        tenevano davvero quell'albero. Il piano e la realta' erano due mondi.

        ⚠️ Quando c'e' una snapshot runtime, i writer permanenti si contano DA LI' e non
        dagli item in corso. Sommare le due fonti conterebbe due volte lo stesso lavoro
        - la issue in corso E il lease che la sessione tiene per farla - e produrrebbe
        una capacita' satura al doppio della verita'.

        Un lease `stale` (proprietario non piu' attivo) conta comunque come occupato: la
        risorsa resta rivendicata finche' qualcuno non la rilascia, e ignorarla qui
        significherebbe pianificare sopra un albero che forse ha ancora uno scrittore.

        🔴 Possedere una risorsa NON e' occuparla con lavoro concorrente. Un lease
        writer tenuto da una sessione viva e libera dice chi puo' scrivere in
        quell'albero, non che dentro ci sia gia' una lavorazione in corso. Contarlo
        come occupancy era il difetto misurato il 2026-09-07: con `DEV-1` proprietario
        e libera, due issue pronte e un solo worktree temporaneo, il piano assegnava
        la prima al TEMPORANEO e rimandava la seconda per capacita' esaurita - mentre
        la risposta giusta era la prima a `DEV-1` nel suo albero e la seconda al
        temporaneo, che cosi' restava disponibile.
        """
        if not runtime:
            return
        sessioni = {s["sessionId"]: s for s in (runtime.get("sessions") or [])}
        for chiave in sorted((runtime.get("writers") or {})):
            voce = runtime["writers"][chiave]
            gruppo = voce.get("workspaceGroup")
            if gruppo:
                if self._riusabile(voce, sessioni, progress):
                    # Ownership senza occupancy: la risorsa ha un padrone, non un
                    # lavoro concorrente. La prossima assegnazione su questo gruppo
                    # va a lui, e NON consuma un worktree temporaneo.
                    self.permanent_owners[gruppo].append(voce["ownerSessionId"])
                else:
                    self.runtime_count[gruppo] += 1
                    self.permanent_holder.setdefault(gruppo, voce["ownerSessionId"])
            self.runtime_writers.append(voce)
        for chiave in sorted((runtime.get("unreal") or {})):
            voce = runtime["unreal"][chiave]
            self.unreal += 1
            self.runtime_unreal.append(voce)

    def _riusabile(self, voce, sessioni, progress):
        """Il proprietario di questo lease puo' ricevere una NUOVA assegnazione?

        Quattro modi di rispondere no, tutti conservativi. Il costo di sbagliare non e'
        simmetrico: trattare per errore un lease come riutilizzabile manda due
        scrittori nello stesso albero, trattarlo per errore come occupato rimanda una
        issue. Nel dubbio, occupato.

        ⛔ RT3 non ruba mai un lease. «Riutilizzabile» significa che il piano assegna
        altro lavoro a chi la risorsa ce l'ha gia', non che la tolga a qualcuno.
        """
        if voce.get("stale"):
            return False                      # il proprietario non risponde piu'
        owner = voce.get("ownerSessionId")
        if not owner:
            return False                      # lease senza padrone identificabile
        sessione = sessioni.get(owner)
        if sessione is None or sessione.get("status") != "ACTIVE":
            # Senza elenco delle sessioni non si puo' affermare che sia viva. E' il
            # ramo che preserva il comportamento di prima quando lo snapshot e'
            # parziale o assente.
            return False
        return not self._ha_lavoro_in_corso(sessione, progress)

    @staticmethod
    def _ha_lavoro_in_corso(sessione, progress):
        """La sessione sta gia' eseguendo una issue ancora aperta?

        ⚠️ `taskId` da solo non basta: resta scritto anche dopo che la issue e' stata
        validata, e leggerlo come «occupata» terrebbe fermo per sempre un writer che si
        e' liberato. Conta se quel task e' ANCORA `IN_PROGRESS`.
        """
        task = (sessione or {}).get("taskId")
        if not task or not progress:
            return False
        candidati = [k for k in progress
                     if k == task or k.split("/")[-1] == task]
        if len(candidati) > 1:
            return True                       # id ambiguo: non si indovina, si occupa
        return bool(candidati) and progress[candidati[0]] == "IN_PROGRESS"

    def claim_permanent_owner(self, group):
        """Assegna il writer permanente di quel gruppo a chi gia' lo possiede.

        Consuma: due assegnazioni concorrenti non possono ereditare lo stesso
        proprietario, perche' una sessione sola non lavora due issue insieme.
        """
        disponibili = self.permanent_owners.get(group)
        if not disponibili:
            return None
        owner = disponibili.pop(0)
        self.permanent_holder[group] = owner
        return owner

    def consolidate(self):
        """Fonde le due fonti dei writer permanenti: il RUNTIME e gli item dichiarati.

        🔴 Il massimo, non la somma. Sommarli conterebbe due volte lo stesso lavoro -
        la issue in corso E il lease che la sessione tiene per farla. Ignorarne una
        perderebbe un caso reale ciascuna:

            solo runtime    una sessione tiene il writer senza che nessuna issue sia
                            dichiarata IN_PROGRESS: succede appena si apre il terminale
            solo dichiarato una issue e' IN_PROGRESS ma nessuno tiene il lease: succede
                            quando la sessione si e' fermata lasciando il lavoro aperto

        La lettura e' conservativa: la risorsa e' occupata se lo dice ALMENO una delle
        due. Occupare di piu' del vero fa rimandare una issue; occupare di meno manda
        due scrittori nello stesso albero.
        """
        for gruppo in set(self.runtime_count) | set(self.declared_count):
            self.writers[gruppo] = max(
                self.runtime_count[gruppo], self.declared_count[gruppo]
            )

    def occupy_existing(self, roadmap, progress, modes=None, runtime=None):
        """Il lavoro GIA' in corso occupa risorse prima che il piano cominci.

        Senza questo passo il planner proporrebbe un writer permanente su un workspace
        dove una sessione sta gia' scrivendo - cioe' esattamente la collisione che le
        capacita' esistono per impedire.

        🔴 Ogni item in corso occupa la risorsa DOVE STA, non quella del suo gruppo.
        Contarli tutti come writer permanenti era il difetto misurato: il planner
        suggeriva un worktree temporaneo, e al giro dopo leggeva quello stesso item come
        writer permanente, arrivando a `used 2 / capacity 1`. La modalita' non era
        scritta da nessuna parte, quindi il piano non poteva ricordare se stesso.

        ⚠️ Modalita' assente = `PERMANENT_WRITER`. E' la lettura conservativa: occupa la
        risorsa piu' scarsa. Assumere il temporaneo libererebbe un writer che magari e'
        occupato davvero, e la collisione si scoprirebbe solo a due sessioni che
        scrivono nello stesso albero.
        """
        modes = modes or {}
        runtime_autorevole = bool(runtime and runtime.get("writers") is not None)
        for key, state in progress.items():
            if state != "IN_PROGRESS":
                continue
            item = roadmap.items[key]
            if modes.get(key) == "TEMPORARY_WORKTREE":
                self.temporary += 1
            elif item.execution_work:
                # Una delle due fonti. `consolidate()` la fonde col runtime prendendo
                # il MASSIMO, non la somma.
                self.declared_count[item.execution_work] += 1
            # Il WIP conta comunque, e conta sul GRUPPO: un worktree temporaneo non e'
            # un quarto workspace, e' un secondo posto dove lo stesso gruppo lavora.
            if item.execution_work:
                self.wip_workspace[item.execution_work] += 1
            self.wip_global += 1
            if item.needs_unreal:
                self.unreal += 1

    def over_committed(self):
        """Risorse il cui uso SUPERA la capacita' dichiarata.

        Non e' un errore del planner: il planner non sfora mai per conto proprio. E'
        cio' che accade quando lo STATO dichiarato descrive piu' lavoro in corso di
        quanto la roadmap ammetta - due sessioni che si dichiarano IN_PROGRESS sullo
        stesso gruppo senza passare di qui.

        ⛔ Va DICHIARATO, non corretto in silenzio. Un piano che nasconde uno stato
        impossibile e' peggio di uno che lo espone: chi legge crede che il vincolo
        regga.
        """
        over = []
        for group in sorted(self.roadmap.resources["workspaces"]):
            capacity = self.roadmap.writer_capacity(group)
            if self.writers[group] > capacity:
                over.append(
                    {
                        "resource": "writer:{}".format(group),
                        "used": self.writers[group],
                        "capacity": capacity,
                        "detail": "lo stato dichiara {} issue in corso su {} con "
                        "writerCapacity {}. Il piano non lo ha prodotto: qualcuno le ha "
                        "dichiarate IN_PROGRESS senza passare dal planner, oppure senza "
                        "dichiarare `--mode TEMPORARY_WORKTREE`.".format(
                            self.writers[group], group, capacity
                        ),
                    }
                )
        if self.temporary > self.roadmap.temporary_worktree_capacity:
            over.append(
                {
                    "resource": "temporaryWorktrees",
                    "used": self.temporary,
                    "capacity": self.roadmap.temporary_worktree_capacity,
                    "detail": "piu' worktree temporanei in uso di quanti la roadmap ne "
                    "ammetta.",
                }
            )
        if self.unreal > self.roadmap.unreal_lease_capacity:
            over.append(
                {
                    "resource": "unrealEditor",
                    "used": self.unreal,
                    "capacity": self.roadmap.unreal_lease_capacity,
                    "detail": "piu' attivita' Unreal in corso dei lease disponibili. "
                    "L'Editor e' esclusivo per macchina: questo stato non e' eseguibile.",
                }
            )
        return over

    def writer_free(self, group):
        return self.writers[group] < self.roadmap.writer_capacity(group)

    def temporary_free(self):
        return self.temporary < self.roadmap.temporary_worktree_capacity

    def unreal_free(self):
        return self.unreal < self.roadmap.unreal_lease_capacity

    def wip_workspace_free(self, group):
        limit = int(self.roadmap.wip.get("perWorkspace") or 0)
        return limit == 0 or self.wip_workspace[group] < limit

    def wip_global_free(self):
        limit = int(self.roadmap.wip.get("global") or 0)
        return limit == 0 or self.wip_global < limit


def _consumers(assignments):
    """Chi ha consumato il budget prima di questa issue. Dice CHI, non solo QUANTO.

    Un «limite raggiunto» senza il nome di chi lo ha raggiunto lascia il lettore a
    indovinare, e la domanda che si pone davanti a una lane ferma non e' «quanto vale il
    limite» ma «chi me lo ha preso».
    """
    return ", ".join(a.key for a in assignments)


def _occupato_da(capacity, group):
    """« da DEV-1», quando il runtime sa chi tiene quell'albero. Stringa vuota altrimenti.

    Un rimando che dice «occupato» senza dire da chi lascia l'operatore a cercarlo a
    mano, ed e' l'informazione che serve per decidere se aspettare o aprire un albero.
    """
    tiene = capacity.permanent_holder.get(group)
    return " da " + tiene if tiene else ""


def _starvation_note(capacity, roadmap, group):
    """Quando una lane resta ferma con le PROPRIE risorse libere, dillo.

    🔴 E' il caso che la prima wave su Epic vere ha prodotto: DESIGNER rimandato per WIP
    globale mentre il suo writer era a 0/1. Il comportamento e' voluto - la precedenza e'
    del cammino critico, ed e' una decisione accettata (D-345) - ma senza questa riga
    sembra un difetto: si legge «limite raggiunto» accanto a una risorsa libera e non si
    capisce quale delle due informazioni sia sbagliata.
    """
    libere = []
    if capacity.writers[group] < roadmap.writer_capacity(group):
        libere.append(
            "writer {} ({}/{})".format(
                group, capacity.writers[group], roadmap.writer_capacity(group)
            )
        )
    if capacity.temporary < roadmap.temporary_worktree_capacity:
        libere.append(
            "worktree temporanei ({}/{})".format(
                capacity.temporary, roadmap.temporary_worktree_capacity
            )
        )
    if not libere:
        return "Nessuna risorsa di {} e' libera comunque.".format(group)
    return (
        "⚠️ {} ha risorse LIBERE - {} - e resta fermo lo stesso: il budget globale e' "
        "gia' stato speso da item con priorita' maggiore. La precedenza del cammino "
        "critico e' assoluta per decisione accettata, non un difetto di questo piano; "
        "alzare `wip.global` e' la leva che libera questa lane.".format(
            group, " e ".join(libere)
        )
    )


def plan(roadmap, graph, states=None, modes=None, runtime=None):
    """Costruisce lo SCHEDULE e gli ASSIGNMENT a partire dallo stato corrente.

    Ritorna un dict con `assignments`, `deferred`, `readiness`, `capacity`, `wip` e
    `overCommitted`. Non muta nulla: e' una funzione del solo stato che le viene passato.

    `modes` dice DOVE ogni issue in corso viene lavorata - il campo senza il quale il
    piano non ricorda le proprie decisioni. Vedi `_Capacity.occupy_existing`.
    """
    progress = normalize_states(roadmap, states)
    ready = readiness(roadmap, graph, progress)
    index = priority_index(roadmap, graph)

    capacity = _Capacity(roadmap)
    capacity.occupy_runtime(runtime, progress)
    capacity.occupy_existing(roadmap, progress, modes, runtime)
    capacity.consolidate()

    candidates = sorted(
        [k for k, r in ready.items() if r.state == "READY"], key=lambda k: index[k]
    )

    assignments, deferred = [], []

    for key in candidates:
        item = roadmap.items[key]
        group = item.execution_work

        if not group:
            deferred.append(
                Deferred(
                    key=key,
                    workspace=None,
                    reason="NO_EXECUTION_WORKSPACE",
                    detail="la issue non dichiara `executionWork` e l'epic non ha "
                    "`homeWork`: non c'e' un workspace dove eseguirla.",
                )
            )
            continue

        if not capacity.wip_global_free():
            deferred.append(
                Deferred(
                    key=key,
                    workspace=group,
                    reason="WIP_GLOBAL",
                    detail="limite WIP globale raggiunto ({}), consumato da {}. {}".format(
                        roadmap.wip.get("global"),
                        _consumers(assignments) or "lavoro gia' in corso",
                        _starvation_note(capacity, roadmap, group),
                    ),
                )
            )
            continue

        if not capacity.wip_workspace_free(group):
            deferred.append(
                Deferred(
                    key=key,
                    workspace=group,
                    reason="WIP_PER_WORKSPACE",
                    detail="limite WIP del workspace {} raggiunto ({}).".format(
                        group, roadmap.wip.get("perWorkspace")
                    ),
                )
            )
            continue

        # ⚠️ Il lease Unreal si verifica PRIMA di consumare un writer: consumarlo e poi
        # scoprire il lease pieno lascerebbe un writer occupato da una issue che non
        # parte, e la issue successiva verrebbe rimandata per una capacita' che in
        # realta' e' libera.
        if item.needs_unreal and not capacity.unreal_free():
            deferred.append(
                Deferred(
                    key=key,
                    workspace=group,
                    reason="UNREAL_LEASE_CAPACITY",
                    detail="richiede UNREAL_EDITOR, e i lease disponibili ({}) sono "
                    "gia' impegnati{}. L'Editor e' esclusivo: la issue non e' "
                    "schedulabile finche' non viene rilasciato.".format(
                        roadmap.unreal_lease_capacity,
                        " da " + ", ".join(
                            v["ownerSessionId"] for v in capacity.runtime_unreal
                        )
                        if capacity.runtime_unreal
                        else "",
                    ),
                )
            )
            continue

        owner = None
        if capacity.writer_free(group):
            mode = "PERMANENT_WRITER"
            # Se quell'albero ha gia' un padrone vivo e libero, il lavoro va a LUI: non
            # si apre una sessione nuova per una risorsa che qualcuno tiene gia'.
            owner = capacity.claim_permanent_owner(group)
            reason = (
                "il writer permanente di {} e' gia' di {}, che e' libera: la issue va "
                "a quella sessione, nel suo albero.".format(group, owner)
                if owner
                else "writer permanente libero su {}.".format(group)
            )
            capacity.writers[group] += 1
        elif capacity.temporary_free():
            mode = "TEMPORARY_WORKTREE_SUGGESTED"
            reason = (
                "il writer permanente di {} e' occupato{} e la issue e' "
                "parallelizzabile: serve un worktree temporaneo. NON creato da qui."
            ).format(group, _occupato_da(capacity, group))
            capacity.temporary += 1
        else:
            deferred.append(
                Deferred(
                    key=key,
                    workspace=group,
                    reason="WRITER_CAPACITY"
                    if roadmap.temporary_worktree_capacity == 0
                    else "TEMPORARY_WORKTREE_CAPACITY",
                    detail="writer permanenti di {} occupati ({}/{}) e worktree "
                    "temporanei esauriti ({}/{}).".format(
                        group,
                        capacity.writers[group],
                        roadmap.writer_capacity(group),
                        capacity.temporary,
                        roadmap.temporary_worktree_capacity,
                    ),
                )
            )
            continue

        used = []
        if item.needs_unreal:
            capacity.unreal += 1
            used.append("UNREAL_EDITOR")

        capacity.wip_workspace[group] += 1
        capacity.wip_global += 1

        assignments.append(
            Assignment(
                key=key, workspace=group, mode=mode, resources=used, reason=reason,
                ownerSessionId=owner,
            )
        )

    return {
        "roadmapId": roadmap.id,
        "assignments": [a._asdict() for a in assignments],
        "deferred": [d._asdict() for d in deferred],
        # Dichiarato sempre, anche vuoto: un campo che compare solo quando c'e' un
        # problema costringe chi legge a distinguere «assente» da «nessuno».
        "overCommitted": capacity.over_committed(),
        # Dichiarato sempre: un piano calcolato SENZA snapshot runtime e' un piano che
        # non sa chi sta scrivendo, e chi lo legge deve poterlo distinguere.
        # 🔴 NIENTE timestamp qui dentro. La prima stesura esponeva `takenAt` della
        # snapshot, e il piano smetteva di essere una funzione del proprio input: due
        # chiamate a cavallo di un secondo davano risultati diversi. Si e' manifestato
        # come un test intermittente sul restart, ed era il determinismo - proprieta'
        # dichiarata di questo modulo - a essere rotto. Quando la snapshot e' stata
        # presa e' un dato del control plane, non del piano.
        "runtime": {
            "used": bool(runtime),
            "writers": [
                {"resourceKey": v["resourceKey"], "owner": v["ownerSessionId"],
                 "workspaceGroup": v.get("workspaceGroup"), "stale": v.get("stale")}
                for v in capacity.runtime_writers
            ],
            "unreal": [
                {"resourceKey": v["resourceKey"], "owner": v["ownerSessionId"],
                 "stale": v.get("stale")}
                for v in capacity.runtime_unreal
            ],
        },
        "readiness": {
            k: {
                "state": r.state,
                "progress": r.progress,
                "blockedBy": r.blocked_by,
                "unmet": r.unmet,
            }
            for k, r in ready.items()
        },
        "capacity": {
            "writers": {
                group: {
                    "used": capacity.writers[group],
                    "capacity": roadmap.writer_capacity(group),
                    "holder": capacity.permanent_holder.get(group),
                }
                for group in sorted(roadmap.resources["workspaces"])
            },
            "temporaryWorktrees": {
                "used": capacity.temporary,
                "capacity": roadmap.temporary_worktree_capacity,
            },
            "unrealEditor": {
                "used": capacity.unreal,
                "capacity": roadmap.unreal_lease_capacity,
            },
        },
        "wip": {
            "perWorkspace": dict(capacity.wip_workspace),
            "global": capacity.wip_global,
            "limits": dict(roadmap.wip),
        },
    }


def summary(roadmap, graph, states=None):
    """Conteggi per il quadro sintetico. Nessuna decisione, solo numeri."""
    ready = readiness(roadmap, graph, states)
    counts = collections.Counter(r.state for r in ready.values())
    return {
        "roadmapId": roadmap.id,
        "items": len(roadmap.items),
        "epics": len(roadmap.epics),
        "byState": {state: counts.get(state, 0) for state in READINESS_STATES},
    }
