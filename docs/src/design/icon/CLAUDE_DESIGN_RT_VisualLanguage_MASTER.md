# CLAUDE DESIGN WEB — RefactorTactics Tactical Visual Language MASTER

## Missione
Produrre artefatti grafici e specifiche UI per i checkpoint `ICON-0`…`ICON-11`, **uno alla volta**, mantenendo lo stesso linguaggio visivo. Non scrivere gameplay code e non inventare regole competitive mancanti.


## Regole globali LOCKED

- RefactorTactics è PC-first, tattico simultaneo, server-authoritative e deterministico.
- La UI è presentation-only: non decide path, hit, reaction, status o outcome.
- Gli intenti avversari privati non devono mai arrivare ai presentation model/client avversari.
- `Confirmed / Predicted / Uncertain` sono semantiche distinte e non dipendono solo dal colore.
- `Validity / Certainty / Knowledge` sono assi separati.
- Icone e visual token sono semantici e componibili, non un'immagine custom per ogni skill.
- Ally/Enemy devono differire per forma oltre che per colore.
- Evitare rosso/verde come coppia primaria Ally/Enemy.
- Default palette iniziale: Ally cyan/blue, Enemy orange; elementi mantengono il proprio colore semantico.
- Il sistema deve restare leggibile in grayscale; CVD e High Contrast rafforzano forma, pattern, luminanza e outline.
- `Electric` e `Reaction` non condividono il fulmine.
- `Line` e `Move` sono distinti: Line = origine + segmento + punta; Move = path con nodi.
- `Cover`, `Guard`, `Brace`, `Shield` sono concetti distinti.
- `Water` payload, `Wet` unit state e `Water Surface` cell state sono distinti tramite composizione/frame.
- Non creare Gameplay Tag solo per soddisfare un'icona.
- Non hardcodare texture/colori nei widget: usare semantic ID/catalog/theme.
- La versione Unreal reale va letta dalla repo; la documentazione corrente usa UE 5.8 come baseline.


## Direzione grafica
- PC-first 1920×1080.
- Master glyph: 24×24 px, safe area circa 20×20, stroke principale circa 2 px.
- Varianti ottiche 16/20/24/32 quando necessarie.
- Silhouette-first, 2–3 componenti visive massime a 24 px.
- Asset sorgente preferibilmente monocromatico/tintable.
- Colore come secondo canale.
- Certainty: Confirmed solid; Predicted dashed; Uncertain dotted/fade + `?`; Invalid slash/`⊘`.
- Palette PLAYTEST: Ally `#56B4E9`, Enemy `#E69F00`, Water `#0072B2`, Fire `#D55E00`, Electric `#F0E442`, Defense `#009E73`, Reaction `#CC79A7`.
- CVD non significa solo cambiare hue: rafforzare forma, pattern, outline, luminanza.
- High Contrast è separato da CVD.
- Reduced Motion conserva sempre equivalente statico.

## 16 glyph proof-of-concept
Ally, Enemy, Cell, Line, Circle, Cone, Damage, Push, Shield, Move, Dash, Water, Electric, Fire, Reaction, Uncertain.

## Regola di review
Prima consegna: monocromatica.
Se i glyph non sono distinguibili in grayscale/silhouette, NON correggere il problema usando il colore.

## Output preferiti
Per ogni checkpoint:
- board 1920×1080 o 2560×1440;
- component/state matrix;
- variante Default;
- grayscale;
- CVD;
- High Contrast se pertinente;
- annotazioni dimensionali;
- nomi semantici;
- do/don't;
- export list (SVG/PNG/vector source quando supportato);
- screenshot/mockup di contesto.
Non includere testo baked dentro le icone.

## Relazione con Claude CLI
Claude Design **non crea issue GitHub**. Ogni brief contiene i titoli issue candidati; Claude CLI li verifica/crea/aggiorna e inserisce i numeri reali.
