"""RT3 Control Plane - coordinamento fra sessioni RT3 su una singola workstation.

Il control plane tiene sessioni, eventi, mailbox e routing. NON tiene Git, worktree,
branch, HEAD, Unreal, build o asset: di quelli conserva soltanto i RIFERIMENTI che una
sessione dichiara al momento della registrazione. E' la separazione fra control plane e
data plane, ed e' il motivo per cui questo pacchetto non esegue mai un comando che muta
il repository.

Le due versioni qui sotto esistono perche' i workspace permanenti sono tre CLONI
distinti dello stesso remote, e nulla garantisce che aggiornino il control plane nello
stesso momento. Senza una versione esplicita, un client vecchio che parla con un daemon
nuovo fallisce in modo oscuro - tipicamente con un KeyError su un campo che non esisteva
ancora. Con la versione, fallisce subito e dice cosa aggiornare.

    PROTOCOL_VERSION  il contratto client <-> daemon (op, argomenti, forma delle
                      risposte). Si alza quando un client vecchio non puo' piu' essere
                      servito correttamente da un daemon nuovo, o viceversa.

    SCHEMA_VERSION    la forma del database. Si alza a ogni migrazione. Il daemon
                      rifiuta di aprire un database con schema PIU' NUOVO del proprio,
                      perche' non sa cosa ci sia dentro; uno piu' vecchio lo migra.

Le due NON sono la stessa cosa e non vanno unificate: una migrazione che aggiunge una
colonna con default non rompe nessun client, e alzare il protocollo in quel caso
costringerebbe tre workspace ad aggiornare per niente.
"""

PROTOCOL_VERSION = 1
SCHEMA_VERSION = 1

__all__ = ["PROTOCOL_VERSION", "SCHEMA_VERSION"]
