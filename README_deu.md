<p align="center">
  <img src="images/EH3D_BANNER.svg" alt="EH3D-NODE-HMI-GUI banner" width="100%">
</p>

# 🖥️ EH3D-NODE-HMI-GUI

<p align="center"><a href="README.md">🇺🇸 English</a> | <a href="README_spa.md">🇪🇸 Español</a> | <a href="README_fra.md">🇫🇷 Français</a> | <a href="README_ita.md">🇮🇹 Italiano</a> | 🇩🇪 <b>Deutsch</b> | <a href="README_zho.md">🇨🇳 中文</a> | <a href="README_jpn.md">🇯🇵 日本語</a> | <a href="README_por.md">🇵🇹 Português</a></p>

### Ein 10,1-Zoll-Touchscreen mit ESP32-P4, der eine CNC-Maschine, einen Laser oder eine Pick-and-Place-Maschine allein steuert: die Bildschirme und der Ablauf der Desktop-Programme in acht Sprachen, gesprochene Hinweise und Befehle und ein Hardware-Stopp, der nicht über den Bildschirm läuft.

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Stack-C%2B%2B17%20%2B%20ESP--IDF-00599C.svg" alt="C++17 and ESP-IDF">
  <img src="https://img.shields.io/badge/Languages-8-00E5FF.svg" alt="Languages">
  <img src="https://img.shields.io/badge/Version-0.0.1-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-FFB020.svg" alt="Maturity">
</p>

---

**Ehrlichkeitscheck - was heute läuft:** Die Regeln des Bildschirms sind in C++17 geschrieben und auf einem Computer getestet: die Einstellungsdatei mit sicheren Standardwerten, das Tor, das jede Bewegung und jeder Spindelbefehl passieren muss (eine Bestätigung auf dem Bildschirm, die Hardware-Stopp-Sperre, die scharfe Verbindung), die gesprochenen Befehle und Hinweise in acht Sprachen, die Bildschirmtexte, die Hoch- und Querformat-Layouts, die Dateien der microSD-Karte und der Client von EH3D-SERVER (die Anfragen, das Lesen der Flotte und der Ablauf eines Auftrags von einer Datei der Karte bis zu seinem Ende). Die ESP-IDF-Firmware mit den LVGL-Bildschirmen ist für das Modul Guition JC8012P4A1C geschrieben, **wurde aber weder gebaut noch geflasht noch auf einem Modul ausgeführt**; sie hat nie mit einem echten Server gesprochen, und die Bewegungen der Maschine selbst (Tippen, Referenz) warten auf die Verbindung zum Knoten, die noch nicht geschrieben ist.

---

> **Sicherheitsstatus (Safety und IT-Sicherheit):** Dies ist kein Sicherheitssystem. Der Stopp, der Menschen und Maschine schützt, ist ein Hardware-Taster, der den Antrieb von selbst abschaltet und getrennt von diesem Bildschirm verdrahtet ist; der Bildschirm liest ihn nur, zeigt ihn an und bewegt nichts, solange er gedrückt oder nicht quittiert ist. Eine Bewegung oder ein Spindelbefehl verlangt eine Bestätigung auf dem Bildschirm, ein gesprochener Befehl immer, und gesprochene Befehle sind aus, bis der Bediener sie einschaltet. Benutze diesen Bildschirm nicht zum Betrieb einer Maschine, bevor die ganze Kette an einer echten erprobt wurde. Das Projekt ist unabhängig: Es wird nicht von Bungard Elektronik oder Guition hergestellt, unterstützt oder gebilligt, und Produktnamen erscheinen nur, um zu sagen, mit welchen Maschinen und Modulen es arbeiten soll.

## 📋 Auf einen Blick

| | |
|---|---|
| Version | `0.0.1` |
| Reifegrad | Gerüst |
| Tests | 5 Host-Testprogramme (1018 Prüfungen gezählt) |
| Technologie-Stack | C++17, ESP-IDF 5.5, LVGL 9, ESP32-P4 |
| Auf echter Hardware | Nein: die Firmware wurde weder gebaut noch geflasht, und nichts wurde an eine Maschine angeschlossen |

