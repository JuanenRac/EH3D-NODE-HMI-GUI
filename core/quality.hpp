// EH3D-NODE-HMI-GUI - core/quality.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The quality check of the work, from the screen of the machine: the camera of the drill head looks at the board, the server judges the picture with the analyser of the
// shop (the same one the desktop program uses) and the screen shows one verdict that can be read from across the room: OK, NOT OK, or "cannot judge" when the picture was too
// poor. The screen also turns the ring of LEDs around the camera on and off. Everything goes through EH3D-SERVER, which holds the token of the node and the profile of the
// machine; this file builds the requests and reads the answers, so all of it is tested on a computer.
#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "lang.hpp"
#include "locales.hpp"
#include "server_link.hpp"

namespace hmi {

// ---- the requests --------------------------------------------------------------------------------------------------------------------------

/// The levels of the ring that the screen offers for the picture; 0 turns it off.
constexpr int kLightLevels[4] = {0, 30, 60, 80};

/// The ring on in the white of the camera at a level (0: off).
inline Request light_request(const std::string& machine, int level) {
    const std::string base = "/api/v1/machines/" + machine + "/head/light";
    if (level <= 0) return {"POST", base + "?on=0", ""};
    return {"POST", base + "?on=1&level=" + std::to_string(level > 100 ? 100 : level) + "&tone=white&sectors=all", ""};
}

/// The ring as it is now.
inline Request light_status_request(const std::string& machine) { return {"GET", "/api/v1/machines/" + machine + "/head/light", ""}; }

/// The check: the ring is lit at `light` (0: left as it is), the picture is taken, and the ring shows how it went. `hole_mm` is the width of the hole of the job that is in the
/// centre of the picture (0: none, only the picture and the channels are judged).
inline Request inspect_request(const std::string& machine, int light, double hole_mm) {
    char hole[96];
    if (hole_mm > 0.0) std::snprintf(hole, sizeof hole, "[{\"x_mm\":0,\"y_mm\":0,\"dia_mm\":%.3f}]", hole_mm); else std::snprintf(hole, sizeof hole, "[]");
    std::string body = std::string("{\"holes\":") + hole;
    if (light > 0) body += ",\"light\":" + std::to_string(light > 100 ? 100 : light) + ",\"restore_light\":true";
    body += "}";
    return {"POST", "/api/v1/machines/" + machine + "/head/inspect", body};
}

/// The last check of the machine (the screen asks for it when it opens, so it shows what the desktop program last found too).
inline Request inspect_last_request(const std::string& machine) { return {"GET", "/api/v1/machines/" + machine + "/head/inspect", ""}; }

/// Makes the profile one of the presets: `standard`, `fine_pitch` or `prototype`; the scale of the picture is kept.
inline Request preset_request(const std::string& machine, int index) {
    static const char* const names[3] = {"standard", "fine_pitch", "prototype"};
    return {"PUT", "/api/v1/machines/" + machine + "/head/inspect/profile", std::string("{\"preset\":\"") + names[index < 0 ? 0 : index > 2 ? 2 : index] + "\"}"};
}

/// Sets the scale of the picture from the hole that was just measured in the centre.
inline Request calibrate_request(const std::string& machine) { return {"POST", "/api/v1/machines/" + machine + "/head/inspect/calibrate", "{}"}; }

// ---- the answers ---------------------------------------------------------------------------------------------------------------------------

enum class QualityVerdict { None, Ok, NotOk, Unknown };

struct Finding {
    std::string code;  // `hole_missing`, `mill_pinched`, `image_too_dark` ...
    int hole = -1;     // the number of the hole (from 0) it is about, -1 for none
    bool fails = true;
};

struct QualityView {
    QualityVerdict verdict = QualityVerdict::None;
    int holes_total = 0;
    int holes_ok = 0;
    double channel_mm = 0.0;  // the middle width of the milled channels, 0 when none were seen
    std::vector<Finding> findings;
};

/// The ring as the node says it.
struct LightView {
    bool known = false;
    bool on = false;
    int level = 0;
};

/// The report of the server (the answer of a check, or of the last one); false when the text is not one.
inline bool parse_quality(const std::string& body, QualityView& out) {
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object) return false;
    const std::string v = j.str("verdict");
    QualityView q;
    if (v == "ok") q.verdict = QualityVerdict::Ok;
    else if (v == "not_ok") q.verdict = QualityVerdict::NotOk;
    else if (v == "unknown") q.verdict = QualityVerdict::Unknown;
    else return false;
    if (const Json* holes = j.get("holes"); holes && holes->type == Json::Type::Array) {
        for (const Json& h : holes->a) {
            ++q.holes_total;
            const Json* ok = h.get("ok");
            if (ok && ok->type == Json::Type::Bool && ok->b) ++q.holes_ok;
        }
    }
    if (const Json* m = j.get("milling"); m && m->type == Json::Type::Object && m->num("samples") > 0) q.channel_mm = m->num("width_mm");
    if (const Json* f = j.get("findings"); f && f->type == Json::Type::Array) {
        for (const Json& item : f->a) {
            Finding x;
            x.code = item.str("code");
            const Json* hole = item.get("hole");
            if (hole && hole->type == Json::Type::Number) x.hole = static_cast<int>(hole->n);
            const Json* fails = item.get("fails");
            x.fails = !(fails && fails->type == Json::Type::Bool && !fails->b);
            if (!x.code.empty()) q.findings.push_back(std::move(x));
        }
    }
    out = std::move(q);
    return true;
}

