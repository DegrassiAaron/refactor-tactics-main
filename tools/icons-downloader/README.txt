PARAGON SKILL ICONS — BULK DOWNLOADER
=====================================

Scopo
-----
Recuperare in blocco le icone delle abilità/skill del MOBA Paragon di Epic Games
da due fonti storiche:

1. Paragon Archive / Fandom — Category:Abilities
   https://paragon-archive.fandom.com/wiki/Category:Abilities

   La categoria dichiara 105 file media. Attenzione: include anche alcuni icon
   generici/stat oltre alle icone dei kit degli eroi.

2. Dump storico dell'API developer di Paragon
   https://github.com/alex-taxiera/ParagoneAPI

   Il file heroes.json conserva i campi abilities[].images.icon e i vecchi URL:
   https://developer-paragon-cdn.epicgames.com/Images/<hash>.png

Uso rapido
----------
Serve Python 3.9+.

Windows:
  py paragon_skill_icons_downloader.py --source both

macOS / Linux:
  python3 paragon_skill_icons_downloader.py --source both

Output:
  Paragon_Skill_Icons/
    Paragon_Archive/
    Epic_CDN/
      <Hero>/
        <slot>_<binding>_<ability>.png
    manifest.csv

Varianti
--------
Solo set archivio Fandom:
  py paragon_skill_icons_downloader.py --source archive

Solo riferimenti originali Epic CDN:
  py paragon_skill_icons_downloader.py --source epic

Crea soltanto il catalogo URL, senza scaricare immagini:
  py paragon_skill_icons_downloader.py --source both --list-only

Cartella personalizzata:
  py paragon_skill_icons_downloader.py --out D:\ParagonIcons

Note
----
- Lo script usa solo la standard library di Python.
- I vecchi URL CDN di Epic possono non essere tutti ancora raggiungibili.
  Per questo il set Paragon Archive è incluso come sorgente alternativa.
- "Category:Abilities" è un archivio community e non equivale a una licenza
  commerciale sulle grafiche originali.
- Per un progetto originale conviene usare queste icone solo come reference,
  prototipo interno o materiale di studio, e creare poi un set grafico originale.
