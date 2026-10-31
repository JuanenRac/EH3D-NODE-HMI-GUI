// EH3D-NODE-HMI-GUI - core/ui_text.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The texts of the screens in the eight languages (the order of lang.hpp: en, es, it, de, fr, pt, zh, ja). The screens of the touch
// display are the ones of the desktop program: the viewer, the job, the machine, the settings and the machines and stations.
#pragma once

#include <array>
#include <cstddef>

#include "gate.hpp"
#include "lang.hpp"

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
    }};
    return rows[static_cast<std::size_t>(t)];
}

inline const char* text(T t, Lang l) { return pick(text_row(t), l); }

/// The sentence that says why the gate refused.
inline const char* why_text(Why w, Lang l) {
    switch (w) {
        case Why::HardwareStop: return text(T::WhyHardwareStop, l);
        case Why::NotArmed: return text(T::WhyNotArmed, l);
        case Why::VoiceOrdersOff: return text(T::WhyVoiceOff, l);
        case Why::NoHardwareStop: return text(T::WhyNoStop, l);
        case Why::SpindleDisabled: return text(T::WhySpindle, l);
        case Why::NoSpindleHere: return text(T::WhyNoSpindleHere, l);
        default: return "";
    }
}

}  // namespace hmi
