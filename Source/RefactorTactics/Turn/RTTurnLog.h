#pragma once

#include "CoreMinimal.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnRules.h"
#include "RTTurnLog.generated.h"

/**
 * Categoria dell'esito registrato nel TurnLog.
 *
 * `Fallback` e' un valore AGGIUNTO in coda (CP 4.3), `Reaction` un altro (CP 5.1): la categoria viaggia come
 * uint8 nel formato serializzato, quindi i log scritti prima restano leggibili e la versione del formato non
 * cambia. Aggiungerli in mezzo avrebbe invece rinumerato `Combat`, cioe' riscritto il significato dei file
 * gia' su disco.
 */
UENUM(BlueprintType)
enum class ERTLogCategory : uint8 { Move, Combat, Fallback, Reaction, Environment, Facing, Predictive, ReactionDecision, ReactionClash };

/**
 * Gli eventi di un boundary **contested** (E14.7 §10, [D-048]). Viaggiano in `FRTTurnLogEntry::Outcome`
 * delle voci di categoria `ReactionClash`.
 *
 * ⚠️ Categoria PROPRIA e non `ReactionDecision`, per la stessa ragione con cui quella si separo' da
 * `Reaction`: `Outcome` e' un `uint8` il cui significato lo decide la categoria, e due enum diversi sotto la
 * stessa categoria renderebbero il campo illeggibile senza sapere quale dei due intendeva chi ha scritto.
 * Aggiunta in **coda**, come `Fallback`, `Reaction` e `ReactionDecision` prima: la categoria viaggia come
 * `uint8`, quindi i file gia' su disco non cambiano significato e **la versione del formato non cambia**.
 *
 * 🔴 **Tutte queste voci si scrivono AL REVEAL, mai al lock**, ed e' la meta' di §7.1 che riguarda il log:
 * *«in rete, nessun evento di scelta viene pubblicato prima del reveal»*. Se `ChoiceLocked` fosse scritto
 * quando il lock arriva, l'**ordine delle voci** direbbe chi ha deciso per primo — cioe' esattamente la
 * latenza di decisione che la scadenza fissa esiste per nascondere. Le due voci `ChoiceLocked` sono scritte
 * insieme alle altre, in ordine **canonico** (§7.3) e non di arrivo.
 */
UENUM(BlueprintType)
enum class ERTClashLogEvent : uint8
{
	/** Il boundary si e' aperto. `OpportunityId` lo identifica; `Amount` porta la cardinalita'. */
	OpportunityCreated,
	/** Un partecipante ha bloccato la propria scelta. Una voce per partecipante, in ordine canonico. */
	ChoiceLocked,
	/** La scadenza e' arrivata e le scelte sono state svelate insieme. */
	Revealed,
	/** Il confronto fra le due intenzioni. `UnitId` e `SelectedTargetUnitId` sono i due contendenti. */
	Compared,
	/** L'esito di un partecipante: `Amount` porta `ERTClashOutcome` (Win/Tie/Lose). */
	OutcomeResolved,
	/** Una maneuver con costo ha consumato la propria risorsa al lock valido (§9). */
	CostConsumed
};

/**
 * Come si e' chiusa una finestra di reazione (CP 14.5): la risposta **e** il perche', in un valore solo.
 * Viaggia in `FRTTurnLogEntry::Outcome` delle voci di categoria `ReactionDecision`.
 *
 * ⚠️ Categoria PROPRIA e non `Reaction`, benche' parlino della stessa meccanica. `Reaction` ha gia' il suo
 * enum di esito — `ERTReactionOutcome` (`Activated`/`NotTriggered`/`Unavailable`) — e `Outcome` e' un `uint8`
 * il cui significato lo decide la categoria: due enum diversi sotto la stessa categoria renderebbero il campo
 * illeggibile senza sapere quale dei due chi ha scritto la voce intendeva. Aggiunta in CODA, come `Fallback` e
 * `Reaction` prima: i file gia' scritti non cambiano significato.
 *
 * ⚠️ **Un enum solo e non due campi**, ed e' una correzione fatta in corsa: la prima stesura teneva la
 * risposta in `Outcome` e il motivo in `Amount`, sul modello di `ERTDisplacementBlockReason`. Non regge, e la
 * ragione e' che li' le celle percorse sono **zero per definizione** — `Amount` e' libero — mentre qui un
 * `Fire` ha un danno da riportare. Due assi per un campo solo avrebbero costretto a scegliere quale dei due
 * perdere: il motivo (e `HOLD` scelto sarebbe indistinguibile da `HOLD` scaduto, cioe' proprio la distinzione
 * per cui il campo esiste) o il danno (e il colpo non lascerebbe traccia nel TurnLog canonico, che e' il
 * difetto di `#625`). Incrociandoli qui, `Amount` resta la quantita' che dichiara di essere.
 *
 * Sono **sette** valori dal 2026-08-19, e la ragione del settimo e' scritta accanto ad esso.
 *
 * ⚠️ **Questa riga diceva «sono SEI valori e non sette»**, e l'argomento che portava — *un `FireImmediate`
 * non puo' esistere, perche' `HOLD` e' sempre fra le risposte legali e quindi cardinalita' <= 1 significa
 * «solo HOLD»* — **regge ancora ed e' su un'altra cosa**: parla di cio' che il caso degenere puo' produrre,
 * non del numero totale. Il conteggio invece era una misura, ed e' scaduto quando [D-047] ha aperto finestre
 * il cui vocabolario non e' `FIRE`/`HOLD`. Corretto il numero, conservato l'argomento: un `FireImmediate`
 * resta impossibile per costruzione.
 */
UENUM(BlueprintType)
enum class ERTReactionDecisionOutcome : uint8
{
	/** Ha scelto di sparare entro la finestra. `SelectedTargetUnitId` dice su chi; `Amount` quanti danni. */
	FireChosen,
	/** Ha scelto di NON sparare. La reaction resta armata e la charge non si spende. */
	HoldChosen,
	/** La finestra e' scaduta: `Timeout -> HOLD` (ADR-0004 §3). Mai `FIRE`, mai la charge. */
	HoldTimeout,
	/** Nessun decisore collegato per quel responder: stesso default, e si dichiara che e' un'altra cosa. */
	HoldNoDecider,
	/** La risposta arrivata non era fra le `AllowedResponses`: rifiutata, e sostituita dal default. */
	HoldRejected,
	/** Cardinalita' <= 1: non c'era niente da scegliere e nessuna finestra si e' aperta. */
	HoldImmediate,
	/**
	 * Ha scelto una risposta **attiva che non e' `FIRE`**: il token sta in `FRTTurnLogEntry::ReactionResponse`
	 * (E14.7, [D-047]). E' `SIDESTEP` oggi, e cio' che i profili aggiungeranno domani.
	 *
	 * 🔴 **Perche' non si riusa `HoldChosen`.** Sarebbe stato possibile — il token disambigua comunque in
	 * rilettura — e avrebbe reso `LogEventCount` incapace di distinguere «ha tenuto la cella» da «ha
	 * scartato». Uno scenario che conta gli esiti verificherebbe due comportamenti opposti sotto lo stesso
	 * numero, ed e' esattamente il difetto per cui `ERTReactionOutcome::NotTriggered` esiste dal CP 5.1:
	 * un esito che non si puo' contare separatamente e' un esito che non si puo' asserire.
	 *
	 * ⚠️ **In CODA all'enum, mai in mezzo**: i valori viaggiano come `uint8` nel formato serializzato, e
	 * inserirlo prima di `HoldImmediate` rinumererebbe le tracce gia' scritte — che e' la regola dichiarata in
	 * testa a questo file e la ragione per cui `Fallback`, `Reaction` e `ReactionDecision` sono in fondo a
	 * `ERTLogCategory`.
	 */
	ResponseChosen
};

