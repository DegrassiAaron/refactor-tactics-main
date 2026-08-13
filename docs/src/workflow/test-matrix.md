# Project Graph v1 — Test Matrix

## Python / source

| Test | Atteso |
|---|---|
| schema_version valida | pass |
| schema_version sconosciuta | error |
| lane sconosciuta | error |
| domain sconosciuto | error |
| node duplicato | error |
| endpoint relazione inesistente | error |
| self requires | error |
| A requires B / B requires A | error |
| ciclo solo follows | non hard-error |
| related non influenza readiness | pass |
| follows non influenza readiness | pass |
| PIE prerequisite non finita | WAITING_FOR_PIE |
| ASSET prerequisite non finita | WAITING_FOR_ASSET |
| capability provider soddisfatto | available |
| capability provider non soddisfatto | unavailable |
| session senza execution_lane | default PIE |
| session execution_lane=asset | ASSET |
| due generate consecutive | no diff |
| mutate project-graph.json + --check | fail |

## JS pure functions

| Test | Atteso |
|---|---|
| index incoming/outgoing | simmetrico |
| filter release | esatto |
| filter lane | esatto |
| filter domain | esatto |
| filter readiness | esatto |
| neighborhood depth 1 | solo adiacenti |
| neighborhood depth 2 | due salti |
| topological depth hard DAG | stabile |
| soft edge ignorata per depth hard | pass |
| broken execution ref | visibile |
| capability missing | visibile |

## Contract real data

| Assertion | Atteso |
|---|---|
| GRAPH.execution | presente |
| lane code/pie/asset | presenti |
| issue:165 | presente |
| issue:170 | presente |
| session:U7/U8/U9 | presenti |
| U7 lane | asset |
| U8 lane | asset |
| U9 lane | pie |
| edge 165→512 | hard requires |
| edge 166→314 | follows soft |
| edge 593→U7 | related soft |
| #170 incoming hard | almeno #512/#66/#75 |
| nessun JS deriva readiness | grep/inspection |

## Manual UI

1. avvia `python -m http.server 8000`;
2. apri `/docs/control-center/`;
3. Execution Map;
4. seleziona v0.1;
5. clicca #165;
6. clicca #170;
7. clicca U7;
8. verifica edge type e drawer;
9. disattiva `related`;
10. verifica che #593 sparisca dal focus U7 senza cambiare readiness;
11. filtra lane ASSET;
12. devono restare U7/U8 nel thin slice.
