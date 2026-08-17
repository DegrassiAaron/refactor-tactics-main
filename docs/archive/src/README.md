# Archivio dei sorgenti recepiti

> `HISTORICAL` · **Materiale NON autorevole** · **Primo lotto archiviato il 2026-08-08**
>
> 🟡 **Tredicesima volta — previsione scritta, e poi verificata nella stessa sessione.** Il ramo
> `docs/consolidamento-4-processi` ha scritto **71** sulla propria base e accanto: «ma
> [#836](https://github.com/DegrassiAaron/refactor-tactics-main/pull/836) archivia nella stessa
> radice: quando entrambe atterrano sarà **72**». Poi #836 **è atterrata mentre la PR era aperta**
> — alle 22:56 UTC, `84cbb70c` — e `git merge origin/main` ha risposto **72** = 17 + 47 + 2 + **6**.
>
> ⚠️ **La lezione nuova non è il numero: è che il controllo è scaduto DENTRO il lavoro.** «#836 è
> viva» era vero quando l'ho misurato e falso un'ora dopo, e stava scritto in quattro punti — questo
> banner, `parallel-batch.yaml`, il triage e il corpo della PR. Nessun gate lo vede: i link
> risolvono, i totali tornano, e il documento descrive un repository che non c'è più. Rileggere
> `origin/main` **dopo ogni passo lungo e prima del merge** non è prudenza, è l'unica misura valida.
>
> ✅ E la previsione ha avuto un dividendo misurabile: dei tre conflitti del merge, **tutti e tre**
> erano righe che questo ramo aveva già dichiarato contese, e nessuno ha richiesto di scegliere un
> lato — il Decision Log ha tenuto entrambe le decisioni (`D-138` e `D-139`, in ordine numerico) e i
> due totali si sono rimisurati da soli col comando.
>
> 🟢 **Dodicesima volta, e questa non ha una previsione da rivendicare: solo il rimedio, applicato.** Il ramo
> `docs/consolidamento-walls-doors-v1` ha archiviato l'handoff *Walls/Doors/InteractionGraph* senza dichiarare
> in anticipo lo scarto — a differenza dell'undicesima — e nel frattempo `docs/wiki-player-first` è atterrato
> su `main` portando la radice da 3 a 4. Il totale non è stato dedotto: **rimisurato sull'albero mergiato** col
> comando completo, **71** = 17 + 47 + 2 + **5**. La riga qui sotto non è stata incrementata a mano.
>
> Vale la pena dirlo così: la previsione è un lusso, il rimedio no. Undici volte su undici il numero è tornato
> giusto **eseguendo il comando dopo l'unione**, che funziona anche quando nessuno ha guardato `gh pr list`.
>
> 🟢 **Undicesima volta — e la prima in cui lo scarto è stato dichiarato prima di prodursi, poi
> verificato.** Il ramo `docs/wiki-player-first` ha scritto nel proprio README, *prima* di aprire la
> PR: «questo ramo porta la radice da 2 a 3, ma #818 archivia nella stessa radice: quando entrambe
> atterrano il totale sarà **70**, e nessuno dei due rami può scriverlo perché nessuno dei due vede
> l'altro». Al merge il comando ha risposto **70** = 17 + 47 + 2 + **4**. Entrambi i rami avevano
> scritto `69`, entrambi misurati col comando buono, entrambi giusti sulla propria base — la nona
> lezione, alla lettera. ⚠️ **E la prima stesura di questo stesso banner aveva lasciato `69` nella
> frase qui sotto** mentre la formula diceva `70`: il numero corretto in un posto e non nell'altro,
> dentro il paragrafo che celebra di aver previsto lo scarto. Trovato dalla code review, non da me.
>
> La differenza rispetto alle nove volte precedenti non è il rimedio, che è sempre lo stesso
> (**rimisurare dopo il merge**, mai sommare i delta): è che il conflitto è stato **previsto
> leggendo `gh pr list`**, non scoperto dopo. Una PR aperta che tocca la tua stessa riga è
> un'informazione disponibile *prima* di scrivere il numero.
>
> 🔵 **Undicesima rimisura, 2026-08-16 — e questa volta il numero era giusto.** `74` → **75** con
> l'archiviazione dell'handoff *Frontend/Menu Features e Tracking*, misurato col comando canonico e
> **scomposto**, che è il controllo che la decima volta ha insegnato:
> `48` `handoff/` + `17` `design/` + `2` `audit/` + `8` in radice = **75**. Gli addendi tornano col totale.
> 🔴 **La verifica in più che questa riga si era aggiunta era falsa**, ed è stata trovata in code review:
> diceva *«e l'indice di `handoff/` qui sotto ha una riga per ciascuno dei 48»*. Misurato — link
> `handoff/*.md` nel README contro i file su disco — sono **45 su 48**, e i tre scoperti sono esattamente
> quelli che le righe più sotto dichiarano mancanti sotto
> [#579](https://github.com/DegrassiAaron/refactor-tactics-main/issues/579) (OPEN). È il caso peggiore di
> questa pagina: una verifica **aggiunta** per irrobustire il conteggio, e che afferma proprio ciò che il
> documento sa essere falso quaranta righe più giù. Il totale e gli addendi restano corretti. ⚠️ Vale la solita riserva, ed è
> l'unica ragione per cui questa riga esiste: **il valore va riletto sull'albero mergiato**, perché un `+1`
> scritto prima dell'unione è giusto solo se nessun altro ramo archivia oggi.
>
> ⏭️ **E la riserva è scattata lo stesso giorno.** `docs/mini01-consolidamento-autobattle` archiviava in
> radice mentre questa riga veniva scritta: sull'albero unito il totale è **76** e la radice **9**, quindi
> la scomposizione qui sopra — `48 + 17 + 2 + 8 = 75` — è corretta **alla propria base** e superata dopo
> l'unione. Non si riscrive: è il suo valore. Il numero vivo sta nella riga del conteggio, e si rimisura
> col comando.
>
> 🔴 **Quindicesima volta, e stavolta la deriva non era di uno soltanto: il numero era vecchio di *tre*.**
> Il 2026-08-17, prima di archiviare qualunque cosa, il comando rispondeva **79** = 17 `design/` + 48
> `handoff/` + 2 `audit/` + **12 in radice** — contro i **76** dichiarati qui sotto. I tre di scarto erano
> già stati **previsti** dalla track `docs_kit` il 2026-08-16, che aveva scritto *«dichiara 72 e la misura
> di oggi ne dà 78»* e correttamente non aveva incrementato a mano. Con l'archiviazione del Graybox Kit il
> valore misurato sulla propria base era **80** = 17 + 48 + 2 + **13**; sull'albero unito di *quel giro*
> è stato **81**, e il paragrafo ⏭️ qui sotto lo documenta con la previsione che l'aveva anticipato.
> ⏭️ **Superato lo stesso giorno**: il valore vivo è **83**, misurato dal paragrafo 🔵 in testa. Questa riga
> diceva *«il valore vivo … è 81»* e l'aggettivo è scaduto in poche ore — è la deriva che questa pagina
> combatte, e la correzione è stata **datare la misura** invece di riscriverne il racconto.
>
> ⚠️ **E l'indice della radice rispondeva 11, non 14** *(misura del 2026-08-17, giro del Graybox)* — ``sed -n '/^## Radice/,/^## Nota/p' … | grep -c '^| \[`' ``.
> È il difetto della **decima volta**, tornato e più grande: tre file stanno sul disco senza una riga
> d'indice, e sono i tre archiviati dalla track `docs_kit`
> (`CLAUDE_Apply_Elemental_Proficiency…`, `CLAUDE_Reconcile_v0.1_Skill_Ability…`,
> `RefactorTactics_v0.1_Characters_Elemental_Consolidation`). **Non le ricostruisco**: una riga d'indice
> dice *cosa quel sorgente ha prodotto*, e scriverla senza l'istruttoria di quei tre kit sostituirebbe una
> lacuna visibile con una plausibile — che è precisamente ciò che la decima lezione vieta. Lo scarto è
> dichiarato qui perché il prossimo che misura lo trovi già spiegato.
>
> ⏱️ **Il valore va rimisurato sull'albero mergiato, non incrementato**: è la lezione che questa riga ha
> già imparato quattordici volte, e questo ramo non vede gli altri.
>
>
> ⏭️ **La previsione del ramo `docs/tactical-designer-consolidamento` ha retto alla lettera, ed è la
> sedicesima volta che questo paragrafo la scrive.** Quel ramo aveva misurato **79** sulla propria base e
> scritto, *prima* del merge: «la PR #1099 archivia nella stessa radice e nessuno dei due rami vede l'altro —
> quando atterrano entrambi il valore sarà **81** = 17 + 48 + 2 + **14**, e questa riga andrà in conflitto».
> #1099 è atterrata, il conflitto è arrivato **esattamente su questa riga**, e il comando eseguito
> **sull'albero unito** ha risposto **81** = 17 `design/` + 48 `handoff/` + 2 `audit/` + **14 in radice**.
> Entrambi i rami avevano scritto **80**, entrambi misurati col comando buono, entrambi giusti sulla propria
> base. Nessun addendo è stato incrementato a mano.
>
> ⚠️ **I tre file senza riga d'indice restano tre**, e le due sessioni li hanno trovati indipendentemente:
> l'indice della radice risponde **11** contro **14** sul disco. Il sorgente *Tactical Designer* ha la sua
> riga; i tre di `docs_kit` no, per la ragione che il paragrafo qui sopra dichiara e che vale identica da
> entrambi i lati — una riga d'indice dice *cosa quel sorgente ha prodotto*, e indovinarlo sostituirebbe una
> lacuna visibile con una plausibile.
>
> 🔴 **Diciannovesima volta, e la formula ha smesso di avere quattro addendi.** Il bundle `GrayToolkit`
> archivia in una **sottocartella nuova**, `graytoolkit/`, e questo conteggio si era sempre scritto come una
> somma di `design/`, `handoff/`, `audit/` e radice. Non basta più: il comando risponde **90** =
> 17 `design/` + 48 `handoff/` + 2 `audit/` + **4 `graytoolkit/`** + **19 in radice**.
>
> ⚠️ **E lo scarto è tutto mio**, contro quanto la prima stesura di questa nota affermava. Diceva che tre
> file erano «arrivati da altri rami», confrontandosi col valore **83 / 16 in radice** — che era vecchio di
> due revisioni. Il valore vero da cui si parte è **86 / 19**, scritto più avanti in questo
> stesso file: la radice **non si è mossa**, e il delta `+4` è per intero `graytoolkit/`. Inventare un
> contributo altrui in un file che esiste per non farlo è il difetto peggiore che potesse ospitare.
> Trovato in code review.
>
> ⚠️ **E l'ordinale si conta, non si sceglie.** La prima stesura scriveva «ventunesima», saltando due
> numeri; questa è la **diciannovesima**. 🔴 Nel contarli è emerso che **«diciottesima» è rivendicata da due
> note diverse** — un difetto preesistente, che non correggo qui perché appartiene a chi le ha scritte, ma
> che rende il contatore inaffidabile per chiunque lo incrementi a occhio.

> 🔵 **Diciassettesima volta, e questa volta lo scarto era zero: `81` era esatto quando è stato scritto.**
> Il ramo `docs/consolidamento-replay-tactical-designer` ha misurato **81** sulla propria base — che è
> `9a1bd1d4`, cioè `main` **dopo** il merge di #1108 — e archivia **due** sorgenti in radice:
> il consolidamento *Tactical Designer Map/Scenario* e l'handoff *Replay / Canonical Intent*. Il comando
> risponde **83** = 17 `design/` + 48 `handoff/` + 2 `audit/` + **16 in radice**, e la somma degli addendi
> è stata verificata contro il totale ricorsivo con **due comandi diversi**, non dedotta da `81 + 2`.
>
> ⏭️ **La previsione era scritta prima del merge, ed è metà giusta e metà sbagliata — la metà sbagliata
> vale più dell'altra.** Diceva: *«#1104 è aperta e tocca questo file. Non archivia in radice, quindi il
> totale dovrebbe restare **83** anche dopo la sua unione, ma il conflitto su questa riga è **atteso**»*.
> #1104 è atterrata, `git merge origin/main` ha risposto **83** — e **il conflitto non è arrivato**: git
> ha unito i due paragrafi in silenzio, perché toccavano righe adiacenti e non le stesse.
>
> 🔴 **Ed è il caso peggiore, non il migliore.** Quindici volte questa pagina ha imparato dal conflitto:
> il conflitto *costringe* a rimisurare. L'auto-merge no — ha prodotto un documento in cui due paragrafi
> dichiaravano `81` e `83` insieme, e nessun gate lo vede. È stato trovato **diffando il risultato**
> (`git diff <mia-base>..HEAD -- docs/archive/src/README.md`), che è l'unico controllo che regge quando
> il merge è pulito. La lezione di questo giro non è un numero: **un merge senza conflitti non è un merge
> verificato**.
>
> ⚠️ **I tre file senza riga d'indice restano tre**: l'indice della radice risponde **13** contro **16**
> sul disco, come previsto. I due sorgenti di questo giro portano la propria riga; i tre di `docs_kit` no,
> per la ragione che questa pagina dichiara due volte — indovinare cosa un sorgente ha prodotto
> sostituisce una lacuna visibile con una plausibile.
>
> 🔵 **Diciottesima volta, e il conflitto è arrivato — cioè il caso buono, per la ragione che la nota
> qui sopra ha appena finito di argomentare.** Il ramo `docs/consolidamento-multihero-timebank` aveva
> scritto **82** sulla propria base (`94575ef4`), il diciassettesimo giro ne aveva scritti **83** sulla
> propria, ed **entrambi erano giusti**: è la nona lezione, alla lettera, per la terza volta in tre giorni.
> Il valore è stato **rimisurato sull'albero unito**, non sommato: `find` risponde **84**, scomposto in
> `17` `design/` + `48` `handoff/` + `2` `audit/` + **`17` in radice**, con gli addendi contati a parte e
> concordi col totale ricorsivo.
> ⚠️ **I file senza riga d'indice restano i soliti tre**: l'indice della radice risponde **14** contro
> **17** sul disco. Sono `CLAUDE_Apply_Elemental_Proficiency_Consolidation_2026-08-16.md`,
> `CLAUDE_Reconcile_v0.1_Skill_Ability_Issues_2026-08-16.md` e
> `RefactorTactics_v0.1_Characters_Elemental_Consolidation.md`, e appartengono alle track `docs_kit` e
> `reconcile_skill` in `parallel-batch.yaml`: indovinare cosa hanno prodotto sostituirebbe una lacuna
> visibile con una plausibile.
>
> 🔵 **Diciottesima volta, e il valore precedente era esatto: `84` reggeva quando è stato scritto.**
> Il ramo `docs/consolidamento-skill-plus` ha misurato **84** sulla propria base (`d849029e`) e archivia
> **due** sorgenti in radice: l'handoff *Architecture / Process Improvement* e l'handoff *Skill Plus*.
> Il comando risponde **86** = 17 `design/` + 48 `handoff/` + 2 `audit/` + **19 in radice**, con gli
> addendi verificati contro il totale ricorsivo da **due comandi**, non dedotti da `84 + 2`.
>
> ⚠️ **Uno dei due non è stato consumato da chi lo archivia**, ed è la prima volta che succede in questa
> cartella. *Architecture / Process Improvement* è stato riscritto da `docs/technical/piano-riduzione-hotspot.md`
> — branch `docs/piano-riduzione-hotspot`, worktree `D:/rt-simulation` — che dichiara di correggerne
> **sette** affermazioni e chiude con *«se esiste ancora in root, va rimosso»*. 🔴 **Quel branch non era
> pushato al momento dell'archiviazione**: non compare in `git ls-remote` né in una PR, e lo vede solo
> `git worktree list`. La riga d'indice qui sotto lo nomina lo stesso, perché un successore invisibile è
> peggio di un successore altrui.
>
> ⚠️ **I file senza riga d'indice restano i soliti tre**: l'indice risponde **16** contro **19** sul
> disco. I due di questo giro portano la propria riga; i tre di `docs_kit`/`reconcile_skill` no, per la
> ragione che questa pagina dichiara tre volte.
>
> I **90** documenti Markdown in questa cartella sono i **sorgenti** da cui è nata parte della documentazione
> normativa. ➕ **E un file che non è Markdown**: `handoff/2026-08-12-grid-data-consolidation-audit.xlsx`.
> Il comando qui sotto filtra `-name '*.md'` e **non lo conta**: è dichiarato qui perché un allegato invisibile
> alla formula è esattamente il modo in cui questo numero è già andato fuori sincrono sei volte.
> *(⏱️ **Quattordicesima volta, e la previsione ha retto alla lettera.** Questo ramo aveva scritto, *prima* del merge: «`docs/menu-frontend-consolidamento` archivia `handoff/2026-08-16-menu-frontend-tracking.md` nella stessa cartella e nessuno dei due rami vede l'altro — quando atterrano entrambi il valore sarà **76** = 17 + **48** + 2 + 9». #948 è atterrata, `git merge origin/main` ha prodotto il conflitto **esattamente** su questa riga, e il comando eseguito **sull'albero unito** ha risposto **76** = 17 `design/` + 48 `handoff/` + 2 `audit/` + **9 in radice**. Nessun addendo è stato incrementato a mano. Entrambi i rami avevano scritto **75**, entrambi misurati col comando buono, entrambi giusti sulla propria base — la nona lezione, alla lettera, per la seconda volta in tre giorni. L'indice della radice risponde **9**, uguale ai file sul disco. Il valore precedente era **74** = 17 `design/` + 47 `handoff/` + 2 `audit/` + **8 in radice**, dopo l'archiviazione del consolidamento *Decision Time Bank*, dei due sorgenti *mouse interaction*, del consolidamento *roadmap→v1.0*, dell'handoff *Wiki player-first*, dell'handoff *Walls/Doors/InteractionGraph* del consolidamento *4 processi paralleli*, dell'handoff *Worktrees + Shared-ID Allocator* e del consolidamento *Camera Roadmap v1.0*, ciascuno con la sua riga d'indice qui sotto. ➕ **L'indice della radice si controlla a parte** — ``sed -n '/^## Radice/,/^## Nota/p' … | grep -c '^| \[`' `` → **8**, uguale ai file sul disco: è il controllo che la decima volta ha insegnato, e che il totale da solo non offre. ⚠️ **Il valore va rimisurato dopo il merge**, non incrementato: è la lezione che questa riga ha già imparato nove volte, e un `+1` scritto prima dell'unione è giusto solo se nessun altro ramo archivia oggi.
> 🔵 **Decima volta, ma per un motivo nuovo: la formula era giusta e l'indice no.** Le tre colonne tornavano — `17 + 47 + 2 + 2 = 68` — e il disco confermava. Quello che non tornava era il **rapporto fra il totale e la tabella**: la radice conteneva **due** file e ne indicizzava **uno**, con `RefactorTactics_Character_Radar_Wiki_Generator_Claude.md` sul disco e assente dall'elenco. È il difetto di [#579](https://github.com/DegrassiAaron/refactor-tactics-main/issues/579) — righe d'indice mancanti — ma **fuori da `handoff/`**, dove nessuno lo cercava perché la radice sembrava troppo piccola per nasconderne. La riga è stata ricostruita al 2026-08-13 dalle fonti che quel sorgente ha prodotto, ed è marcata come tale. La lezione non è aritmetica: **un totale corretto non dice nulla sull'indice che riassume**, e i due si controllano con due comandi diversi.
> 🔴 **Ottava volta, e stavolta la lezione è doppia perché i rami erano due e avevano ragione entrambi.**
> Due consolidamenti paralleli hanno trovato **lo stesso** off-by-one preesistente da due angoli diversi, e
> nessuno dei due lo aveva introdotto: la riga diceva **64** mentre il disco ne aveva **65** già prima di
> qualunque archiviazione di oggi.
>
> - *Time Bank* l'ha diagnosticato **dal file**: `handoff/2026-08-13-facing-visualdocs.md` è atterrato alle
>   10:34 (`482695fc`) **con** la sua riga nella tabella qui sotto — quindi
>   [#579](https://github.com/DegrassiAaron/refactor-tactics-main/issues/579) non c'entra — e senza toccare
>   questo numero. È il difetto di #579 **ruotato**: là manca la riga e il totale regge, qui regge la riga e
>   manca il totale.
> - *Mouse interaction* l'ha diagnosticato **dalla colonna**: `handoff/` ne conteneva **46** mentre la
>   formula ne dichiarava **45**, verificato con
>   `git ls-tree -r HEAD --name-only docs/archive/src/handoff | grep -c '\.md$'`. Una colonna sbagliata e un
>   totale aritmeticamente coerente con essa — il caso più difficile da vedere, perché la somma torna.
>
> **E i due totali che ne sono usciti sono entrambi falsi dopo l'unione**: `66` da una parte, `67`
> dall'altra, ciascuno giusto sulla propria base. Il valore di questa riga è stato **rimisurato eseguendo il
> comando sull'albero mergiato**, non scegliendo un lato né sommando i delta — che è la nona volta che
> questa cartella impara la stessa cosa. Delle due metà dell'archiviazione se ne fa sempre una sola, e non è
> sempre la stessa: il rimedio non è ricordarsene, è misurare **dopo** ogni `mv` **e dopo ogni merge**.
> 🔁 **Settima volta, e questa è la più pulita: nessuno dei due rami aveva torto.** Il consolidamento
> HexGeometry ha scritto **63** e quello *Focus Decisions* **60**, entrambi misurati col comando buono ed
> entrambi giusti sulla propria base; l'unione ne ha fatti **64**, che non è nessuno dei due e non è la loro
> somma. Il conflitto ha toccato solo questa riga — l'indice dei file si è auto-mergiato in silenzio — quindi
> la riga in conflitto è stata **la fortunata**: è l'unica che ha chiesto di essere guardata. Rimisurata
> eseguendo il comando sull'albero mergiato, non scegliendo un lato né sommando i delta. Prima era 57,
> rimisurato il 2026-08-12 **dopo il merge** — non incrementato a mano: due rami
> lo dichiaravano diverso — «47» da una parte, «48» dall'altra — ed entrambi erano giusti sulla propria base
> e falsi dopo l'unione. Il numero di questa riga si rimisura **dopo** un merge, mai prima. ⚠️ La tabella
> `handoff/` ne elenca comunque meno del misurato: **tre** sorgenti del 2026-08-10 sono sul disco senza una
> riga d'indice — tracciato in [#579](https://github.com/DegrassiAaron/refactor-tactics-main/issues/579) — `2026-08-10-baseaction-signatures-brace-overwatch.md`,
> `2026-08-10-facing-consolidation.md`, `2026-08-10-overwatch-runtime-lifecycle-watch-reposition.md`.
> ➕ **Un quarto caso lo stesso 2026-08-12**: `2026-08-12-teleport-instant-movement.md` era stato archiviato
> **senza riga d'indice** dal consolidamento del mattino, e la lacuna è stata trovata da quello della sera
> archiviando il suo seguito. La riga esiste ora, e il caso resta qui perché è la prova che #579 non è un
> arretrato chiuso: è un difetto che si ripete a ogni archiviazione fatta di corsa.
> 🔴 **E la formula di questa riga era cieca, il che spiega perché il numero sbaglia da sei versioni.**
> Il conteggio si era sempre scritto come `design + handoff + audit`, che **struttura**lmente esclude la
> sezione *Radice* qui sotto — due file. Il consolidamento della sera ha scritto «53» applicando la formula
> vecchia, e la code review l'ha misurato a **55** con
> `find docs/archive/src -name '*.md' ! -name README.md | wc -l`, che è l'unico comando che non ha un punto
> cieco. ⚠️ Uno dei due file di radice — `RefactorTactics_Character_Radar_Wiki_Generator_Claude.md` —
> **non compare in nessuna tabella di questo indice**: è il quinto caso del difetto #579, trovato contando.
> 🔁 **E la dimostrazione è arrivata entro l'ora.** Il reconciliation di roadmap, aperto in parallelo,
> archiviava un sesto sorgente e aveva scritto **53** applicando la formula cieca — indipendentemente, e
> senza vedere questa correzione. Al merge il valore misurato col comando buono è **56**. Le due riscritture
> sono la stessa lezione da due lati: la formula sbagliava *per struttura*, il totale sbagliava *per merge*.)*
> Il primo lotto era in [`../../src/`](../../src/); ogni sorgente si sposta qui quando un owner documentale lo
> ha recepito. Restano per **provenienza**: servono a ricostruire *da dove* è nata una decisione, non a deciderla.
>
> **Il testo originale non è stato riscritto.** Dove un sorgente conteneva un errore di fatto, la correzione è
> una nota `⚠️` accanto all'affermazione, non una modifica del paragrafo.
>
> ➕ **Dal 2026-08-09 la cartella accoglie anche i sorgenti *revisionati e non applicati***: un brief che il
> canone contraddice si archivia con l'esito della revisione in testa, non si scarta. La colonna «Recepito da»
> in quel caso punta al referto, non a un owner — perché non c'è nulla da possedere.
>
> ➕ **Dal 2026-08-10 ci sono anche i dodici sorgenti del pacchetto** `todo/consolidazione-chat-openai/` —
> **sei** master, tre kit e tre documenti *meta* che riguardavano il progetto ChatGPT e non il repository.
> Il pacchetto ne conteneva quattordici: il settimo master,
> `RT_Common_Actions_Master_Consolidation_v0.1.md`, **non è qui** perché non è ancora recepito, e il
> quattordicesimo era il duplicato di un kit già archiviato.
>
> ➕ **Dal 2026-08-11 c'è anche l'handoff Bot/AI** `2026-08-11-bot-ai-team-planner-belief-e-tracking.md`, con
> l'esito della revisione in testa: quattro delle sue premesse di stato erano false al momento in cui è stato
> scritto. Referto: [`../roadmap-plans/bot-ai-consolidamento-2026-08-11.md`](../roadmap-plans/bot-ai-consolidamento-2026-08-11.md).
>
> ⚠️ **Il conteggio era già sbagliato prima del 2026-08-09**: l'intestazione diceva «25» mentre le sue stesse
> tabelle elencavano **26** righe. Il numero qui sopra è **misurato**
> (`ls docs/archive/src/{design,handoff,audit}/*.md | wc -l` → 15 + 32 + 2), non incrementato a mano — che è
> il modo in cui era andato fuori sincrono.
>
> ⚠️ **Ed era andato fuori sincrono di nuovo, esattamente come la nota qui sopra descrive.** Il 2026-08-11
> l'intestazione diceva «40» e la formula «15 + 28 + 2» (cioè 45), mentre i file erano **46**: due numeri
> sbagliati in due modi diversi, nello stesso paragrafo che spiega perché non si contano a mano.
>
> ⚠️ **E il consolidamento che li correggeva ha sbagliato a sua volta, nello stesso paragrafo.** Archiviando
> il secondo handoff ha scritto «49 · 15 + 32 + 2» invece di **48 · 15 + 31 + 2**: il numero è stato
> *incrementato* invece che *misurato*, che è la cosa precisa contro cui questo riquadro mette in guardia.
> Preso in code review. La lezione non regge da sola — la formula c'era, e nessuno l'ha eseguita — quindi
> vale la riga operativa: **esegui il comando, non aggiungere uno.** Rimisurato il 2026-08-11.
>
> 🔁 **Il 2026-08-12 «49 · 15 + 32 + 2» è diventato il valore giusto** — e non perché la nota qui sopra
> sbagliasse: un merge ha portato un handoff in più, e la misura è cambiata sotto una riga che restava ferma.
> È lo stesso difetto visto da un'altra angolazione: due rami dichiaravano «47» e «48», **entrambi corretti
> sulla propria base e falsi dopo l'unione**. Un numero che era vero quando è stato scritto non resta vero;
> va rimisurato **dopo il merge**, come dice il registro delle verifiche PIE per il proprio conteggio.

**Se cerchi la regola, non sei nel posto giusto**: la colonna «Recepito da» dice chi la possiede oggi.
In caso di conflitto prevalgono [`../../product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md),
[`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) e gli ADR.

## ⚠️ Archiviare un documento: il gate dei nomi ora guarda anche qui

Dal 2026-08-17 (`#756`, D-130) `scripts/check-docs-naming.py` **non esclude più `archive/`**. La
copertura è 382 file su 382, e archiviare un documento che nomina il roster con i nomi ritirati
— `Flux`, `Riva`, `Bastion`, `Vektor` — **fa fallire il gate**.

Non è un effetto collaterale: è come si impedisce che l'archivio torni a riempirsi. Prima di
questa fetta conteneva **1195 occorrenze in 54 file**, cresciute di 88 nei quattro giorni in cui
la issue è rimasta aperta — perché l'archivio è dove i documenti vanno quando finiscono, e ognuno
arriva coi nomi che il codice aveva quando è stato scritto.

**Due modi di chiudere il rosso, e vanno distinti:**

1. **Il nome è un residuo** → si sostituisce, e il file prende in testa la riga di provenienza che
   i 54 già portano: *«Nomi del roster sostituiti il … per D-130 … le issue citate usano ancora i
   nomi precedenti»*. Non ripristina la provenienza, ma dice a chi legge cosa cercare.
2. **Il nome è il soggetto della frase** — «rinominato da …», o la mappatura stessa → si marca la
   riga, in fondo se è dentro una tabella:

   ```md
   <!-- rename-exempt: la riga dichiara la mappatura -->
   La decisione mappa `Flux` su **Gadget**.
   ```

   Il marcatore esenta **una riga**, e fallisce da sé quando la sua ragione cade: un marcatore su
   una riga senza nomi ritirati è un errore, non un'esenzione dimenticata.

   > 🔎 L'esempio qui sopra è **un marcatore vero**, non una finzione tipografica — e non poteva
   > essere altro: la prima stesura di questa nota scriveva `rename-exempt: <ragione>` come
   > segnaposto, e il gate l'ha subito segnalato **stantio**, perché la riga dopo non conteneva
   > nomi ritirati. Un meccanismo che legge il testo grezzo non distingue la documentazione di sé
   > stesso dal proprio uso, quindi l'unico esempio che regge è uno che funziona davvero.

🔴 **Ciò che NON si fa è aggiungere una cartella a `EXCLUDED_DIRS`.** Era il meccanismo di prima, e
il suo costo si è visto quando è stato misurato: 89 file esenti su 240, copertura 63%, e nel
Decision Log erano esenti anche le voci **nuove**, quelle che descrivono il roster corrente.

## `design/` — specifiche per sistema

| File | Sistema | Recepito da |
|---|---|---|
| [`overwatch-e-fast-reaction.md`](design/overwatch-e-fast-reaction.md) | Overwatch, Fast Action, Fast Reaction | ADR-0004, [`brief-overwatch-reazioni.md`](../../gameplay/brief-overwatch-reazioni.md), piano canonico |
| [`action-ghosts-fasi-fast-reactions.md`](design/action-ghosts-fasi-fast-reactions.md) | Ghost di azione, ordine fasi, `Facing` | ADR-0005, [`brief-planning-visuale.md`](../../technical/brief-planning-visuale.md) |
| [`rumore-e-percezione-acustica.md`](design/rumore-e-percezione-acustica.md) | Rumore, percezione acustica, fog of war | [`brief-conoscenza-parziale.md`](../../gameplay/brief-conoscenza-parziale.md) + roadmap v0.1 |
| [`delayed-actions-e-phase-windows.md`](design/delayed-actions-e-phase-windows.md) | Delayed actions, phase boundaries | [`brief-delayed-actions.md`](../../gameplay/brief-delayed-actions.md) |
| [`terreno-ghiaccio-v0.1.md`](design/terreno-ghiaccio-v0.1.md) | Terreno ghiaccio in UE5 | [`brief-ghiaccio.md`](../../gameplay/brief-ghiaccio.md) |
| [`auxiliary-units.md`](design/auxiliary-units.md) | Pet, evocazioni, droni, torrette | [`brief-unita-ausiliarie.md`](../../gameplay/brief-unita-ausiliarie.md) |
| [`azioni-generiche-overwatch-universale-v0.1.md`](design/azioni-generiche-overwatch-universale-v0.1.md) | Azioni generiche, Overwatch universale | [`brief-azioni-generiche-overwatch.md`](../../gameplay/brief-azioni-generiche-overwatch.md) |
| [`predictive-actions-e-trappole.md`](design/predictive-actions-e-trappole.md) | Azioni predittive, trappole, gambit | [`brief-delayed-actions.md`](../../gameplay/brief-delayed-actions.md) |
| [`fazioni-v0.2-identita-visiva-e-roster.md`](design/fazioni-v0.2-identita-visiva-e-roster.md) | Fazioni, identità visiva, cooperazione | D-029 / ADR-0006 |
| [`match-timing-e-scala-mappe.md`](design/match-timing-e-scala-mappe.md) | Durata partita, round budget, scala mappe | [`spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md) · D-030 · **E19** |
| [`2026-08-08-hud-faction-icons.md`](design/2026-08-08-hud-faction-icons.md) | Icone fazioni, HUD icon language | D-031 · **E20** · immagini in [`../../src/design/hud/`](../../src/design/hud/) |
| [`2026-08-08-roster-8-conflux-constrine.md`](design/2026-08-08-roster-8-conflux-constrine.md) | Roster 8, Conflux e Constrine | [`Fazioni` (Wiki)](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/Fazioni) + [`../../characters/v0.2/`](../../characters/v0.2/) · runtime in **E35** *(era `E21`, rinumerata il 2026-08-09: [D-039](../../decisions/RT_PDR_00_Decision_Log.md))* |
| [`2026-08-08-cover-window-open-fire-seal.md`](design/2026-08-08-cover-window-open-fire-seal.md) | Cover Window, Open → Fire → Seal | 📅 **E22** (v0.2), con i 12 scenari di test |
| [`2026-08-08-muri-porte-e-interazioni.md`](design/2026-08-08-muri-porte-e-interazioni.md) | Muri, porte, interazioni, validazione | 📅 **E23** (v0.2) |
| [`trasformazioni-e-stati-personaggio.md`](design/trasformazioni-e-stati-personaggio.md) | Trasformazioni, stance, stati del personaggio | [`brief-stati-personaggio-e-trasformazioni.md`](../../gameplay/brief-stati-personaggio-e-trasformazioni.md) · D-035 · 📅 **E34** |
| [`2026-08-13-mouse-world-ui-interaction.md`](design/2026-08-13-mouse-world-ui-interaction.md) | Hover / LMB / RMB, target mode, priorità semantica | ⚠️ **proponeva un secondo owner**, non adottato: la superficie era già di [`spec-pointer-interaction.md`](../../technical/spec-pointer-interaction.md) (CP 11.8). Contenuto recepito in §2.1, §4.1, §5.4–§5.6, §6.4–§6.5 · [D-128](../../decisions/RT_PDR_00_Decision_Log.md) · [#737](https://github.com/DegrassiAaron/refactor-tactics-main/issues/737) |
| [`2026-08-13-mouse-interaction-integration-plan.md`](design/2026-08-13-mouse-interaction-integration-plan.md) | Piano di integrazione del precedente: issue, cross-link, PIE, roadmap MI-0…MI-6 | 🔄 **applicato in parte** — la tabella degli esiti è nel banner del file. Scartati: issue ausiliaria (era #705), test `UI.Mouse.*` (esiste `PlayerInput.*`), `PIE-V01-MOUSE-INTERACTION` (esiste `PIE-V01-POINTER`) |

## `handoff/` — task esecutivi

| File | Oggetto | Recepito da |
|---|---|---|
| [`consolidamento-prd-source-of-truth.md`](handoff/consolidamento-prd-source-of-truth.md) | Consolidare PRD e source of truth | [`brief-consolidamento-documentale.md`](../../roadmap/plans/brief-consolidamento-documentale.md) |
| [`scenario-browser-bp-gamemode.md`](handoff/scenario-browser-bp-gamemode.md) | Selettore scenari in `BP_GameMode` | [`scenario-index-e-tag.md`](../../technical/scenario-index-e-tag.md) |
| [`scenario-harness-task-originale.md`](handoff/scenario-harness-task-originale.md) | Task originale dello Scenario Test Harness | [`test-automatico-unreal.md`](../../technical/test-automatico-unreal.md) |
| [`roadmap-v0.1-prompt-originale.md`](handoff/roadmap-v0.1-prompt-originale.md) | Prompt da cui è nata la roadmap v0.1 | ADR-0003 |
| [`roadmap-docs-test-e-showcase-v0.1.md`](handoff/roadmap-docs-test-e-showcase-v0.1.md) | Consolidamento roadmap/test/showcase v0.1 | [`showcase-v01-audit.md`](../../roadmap/plans/showcase-v01-audit.md) |
| [`2026-08-07-nuove-decisioni-e-scenario-4v4.md`](handoff/2026-08-07-nuove-decisioni-e-scenario-4v4.md) | Nuove decisioni, scenario 4v4, roadmap | decisioni §3 già canone · scenario → **E17** / **E32** |
| [`2026-08-08-bot-ai-roadmap-e-test-pie.md`](handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md) | Bot AI tattica, test PIE, scenari | `PIE-AI-01…05` · [`avversario-bot.md`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/avversario-bot) · **E26**/**E28** |
| [`2026-08-08-tre-aggiunte-signature-mechanics.md`](handoff/2026-08-08-tre-aggiunte-signature-mechanics.md) | ConditionalIntent, GenericActionModifier, Misplay | D-032 · D-033 · D-034 — vedi il banner in testa al file: **una sola** delle tre era davvero assente |
| [`2026-08-08-azioni-base-e-facing.md`](handoff/2026-08-08-azioni-base-e-facing.md) | Azioni base e facing: consolidamento | [ADR-0005](../../decisions/adr-0005-orientamento.md) copriva già il canone. Restano tre **proposte di modifica** (righe 50–52 della conflict matrix) e `FAC-4…FAC-10` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| [`2026-08-09-attacco-base-per-eroe.md`](handoff/2026-08-09-attacco-base-per-eroe.md) | Profili di attacco base per eroe | [ADR-0007](../../decisions/adr-0007-attacco-base-per-eroe.md) · il documento porta **inline** le sezioni corrette (§9, §10–13, §15-bis, §24, §27, §28): tre valori su quattro della matrice originale contraddicevano il catalogo, e i nomi candidati collidevano con azioni gia' spedite |
| [`2026-08-09-map-editor-roadmap.md`](handoff/2026-08-09-map-editor-roadmap.md) | Roadmap e consolidamento del Map Editor (v0.1–v0.5) | ⛔ **Revisionato e non applicato** — [`map-editor-brief-spec-panel-2026-08-09.md`](../../roadmap/plans/map-editor-brief-spec-panel-2026-08-09.md). 9 duplicati, 5 conflitti (muri vs **E23.1**, porte, terreni, profili di movimento). Sopravvive **una** proposta: la sonda di movimento nell'editor |
| [`2026-08-10-wait-guard-brace-overwatch-e-geometria.md`](handoff/2026-08-10-wait-guard-brace-overwatch-e-geometria.md) | Wait/Guard/Brace/Overwatch, facing e geometria muri/hex | [`handoff-geometry-reazioni-conflict-report-2026-08-10.md`](../../roadmap/plans/handoff-geometry-reazioni-conflict-report-2026-08-10.md) · [D-065](../../decisions/RT_PDR_00_Decision_Log.md) (geometria → **E23.6/23.7**) · [D-066](../../decisions/RT_PDR_00_Decision_Log.md) (Guard/Brace **non** applicata → `BAL-1`). Su 45 righe di triage **18 erano già canone**: la §7 «decisione canonica da consolidare» risolveva un problema che il repository non aveva |
| [`2026-08-10-status-control-brace-overwatch.md`](handoff/2026-08-10-status-control-brace-overwatch.md) | Status, buff/debuff, control, Brace e Overwatch | [`handoff-status-control-triage-2026-08-10.md`](../../roadmap/plans/handoff-status-control-triage-2026-08-10.md) · **Quarto** sorgente del 2026-08-10 sullo stesso perimetro. La meta' su Brace/Overwatch era gia' decisa (catena #390 → #394 → #397) e proponeva **tre nomi gia' presi**, uno respinto il giorno prima (`Reposition` → `Withdraw`, `D-067`). La meta' sugli status ha contenuto: `RT-FEAT-STATUS-FRAMEWORK`, **DESIGNED**, e da qui l'epic **E36** (v0.2, sei checkpoint). `STA-1` e `STA-2` chiuse da `D-072` — primitive e severity si **derivano** dal dato — che ha pero' aperto `STA-4`, la tassonomia delle capability, prerequisito di entrambe. Restano aperte `STA-3` e `STA-4` |
| [`2026-08-10-full-grid-geometry-walls-water.md`](handoff/2026-08-10-full-grid-geometry-walls-water.md) | Griglia, geometria, muri, cover, traversal, strutture, acqua ed elettricita' | [`triage-grid-geometry-water-2026-08-10.md`](../../roadmap/plans/triage-grid-geometry-water-2026-08-10.md) · **3159 righe, 55+ sezioni `LOCKED`** — il piu' grande della serie. Un conflitto sulla soglia di calpestabilita', **risolto a favore di [D-071](../../decisions/RT_PDR_00_Decision_Log.md)**; la sua «ultima decisione prima della pausa» (§53, elettricita' sulla rete d'acqua) era **gia' implementata e testata** da CP 8.3. Entrano tre feature `IDEA`: acqua dinamica, strutture, verticalita' · `GEO-1`…`GEO-3` |
| [`2026-08-11-five-lane-roadmap-editor-replay.md`](handoff/2026-08-11-five-lane-roadmap-editor-replay.md) | Roadmap a 5 lane: Spatial/Simulation/Client + Editor/Tooling + Replay/Audit | ⛔ **Revisionato e non applicato** — [`five-lane-roadmap-spec-panel-2026-08-11.md`](../../roadmap/plans/five-lane-roadmap-spec-panel-2026-08-11.md). La premessa non regge: la «roadmap a 3 lane» che dichiara di estendere **non esiste** (zero occorrenze in `docs/`). **45 dei 51 path** che assegna alle lane non esistono — il modulo runtime non ha lo split `Public/`/`Private/` e il content root è `Content/RT/`; tutti e **11 i gate `G0`–`G10`** collidono con `G1`–`G15` già in uso, tre *quasi* con lo stesso significato. Il dominio replay è già chiuso da `D-077`/`D-078`/`D-083` con **16 test**. Sopravvivono **tre** proposte: livello `DoD Replay` (§23), checklist di gate a cinque caselle (§25), classificazione dati replay per la v0.2 (§29) |
| [`2026-08-12-map-sketch-editor.md`](handoff/2026-08-12-map-sketch-editor.md) | Map Sketch Editor v0.1: griglia visibile, geometria quantizzata, occupancy a 12 settori | [`map-sketch-editor-spec-panel-2026-08-12.md`](../../roadmap/plans/map-sketch-editor-spec-panel-2026-08-12.md) · **Applicato in parte**. **Terzo** prompt map-editor: la sua tesi centrale (§3, muri non vincolati ai lati) aveva già un verdetto — collocata in **E23.1**, v0.2. Ciò che lo distingue: la §4 (dodici settori) **risolve** l'obiezione che aveva fermato il predecessore, cioè i float nell'hash. Su 32 voci, **16 hanno già un padrone**. Anticipato in v0.1 come tooling per decisione dell'autore (`#619`…`#621`, anticipazione dichiarata su **E23**/`#324`, che **non** si apre) · `#622` `#623` · `MSE-1`. Respinti: le priorità `P1.1`…`P2.5` (quarto asse), la roadmap editor a mano di §30 (`roadmap-editor.md` è `HISTORICAL` **proprio** per quello), il `UDeveloperSettings` di §15, gli «scenari» `MapSketch_*` (sono classe **C**) |
| [`2026-08-12-action-economy-movement-facing.md`](handoff/2026-08-12-action-economy-movement-facing.md) | Economia delle azioni, accoppiamento col movimento, costi del facing | [`spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md) · **E38** (v0.2) · `AE-1`…`AE-7` · [referto](../roadmap-plans/action-economy-consolidamento-2026-08-12.md). ⚠️ **Recepito in parte**: §6/§7/§8 sono l'unico contributo nuovo; §4/§5 contraddicono il modello a slot di `D-028`, §15 contraddice ADR-0008 ed era gia' `FAC-12`, §30 e' respinta da `balance/README.md` |
| [`2026-08-12-teleport-instant-movement.md`](handoff/2026-08-12-teleport-instant-movement.md) | Teletrasporto e movimenti istantanei: famiglia, resolver, scenari | [`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) · [referto](../roadmap-plans/teleport-instant-movement-2026-08-12.md). **Tesi giusta, premessa falsa**: «un movimento veloce non è un teletrasporto» è già canone, ma la premessa che nel repository esista solo il primo è **falsa** — `ERTMovementStyle::LinearLeap` fa `Result.Entered = { destinazione }`. Sei scenari proposti → **uno**, e **esisteva già**. Ha prodotto `MOV-1`/`MOV-2` e [#645](https://github.com/DegrassiAaron/refactor-tactics-main/issues/645). ⚠️ **La riga d'indice mancava**: aggiunta il 2026-08-12 dal consolidamento successivo, stesso difetto di [#579](https://github.com/DegrassiAaron/refactor-tactics-main/issues/579) |
| [`2026-08-12-spatial-transfer-epic.md`](handoff/2026-08-12-spatial-transfer-epic.md) | Spatial Transfer: epic, tredici checkpoint, resolver puro, Blink, Swap, Recall, Portal | [`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) · **E39** (v0.2) · `D-118` `D-119` · [referto](../roadmap-plans/spatial-transfer-epic-2026-08-12.md). **Seguito del precedente, e accurato**: nove stati di issue su nove verificati, l'enum, lo scenario e il numero di epic libero. ✂️ **13 checkpoint → 4 issue** ([#700](https://github.com/DegrassiAaron/refactor-tactics-main/issues/700) [#701](https://github.com/DegrassiAaron/refactor-tactics-main/issues/701) [#702](https://github.com/DegrassiAaron/refactor-tactics-main/issues/702) [#703](https://github.com/DegrassiAaron/refactor-tactics-main/issues/703)) + 1 già chiusa + 8 rinviate. ⚠️ La DoD del §8 era una lista di sostantivi; il §15 (nomi di test) non è dichiarabile nel registry; e **nessuna sezione dice quale eroe** |
| [`2026-08-12-mapeditor-roadmap-issue-integration.md`](handoff/2026-08-12-mapeditor-roadmap-issue-integration.md) | Map Editor: audit delle tredici issue, roadmap `R0`–`R8`, percorso critico e DoD dell'Editor v0.1 | [referto](../../roadmap/plans/mapeditor-integration-spec-panel-2026-08-12.md) · **Applicato in parte**. **Quarto** prompt map-editor, e il primo che non ricostruisce l'editor: zero duplicati di tool, camera o toolbar. ⏱️ **Il fatto centrale è scaduto sedici minuti dopo la scrittura**: dà `#554` OPEN e chiede di recuperare la PR `#694`; il documento è timbrato `20:10 CEST` e `#707` è mergiata alle `18:26:29Z` = `20:26:29 CEST`. Era **vero alla scrittura**, falso al consumo (~2h dopo) — cinque voci (`S1`…`S5`) ne discendono. È il caso più puro di *fotografia datata* dell'archivio: nessun controllo dell'autore lo avrebbe evitato, il controllo spetta a chi consuma. 🔴 La prima stesura del referto aveva invertito il verso confrontando un `mergedAt` **UTC** con un timbro **CEST**. ✅ Il suo §R6 **chiude `P7`**, la sonda di movimento che il panel 2026-08-09 aveva proposto e nessuno aveva aperto: → [#711](https://github.com/DegrassiAaron/refactor-tactics-main/issues/711). Sopravvive anche il Geometry Authoring Tool → [#712](https://github.com/DegrassiAaron/refactor-tactics-main/issues/712). Respinti: `M9 Integration Gate` (il wiring è la condizione di chiusura di **ogni** issue, non un passo finale) e l'elenco DoD di §7 (già derivabile dai gate + voci `PIE-*`) |
| [`2026-08-12-roadmap-reconciliation.md`](handoff/2026-08-12-roadmap-reconciliation.md) | Riallineamento di roadmap, Feature Registry ed Epic/issue GitHub prima di proseguire la v0.1; contratto del puntatore mancante | [`../roadmap-plans/roadmap-reconciliation-2026-08-12.md`](../roadmap-plans/roadmap-reconciliation-2026-08-12.md) · owner nato: [`../../technical/spec-pointer-interaction.md`](../../technical/spec-pointer-interaction.md) (**CP 11.8**, [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705)) · **Applicato in parte**. ✅ Il suo contributo vero è la §1: la catena `#159 → #165` **non regge** e le lane sono parallele — verificato su `#160`, che lo dichiara in proprio. ⚠️ **Quattro premesse fuori data**, e il documento lo aveva previsto: `#152` era stale come **issue**, non come roadmap; il bot partial-knowledge era una **contraddizione interna** alla §2; la §7 tratta il contratto del puntatore come da progettare mentre `RMB` è **già** `UndoAction` e l'hover è **già** presentazione. ❌ Respinte le cinque epic §10 (Super Actions, Modular Effects, Seeded Map, Level Designer, Networking): il documento stesso le marca **PROPOSTE** |
| [`2026-08-12-v01-focus-decisions-naming.md`](handoff/2026-08-12-v01-focus-decisions-naming.md) | Naming canonico del roster v0.1, confine Guard/Brace, profili Overwatch, rumore per azione, ceiling E21 | [D-120…D-124](../../decisions/RT_PDR_00_Decision_Log.md) · [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) (`BAL-1`, `BAS-2` chiuse) · [`../../roadmap/roadmap-v0.1.md`](../../roadmap/roadmap-v0.1.md) · [`../../characters/index.md`](../../characters/index.md) · **Applicato con due correzioni di merito**. 🔴 Il pacchetto assumeva che non rinominare gli Stable ID bastasse a rendere coerente la UI: il nome a schermo è **derivato** da `ARTUnit::ShortHeroName`, quindi il prodotto mostra ancora i nomi storici ([#715](https://github.com/DegrassiAaron/refactor-tactics-main/issues/715)). 🔴 E supera D-037 senza rispondere alla sua prova — `Gadget` è già una categoria di equipaggiamento, e la collisione diventa il blocker della migrazione ([#716](https://github.com/DegrassiAaron/refactor-tactics-main/issues/716)). ⚠️ Base dichiarata `79b9d891`, `origin/main` reale al consumo `05bbe3dc`: gli ID sono stati rimisurati liberi, non ereditati |
| [`2026-08-12-level-designer-01-context.md`](handoff/2026-08-12-level-designer-01-context.md) | Level Designer: contesto, stato dei tool dell'editor, principi anti-duplicazione, casi d'uso e divieti | [referto](../../roadmap/plans/level-designer-handoff-spec-panel-2026-08-12.md) · **Applicato in parte** (10 blocchi `🔎 PANEL` inline). **Quinto** prompt map-editor, e il meglio orientato della serie: la tesi «non costruire un secondo editor, completa quello che c'è» regge sul codice, e i quattro tool di §4 sono confermati (`RTHexEditorMode.cpp:29-32`). ⚠️ Predica «non duplicare» e poi mantiene **a mano** la matrice di stato di §5, che duplica e contraddice [`featuremap.shortlist.md`](../../roadmap/featuremap.shortlist.md) — generata — senza una sola colonna di evidenza. Respinti gli oracoli in aggettivi delle §10-11 («chiaramente ghost», «no zone quasi nere»): una seduta senza condizione di pass produce un ✅ che significa «l'ho guardato» |
| [`2026-08-12-level-designer-02-implementazione-consolidamento.md`](handoff/2026-08-12-level-designer-02-implementazione-consolidamento.md) | Level Designer: piano d'implementazione, grammatica delle direttrici, suite di test, sequenza di commit | stesso [referto](../../roadmap/plans/level-designer-handoff-spec-panel-2026-08-12.md) · **Applicato in parte** (7 blocchi `🔎 PANEL` più una §10-bis nuova su formato, hash e migrazione). ✅ **Da preservare intatti** il §29 (referto obbligatorio con `WHAT WAS REUSED`) e il §32 (`Runtime Rule → Pure query → Editor visualization`): sono i due meccanismi che davvero prevengono la duplicazione. ⚠️ Il §25 **pianifica un commit** — `feat(map): add edge center/orientation helper` — per una funzione che esiste, è testata e ha già consumatori (`EdgeMidpointWorld` `RTHexLibrary.h:90`, `EdgeRotation` `:100`); e due delle sue domande aperte avevano già risposta nel repository, `MSE-1` inclusa. Ha prodotto `MSE-2` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) e due test `HexOccupancy.*` |
| [`2026-08-13-hexgeometry-editor-implementation-brief.md`](handoff/2026-08-13-hexgeometry-editor-implementation-brief.md) | Hex grid, geometry authoring, bake e Map Editor: modello completo, audit obbligatorio, sequenza di implementazione #620→#711 | [referto](../../roadmap/plans/hexgeometry-editor-spec-panel-2026-08-13.md) · **Applicato in parte**. ✅ **Il più accurato della serie**: 29 verifiche di stato su 29 corrette — dieci issue, sette simboli, otto feature ID, quattro decisioni — primo handoff senza una premessa falsa. Il suo §27 ha prodotto l'owner mancante [`spec-hex-geometry-authoring.md`](../../technical/spec-hex-geometry-authoring.md); §45 e §26 ci sono entrati quasi intatti. 🔴 **Il §12 è superato e sta nel percorso critico**: prescrive `void/cliff → ERTHexSurface::Void`, respinto il 2026-08-12 (*«il bake non scrive Surface»*), ed è citato dal §32 Step B come DoD di `#621`. ⚠️ Il §7 dà le soglie di occupancy come stabili senza sapere di `MSE-2`. Ha prodotto `MSE-3` |
| [`2026-08-12-grid-consolidation-summary.md`](handoff/2026-08-12-grid-consolidation-summary.md) | Sintesi del focus griglia, muri, acqua/elettricità e Planning HUD: stato vs futuro, governance source-vs-generated | stesso [referto](../../roadmap/plans/hexgeometry-editor-spec-panel-2026-08-13.md) · **Recepito come conferma, non come delta**. La sua sezione «già presente nel repository» è corretta voce per voce, e la parte di governance (feature source, editor source, shortlist generate, workbook `RESEARCH`) coincide con `D-023`/`D-076` già in vigore |
| [`2026-08-12-grid-consolidation-apply-guide.md`](handoff/2026-08-12-grid-consolidation-apply-guide.md) | Guida d'applicazione del bundle: preflight, root files, spec proposte, registry, maps, wiki, Excel, issue, gate | idem · **Applicato in parte**. ✅ I **gate** che propone al §11 erano già verdi e nessuno li aveva misurati: `WaterDepth` 0 nel codice, `BreachSlot` 0, `TransitOnly` 0. ⚠️ Il §2 chiede di sostituire `CLAUDE.md`, `AGENTS.md` e `README.md` con «replacement» che **il bundle non contiene** |
| [`2026-08-12-grid-roadmap-geometry-water-planning.md`](handoff/2026-08-12-grid-roadmap-geometry-water-planning.md) | Roadmap `R0`–`R11` su geometry, movement, verticalità, strutture, acqua/elettricità, intel e Planning UI | idem · ⛔ **Respinto come roadmap, archiviato come inventario di design**. Venticinque work item numerati che duplicano il Feature Registry — l'errore che il brief HexGeometry elenca al proprio §40 — su otto feature che **esistono già**; il §16 introduce un **terzo** vocabolario di milestone. ✅ Il §1 (quindici vincoli da non riaprire) è verificato e corretto. ⚠️ I sette scenari `Spec.Environment.*` descrivono capability inesistenti: restano `planned`. ➕ Il workbook allegato `2026-08-12-grid-data-consolidation-audit.xlsx` è un audit di **governance dei dati**, non una fonte di gameplay: i suoi due fogli confermano `D-023` |
| [`2026-08-13-facing-visualdocs.md`](handoff/2026-08-13-facing-visualdocs.md) | Facing: consolidamento di documentazione, Wiki, roadmap, issue/epic, scenario map e **sette diagrammi** | [`../../roadmap/plans/facing-visualdocs-triage-2026-08-13.md`](../../roadmap/plans/facing-visualdocs-triage-2026-08-13.md) · decisione nata: **D-126** (`FAC-11`) · **Recepito in parte**. ⚠️ La richiesta centrale è stata accolta **nel verso opposto** a quello implicito: i sei lati diventano la primitiva *semantica*, ma `HexCone` **non** viene sostituito nei consumatori — misurato, il cono è **strettamente contenuto** nell'insieme dei tre lati (**50** celle di divergenza su raggio `1..10`, **zero** nel verso opposto), quindi sostituirlo sarebbe un **buff difensivo**. 🔴 **Due asset arrivavano col nome scambiato** (`F4`↔`F6`) e i sette hash corrispondevano **tutti** al manifest: l'audit per hash prescritto dal pacchetto dà verde, perché a mentire erano i nomi. ⚠️ **F3** disegna come canone il pannello della direzione d'impatto, che è `FAC-13` **aperta**. ✂️ 5 scenari proposti → **1** creato; **0** gate del registry cambiati; nessuna seconda epic Facing |
| [`2026-08-16-menu-frontend-tracking.md`](handoff/2026-08-16-menu-frontend-tracking.md) | Frontend/menu: albero delle schermate, feature v0.1, roadmap menu v0.1→v1.0, epic/issue e tracking | [referto](../../roadmap/plans/menu-frontend-spec-panel-2026-08-16.md) · decisione nata: **D-144** · **Recepito in parte**. ✅ **Il primo handoff della serie che non duplica niente**, verificato in cinque punti: zero `WBP_*` in `Content/`, nove file UI tutti in-match, nessuna feature né epic di frontend, zero occorrenze in `roadmap-post-v0.1.md`. ✅ **Il suo contributo migliore è uno che non rivendica**: chiede un menu avviabile in packaged senza sapere che il gate **G13** è 🟡 esattamente per quello — ed è l'unico argomento che ha fatto entrare **E46** in v0.1. 🔴 **Lo scope del §26 non regge**: mette in v0.1 `P0` quattro sezioni che il documento stesso qualifica DEV/TEST e che il registry ha già come `out_of_release_scope`. ⏱️ **Un secondo argomento contro le §5–§8 è caduto lo stesso giorno**: il referto le dava ineseguibili perché `Scenarios/` non era staged nel pacchetto (`#926`), e [`#935`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/935) ha chiuso quella causa. Restano fuori per il solo motivo sopra. ⚠️ Il §15 collide col naming `WBP_RT_` deciso a CP 11.7; il §18 mette il networking in v0.7 dov'è **v0.5** (E40); il §22 apre un terzo vocabolario di tracking di cui sopravvivono **2 campi su 11**. ✂️ 6 epic proposte → **1** creata; 14 feature → **4** |

### Il pacchetto `consolidazione-chat-openai` — dodici sorgenti, un solo triage

Sei master, tre kit di dettaglio e tre *meta* — dodici righe, contate sulla tabella. Il referto comune è
[`consolidamento-chat-openai-triage-2026-08-09.md`](../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md):
classifica ogni affermazione contro il canone **prima** che qualcuno la applichi, e la colonna qui sotto dice
dove è finita quella parte che è sopravvissuta al filtro.

| File | Oggetto | Recepito da |
|---|---|---|
| [`2026-08-08-master-mappa-e-ambiente.md`](handoff/2026-08-08-master-mappa-e-ambiente.md) | Mappa, ambiente, propagazione, interazioni | [`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) — E10 · CP 10.1 |
| [`elementi-interattivi-della-mappa.md`](handoff/elementi-interattivi-della-mappa.md) | Catalogo degli elementi interattivi (kit del precedente) | idem, §2–§13. Aperte `INT-1`, `INT-2`, `INT-4` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| [`2026-08-08-master-ui-ux.md`](handoff/2026-08-08-master-ui-ux.md) | UI / UX, pannelli, leggibilità | [`progettazione-hud.md`](../../technical/progettazione-hud.md) · E11 / E20 — su più punti il repository era **più avanti della fonte** |
| [`hud-consolidation-kit.md`](handoff/hud-consolidation-kit.md) | HUD (kit del precedente, e più vecchio) | ⛔ tre dei sette conflitti del pacchetto nascono qui: `TEAM READY`, «Fog of War», eleggibilità per nome d'eroe |
| [`2026-08-08-master-governance.md`](handoff/2026-08-08-master-governance.md) | Governance, Feature Registry, roadmap | I **nove gate coincidono alla lettera** con [`feature-registry.yaml`](../../roadmap/feature-registry.yaml). Il vocabolario di status no: 13 contro 10 derivati |
| [`2026-08-08-master-scenari-qa-e-bot.md`](handoff/2026-08-08-master-scenari-qa-e-bot.md) | Scenari, QA, bot | Il più assorbito: [`test-automatico-unreal.md`](../../technical/test-automatico-unreal.md) · [`scenario-index-e-tag.md`](../../technical/scenario-index-e-tag.md) · il bot era già recepito il 2026-08-08 |
| [`2026-08-08-master-characters-e-roster.md`](handoff/2026-08-08-master-characters-e-roster.md) | Personaggi, roster, fazioni, Super | [ADR-0007](../../decisions/adr-0007-attacco-base-per-eroe.md) per l'attacco base; il residuo in [`brief-super-e-cooldown.md`](../../gameplay/brief-super-e-cooldown.md) — issue `#336`, PR `#349` |
| [`2026-08-08-master-reaction-system.md`](handoff/2026-08-08-master-reaction-system.md) | Cluster Reaction | PR `#305` — `D-047`, `D-048`, `D-049`, CP 14.7. Era già assorbito quando il triage è iniziato |
| [`2026-08-09-decision-time-bank.md`](handoff/2026-08-09-decision-time-bank.md) | Decision / Reaction Time Bank | [`spec-decision-time-bank.md`](../../gameplay/spec-decision-time-bank.md) (CP 14.8) · `D-050`…`D-057` · [conflict report](../../roadmap/plans/decision-time-bank-conflict-report-2026-08-09.md) |
| [`2026-08-13-decision-time-bank-consolidamento.md`](handoff/2026-08-13-decision-time-bank-consolidamento.md) | Seguito del precedente: stato reale di CP 14.8 contro il repository — issue, feature, decisioni, scenari, TurnLog | **Recepito come conferma, non come delta**. Ogni affermazione di stato ricontrollata è risultata corretta e il documento non ha prodotto né decisioni né feature né epic: `#319` resta l'unico lavoro, dopo `#165`→`#166`→`#314`. ⚠️ Dichiarava un `HEAD` vecchio di **17 commit** (`744a25b8` contro `0cff74ec`), senza che questo invalidasse nulla. 🔴 **Il valore è stato ciò che non diceva**: affermava `#318`/`#361` chiuse e si fermava lì, mentre cinque righe in quattro file `CURRENT` più il «prerequisito bloccante» di `#319` dichiaravano ancora assente una capability consegnata il 2026-08-10 (`a7e4677b`), con i conteggi `otto`/`undici` superati da **dieci**/**tredici**. Corretti qui; il gate è [#738](https://github.com/DegrassiAaron/refactor-tactics-main/issues/738) |
| [`2026-08-08-chat-cleanup-tracker.md`](handoff/2026-08-08-chat-cleanup-tracker.md) | *Meta* — tracker del progetto ChatGPT | ⛔ nessun owner: **sette dei nove «conflitti aperti» che elenca erano già chiusi**. Citato dal triage §4 |
| [`2026-08-08-chat-cleanup-tracker-prima-versione.md`](handoff/2026-08-08-chat-cleanup-tracker-prima-versione.md) | *Meta* — la stessa cosa, una versione prima | ⛔ da ignorare: la seconda copia è più recente. Le due erano indistinguibili per nome |
| [`2026-08-08-final-chat-cleanup-plan.md`](handoff/2026-08-08-final-chat-cleanup-plan.md) | *Meta* — piano di chiusura delle conversazioni | ⛔ riguarda il progetto ChatGPT, non il repository |
| [`2026-08-11-bot-ai-team-planner-belief-e-tracking.md`](handoff/2026-08-11-bot-ai-team-planner-belief-e-tracking.md) | Bot/AI: team planner, belief, tracking | [`spec-bot-tattico.md`](../../gameplay/spec-bot-tattico.md) · `D-095`…`D-099` · [referto](../roadmap-plans/bot-ai-consolidamento-2026-08-11.md). ⚠️ **Quattro premesse di stato false**: nove feature `RT-FEAT-BOT-*` che non esistono, sei Epic «da creare» che esistevano già |
| [`2026-08-11-battle-simulation-harness-unificato-e-release-bot.md`](handoff/2026-08-11-battle-simulation-harness-unificato-e-release-bot.md) | Battle Simulation, harness unificato, release del bot | [`test-e-diagnosi.md`](../../technical/test-e-diagnosi.md) §3-bis/3-ter · `D-101`, `D-102` · [referto](../roadmap-plans/bot-ai-consolidamento-2026-08-11.md) §9. ✅ **Il meglio calibrato dei due**: nomina i Feature ID e le Epic reali, e dice da sé di aggiornarli invece di moltiplicarli |

> ⬜ **Manca il tredicesimo.** `RT_Common_Actions_Master_Consolidation_v0.1.md` è ancora in
> `docs/archive/consolidazione-chat-openai/`, **untracked e non recepito**: la decisione che lo bloccava è
> stata presa il 2026-08-09 — la migrazione degli Stable ID resta *dichiarata*, i documenti si allineano — ma
> nessun owner ha ancora consumato il master. Finché è così non appartiene a questa cartella, che è
> l'archivio dei sorgenti **recepiti**.

## `audit/` — stato della documentazione

| File | Oggetto | Recepito da |
|---|---|---|
| [`2026-08-08-docs-gameplay.md`](audit/2026-08-08-docs-gameplay.md) | Audit di `gameplay/` + piano di consolidamento | [`CHANGELOG_DOCUMENTATION.md`](../../CHANGELOG_DOCUMENTATION.md), Decision Log |
| [`2026-08-08-docs-non-gameplay-v2.md`](audit/2026-08-08-docs-non-gameplay-v2.md) | Audit del resto di `docs/` | [`CHANGELOG_DOCUMENTATION.md`](../../CHANGELOG_DOCUMENTATION.md), Decision Log |

## `graytoolkit/` — bundle Gray Toolkit, 2026-08-17

Archiviato come **cartella** e non in radice perché è un bundle: quattro documenti e due immagini che si
leggono insieme. L'istruttoria del consumo, con cosa è entrato e cosa no, sta in
[`graytoolkit/README.md`](graytoolkit/README.md).

| File | Oggetto | Recepito da |
|---|---|---|
| [`02_GrayToolkit_AssetRoadmap_Wiki_Issues.md`](graytoolkit/02_GrayToolkit_AssetRoadmap_Wiki_Issues.md) | Handoff operativo: world scale, asset rules, lane art, roadmap asset, cinque pagine Wiki | [D-158](../../decisions/RT_PDR_00_Decision_Log.md) · lane `AC0–AC6`/`AE0–AE5` in [`spec-asset-pipeline.md`](../../technical/spec-asset-pipeline.md) §11-bis · `GBX-5` e `GBX-6` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). ⚠️ **Quattro delle cinque pagine Wiki non sono state create**: descrivono materiale con owner già esistente |
| [`03_GrayToolkit_Wiki_Pages_Original.md`](graytoolkit/03_GrayToolkit_Wiki_Pages_Original.md) | Prima stesura delle pagine Wiki | **Superato dentro il bundle stesso** da `04`; conservato per confronto |
| [`04_GrayToolkit_Wiki_Pages_v2_Latest.md`](graytoolkit/04_GrayToolkit_Wiki_Pages_v2_Latest.md) | Struttura Wiki v2 con testo pronto | Pagina **Graybox Toolkit** pubblicata; le altre quattro no |
| [`README_FIRST.md`](graytoolkit/README_FIRST.md) | Ordine di lettura proposto dall'autore | Conservato per provenienza |
| `images/RT_GrayToolkit_Public_Infographic_v2.png` | Infografica pubblica | Pubblicata come `images/wiki/core/24_graybox-toolkit-overview.png`. ⚠️ Tre delle sue «regole asset» contraddicono il canone, e la pagina che la ospita le corregge |
| `images/RT_GrayToolkit_UML_Developer_v2.png` | UML «per sviluppatori» | Pubblicata come **proposta**: delle classi che nomina, **una sola** esiste in `Source/` |

> ⚠️ **`01_Graybox_Kit_Cover_CellVolume_Consolidation.md` non è qui**, ed è voluto: era **byte-identico**
> al kit già archiviato in radice (`md5 4048a39b17513e88da41d3c7ba75aaee`). Archiviarlo due volte avrebbe
> creato una seconda copia da tenere allineata.

---

## Radice — sorgenti senza cartella

| File | Oggetto | Recepito da |
|---|---|---|
| [`RefactorTactics_WeaponVariants_Claude_Consolidation.md`](RefactorTactics_WeaponVariants_Claude_Consolidation.md) | Varianti d'arma, affinità eroe/variante, fasce di danno | [D-085](../../decisions/RT_PDR_00_Decision_Log.md)…[D-088](../../decisions/RT_PDR_00_Decision_Log.md) (le quattro `Locked`) · `WV-1`…`WV-5` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) · catalogo owner [`RT_EquipmentCatalog_v0.1.md`](../../balance/RT_EquipmentCatalog_v0.1.md). Revisione dell'esito: [`weapon-variants-spec-panel-2026-08-11.md`](../../roadmap/plans/weapon-variants-spec-panel-2026-08-11.md) — le §18–§29 **non** sono state applicate, costruite su una fotografia più arretrata del repository stesso |
| [`RefactorTactics_Character_Radar_Wiki_Generator_Claude.md`](RefactorTactics_Character_Radar_Wiki_Generator_Claude.md) | Radar di personaggio, rubrica dei rating e generatore SVG per la Wiki | [D-105](../../decisions/RT_PDR_00_Decision_Log.md)…[D-108](../../decisions/RT_PDR_00_Decision_Log.md) · epic **E37** in [`roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md) · owner [`spec-radar-profilo-personaggio.md`](../../characters/spec-radar-profilo-personaggio.md) · generatore in [`tools/radar/`](../../../tools/radar/). ⚠️ **Riga ricostruita il 2026-08-13**: il file era archiviato sul disco senza comparire in questo indice, ed è la stessa forma di [#579](https://github.com/DegrassiAaron/refactor-tactics-main/issues/579) fuori da `handoff/`. Ricostruita dalle fonti che il sorgente ha prodotto, non dal ricordo di chi l'ha archiviato |
| [`RefactorTactics_Walls_Doors_InteractionGraph_v1_Claude_Handoff_2026-08-13.md`](RefactorTactics_Walls_Doors_InteractionGraph_v1_Claude_Handoff_2026-08-13.md) | Muri, porte, strutture e interaction graph dalla v0.1 alla v1.0 | [D-138](../../decisions/RT_PDR_00_Decision_Log.md) · epic **E23** [#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324) con sette sub-issue · issue [#832](https://github.com/DegrassiAaron/refactor-tactics-main/issues/832)/[#833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/833)/[#834](https://github.com/DegrassiAaron/refactor-tactics-main/issues/834) · `INT-5` e `INT-6` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) · owner [`roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md) §E23. Referto del filtro: [`walls-doors-interaction-spec-panel-2026-08-13.md`](../../roadmap/plans/walls-doors-interaction-spec-panel-2026-08-13.md). ⚠️ **La sua §5 e la sua §18 erano superate da `D-136` di sette ore**, e le §7/§25/§27 davano per mancante il gruppo atomico multi-transition, che ha test verdi dalla v0.1: il banner del sorgente elenca cosa non è entrato e perché |
| [`RefactorTactics_Claude_Consolidamento_Roadmap_v1_0_2026-08-13.md`](RefactorTactics_Claude_Consolidamento_Roadmap_v1_0_2026-08-13.md) | Consolidamento roadmap, documentazione, Wiki e issue fino alla v1.0 | [D-136](../../decisions/RT_PDR_00_Decision_Log.md) (modello di release fino a `v1.0`, epic post-v0.1 esprimibili) · epic **E40**–**E45** in [`roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md) · `REL-1` e `REL-2` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). Le §3–§9 descrivevano come da preservare cose **già vere**, e la §12 diagnosticava come ownership mancante ciò che era un campo non scrivibile: il banner del sorgente elenca cosa non è entrato e perché |
| [`RefactorTactics_Wiki_PlayerFirst_Claude_Handoff_2026-08-13.md`](RefactorTactics_Wiki_PlayerFirst_Claude_Handoff_2026-08-13.md) | Ristrutturazione della Wiki in manuale player-first con Developer Zone separata: IA target, quattro template di pagina, visual e icon grammar, otto wave | Audit: [`wiki-audit-player-first-2026-08-13.md`](../../roadmap/plans/wiki-audit-player-first-2026-08-13.md) · epic [#422](https://github.com/DegrassiAaron/refactor-tactics-main/issues/422) **riscritta**, non duplicata · wave [#821](https://github.com/DegrassiAaron/refactor-tactics-main/issues/821)–[#828](https://github.com/DegrassiAaron/refactor-tactics-main/issues/828) · relation su [#757](https://github.com/DegrassiAaron/refactor-tactics-main/issues/757). Le wave 1–8 **non** sono state eseguite: il documento stesso vieta il big bang (§15, §26) |
| [`RefactorTactics_4_Process_Parallel_Roadmap_Claude_Consolidation.md`](RefactorTactics_4_Process_Parallel_Roadmap_Claude_Consolidation.md) | Quattro processi paralleli — tre sessioni Claude e l'autore davanti a Unreal — con write-set di batch, lease sui binari, audit dei namespace monotoni e roadmap per track fino alla v1.0 | [D-139](../../decisions/RT_PDR_00_Decision_Log.md) (write-set di batch + Binary Asset Lease) · [`../../roadmap/parallel-batch.yaml`](../../roadmap/parallel-batch.yaml) · [`workflow-parallel-claude.md`](../../technical/workflow-parallel-claude.md) §11–§15 · epic [#839](https://github.com/DegrassiAaron/refactor-tactics-main/issues/839), lavoro [#840](https://github.com/DegrassiAaron/refactor-tactics-main/issues/840). Triage: [`quattro-processi-paralleli-triage-2026-08-14.md`](../../roadmap/plans/quattro-processi-paralleli-triage-2026-08-14.md) — **13 sezioni su 57 applicate**. 🔴 **Cosa non è entrato, e perché**: la §3 chiedeva sei passi per ritirare sette file di lane che erano stati archiviati **la mattina stessa**, con banner e provenienza già a posto; le §21–§27 proponevano un terzo vocabolario di classificazione (`parallel-tracks.yaml`, `work_tracks`, quattro shortlist, filtro nel Control Center) accanto a `execution_lanes` e `domain_groups`, che sono già validati — e i tre processi Claude **non sono una funzione** di `domain_group`, perché `characters_content` si spezza su tre e `tooling_data_qa` su due; le §30–§39 erano una seconda copia della roadmap `v0.1→v1.0` affettata per track, cioè stato duplicato che nessun `--check` avrebbe visto invecchiare. ⚠️ Anche l'esempio di lease della §12 scrive un path che **non esiste**: una mappa è una cartella, e due delle tre portano un secondo package |
| [`RefactorTactics_Parallel_Claude_Worktrees_Shared_ID_Allocator.md`](RefactorTactics_Parallel_Claude_Worktrees_Shared_ID_Allocator.md) | Workflow parallelo su `git worktree` + allocatore atomico degli ID `D-nnn`: stato nel git common dir, lock cross-platform, `reserve`/`check`/`audit-refs`, bootstrap, recovery e test | [D-135](../../decisions/RT_PDR_00_Decision_Log.md) · [`scripts/rt_shared_id.py`](../../../scripts/rt_shared_id.py) (33 test, uno a venti processi) · owner [`workflow-parallel-claude.md`](../../technical/workflow-parallel-claude.md) · `AGENTS.md` §Git e `CLAUDE.md` §4. ✅ **Recepito quasi per intero, ed è il sorgente meglio calibrato dell'archivio**: descrive un difetto misurato invece che immaginato — quindici collisioni già registrate nel Decision Log — dichiara da sé il proprio limite (§16: l'atomicità si ferma al clone) e vieta di generalizzare prima di aver dimostrato il caso `D-nnn`. Persino gli errori che elenca al §15 sono quelli che il repository avrebbe commesso: `next-id.txt` versionato, «leggi `main` e fai +1», lock dentro il worktree. ⚠️ **Un solo path sbagliato, e lo dichiara**: proponeva `tests/scripts/test_rt_shared_id.py` scrivendo accanto *«non assumere questo path: verificare prima»* — la convenzione reale è [`scripts/test_rt_shared_id.py`](../../../scripts/test_rt_shared_id.py). 🔵 **La sua unica domanda aperta è stata chiusa il giorno dopo, non è pendente**: la §21 rimandava l'estensione ad altri contatori (`E-nn`, reservation remota) a *dopo una misura*, e la misura è l'audit degli undici namespace monotoni di [D-139](../../decisions/RT_PDR_00_Decision_Log.md) — risposta **no**: i tre serializzati hanno una sola sorgente di scrittura ciascuno, e per `E-nn`/`XXX-n` la scelta manuale era già motivata in `AGENTS.md`. 🔴 **E il residuo che restava era nel suo Scopo, non in ciò che ha mancato**: prometteva di eliminare *«collisioni di numerazione **e sovrascritture**»*, e il §2.1 le aveva identificate correttamente — *«l'allocatore ID da solo non basta»* — rispondendo con un worktree per sessione. Quella regola risolve **due sessioni nella stessa directory**, non **due worktree che scrivono lo stesso file**: è il buco che [D-139](../../decisions/RT_PDR_00_Decision_Log.md) ha chiuso il 2026-08-14 col write-set di batch, e che il primo batch reale ha violato subito |
| [`RefactorTactics_Camera_Roadmap_v1.0_Claude_Consolidation_2026-08-14.md`](RefactorTactics_Camera_Roadmap_v1.0_Claude_Consolidation_2026-08-14.md) | Camera tattica consolidata fino alla v1.0: modello camera e input, Strategic View a soglia con isteresi, multilayer e marker verticali autorizzati, cutaway/occlusion, Camera Director su TurnLog, replay/spectator, accessibilità e Camera Lab — 51 issue candidate su dieci release, 34 scenari, 9 decisioni | [D-142](../../decisions/RT_PDR_00_Decision_Log.md) (snap a 45°) · [D-143](../../decisions/RT_PDR_00_Decision_Log.md) (camera presentation-only) · [`../../roadmap/feature-registry.yaml`](../../roadmap/feature-registry.yaml) (`RT-FEAT-UI-TACTICAL-CAMERA`, `automation: todo → partial`). Triage: [`camera-roadmap-v1-triage-2026-08-14.md`](../../roadmap/plans/camera-roadmap-v1-triage-2026-08-14.md). 🔴 **Sei dei tredici Feature ID che chiede di auditare non esistono**, e due di essi hanno un omonimo semantico con un altro nome (`RT-FEAT-UI-CELL-SELECTION` → `POINTER-INTERACTION`, `RT-FEAT-PERCEPTION-SOUND-OVERLAY` → `NOISE`): sono nomi plausibili di feature assenti, e prenderli per buoni avrebbe prodotto la *Feature ID explosion* che il documento stesso vieta al §17. Riporta inoltre lo stato `IMPLEMENTED`, che non è un valore ammesso dallo schema. ⚠️ **Il §8 lascia aperto «snap 60° vs 90°» quando entrambe le opzioni erano già respinte** da un commento in `RTCameraPawn.h`, e il §3.8 teme un popping da SpringArm che `bDoCollisionTest = false` già impedisce. ✅ **Il suo merito è indiretto e concreto**: cercando i test che dava per assenti ne sono emersi **quattro** già in repository e non dichiarati dal registry — un difetto che nessun gate poteva vedere, perché `validate` controlla solo il verso *dichiarato → esistente*. 🔵 **48 issue su 51 non aperte**: descrivono release da v0.2 a v1.0 le cui epic esistono già |
| [`RefactorTactics_Mini_Roadmap_v01_Autobattle_Claude_Consolidation_2026-08-16.md`](RefactorTactics_Mini_Roadmap_v01_Autobattle_Claude_Consolidation_2026-08-16.md) | Mini roadmap accelerata verso una demo 2v2 bot-contro-bot automatica e osservabile: sette release intermedie `a1…rc1`, board tattica astratta con grammatica visiva, determinismo con seed, scenario runner configurabile, tre processi paralleli | [D-145](../../decisions/RT_PDR_00_Decision_Log.md) (execution slice, epic **E47**, nessuna milestone nuova) · [D-146](../../decisions/RT_PDR_00_Decision_Log.md) (grammatica visiva derivata, encoding ridondante) · epic [#952](https://github.com/DegrassiAaron/refactor-tactics-main/issues/952) con i checkpoint [#954](https://github.com/DegrassiAaron/refactor-tactics-main/issues/954)–[#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959) · feature `RT-FEAT-MATCH-AUTOBATTLE` e `RT-FEAT-UI-BOARD-GRAMMAR` in [`feature-registry.yaml`](../../roadmap/feature-registry.yaml) · `RNG-1`/`RNG-2` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), issue [#960](https://github.com/DegrassiAaron/refactor-tactics-main/issues/960) · seduta **U23** in [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml). Referto: [`mini-roadmap-autobattle-spec-panel-2026-08-16.md`](../../roadmap/plans/mini-roadmap-autobattle-spec-panel-2026-08-16.md) — **9 sezioni su 38 applicate**. 🔴 **È il primo sorgente dell'archivio le cui sezioni respinte chiedono *meno* di ciò che è consegnato**, non di più: la §5 limita il gioco a `Move · BasicAttack · Wait` mentre E6 è chiusa e `Action.Wait` è a catalogo; la §7 e la §12 propongono un bot a scelta casuale da promuovere *poi* a utility scoring, mentre `RT-FEAT-BOT-BASE` è `RELEASE_READY` con quello scoring dal 2026-08-06. Applicarle sarebbe stata una **rimozione**. ⚠️ Quattro sezioni contraddicono un modello deciso: `Cover` come categoria di cella contro la copertura per **bordo** di **E9.1** (§4/§11), «massimo tre processi» contro le sei track di [`parallel-batch.yaml`](../../roadmap/parallel-batch.yaml) (§18), otto Release field `0.1-a1…0.1` che sarebbero un **terzo** spazio di numerazione (§24). ⏸️ Il seed della §6 è **differito, non respinto**: zero `FRandomStream` nel runtime e `FRTTestScenario::Seed` documentato come «dichiarato ma non consumato». ✅ **Ciò che resta è piccolo e vero**: `RefactorTactics.HexMatch.PlaysToCompletion` gioca già un 2v2 bot-contro-bot headless fino all'eliminazione, e nessuno può guardarlo perché `ARTGameMode::SpawnHero` assegna il bot alla sola squadra 1 |
| [`CLAUDE_RefactorTactics_Graybox_Kit_Cover_CellVolume_Consolidation_2026-08-17.md`](CLAUDE_RefactorTactics_Graybox_Kit_Cover_CellVolume_Consolidation_2026-08-17.md) | Graybox Kit, grammatica visuale della cover, Cell Placement Volume, tassonomia di posizionamento, pivot e ladder di maturita' degli asset fino alla v1.0 — 28 sezioni (`§0`–`§27`), 19 elementi di catalogo | [D-152](../../decisions/RT_PDR_00_Decision_Log.md) (contratto di ingombro/pivot/presentazione) · [D-153](../../decisions/RT_PDR_00_Decision_Log.md) (innesto sulla release ladder canonica) · owner [`spec-graybox-placement-contract.md`](../../technical/spec-graybox-placement-contract.md) · feature `RT-FEAT-UI-GRAYBOX-KIT` in [`feature-registry.yaml`](../../roadmap/feature-registry.yaml) · `GBX-1`…`GBX-4` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), issue [#1094](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1094) · seduta **U25** in [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), issue [#1095](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1095) · voci PIE [#1096](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1096) · epic **E21** [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) e **E45** [#778](https://github.com/DegrassiAaron/refactor-tactics-main/issues/778) aggiornate, **nessuna creata**. 🔴 **Tre prescrizioni respinte perche' descrivono un repository che non esiste**: lo step di rotazione a 30° (la grammatica canonica e' `ERTTacticalAxis` + offset **interi**, e i dodici settori sono un **righello**, non uno step), la cover come oggetto della cella (e' direzionale per **bordo** da **E9.1**), la *Temporary/Energy Cover* (`ERTHexCoverType` la esclude di proposito, e l'owner e' [`spec-coperture-temporanee-cp95.md`](../../gameplay/spec-coperture-temporanee-cp95.md)). ⚠️ **Una quarta la respinge il repository, non il filtro**: la **valvola** sta fra i diciannove elementi «v0.1» del kit, mentre [`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) §11 la dichiara fuori scope con motivazione registrata. 🔵 **La sua ladder di maturita' e' arretrata rispetto al progetto, non in anticipo**: mette Environment in v0.2, Perception in v0.7 e Objectives in v0.8, e tutti e tre sono lavoro **della v0.1** — seguirla avrebbe rinviato gli asset di sistemi che la release possiede gia'. ⚠️ *Il grado differisce e la prima stesura lo appiattiva: `INTEGRATED` vale per **E8** (8 su 8); E13 ed E10 non hanno nessuna feature `INTEGRATED`. Corretto in code review — la tesi regge, la prova era piu' forte del vero.* ✅ **Due contributi che la sorgente non aveva, trovati misurando**: `ERTHexDoorState` ha **quattro** stati e il kit ne conosce tre — `Locked` e `Closed` sono geometricamente identici, quindi distinguerli col solo colore violerebbe [D-146](../../decisions/RT_PDR_00_Decision_Log.md) al primo asset (`GBX-2`) — e gli elementi interattivi **non si esauriscono nell'`Online/Offline` proposto** — l'esempio di CP 10.1 §5 ne mostra cinque, ed e' un *esempio*, non un catalogo: il contratto impone la ridondanza per qualunque cardinalita'. Il conto del catalogo e' `REUSE 2 · UPDATE 8 · CREATE 2 · **DEFER 7**`, e i sette differiti si dividono per ragione: **tre** per dipendenza da feature `IDEA`, **due** fuori scope v0.1 dichiarato, **due** proxy senza produttore |
| [`RefactorTactics_Claude_MultiHero_TimeBank_PreferredReaction_2026-08-17.md`](RefactorTactics_Claude_MultiHero_TimeBank_PreferredReaction_2026-08-17.md) | Un Player può controllare più Hero · Decision Time Bank sensibile al carico di controllo · `PreferredResponse` distinta dalla risposta di timeout · Quick Confirm | [D-155](../../decisions/RT_PDR_00_Decision_Log.md) · [D-156](../../decisions/RT_PDR_00_Decision_Log.md) · [D-157](../../decisions/RT_PDR_00_Decision_Log.md) · [`spec-decision-time-bank.md`](../../gameplay/spec-decision-time-bank.md) §3.4 e §4.4 · [`spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md) §16.4 · [`spec-reaction-clash-e14.md`](../../gameplay/spec-reaction-clash-e14.md) §2.6 · righe **73–75** di [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) · [#1124](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1124) (CP 19.3) · referto [`multihero-timebank-preferred-response-spec-panel-2026-08-17.md`](../../roadmap/plans/multihero-timebank-preferred-response-spec-panel-2026-08-17.md) |
| [`RefactorTactics_TacticalDesigner_03_Roadmap_v1.0_Scenari_SkillWorkbench_Claude.md`](RefactorTactics_TacticalDesigner_03_Roadmap_v1.0_Scenari_SkillWorkbench_Claude.md) | Consolidamento di Level/Map Designer, Skill Workbench e Scenario Composer in un unico **Tactical Designer**, con scala di maturità `v0.1→v1.0`, modello di scenario, profili di abilità e analytics di bilanciamento — 3087 righe, 76 sezioni | [D-154](../../decisions/RT_PDR_00_Decision_Log.md) · owner [`spec-tactical-designer.md`](../../technical/spec-tactical-designer.md) · checkpoint **M9.4** in [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) · feature `RT-FEAT-TOOL-SCENARIO-COMPOSER` e `RT-FEAT-TOOL-SKILL-WORKBENCH` in [`feature-registry.yaml`](../../roadmap/feature-registry.yaml) · epic di processo [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) **senza numero `E`** (la numerazione `E` e' quella delle release; stessa forma di [#839](https://github.com/DegrassiAaron/refactor-tactics-main/issues/839) e [#422](https://github.com/DegrassiAaron/refactor-tactics-main/issues/422)) · issue [#1106](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1106) · seduta **U26** in [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml). Referto: [`tactical-designer-consolidamento-2026-08-17.md`](../../roadmap/plans/tactical-designer-consolidamento-2026-08-17.md). 🔴 **Dieci sezioni su 76 erano superate all'arrivo**: progettano `#620` e `#621`, chiuse il 2026-08-13, e il tool d'editor che chiedono di costruire esiste. 🔴 **Il §68 sbaglia tre nomi del roster su quattro** («Phaser, Victor, Wrath») contro [D-120](../../decisions/RT_PDR_00_Decision_Log.md)/[D-130](../../decisions/RT_PDR_00_Decision_Log.md): nessuno è entrato in un documento normativo. ⚠️ Il §36 vieta un secondo formato di scenario e il §37 ne progetta uno che per metà esiste già in `FRTTestScenario`. ✅ **Ciò che era nuovo e vale**: §3 (una dipendenza falsa in un issue body cambia come il prossimo legge il lavoro), §32 (runtime rule → pure query → visualizzazione) e §41 (le quattro nature di un'aspettativa) |
| [`RefactorTactics_Claude_Consolidamento_TacticalDesigner_Map_Scenario_2026-08-16.md`](RefactorTactics_Claude_Consolidamento_TacticalDesigner_Map_Scenario_2026-08-16.md) | Consolidamento del Tactical Designer v0.1 lato Map Editor + Scenario Composer: stato di `#711`/`#622`/`#712`/`#623`, riuso del modello scenario esistente e quattro slice di authoring visuale (`SC-1`…`SC-4`) — 13 sezioni | issue [#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114) · [#1115](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1115) · [#1116](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1116) · [#1117](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1117), tutte sub-issue dell'epic [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) · feature `RT-FEAT-TOOL-SCENARIO-COMPOSER`, che aveva `issues: []`. 🔵 **Recepito in due tempi, ed è una forma che l'archivio non aveva ancora**: la sua parte concettuale — owner documentale, `D-154`, epic, feature, `M9.4`, seduta `U26` — era già atterrata mezz'ora prima con [#1108](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1108), che consumava un **sorgente diverso e adiacente**. Quando questo handoff è stato aperto, il suo §5.2 chiedeva una feature che esisteva già. ✅ **Il residuo era il §6, e il §4 chiedeva di riconfermarlo su HEAD prima di creare le issue: la riconferma ha retto** — `URTScenarioLoader` espone `LoadFromString`, `LoadFromFile`, `Validate` e **nessun writer**, e l'unico `ToJson` del modulo serializza `FRTTestResult`, non lo scenario. ⚠️ **Il §10 non è entrato**: proponeva due lane e un elenco di file «da non toccare in contemporanea», che è ciò che [`parallel-batch.yaml`](../../roadmap/parallel-batch.yaml) fa con un write-set misurabile invece che con una raccomandazione |
| [`RefactorTactics_Claude_Replay_CanonicalIntent_Roadmap_v0.1-v1.0_2026-08-16.md`](RefactorTactics_Claude_Replay_CanonicalIntent_Roadmap_v0.1-v1.0_2026-08-16.md) | Replay logico, TurnLog canonico, decisioni runtime, `OpportunityId`, Reaction Profile/Clash, `FRTCanonicalIntent`, confine proposal↔canonical↔snapshot, privacy server/client e roadmap `v0.1`→`v1.0` — 46 sezioni, 8 proposte (`P1`–`P8`) | issue [#1118](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1118) (`P4`, sub-issue di **E14** [#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152)) · [#1119](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1119) (`RCI-1`…`RCI-4`, sub-issue di **E40** [#773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/773)) · commento di consolidamento su [#780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/780), che il §26 chiedeva. Triage: [`replay-canonical-intent-triage-2026-08-17.md`](../../roadmap/plans/replay-canonical-intent-triage-2026-08-17.md) — `PROPOSED 1 · ALREADY DECIDED 2 · OPEN ISSUE 1 · DEFERRED 3 · REJECTED 1`. **Nessun `D-nnn` nuovo**: le due proposte che dichiarava canoniche (`P2`, `P5`) erano già decise altrove. 🔴 **Più della metà della sua roadmap `R0`–`R6` era superata all'arrivo**: `#165` è `CLOSED`, `#886` ha un DoD riscritto da uno spec panel del 2026-08-17, `#542` ha un design approvato. Il documento apre chiedendo di non fidarsi del proprio HEAD, e aveva ragione. ✅ **La sua parte più accurata è il §40, ed è quella che nessuno aveva eseguito**: i nove test che propone sono assenti **tutti e nove**, i quattro che dà per esistenti ci sono **tutti e quattro**, e il repository ne dichiara **48** col prefisso `Replay.`. ⚠️ **Il §35 non è entrato** — seconda copia della roadmap per release, i cui owner sono [`roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md) e le epic `E36`–`E45` già aperte — né il §10, che assegna i Reaction Profile per eroe ed è contenuto di `#314` |
| [`RefactorTactics_Architecture_Process_Improvement_Claude.md`](RefactorTactics_Architecture_Process_Improvement_Claude.md) | Miglioramenti architetturali e di processo dall'audit del 2026-08-17: strangler refactor di `RTTurnManager`, confini dello Scenario Harness, riduzione di `RTGameMode`, preflight locale unico, semplificazione di `parallel-batch.yaml`, gate di naming sulle sorgenti e cinque workstream (`BASE` + `WS-A`..`WS-D`) — 23 sezioni | ⚠️ **Consumato da un'altra sessione**: la sua riscrittura è `docs/technical/piano-riduzione-hotspot.md` (1293 righe, branch `docs/piano-riduzione-hotspot`), che dichiara di correggerne **sette** affermazioni misurate false o superate e chiude con *«se esiste ancora in root, va rimosso»*. 🔴 **Quel branch non era pushato al momento dell'archiviazione** — invisibile a `git ls-remote` e alle PR, lo vede solo `git worktree list`. ➕ **E il gate lo ha dimostrato**: la prima stesura di questa riga scriveva il successore come **link relativo**, e `check-docs-links` è caduto con *«1 link a un target inesistente»*. Il path resta in backtick, non linkato, finché quel branch non atterra — un successore invisibile è peggio di un successore altrui, ma un link che non risolve è peggio di entrambi. ⏸️ **Nessun suo atto è stato applicato da questa archiviazione**: le issue che nomina restano dei loro owner — [#886](https://github.com/DegrassiAaron/refactor-tactics-main/issues/886) (P0, track `simulation`), [#833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/833), [#170](https://github.com/DegrassiAaron/refactor-tactics-main/issues/170), [#950](https://github.com/DegrassiAaron/refactor-tactics-main/issues/950), [#1109](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1109), [#1096](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1096). ➕ La sua §3 è la stessa materia di [`cinque-processi-paralleli-2026-08-17.md`](../../roadmap/plans/cinque-processi-paralleli-2026-08-17.md), scritto lo stesso giorno da una sessione diversa: **tre letture indipendenti** dello stesso problema, da confrontare e non da sommare |
| [`RefactorTactics_SkillPlus_Claude_Handoff_2026-08-17.md`](RefactorTactics_SkillPlus_Claude_Handoff_2026-08-17.md) | *Skill Plus* — livello sistemico opzionale di un'abilità (`Primary + Context → Secondary`), caso di riferimento `Wind + Debris`, grammatica UI, team combo discovery, 10 epic `SP-01`..`SP-10` e ~70 issue candidate fino alla v1.0 — 34 sezioni | issue [#1132](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1132) (i detriti) e [#1133](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1133) (`SPN-1`, naming). Matrice: [`skill-plus-consolidamento-2026-08-17.md`](../../roadmap/plans/skill-plus-consolidamento-2026-08-17.md) — `REUSE 6 · CREATE 1 · LINK 2 · DEFER 5 · CONFLICT 1 · NOT NEEDED 1`. **Nessun `D-nnn`, nessuna feature, nessuna epic.** 🔴 **Il sistema che propone di costruire esiste già, è `INTEGRATED` ed è in `v0.1`**: `RT-FEAT-ENV-SYSTEMIC-COMBOS` ha nove gate su dieci `done`, l'`ADR-0006`, il test `RefactorTactics.Reactions.NoHeroSpecificBranchInResolver` che pinna il suo §5.2 e gli scenari `Visual.Combat.WaterElectric*` che sono il suo §7.2 — e le sue `SP-040`/`SP-041`, messe in v0.3–v0.4, sono `CP 8.3`/`CP 8.4`, **chiusi**. ⚠️ **Il suo caso di riferimento ha entrambi i termini assenti**: `Debris`/`Rubble`/`Scatter` danno **0** occorrenze, e le sei di `Wind` sono **tutte** `Window`/`Rewind`. ✅ Il gap reale è contenuto, non architettura. 🔴 **CONFLICT sul nome**: la §9 della spec owner decide già `Sinergia` · `Interazione sistemica` · `Setup → Payoff`, e *«Skill Plus»* sarebbe il quarto termine, l'unico inglese |

> ~~⚠️ **Il conteggio in testa è alla deriva**: la riga 5 dichiara **40** documenti, ma la cartella ne
> contiene **47**.~~ 🔴 **Questa nota era falsa in entrambi i termini, ed è stata trovata in code review il
> 2026-08-17**: la riga in testa non dice più `40` da parecchi giri, e quel comando risponde oggi **81** —
> misurato duecento righe più in alto **nello stesso file**, dal giro che ha aggiunto l'81° documento.
> Restava perché nessuno la rileggeva mentre riscriveva il paragrafo di testa: due parti dello stesso
> documento che si contraddicono, che è esattamente il difetto contro cui è scritto tutto il resto di
> questa pagina.
>
> Ciò che sopravvive della nota è il **criterio**, e vale ancora: *riscrivere un numero in un indice owner
> senza sapere cosa contava è il modo di sostituire una deriva nota con una falsa precisione*. È la ragione
> per cui i tre file di `docs_kit` senza riga d'indice restano senza.

## Nota sui path interni

I documenti sono scesi di un livello (`docs/src/X/` → `docs/archive/src/X/`) e i loro link relativi sono stati
riscritti di conseguenza. Restano **volutamente non corretti** i riferimenti *in prosa* a percorsi che non
esistono più — per esempio l'audit del 2026-08-08 che cita `docs/src/` o un nome file poi rinominato: quella
è la fotografia di com'era il repository quel giorno, e riscriverla falsificherebbe l'audit.
