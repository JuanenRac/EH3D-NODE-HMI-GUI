<p align="center">
  <img src="images/EH3D_BANNER.svg" alt="EH3D-NODE-HMI-GUI banner" width="100%">
</p>

# 🖥️ EH3D-NODE-HMI-GUI

<p align="center"><a href="README.md">🇺🇸 English</a> | <a href="README_spa.md">🇪🇸 Español</a> | <a href="README_fra.md">🇫🇷 Français</a> | 🇮🇹 <b>Italiano</b> | <a href="README_deu.md">🇩🇪 Deutsch</a> | <a href="README_zho.md">🇨🇳 中文</a> | <a href="README_jpn.md">🇯🇵 日本語</a> | <a href="README_por.md">🇵🇹 Português</a></p>

### Un touch screen da 10,1 pollici con un ESP32-P4 che controlla da solo una macchina CNC, un laser o una macchina pick and place: le schermate e il flusso dei programmi desktop, in otto lingue, avvisi e ordini vocali e un arresto hardware che non passa dallo schermo.

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Stack-C%2B%2B17%20%2B%20ESP--IDF-00599C.svg" alt="C++17 and ESP-IDF">
  <img src="https://img.shields.io/badge/Languages-8-00E5FF.svg" alt="Languages">
  <img src="https://img.shields.io/badge/Version-0.0.1-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-FFB020.svg" alt="Maturity">
</p>

---

