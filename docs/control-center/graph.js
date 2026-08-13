// Project Control Center — le funzioni pure.
//
// REGOLA DEL MODULO (R-1 della spec): qui non si calcola stato. Niente `status`, niente gate,
// niente ✅/🟡/⏳ derivati. Quei valori arrivano gia' calcolati da `feature_registry.py`, che ne
// e' l'unica implementazione: una seconda regola in JavaScript divergerebbe in silenzio, e
// divergerebbe proprio sul dato che questa pagina esiste per mostrare.
//
// Cosa fa invece: deriva i LINK, indicizza le relazioni nei due versi, trova i riferimenti che
// non risolvono, applica i filtri. Tutto senza rete e senza DOM — cosi' i test lo eseguono.

// --- Link ------------------------------------------------------------------------------------

/** Base degli URL GitHub, da `meta.project.github`. Nessun URL assoluto vive altrove (R-5). */
export function githubBase(project) {
  const gh = (project && project.github) || {};
  if (!gh.owner || !gh.repository) return null;
  return {
    owner: gh.owner,
    repository: gh.repository,
    branch: gh.branch || 'main',
    root: `https://github.com/${gh.owner}/${gh.repository}`,
  };
}

export function issueUrl(project, number) {
  const base = githubBase(project);
  return base ? `${base.root}/issues/${number}` : null;
}

export function milestoneUrl(project, number) {
  const base = githubBase(project);
  return base ? `${base.root}/milestone/${number}` : null;
}

