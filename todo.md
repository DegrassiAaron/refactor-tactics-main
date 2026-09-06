Il criterio di successo è una caduta. RebuildCostScalesWithTheMapNotTheEdit (RTHexMapActorTests.cpp:1263) oggi passa asserendo Grande > Piccola * 3. Quando il per-cella arriva quel test deve cadere, e va sostituito col suo opposto — non riallineato.

Il rischio è misurato, non temuto. RemoveInstance non è mai chiamato nel runtime: introdurlo è nuovo. Rimescola gli indici su cui poggiano 18 array paralleli (RTHexMapActor.h:939–1006), e il guardiano che troverai — l'ensureMsgf a RTHexMapActor.cpp:2132 — confronta solo i conteggi: due array della stessa lunghezza con le celle scambiate lo superano in silenzio. Il commento accanto dichiara la posta: celle velate sbagliate, che si leggono come «problema grafico» per settimane.

Serve una terza asserzione che il per-famiglia non aveva: dopo N modifiche incrementali la board deve essere identica a quella di un rebuild totale — celle giuste alle posizioni giuste, non solo lo stesso numero. È l'unica che coglie un rimescolamento.

E la disciplina che oggi è costata un giro: rt-suite.ps1 non compila, e VALIDA non lega binario e sorgente. Un fallimento che nomina un sistema che non hai toccato è il binario, non main.

Due correzioni a cose che ho detto in questa sessione, ora verificate: gli array paralleli sono 18, non dodici; e StructuralBodies non ne ha nessuno — i corpi non partecipano al velo.