**Verifica di onestà - cosa gira oggi:** le regole dello schermo sono scritte in C++17 e provate su un computer: il file delle impostazioni con i valori sicuri di default, il varco che ogni movimento e ogni ordine al mandrino deve superare (una conferma sullo schermo, il blocco dell'arresto hardware, il collegamento armato), gli ordini e gli avvisi vocali in otto lingue, i testi delle schermate, i layout in verticale e in orizzontale, i file della microSD e il client di EH3D-SERVER (le richieste, la lettura della flotta e il flusso di un lavoro da un file della scheda fino alla fine). Il firmware ESP-IDF con le schermate LVGL è scritto per il modulo Guition JC8012P4A1C ma **non è stato compilato, scritto né eseguito su un modulo**; non ha mai parlato con un server reale, e i movimenti della macchina stessa (spostamento, origine) aspettano il collegamento con il nodo, non ancora scritto.

---

> **Stato di sicurezza (safety) e di sicurezza informatica:** questo non è un sistema di sicurezza (safety). L'arresto che protegge le persone e la macchina è un pulsante hardware che toglie l'azionamento da solo, cablato a parte rispetto a questo schermo; lo schermo lo legge soltanto, lo mostra e si rifiuta di muovere qualsiasi cosa finché è premuto o non confermato. Un movimento o un ordine al mandrino chiede una conferma sullo schermo, un ordine vocale la chiede sempre, e gli ordini vocali sono spenti finché l'operatore non li attiva. Non usare questo schermo per far funzionare una macchina finché l'intera catena non è stata provata su una reale. Il progetto è indipendente: non è realizzato, approvato né supportato da Bungard Elektronik o Guition, e i nomi dei prodotti compaiono solo per dire con quali macchine e moduli è pensato per funzionare.

## 📋 In breve

| | |
|---|---|
| Versione | `0.0.1` |
| Maturità | Impalcatura |
| Test | 5 programmi di test sul computer (1018 controlli contati) |
| Stack tecnologico | C++17, ESP-IDF 5.5, LVGL 9, ESP32-P4 |
| Su hardware reale | No: il firmware non è stato compilato né scritto e nulla è stato collegato a una macchina |

**Vai a:** [Panoramica](#-panoramica) · [Architettura](#-architettura) · [Avvio rapido](#-avvio-rapido) · [Test](#-test) · [Struttura](#-struttura-del-repository) · [Correlati](#-progetti-correlati)

## 🚦 Stato del progetto

Ogni riga è una parte di questo repository e il semaforo dice a che punto è oggi. È scritta in base a ciò che il repository può mostrare (test, compilazioni, esecuzioni), non a ciò che è previsto.

🟢 Fatto e verificato · 🟡 Funziona, con limiti noti · 🟠 Fatto a metà o non ancora verificato · 🔴 Non fatto o non funzionante

| Area | Stato | Cosa significa qui |
|---|---|---|
| Software | 🟠 | Il progetto ESP-IDF (ESP32-P4, LVGL 9, il file delle impostazioni in una partizione propria) è scritto; non è stato compilato né scritto sul chip. |
| Test | 🟢 | 5 programmi di test sul computer passano (`ctest`): 1018 controlli sul file delle impostazioni, il varco, gli ordini e gli avvisi vocali, le tastiere, gli avvisi registrati, il client del server e i testi delle schermate in otto lingue. |
| Regole di sicurezza | 🟡 | Ogni movimento e ogni ordine al mandrino passa dal varco: conferma sullo schermo, blocco dell'arresto hardware che solo una conferma sullo schermo toglie a pulsante rilasciato, nessun movimento con il collegamento non armato. Coperto da test; mai provato con un vero pulsante di arresto. |
| Voce | 🟡 | Ordini e avvisi vocali sono riconosciuti e formulati in otto lingue e provati come testo; nessun microfono o altoparlante è stato usato. |
| Schermate | 🟠 | Le schermate macchina, lavoro (microSD), macchine e stazioni e impostazioni sono scritte, in orizzontale e in verticale, con l'aspetto del programma desktop; la scheda della vista è ancora una scheda vuota. Esiste solo un disegno del layout, non una cattura di un modulo. |
| Hardware reale | 🔴 | Nessun modulo ha eseguito questo firmware, il collegamento con il server non esiste ancora e nulla è stato collegato a una macchina. |
| Documentazione | 🟢 | README in otto lingue, il riferimento delle impostazioni e le note hardware. |
| Server e scheda | 🟡 | I file della microSD (elenco, nomi sicuri, tipi), le richieste a EH3D-SERVER, la lettura della flotta e il flusso di un lavoro (inviarlo pronto, avviarlo dal varco, seguirlo, rispondere al cambio utensile) sono provati su un computer con risposte del server scritte a mano; i socket e il driver della scheda non hanno mai girato su un modulo. |

## 🎯 Panoramica

* Il **varco**: ogni movimento e ogni ordine al mandrino, da un tocco o dalla voce, passa da un unico punto che decide se eseguirlo, chiedere una conferma o rifiutarlo con un motivo; un arresto passa sempre.
* Il **file delle impostazioni** `hmi.cfg`: ogni valore regolato (lingua, luminosità, il server, gli interruttori di sicurezza, la voce) è una riga di un file in una partizione propria, mai un numero nel codice; un valore fuori intervallo viene segnalato e resta il valore sicuro di default.
* La **voce**: avvisi parlati (lavoro finito, cambio utensile, errore, collegamento perso, arresto hardware) e un breve elenco fisso di ordini vocali in otto lingue; gli ordini che fermano soltanto si capiscono dentro una frase, quelli che muovono o girano vanno detti per intero e chiedono sempre una conferma.
* Le **schermate**: la macchina (spostamento, origine, mandrino, avvio, arresto), il lavoro, la vista, le macchine e stazioni del server e le impostazioni, nell'ordine e con i nomi del programma desktop; il pulsante di arresto è sempre in alto.
* Il **modulo**: il Guition JC8012P4A1C (ESP32-P4 con 32 MB di RAM esterna, display touch MIPI-DSI da 10,1 pollici 800x1280, un ESP32-C6 per il Wi-Fi, il codec ES8311 con amplificatore per altoparlante e l'ingresso microfono ES7210). Il pannello dell'unità è quello nuovo.
* **In piedi o sdraiato:** l'impostazione `orientation` (`auto`, `landscape`, `portrait`) ruota l'immagine del pannello: in orizzontale (1280 x 800) il lavoro, la vista in pianta e il pad di spostamento stanno in tre colonne sopra la barra di posizione, in verticale (800 x 1280) si impilano; il tocco cade dove si tocca lo schermo in entrambi.
* **La microSD e il server, come un solo flusso:** la scheda elenca i file di foratura e fresatura; lo schermo invia il file scelto a EH3D-SERVER come lavoro che aspetta pronto, l'operatore lo avvia dal varco, e l'avanzamento, le soste per il cambio utensile e la fine tornano dalla flotta in tempo reale, come nei programmi desktop e nel nodo. La risposta del server per ogni lavoro viene salvata sulla scheda.

## 🧩 Ruolo in EH3D

EH3D-NODE-HMI-GUI è lo schermo dell'operatore di una macchina CNC, un laser o una macchina pick and place (lo stesso hardware di EH3D-NODE-BUNGARD-HMI): prende il posto del PC accanto alla macchina ed è un client in più di EH3D-SERVER, come EH3D-DRILLPRO, EH3D-ROUTEPRO ed EH3D-BUNGARD-GUI. Un lavoro che lo schermo invia dalla microSD è pianificato dal server ed è nello stesso istante nei programmi desktop; un lavoro avviato da un programma desktop mostra qui il suo avanzamento; il server lo esegue sulla macchina tramite il nodo della macchina. I movimenti diretti dello schermo (spostamento, origine) useranno lo stesso nodo, e quel collegamento non è ancora scritto.

## 🏛️ Architettura

<p align="center">
  <img src="images/architecture.svg" alt="Architettura di EH3D-NODE-HMI-GUI: chi lo usa, che cosa contiene e da che cosa dipende" width="100%">
</p>

Come si incastrano le parti di questo repository: chi parla con esso, cosa contiene e da cosa dipende. Il disegno nasce da `docs/architecture.json` ed è verificato sul codice.

## 🖼️ Schermate

<p align="center">
  <img src="images/screenshots/screen-machine.png" alt="machine screen lying down" width="100%">
</p>

<p align="center"><em>La schermata della macchina in orizzontale, in inglese: il lavoro, la vista in pianta e il pad di spostamento in tre colonne, la posizione e il pulsante di arresto sulla barra. Un disegno del layout fatto con i testi del firmware, non una cattura di un modulo.</em></p>

<p align="center">
  <img src="images/screenshots/screen-portrait.png" alt="machine screen standing up" width="40%">
</p>

<p align="center"><em>La stessa schermata in verticale, in tedesco: le schede si impilano e la barra di posizione resta in basso.</em></p>

<p align="center">
  <img src="images/screenshots/screen-card.png" alt="files of the microSD card" width="100%">
</p>

<p align="center"><em>La pagina del lavoro (qui in italiano): i file di foratura e fresatura della microSD, quello scelto e il pulsante che lo invia al server come lavoro che aspetta pronto.</em></p>

<p align="center">
  <img src="images/screenshots/screen-fleet.png" alt="machines and stations" width="100%">
</p>

<p align="center"><em>Macchine e stazioni (qui in portoghese): tutte le macchine del server per stazione, con il loro stato e l'avanzamento del lavoro, come le vedono i programmi desktop.</em></p>

<p align="center">
  <img src="images/screenshots/screen-confirm.png" alt="confirmation of a move" width="100%">
</p>

<p align="center"><em>Un movimento chiede una conferma prima che accada qualcosa (qui in spagnolo).</em></p>

<p align="center">
  <img src="images/screenshots/screen-stop.png" alt="hardware stop pressed" width="100%">
</p>

<p align="center"><em>L'arresto hardware è premuto: lo schermo lo dice, attenua i pulsanti che muovono e si rifiuta di muovere qualsiasi cosa finché il rilascio non è confermato (qui in giapponese).</em></p>

## ✨ Funzionalità

* **Un arresto che non si può dimenticare:** il pulsante di arresto è sempre in alto e funziona sempre; l'arresto hardware si blocca e solo una conferma sullo schermo, a pulsante rilasciato, lo toglie.
* **Conferma prima del movimento:** un movimento o un ordine al mandrino mostra cosa farà e aspetta un tocco, con un limite di tempo; una nuova richiesta o un collegamento perso la scarta.
* **Otto lingue, anche a voce:** inglese, spagnolo, italiano, tedesco, francese, portoghese europeo, cinese e giapponese nelle schermate, negli avvisi parlati e negli ordini vocali.
* **Sicuro di default:** ogni interruttore di sicurezza ha la sua impostazione e parte nella posizione prudente: ordini vocali spenti, conferma vocale spenta, conferma attiva, arresto hardware richiesto.
* **La tastiera giusta per ogni impostazione:** un numero si scrive su un tastierino numerico che conosce l'intervallo e i decimali di quell'impostazione (le frecce lo spostano di un'unità, un valore fuori intervallo non è accettato); un testo, come l'indirizzo del server o il token, su una tastiera completa con i tasti della lingua in uso (dieresi, accenti, tilde, cediglia), con un maiuscolo di una lettera e un livello di simboli; interruttori ed elenchi non hanno bisogno di tastiera.
* **Pulsanti con simbolo:** il pad di movimento, l'origine, il mandrino, pausa, avvio, arresto e le risposte a una domanda usano simboli e la lettera dell'asse al posto delle parole, con i colori del programma desktop (verde per avviare, blu per la pausa, rosso per fermare).
* **Avvisi registrati:** la fine di un lavoro e la richiesta di utensile possono essere dette da una voce registrata, nella lingua e nel genere impostati (`sound_finish = voice:es:f`), dalla cartella `voices` della scheda; le registrazioni sono quelle di EH3D-BUNGARD-GUI. L'altoparlante non è ancora collegato nel firmware.
* **Il lavoro viene seguito di nuovo dopo un riavvio:** da quando l'operatore avvia un lavoro, lo schermo conserva nella flash quale è (il suo id, la macchina, il suo nome); dopo un'interruzione di corrente o un riavvio segue lo stesso lavoro, la lettura successiva del server dice se è ancora in corso, aspetta a un cambio utensile o è finito, e il pulsante annulla lo lascia. Il lavoro vive nel server. Scritto e testato su un computer; il firmware non è mai stato eseguito su un modulo.
* **Il tipo di macchina:** `machine_kind` nel file delle impostazioni (`cnc`, `laser` o `pnp`) decide cosa lo schermo può ordinare. Solo una macchina CNC ha il pulsante del mandrino, e il varco rifiuta un ordine al mandrino su un laser o su una macchina pick and place qualunque cosa dicano le altre impostazioni: lo schermo non fa mai partire un laser né avvia da solo una testa pick and place.

## 🔒 Sicurezza e protezione

| Minaccia | Difesa |
|---|---|
| Un tocco o un ordine vocale che muove la macchina per errore | un movimento chiede una conferma sullo schermo con limite di tempo; un ordine vocale la chiede sempre; un sì parlato conferma solo con l'impostazione `voice_confirm` attiva (spenta di default) |
| Lo schermo inizia a muovere con l'arresto hardware premuto | l'arresto hardware è un blocco: premuto, o rilasciato ma non confermato sullo schermo, ogni movimento e ordine al mandrino è rifiutato e ciò che attendeva è scartato; un arresto premuto mentre attende una conferma la annulla |
| Una macchina non pronta | nulla si muove con il collegamento non armato, con l'ingresso di arresto hardware non collegato (`hw_stop_required`, attivo di default) o con il limite del mandrino a 0 |
| Una parola detta scambiata per un ordine | gli ordini vengono da un breve elenco fisso per lingua, mai da testo libero; una parola che contiene solo un ordine (`stopwatch`) non è un ordine; gli ordini vocali sono spenti finché l'operatore non li attiva |
| Un errore di battitura nelle impostazioni | un valore fuori intervallo o una chiave sconosciuta viene segnalata e resta il valore sicuro di default; nessun interruttore si apre per errore |
| Un segreto nel firmware o nel repository | nulla di segreto è compilato; il token è un'impostazione nella sua partizione e non viene mai mostrato né registrato |

**Ciò che non è difeso:** lo schermo non è un dispositivo di sicurezza e non può togliere l'azionamento della macchina; deve farlo da solo l'arresto hardware. Non c'è ancora il collegamento con il server, quindi oggi lo schermo non può muovere alcuna macchina. Il riconoscimento vocale è provato come testo; il percorso reale del microfono (cancellazione dell'eco, rumore, accenti) non è stato provato. Chi ha il modulo in mano può cambiare il file delle impostazioni.

## 🔌 Interfacce

| Interfaccia | Che cos'è | Dove |
|---|---|---|
| File delle impostazioni | `hmi.cfg` nella partizione `config`: righe `chiave = valore`; vedi `hmi.cfg.example` e la tabella sopra | `core/hmi_config.hpp` |
| Ingresso arresto hardware | un GPIO (`HMI_HW_STOP_GPIO` in menuconfig, attivo basso di default, con pull-up così un filo aperto vale come premuto) | `main/board_p4.cpp` |
| Display touch | MIPI-DSI 800x1280 tramite LVGL 9 | `main/ui.cpp` |
| Suono | codec ES8311 con amplificatore per altoparlante e ingresso microfono ES7210 | `main/board_p4.cpp` |
| microSD | la radice della scheda in `/sdcard`: si elencano e leggono file di foratura, fresatura, Gerber, DXF e ZIP e si scrivono rapporti; si accettano solo nomi di file semplici | `core/sdfiles.hpp`, `main/board_p4.cpp` |
| EH3D-SERVER | HTTP con il token di accesso: `GET /api/v1/fleet` ogni `poll_s` secondi, `POST /api/v1/machines/:id/jobs`, `POST /api/v1/jobs/:id/start`, `confirm` e `cancel` | `core/server_link.hpp`, `main/server_link.cpp` |

## ⚙️ Configurazione

Tutto ciò che si regola è una riga di `hmi.cfg` (copia `hmi.cfg.example` nella partizione `config`); un valore fuori intervallo viene segnalato nel registro e resta il valore di default. I valori qui sotto sono quelli di default; ogni default di sicurezza è quello prudente.

| Impostazione | Default | Intervallo | Cosa fa |
|---|---|---|---|
| `language` | `en` | `en es it de fr pt zh ja` | la lingua delle schermate e della voce |
| `confirm_moves` | on | on / off | chiede conferma per muovere: spegnere solo sul banco |
| `confirm_timeout_ms` | 8000 | 1000 - 60000 | quanto attende una conferma |
| `jog_max_mm` | 20 | 1 - 200 | il movimento manuale più lungo che lo schermo invia |
| `spindle_max_rpm` | 24000 | 0 - 65535 | la velocità massima del mandrino richiesta (0: non avvia mai il mandrino) |
| `hw_stop_required` | on | on / off | comanda la macchina solo con l'ingresso di arresto hardware collegato e rilasciato |
| `voice_orders` | off | on / off | accetta ordini vocali |
| `voice_confirm` | off | on / off | accetta un sì parlato come conferma di un movimento |
| `token` | none | 8 - 64 characters | il token di accesso al server; solo scrittura, mai mostrato né registrato |
| `orientation` | `auto` | `auto landscape portrait` | in verticale o in orizzontale; `auto` segue come è montato il modulo |
| `poll_s` | 3 | 1 - 60 | ogni quanto lo schermo chiede la flotta al server |

## 🚀 Avvio rapido

Servono un compilatore C++17 e CMake per i test sul computer, ed ESP-IDF 5.5.5 per il firmware (il target ESP32-P4 del modulo).

```bash
build.bat            # the host tests (Windows)  -  ./build.sh on Linux
python -I tools/ci_validate.py
```

Il firmware non è ancora stato compilato: quando lo sarà, `idf.py set-target esp32p4 && idf.py build` crea l'immagine e una partizione chiamata `config` contiene `hmi.cfg`.

## 🧪 Test

I test sul computer (`build.bat` o `./build.sh`, poi `ctest`) sono due programmi e passa tutto: `test_hmi` (638 controlli) su file delle impostazioni, varco (l'arresto passa sempre, blocco dell'arresto hardware, conferma, tempo limite e giro dell'orologio, regole vocali, interruttori), ordini e avvisi vocali e testi delle schermate in otto lingue; e `test_workflow` (115 controlli) su orientamento e tocco, file della scheda (tipi, nomi sicuri, ordinamento), lettore JSON, richieste e risposte del server e flusso di un lavoro dalla scheda alla fine. Gli altri due programmi coprono le tastiere (`test_keyboard`, 137 controlli: ogni impostazione ha la sua tastiera, gli intervalli del tastierino sono quelli del file delle impostazioni, i decimali, il segno, le frecce, la tastiera completa di ogni lingua, il maiuscolo, i simboli e la lunghezza) e gli avvisi registrati (`test_notices`, 66 controlli: l'impostazione, il file di ogni lingua e genere).

## 🏗️ Compilazione e rilascio

La versione vive in `eh3d.project.json` e ogni versione ha la sua voce in [CHANGELOG.md](CHANGELOG.md). `build.sh` / `build.bat` compilano ed eseguono i test sul computer e `test.sh` / `test.bat` eseguono i controlli del repository. Il firmware si compila fuori dagli script di questo repository, in una finestra visibile, una compilazione alla volta.

## 🩺 Risoluzione dei problemi

| Sintomo | Cosa fare |
|---|---|
| Nulla si muove e lo schermo dice che l'arresto hardware è premuto o non confermato | Rilascia il pulsante e premi il pulsante di conferma sullo schermo: un arresto rilasciato non si cancella da solo. |
| Lo schermo dice che la macchina non è armata | In questa versione non c'è ancora il collegamento con la macchina; il varco rifiuta ogni movimento finché un collegamento non la arma. |
| Lo schermo dice che l'ingresso di arresto hardware non è collegato | Collega il pulsante di arresto al pin impostato in menuconfig (`HMI_HW_STOP_GPIO`), oppure spegni `hw_stop_required` solo sul banco. |
| Un ordine vocale non fa nulla | Gli ordini vocali sono spenti di default: imposta `voice_orders = on`. Quelli che muovono o girano chiedono un tocco sullo schermo se `voice_confirm` non è attivo. |

## 📂 Struttura del repository

<p align="center">
  <img src="images/repository-structure.svg" alt="Struttura del repository EH3D-NODE-HMI-GUI" width="100%">
</p>

<details>
<summary>Versione testuale dell'albero</summary>

```text
EH3D-NODE-HMI-GUI/
├── core/
│   ├── hmi_config.hpp lang.hpp                       il file delle impostazioni e i suoi valori sicuri
│   ├── gate.hpp                                      il varco: conferma, blocco dell'arresto
│   ├── machine.hpp                                   il tipo di macchina
│   ├── voice.hpp                                     ordini e avvisi vocali in otto lingue
│   ├── orientation.hpp sdfiles.hpp                   layout verticale e orizzontale, file della scheda
│   ├── server_link.hpp workflow.hpp job_memory.hpp   client del server e flusso di un lavoro
│   └── ui_text.hpp keyboard.hpp notices.hpp          testi, tastiere e avvisi
├── main/
│   ├── app_main.cpp                                  avvio, impostazioni, il ciclo del varco
│   ├── ui.cpp ui.hpp                                 le schermate LVGL e il pulsante di arresto
│   ├── board_p4.cpp board.hpp                        il modulo: display, suono, ingresso di arresto
│   ├── server_link.cpp server_link.hpp               i socket del client del server
│   └── idf_component.yml Kconfig.projbuild           elenco dei componenti e pin di menuconfig
├── tests/                                            test su computer di impostazioni, varco, voce e testi delle schermate
├── docs/                                             le note hardware e le fonti dei due disegni
├── tools/                                            controllo CI, README e strumenti dei disegni
├── CMakeLists.txt                                    il progetto ESP-IDF
├── partitions.csv                                    la mappa della flash con due slot
├── hmi.cfg.example                                   ogni impostazione con il suo default e intervallo
├── sdkconfig.defaults                                impostazioni ESP-IDF per il modulo ESP32-P4
├── sdkconfig.board.p4                                impostazioni della scheda
├── dependencies.lock                                 le versioni esatte dei componenti
├── images/                                           banner e le immagini di Architettura e Struttura del repository
├── .github/                                          workflow di CI e di rilascio, modelli di issue e pull request
├── build.sh build.bat                                compila ed esegue i test su computer in un pannello dal vivo
├── eh3d.project.json                                 manifesto: versione, ruolo, maturità e comando di compilazione
├── CHANGELOG.md                                      cosa è cambiato in ogni versione
├── SECURITY.md SUPPORT.md                            come segnalare una vulnerabilità e dove chiedere
├── CONTRIBUTING.md CODE_OF_CONDUCT.md LICENSE        come contribuire, codice di condotta e licenza
└── bump_manifest_version.py                          utilità di versione del manifesto
```

</details>

## 🗺️ Roadmap e limiti noti

**Limiti noti oggi**

* Nulla ha girato su un modulo: il firmware non è stato compilato, scritto né provato, e il pacchetto di supporto scheda richiesto potrebbe richiedere modifiche per questo modulo.
* Lo schermo raggiunge la macchina solo tramite un lavoro di EH3D-SERVER; i suoi movimenti diretti (spostamento, origine, mandrino) passano dal varco e poi aspettano il collegamento con il nodo, non ancora scritto. Il client del server e la scheda non hanno mai girato su un modulo.
* Il pin dell'arresto hardware e il controller touch non sono ancora fissati: vanno verificati sullo schema dell'unità.
* Il riconoscitore vocale e l'altoparlante non sono ancora nel firmware; ordini e avvisi sono provati come testo.
* La logica dei programmi desktop (formati, controller, flotta) non è riutilizzata: le schermate seguono il programma desktop, ma le regole sono riscritte in C++.

**Prossimi passi**

* Compilare il firmware e avviare il modulo: display, touch e suono.
* Scrivere il collegamento con EH3D-SERVER e con il nodo e riempire le altre schede delle schermate.
* Aggiungere il riconoscitore vocale e l'altoparlante e provarli con cancellazione dell'eco.
* Collegare un vero pulsante di arresto e provare il blocco su una macchina.

## 🌍 Lingue

Il README e i testi esistono in inglese, spagnolo, francese, italiano, tedesco, cinese, giapponese e portoghese europeo.

## 🔗 Progetti correlati

* [EH3D-DRILLPRO](https://github.com/JuanenRac/EH3D-DRILLPRO): il programma desktop che le schermate seguono
* [EH3D-ROUTEPRO](https://github.com/JuanenRac/EH3D-ROUTEPRO): il programma di fresatura; condivide il file di macchina e la libreria del protocollo
* [EH3D-SERVER](https://github.com/JuanenRac/EH3D-SERVER): il centro della famiglia: esegue i lavori di ogni client
* [EH3D-NODE-BUNGARD](https://github.com/JuanenRac/EH3D-NODE-BUNGARD): il nodo sulla porta seriale dell'unità di controllo della macchina
* [EH3D-COMMON](https://github.com/JuanenRac/EH3D-COMMON): le parti comuni della famiglia (formati, controller, flotta, temi)

## 📚 Documentazione

* Le impostazioni: `hmi.cfg.example` elenca ogni chiave con il suo default e l'intervallo.
* L'hardware: i pin del modulo e le note che accompagnano la demo del produttore, in `docs/HARDWARE.md`.
* [Registro delle modifiche](CHANGELOG.md)

## 🤝 Contribuire e comunità

* Prima di una pull request esegui i test sul computer (`./build.sh` o `build.bat`) e `python -I tools/ci_validate.py`: tutto deve passare. Vedi [CONTRIBUTING.md](CONTRIBUTING.md) e il [codice di condotta](CODE_OF_CONDUCT.md).
* Il README esiste in otto lingue: una modifica del testo si fa in tutti e otto i file, e i due disegni nascono da `docs/architecture.json` e `docs/structure.json` con `tools/make_architecture.py` e `tools/make_structure.py`.
* Nulla che permetta allo schermo di muovere una macchina cambia senza un test del varco che lo copra e una nota di ciò che è stato osservato sull'hardware.
* Segnala una vulnerabilità in privato: [SECURITY.md](SECURITY.md). Domande, idee e segnalazioni: electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTORE

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENZA

Software: GPL-3.0-or-later. Vedi [LICENSE](LICENSE). I progetti hardware della suite usano la CERN OHL-S.