/** Un path repository-relative diventa un blob sul branch dichiarato. */
export function docUrl(project, path) {
  const base = githubBase(project);
  if (!base || !path) return null;
  const clean = String(path).replace(/^\.\//, '').replace(/^\/+/, '');
  return `${base.root}/blob/${base.branch}/${clean}`;
}

/**
 * I `wiki_refs` hanno due forme: un path del repository, oppure `wiki:<PageName>` che vive solo
 * nel clone della Wiki. La seconda non e' un file del repository e non va linkata come tale —
 * confonderebbe un riferimento valido con uno rotto.
 */
export function wikiRefUrl(project, ref) {
  const base = githubBase(project);
  if (!base || !ref) return null;
  if (ref.startsWith('wiki:')) return `${base.root}/wiki/${ref.slice(5)}`;
  return docUrl(project, ref);
}

// --- Chiavi di roadmap -----------------------------------------------------------------------

/**
 * La chiave prefissata di un checkpoint di release. I `checkpoints` del registry sono numeri
 * nello spazio delle EPIC — e non sempre dell'epic dichiarata: `RT-FEAT-CORE-TURN` sta in E2 e
 * cita anche `4.1`, che e' di E4. Il prefisso si costruisce dal numero, mai dal contenitore.
 */
export function epicCheckpointKey(cp) {
  return /^\d+\.\d+$/.test(String(cp)) ? `E${cp}` : String(cp);
}

/** Il contenitore di roadmap di una feature, gia' prefissato (`E12`, `M9`) o null. */
export function roadmapContainer(feature) {
  const r = feature.roadmap || {};
  return r.epic || r.milestone || null;
}

// --- Indice e relazioni inverse --------------------------------------------------------------

/**
 * Indicizza i due artefatti e costruisce i versi che i dati non dichiarano.
 *
 * Le relazioni dirette esistono nel dato (una feature dichiara le sue issue); le inverse no —
 * nessuno scrive «questo scenario e' citato da queste tre feature». Costruirle qui e' l'unica
 * cosa che la pagina aggiunge, ed e' derivazione di struttura, non di stato.
 */
export function buildIndex(registry, graph) {
  const features = (registry && registry.features) || [];
  const sessions = (graph && graph.editor_sessions) || [];
  const checkpoints = (graph && graph.checkpoints) || {};

  const featureById = new Map(features.map((f) => [f.feature_id, f]));
  const sessionById = new Map(sessions.map((s) => [s.id, s]));
  const scenarioById = new Map(((graph && graph.scenarios) || []).map((s) => [s.id, s]));

  const featuresByScenario = new Map();
  const featuresByContainer = new Map();
  const featuresByCheckpoint = new Map();
  const dependents = new Map();
  const completes = new Map();
  const featuresByIssue = new Map();

  const push = (map, key, value) => {
    if (!key) return;
    if (!map.has(key)) map.set(key, []);
    map.get(key).push(value);
  };

  for (const f of features) {
    for (const sid of f.scenarios || []) push(featuresByScenario, sid, f.feature_id);
    push(featuresByContainer, roadmapContainer(f), f.feature_id);
    for (const cp of (f.roadmap && f.roadmap.checkpoints) || []) {
      push(featuresByCheckpoint, epicCheckpointKey(cp), f.feature_id);
    }
    for (const dep of f.dependencies || []) push(dependents, dep, f.feature_id);
    for (const other of f.completed_by || []) push(completes, other, f.feature_id);
    for (const issue of f.issues || []) push(featuresByIssue, issue, f.feature_id);
  }

  // Seduta → feature: non esiste un campo che le colleghi. Il ponte e' il checkpoint che la
  // seduta sblocca o attende, e che una feature dichiara. E' un'inferenza, e va detta come tale
  // nella UI: «feature che passano per gli stessi checkpoint», non «feature di questa seduta».
  const featuresBySession = new Map();
  for (const s of sessions) {
    const refs = [...(s.unblocks || []), ...((s.unblocked_by || []).map((p) => p.ref))];
    const found = new Set();
    for (const ref of refs) {
      for (const fid of featuresByCheckpoint.get(ref) || []) found.add(fid);
      for (const fid of featuresByContainer.get(ref) || []) found.add(fid);
    }
    featuresBySession.set(s.id, [...found]);
  }

  const sessionsByCheckpoint = new Map();
  for (const s of sessions) {
    for (const ref of s.unblocks || []) push(sessionsByCheckpoint, ref, s.id);
  }

  return {
    featureById, sessionById, scenarioById, checkpoints,
    featuresByScenario, featuresByContainer, featuresByCheckpoint, featuresByIssue,
    dependents, completes, featuresBySession, sessionsByCheckpoint,
  };
}

// --- Riferimenti che non risolvono -------------------------------------------------------------

/**
 * Ogni riferimento che punta a qualcosa che non esiste, con la sua origine.
 *
 * Non duplica il validator Python: quello vede il filesystem (test, scenari, file Wiki) e ha
 * l'ultima parola. Qui si controlla cio' che e' verificabile **dentro i due artefatti**, che e'
 * esattamente cio' che la pagina disegna — un link che la pagina mostrerebbe rotto.
 */
export function brokenRefs(registry, graph, index) {
  const out = [];
  const has = (id) => index.featureById.has(id);
  for (const f of (registry.features || [])) {
    for (const dep of f.dependencies || []) {
      if (!has(dep)) out.push({ from: f.feature_id, kind: 'dependency', ref: dep });
    }
    for (const other of f.completed_by || []) {
      if (!has(other)) out.push({ from: f.feature_id, kind: 'completed_by', ref: other });
    }
    for (const sid of f.scenarios || []) {
      if (!index.scenarioById.has(sid)) {
        out.push({ from: f.feature_id, kind: 'scenario', ref: sid });
      }
    }
    const container = roadmapContainer(f);
    if (container && !(container in (graph.epics || {})) && !(container in (graph.milestones || {}))) {
      out.push({ from: f.feature_id, kind: 'roadmap', ref: container });
    }
    for (const cp of (f.roadmap && f.roadmap.checkpoints) || []) {
      const key = epicCheckpointKey(cp);
      if (!(key in (graph.checkpoints || {}))) {
        out.push({ from: f.feature_id, kind: 'checkpoint', ref: key });
      }
    }
  }
  for (const s of graph.editor_sessions || []) {
    for (const p of s.unblocked_by || []) {
      if (!p.kind) out.push({ from: s.id, kind: 'prerequisito', ref: p.ref });
    }
  }
  return out;
}

/**
 * Cicli fra dipendenze di feature. Un ciclo non e' sempre un difetto — ma nessuno lo vede
 * leggendo, e una pagina che navigasse le dipendenze ci girerebbe dentro.
 */
export function dependencyCycles(registry) {
  const graph = new Map((registry.features || []).map((f) => [f.feature_id, f.dependencies || []]));
  const state = new Map();
  const cycles = [];
  const walk = (node, trail) => {
    if (state.get(node) === 'done') return;
    if (state.get(node) === 'open') {
      const start = trail.indexOf(node);
      if (start >= 0) cycles.push([...trail.slice(start), node]);
      return;
    }
    state.set(node, 'open');
    for (const next of graph.get(node) || []) {
      if (graph.has(next)) walk(next, [...trail, node]);
    }
    state.set(node, 'done');
  };
  for (const id of graph.keys()) walk(id, []);
  return cycles;
}

// --- Filtri ------------------------------------------------------------------------------------

/** I campi su cui la ricerca testuale morde. Nessuno di questi e' derivato. */
function haystack(feature) {
  return [
    feature.feature_id, feature.title, feature.area, feature.kind, feature.status,
    ...(feature.issues || []).map((i) => `#${i}`),
    ...(feature.scenarios || []), ...(feature.scenarios_planned || []),
    ...(feature.owner_specs || []), ...(feature.wiki_refs || []),
    ...(feature.tests || []),
  ].filter(Boolean).join(' ').toLowerCase();
}

/**
 * Filtra le feature. Ogni criterio corrisponde a una chiave che esiste nel dato (R-10): se un
 * filtro avesse bisogno di un campo nuovo, il campo va aggiunto alla sorgente, non qui.
 */
export function filterFeatures(features, filters = {}, index = null) {
  const f = filters;
  const text = (f.text || '').trim().toLowerCase();
  return features.filter((feature) => {
    if (f.release && feature.release !== f.release) return false;
    if (f.area && feature.area !== f.area) return false;
    if (f.kind && feature.kind !== f.kind) return false;
    if (f.priority && feature.priority !== f.priority) return false;
    if (f.status && feature.status !== f.status) return false;
    if (f.container && roadmapContainer(feature) !== f.container) return false;
    if (f.gate && feature.gates[f.gate] === 'done') return false;   // «gate mancante»
    if (f.blocked === 'yes' && feature.status !== 'BLOCKED') return false;
    if (f.blocked === 'no' && feature.status === 'BLOCKED') return false;
    if (f.editor === 'yes') {
      const sessions = sessionsForFeature(feature, index);
      if (!sessions.length) return false;
    }
    if (f.scenarioReady === 'yes' && !(feature.scenarios || []).length) return false;
    if (f.scenarioReady === 'planned' && !(feature.scenarios_planned || []).length) return false;
    if (text && !haystack(feature).includes(text)) return false;
    return true;
  });
}

/** Le sedute che toccano i checkpoint di questa feature. Vedi la nota in `buildIndex`. */
export function sessionsForFeature(feature, index) {
  if (!index) return [];
  const out = new Set();
  for (const [sid, fids] of index.featuresBySession || []) {
    if (fids.includes(feature.feature_id)) out.add(sid);
  }
  return [...out];
}

// --- Staleness ---------------------------------------------------------------------------------

/**
 * Il dato mostrato e' piu' vecchio della sorgente?
 *
 * Il repository non ha CI: `generate` lo esegue una persona, e un `.yaml` modificato senza
 * rigenerare produce una pagina plausibile e vecchia — il difetto peggiore, perche' non si vede.
 * La pagina non puo' impedirlo; puo' renderlo visibile, che e' cio' che le compete.
 */
export function stalenessVerdict(yamlCommit, jsonCommit) {
  if (!yamlCommit || !jsonCommit) return { known: false };
  const yamlDate = new Date(yamlCommit.date);
  const jsonDate = new Date(jsonCommit.date);
  return {
    known: true,
    stale: yamlDate > jsonDate,
    yamlDate, jsonDate,
    yamlSha: yamlCommit.sha, jsonSha: jsonCommit.sha,
  };
}

// --- Execution graph ---------------------------------------------------------------------------
//
// Topologia, non stato. `readiness` e `state` arrivano gia' derivati dal generatore (R-1): qui si
// indicizza, si filtra e si calcola il LAYOUT, che e' struttura visiva e non un giudizio.

/** Indice dell'execution graph: nodi per id, archi nei due versi, hard e soft separati. */
export function executionIndex(graph) {
  const execution = (graph && graph.execution) || null;
  const empty = { nodes: [], byId: new Map(), incoming: new Map(), outgoing: new Map(), capabilities: new Map() };
  if (!execution) return empty;
  const byId = new Map(execution.nodes.map((n) => [n.id, n]));
  const incoming = new Map();
  const outgoing = new Map();
  for (const edge of execution.edges || []) {
    if (!outgoing.has(edge.from)) outgoing.set(edge.from, []);
    if (!incoming.has(edge.to)) incoming.set(edge.to, []);
    outgoing.get(edge.from).push(edge);
    incoming.get(edge.to).push(edge);
  }
  return {
    nodes: execution.nodes,
    byId,
    incoming,
    outgoing,
    capabilities: new Map((execution.capabilities || []).map((c) => [c.id, c])),
  };
}

export function executionIncoming(index, id, { hardOnly = false } = {}) {
  return (index.incoming.get(id) || []).filter((e) => !hardOnly || e.hard);
}

export function executionOutgoing(index, id, { hardOnly = false } = {}) {
  return (index.outgoing.get(id) || []).filter((e) => !hardOnly || e.hard);
}

/**
 * I due lati liberi di un nodo — quelli che la vista disegna come collegamenti STACCATI.
 *
 * Un nodo senza archi hard entranti non e' un nodo «senza freccia»: e' una RADICE, e nulla lo
 * trattiene. Un nodo senza archi hard uscenti e' una FOGLIA: finirlo non sblocca niente di
 * dichiarato. Disegnare il nulla renderebbe le due cose indistinguibili dal margine della
 * figura, e il moncone e' il modo di dire «qui la catena finisce davvero» invece di «qui la
 * figura e' tagliata».
 */
export function danglingEnds(index, id) {
  return {
    inbound: executionIncoming(index, id, { hardOnly: true }).length === 0,
    outbound: executionOutgoing(index, id, { hardOnly: true }).length === 0,
  };
}

/**
 * Profondita' topologica sul solo sottografo hard: la X del layout.
 *
 * `follows` e `related` non partecipano — sono ordine e navigazione, e farli contare
 * spingerebbe a destra nodi che nulla blocca, cioe' direbbe con la posizione l'opposto di
 * quello che la readiness dice a parole. Un ciclo (che il validator gia' rifiuta) non manda in
 * loop: i nodi visitati non si rivisitano.
 */
export function topologicalDepths(index) {
  const depth = new Map();
  const visiting = new Set();
  const compute = (id) => {
    if (depth.has(id)) return depth.get(id);
    if (visiting.has(id)) return 0;
    visiting.add(id);
    const parents = executionIncoming(index, id, { hardOnly: true })
      .filter((e) => index.byId.has(e.from));
    const value = parents.length === 0 ? 0 : Math.max(...parents.map((e) => compute(e.from) + 1));
    visiting.delete(id);
    depth.set(id, value);
    return value;
  };
  for (const node of index.nodes) compute(node.id);
  return depth;
}

export function filterExecution(nodes, filters = {}) {
  return nodes.filter((n) => {
    if (filters.release && n.release !== filters.release) return false;
    if (filters.lane && n.execution_lane !== filters.lane) return false;
    if (filters.domain && n.domain_group !== filters.domain) return false;
    if (filters.readiness && n.readiness !== filters.readiness) return false;
    if (filters.q) {
      const hay = `${n.id} ${n.ref} ${(n.feature_ids || []).join(' ')} ${n.checkpoint || ''}`;
      if (!hay.toLowerCase().includes(filters.q.toLowerCase())) return false;
    }
    return true;
  });
}

/** Il vicinato di un nodo entro `depth` salti, nei due versi. Per il focus, non per il globale. */
export function executionNeighborhood(index, id, depth = 1) {
  const seen = new Set([id]);
  let frontier = [id];
  for (let step = 0; step < depth; step += 1) {
    const next = [];
    for (const current of frontier) {
      for (const edge of executionIncoming(index, current)) {
        if (index.byId.has(edge.from) && !seen.has(edge.from)) { seen.add(edge.from); next.push(edge.from); }
      }
      for (const edge of executionOutgoing(index, current)) {
        if (index.byId.has(edge.to) && !seen.has(edge.to)) { seen.add(edge.to); next.push(edge.to); }
      }
    }
    frontier = next;
  }
  return seen;
}