/**
 * Esito di una **Predictive Action** al suo boundary di risoluzione (E18 CP 18.1, [D-016]).
 *
 * Sono due e non tre: una previsione o coglie o va a vuoto. Il terzo caso — «in attesa che il giocatore
 * decida» — appartiene alle finestre di reazione di E14, ed e' precisamente cio' che una Predictive Action
 * NON fa: la decisione e' completa in Planning, e la Resolution non chiede piu' niente a nessuno.
 *
 * La cella BLOCCATA viaggia in `FRTTurnLogEntry::TgtCell`, e in `Amount` il danno inflitto (0 sul whiff).
 * Cosi' una traccia dice sempre *dove* si e' scommesso, anche quando non e' successo niente — che e' il caso
 * che serve leggere per capire il turno.
 */
UENUM(BlueprintType)
enum class ERTPredictiveOutcome : uint8
{
	/** Un'unita' ostile e' entrata nella cella bloccata: il colpo risolve e le tronca il movimento. */
	TriggerMatched,
	/**
	 * Nessuno e' entrato: la previsione era sbagliata e il colpo va a vuoto, applicando il fallback
	 * DICHIARATO nel catalogo (`Cancel` ≡ fizzle per la thin slice).
	 *
	 * Il whiff e' registrato invece di essere taciuto perche' e' il `Misplay / Failure State` di D-032: il
	 * costo di aver letto male il turno dev'essere leggibile, o la scommessa non si vede.
	 */
	PredictionWhiffed
};

/**
 * Orientamento (CP 16.1): perche' il facing di un'unita' e' CAMBIATO, oppure quale consumatore l'ha LETTO.
 *
 * Le due cose stanno nello stesso enum perche' rispondono alla stessa domanda del replay: *quale valore
 * valeva quando*. [D-020](../../../docs/decisions/RT_PDR_00_Decision_Log.md) stabilisce che il facing cambia
 * piu' volte per round su una timeline nominata e che ogni consumatore legge il valore autorevole **piu'
 * recente**: un solo campo per turno non basterebbe a ricostruire il round, perche' non direbbe se il Blast
 * ha sparato prima o dopo la rotazione dello scatto.
 *
 * La DIREZIONE viaggia in `FRTTurnLogEntry::Amount` come valore di `ERTHexDirection` (0..5). E' un intero,
 * quindi non rompe l'invariante #4 ne' il formato serializzato: `Amount` esiste gia' e per le altre categorie
 * porta danni o celle percorse.
 */
UENUM(BlueprintType)
enum class ERTFacingOutcome : uint8
{
	/** Derivato dall'ultimo passo del movimento: e' il `FacingFinalAfterMove` di D-020. */
	DerivedFromMove,
	/** Derivato dallo scatto (`FacingAfterDash`), che risolve prima del Blast. */
	DerivedFromDash,
	/** Rotazione dichiarata in planning e accettata. */
	DeclaredInPlanning,
	/** Rotazione dichiarata FUORI dall'insieme legale: rifiutata, il facing resta quello di prima. */
	DeclarationRejected,
	/** Un'azione con bersaglio ha orientato l'unita' PRIMA di risolvere (`FacingAfterPrepActionTargeting`). */
	TargetingReoriented,
	/** Spostamento forzato subito: girata verso la sorgente. */
	TurnedToDisplacementSource,
	/** Spostamento ambientale (ghiaccio, corrente): nessuna sorgente, orientamento invariato. */
	KeptOnEnvironmentalDisplacement,
	/** LETTURA: il combattimento della fase Blast ha usato questo valore. */
	UsedByBlast,
	/** LETTURA: l'Overwatch ha usato questo valore (il cono pianificato, E14). */
	UsedByOverwatch,
	/**
	 * Il colpo e' arrivato FUORI dall'arco frontale e la protezione da copertura/`Guard` non ha retto
	 * (CP 16.2). La direzione registrata e' il facing del BERSAGLIO, cioe' il lato che stava guardando
	 * mentre veniva colpito dall'altra parte.
	 *
	 * Ha un valore proprio invece di riusare `UsedByBlast` perche' il giocatore deve poter leggere *perche'*
	 * la copertura non l'ha protetto: «il colpo usa l'orientamento SE» non risponde a quella domanda.
	 */
	RearHitBypassedCover
};

/**
 * Come una CELLA e' cambiata durante il turno (CP 8.4). La mappa e' un sistema di gioco: se una cella prende
 * fuoco o si allaga, il replay deve poterlo dire — altrimenti una unita' che a T+1 incassa danno «senza
 * motivo» resta inspiegabile, e il TurnLog non e' piu' una traccia autoritativa.
 *
 * `Amount` della voce porta i turni di durata; `TgtCell` la cella modificata; `ActionId` chi l'ha causata.
 */