/// The ring from the answer of the node (through the server).
inline bool parse_light(const std::string& body, LightView& out) {
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object) return false;
    const Json* on = j.get("on");
    if (!on || on->type != Json::Type::Bool) return false;
    out.known = true;
    out.on = on->b;
    out.level = static_cast<int>(j.num("level"));
    return true;
}

// ---- the words -----------------------------------------------------------------------------------------------------------------------------

/// The words of the screen of the quality check. Order of the languages: that of lang.hpp (en, es, it, de, fr, pt, zh, ja).
enum class Q : unsigned {
    Title, Light, LightOff, CheckNow, Checking, VerdictOk, VerdictNotOk, VerdictUnknown, NotChecked, HoleWidth, HoleNone, Profile, Standard, FinePitch, Prototype,
    Calibrate, HolesOk, ChannelWidth, NeedServer,
    Count
};

inline const Row& quality_row(Q q) {
    static const std::array<Row, static_cast<std::size_t>(Q::Count)> rows = {{
        {"Quality check", "Verificación de calidad", "Controllo qualità", "Qualitätsprüfung", "Contrôle qualité", "Verificação de qualidade", "质量检查", "品質チェック"},
        {"Light ring", "Aro de luz", "Anello luminoso", "Lichtring", "Anneau lumineux", "Anel de luz", "环形灯", "リングライト"},
        {"Off", "Apagado", "Spento", "Aus", "Éteint", "Desligado", "关", "オフ"},
        {"Check now", "Verificar ahora", "Controlla ora", "Jetzt prüfen", "Contrôler maintenant", "Verificar agora", "立即检查", "今すぐチェック"},
        {"Checking...", "Verificando...", "Controllo in corso...", "Prüfe...", "Contrôle en cours...", "A verificar...", "正在检查...", "チェック中..."},
        {"OK", "CORRECTO", "CONFORME", "IN ORDNUNG", "VALIDE", "APROVADO", "合格", "合格OK"},
        {"NOT OK", "NO CORRECTO", "NON CONFORME", "NICHT IN ORDNUNG", "NON VALIDE", "REPROVADO", "不合格", "不合格NG"},
        {"CANNOT JUDGE", "NO SE PUEDE JUZGAR", "NON VALUTABILE", "NICHT BEURTEILBAR", "NON ÉVALUABLE", "NÃO AVALIÁVEL", "无法判定", "判定不可"},
        {"Not checked yet", "Aún sin verificar", "Non ancora controllato", "Noch nicht geprüft", "Pas encore contrôlé", "Ainda sem verificar", "尚未检查", "未チェック"},
        {"Hole in the centre (mm)", "Agujero en el centro (mm)", "Foro al centro (mm)", "Bohrung in der Mitte (mm)", "Trou au centre (mm)", "Furo ao centro (mm)", "中心孔径（毫米）", "中央の穴径 (mm)"},
        {"no hole", "sin agujero", "nessun foro", "keine Bohrung", "pas de trou", "sem furo", "无孔", "穴なし"},
        {"Profile", "Perfil", "Profilo", "Profil", "Profil", "Perfil", "配置", "プロファイル"},
        {"Standard", "Estándar", "Standard", "Standard", "Standard", "Padrão", "标准", "標準"},
        {"Fine pitch", "Paso fino", "Passo fine", "Feinraster", "Pas fin", "Passo fino", "细间距", "ファインピッチ"},
        {"Prototype", "Prototipo", "Prototipo", "Prototyp", "Prototype", "Protótipo", "原型", "試作"},
        {"Calibrate from this hole", "Calibrar con este agujero", "Calibra con questo foro", "Mit diesem Loch kalibrieren", "Étalonner avec ce trou", "Calibrar com este furo", "用此孔校准", "この穴で校正"},
        {"holes ok", "agujeros bien", "fori ok", "Bohrungen in Ordnung", "trous conformes", "furos bem", "个孔合格", "穴 合格"},
        {"channel width", "ancho de canal", "larghezza canale", "Kanalbreite", "largeur de canal", "largura do canal", "沟槽宽度", "溝の幅"},
        {"Set the machine of the server in hmi.cfg.", "Indica la máquina del servidor en hmi.cfg.", "Imposta la macchina del server in hmi.cfg.", "Maschine des Servers in hmi.cfg eintragen.", "Indiquez la machine du serveur dans hmi.cfg.", "Indica a máquina do servidor em hmi.cfg.", "请在 hmi.cfg 中设置服务器的机床。", "hmi.cfg にサーバーのマシンを設定してください。"},
    }};
    return rows[static_cast<std::size_t>(q)];
}