**Springe zu:** [Überblick](#-überblick) · [Architektur](#-architektur) · [Schnellstart](#-schnellstart) · [Tests](#-tests) · [Struktur](#-struktur-des-repositorys) · [Verwandte](#-verwandte-projekte)

## 🚦 Projektstatus

Jede Zeile ist ein Teil dieses Repositorys, und die Ampel sagt, wie weit er heute ist. Sie beruht auf dem, was das Repository zeigen kann (Tests, Builds, Läufe), nicht auf dem, was geplant ist.

🟢 Fertig und überprüft · 🟡 Funktioniert, mit bekannten Grenzen · 🟠 Teilweise fertig oder noch nicht überprüft · 🔴 Nicht gemacht oder funktioniert nicht

| Bereich | Stand | Was das hier bedeutet |
|---|---|---|
| Software | 🟠 | Das ESP-IDF-Projekt (ESP32-P4, LVGL 9, die Einstellungsdatei in einer eigenen Partition) ist geschrieben; es wurde weder gebaut noch geflasht. |
| Tests | 🟢 | 5 Host-Testprogramme bestehen (`ctest`): 1018 Prüfungen zu Einstellungsdatei, Tor, gesprochenen Befehlen und Hinweisen, den Tastaturen, den aufgenommenen Hinweisen, dem Server-Client und den Bildschirmtexten in acht Sprachen. |
| Sicherheitsregeln | 🟡 | Jede Bewegung und jeder Spindelbefehl läuft durch das Tor: Bestätigung auf dem Bildschirm, Hardware-Stopp-Sperre, die bei losgelassenem Taster nur eine Quittung auf dem Bildschirm löst, keine Bewegung bei nicht scharfer Verbindung. Durch Tests gedeckt; nie mit einem echten Stopp-Taster erprobt. |
| Sprache | 🟡 | Gesprochene Befehle und Hinweise werden in acht Sprachen erkannt und formuliert und als Text getestet; es wurde kein Mikrofon und kein Lautsprecher benutzt. |
| Bildschirme | 🟠 | Die Bildschirme Maschine, Auftrag (microSD), Maschinen und Stationen und Einstellungen sind geschrieben, im Quer- und im Hochformat, im Aussehen des Desktop-Programms; der Reiter Ansicht ist noch eine leere Karte. Es gibt nur eine Zeichnung des Layouts, keine Aufnahme eines Moduls. |
| Echte Hardware | 🔴 | Kein Modul hat diese Firmware ausgeführt, die Verbindung zum Server gibt es noch nicht, und nichts wurde an eine Maschine angeschlossen. |
| Dokumentation | 🟢 | README in acht Sprachen, die Einstellungsreferenz und die Hardware-Hinweise. |
| Server und Karte | 🟡 | Die Dateien der microSD-Karte (Liste, sichere Namen, Arten), die Anfragen an EH3D-SERVER, das Lesen der Flotte und der Ablauf eines Auftrags (bereit senden, über das Tor starten, verfolgen, den Werkzeugwechsel beantworten) sind auf einem Computer mit aufgeschriebenen Serverantworten getestet; die Sockets und der Kartentreiber liefen nie auf einem Modul. |

## 🎯 Überblick

* Das **Tor**: jede Bewegung und jeder Spindelbefehl, per Touch oder Sprache, läuft über eine Stelle, die entscheidet, ob sie ausgeführt, bestätigt oder mit Begründung abgelehnt wird; ein Stopp geht immer durch.
* Die **Einstellungsdatei** `hmi.cfg`: jeder abgestimmte Wert (Sprache, Helligkeit, der Server, die Sicherheitsschalter, die Sprache) ist eine Zeile einer Datei in einer eigenen Partition, nie eine Zahl im Code; ein Wert außerhalb des Bereichs wird gemeldet und der sichere Standard bleibt.
* Die **Sprache**: gesprochene Hinweise (Auftrag fertig, Werkzeugwechsel, Fehler, Verbindung verloren, Hardware-Stopp) und eine kurze feste Liste gesprochener Befehle in acht Sprachen; Befehle, die nur anhalten, werden auch in einem Satz verstanden, die zum Bewegen oder Drehen müssen vollständig gesagt werden und verlangen immer eine Bestätigung.
* Die **Bildschirme**: die Maschine (Tippen, Referenz, Spindel, Start, Stopp), der Auftrag, die Ansicht, die Maschinen und Stationen des Servers und die Einstellungen, in der Reihenfolge und mit den Namen des Desktop-Programms; der Stopp-Knopf ist immer oben.
* Das **Modul**: das Guition JC8012P4A1C (ESP32-P4 mit 32 MB externem RAM, 10,1-Zoll-MIPI-DSI-Touchdisplay mit 800x1280, ein ESP32-C6 für WLAN, der Codec ES8311 mit Lautsprecherverstärker und der Mikrofoneingang ES7210). Das Panel der Einheit ist das neue.
* **Hoch- oder Querformat:** die Einstellung `orientation` (`auto`, `landscape`, `portrait`) dreht das Bild des Panels: im Querformat (1280 x 800) stehen Auftrag, Draufsicht und Tipp-Pad in drei Spalten über der Positionsleiste, im Hochformat (800 x 1280) sind sie gestapelt; die Berührung trifft in beiden dort, wo der Bildschirm berührt wird.
* **microSD und Server als ein Ablauf:** die Karte listet die Bohr- und Fräsdateien; der Bildschirm sendet die gewählte Datei als bereit wartenden Auftrag an EH3D-SERVER, der Bediener startet ihn über das Tor, und Fortschritt, Halte für den Werkzeugwechsel und das Ende kommen in Echtzeit aus der Flotte zurück, wie bei den Desktop-Programmen und dem Knoten. Die Antwort des Servers zu jedem Auftrag wird auf der Karte gespeichert.

## 🧩 Rolle in EH3D

EH3D-NODE-HMI-GUI ist der eigene Bildschirm des Bedieners für eine CNC-Maschine, einen Laser oder eine Pick-and-Place-Maschine (dieselbe Hardware wie EH3D-NODE-BUNGARD-HMI): Er ersetzt den PC neben der Maschine und ist ein weiterer Client von EH3D-SERVER, wie EH3D-DRILLPRO, EH3D-ROUTEPRO und EH3D-BUNGARD-GUI. Ein Auftrag, den der Bildschirm von der microSD-Karte sendet, wird vom Server geplant und ist im selben Moment in den Desktop-Programmen; ein von einem Desktop-Programm gestarteter Auftrag zeigt hier seinen Fortschritt; der Server führt ihn über den Knoten der Maschine an der Maschine aus. Die direkten Bewegungen des Bildschirms (Tippen, Referenz) werden denselben Knoten nutzen, und diese Verbindung ist noch nicht geschrieben.

## 🏛️ Architektur

<p align="center">
  <img src="images/architecture.svg" alt="Architektur von EH3D-NODE-HMI-GUI: wer es nutzt, was es enthält und wovon es abhängt" width="100%">
</p>

Wie die Teile dieses Repositorys zusammenpassen: wer mit ihm spricht, was es enthält und wovon es abhängt. Das Bild wird aus `docs/architecture.json` gezeichnet und gegen den Code geprüft.

## 🖼️ Screenshots

<p align="center">
  <img src="images/screenshots/screen-machine.png" alt="machine screen lying down" width="100%">
</p>

<p align="center"><em>Der Maschinenbildschirm im Querformat, auf Englisch: der Auftrag, die Draufsicht und das Tipp-Pad in drei Spalten, die Position und der Stopp-Knopf in der Leiste. Eine Zeichnung des Layouts aus den Texten der Firmware, keine Aufnahme eines Moduls.</em></p>

<p align="center">
  <img src="images/screenshots/screen-portrait.png" alt="machine screen standing up" width="40%">
</p>

<p align="center"><em>Derselbe Bildschirm im Hochformat, auf Deutsch: die Karten sind gestapelt und die Positionsleiste bleibt unten.</em></p>

<p align="center">
  <img src="images/screenshots/screen-card.png" alt="files of the microSD card" width="100%">
</p>

<p align="center"><em>Die Auftragsseite (hier auf Italienisch): die Bohr- und Fräsdateien der microSD-Karte, die gewählte Datei und der Knopf, der sie als bereit wartenden Auftrag an den Server sendet.</em></p>

<p align="center">
  <img src="images/screenshots/screen-fleet.png" alt="machines and stations" width="100%">
</p>

<p align="center"><em>Maschinen und Stationen (hier auf Portugiesisch): alle Maschinen des Servers nach Station, mit Zustand und Fortschritt ihres Auftrags, so wie die Desktop-Programme sie sehen.</em></p>

<p align="center">
  <img src="images/screenshots/screen-confirm.png" alt="confirmation of a move" width="100%">
</p>

<p align="center"><em>Eine Bewegung verlangt eine Bestätigung, bevor etwas geschieht (hier auf Spanisch).</em></p>

<p align="center">
  <img src="images/screenshots/screen-stop.png" alt="hardware stop pressed" width="100%">
</p>

<p align="center"><em>Der Hardware-Stopp ist gedrückt: Der Bildschirm sagt es, dimmt die Knöpfe, die bewegen, und bewegt nichts, bis das Lösen quittiert ist (hier auf Japanisch).</em></p>

## ✨ Funktionen

* **Ein Stopp, den man nicht vergessen kann:** Der Stopp-Knopf ist immer oben und funktioniert immer; der Hardware-Stopp rastet ein, und nur eine Quittung auf dem Bildschirm bei losgelassenem Taster löst ihn.
* **Bestätigung vor der Bewegung:** Eine Bewegung oder ein Spindelbefehl zeigt, was sie tun wird, und wartet mit Zeitlimit auf eine Berührung; eine neue Anfrage oder eine verlorene Verbindung verwirft sie.
* **Acht Sprachen, auch per Sprache:** Englisch, Spanisch, Italienisch, Deutsch, Französisch, europäisches Portugiesisch, Chinesisch und Japanisch auf den Bildschirmen, in den gesprochenen Hinweisen und in den gesprochenen Befehlen.
* **Standardmäßig sicher:** Jeder Sicherheitsschalter hat seine Einstellung und beginnt in der vorsichtigen Stellung: gesprochene Befehle aus, gesprochene Bestätigung aus, Bestätigung an, Hardware-Stopp erforderlich.
* **Die passende Tastatur für jede Einstellung:** eine Zahl wird auf einem Zahlenblock getippt, der Bereich und Nachkommastellen dieser Einstellung kennt (die Pfeile verschieben um eine Einheit, ein Wert außerhalb des Bereichs wird nicht angenommen); ein Text, etwa die Adresse des Servers oder das Token, auf einer vollen Tastatur mit den Tasten der gewählten Sprache (Umlaute, Akzente, Tilde, Cedille), mit Umschalten für einen Buchstaben und einer Symbolebene; Schalter und Listen brauchen keine Tastatur.
* **Tasten mit Symbol:** das Jog-Feld, Referenzfahrt, Spindel, Pause, Start, Stopp und die Antworten auf eine Frage nutzen Symbole und den Buchstaben der Achse statt Wörter, in den Farben des Desktop-Programms (grün zum Starten, blau für Pause, rot zum Stoppen).
* **Aufgenommene Hinweise:** das Auftragsende und die Werkzeuganforderung können von einer aufgenommenen Stimme gesagt werden, in der eingestellten Sprache und dem Geschlecht (`sound_finish = voice:es:f`), aus dem Ordner `voices` der Karte; die Aufnahmen sind die von EH3D-BUNGARD-GUI. Der Lautsprecher ist in der Firmware noch nicht angeschlossen.
* **Der Auftrag wird nach einem Neustart wieder verfolgt:** sobald der Bediener einen Auftrag startet, merkt sich der Bildschirm im Flash, welcher es ist (seine ID, die Maschine, sein Name); nach einem Stromausfall oder Neustart verfolgt er denselben Auftrag, die nächste Abfrage des Servers sagt, ob er noch läuft, an einem Werkzeugwechsel wartet oder zu Ende ist, und die Abbrechen-Taste lässt ihn los. Der Auftrag selbst lebt im Server. Auf einem Computer geschrieben und getestet; die Firmware lief nie auf einem Modul.
* **Die Art der Maschine:** `machine_kind` in der Einstellungsdatei (`cnc`, `laser` oder `pnp`) bestimmt, was der Bildschirm befehlen darf. Nur eine CNC-Maschine bekommt die Spindeltaste, und die Schranke lehnt einen Spindelbefehl an einem Laser oder einer Pick-and-Place-Maschine ab, was die anderen Einstellungen auch sagen: Der Bildschirm löst nie einen Laser aus und startet nie von selbst einen Pick-and-Place-Kopf.

## 🔒 Sicherheit und Schutz

| Bedrohung | Abwehr |
|---|---|
| Eine Berührung oder ein gesprochener Befehl bewegt die Maschine versehentlich | eine Bewegung verlangt eine Bestätigung auf dem Bildschirm mit Zeitlimit; ein gesprochener Befehl immer; ein gesprochenes Ja bestätigt nur bei eingeschalteter Einstellung `voice_confirm` (standardmäßig aus) |
| Der Bildschirm bewegt bei gedrücktem Hardware-Stopp | der Hardware-Stopp ist eine Sperre: gedrückt, oder gelöst aber nicht auf dem Bildschirm quittiert, wird jede Bewegung und jeder Spindelbefehl abgelehnt und Wartendes verworfen; ein Stopp während einer wartenden Bestätigung bricht sie ab |
| Eine Maschine, die nicht bereit ist | nichts bewegt sich bei nicht scharfer Verbindung, bei nicht verdrahtetem Hardware-Stopp-Eingang (`hw_stop_required`, standardmäßig an) oder bei Spindelgrenze 0 |
| Ein gesprochenes Wort wird für einen Befehl gehalten | Befehle stammen aus einer kurzen festen Liste je Sprache, nie aus freiem Text; ein Wort, das nur einen Befehl enthält (`stopwatch`), ist kein Befehl; gesprochene Befehle sind aus, bis der Bediener sie einschaltet |
| Ein Tippfehler in den Einstellungen | ein Wert außerhalb des Bereichs oder ein unbekannter Schlüssel wird gemeldet und der sichere Standard bleibt; kein Schalter öffnet sich durch einen Fehler |
| Ein Geheimnis in der Firmware oder im Repository | nichts Geheimes ist einkompiliert; das Token ist eine Einstellung in seiner Partition und wird nie angezeigt oder protokolliert |

**Was nicht abgesichert ist:** Der Bildschirm ist kein Sicherheitsgerät und kann den Antrieb der Maschine nicht abschalten; das muss der Hardware-Stopp von selbst tun. Es gibt noch keine Verbindung zum Server, daher kann der Bildschirm heute keine Maschine bewegen. Die Spracherkennung wird als Text getestet; der echte Mikrofonweg (Echounterdrückung, Rauschen, Akzente) wurde nicht erprobt. Wer das Modul in der Hand hat, kann die Einstellungsdatei ändern.

## 🔌 Schnittstellen

| Schnittstelle | Was es ist | Wo |
|---|---|---|
| Einstellungsdatei | `hmi.cfg` in der Partition `config`: Zeilen `Schlüssel = Wert`; siehe `hmi.cfg.example` und die Tabelle oben | `core/hmi_config.hpp` |
| Hardware-Stopp-Eingang | ein GPIO (`HMI_HW_STOP_GPIO` in menuconfig, standardmäßig aktiv niedrig, mit Pull-up, damit ein offenes Kabel als gedrückt gilt) | `main/board_p4.cpp` |
| Touchdisplay | 800x1280 MIPI-DSI über LVGL 9 | `main/ui.cpp` |
| Ton | Codec ES8311 mit Lautsprecherverstärker und Mikrofoneingang ES7210 | `main/board_p4.cpp` |
| microSD | die Wurzel der Karte unter `/sdcard`: Bohr-, Fräs-, Gerber-, DXF- und ZIP-Dateien werden gelistet und gelesen, Berichte geschrieben; nur einfache Dateinamen werden angenommen | `core/sdfiles.hpp`, `main/board_p4.cpp` |
| EH3D-SERVER | HTTP mit dem Zugriffstoken: `GET /api/v1/fleet` alle `poll_s` Sekunden, `POST /api/v1/machines/:id/jobs`, `POST /api/v1/jobs/:id/start`, `confirm` und `cancel` | `core/server_link.hpp`, `main/server_link.cpp` |

## ⚙️ Konfiguration

Alles Abgestimmte ist eine Zeile in `hmi.cfg` (kopiere `hmi.cfg.example` in die Partition `config`); ein Wert außerhalb des Bereichs wird im Protokoll gemeldet und der Standard bleibt. Die Werte unten sind die Standardwerte; jeder Sicherheitsstandard ist der vorsichtige.

| Einstellung | Standard | Bereich | Was sie tut |
|---|---|---|---|
| `language` | `en` | `en es it de fr pt zh ja` | die Sprache der Bildschirme und der Stimme |
| `confirm_moves` | on | on / off | verlangt Bestätigung zum Bewegen: nur auf dem Prüfstand ausschalten |
| `confirm_timeout_ms` | 8000 | 1000 - 60000 | wie lange eine Bestätigung wartet |
| `jog_max_mm` | 20 | 1 - 200 | die längste einzelne Tippbewegung, die der Bildschirm sendet |
| `spindle_max_rpm` | 24000 | 0 - 65535 | die höchste angeforderte Spindeldrehzahl (0: startet nie die Spindel) |
| `hw_stop_required` | on | on / off | steuert die Maschine nur bei verdrahtetem und gelöstem Hardware-Stopp-Eingang |
| `voice_orders` | off | on / off | nimmt gesprochene Befehle an |
| `voice_confirm` | off | on / off | nimmt ein gesprochenes Ja als Bestätigung einer Bewegung an |
| `token` | none | 8 - 64 characters | das Zugriffstoken des Servers; nur schreibbar, nie angezeigt oder protokolliert |
| `orientation` | `auto` | `auto landscape portrait` | Hoch- oder Querformat; `auto` folgt der Einbaulage des Moduls |
| `poll_s` | 3 | 1 - 60 | wie oft der Bildschirm die Flotte beim Server abfragt |

## 🚀 Schnellstart

Du brauchst einen C++17-Compiler und CMake für die Host-Tests und ESP-IDF 5.5.5 für die Firmware (das Ziel ESP32-P4 des Moduls).

```bash
build.bat            # the host tests (Windows)  -  ./build.sh on Linux
python -I tools/ci_validate.py
```

Die Firmware wurde noch nicht gebaut: wenn es so weit ist, erzeugt `idf.py set-target esp32p4 && idf.py build` das Abbild, und eine Partition namens `config` enthält `hmi.cfg`.

## 🧪 Tests

Die Host-Tests (`build.bat` oder `./build.sh`, dann `ctest`) sind zwei Programme und alles besteht: `test_hmi` (638 Prüfungen) zu Einstellungsdatei, Tor (Stopp geht immer, Hardware-Stopp-Sperre, Bestätigung, Zeitlimit und Uhrenüberlauf, Sprachregeln, Schalter), gesprochenen Befehlen und Hinweisen und Bildschirmtexten in acht Sprachen; und `test_workflow` (115 Prüfungen) zu Ausrichtung und Berührung, Dateien der Karte (Arten, sichere Namen, Sortierung), JSON-Leser, Anfragen und Antworten des Servers und dem Ablauf eines Auftrags von der Karte bis zum Ende. Die beiden anderen Programme decken die Tastaturen ab (`test_keyboard`, 137 Prüfungen: jede Einstellung hat ihre Tastatur, die Bereiche des Zahlenblocks sind die der Einstellungsdatei, die Nachkommastellen, das Vorzeichen, die Pfeile, die volle Tastatur jeder Sprache, die Umschaltung, die Symbole und die Länge) und die aufgenommenen Hinweise (`test_notices`, 66 Prüfungen: die Einstellung, die Datei jeder Sprache und jedes Geschlechts).

## 🏗️ Build und Veröffentlichung

Die Version steht in `eh3d.project.json`, und jede Version hat ihren Eintrag in [CHANGELOG.md](CHANGELOG.md). `build.sh` / `build.bat` bauen und führen die Host-Tests aus, `test.sh` / `test.bat` die Prüfungen des Repositorys. Die Firmware wird außerhalb der Skripte dieses Repositorys gebaut, in einem sichtbaren Fenster, immer nur ein Build.

## 🩺 Fehlerbehebung

| Symptom | Was zu tun ist |
|---|---|
| Nichts bewegt sich und der Bildschirm meldet, der Hardware-Stopp sei gedrückt oder nicht quittiert | Taster loslassen und dann die Quittierungstaste auf dem Bildschirm drücken: ein gelöster Stopp wird nicht von selbst aufgehoben. |
| Der Bildschirm meldet, die Maschine sei nicht scharf | In dieser Version gibt es noch keine Verbindung zur Maschine; das Tor lehnt jede Bewegung ab, bis eine Verbindung sie scharf macht. |
| Der Bildschirm meldet, der Hardware-Stopp-Eingang sei nicht verdrahtet | Den Stopp-Taster an den in menuconfig gesetzten Pin (`HMI_HW_STOP_GPIO`) anschließen oder `hw_stop_required` nur auf dem Prüfstand ausschalten. |
| Ein gesprochener Befehl bewirkt nichts | Gesprochene Befehle sind standardmäßig aus: `voice_orders = on` setzen. Befehle zum Bewegen oder Drehen verlangen eine Berührung, außer `voice_confirm` ist an. |

## 📂 Struktur des Repositorys

<p align="center">
  <img src="images/repository-structure.svg" alt="Struktur des Repositorys EH3D-NODE-HMI-GUI" width="100%">
</p>

<details>
<summary>Textversion des Baums</summary>

```text
EH3D-NODE-HMI-GUI/
├── core/
│   ├── hmi_config.hpp lang.hpp                       die Einstellungsdatei und ihre sicheren Standardwerte
│   ├── gate.hpp                                      das Tor: Bestätigung, Stopp-Sperre
│   ├── machine.hpp                                   die Art der Maschine
│   ├── voice.hpp                                     gesprochene Befehle und Hinweise in acht Sprachen
│   ├── orientation.hpp sdfiles.hpp                   Hoch- und Querformat, Dateien der Karte
│   ├── server_link.hpp workflow.hpp job_memory.hpp   Client des Servers und Ablauf eines Auftrags
│   └── ui_text.hpp keyboard.hpp notices.hpp          Texte, Tastaturen und Hinweise
├── main/
│   ├── app_main.cpp                                  Start, Einstellungen, die Schleife des Tors
│   ├── ui.cpp ui.hpp                                 die LVGL-Bildschirme und der Stopp-Knopf
│   ├── board_p4.cpp board.hpp                        das Modul: Display, Ton, Stopp-Eingang
│   ├── server_link.cpp server_link.hpp               die Sockets des Server-Clients
│   └── idf_component.yml Kconfig.projbuild           Komponentenliste und menuconfig-Pins
├── tests/                                            Host-Tests von Einstellungen, Tor, Sprache und Bildschirmtexten
├── docs/                                             die Hardware-Hinweise und die Quellen der beiden Bilder
├── tools/                                            CI-Prüfung, README-Prüfung und Bildwerkzeuge
├── CMakeLists.txt                                    das ESP-IDF-Projekt
├── partitions.csv                                    die Flash-Aufteilung mit zwei Slots
├── hmi.cfg.example                                   jede Einstellung mit Standardwert und Bereich
├── sdkconfig.defaults                                ESP-IDF-Einstellungen für das ESP32-P4-Modul
├── sdkconfig.board.p4                                Einstellungen der Platine
├── dependencies.lock                                 die genauen Versionen der Komponenten
├── images/                                           Banner und die Bilder von Architektur und Repository-Struktur
├── .github/                                          CI- und Release-Workflows, Vorlagen für Issues und Pull Requests
├── build.sh build.bat                                baut und führt die Host-Tests in einem Live-Panel aus
├── eh3d.project.json                                 Manifest: Version, Rolle, Reifegrad und Build-Befehl
├── CHANGELOG.md                                      was sich in jeder Version geändert hat
├── SECURITY.md SUPPORT.md                            Schwachstellen melden und Fragen stellen
├── CONTRIBUTING.md CODE_OF_CONDUCT.md LICENSE        Mitwirken, Verhaltenskodex und Lizenz
└── bump_manifest_version.py                          Versionswerkzeug des Manifests
```

</details>

## 🗺️ Roadmap und bekannte Grenzen

**Bekannte Grenzen heute**

* Nichts hat auf einem Modul gelaufen: Die Firmware wurde weder gebaut noch geflasht noch erprobt, und das verlangte Board-Support-Paket braucht für dieses Modul vielleicht Änderungen.
* Der Bildschirm erreicht die Maschine nur über einen Auftrag von EH3D-SERVER; seine eigenen Bewegungen (Tippen, Referenz, Spindel) fragen das Tor und warten dann auf die Verbindung zum Knoten, die noch nicht geschrieben ist. Der Client des Servers und die Karte liefen nie auf einem Modul.
* Der Pin des Hardware-Stopps und der Touch-Controller sind noch nicht festgelegt: Sie müssen am Schaltplan der Einheit geprüft werden.
* Spracherkenner und Lautsprecher sind noch nicht in der Firmware; Befehle und Hinweise werden als Text getestet.
* Die Logik der Desktop-Programme (Formate, Controller, Flotte) wird nicht wiederverwendet: Die Bildschirme folgen dem Desktop-Programm, die Regeln sind aber in C++ neu geschrieben.

**Als Nächstes**

* Die Firmware bauen und das Modul starten: Display, Touch und Ton.
* Die Verbindung zu EH3D-SERVER und zum Knoten schreiben und die übrigen Reiter der Bildschirme füllen.
* Spracherkenner und Lautsprecher hinzufügen und mit Echounterdrückung erproben.
* Einen echten Stopp-Taster verdrahten und die Sperre an einer Maschine erproben.

## 🌍 Sprachen

README und Texte gibt es auf Englisch, Spanisch, Französisch, Italienisch, Deutsch, Chinesisch, Japanisch und europäischem Portugiesisch.

## 🔗 Verwandte Projekte

* [EH3D-DRILLPRO](https://github.com/JuanenRac/EH3D-DRILLPRO): das Desktop-Programm, dem die Bildschirme folgen
* [EH3D-ROUTEPRO](https://github.com/JuanenRac/EH3D-ROUTEPRO): das Fräsprogramm; teilt die Maschinendatei und die Protokollbibliothek
* [EH3D-SERVER](https://github.com/JuanenRac/EH3D-SERVER): das Zentrum der Familie: führt die Aufträge aller Clients aus
* [EH3D-NODE-BUNGARD](https://github.com/JuanenRac/EH3D-NODE-BUNGARD): der Knoten am seriellen Anschluss der Steuereinheit der Maschine
* [EH3D-COMMON](https://github.com/JuanenRac/EH3D-COMMON): die gemeinsamen Teile der Familie (Formate, Controller, Flotte, Themen)

## 📚 Dokumentation

* Die Einstellungen: `hmi.cfg.example` listet jeden Schlüssel mit Standardwert und Bereich.
* Die Hardware: die Pins des Moduls und die Hinweise zur Demo des Herstellers in `docs/HARDWARE.md`.
* [Änderungsprotokoll](CHANGELOG.md)

## 🤝 Mitwirken und Community

* Führe vor einem Pull Request die Host-Tests (`./build.sh` oder `build.bat`) und `python -I tools/ci_validate.py` aus: alles muss bestehen. Siehe [CONTRIBUTING.md](CONTRIBUTING.md) und den [Verhaltenskodex](CODE_OF_CONDUCT.md).
* Das README gibt es in acht Sprachen: Eine Textänderung wird in allen acht Dateien gemacht, und die beiden Bilder werden aus `docs/architecture.json` und `docs/structure.json` mit `tools/make_architecture.py` und `tools/make_structure.py` gezeichnet.
* Nichts, was dem Bildschirm erlaubt, eine Maschine zu bewegen, ändert sich ohne einen Test des Tors, der es abdeckt, und eine Notiz dessen, was an der Hardware beobachtet wurde.
* Eine Schwachstelle bitte privat melden: [SECURITY.md](SECURITY.md). Fragen, Ideen und Meldungen: electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LIZENZ

Software: GPL-3.0-or-later. Siehe [LICENSE](LICENSE). Die Hardware-Entwürfe der Suite verwenden die CERN OHL-S.