UENUM(BlueprintType)
enum class ERTEnvironmentOutcome : uint8
{
	/** La superficie della cella e' cambiata (fuoco acceso, acqua creata). */
	SurfaceChanged,
	/** Una modifica temporanea e' scaduta: la cella e' tornata alla superficie originale. */
	SurfaceRestored,
	/** La modifica non e' avvenuta: la superficie di destinazione non l'ammette (acqua e metallo non bruciano). */
	SurfaceRejected,
	/** Una superficie ne ha rimossa un'altra: l'acqua che arriva sul fuoco lo spegne. */
	SurfaceExtinguished,
	/**
	 * Una copertura ha incassato danno ed e' ancora in piedi (CP 9.2). `SrcCell` e `TgtCell` sono le due
	 * celle del BORDO — la coppia lo identifica senza aggiungere un campo direzione al log — e `Amount`
	 * l'integrita' RESIDUA, cosi' chi legge il replay sa quanto manca al crollo.
	 */
	CoverDamaged,
	/**
	 * Una copertura e' stata abbattuta: da qui in poi quel bordo si attraversa. E' l'evento che spiega perche'
	 * al turno successivo una linea di tiro esiste dove prima non c'era.
	 */
	CoverDestroyed,
	/**
	 * Una porta si e' chiusa (CP 9.3): da qui in poi quel bordo non si attraversa e non si vede attraverso.
	 * `SrcCell`/`TgtCell` sono le due celle del BORDO, come per le coperture — nessun campo nuovo nel log.
	 */
	DoorClosed,
	/** Una porta si e' aperta: spiega perche' esiste un passaggio dove prima non c'era. */
	DoorOpened,
	/**
	 * Un ponte e' comparso (CP 9.4): `SrcCell`/`TgtCell` sono le due celle collegate e `Amount` i turni che
	 * durera' (0 = permanente).
	 */
	BridgeCreated,
	/** Un ponte e' stato tolto o e' scaduto: da qui i due layer sono di nuovo separati. */
	BridgeRemoved,
	/** Un ponte ha incassato danno ed e' ancora in piedi; `Amount` e' l'integrita' RESIDUA. */
	BridgeDamaged,
	/** Un ponte e' crollato: e' l'evento che spiega perche' un percorso fra due layer non esiste piu'. */
	BridgeDestroyed,
	/**
	 * Una copertura e' stata ERETTA in partita (CP 9.5): `SrcCell`/`TgtCell` sono le due celle del bordo, come
	 * per ogni altro evento di struttura, e `Amount` i turni che durera' (0 = permanente).
	 *
	 * Aggiunto IN CODA: l'esito viaggia come `uint8` nel TurnLog serializzato, quindi inserirlo in mezzo
	 * rinumererebbe gli eventi gia' scritti e i replay del corpus golden racconterebbero un'altra partita.
	 */
	CoverCreated,
	/** Una copertura temporanea e' scaduta: da qui in poi quel bordo non ripara piu' nessuno. */
	CoverExpired,
	/** Una copertura e' stata SPOSTATA su un altro bordo (`Riktor.Reconfigure`): non ne nasce una seconda. */
	CoverMoved,
	/**
	 * La copertura non e' nata: bersaglio fuori portata, bordo non dichiarato o gia' riparato. E' il `Cancel`
	 * che il catalogo dichiara, reso VISIBILE — un'azione che sparisce in silenzio e' indistinguibile da un bug.
	 */
	CoverRejected
};

/**
 * Quale fallback e' stato applicato a un'azione che non era piu' eseguibile. Speculare a `ERTActionFallback`
 * (catalogo §7), ma e' un tipo separato: questo e' cio' che il turno ha REGISTRATO, quello e' cio' che
 * l'azione DICHIARA — e i due possono differire (`BasicAttack` degrada a `Cancel` in v0.1).
 */
UENUM(BlueprintType)
enum class ERTFallbackOutcome : uint8
{
	Stopped,      // Fallback.Stop: fermata all'ultima posizione valida
	Waited,       // Fallback.Wait: azione sostituita con l'attesa
	AttackedCell, // Fallback.AttackCell: persa l'unita' bersaglio, colpita la cella pianificata
	Cancelled     // Fallback.Cancel: nessun effetto (ci finisce anche BasicAttack, non praticabile in v0.1)
};

/**
 * Esito del movimento di un'unita' nel turno (dal resolver ResolvePaths).
 *
 * `BlockedByPriority` e `BlockedByImpact` sono valori AGGIUNTI in coda (CP 4.8, collisioni con priorita'):
 * viaggiano come uint8 nel formato serializzato, quindi i log gia' scritti restano leggibili.
 */
UENUM(BlueprintType)
enum class ERTMoveOutcome : uint8
{
	Stayed,            // non pianificava movimento (path < 2 celle)
	Moved,             // raggiunta la destinazione pianificata (scambio incluso)
	BlockedContested,  // fermata (o parziale) per destinazione contesa a PARITA' di priorita'
	BlockedByUnit,     // fermata (o parziale) per cella occupata da un'unita' ferma
	BlockedByPriority, // fermata per destinazione contesa persa contro una mobilita' con priorita' migliore
	BlockedByImpact,   // fermata per scontro frontale con un'altra mobilita' lineare in arrivo opposto
	/**
	 * Fermata perche' la TOPOLOGIA e' cambiata dopo che il percorso era stato pianificato: una porta chiusa,
	 * un muro alzato, una cella sparita (CP 9.3). Aggiunto in CODA: le tracce gia' scritte non cambiano
	 * significato.
	 *
	 * Lo scrive il chiamante e non `ResolveHexPaths`: il troncamento avviene PRIMA che il resolver veda il
	 * percorso, quindi lui classificherebbe `Moved` — vero sul percorso troncato, falso su cio' che l'unita'
	 * aveva pianificato.
	 */
	BlockedByTopology,
	/**
	 * Fermata da una **Predictive Action** andata a segno (E18 CP 18.1): l'unita' e' ENTRATA nella cella
	 * bloccata e li' si e' fermata. Aggiunto in CODA, come `BlockedByTopology` prima: le tracce gia' scritte
	 * non cambiano significato.
	 *
	 * Ha un valore proprio e non riusa `BlockedByUnit` perche' quello dice «c'era qualcuno», che qui e' falso:
	 * la cella e' libera, e cio' che ha fermato l'unita' e' un colpo deciso un turno prima. Senza questa
	 * distinzione il replay mostrerebbe un arresto senza causa — il difetto che #307 descrive per gli archi.
	 */
	StoppedByPrediction,
	/**
	 * Spostamento SUBITO, non scelto: una spinta (`Push`) o una trazione (`Pull`) l'ha portata altrove
	 * (#307). Aggiunto in CODA, come `BlockedByTopology` e `StoppedByPrediction` prima: l'esito viaggia come
	 * `uint8` nel formato serializzato, quindi i log gia' scritti non cambiano significato.
	 *
	 * Ha un valore proprio e non riusa `Moved` perche' quello dice «e' andata dove voleva», che qui e' falso:
	 * l'unita' non aveva pianificato nulla di tutto cio'. Distinguerli e' il punto dell'issue — senza,
	 * un'unita' che si ritrova due celle piu' in la' e' indistinguibile da un difetto del resolver.
	 *
	 * **Perche' non serve un campo «sorgente».** Chi ha spinto si ricostruisce dal log stesso, senza allargare
	 * il formato: la voce di categoria `Combat` dello stesso Blast che ha `TgtCell` uguale a `SrcCell` di
	 * questa e lo stesso `ActionId` porta in `SrcCell` la cella dell'attaccante. La chiave regge perche' una
	 * cella ospita al piu' un'unita': il bersaglio identifica il colpo in modo univoco. `ActionId` dice
	 * **con quale azione**, ed e' scritto qui direttamente.
	 */
	Displaced,
	/**
	 * Spostamento forzato ANNULLATO: la spinta e' stata registrata e risolta, e l'unita' e' rimasta dov'era
	 * (#420). Aggiunto in CODA, come i tre valori sopra: l'esito viaggia come `uint8` nel formato
	 * serializzato, quindi le tracce gia' scritte non cambiano significato.
	 *
	 * **Perche' un valore proprio e non `Stayed`.** `Stayed` dice «non pianificava movimento», che qui e'
	 * vero e irrilevante: la voce non esiste per raccontare cosa l'unita' voleva fare, ma per rispondere alla
	 * domanda che il giocatore pone davvero — *perche' non si e' mosso, se l'ho colpito?*. Senza questa voce
	 * il TurnLog e' asimmetrico: `#307` ha spiegato lo spostamento AVVENUTO e ha lasciato muto quello
	 * MANCATO, che e' il caso piu' frequente e l'unico su cui si sospetta un difetto del resolver.
	 *
	 * **Il PERCHE' viaggia in `Amount`**, come `ERTDisplacementBlockReason`. Non e' un'invenzione di questa
	 * voce: e' la stessa disciplina delle voci `Fallback`, dove `Amount` porta gia' `ERTActionInvalidReason`.
	 * Il campo e' libero per costruzione — le celle percorse sono zero, ed e' esattamente cio' che la voce
	 * dichiara — e riusarlo evita un campo nuovo, cioe' una versione di formato, per un dato che esiste solo
	 * su un esito.
	 *
	 * `SrcCell` e `TgtCell` sono la stessa cella: e' la forma leggibile di «non si e' spostata».
	 */
	DisplacementResisted,
	/**
	 * Fermata da un **Overwatch** che ha risposto `FIRE` alla propria finestra (CP 14.5): l'unita' e' entrata
	 * nella cella controllata, li' e' stata colpita, e li' resta — il movimento residuo non viene percorso.
	 * Aggiunto in CODA, come i quattro valori sopra: l'esito viaggia come `uint8` nel formato serializzato,
	 * quindi le tracce gia' scritte non cambiano significato.
	 *
	 * **Perche' non riusa `StoppedByPrediction`.** Le due si somigliano — un colpo armato prima, un movimento
	 * troncato — ma dicono al giocatore due cose diverse, e la differenza e' proprio cio' che E14 aggiunge al
	 * gioco: la previsione e' stata **decisa un turno prima** su una cella scelta al buio, l'Overwatch e' stato
	 * deciso **mentre passavi**, da qualcuno che ti ha visto arrivare e ha scelto te invece del tuo compagno.
	 * Un solo valore per entrambe renderebbe il replay incapace di distinguere una scommessa da una lettura.
	 *
	 * ⚠️ Chi lo scrive **sovrascrive** `FRTMovementResolutionState::BlockReason` anche se era gia' fissato, ed
	 * e' voluto: quella memoria tiene «il motivo del PRIMO congelamento» perche' i blocchi transitori si
	 * ripetono — un'unita' che perde una cella contesa la ritenta al passo dopo. Un `FIRE` non e' transitorio,
	 * e' terminale. E' la stessa scelta che `ApplyPredictionsToMoves` fa gia', assegnando l'esito senza
	 * chiedere se ce n'era uno.
	 */
	StoppedByOverwatch
};