static_assert(static_cast<std::size_t>(Q::Count) == kQualityKeyCount, "the keys of locales.hpp follow the enum Q");

inline const char* quality_text(Q q, Lang l) { return translated(l, kUiKeyCount + static_cast<std::size_t>(q), quality_row(q)); }

struct FindingEntry {
    const char* code;
    Row row;
};

/// The words of the findings of the analyser, in the order of the keys `f.` of locales.hpp.
inline const std::array<FindingEntry, kFindingKeyCount>& finding_table() {
    using Entry = FindingEntry;
    static const std::array<Entry, kFindingKeyCount> table = {{
        {"hole_missing", {"no hole there", "no hay agujero", "nessun foro", "keine Bohrung", "pas de trou", "não há furo", "此处无孔", "穴がありません"}},
        {"hole_wrong_size", {"wrong size", "tamaño incorrecto", "dimensione errata", "falsche Größe", "mauvaise taille", "tamanho errado", "孔径不符", "サイズ不良"}},
        {"hole_off_centre", {"off centre", "descentrado", "fuori centro", "außermittig", "décentré", "descentrado", "偏离中心", "中心ずれ"}},
        {"hole_not_round", {"not round", "no es redondo", "non rotondo", "nicht rund", "pas rond", "não é redondo", "不圆", "真円でない"}},
        {"hole_debris", {"burrs or dust around", "rebabas o polvo alrededor", "bave o polvere attorno", "Grat oder Staub rundherum", "bavures ou poussière autour", "rebarbas ou pó à volta", "周围有毛刺或灰尘", "周囲にバリや粉塵"}},
        {"hole_out_of_view", {"out of view", "fuera de la imagen", "fuori immagine", "außerhalb des Bildes", "hors de l'image", "fora da imagem", "不在画面内", "画像の外"}},
        {"mill_no_channels", {"no channels seen", "no se ven canales", "nessun canale visibile", "keine Kanäle sichtbar", "aucun canal visible", "não se veem canais", "未看到沟槽", "溝が見えません"}},
        {"mill_too_narrow", {"channels too narrow", "canales demasiado estrechos", "canali troppo stretti", "Kanäle zu schmal", "canaux trop étroits", "canais demasiado estreitos", "沟槽过窄", "溝が狭すぎます"}},
        {"mill_too_wide", {"channels too wide", "canales demasiado anchos", "canali troppo larghi", "Kanäle zu breit", "canaux trop larges", "canais demasiado largos", "沟槽过宽", "溝が広すぎます"}},
        {"mill_pinched", {"a channel pinches: risk of a short", "un canal se estrecha: riesgo de cortocircuito", "un canale si restringe: rischio di corto", "ein Kanal verengt sich: Kurzschlussgefahr", "un canal se pince : risque de court-circuit", "um canal estreita: risco de curto-circuito", "沟槽变窄：有短路风险", "溝がくびれています：短絡の恐れ"}},
        {"mill_uneven", {"channels uneven", "canales irregulares", "canali irregolari", "Kanäle ungleichmäßig", "canaux irréguliers", "canais irregulares", "沟槽不均匀", "溝が不均一です"}},
        {"image_too_dark", {"picture too dark: more light", "foto demasiado oscura: más luz", "foto troppo scura: più luce", "Bild zu dunkel: mehr Licht", "photo trop sombre : plus de lumière", "foto demasiado escura: mais luz", "图像过暗：需要更多光", "画像が暗すぎます：照明を上げてください"}},
        {"image_too_bright", {"picture too bright: less light", "foto demasiado clara: menos luz", "foto troppo chiara: meno luce", "Bild zu hell: weniger Licht", "photo trop claire : moins de lumière", "foto demasiado clara: menos luz", "图像过亮：需要更少光", "画像が明るすぎます：照明を下げてください"}},
        {"image_low_contrast", {"picture without contrast", "foto sin contraste", "foto senza contrasto", "Bild ohne Kontrast", "photo sans contraste", "foto sem contraste", "图像缺乏对比度", "コントラストがありません"}},
        {"image_blurry", {"picture out of focus", "foto desenfocada", "foto sfocata", "Bild unscharf", "photo floue", "foto desfocada", "图像未对焦", "ピントが合っていません"}},
        {"not_calibrated", {"set the scale first", "fija antes la escala", "imposta prima la scala", "zuerst den Maßstab festlegen", "réglez d'abord l'échelle", "define primeiro a escala", "请先设置比例", "先にスケールを設定してください"}},
        {"nothing_to_check", {"no check is switched on", "no hay ninguna comprobación activa", "nessun controllo attivo", "keine Prüfung aktiv", "aucun contrôle activé", "nenhuma verificação ativa", "未启用任何检查", "有効なチェックがありません"}},
        {"no_expected_holes", {"no hole of the job in view", "ningún agujero del trabajo a la vista", "nessun foro del lavoro in vista", "keine Bohrung des Auftrags im Bild", "aucun trou du travail en vue", "nenhum furo do trabalho à vista", "画面中没有作业孔", "視野にジョブの穴がありません"}},
    }};
    return table;
}

/// What a finding of the analyser says, in a language; the code itself when it is a word this version does not know.
inline const char* finding_text(const std::string& code, Lang l) {
    const auto& table = finding_table();
    for (std::size_t i = 0; i < table.size(); ++i) {
        if (code == table[i].code) return translated(l, kUiKeyCount + kQualityKeyCount + i, table[i].row);
    }
    return code.c_str();
}

/// The line the screen shows for a finding: `#3: no hole there`.
inline std::string finding_line(const Finding& f, Lang l) {
    const std::string words = finding_text(f.code, l);
    return f.hole >= 0 ? "#" + std::to_string(f.hole + 1) + ": " + words : words;
}

/// The widths of the hole that the screen offers for the centre of the picture, from the tools of a Bungard machine (0 is none).
constexpr double kHoleWidths[5] = {0.0, 0.6, 0.8, 1.0, 1.2};

}  // namespace hmi
