// EH3D-NODE-HMI-GUI - core/ui_text.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The texts of the screens in the eight languages of the tables (the order of lang.hpp: en, es, it, de, fr, pt, zh, ja) and, through locales.hpp, in the other twenty-nine. The screens of the touch
// display are the ones of the desktop program: the viewer, the job, the machine, the settings and the machines and stations.
#pragma once

#include <array>
#include <cstddef>

#include "gate.hpp"
#include "lang.hpp"
#include "locales.hpp"

namespace hmi {

enum class T : unsigned {
    Viewer, Job, Machine, Settings, Fleet,
    Start, Pause, Stop, Home, Confirm, Cancel,
    Armed, NotArmed, StopPressed, AckRelease,
    Language, Voice, Brightness, Volume,
    JogX, JogY, JogZ, Spindle,
    AskMove, AskSpindle,
    WhyHardwareStop, WhyNotArmed, WhyVoiceOff, WhyNoStop, WhySpindle, WhyNoSpindleHere,
    Feed, Progress, Tool, Jog,
    Holes, Cuts,
    SpindleCut, WhySpindleFast,
    Resume, Stopped, Next,
    ResumedFrom, ToolChanged,
    BoardTooBig, Failed,
    OrdStart, OrdPause, OrdResume, OrdToolChanged, OrdHome, OrdStop, CancelsIn, LinkOk, LinkLost, LinkOther, LinkOld, Firmware, CfgLine, CfgMore, CfgNoFile, CfgNotPair, CfgRange, CfgOnOff, CfgChoice, CfgToken, CfgUnknown, SrvNone, SrvSilent, SrvOther, NoteStopFail, NoteStopRefused, NoteStopSent, NoteNoJobToStop, NoteMoveNoLink, NoteBusy, NoteNothingToSend, NoteOldServer, NoteOk, NoteFollowing, NoteNoJobReady, NoteGoingOn, NoteFile, NoteUnreadable, NoteAllOff, ErrClient, ErrTooLong, ErrBroke, ErrSend, WfKind, WfEmpty, WfOldNoFile, WfNothing, OrientAuto, OrientLandscape, OrientPortrait, OverClip, OverRefuse,
    Tools, TimeLeft, EstTime, AtcAuto, AtcByHand, PutTool, WarnTiny, WarnFar, WarnScale, WarnCourse, WfRunning,
    StageDone, FollowRouting, RackTitle, RackNoSlot, RackShared, RackUndefined, RackNotChained,
    Count
};

inline const Row& text_row(T t) {
    static const std::array<Row, static_cast<std::size_t>(T::Count)> rows = {{
        {"Viewer", "Visor", "Vista", "Ansicht", "Vue", "Visor", "查看", "ビューア"},
        {"Job", "Trabajo", "Lavoro", "Auftrag", "Travail", "Trabalho", "作业", "ジョブ"},
        {"Machine", "Máquina", "Macchina", "Maschine", "Machine", "Máquina", "机床", "マシン"},
        {"Settings", "Ajustes", "Impostazioni", "Einstellungen", "Réglages", "Definições", "设置", "設定"},
        {"Machines and stations", "Máquinas y estaciones", "Macchine e stazioni", "Maschinen und Stationen", "Machines et stations", "Máquinas e estações", "机床和工位", "マシンとステーション"},
        {"Start", "Iniciar", "Avvia", "Start", "Démarrer", "Iniciar", "开始", "開始"},
        {"Pause", "Pausa", "Pausa", "Pause", "Pause", "Pausa", "暂停", "一時停止"},
        {"Stop", "Parar", "Ferma", "Stopp", "Arrêt", "Parar", "停止", "停止"},
        {"Home", "Origen", "Origine", "Referenz", "Origine", "Origem", "原点", "原点"},
        {"Confirm", "Confirmar", "Conferma", "Bestätigen", "Confirmer", "Confirmar", "确认", "確認"},
        {"Cancel", "Cancelar", "Annulla", "Abbrechen", "Annuler", "Cancelar", "取消", "キャンセル"},
        {"Armed", "Armada", "Armata", "Scharf", "Armée", "Armada", "已就绪", "有効"},
        {"Not armed", "Sin armar", "Non armata", "Nicht scharf", "Non armée", "Sem armar", "未就绪", "無効"},
        {"HARDWARE STOP PRESSED", "PARO POR HARDWARE PULSADO", "ARRESTO HARDWARE PREMUTO", "HARDWARE-STOPP GEDRÜCKT", "ARRÊT MATÉRIEL ENFONCÉ", "PARAGEM POR HARDWARE PREMIDA", "硬件急停已按下", "ハードウェア停止中"},
        {"Stop released: acknowledge", "Paro liberado: reconocer", "Arresto rilasciato: conferma", "Stopp gelöst: quittieren", "Arrêt relâché : acquitter", "Paragem libertada: reconhecer", "急停已释放：确认", "停止解除：確認"},
        {"Language", "Idioma", "Lingua", "Sprache", "Langue", "Idioma", "语言", "言語"},
        {"Voice", "Voz", "Voce", "Stimme", "Voix", "Voz", "语音", "音声"},
        {"Brightness", "Brillo", "Luminosità", "Helligkeit", "Luminosité", "Brilho", "亮度", "明るさ"},
        {"Volume", "Volumen", "Volume", "Lautstärke", "Volume", "Volume", "音量", "音量"},
        {"Jog X", "Mover X", "Sposta X", "X fahren", "Déplacer X", "Mover X", "移动 X", "X 移動"},
        {"Jog Y", "Mover Y", "Sposta Y", "Y fahren", "Déplacer Y", "Mover Y", "移动 Y", "Y 移動"},
        {"Jog Z", "Mover Z", "Sposta Z", "Z fahren", "Déplacer Z", "Mover Z", "移动 Z", "Z 移動"},
        {"Spindle", "Husillo", "Mandrino", "Spindel", "Broche", "Fuso", "主轴", "スピンドル"},
        {"Move the machine?", "¿Mover la máquina?", "Muovere la macchina?", "Maschine bewegen?", "Déplacer la machine ?", "Mover a máquina?", "移动机床？", "マシンを動かしますか？"},
        {"Turn the spindle?", "¿Girar el husillo?", "Far girare il mandrino?", "Spindel drehen?", "Faire tourner la broche ?", "Rodar o fuso?", "转动主轴？", "スピンドルを回しますか？"},
        {"The hardware stop is pressed or not acknowledged.", "El paro por hardware está pulsado o sin reconocer.", "L'arresto hardware è premuto o non confermato.", "Der Hardware-Stopp ist gedrückt oder nicht quittiert.", "L'arrêt matériel est enfoncé ou non acquitté.", "A paragem por hardware está premida ou por reconhecer.", "硬件急停已按下或未确认。", "ハードウェア停止が押されているか未確認です。"},
        {"The machine is not armed.", "La máquina no está armada.", "La macchina non è armata.", "Die Maschine ist nicht scharf.", "La machine n'est pas armée.", "A máquina não está armada.", "机床未就绪。", "マシンが有効ではありません。"},
        {"Spoken orders are off in the settings.", "Las órdenes por voz están desactivadas en los ajustes.", "Gli ordini vocali sono disattivati nelle impostazioni.", "Sprachbefehle sind in den Einstellungen aus.", "Les ordres vocaux sont désactivés dans les réglages.", "As ordens por voz estão desligadas nas definições.", "设置中已关闭语音指令。", "設定で音声命令がオフです。"},
        {"The hardware stop input is not wired.", "La entrada de paro por hardware no está cableada.", "L'ingresso di arresto hardware non è collegato.", "Der Hardware-Stopp-Eingang ist nicht verdrahtet.", "L'entrée d'arrêt matériel n'est pas câblée.", "A entrada de paragem por hardware não está ligada.", "硬件急停输入未接线。", "ハードウェア停止入力が配線されていません。"},
        {"The spindle is switched off in the settings.", "El husillo está desactivado en los ajustes.", "Il mandrino è disattivato nelle impostazioni.", "Die Spindel ist in den Einstellungen abgeschaltet.", "La broche est désactivée dans les réglages.", "O fuso está desligado nas definições.", "设置中已禁用主轴。", "設定でスピンドルが無効です。"},
        {"This kind of machine has no spindle for the screen to start.", "Este tipo de máquina no tiene un husillo que la pantalla pueda arrancar.", "Questo tipo di macchina non ha un mandrino che lo schermo possa avviare.", "Diese Art Maschine hat keine Spindel, die der Bildschirm starten könnte.", "Ce type de machine n'a pas de broche que l'écran puisse démarrer.", "Este tipo de máquina não tem um fuso que o ecrã possa arrancar.", "这类机床没有可由屏幕启动的主轴。", "この種類のマシンには、画面から始動できるスピンドルがありません。"},
        {"Feed", "Avance", "Avanzamento", "Vorschub", "Avance", "Avanço", "进给", "送り"},
        {"Progress", "Progreso", "Avanzamento lavoro", "Fortschritt", "Progression", "Progresso", "进度", "進行"},
        {"Tool", "Herramienta", "Utensile", "Werkzeug", "Outil", "Ferramenta", "刀具", "工具"},
        {"Jog", "Movimiento manual", "Spostamento manuale", "Tippbetrieb", "Jog", "Movimento manual", "点动", "ジョグ"},
        {"holes", "taladros", "fori", "Bohrungen", "trous", "furos", "孔", "穴"},
        {"cuts", "cortes", "tagli", "Fräsungen", "découpes", "cortes", "切削", "切削"},
        {"Order cut to the top speed of the spindle", "Orden recortada a la velocidad máxima del husillo", "Ordine ridotto alla velocità massima del mandrino", "Befehl auf die Höchstdrehzahl der Spindel gekürzt", "Ordre ramené à la vitesse maximale de la broche", "Ordem reduzida à velocidade máxima do fuso", "指令已降至主轴最高转速", "指令をスピンドルの最高回転数に制限しました"},
        {"The order asks for more turns than the spindle gives.", "La orden pide más vueltas de las que da el husillo.", "L'ordine chiede più giri di quanti ne dia il mandrino.", "Der Befehl verlangt mehr Drehzahl, als die Spindel liefert.", "L'ordre demande plus de tours que la broche n'en donne.", "A ordem pede mais rotações do que o fuso dá.", "指令要求的转速超过主轴能提供的转速。", "指令の回転数がスピンドルの上限を超えています。"},
        {"Resume", "Reanudar", "Riprendi", "Fortsetzen", "Reprendre", "Retomar", "继续", "再開"},
        {"Stopped", "Parado", "Fermato", "Gestoppt", "Arrêté", "Parado", "已停止", "停止済み"},
        {"Next", "Siguiente", "Successivo", "Weiter", "Suivant", "Seguinte", "下一个", "次"},
        {"Resumed from", "Reanudado desde", "Ripreso da", "Fortgesetzt von", "Repris de", "Retomado de", "接续自", "再開元"},
        {"Tool changed", "Herramienta cambiada", "Utensile cambiato", "Werkzeug gewechselt", "Outil changé", "Ferramenta trocada", "已换刀", "工具交換済み"},
        {"The board does not fit the table", "La placa no cabe en la mesa", "La scheda non entra nel tavolo", "Die Platine passt nicht auf den Tisch", "La carte ne tient pas sur la table", "A placa não cabe na mesa", "电路板放不下工作台", "基板がテーブルに収まりません"},
        {"Failed", "Fallido", "Fallito", "Fehlgeschlagen", "Échec", "Falhou", "失败", "失敗"},
        {"Start the job", "Empezar el trabajo", "Avvia il lavoro", "Auftrag starten", "Démarrer le travail", "Iniciar o trabalho", "开始作业", "ジョブを開始"},
        {"Pause the job", "Pausar el trabajo", "Metti in pausa il lavoro", "Auftrag pausieren", "Mettre le travail en pause", "Pausar o trabalho", "暂停作业", "ジョブを一時停止"},
        {"Resume the job", "Reanudar el trabajo", "Riprendi il lavoro", "Auftrag fortsetzen", "Reprendre le travail", "Retomar o trabalho", "继续作业", "ジョブを再開"},
        {"Confirm the tool change", "Confirmar el cambio de herramienta", "Conferma il cambio utensile", "Werkzeugwechsel bestätigen", "Confirmer le changement d'outil", "Confirmar a troca de ferramenta", "确认换刀", "工具交換を確認"},
        {"Home the machine", "Llevar la máquina a origen", "Porta la macchina all'origine", "Maschine referenzieren", "Ramener la machine à l'origine", "Levar a máquina à origem", "机床回原点", "マシンを原点に戻す"},
        {"Stop the machine", "Parar la máquina", "Ferma la macchina", "Maschine stoppen", "Arrêter la machine", "Parar a máquina", "停止机床", "マシンを停止"},
        {"Cancels in", "Se cancela en", "Si annulla tra", "Abbruch in", "Annulation dans", "Cancela em", "自动取消倒计时", "自動キャンセルまで"},
        {"Connected", "Conectado", "Connesso", "Verbunden", "Connecté", "Ligado", "已连接", "接続済み"},
        {"No connection", "Sin conexión", "Nessuna connessione", "Keine Verbindung", "Pas de connexion", "Sem ligação", "无连接", "接続なし"},
        {"Other server", "Otro servidor", "Altro server", "Anderer Server", "Autre serveur", "Outro servidor", "其他服务器", "別のサーバー"},
        {"Old data", "Datos antiguos", "Dati vecchi", "Alte Daten", "Données anciennes", "Dados antigos", "数据已过期", "古いデータ"},
        {"Firmware", "Firmware", "Firmware", "Firmware", "Firmware", "Firmware", "固件", "ファームウェア"},
        {"hmi.cfg, line", "hmi.cfg, línea", "hmi.cfg, riga", "hmi.cfg, Zeile", "hmi.cfg, ligne", "hmi.cfg, linha", "配置文件 hmi.cfg，行", "設定 hmi.cfg、行"},
        {"more", "más", "altri", "weitere", "autres", "mais", "更多", "他"},
        {"hmi.cfg not found: the defaults are used", "No se encuentra hmi.cfg: se usan los valores por defecto", "hmi.cfg non trovato: si usano i valori predefiniti", "hmi.cfg nicht gefunden: die Standardwerte gelten", "hmi.cfg introuvable : les valeurs par défaut sont utilisées", "hmi.cfg não encontrado: usam-se os valores predefinidos", "找不到 hmi.cfg：使用默认值", "設定ファイル hmi.cfg が見つかりません：初期値を使います"},
        {"not a key = value line", "no es una línea clave = valor", "non è una riga chiave = valore", "keine Zeile der Form Schlüssel = Wert", "ce n'est pas une ligne clé = valeur", "não é uma linha chave = valor", "不是“键 = 值”的行", "「キー = 値」の行ではありません"},
        {"out of range or not a number", "fuera de rango o no es un número", "fuori intervallo o non è un numero", "außerhalb des Bereichs oder keine Zahl", "hors limites ou pas un nombre", "fora do intervalo ou não é um número", "超出范围或不是数字", "範囲外か数値ではありません"},
        {"expected on or off", "se esperaba on u off", "atteso on oppure off", "on oder off erwartet", "on ou off attendu", "esperava-se on ou off", "应为 on 或 off", "値は on か off にしてください"},
        {"not an allowed value", "valor no permitido", "valore non consentito", "Wert nicht erlaubt", "valeur non permise", "valor não permitido", "值不被允许", "許可されない値です"},
        {"must have 8 to 64 characters", "debe tener de 8 a 64 caracteres", "deve avere da 8 a 64 caratteri", "muss 8 bis 64 Zeichen haben", "doit avoir de 8 à 64 caractères", "deve ter de 8 a 64 caracteres", "必须为 8 到 64 个字符", "文字数は 8 から 64 にしてください"},
        {"unknown key", "clave desconocida", "chiave sconosciuta", "unbekannter Schlüssel", "clé inconnue", "chave desconhecida", "未知的键", "不明なキーです"},
        {"No server_host in hmi.cfg: nothing is controlled", "Falta server_host en hmi.cfg: no se controla nada", "Manca server_host in hmi.cfg: non si controlla nulla", "Kein server_host in hmi.cfg: nichts wird gesteuert", "Pas de server_host dans hmi.cfg : rien n'est commandé", "Falta server_host em hmi.cfg: nada é controlado", "未设置：hmi.cfg 中没有 server_host，不控制任何设备", "未設定：hmi.cfg に server_host がありません。何も制御しません"},
        {"The server does not answer", "El servidor no responde", "Il server non risponde", "Der Server antwortet nicht", "Le serveur ne répond pas", "O servidor não responde", "服务器无响应", "サーバーが応答しません"},
        {"The answer is not from an EH3D server", "La respuesta no es de un servidor EH3D", "La risposta non è di un server EH3D", "Die Antwort stammt nicht von einem EH3D-Server", "La réponse ne vient pas d'un serveur EH3D", "A resposta não é de um servidor EH3D", "该响应不是来自 EH3D 服务器", "EH3D サーバーからの応答ではありません"},
        {"STOP NOT SENT: use the hardware stop", "PARO NO ENVIADO: use el paro por hardware", "ARRESTO NON INVIATO: usare l'arresto hardware", "STOPP NICHT GESENDET: Hardware-Stopp benutzen", "ARRÊT NON ENVOYÉ : utiliser l'arrêt matériel", "PARAGEM NÃO ENVIADA: use a paragem por hardware", "急停未发送：请使用硬件急停", "停止を送信できません：ハードウェア停止を使ってください"},
        {"Stop refused", "Paro rechazado", "Arresto rifiutato", "Stopp abgelehnt", "Arrêt refusé", "Paragem recusada", "急停被拒绝", "停止は拒否されました"},
        {"Stop sent", "Paro enviado", "Arresto inviato", "Stopp gesendet", "Arrêt envoyé", "Paragem enviada", "急停已发送", "停止を送信しました"},
        {"No job of the machine to stop", "La máquina no tiene ningún trabajo que parar", "La macchina non ha nessun lavoro da fermare", "Kein Auftrag der Maschine zum Stoppen", "Aucun travail de la machine à arrêter", "A máquina não tem nenhum trabalho para parar", "机床没有可停止的作业", "停止するジョブがありません"},
        {"This move needs the link to the node, which is not ready yet", "Este movimiento necesita el enlace con el nodo, que aún no está listo", "Questo movimento richiede il collegamento al nodo, non ancora pronto", "Diese Bewegung braucht die Verbindung zum Knoten, die noch nicht bereit ist", "Ce mouvement exige la liaison avec le nœud, pas encore prête", "Este movimento precisa da ligação ao nó, que ainda não está pronta", "此动作需要与节点的链路，目前尚未就绪", "この動作にはノードとのリンクが必要ですが、まだ準備できていません"},
        {"The link is busy: ask again", "El enlace está ocupado: pídalo otra vez", "Il collegamento è occupato: riprovare", "Die Verbindung ist belegt: bitte erneut anfragen", "La liaison est occupée : redemandez", "A ligação está ocupada: peça outra vez", "链路繁忙：请再试一次", "リンクが使用中です：もう一度お試しください"},
        {"Nothing to send: choose a file and the machine of the server", "Nada que enviar: elija un archivo y la máquina del servidor", "Niente da inviare: scegliere un file e la macchina del server", "Nichts zu senden: Datei und Maschine des Servers wählen", "Rien à envoyer : choisissez un fichier et la machine du serveur", "Nada para enviar: escolha um ficheiro e a máquina do servidor", "没有可发送的内容：请选择文件和服务器的机床", "送るものがありません：ファイルとサーバーのマシンを選んでください"},
        {"Old server: the file is sent again", "Servidor antiguo: se envía el archivo otra vez", "Server vecchio: il file viene inviato di nuovo", "Alter Server: die Datei wird erneut gesendet", "Ancien serveur : le fichier est renvoyé", "Servidor antigo: o ficheiro é enviado outra vez", "旧版服务器：将重新发送文件", "古いサーバー：ファイルを再送します"},
        {"Done", "Hecho", "Fatto", "Fertig", "Terminé", "Feito", "完成", "完了"},
        {"Following the job left running", "Siguiendo el trabajo que quedó en marcha", "Segue il lavoro rimasto in corso", "Der laufende Auftrag wird weiterverfolgt", "Suivi du travail resté en cours", "A acompanhar o trabalho que ficou em curso", "正在跟进仍在运行的作业", "実行中のままのジョブを追跡しています"},
        {"No job is ready: choose a file and send it first", "No hay ningún trabajo listo: elija un archivo y envíelo primero", "Nessun lavoro pronto: scegliere un file e inviarlo prima", "Kein Auftrag bereit: zuerst eine Datei wählen und senden", "Aucun travail prêt : choisissez un fichier et envoyez-le d'abord", "Nenhum trabalho pronto: escolha um ficheiro e envie-o primeiro", "没有就绪的作业：请先选择文件并发送", "準備できたジョブがありません：先にファイルを選んで送ってください"},
        {"Going on from the holes already done", "Continuando desde los taladros ya hechos", "Si riprende dai fori già fatti", "Weiter ab den bereits gebohrten Löchern", "Reprise à partir des trous déjà faits", "A continuar a partir dos furos já feitos", "从已完成的孔继续", "完了済みの穴から続けます"},
        {"File", "Archivo", "File", "Datei", "Fichier", "Ficheiro", "文件", "ファイル"},
        {"The file cannot be read", "No se puede leer el archivo", "Impossibile leggere il file", "Die Datei kann nicht gelesen werden", "Le fichier est illisible", "Não é possível ler o ficheiro", "无法读取文件", "ファイルを読み込めません"},
        {"Every tool is switched off: nothing to send", "Todas las herramientas están desactivadas: nada que enviar", "Tutti gli utensili sono disattivati: niente da inviare", "Alle Werkzeuge sind abgeschaltet: nichts zu senden", "Tous les outils sont désactivés : rien à envoyer", "Todas as ferramentas estão desligadas: nada para enviar", "所有刀具均已关闭：没有可发送的内容", "すべての工具がオフです：送るものがありません"},
        {"Cannot start the network client", "No se puede iniciar el cliente de red", "Impossibile avviare il client di rete", "Der Netzwerk-Client startet nicht", "Impossible de démarrer le client réseau", "Não é possível iniciar o cliente de rede", "无法启动网络客户端", "ネットワーククライアントを起動できません"},
        {"The answer is too long", "La respuesta es demasiado larga", "La risposta è troppo lunga", "Die Antwort ist zu lang", "La réponse est trop longue", "A resposta é demasiado longa", "响应过长", "応答が長すぎます"},
        {"The connection broke", "Se cortó la conexión", "La connessione si è interrotta", "Die Verbindung brach ab", "La connexion s'est interrompue", "A ligação foi interrompida", "连接已中断", "接続が切れました"},
        {"Cannot send the request", "No se puede enviar la petición", "Impossibile inviare la richiesta", "Die Anfrage kann nicht gesendet werden", "Impossible d'envoyer la requête", "Não é possível enviar o pedido", "无法发送请求", "リクエストを送信できません"},
        {"Only drilling, routing and G-code files can be sent", "Solo se pueden enviar archivos de taladrado, fresado y G-code", "Si possono inviare solo file di foratura, fresatura e G-code", "Nur Bohr-, Fräs- und G-Code-Dateien können gesendet werden", "Seuls les fichiers de perçage, de fraisage et de G-code peuvent être envoyés", "Só se podem enviar ficheiros de furação, fresagem e G-code", "只能发送钻孔、铣削和 G 代码文件", "穴あけ、切削、G コードのファイルだけ送れます"},
        {"The file is empty", "El archivo está vacío", "Il file è vuoto", "Die Datei ist leer", "Le fichier est vide", "O ficheiro está vazio", "文件为空", "ファイルが空です"},
        {"Old server and no file to send again", "Servidor antiguo y sin archivo para enviar otra vez", "Server vecchio e nessun file da inviare di nuovo", "Alter Server und keine Datei zum erneuten Senden", "Ancien serveur et aucun fichier à renvoyer", "Servidor antigo e sem ficheiro para enviar outra vez", "旧版服务器且没有可重新发送的文件", "古いサーバーで、再送するファイルがありません"},
        {"There is nothing to go on from", "No hay nada desde donde continuar", "Non c'è nulla da cui riprendere", "Es gibt nichts, wo man fortsetzen könnte", "Il n'y a rien à partir de quoi reprendre", "Não há nada de onde continuar", "没有可继续的位置", "続ける元がありません"},
        {"Automatic", "Automática", "Automatico", "Automatisch", "Automatique", "Automática", "自动", "自動"},
        {"Landscape", "Horizontal", "Orizzontale", "Querformat", "Paysage", "Horizontal", "横屏", "横向き"},
        {"Portrait", "Vertical", "Verticale", "Hochformat", "Portrait", "Vertical", "竖屏", "縦向き"},
        {"Cut to top speed", "Recortar a la velocidad máxima", "Riduci alla velocità massima", "Auf Höchstdrehzahl kürzen", "Ramener à la vitesse maximale", "Limitar à velocidade máxima", "限制到最高转速", "最高回転数に制限"},
        {"Refuse", "Rechazar", "Rifiuta", "Ablehnen", "Refuser", "Recusar", "拒绝", "拒否"},
        {"Tools", "Herramientas", "Utensili", "Werkzeuge", "Outils", "Ferramentas", "刀具", "工具"},
        {"Time left", "Tiempo restante", "Tempo rimasto", "Restzeit", "Temps restant", "Tempo restante", "剩余时间", "残り時間"},
        {"Estimated time", "Tiempo estimado", "Tempo stimato", "Geschätzte Zeit", "Temps estimé", "Tempo estimado", "预计时间", "推定時間"},
        {"Changes the tools by itself", "Cambia las herramientas sola", "Cambia gli utensili da sola", "Wechselt die Werkzeuge selbst", "Change les outils elle-même", "Troca as ferramentas sozinha", "自动换刀", "工具を自動交換"},
        {"Tool bar not ready: change the tool by hand", "Barra de herramientas sin preparar: cambie la herramienta a mano", "Barra utensili non pronta: cambiare l'utensile a mano", "Werkzeugleiste nicht bereit: Werkzeug von Hand wechseln", "Barre d'outils non prête : changez l'outil à la main", "Barra de ferramentas por preparar: troque a ferramenta à mão", "刀具架未就绪：请手动换刀", "ツールバーが未設定です：手動で工具を交換してください"},
        {"Put in the tool", "Ponga la herramienta", "Inserire l'utensile", "Werkzeug einsetzen", "Mettez l'outil", "Coloque a ferramenta", "请装入刀具", "工具を取り付けてください"},
        {"The work is tiny", "El trabajo es diminuto", "Il lavoro è minuscolo", "Die Arbeit ist winzig", "Le travail est minuscule", "O trabalho é minúsculo", "工件极小", "加工範囲が極端に小さいです"},
        {"The work is far from the corner", "El trabajo está lejos de la esquina", "Il lavoro è lontano dall'angolo", "Die Arbeit liegt weit von der Ecke", "Le travail est loin du coin", "O trabalho está longe do canto", "工件远离原点角", "加工位置が角から遠く離れています"},
        {"The scale is not 1", "La escala no es 1", "La scala non è 1", "Der Maßstab ist nicht 1", "L'échelle n'est pas 1", "A escala não é 1", "缩放比例不是 1", "倍率が 1 ではありません"},
        {"The plan goes past the course of an axis", "El plan pasa del recorrido de un eje", "Il piano supera la corsa di un asse", "Der Plan überschreitet den Verfahrweg einer Achse", "Le plan dépasse la course d'un axe", "O plano ultrapassa o curso de um eixo", "计划超出某个轴的行程", "計画が軸の可動範囲を超えます"},
        {"A job is running: the file cannot be changed", "Hay un trabajo en marcha: no se puede cambiar el archivo", "Un lavoro è in corso: il file non si può cambiare", "Ein Auftrag läuft: die Datei kann nicht gewechselt werden", "Un travail est en cours : le fichier ne peut pas être changé", "Há um trabalho em curso: não é possível mudar o ficheiro", "作业正在运行：无法更换文件", "ジョブ実行中：ファイルは変更できません"},
        {"The drilling is done and the routing comes next", "El taladrado ha terminado y sigue el fresado", "La foratura è terminata e segue la fresatura", "Das Bohren ist fertig, als Nächstes folgt das Fräsen", "Le perçage est terminé et le fraisage suit", "A furação terminou e segue-se a fresagem", "钻孔已完成，接下来是铣削", "穴あけが終わり、次はルーティングです"},
        {"Follow the routing", "Seguir el fresado", "Segui la fresatura", "Fräsen verfolgen", "Suivre le fraisage", "Acompanhar a fresagem", "跟进铣削", "ルーティングを追う"},
        {"Positions in the bar", "Posiciones en la barra", "Posizioni nella barra", "Positionen in der Leiste", "Positions dans la barre", "Posições na barra", "刀库位置", "バーの位置"},
        {"has no place in the bar", "no tiene hueco en la barra", "non ha un posto nella barra", "hat keinen Platz in der Leiste", "n'a pas de place dans la barre", "não tem lugar na barra", "在刀库中没有位置", "バーに位置がありません"},
        {"shares its place in the bar with another tool", "comparte hueco en la barra con otra herramienta", "condivide il posto nella barra con un altro utensile", "teilt seinen Platz in der Leiste mit einem anderen Werkzeug", "partage sa place dans la barre avec un autre outil", "partilha o lugar na barra com outra ferramenta", "与另一把刀共用刀库位置", "バーの位置が他の工具と重複しています"},
        {"its place is not defined in the machine", "su hueco no está definido en la máquina", "il suo posto non è definito nella macchina", "sein Platz ist in der Maschine nicht definiert", "sa place n'est pas définie dans la machine", "o seu lugar não está definido na máquina", "其位置在机床中未定义", "位置がマシンに定義されていません"},
        {"The server did not chain the tool changes: put the tools in the bar in the order of the job", "El servidor no encadenó los cambios de herramienta: ponga las herramientas en la barra en el orden del trabajo", "Il server non ha concatenato i cambi utensile: mettere gli utensili nella barra nell'ordine del lavoro", "Der Server hat die Werkzeugwechsel nicht verkettet: Werkzeuge in der Reihenfolge des Auftrags in die Leiste legen", "Outils non enchaînés : mettez-les dans la barre dans l'ordre du travail", "O servidor não encadeou as trocas: ponha as ferramentas na barra pela ordem do trabalho", "服务器没有连接换刀：请按作业顺序把刀具放入刀库", "サーバーが工具交換を連鎖しませんでした：ジョブの順にバーへ工具を置いてください"}
    }};
    return rows[static_cast<std::size_t>(t)];
}

static_assert(static_cast<std::size_t>(T::Count) == kUiKeyCount, "the keys of locales.hpp follow the enum T");

inline const char* text(T t, Lang l) { return translated(l, static_cast<std::size_t>(t), text_row(t)); }

/// The sentence that says why the gate refused.
inline const char* why_text(Why w, Lang l) {
    switch (w) {
        case Why::HardwareStop: return text(T::WhyHardwareStop, l);
        case Why::NotArmed: return text(T::WhyNotArmed, l);
        case Why::VoiceOrdersOff: return text(T::WhyVoiceOff, l);
        case Why::NoHardwareStop: return text(T::WhyNoStop, l);
        case Why::SpindleDisabled: return text(T::WhySpindle, l);
        case Why::NoSpindleHere: return text(T::WhyNoSpindleHere, l);
        case Why::SpindleTooFast: return text(T::WhySpindleFast, l);
        default: return "";
    }
}

}  // namespace hmi