/**
 * PERCHE' uno spostamento forzato non ha spostato nessuno (#420). Viaggia in `FRTTurnLogEntry::Amount`
 * delle voci con esito `ERTMoveOutcome::DisplacementResisted`.
 *
 * Le cinque cause **non** sono tutte difese, e la tassonomia lo dice: due sono decisioni dell'unita'
 * (`Guarded`, `Braced`), tre sono geometria del turno (`OpposingForces`, `NoDestination`,
 * `ContestedDestination`). Distinguerle e' il punto: un giocatore che legge «la guardia ha retto» impara una
 * regola, uno che legge «non si e' mosso» impara che il gioco e' rotto.
 *
 * ⚠️ **`PushResistance` non e' qui, e non e' una dimenticanza.** E' la terza resistenza sulla carta
 * (`RTTurnManager.cpp`, ramo `ERTActionEffect::Push`), ma [D-075](../../../docs/decisions/RT_PDR_00_Decision_Log.md)
 * l'ha portata a `0` su tutto il roster: nessun eroe la possiede, quindi un produttore su quel ramo sarebbe
 * codice non coperto **per costruzione** e un valore di enum che nessun test puo' raggiungere. Quando la
 * meccanica si risveglia, il valore si aggiunge in coda insieme al suo produttore e al suo test — e il punto
 * in cui va scritto porta gia' il commento che lo dice.
 *
 * Come per `ERTMoveOutcome`, i valori nuovi si aggiungono **in coda**: viaggiano come `int32` in `Amount`,
 * ma il lettore li interpreta per posizione.
 */
UENUM(BlueprintType)
enum class ERTDisplacementBlockReason : uint8
{
	/** `Action.Guard` ha retto: la guardia assorbe le spinte fino a `GuardResistedPushDistance` celle. */
	Guarded,
	/** `Action.Brace` ha retto: il primo spostamento del turno non sposta, a qualunque distanza. */
	Braced,
	/**
	 * Spinto da DUE o piu' attaccanti nello stesso Blast: le forze si annullano e la contesa resta ferma.
	 * Non e' una difesa — l'unita' non ha fatto nulla — ed e' la ragione per cui questo enum non si chiama
	 * `...ResistReason`.
	 */
	OpposingForces,
	/** Nessuna destinazione: bordo mappa, ostacolo o unita' subito dietro. La spinta non ha dove andare. */
	NoDestination,
	/**
	 * Due bersagli spinti verso la STESSA cella nello stesso Blast: restano entrambi fermi, perche' l'esito
	 * non deve dipendere da quale dei due si risolve prima.
	 *
	 * ⚠️ Questo caso non era nell'elenco di `#420`, che ne contava cinque: e' emerso leggendo il secondo
	 * ciclo del knockback, dove `bContested` faceva `continue` senza scrivere ne' una riga di combat log ne'
	 * una voce. Era il piu' muto dei sei.
	 */
	ContestedDestination,
	/**
	 * `Reaction.Anchor` ha annullato lo spostamento (CP 7.5, `#505`): una reazione ATTIVATA, quindi consumata
	 * per il turno — a differenza di `Guarded` e `Braced`, che sono stati e non spendono un'attivazione.
	 *
	 * Vale per la spinta e per la trazione, e regge a qualunque distanza: «impedisce **una** spinta» e' un
	 * conteggio, non una soglia (D-094). Sta in coda all'enum perche' i valori finiscono nel TurnLog
	 * serializzato.
	 */
	Anchored
};

/** Esito di un attacco nel turno. Priorita': Lethal > ShieldAbsorbed > TerrainBonus > Hit. */
UENUM(BlueprintType)
enum class ERTCombatOutcome : uint8
{
	Hit,            // danno inflitto agli HP
	ShieldAbsorbed, // danno assorbito interamente dallo scudo (HP invariati)
	Lethal,         // bersaglio portato a HP <= 0
	NoLineOfSight,  // attacco pianificato scartato per LOS bloccata
	TerrainBonus,   // colpo a segno con bonus altura (+danno), non letale
	/**
	 * Cura applicata (CP 8.5, `Action.Heal`). Valore AGGIUNTO in coda come `Fallback`/`Reaction` prima: le
	 * tracce gia' scritte non cambiano significato. Sta fra gli esiti di combattimento e non in una categoria
	 * propria perche' e' la stessa domanda — quanti punti vita ha cambiato quell'azione, e a chi.
	 */
	Healed
};

/**
 * Voce del TurnLog: un esito autoritativo del turno con il suo reason code. Osservabilita' separata
 * dalla presentazione (non e' FRTResolvedEvent). Deterministica: mai un pointer, mai l'ordine di spawn.
 *
 * ⚠️ La cella di partenza resta la chiave di ORDINAMENTO, non l'identita' dell'unita': non regge per le
 * voci ambientali, per l'interposizione (che scrive la cella del protetto) e dopo un Dash. L'identita' la
 * porta `UnitId` dal formato v6 (D-063).
 */
