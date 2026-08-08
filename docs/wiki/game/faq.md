# FAQ — Domande rapide

> **Tipo:** guida giocatore, non normativa

## È un gioco a turni?

Sì, ma i turni sono **simultanei**. Tutte le unità pianificano nello stesso round.

## Posso muovermi e poi sparare?

Non con il normale `Move`. La sequenza standard mette il **Blast prima del Move**. Alcune mobilità speciali, come Dash o Reposition, possono invece avvenire prima del Blast.

## Posso attaccare e andare in Overwatch nello stesso round?

Normalmente no. La regola è `Attack OR Ability OR Overwatch`, salvo eccezioni dichiarate dal kit.

## Sprint e Dash sono la stessa cosa?

No. Sprint è un **profilo del Move**; Dash è una mobilità speciale pre-Blast.

## Vedo cosa sta pianificando il nemico?

No. Il planning avversario non deve essere consegnato alla tua squadra prima della Resolution.

## Le animazioni decidono se un colpo arriva?

No. La simulazione logica decide l'esito; animazioni e VFX lo rappresentano.

## Se due unità vogliono la stessa cella?

Il resolver applica regole di collisione simultanee. Non vince semplicemente «chi è stato processato per primo».

## Una porta o un ponte possono cambiare un percorso già pianificato?

Sì. La topologia è parte dello stato di gioco. Se un collegamento non è più valido quando il Move risolve, il percorso può essere troncato o fallire secondo le regole.

## L'acqua serve solo a rallentare?

No. Può applicare `Wet` ed è conduttiva, quindi interagisce con l'elettricità.

## High Ground dà più range visivo?

Non automaticamente nella v0.1. La quota ha già valore attraverso geometria, LOS, coperture e layer.

## Le reazioni sono sempre automatiche?

No. Alcune Prepared Reaction hanno una sola risposta e risolvono automaticamente; sistemi come Overwatch possono offrire una scelta live a un decision boundary.

## Fonti normative

- `docs/gameplay/spec-sequenza-turno.md`
- `docs/gameplay/brief-azioni-generiche-overwatch.md`
- `docs/gameplay/spec-terreni-e8.md`
