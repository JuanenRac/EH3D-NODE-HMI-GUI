// EH3D-NODE-HMI-GUI - core/settings_text.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The sentences of the page of the settings of the server, in the eight languages of the tables (the order of lang.hpp: en, es, it, de, fr, pt, zh, ja); the other twenty-nine show the
// English ones, like the refusals of the server (server_errors.hpp). A text with a character that the fonts of the screen do not hold is not drawn (it would be boxes): the English
// text is shown instead until the fonts are built with the characters of this file. `{name}` holes are filled by `fill`. The words hmi.cfg, server.cfg and the names of the keys never change.
#pragma once

#include <array>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <utility>

#include "font_ranges.hpp"
#include "lang.hpp"

namespace hmi {

enum class ST : unsigned {
    Title, SourceLive, SourceCopy,
    WhyUnreachable, WhyForbidden, WhyShape, WhyTooLong,
    On, Off, Hidden, RestartTag, Default,
    RoFile, RoSecret, RoCopy, RoRole, RoDisabled, RoNotListed, RoRunning, RoUnknown, RoRoute,
    AskChange, AskAgain, AskWeak, Saved, SavedRestart, Refused, NotSent,
    BadNumber, BadWhole, BadText, BadChoice,
    NewKeys, RackWarn, BodyWarn,
    SourceLight, NotAvailable, HashDiffers,
    Count
};

inline const Row& settings_text_row(ST t) {
    static const std::array<Row, static_cast<std::size_t>(ST::Count)> rows = {{
        {"Server settings", "Ajustes del servidor", "Impostazioni del server", "Servereinstellungen", "Réglages du serveur", "Definições do servidor", "服务器设置", "サーバー設定"},
        {"Values in force, read from the server", "Valores en vigor, leídos del servidor", "Valori in vigore, letti dal server", "Aktuelle Werte, vom Server gelesen", "Valeurs en vigueur, lues sur le serveur",
         "Valores em vigor, lidos do servidor", "当前生效的值，读自服务器", "現在有効な値（サーバーから取得）"},
        {"Copy kept in the screen: read only, without values", "Copia guardada en la pantalla: solo lectura, sin valores", "Copia conservata nello schermo: sola lettura, senza valori",
         "Im Bildschirm gespeicherte Kopie: nur lesen, ohne Werte", "Copie conservée dans l'écran : lecture seule, sans valeurs", "Cópia guardada no ecrã: só de leitura, sem valores",
         "屏幕内保存的副本：只读，无数值", "画面に保存したコピー：読み取り専用、値なし"},
        {"The server cannot be reached.", "No se puede contactar con el servidor.", "Il server non è raggiungibile.", "Der Server ist nicht erreichbar.", "Le serveur est injoignable.",
         "Não é possível contactar o servidor.", "无法连接服务器。", "サーバーに接続できません。"},
        {"The server gives its settings to an administrator only.", "El servidor solo da sus ajustes a un administrador.", "Il server dà le sue impostazioni solo a un amministratore.",
         "Der Server gibt seine Einstellungen nur an einen Administrator.", "Le serveur ne donne ses réglages qu'à un administrateur.", "O servidor só dá as suas definições a um administrador.",
         "服务器仅向管理员提供其设置。", "サーバーは設定を管理者にのみ提供します。"},
        {"The list of settings of the server is not one this screen understands (it may be newer): shown read only.",
         "La lista de ajustes del servidor no es una que esta pantalla entienda (puede ser más nueva): se muestra solo en lectura.",
         "L'elenco delle impostazioni del server non è uno che questo schermo comprende (potrebbe essere più recente): mostrato in sola lettura.",
         "Die Einstellungsliste des Servers wird von diesem Bildschirm nicht verstanden (vielleicht neuer): nur lesbar angezeigt.",
         "La liste des réglages du serveur n'est pas comprise par cet écran (elle est peut-être plus récente) : affichée en lecture seule.",
         "A lista de definições do servidor não é uma que este ecrã entenda (pode ser mais recente): mostrada só de leitura.",
         "此屏幕无法理解服务器的设置列表（可能更新）：仅以只读方式显示。", "サーバーの設定一覧はこの画面で解釈できません（新しい版かもしれません）：読み取り専用で表示します。"},
        {"The answer of the server is too long for the screen.", "La respuesta del servidor es demasiado larga para la pantalla.", "La risposta del server è troppo lunga per lo schermo.",
         "Die Antwort des Servers ist für den Bildschirm zu lang.", "La réponse du serveur est trop longue pour l'écran.", "A resposta do servidor é demasiado longa para o ecrã.",
         "服务器的响应对屏幕来说太长。", "サーバーの応答が画面には長すぎます。"},
        {"On", "Activado", "Attivo", "Ein", "Activé", "Ativado", "开", "オン"},
        {"Off", "Desactivado", "Spento", "Aus", "Désactivé", "Desativado", "关", "オフ"},
        {"Hidden", "Oculto", "Nascosto", "Verborgen", "Masqué", "Oculto", "已隐藏", "非表示"},
        {"after a restart", "tras reiniciar", "dopo il riavvio", "nach Neustart", "après redémarrage", "após reiniciar", "重启后生效", "再起動後に有効"},
        {"Default", "Por defecto", "Predefinito", "Standard", "Par défaut", "Predefinição", "默认值", "既定値"},
        {"Set in server.cfg; read when the server starts.", "Se fija en server.cfg; se lee al arrancar el servidor.", "Si imposta in server.cfg; letto all'avvio del server.",
         "Wird in server.cfg gesetzt; beim Start des Servers gelesen.", "Défini dans server.cfg ; lu au démarrage du serveur.", "Define-se em server.cfg; lido ao arrancar o servidor.",
         "在 server.cfg 中设置；服务器启动时读取。", "server.cfg で設定し、サーバー起動時に読み込まれます。"},
        {"A secret: never shown or written on the screen.", "Un secreto: nunca se muestra ni se escribe en la pantalla.", "Un segreto: mai mostrato né scritto sullo schermo.",
         "Ein Geheimnis: wird am Bildschirm nie angezeigt oder geschrieben.", "Un secret : jamais affiché ni écrit sur l'écran.", "Um segredo: nunca é mostrado nem escrito no ecrã.",
         "机密项：绝不在屏幕上显示或写入。", "秘密情報：画面には表示も書き込みもしません。"},
        {"Only the live list of the server allows a change.", "Solo la lista en vivo del servidor permite un cambio.", "Solo l'elenco in tempo reale del server consente una modifica.",
         "Nur die Live-Liste des Servers erlaubt eine Änderung.", "Seule la liste en direct du serveur permet un changement.", "Só a lista em direto do servidor permite uma alteração.",
         "只有服务器的实时列表才允许修改。", "サーバーのライブ一覧でのみ変更できます。"},
        {"Only an administrator can change it.", "Solo un administrador puede cambiarlo.", "Solo un amministratore può modificarlo.", "Nur ein Administrator kann es ändern.",
         "Seul un administrateur peut le modifier.", "Só um administrador o pode alterar.", "只有管理员可以修改。", "管理者のみ変更できます。"},
        {"Changing server settings from this screen is off (settings_write in hmi.cfg).", "Cambiar los ajustes del servidor desde esta pantalla está desactivado (settings_write en hmi.cfg).",
         "La modifica delle impostazioni del server da questo schermo è disattivata (settings_write in hmi.cfg).",
         "Das Ändern der Servereinstellungen von diesem Bildschirm ist ausgeschaltet (settings_write in hmi.cfg).",
         "La modification des réglages du serveur depuis cet écran est désactivée (settings_write dans hmi.cfg).",
         "Alterar as definições do servidor a partir deste ecrã está desativado (settings_write em hmi.cfg).", "已关闭在此屏幕上修改服务器设置（hmi.cfg 中的 settings_write）。",
         "この画面からのサーバー設定の変更はオフです（hmi.cfg の settings_write）。"},
        {"This screen may not change this setting (settings_write_keys in hmi.cfg).", "Esta pantalla no puede cambiar este ajuste (settings_write_keys en hmi.cfg).",
         "Questo schermo non può modificare questa impostazione (settings_write_keys in hmi.cfg).", "Dieser Bildschirm darf diese Einstellung nicht ändern (settings_write_keys in hmi.cfg).",
         "Cet écran ne peut pas modifier ce réglage (settings_write_keys dans hmi.cfg).", "Este ecrã não pode alterar esta definição (settings_write_keys em hmi.cfg).",
         "此屏幕无权修改该设置（hmi.cfg 中的 settings_write_keys）。", "この画面ではこの設定を変更できません（hmi.cfg の settings_write_keys）。"},
        {"Not while a job runs.", "No mientras corre un trabajo.", "Non mentre un lavoro è in corso.", "Nicht, solange ein Auftrag läuft.", "Pas pendant qu'un travail est en cours.",
         "Não enquanto um trabalho decorre.", "作业运行时不可修改。", "ジョブの実行中は変更できません。"},
        {"The screen does not know how to edit this setting.", "La pantalla no sabe cómo editar este ajuste.", "Lo schermo non sa come modificare questa impostazione.",
         "Der Bildschirm kann diese Einstellung nicht bearbeiten.", "L'écran ne sait pas modifier ce réglage.", "O ecrã não sabe editar esta definição.", "屏幕不知道如何编辑此设置。",
         "この画面ではこの設定を編集できません。"},
        {"The route that writes it is not one this screen trusts.", "La ruta que lo escribe no es una en la que esta pantalla confía.", "Il percorso che lo scrive non è uno di cui questo schermo si fida.",
         "Die Route, die es schreibt, ist keine, der dieser Bildschirm vertraut.", "La route qui l'écrit n'est pas de celles auxquelles cet écran se fie.",
         "A rota que o escreve não é uma em que este ecrã confie.", "写入它的路径不在此屏幕信任的范围内。", "これを書き込む経路はこの画面が信頼するものではありません。"},
        {"Change {label} to {value}?", "¿Cambiar {label} a {value}?", "Cambiare {label} in {value}?", "{label} auf {value} ändern?", "Changer {label} en {value} ?", "Alterar {label} para {value}?",
         "将 {label} 改为 {value}？", "{label} を {value} に変更しますか？"},
        {"This can open the server or weaken its security. Confirm again to apply it.", "Esto puede abrir el servidor o debilitar su seguridad. Confirma otra vez para aplicarlo.",
         "Questo può aprire il server o indebolirne la sicurezza. Conferma di nuovo per applicarlo.", "Das kann den Server öffnen oder seine Sicherheit schwächen. Bestätigen Sie erneut, um es anzuwenden.",
         "Cela peut ouvrir le serveur ou affaiblir sa sécurité. Confirmez à nouveau pour l'appliquer.", "Isto pode abrir o servidor ou enfraquecer a sua segurança. Confirme outra vez para aplicar.",
         "这可能使服务器暴露或削弱其安全性。请再次确认以应用。", "サーバーが外部に開放されたり、安全性が弱まったりする可能性があります。適用するには、もう一度確認してください。"},
        {"The server says this value is looser than recommended. Apply it anyway?", "El servidor dice que este valor es más laxo de lo recomendado. ¿Aplicarlo igualmente?",
         "Il server dice che questo valore è meno restrittivo del consigliato. Applicarlo comunque?", "Der Server meldet, dass dieser Wert lockerer ist als empfohlen. Trotzdem anwenden?",
         "Le serveur indique que cette valeur est plus laxiste que recommandé. L'appliquer quand même ?", "O servidor diz que este valor é mais permissivo do que o recomendado. Aplicar mesmo assim?",
         "服务器提示该值比推荐的更宽松。仍要应用吗？", "サーバーは、この値が推奨より緩いと伝えています。それでも適用しますか？"},
        {"Saved: {label}", "Guardado: {label}", "Salvato: {label}", "Gespeichert: {label}", "Enregistré : {label}", "Guardado: {label}", "已保存：{label}", "保存しました：{label}"},
        {"Saved: {label}. It takes effect when the server restarts.", "Guardado: {label}. Se aplica al reiniciar el servidor.", "Salvato: {label}. Ha effetto al riavvio del server.",
         "Gespeichert: {label}. Wirkt nach einem Neustart des Servers.", "Enregistré : {label}. Pris en compte au redémarrage du serveur.", "Guardado: {label}. Aplica-se ao reiniciar o servidor.",
         "已保存：{label}。服务器重启后生效。", "保存しました：{label}。サーバーの再起動後に有効になります。"},
        {"The server refused it: {why}", "El servidor lo rechazó: {why}", "Il server l'ha rifiutato: {why}", "Der Server hat es abgelehnt: {why}", "Le serveur l'a refusé : {why}",
         "O servidor recusou: {why}", "服务器拒绝：{why}", "サーバーが拒否しました：{why}"},
        {"Nothing was sent: {why}", "No se envió nada: {why}", "Non è stato inviato nulla: {why}", "Es wurde nichts gesendet: {why}", "Rien n'a été envoyé : {why}", "Não foi enviado nada: {why}",
         "未发送任何内容：{why}", "何も送信されませんでした：{why}"},
        {"Enter a whole number from {min} to {max}.", "Escribe un número entero de {min} a {max}.", "Inserisci un numero intero da {min} a {max}.", "Geben Sie eine ganze Zahl von {min} bis {max} ein.",
         "Saisissez un nombre entier de {min} à {max}.", "Introduza um número inteiro de {min} a {max}.", "请输入 {min} 到 {max} 之间的整数。", "{min} から {max} までの整数を入力してください。"},
        {"Enter a whole number.", "Escribe un número entero.", "Inserisci un numero intero.", "Geben Sie eine ganze Zahl ein.", "Saisissez un nombre entier.", "Introduza um número inteiro.",
         "请输入整数。", "整数を入力してください。"},
        {"Too long: {max} characters at most.", "Demasiado largo: {max} caracteres como máximo.", "Troppo lungo: al massimo {max} caratteri.", "Zu lang: höchstens {max} Zeichen.",
         "Trop long : {max} caractères au maximum.", "Demasiado longo: no máximo {max} carateres.", "太长：最多 {max} 个字符。", "長すぎます：最大 {max} 文字。"},
        {"Not one of the choices.", "No es una de las opciones.", "Non è una delle opzioni.", "Keine der Optionen.", "Ce n'est pas l'une des options.", "Não é uma das opções.", "不是可选项之一。",
         "選択肢にありません。"},
        {"{n} settings of the server are not in the copy of the screen; they are shown read only.", "{n} ajustes del servidor no están en la copia de la pantalla; se muestran solo en lectura.",
         "{n} impostazioni del server non sono nella copia dello schermo; sono mostrate in sola lettura.", "{n} Einstellungen des Servers fehlen in der Kopie des Bildschirms; sie werden nur lesbar angezeigt.",
         "{n} réglages du serveur ne sont pas dans la copie de l'écran ; ils sont affichés en lecture seule.", "{n} definições do servidor não estão na cópia do ecrã; são mostradas só de leitura.",
         "服务器的 {n} 项设置不在屏幕副本中；以只读方式显示。", "サーバーの {n} 件の設定が画面のコピーにありません。読み取り専用で表示します。"},
        {"The tool-changer bar of the server has {server} places but hmi.cfg uses up to place {screen} (rack_places).", "La barra del cambiador del servidor tiene {server} lugares pero hmi.cfg usa hasta el lugar {screen} (rack_places).",
         "La barra del cambio utensili del server ha {server} posti ma hmi.cfg arriva al posto {screen} (rack_places).", "Die Wechslerleiste des Servers hat {server} Plätze, hmi.cfg nutzt aber bis Platz {screen} (rack_places).",
         "La barre du changeur du serveur a {server} places mais hmi.cfg va jusqu'à la place {screen} (rack_places).", "A barra do trocador do servidor tem {server} lugares mas hmi.cfg usa até ao lugar {screen} (rack_places).",
         "服务器的换刀器刀库有 {server} 个位置，但 hmi.cfg 用到第 {screen} 位（rack_places）。", "サーバーのツールチェンジャーのバーは {server} か所ですが、hmi.cfg は {screen} 番まで使っています（rack_places）。"},
        {"The file is larger than the {mb} MB the server takes in a request.", "El archivo es mayor que los {mb} MB que el servidor admite en una petición.", "Il file supera i {mb} MB che il server accetta in una richiesta.",
         "Die Datei ist größer als die {mb} MB, die der Server pro Anfrage annimmt.", "Le fichier dépasse les {mb} Mo que le serveur accepte dans une requête.", "O ficheiro é maior do que os {mb} MB que o servidor aceita num pedido.",
         "文件大于服务器单次请求接受的 {mb} MB。", "ファイルがサーバーが1回のリクエストで受け付ける {mb} MB を超えています。"},
        {"Values in force, read from the server (light view); texts from the copy kept in the screen", "Valores en vigor, leídos del servidor (vista ligera); textos de la copia guardada en la pantalla",
         "Valori in vigore, letti dal server (vista leggera); testi dalla copia conservata nello schermo", "Aktuelle Werte, vom Server gelesen (leichte Ansicht); Texte aus der im Bildschirm gespeicherten Kopie",
         "Valeurs en vigueur, lues sur le serveur (vue légère) ; textes de la copie conservée dans l'écran", "Valores em vigor, lidos do servidor (vista leve); textos da cópia guardada no ecrã",
         "当前生效的值，读自服务器（精简视图）；文本来自屏幕内保存的副本", "現在有効な値（サーバーから取得、簡易表示）；文言は画面に保存したコピーから"},
        {"Not shown by the server", "El servidor no lo muestra", "Il server non lo mostra", "Vom Server nicht angezeigt", "Non communiqué par le serveur", "O servidor não o mostra", "服务器未提供", "サーバーは表示しません"},
        {"The list of settings of the server is not the copy this screen carries: texts and keys may be out of date.", "La lista de ajustes del servidor no es la copia que lleva esta pantalla: los textos y las claves pueden estar desactualizados.",
         "L'elenco delle impostazioni del server non è la copia di questo schermo: testi e chiavi potrebbero non essere aggiornati.", "Die Einstellungsliste des Servers ist nicht die Kopie dieses Bildschirms: Texte und Schlüssel können veraltet sein.",
         "La liste des réglages du serveur n'est pas la copie de cet écran : textes et clés peuvent être périmés.", "A lista de definições do servidor não é a cópia deste ecrã: os textos e as chaves podem estar desatualizados.",
         "服务器的设置列表与此屏幕携带的副本不一致：文本和键可能已过期。", "サーバーの設定一覧はこの画面のコピーと一致しません：文言やキーが古い可能性があります。"},
    }};
    return rows[static_cast<std::size_t>(t)];
}

namespace detail {
/// Whether every character of a UTF-8 text is in the fonts of the screen; a malformed sequence is not (the same rule as `text_in_font` of server_errors.hpp, kept here so that this file stands alone).
inline bool drawable(const char* utf8) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8);
    while (*p != 0) {
        unsigned cp = 0;
        int extra = 0;
        if (*p < 0x80) { cp = *p; extra = 0; }
        else if ((*p & 0xE0) == 0xC0) { cp = *p & 0x1Fu; extra = 1; }
        else if ((*p & 0xF0) == 0xE0) { cp = *p & 0x0Fu; extra = 2; }
        else if ((*p & 0xF8) == 0xF0) { cp = *p & 0x07u; extra = 3; }
        else return false;
        ++p;
        for (int i = 0; i < extra; ++i, ++p) {
            if ((*p & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (*p & 0x3Fu);
        }
        if (cp < 0x20) continue;
        bool found = false;
        for (std::size_t i = 0; i < kFontRangeCount && !found; ++i) found = kFontRanges[i].lo <= cp && cp <= kFontRanges[i].hi;
        if (!found) return false;
    }
    return true;
}
}  // namespace detail

/// The sentence in the language of the operator; the English one when the text of the language cannot be drawn with the fonts of the screen.
inline std::string settings_text(ST t, Lang l) {
    const Row& row = settings_text_row(t);
    const char* wanted = pick(row, l);
    return detail::drawable(wanted) ? wanted : row[0];
}

/// A sentence with its `{name}` holes filled.
inline std::string fill(std::string text, std::initializer_list<std::pair<const char*, std::string>> vars) {
    for (const auto& v : vars) {
        const std::string hole = std::string("{") + v.first + "}";
        for (std::size_t at = text.find(hole); at != std::string::npos; at = text.find(hole, at + v.second.size())) text.replace(at, hole.size(), v.second);
    }
    return text;
}

}  // namespace hmi