USTRUCT(BlueprintType)
struct FRTTurnLogEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	ERTMatchPhase Phase = ERTMatchPhase::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	ERTLogCategory Category = ERTLogCategory::Move;

	/**
	 * Valore dell'enum di categoria: `ERTMoveOutcome` se Move, `ERTCombatOutcome` se Combat, `ERTFallbackOutcome`
	 * se Fallback, `ERTReactionOutcome` se Reaction, `ERTFacingOutcome` se Facing. Intero: no float (#4).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	uint8 Outcome = 0;

	/** Chiave stabile: cella di partenza dell'unita' nel turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FRTCellId SrcCell;

	/** Bersaglio (Combat) o destinazione (Move); = SrcCell se non applicabile. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FRTCellId TgtCell;

	/**
	 * Danno effettivo (Combat), numero di celle percorse (Move) o direzione come `ERTHexDirection` (Facing).
	 *
	 * Su alcuni esiti porta un REASON CODE invece di una quantita', perche' la quantita' li' non esiste:
	 * `ERTActionInvalidReason` sulle voci `Fallback`, e `ERTDisplacementBlockReason` sulle voci `Move` con
	 * esito `DisplacementResisted` (#420), dove le celle percorse sono zero per definizione. Il campo si
	 * legge sempre guardando prima `Category` e `Outcome` — non e' un intero con un significato solo.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 Amount = 0;

	/**
	 * IDENTITA' dell'azione che ha prodotto la voce, quando ne ha una (CP 5.5). `NAME_None` = non dichiarata.
	 *
	 * Serve perche' le reazioni degli eroi riusano la semantica delle azioni core: senza questo campo
	 * `Riktor.Interposition` e `Action.Intercept` produrrebbero voci IDENTICHE, e un replay non potrebbe piu'
	 * dire quale abilita' e' scattata. E' l'ActionId del catalogo, cioe' la chiave stabile: non cambia mai.
	 *
	 * Dal 2026-08-10 (CP 11.3, `#79`) lo popolano **anche le voci di combattimento** — colpi pianificati,
	 * contrattacchi e attacchi fermati dalla copertura — piu' le voci di movimento di `Move`, `Dash` e
	 * `Displaced` (`#307`). Prima lo riempivano solo le voci `Reaction`, e questo commento diceva che
	 * completare le altre era «lavoro di CP 11.3»: quel lavoro e' questo.
	 *
	 * Restano legittimamente vuote le voci che **non hanno** un'azione dietro: gli eventi ambientali che
	 * nessuno ha causato, e le voci di categoria `Facing` che registrano una LETTURA e non una scelta.
	 * `NAME_None` li' non e' un buco, e' la verita'.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FName ActionId;

	/**
	 * L'azione GENERICA di cui `ActionId` e' un profilo (es. `Action.BasicAttack` per `Riktor.ImpactShot`).
	 * `NAME_None` quando l'azione non e' profilo di niente, o quando chi ha scritto la voce non lo sapeva.
	 *
	 * Sta QUI e non solo nel catalogo perche' `Riktor.ImpactShot` e' un'azione d'EROE: chi legge una traccia
	 * non la risolve consultando il catalogo core, gli servirebbero i data asset del roster. Senza questo
	 * campo la traccia non e' spiegabile da sola, e [D-033](../../../docs/decisions/RT_PDR_00_Decision_Log.md)
	 * chiede esattamente questo.
	 *
	 * ⚠️ **NON entra nell'hash**, ed e' una scelta con un argomento: `BaseActionId` e' una FUNZIONE di
	 * `ActionId`, che nell'hash c'e' gia'. Due tracce non possono differire solo per questo campo, quindi
	 * includerlo aggiungerebbe zero potere discriminante — e invaliderebbe in blocco ogni hash golden.
	 * E' lo stesso ragionamento con cui `FormatId` e' rimasto fuori dall'hash (CP 10.3).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FName BaseActionId;

	/**
	 * IDENTITA' dell'unita' che ha prodotto la voce. `0` = nessuna unita' (voci ambientali).
	 *
	 * Esiste perche' `D-TL-2` — «chiave unita' = cella di partenza del turno» — **non regge** come chiave di
	 * identita', e il codice lo dimostra in tre punti: le voci ambientali non hanno un'unita' e `SrcCell` e' la
	 * cella che cambia superficie; l'interposizione scrive nel campo la cella del **protetto**, non dell'attore;
	 * e dopo un Dash la cella in fase Blast non e' piu' quella di partenza. La cella resta ottima chiave di
	 * **ordinamento** (`EntryLess`), che e' l'uso per cui D-TL-2 e' stata scritta.
	 *
	 * ⚠️ **NON entra nell'hash** ([D-063](../../../docs/decisions/RT_PDR_00_Decision_Log.md)): serve a rendere
	 * la traccia spiegabile, non a discriminarla. Stesso ragionamento di `FormatId` e `BaseActionId`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 UnitId = 0;

	/**
	 * Turno in cui la voce e' stata emessa. `0` = non dichiarato (tracce scritte prima del formato v6).
	 *
	 * Il TurnLog vive per turno, quindi il numero e' una costante per traccia — ma una traccia estratta dal suo
	 * contenitore, che e' esattamente cio' che un replay archive fa, senza questo campo non saprebbe piu' dirlo.
	 * ⚠️ **NON entra nell'hash**, per la stessa ragione di `UnitId`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 TurnNumber = 0;

	/**
	 * Revisione del grafo di mappa in vigore quando la voce e' stata emessa. `0` = non dichiarata.
	 *
	 * `URTHexMapAsset::Revision` sale a ogni modifica strutturale, e sale **durante** la risoluzione: una
	 * porta che si apre, una superficie che cambia, un ponte che crolla. Senza questo campo una traccia non
	 * puo' dire su QUALE grafo un movimento e' stato validato — che e' esattamente la domanda del caso
	 * «un'unita' ha attraversato un muro», dove il muro potrebbe essere caduto due eventi prima.
	 *
	 * ⚠️ **ENTRA nell'hash**, al contrario di `UnitId` e `TurnNumber`, e per lo stesso criterio: due tracce
	 * POSSONO differire solo per questo campo — stessi eventi, grafo diverso perche' modificato in un turno
	 * precedente — e sono due partite diverse. Entrando nell'hash entra anche in `EntryLess`: un campo
	 * serializzato che l'ordinamento non guarda lascia la forma canonica indefinita fra due voci a pari merito.
	 *
	 * Non esiste un `TransitionId` che lo accompagni, ed e' deliberato: `FRTHexEdge` e' `From`/`To`/`Cost`/`Kind`
	 * **senza ID**, perche' nel progetto l'identita' di un bordo E' la coppia di celle — che questa voce porta
	 * gia' in `SrcCell`/`TgtCell`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 GraphRevision = 0;

	/**
	 * Priorita' intra-fase dell'azione che ha prodotto la voce (CP 11.3, `#79`). `0` = non dichiarata.
	 *
	 * E' il numero che decide **in che ordine** due azioni della stessa fase risolvono (motore di E4:
	 * priorita' intera, nessun bias di Player ID). Senza, una traccia dice *cosa* e' successo e non permette
	 * di ricostruire *perche' in quell'ordine* — che e' la domanda di ogni turno in cui due unita' agiscono
	 * insieme e una vince la contesa.
	 *
	 * ⚠️ **NON entra nell'hash**: e' una FUNZIONE di `ActionId`, che nell'hash c'e' gia' — stesso argomento
	 * di `BaseActionId`. **Entra invece in `EntryLess`**, perche' viene SCRITTO: vedi la dichiarazione di
	 * `ERTTurnLogFormatVersion::WithPriority` per il perche' le due cose non si contraddicono.
	 *
	 * Il valore viene dal catalogo (`FRTActionDef::Priority`), non e' ricalcolato qui: chi legge la traccia
	 * non deve caricare i data asset del roster per sapere in che ordine il turno ha risolto — la stessa
	 * ragione per cui `BaseActionId` sta nella voce e non solo nel catalogo.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 Priority = 0;

	/**
	 * Identita' della finestra a cui questa voce risponde (CP 14.5). Vuota su ogni voce che non sia una
	 * decisione — cioe' quasi tutte.
	 *
	 * E' `URTReactionOpportunityLibrary::DeriveOpportunityId`, che e' una FUNZIONE dei sei campi della chiave:
	 * turno, macro-fase, micro-step, proprietario, reaction e progressivo. E' questo che rende il replay
	 * possibile — rieseguendo lo stesso turno il resolver ricalcola gli stessi id, ci ritrova le risposte
	 * registrate, e non deve chiedere niente a nessuno.
	 *
	 * ⚠️ **Il `DecisionBoundary` non ha un campo proprio, ed e' deliberato**: il micro-step e' gia' uno dei sei
	 * componenti dell'id. Un campo separato sarebbe una seconda copia dello stesso fatto, cioe' due dati che
	 * possono contraddirsi — e quando si contraddicono non c'e' modo di sapere quale sia quello giusto. E' lo
	 * stesso argomento con cui `BaseActionId` resta fuori dall'hash perche' e' funzione di `ActionId`.
	 *
	 * ⚠️ **ENTRA nell'hash**: due tracce possono differire solo per quale finestra una decisione stia
	 * chiudendo, e sono due partite diverse.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FString OpportunityId;

	/**
	 * Quale ISTANZA di reaction ha risposto, quando la stessa unita' ne ha piu' d'una armata (CP 14.5).
	 * `INDEX_NONE` = non applicabile.
	 *
	 * ⚠️ **NON entra nell'hash**, stesso criterio di `UnitId` e `BaseActionId`: e' un numero d'ordine
	 * dell'armamento, quindi serve a rendere la traccia spiegabile — *quale* delle due Overwatch ha sparato —
	 * non a discriminarla. Due partite che differissero solo per questo differirebbero gia' per l'`OpportunityId`,
	 * che l'istanza la contiene attraverso `Seq`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 ReactionInstanceId = INDEX_NONE;

	/**
	 * CHI e' stato scelto da un `FIRE` (CP 14.5). `INDEX_NONE` su `HOLD`, ed e' la verita': non c'e' bersaglio.
	 *
	 * Un campo proprio e non la sola `TgtCell`, benche' la cella del bersaglio sia scritta li': dedurre
	 * l'unita' dalla cella e' precisamente l'inferenza che [D-063] ha dichiarato non valida quando ha
	 * introdotto `UnitId`. Vale qui per la stessa ragione, e in piu' per una sua: fra il micro-step in cui si
	 * spara e la fine del turno quella cella puo' cambiare occupante.
	 *
	 * ⚠️ **ENTRA nell'hash**: sparare a `A` invece che a `B` e' la decisione stessa, ed e' la differenza fra
	 * due partite.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 SelectedTargetUnitId = INDEX_NONE;

	/**
	 * CHI era il bersaglio ORIGINALE, quando un colpo e' stato **redirezionato** (`#1060`). `INDEX_NONE` su
	 * ogni voce che non redirige — che e' la verita': non c'e' stato nessun trasferimento.
	 *
	 * Oggi lo produce la sola interposizione (`ERTReactionTrigger::AllyHitByDirectAttack`): Riktor si mette
	 * davanti a Wraith, e il colpo che era per Wraith lo incassa Riktor. `UnitId` dice **chi lo incassa** —
	 * e' l'unita' che reagisce — quindi con questo campo la voce nomina entrambi i capi del trasferimento.
	 *
	 * 🔴 **Porta uno `StableUnitId`, come `UnitId` e a differenza di `SelectedTargetUnitId`**, che invece porta
	 * l'indice di risoluzione. I due spazi di identificatori convivono in questa struct dalla v8, e la scelta
	 * qui non e' stilistica: i due capi del trasferimento stanno nella **stessa voce**, e nominarli in spazi
	 * diversi sarebbe illeggibile — sono entrambi `int32`, e su un'arena piccola i valori coincidono per caso,
	 * quindi l'errore non si manifesterebbe finche' qualcuno non gioca una partita grande.
	 *
	 * ⚠️ **Un campo proprio e non la sola `SrcCell`**, benche' la cella del protetto sia scritta li'. E' la
	 * stessa inferenza che [D-063] ha dichiarato non valida introducendo `UnitId`, e qui e' pure peggio: la
	 * `SrcCell` di questa voce e' la cella di un'ALTRA unita', quindi chi legge deve gia' sapere che questa
	 * voce e' un'interposizione per interpretarla. Un'assertion di scenario che risolvesse la cella
	 * troverebbe l'occupante di fine turno, non chi era bersagliato al Blast.
	 *
	 * ⚠️ **NON entra nell'hash** ([D-063]): il trasferimento e' **gia' discriminato** da `SrcCell`, che
	 * nell'hash c'e' — interporsi per Wraith invece che per Phase da' due celle diverse e quindi due hash
	 * diversi. Questo campo rende quel fatto **leggibile** senza inferenza, non lo aggiunge. E' lo stesso
	 * argomento di `BaseActionId` (funzione di `ActionId`) e di `Priority`: zero potere discriminante in piu',
	 * e includerlo invaliderebbe in blocco gli hash golden per un dato che non discrimina nulla.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 OriginalTargetUnitId = INDEX_NONE;

	/**
	 * Il **token della risposta** applicata a un decision boundary, quando non e' derivabile dall'esito
	 * (E14.7, [D-047]). Vuoto altrimenti — che e' il caso di ogni finestra dell'Overwatch.
	 *
	 * 🔴 **Perche' un campo e non una deduzione.** `ArmRecordedReactionDecisions` ricostruisce la risposta
	 * dall'`Outcome`: `FireChosen` -> `FIRE:<SelectedTargetUnitId>`, ogni altro esito -> `HOLD`. Regge finche'
	 * il vocabolario e' chiuso, e quello dell'Overwatch lo e'. Il `Brace` apre finestre le cui risposte
	 * vengono dal **catalogo** — `Hold Ground`, `SIDESTEP`, e quelle che i profili aggiungeranno — e per
	 * quelle la deduzione non e' incompleta: e' **sbagliata**, perche' produrrebbe `HOLD`, che in una finestra
	 * di `Brace` non e' nemmeno una risposta legale. Il replay la rifiuterebbe come «registrata illegale»,
	 * accusando la traccia di un difetto del lettore.
	 *
	 * ⚠️ **Vuoto = «deducila come sempre»**, e questa e' la proprieta' che tiene l'Overwatch fuori dal
	 * cambiamento: le sue voci non portano il token, si rileggono con la regola di prima, e una traccia
	 * scritta ieri significa oggi la stessa cosa. Non e' un default di comodo — e' il ponte fra le due
	 * versioni del formato.
	 *
	 * ⚠️ **NON entra nell'hash**, per lo stesso argomento di `BaseActionId` e `OriginalTargetUnitId`: la
	 * decisione e' **gia' discriminata** da `Outcome` e — dove c'e' — da `SelectedTargetUnitId`, che l'hash
	 * mescola. ⛔ La conseguenza va detta invece che taciuta: due risposte di profilo **diverse** con lo
	 * stesso esito danno oggi lo stesso hash. Non e' un buco del determinismo — la risposta E' nella traccia,
	 * e il replay la rilegge da qui — ma un `StateHash` uguale non prova piu' che la stessa risposta sia stata
	 * scelta. Prova che lo **stato finale** coincide, che e' cio' che quell'hash ha sempre dichiarato di
	 * misurare. Il giorno in cui servisse discriminare la scelta, il campo entra nell'hash e gli hash golden
	 * si rifanno: e' una decisione, non un'omissione.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FString ReactionResponse;

	FRTTurnLogEntry() = default;
};

/**
 * Versione del formato di serializzazione binaria del TurnLog. Ogni formato serializzato e' versionato
 * (invariante #4): il loader rifiuta versioni sconosciute invece di interpretare byte arbitrari.
 * Non e' UENUM (uint16 esce dai vincoli UHT del BlueprintType uint8): e' una costante di formato interna.
 */
