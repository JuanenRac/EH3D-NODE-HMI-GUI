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
