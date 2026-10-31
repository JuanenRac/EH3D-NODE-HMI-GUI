<p align="center">
  <img src="images/EH3D_BANNER.svg" alt="EH3D-NODE-HMI-GUI banner" width="100%">
</p>

# 🖥️ EH3D-NODE-HMI-GUI

<p align="center"><a href="README.md">🇺🇸 English</a> | <a href="README_spa.md">🇪🇸 Español</a> | 🇫🇷 <b>Français</b> | <a href="README_ita.md">🇮🇹 Italiano</a> | <a href="README_deu.md">🇩🇪 Deutsch</a> | <a href="README_zho.md">🇨🇳 中文</a> | <a href="README_jpn.md">🇯🇵 日本語</a> | <a href="README_por.md">🇵🇹 Português</a></p>

### Un écran tactile de 10,1 pouces avec un ESP32-P4 qui pilote seul une machine CNC, un laser ou une machine pick and place : les écrans et le déroulement des programmes de bureau, en huit langues, des alertes et des ordres vocaux, et un arrêt matériel qui ne passe pas par l'écran.

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Stack-C%2B%2B17%20%2B%20ESP--IDF-00599C.svg" alt="C++17 and ESP-IDF">
  <img src="https://img.shields.io/badge/Languages-8-00E5FF.svg" alt="Languages">
  <img src="https://img.shields.io/badge/Version-0.0.1-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-FFB020.svg" alt="Maturity">
</p>

---

**Vérification d'honnêteté - ce qui tourne aujourd'hui :** les règles de l'écran sont écrites en C++17 et testées sur un ordinateur : le fichier de réglages avec ses valeurs sûres par défaut, la porte que tout déplacement et tout ordre de broche doit franchir (une confirmation à l'écran, le verrou de l'arrêt matériel, la liaison armée), les ordres et alertes vocaux en huit langues, les textes des écrans, les dispositions en portrait et en paysage, les fichiers de la microSD et le client d'EH3D-SERVER (les requêtes, la lecture de la flotte et le déroulement d'un travail depuis un fichier de la carte jusqu'à sa fin). Le firmware ESP-IDF avec les écrans LVGL est écrit pour le module Guition JC8012P4A1C mais **n'a pas été compilé, flashé ni exécuté sur un module** ; il n'a jamais parlé à un vrai serveur, et les déplacements de la machine elle-même (jog, origine) attendent la liaison avec le nœud, pas encore écrite.

---

> **État de sécurité (safety) et de sécurité informatique :** ceci n'est pas un système de sécurité (safety). L'arrêt qui protège les personnes et la machine est un bouton matériel qui coupe l'entraînement de lui-même, câblé à part de cet écran ; l'écran ne fait que le lire, l'afficher et refuser de déplacer quoi que ce soit tant qu'il est enfoncé ou non acquitté. Un déplacement ou un ordre de broche demande une confirmation à l'écran, un ordre vocal toujours, et les ordres vocaux sont désactivés jusqu'à ce que l'opérateur les active. N'utilisez pas cet écran pour conduire une machine avant que toute la chaîne ait été essayée sur une vraie. Le projet est indépendant : il n'est ni fabriqué, ni approuvé, ni soutenu par Bungard Elektronik ou Guition, et les noms de produits n'apparaissent que pour dire avec quelles machines et quels modules il est prévu de fonctionner.

## 📋 En un coup d'œil

| | |
|---|---|
| Version | `0.0.1` |
| Maturité | Échafaudage |
| Tests | 5 programmes de test sur ordinateur (1018 vérifications comptées) |
| Pile technique | C++17, ESP-IDF 5.5, LVGL 9, ESP32-P4 |
| Sur du matériel réel | Non : le firmware n'a été ni compilé ni flashé et rien n'a été branché sur une machine |

