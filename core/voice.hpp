// EH3D-NODE-HMI-GUI - core/voice.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The spoken orders and the spoken alerts, in the eight languages.
//
// The speech recogniser gives a text; this file turns it into an order of a short fixed list (never into free text for the machine) and
// gives the sentence the speaker says for each alert. A text that is not on the list is no order. The orders that only halt (stop,
// pause, spindle off) are recognised inside a longer sentence, so "please stop now" stops; the orders that move or spin must be said whole.
#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "gate.hpp"
#include "lang.hpp"

namespace hmi {

enum class Intent { None, Stop, Pause, SpindleOff, Resume, Start, SpindleOn, Home, Yes, No };

namespace detail {

inline std::string normalise(const std::string& text) {
    std::string out;
    bool space = true;
    for (unsigned char c : text) {
        if (c == '.' || c == ',' || c == '!' || c == '?' || c == ';' || c == ':') c = ' ';
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!space) out.push_back(' ');
            space = true;
            continue;
        }
        if (c >= 'A' && c <= 'Z') c = static_cast<unsigned char>(c - 'A' + 'a');
        out.push_back(static_cast<char>(c));
        space = false;
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

struct Phrase {
    Intent intent;
    Row words;  // the phrase of each language; several alternatives are separated by `|`
};

inline const std::array<Phrase, 9>& phrases() {
    static const std::array<Phrase, 9> table = {{
        {Intent::Stop, {"stop|emergency", "para|parar|alto|emergencia", "ferma|fermati|stop|emergenza", "stopp|halt|nothalt", "arrête|arrêt|stop|urgence", "para|parar|pára|emergência", "停止|急停|停", "止めて|停止|ストップ|非常停止"}},
        {Intent::Pause, {"pause|hold", "pausa|pausar", "pausa|metti in pausa", "pause|pausiere", "pause|mets en pause", "pausa|pausar", "暂停", "一時停止|ポーズ"}},
        {Intent::SpindleOff, {"spindle off|stop spindle", "apaga el husillo|parar husillo", "spegni il mandrino|ferma il mandrino", "spindel aus|spindel stoppen", "arrête la broche|broche arrêtée", "desliga o fuso|parar fuso", "关闭主轴|停止主轴", "スピンドル停止|スピンドルオフ"}},
        {Intent::Resume, {"resume|continue", "continúa|continuar|reanuda", "riprendi|continua", "weiter|fortsetzen", "reprends|continue", "continua|retoma", "继续", "再開|続ける"}},
        {Intent::Start, {"start job|start the job", "empieza el trabajo|iniciar trabajo", "avvia il lavoro|inizia il lavoro", "auftrag starten|starte den auftrag", "démarre le travail|lance le travail", "inicia o trabalho|iniciar trabalho", "开始作业|开始任务", "ジョブ開始|作業開始"}},
        {Intent::SpindleOn, {"spindle on|start spindle", "enciende el husillo|arrancar husillo", "accendi il mandrino|avvia il mandrino", "spindel an|spindel starten", "démarre la broche|broche en marche", "liga o fuso|arrancar fuso", "启动主轴|打开主轴", "スピンドル開始|スピンドルオン"}},
        {Intent::Home, {"go home|home the machine", "ve a casa|referenciar", "vai a casa|azzera la macchina", "referenzfahrt|nullpunkt anfahren", "va à l'origine|prise d'origine", "vai a casa|referenciar", "回原点|回零", "原点復帰|ホーム"}},
        {Intent::Yes, {"yes|confirm|okay", "sí|confirmo|vale", "sì|conferma|ok", "ja|bestätigt|okay", "oui|confirme|d'accord", "sim|confirmo|está bem", "是|确认|好的", "はい|確認|オーケー"}},
        {Intent::No, {"no|cancel", "no|cancela|cancelar", "no|annulla", "nein|abbrechen", "non|annule", "não|cancela|cancelar", "不|取消", "いいえ|キャンセル"}},
    }};
    return table;
}

inline bool contains_alternative(const std::string& said, const char* alternatives, bool whole) {
    std::string all = alternatives;
    std::size_t pos = 0;
    while (pos <= all.size()) {
        std::size_t bar = all.find('|', pos);
        if (bar == std::string::npos) bar = all.size();
        std::string phrase = all.substr(pos, bar - pos);
        pos = bar + 1;
        if (phrase.empty()) continue;
        if (whole) {
            if (said == phrase) return true;
        } else {
            std::size_t at = said.find(phrase);
            if (at == std::string::npos) continue;
            // inside a longer sentence a latin word must stand alone, so "stopwatch" is no stop; the scripts without spaces match anywhere
            bool latin = static_cast<unsigned char>(phrase[0]) < 0x80;
            bool before = at == 0 || said[at - 1] == ' ' || !latin;
            bool after = at + phrase.size() == said.size() || said[at + phrase.size()] == ' ' || !latin;
            if (before && after) return true;
        }
    }
    return false;
}

}  // namespace detail

/// The order in a recognised text, or `Intent::None`.
inline Intent recognise(const std::string& text, Lang lang) {
    std::string said = detail::normalise(text);
    if (said.empty()) return Intent::None;
    // the orders that halt come first and are found inside a sentence
    for (const auto& p : detail::phrases()) {
        bool halts = p.intent == Intent::Stop || p.intent == Intent::Pause || p.intent == Intent::SpindleOff;
        if (halts && detail::contains_alternative(said, pick(p.words, lang), false)) return p.intent;
    }
    for (const auto& p : detail::phrases()) {
        bool halts = p.intent == Intent::Stop || p.intent == Intent::Pause || p.intent == Intent::SpindleOff;
        if (!halts && detail::contains_alternative(said, pick(p.words, lang), true)) return p.intent;
    }
    return Intent::None;
}

/// What an order asks of the gate.
inline Kind kind_of(Intent i) {
    switch (i) {
        case Intent::Stop: case Intent::Pause: case Intent::SpindleOff: return Kind::Stop;
        case Intent::Resume: case Intent::Start: case Intent::Home: return Kind::Motion;
        case Intent::SpindleOn: return Kind::Spindle;
        default: return Kind::Info;
    }
}

enum class Alert { JobDone, ToolChange, Error, LinkLost, HardwareStop, Armed };

/// The sentence the speaker says for an alert.
inline const char* alert_text(Alert a, Lang l) {
    static const std::array<Row, 6> rows = {{
        {"The job is finished.", "El trabajo ha terminado.", "Il lavoro è terminato.", "Der Auftrag ist fertig.", "Le travail est terminé.", "O trabalho terminou.", "作业已完成。", "ジョブが完了しました。"},
        {"Change the tool.", "Cambia la herramienta.", "Cambia l'utensile.", "Werkzeug wechseln.", "Change l'outil.", "Muda a ferramenta.", "请更换刀具。", "工具を交換してください。"},
        {"There is an error.", "Hay un error.", "C'è un errore.", "Es liegt ein Fehler vor.", "Il y a une erreur.", "Há um erro.", "出现错误。", "エラーが発生しました。"},
        {"The link to the machine is lost.", "Se ha perdido el enlace con la máquina.", "Il collegamento con la macchina è perso.", "Die Verbindung zur Maschine ist verloren.", "La liaison avec la machine est perdue.", "Perdeu-se a ligação à máquina.", "与机床的连接已丢失。", "マシンとの接続が失われました。"},
        {"The hardware stop is pressed.", "El paro por hardware está pulsado.", "L'arresto hardware è premuto.", "Der Hardware-Stopp ist gedrückt.", "L'arrêt matériel est enfoncé.", "A paragem por hardware está premida.", "硬件急停已按下。", "ハードウェア停止が押されています。"},
        {"The machine is armed.", "La máquina está armada.", "La macchina è armata.", "Die Maschine ist scharf.", "La machine est armée.", "A máquina está armada.", "机床已就绪。", "マシンは有効です。"},
    }};
    return pick(rows[static_cast<std::size_t>(a)], l);
}

}  // namespace hmi