enum class ERTTurnLogFormatVersion : uint16
{
	Initial      = 1, // header + voci, senza checksum (mai persistito su file)
	WithChecksum = 2, // + checksum FNV del payload in coda: rileva la corruzione del contenuto
	/**
	 * + `ActionId` per voce (CP 5.5), scritto come lunghezza uint16 e byte UTF-8 in coda alla voce.
	 * E' il primo campo a lunghezza variabile del formato. Le tracce in versione 2 restano LEGGIBILI: il
	 * loader le accetta e ne lascia l'ActionId vuoto, che e' esattamente cio' che quei byte dicevano.
	 */
	WithActionId = 3,
	/**
	 * + `FormatId` nell'HEADER (CP 10.3): l'identita' del formato di partita in vigore, subito dopo i flags.
	 * Sta nell'header e non nelle voci perche' sarebbe una costante ripetuta N volte, e **non entra
	 * nell'hash**: l'hash mescola i campi delle voci, e includervi un campo di contesto invaliderebbe in
	 * blocco ogni hash golden. Le tracce in versione 3 restano LEGGIBILI, con `FormatId` neutro.
	 */
	WithFormatId = 4,
	/**
	 * + `BaseActionId` per voce (issue #354): l'azione generica di cui `ActionId` e' un profilo, scritta come
	 * l'ActionId — lunghezza uint16 e byte UTF-8, subito dopo di esso. Serve a D-033, che chiede che una
	 * traccia sia spiegabile come *azione base + profilo* senza dover caricare i data asset degli eroi.
	 *
	 * Le tracce in versione 2, 3 e 4 restano LEGGIBILI, con `BaseActionId` vuoto — che e' esattamente cio'
	 * che quei byte dicevano. E **gli hash golden non cambiano**: il campo sta fuori dall'hash (vedi la
	 * dichiarazione di `FRTTurnLogEntry::BaseActionId` per il perche').
	 */
	WithBaseActionId = 5,
	/**
	 * + `UnitId` e `TurnNumber` (D-063) e `GraphRevision` ([D-067](../../../docs/decisions/RT_PDR_00_Decision_Log.md)):
	 * **tre** int32 in coda alla voce, dopo `BaseActionId`. I campi precedenti non si spostano, quindi un
	 * lettore che sa saltare le stringhe trova tutto dov'era.
	 *
	 * Le tracce dalla 2 alla 5 restano LEGGIBILI, con i tre campi a `0` — che e' esattamente cio' che quei
	 * byte dicevano: `UnitId = 0` significa «nessuna unita'», e dedurre l'unita' dalla cella sarebbe
	 * l'inferenza che D-063 ha dichiarato non valida.
	 *
	 * ⚠️ **Gli hash golden CAMBIANO**, e va detto perche' la stesura precedente affermava il contrario:
	 * `UnitId` e `TurnNumber` restano fuori dall'hash, ma `GraphRevision` vi entra (D-067), e un passo FNV
	 * in piu' cambia il valore di **ogni** traccia anche mescolando `0`. Il corpus golden non si rompe per
	 * un'altra ragione — confronta tracce ricalcolate su entrambi i lati, non costanti pinnate — non perche'
	 * gli hash siano rimasti quelli.
	 *
	 * ⚠️ L'hash ordinato di D-062 **non e' qui, ed e' deliberato**: i byte sono in forma canonica (`D-SR-1`),
	 * quindi la serializzazione perde l'ordine di emissione e un hash dell'ordine scritto in questo header
	 * renderebbe i byte dipendenti dall'ordine d'inserimento — cioe' romperebbe `D-SR-1` e il test
	 * `SerializeCanonicalPermutationInvariant`. Quel valore appartiene all'header del Replay Archive.
	 */
	WithUnitId = 6,
	/**
	 * + `Priority` per voce (CP 11.3, `#79`): **un** int32 in coda, dopo i tre della v6. I campi precedenti
	 * non si spostano.
	 *
	 * Chiude l'ultima voce di codice del DoD di CP 11.3 — «ogni voce riporta `ActionId`, `Priority`,
	 * coordinate assiali, bersaglio e `ValidationResult`». Era stata rimandata di proposito il 2026-08-10:
	 * la `v6` era gia' rivendicata da un altro ramo, e **due formati con lo stesso numero** sono il difetto
	 * peggiore possibile per un file versionato, perche' il loader sceglie l'interpretazione dal numero e non
	 * ha modo di accorgersi dello scambio. La `v7` e' stata presa dopo aver verificato **tutti** i branch
	 * remoti, non solo `main`: e' il controllo che [D-070] ha reso obbligatorio.
	 *
	 * Le tracce dalla 2 alla 6 restano LEGGIBILI, con `Priority = 0`.
	 *
	 * ⚠️ **NON entra nell'hash**, stesso argomento di `BaseActionId`: la priorita' e' una **funzione**
	 * dell'`ActionId`, che nell'hash c'e' gia'. Due tracce non possono differire solo per questo campo, quindi
	 * includerlo aggiungerebbe zero potere discriminante e invaliderebbe in blocco ogni hash golden.
	 *
	 * ⚠️ **Entra invece in `EntryLess`**, e non e' una contraddizione: l'hash risponde a «due tracce sono la
	 * stessa partita?», l'ordinamento a «quale voce scrivo prima nel file». Un campo SCRITTO che il confronto
	 * non guarda lascia due voci a pari merito, dove a decidere resta `TArray::Sort`, che non e' stabile —
	 * due inserimenti diversi produrrebbero due file diversi con lo stesso contenuto, rompendo `D-SR-1`.
	 */
	WithPriority = 7,
	/**
	 * + `OpportunityId` (stringa), `ReactionInstanceId` e `SelectedTargetUnitId` per voce (CP 14.5): la
	 * **decisione** di una finestra di reazione. I campi precedenti non si spostano — la stringa va dopo
	 * `Priority`, i due interi dopo di lei.
	 *
	 * E' la dimensione che ADR-0004 aveva previsto fra i costi del modello: *«il TurnLog cresce di una
	 * dimensione (decisioni), e la sua serializzazione va versionata»*. Senza, la decisione di un giocatore
	 * vivrebbe solo nella memoria della sessione che l'ha presa, e il replay dovrebbe reinterrogare qualcuno —
	 * cioe' non sarebbe un replay.
	 *
	 * Le tracce dalla 2 alla 7 restano LEGGIBILI, con `OpportunityId` vuoto e i due interi a `INDEX_NONE`:
	 * e' esattamente cio' che quei byte dicevano, perche' in quelle versioni nessuna finestra si apriva in
	 * partita.
	 *
	 * ⚠️ **Gli hash golden CAMBIANO** per le tracce che contengono decisioni, e non cambiano per le altre:
	 * `OpportunityId` e `SelectedTargetUnitId` entrano nell'hash, ma un id vuoto non mescola nulla e
	 * `INDEX_NONE` viene mescolato solo quando l'id c'e'. Una traccia senza decisioni ha lo stesso hash di
	 * prima. `ReactionInstanceId` resta fuori (vedi la sua dichiarazione).
	 *
	 * ⚠️ Il numero **8 e' stato verificato su tutti i branch remoti**, non solo su `main`: e' il controllo che
	 * [D-070] ha reso obbligatorio dopo il caso della v6 rivendicata due volte, e il commento della v7 lo
	 * ripete perche' e' il difetto peggiore possibile per un formato versionato — il loader sceglie
	 * l'interpretazione dal numero e non ha modo di accorgersi dello scambio.
	 */
	WithReactionDecision = 8,
	/**
	 * + `OriginalTargetUnitId` per voce (`#1060`): CHI era il bersaglio prima di un redirect. Il campo va in
	 * coda, dopo `SelectedTargetUnitId`; nessuno dei precedenti si sposta.
	 *
	 * Serve perche' la feature dell'interposizione esiste dal `#200` e **non e' verificabile da uno scenario**:
	 * `OriginalTargetEquals` ed `EffectiveTargetEquals` non avevano un dato su cui poggiare. Il bersaglio
	 * effettivo era gia' leggibile (`UnitId`), l'originale no — viveva solo come `SrcCell`, cioe' come
	 * inferenza che [D-063] vieta.
	 *
	 * Le tracce dalla 2 alla 8 restano LEGGIBILI, con `OriginalTargetUnitId` a `INDEX_NONE`: e' esattamente
	 * cio' che quei byte dicevano, perche' in quelle versioni il redirect non era registrato.
	 *
	 * ✅ **Gli hash golden NON cambiano, e non e' un caso fortunato**: il campo sta fuori dall'hash per
	 * costruzione (vedi la sua dichiarazione), quindi ogni traccia — con o senza interposizioni — conserva
	 * l'hash che aveva. E' la differenza con la v8, che invece li fece cambiare per le tracce con decisioni.
	 *
	 * ⚠️ Il numero **9 e' stato verificato su tutti i ref**, come [D-070] impone dopo il caso della v6
	 * rivendicata due volte: 44 ref esaminati leggendo `ERTTurnLogFormatVersion` in ciascuno — 8 e' il massimo
	 * dichiarato ovunque, e nessuno rivendica il 9.
	 */
	WithRedirectOrigin = 9,
	/**
	 * + `ReactionResponse` per voce (E14.7, [D-047]): il **token** della risposta applicata a un decision
	 * boundary, scritto come l'`ActionId` — lunghezza uint16 e byte UTF-8 in coda alla voce.
	 *
	 * 🔴 **Esiste perche' la risposta ha smesso di essere DERIVABILE dall'esito.** Con il solo Overwatch il
	 * vocabolario era chiuso e la ricostruzione bastava: `FireChosen` -> `FIRE:<bersaglio>`, ogni altro esito
	 * -> `HOLD`, ed e' cio' che `ArmRecordedReactionDecisions` fa. Il `Brace` di [D-047] apre finestre il cui
	 * vocabolario viene dal **catalogo** (`Hold Ground`, `SIDESTEP`, e quelle che i profili aggiungeranno):
	 * per quelle, ricostruire da un `uint8` significa indovinare. Il campo porta la risposta invece di
	 * dedurla.
	 *
	 * ⚠️ **Vuoto quando la risposta E' derivabile**, e non e' un'ottimizzazione: e' cio' che tiene l'Overwatch
	 * fuori da questo cambiamento. Una voce senza token si rilegge con la regola di sempre, quindi le tracce
	 * dell'Overwatch — scritte prima o dopo questa versione — significano esattamente la stessa cosa.
	 *
	 * Le tracce in versione 2..9 restano LEGGIBILI, con `ReactionResponse` vuoto — che e' esattamente cio'
	 * che quei byte dicevano. E **gli hash golden non cambiano**: il campo sta fuori dall'hash, per la stessa
	 * ragione di `BaseActionId` e `OriginalTargetUnitId` — l'esito e il bersaglio, che l'hash gia' mescola,
	 * discriminano gia' la decisione; il token la rende *leggibile*, non piu' distinguibile.
	 *
	 * ⚠️ Il numero **10 e' stato verificato su tutti i ref**, come [D-070] impone: 42 ref esaminati leggendo
	 * `ERTTurnLogFormatVersion` in ciascuno — 9 e' il massimo dichiarato ovunque, e nessuno rivendica il 10.
	 */
	WithReactionResponse = 10
};

/**
 * Topologia della griglia a cui appartengono le celle del log, dichiarata nei flags dell'header.
 * Le voci portano 3 interi per cella: nel quadrato sono offset (X,Y,Layer), nell'esagonale assiali (q,r,Layer).
 * Senza questo marcatore le due tracce sarebbero indistinguibili e un confronto incrociato darebbe un falso
 * "nessuna divergenza". Square = 0 -> i file scritti prima di questa estensione restano leggibili.
 * Non e' UENUM (uint16, come ERTTurnLogFormatVersion): e' una costante di formato interna.
 */
enum class ERTLogTopology : uint16
{
	Square = 0,
	Hex    = 1
};