**Aller à  :** [Présentation](#-présentation) · [Architecture](#-architecture) · [Démarrage rapide](#-démarrage-rapide) · [Tests](#-tests) · [Structure](#-structure-du-dépôt) · [Liés](#-projets-liés)

## 🚦 État du projet

Chaque ligne est une partie de ce dépôt et le feu dit où elle en est aujourd'hui. Elle est écrite d'après ce que le dépôt peut montrer (tests, compilations, exécutions), pas d'après ce qui est prévu.

🟢 Fait et vérifié · 🟡 Fonctionne, avec des limites connues · 🟠 À moitié fait ou pas encore vérifié · 🔴 Pas fait ou ne fonctionne pas

| Domaine | État | Ce que cela veut dire ici |
|---|---|---|
| Logiciel | 🟠 | Le projet ESP-IDF (ESP32-P4, LVGL 9, le fichier de réglages dans sa propre partition) est écrit ; il n'a été ni compilé ni flashé. |
| Tests | 🟢 | 5 programmes de test sur ordinateur réussissent (`ctest`) : 1018 vérifications du fichier de réglages, de la porte, des ordres et alertes vocaux, des claviers, des avis enregistrés, du client du serveur et des textes des écrans en huit langues. |
| Règles de sécurité | 🟡 | Tout déplacement et tout ordre de broche passent par la porte : confirmation à l'écran, verrou de l'arrêt matériel que seul un acquittement à l'écran lève une fois le bouton relâché, aucun déplacement si la liaison n'est pas armée. Couvert par des tests ; jamais essayé avec un vrai bouton d'arrêt. |
| Voix | 🟡 | Les ordres et alertes vocaux sont reconnus et formulés en huit langues et testés comme du texte ; aucun micro ni haut-parleur n'a été utilisé. |
| Écrans | 🟠 | Les écrans machine, travail (microSD), machines et stations et réglages sont écrits, en paysage et en portrait, avec l'aspect du programme de bureau ; l'onglet vue est encore une carte vide. Il n'existe qu'un dessin de la mise en page, pas une capture d'un module. |
| Matériel réel | 🔴 | Aucun module n'a exécuté ce firmware, la liaison avec le serveur n'existe pas encore et rien n'a été branché sur une machine. |
| Documentation | 🟢 | README en huit langues, la référence des réglages et les notes matérielles. |
| Serveur et carte | 🟡 | Les fichiers de la microSD (liste, noms sûrs, types), les requêtes à EH3D-SERVER, la lecture de la flotte et le déroulement d'un travail (l'envoyer prêt, le lancer par la porte, le suivre, répondre au changement d'outil) sont testés sur ordinateur avec des réponses du serveur écrites à la main ; les sockets et le pilote de la carte n'ont jamais tourné sur un module. |

## 🎯 Présentation

* La **porte** : tout déplacement et tout ordre de broche, par toucher ou par la voix, passe par un seul endroit qui décide de l'exécuter, de demander une confirmation ou de le refuser avec un motif ; un arrêt passe toujours.
* Le **fichier de réglages** `hmi.cfg` : chaque valeur ajustée (langue, luminosité, le serveur, les interrupteurs de sécurité, la voix) est une ligne d'un fichier dans sa propre partition, jamais un nombre dans le code ; une valeur hors plage est signalée et la valeur sûre par défaut reste.
* La **voix** : alertes parlées (travail terminé, changement d'outil, erreur, liaison perdue, arrêt matériel) et une courte liste fixe d'ordres vocaux en huit langues ; les ordres qui ne font qu'arrêter se comprennent dans une phrase, ceux qui déplacent ou font tourner doivent être dits en entier et demandent toujours une confirmation.
* Les **écrans** : la machine (jog, origine, broche, démarrage, arrêt), le travail, la vue, les machines et stations du serveur et les réglages, dans l'ordre et avec les noms du programme de bureau ; le bouton d'arrêt est toujours en haut.
* Le **module** : le Guition JC8012P4A1C (ESP32-P4 avec 32 Mo de RAM externe, écran tactile MIPI-DSI de 10,1 pouces en 800x1280, un ESP32-C6 pour le Wi-Fi, le codec ES8311 avec amplificateur de haut-parleur et l'entrée micro ES7210). La dalle de l'unité est la nouvelle.
* **Debout ou couché :** le réglage `orientation` (`auto`, `landscape`, `portrait`) tourne l'image de la dalle : en paysage (1280 x 800) le travail, la vue en plan et le pavé de jog sont en trois colonnes au-dessus de la barre de position, en portrait (800 x 1280) ils s'empilent ; le toucher tombe où l'on touche l'écran dans les deux cas.
* **La microSD et le serveur, comme un seul flux :** la carte liste les fichiers de perçage et de défonçage ; l'écran envoie le fichier choisi à EH3D-SERVER comme un travail qui attend prêt, l'opérateur le lance par la porte, et la progression, les arrêts pour changement d'outil et la fin reviennent de la flotte en temps réel, comme dans les programmes de bureau et le nœud. La réponse du serveur pour chaque travail est enregistrée sur la carte.

## 🧩 Rôle dans EH3D

EH3D-NODE-HMI-GUI est l'écran propre de l'opérateur d'une machine CNC, d'un laser ou d'une machine pick and place (le même matériel qu'EH3D-NODE-BUNGARD-HMI) : il remplace le PC à côté de la machine et c'est un client de plus d'EH3D-SERVER, comme EH3D-DRILLPRO, EH3D-ROUTEPRO et EH3D-BUNGARD-GUI. Un travail que l'écran envoie depuis la microSD est planifié par le serveur et se trouve au même instant dans les programmes de bureau ; un travail lancé depuis un programme de bureau montre ici sa progression ; le serveur l'exécute sur la machine via le nœud de la machine. Les déplacements directs de l'écran (jog, origine) utiliseront le même nœud, et cette liaison n'est pas encore écrite.

## 🏛️ Architecture

<p align="center">
  <img src="images/architecture.svg" alt="Architecture de EH3D-NODE-HMI-GUI : qui l'utilise, ce qu'il contient et de quoi il dépend" width="100%">
</p>

Comment s'assemblent les parties de ce dépôt : qui lui parle, ce qu'il contient et de quoi il dépend. Le dessin est tiré de `docs/architecture.json` et vérifié par rapport au code.

## 🖼️ Captures d'écran

<p align="center">
  <img src="images/screenshots/screen-machine.png" alt="machine screen lying down" width="100%">
</p>

<p align="center"><em>L'écran de la machine en paysage, en anglais : le travail, la vue en plan et le pavé de jog en trois colonnes, la position et le bouton d'arrêt sur la barre. Un dessin de la mise en page à partir des textes du firmware, pas une capture d'un module.</em></p>

<p align="center">
  <img src="images/screenshots/screen-portrait.png" alt="machine screen standing up" width="40%">
</p>

<p align="center"><em>Le même écran en portrait, en allemand : les cartes s'empilent et la barre de position reste en bas.</em></p>

<p align="center">
  <img src="images/screenshots/screen-card.png" alt="files of the microSD card" width="100%">
</p>

<p align="center"><em>La page du travail (ici en italien) : les fichiers de perçage et de défonçage de la microSD, celui choisi et le bouton qui l'envoie au serveur comme un travail qui attend prêt.</em></p>

<p align="center">
  <img src="images/screenshots/screen-fleet.png" alt="machines and stations" width="100%">
</p>

<p align="center"><em>Machines et stations (ici en portugais) : toutes les machines du serveur par station, avec leur état et la progression de leur travail, comme les voient les programmes de bureau.</em></p>

<p align="center">
  <img src="images/screenshots/screen-confirm.png" alt="confirmation of a move" width="100%">
</p>

<p align="center"><em>Un déplacement demande une confirmation avant que rien ne se passe (ici en espagnol).</em></p>

<p align="center">
  <img src="images/screenshots/screen-stop.png" alt="hardware stop pressed" width="100%">
</p>

<p align="center"><em>L'arrêt matériel est enfoncé : l'écran le dit, grise les boutons qui déplacent et refuse de déplacer quoi que ce soit tant que le relâchement n'est pas acquitté (ici en japonais).</em></p>

## ✨ Fonctionnalités

* **Un arrêt qu'on ne peut pas oublier :** le bouton d'arrêt est toujours en haut et fonctionne toujours ; l'arrêt matériel se verrouille et seul un acquittement à l'écran, bouton relâché, le lève.
* **Confirmation avant le mouvement :** un déplacement ou un ordre de broche montre ce qu'il va faire et attend un toucher, avec une limite de temps ; une nouvelle demande ou une liaison perdue l'abandonne.
* **Huit langues, aussi à la voix :** anglais, espagnol, italien, allemand, français, portugais européen, chinois et japonais dans les écrans, les alertes parlées et les ordres vocaux.
* **Sûr par défaut :** chaque interrupteur de sécurité a son réglage et démarre dans sa position prudente : ordres vocaux désactivés, confirmation vocale désactivée, confirmation activée, arrêt matériel exigé.
* **Le bon clavier pour chaque réglage :** un nombre se saisit sur un pavé numérique qui connaît la plage et les décimales de ce réglage (les flèches le déplacent d'une unité, une valeur hors plage n'est pas acceptée) ; un texte, comme l'adresse du serveur ou le jeton, sur un clavier complet avec les touches de la langue utilisée (trémas, accents, tilde, cédille), avec une majuscule pour une lettre et une couche de symboles ; les interrupteurs et les listes n'ont pas besoin de clavier.
* **Boutons avec symbole :** le pavé de déplacement, l'origine, la broche, pause, démarrage, arrêt et les réponses à une question utilisent des symboles et la lettre de l'axe au lieu de mots, aux couleurs du programme de bureau (vert pour démarrer, bleu pour la pause, rouge pour arrêter).
* **Avis enregistrés :** la fin d'un travail et la demande d'outil peuvent être dites par une voix enregistrée, dans la langue et le genre réglés (`sound_finish = voice:es:f`), depuis le dossier `voices` de la carte ; les enregistrements sont ceux d'EH3D-BUNGARD-GUI. Le haut-parleur n'est pas encore câblé dans le firmware.
* **Le travail est suivi de nouveau après un redémarrage :** dès que l'opérateur lance un travail, l'écran garde dans sa flash lequel c'est (son id, la machine, son nom) ; après une coupure de courant ou un redémarrage il suit le même travail, la lecture suivante du serveur dit s'il tourne encore, attend à un changement d'outil ou est terminé, et le bouton annuler le laisse. Le travail lui-même vit dans le serveur. Écrit et testé sur un ordinateur ; le firmware n'a jamais tourné sur un module.
* **Le type de machine :** `machine_kind` dans le fichier de réglages (`cnc`, `laser` ou `pnp`) décide de ce que l'écran peut ordonner. Seule une machine CNC a le bouton de broche, et la barrière refuse un ordre de broche sur un laser ou une machine pick and place quoi que disent les autres réglages : l'écran ne déclenche jamais un laser et ne démarre jamais seul une tête pick and place.

## 🔒 Sécurité et protection

| Menace | Défense |
|---|---|
| Un toucher ou un ordre vocal qui déplace la machine par accident | un déplacement demande une confirmation à l'écran avec une limite de temps ; un ordre vocal en demande toujours une ; un oui parlé ne confirme que si le réglage `voice_confirm` est activé (désactivé par défaut) |
| L'écran déplace pendant que l'arrêt matériel est enfoncé | l'arrêt matériel est un verrou : enfoncé, ou relâché mais non acquitté à l'écran, tout déplacement et tout ordre de broche sont refusés et ce qui attendait est abandonné ; un arrêt enfoncé pendant qu'une confirmation attend l'annule |
| Une machine pas prête | rien ne bouge si la liaison n'est pas armée, si l'entrée d'arrêt matériel n'est pas câblée (`hw_stop_required`, activé par défaut) ou si la limite de broche vaut 0 |
| Un mot parlé pris pour un ordre | les ordres viennent d'une courte liste fixe par langue, jamais d'un texte libre ; un mot qui ne fait que contenir un ordre (`stopwatch`) n'est pas un ordre ; les ordres vocaux sont désactivés jusqu'à ce que l'opérateur les active |
| Une faute de frappe dans les réglages | une valeur hors plage ou une clé inconnue est signalée et la valeur sûre par défaut reste ; aucun interrupteur ne s'ouvre par erreur |
| Un secret dans le firmware ou dans le dépôt | rien de secret n'est compilé ; le jeton est un réglage dans sa partition et n'est jamais affiché ni journalisé |

**Ce qui n'est pas défendu :** l'écran n'est pas un dispositif de sécurité et ne peut pas couper l'entraînement de la machine ; l'arrêt matériel doit le faire seul. Il n'y a pas encore de liaison avec le serveur, donc l'écran ne peut aujourd'hui déplacer aucune machine. La reconnaissance vocale est testée comme du texte ; le vrai chemin du micro (annulation d'écho, bruit, accents) n'a pas été essayé. Qui a le module en main peut changer le fichier de réglages.

## 🔌 Interfaces

| Interface | Ce que c'est | Où |
|---|---|---|
| Fichier de réglages | `hmi.cfg` dans la partition `config` : lignes `clé = valeur` ; voir `hmi.cfg.example` et le tableau ci-dessus | `core/hmi_config.hpp` |
| Entrée d'arrêt matériel | un GPIO (`HMI_HW_STOP_GPIO` dans menuconfig, actif bas par défaut, avec pull-up pour qu'un fil ouvert compte comme enfoncé) | `main/board_p4.cpp` |
| Écran tactile | MIPI-DSI 800x1280 via LVGL 9 | `main/ui.cpp` |
| Son | codec ES8311 avec amplificateur de haut-parleur et entrée micro ES7210 | `main/board_p4.cpp` |
| microSD | la racine de la carte à `/sdcard` : les fichiers de perçage, de défonçage, Gerber, DXF et ZIP sont listés et lus, des rapports sont écrits ; seuls des noms de fichier simples sont acceptés | `core/sdfiles.hpp`, `main/board_p4.cpp` |
| EH3D-SERVER | HTTP avec le jeton d'accès : `GET /api/v1/fleet` toutes les `poll_s` secondes, `POST /api/v1/machines/:id/jobs`, `POST /api/v1/jobs/:id/start`, `confirm` et `cancel` | `core/server_link.hpp`, `main/server_link.cpp` |

## ⚙️ Configuration

Tout ce qui est ajusté est une ligne de `hmi.cfg` (copiez `hmi.cfg.example` dans la partition `config`) ; une valeur hors plage est signalée dans le journal et la valeur par défaut reste. Les valeurs ci-dessous sont celles par défaut ; chaque défaut de sécurité est le défaut prudent.

| Réglage | Défaut | Plage | Ce que ça fait |
|---|---|---|---|
| `language` | `en` | `en es it de fr pt zh ja` | la langue des écrans et de la voix |
| `confirm_moves` | on | on / off | demande une confirmation pour déplacer : à désactiver seulement sur le banc |
| `confirm_timeout_ms` | 8000 | 1000 - 60000 | combien de temps une confirmation attend |
| `jog_max_mm` | 20 | 1 - 200 | le plus long jog que l'écran envoie |
| `spindle_max_rpm` | 24000 | 0 - 65535 | la vitesse de broche maximale demandée (0 : ne démarre jamais la broche) |
| `hw_stop_required` | on | on / off | ne pilote la machine que si l'entrée d'arrêt matériel est câblée et relâchée |
| `voice_orders` | off | on / off | accepte les ordres vocaux |
| `voice_confirm` | off | on / off | accepte un oui parlé comme confirmation d'un déplacement |
| `token` | none | 8 - 64 characters | le jeton d'accès au serveur ; en écriture seule, jamais affiché ni journalisé |
| `orientation` | `auto` | `auto landscape portrait` | portrait ou paysage ; `auto` suit le montage du module |
| `poll_s` | 3 | 1 - 60 | à quelle fréquence l'écran demande la flotte au serveur |

## 🚀 Démarrage rapide

Il faut un compilateur C++17 et CMake pour les tests sur ordinateur, et ESP-IDF 5.5.5 pour le firmware (la cible ESP32-P4 du module).

```bash
build.bat            # the host tests (Windows)  -  ./build.sh on Linux
python -I tools/ci_validate.py
```

Le firmware n'a pas encore été compilé : le moment venu, `idf.py set-target esp32p4 && idf.py build` produit l'image et une partition nommée `config` contient `hmi.cfg`.

## 🧪 Tests

Les tests sur ordinateur (`build.bat` ou `./build.sh`, puis `ctest`) sont deux programmes et tout passe : `test_hmi` (638 vérifications) sur le fichier de réglages, la porte (l'arrêt passe toujours, verrou de l'arrêt matériel, confirmation, délai et débordement de l'horloge, règles vocales, interrupteurs), les ordres et alertes vocaux et les textes d'écran en huit langues ; et `test_workflow` (115 vérifications) sur l'orientation et le toucher, les fichiers de la carte (types, noms sûrs, tri), le lecteur JSON, les requêtes et réponses du serveur et le déroulement d'un travail de la carte à sa fin. Les deux autres programmes couvrent les claviers (`test_keyboard`, 137 vérifications : chaque réglage a son clavier, les plages du pavé sont celles du fichier de réglages, les décimales, le signe, les flèches, le clavier complet de chaque langue, la majuscule, les symboles et la longueur) et les avis enregistrés (`test_notices`, 66 vérifications : le réglage, le fichier de chaque langue et genre).

## 🏗️ Compilation et publication

La version vit dans `eh3d.project.json`, et chaque version a son entrée dans [CHANGELOG.md](CHANGELOG.md). `build.sh` / `build.bat` compilent et lancent les tests sur ordinateur et `test.sh` / `test.bat` lancent les contrôles du dépôt. Le firmware se compile en dehors des scripts de ce dépôt, dans une fenêtre visible, une compilation à la fois.

## 🩺 Dépannage

| Symptôme | Que faire |
|---|---|
| Rien ne bouge et l'écran dit que l'arrêt matériel est enfoncé ou non acquitté | Relâchez le bouton puis appuyez sur le bouton d'acquittement de l'écran : un arrêt relâché ne se lève pas tout seul. |
| L'écran dit que la machine n'est pas armée | Dans cette version il n'y a pas encore de liaison avec la machine ; la porte refuse tout déplacement tant qu'une liaison ne l'arme pas. |
| L'écran dit que l'entrée d'arrêt matériel n'est pas câblée | Câblez le bouton d'arrêt à la broche fixée dans menuconfig (`HMI_HW_STOP_GPIO`), ou désactivez `hw_stop_required` uniquement sur le banc. |
| Un ordre vocal ne fait rien | Les ordres vocaux sont désactivés par défaut : mettez `voice_orders = on`. Ceux qui déplacent ou font tourner demandent un toucher à l'écran sauf si `voice_confirm` est activé. |

## 📂 Structure du dépôt

<p align="center">
  <img src="images/repository-structure.svg" alt="Structure du dépôt EH3D-NODE-HMI-GUI" width="100%">
</p>

<details>
<summary>Version texte de l'arborescence</summary>

```text
EH3D-NODE-HMI-GUI/
├── core/
│   ├── hmi_config.hpp lang.hpp                       le fichier de réglages et ses valeurs sûres
│   ├── gate.hpp                                      la porte : confirmation, verrou d'arrêt
│   ├── machine.hpp                                   le type de machine
│   ├── voice.hpp                                     ordres et alertes vocaux en huit langues
│   ├── orientation.hpp sdfiles.hpp                   portrait et paysage, fichiers de la carte
│   ├── server_link.hpp workflow.hpp job_memory.hpp   client du serveur et déroulement d'un travail
│   └── ui_text.hpp keyboard.hpp notices.hpp          textes, claviers et avis
├── main/
│   ├── app_main.cpp                                  démarrage, réglages, la boucle de la porte
│   ├── ui.cpp ui.hpp                                 les écrans LVGL et le bouton d'arrêt
│   ├── board_p4.cpp board.hpp                        le module : écran, son, entrée d'arrêt
│   ├── server_link.cpp server_link.hpp               les sockets du client du serveur
│   └── idf_component.yml Kconfig.projbuild           liste des composants et broches de menuconfig
├── tests/                                            tests sur ordinateur des réglages, de la porte, de la voix et des textes d'écran
├── docs/                                             les notes matérielles et les sources des deux images
├── tools/                                            contrôle CI, contrôle du README et outils des images
├── CMakeLists.txt                                    le projet ESP-IDF
├── partitions.csv                                    la répartition de la flash à deux emplacements
├── hmi.cfg.example                                   chaque réglage avec sa valeur par défaut et sa plage
├── sdkconfig.defaults                                réglages ESP-IDF pour le module ESP32-P4
├── sdkconfig.board.p4                                réglages de la carte
├── dependencies.lock                                 les versions exactes des composants
├── images/                                           bannière et images d'Architecture et de Structure du dépôt
├── .github/                                          workflows de CI et de publication, modèles d'issues et de pull requests
├── build.sh build.bat                                compile et exécute les tests sur ordinateur dans un panneau en direct
├── eh3d.project.json                                 manifeste : version, rôle, maturité et commande de compilation
├── CHANGELOG.md                                      ce qui a changé dans chaque version
├── SECURITY.md SUPPORT.md                            signaler une vulnérabilité et poser des questions
├── CONTRIBUTING.md CODE_OF_CONDUCT.md LICENSE        contribuer, code de conduite et licence
└── bump_manifest_version.py                          utilitaire de version du manifeste
```

</details>

## 🗺️ Feuille de route et limites connues

**Limites connues aujourd'hui**

* Rien n'a tourné sur un module : le firmware n'a été ni compilé, ni flashé, ni essayé, et le paquet de support de carte demandé peut nécessiter des changements pour ce module.
* L'écran n'atteint la machine que par un travail d'EH3D-SERVER ; ses propres déplacements (jog, origine, broche) interrogent la porte puis attendent la liaison avec le nœud, pas encore écrite. Le client du serveur et la carte n'ont jamais tourné sur un module.
* La broche de l'arrêt matériel et le contrôleur tactile ne sont pas encore fixés : il faut les vérifier sur le schéma de l'unité.
* Le reconnaisseur vocal et le haut-parleur ne sont pas encore dans le firmware ; les ordres et alertes sont testés comme du texte.
* La logique des programmes de bureau (formats, contrôleur, flotte) n'est pas réutilisée : les écrans suivent le programme de bureau, mais les règles sont réécrites en C++.

**La suite**

* Compiler le firmware et démarrer le module : écran, tactile et son.
* Écrire la liaison avec EH3D-SERVER et avec le nœud, et remplir les autres onglets des écrans.
* Ajouter le reconnaisseur vocal et le haut-parleur, et les essayer avec annulation d'écho.
* Câbler un vrai bouton d'arrêt et essayer le verrou sur une machine.

## 🌍 Langues

Le README et les textes existent en anglais, espagnol, français, italien, allemand, chinois, japonais et portugais européen.

## 🔗 Projets liés

* [EH3D-DRILLPRO](https://github.com/JuanenRac/EH3D-DRILLPRO): le programme de bureau que suivent les écrans
* [EH3D-ROUTEPRO](https://github.com/JuanenRac/EH3D-ROUTEPRO): le programme de défonçage ; partage le fichier de machine et la bibliothèque du protocole
* [EH3D-SERVER](https://github.com/JuanenRac/EH3D-SERVER): le centre de la famille : il exécute les travaux de chaque client
* [EH3D-NODE-BUNGARD](https://github.com/JuanenRac/EH3D-NODE-BUNGARD): le nœud sur le port série de l'unité de commande de la machine
* [EH3D-COMMON](https://github.com/JuanenRac/EH3D-COMMON): les parties communes de la famille (formats, contrôleur, flotte, thèmes)

## 📚 Documentation

* Les réglages : `hmi.cfg.example` liste chaque clé avec sa valeur par défaut et sa plage.
* Le matériel : les broches du module et les notes qui accompagnent la démo du fabricant, dans `docs/HARDWARE.md`.
* [Journal des modifications](CHANGELOG.md)

## 🤝 Contribuer et communauté

* Avant une pull request, lancez les tests sur ordinateur (`./build.sh` ou `build.bat`) et `python -I tools/ci_validate.py` : tout doit passer. Voir [CONTRIBUTING.md](CONTRIBUTING.md) et le [code de conduite](CODE_OF_CONDUCT.md).
* Le README existe en huit langues : une modification du texte se fait dans les huit fichiers, et les deux images sont dessinées depuis `docs/architecture.json` et `docs/structure.json` par `tools/make_architecture.py` et `tools/make_structure.py`.
* Rien de ce qui permet à l'écran de déplacer une machine ne change sans un test de la porte qui le couvre et une note de ce qui a été observé sur le matériel.
* Signalez une vulnérabilité en privé : [SECURITY.md](SECURITY.md). Questions, idées et signalements : electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTEUR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENCE

Logiciel : GPL-3.0-or-later. Voir [LICENSE](LICENSE). Les conceptions matérielles de la suite utilisent la CERN OHL-S.
