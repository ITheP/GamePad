#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <esp_cpu.h>
#include <xtensa/core-macros.h>
#include "Debug.h"
#include "Config.h"
#include "Defines.h"
#include "Prefs.h"

// Serial friendly characters (show fine in VSCode terminal)

// Common ones used...
// ❌ Failed/false/stop/bad
// ✅ Yes/OK/Success
// ⚠️ Warning
// ℹ️ Information
// ❔ Query
// ❓ Query
// 🛑 Stop
// ⛔ Blocked
// 🚫 Prohibited / Rejected
// ▶️ Play/Start
// ⏸️ Pause
// ⏹️ Stop
// ✋ Raised Hand
// ⭐ Star
// 🌟 Glowing Star
// 🌐 Web
// 📨 Request
// 🌍 Planet
// 💾 Save
// 🔢 Numbers
// 🔣 Symbols
// 📊 Stats/Graph
// 📄 File
// 📁 Folder
// ⏳ Waiting
// ⌛ Done
// 🕒 Time
// ⏱ Stopwatch
// 📺 Display
// 🛜 WiFi
// 📶 Signal Strength
// 🔗 Link
// 📩 Request
// 🗺️ Map
// 🧭 Compass
// 🖥 Desktop
// 💻 Laptop
// 🔌 Serial
// 🔢 Digital
// 🎚 Analog
// 🎩 Hat
// 🔘 Buttons
// 🎛 Inputs
// 🎤 Microphone
// ⌨️ Keyboard
// 🎧 Headphones
// 🎮 Controller
// ⚙️ Settings
// 🔧 Tool
// 👤 Profile
// 👥 Profiles
// 💡 Internal LED
// 🔦 External LED

// Serial output in VSCode appears to be able to display following OK - though editor font and terminal font may differ...
// Sometimes VSCode get's knickers in a twist and doesn't display emoji/icons

//  !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
// `abcdefghijklmnopqrstuvwxyz{|}~☀☁☂☃☄★☆☇☈☉☊☋☌☍☎☏☐☑☒☓☔☕☖☗☘☙☚☛☜☝☞☟☠
// ☡☢☣☤☥☦☧☨☩☪☫☬☭☮☯☰☱☲☳☴☵☶☷☸☹☺☻☼☽☾☿♀♁♂♃♄♅♆♇♈♉♊♋♌♍♎♏♐♑♒♓♔♕♖♗♘♙♚♛♜♝♞♟♠
// ♡♢♣♤♥♦♧♨♩♪♫♬♭♮♯♰♱♲♳♴♵♶♷♸♹♺♻♼♽♾♿⚀⚁⚂⚃⚄⚅⚆⚇⚈⚉⚊⚋⚌⚍⚎⚏⚐⚑⚒⚓⚔⚕⚖⚗⚘⚙⚚⚛⚜⚝⚞⚟⚠
// ⚡⚢⚣⚤⚥⚦⚧⚨⚩⚪⚫⚬⚭⚮⚯⚰⚱⚲⚳⚴⚵⚶⚷⚸⚹⚺⚻⚼⚽⚾⚿⛀⛁⛂⛃⛄⛅⛆⛇⛈⛉⛊⛋⛌⛍⛎⛏⛐⛑⛒⛓⛔⛕⛖⛗⛘⛙⛚⛛⛜⛝⛞⛟⛠
// ⛡⛢⛣⛤⛥⛦⛧⛨⛩⛪⛫⛬⛭⛮⛯⛰⛱⛲⛳⛴⛵⛶⛷⛸⛹⛺⛻⛼⛽⛾⛿✀✁✂✃✄✅✆✇✈✉✊✋✌✍✎✏✐✑✒✓✔✕✖✗✘✙✚✛✜✝✞✟✠
// ✡✢✣✤✥✦✧✨✩✪✫✬✭✮✯✰✱✲✳✴✵✶✷✸✹✺✻✼✽✾✿❀❁❂❃❄❅❆❇❈❉❊❋❌❍❎❏❐❑❒❓❔❕❖❗❘❙❚❛❜❝❞❟❠
// ❡❢❣❤❥❦❧❨❩❪❫❬❭❮❯❰❱❲❳❴❵❶❷❸❹❺❻❼❽❾❿➀➁➂➃➄➅➆➇➈➉➊➋➌➍➎➏➐➑➒➓➔➕➖➗➘➙➚➛➜➝➞➟➠
// ➡➢➣➤➥➦➧➨➩➪➫➬➭➮➯➰➱➲➳➴➵➶➷➸➹➺➻➼➽➾➿⬀⬁⬂⬃⬄⬅⬆⬇⬈⬉⬊⬋⬌⬍⬎⬏⬐⬑⬒⬓⬔⬕⬖⬗⬘⬙⬚⬛⬜⬝⬞⬟⬠
// ⬡⬢⬣⬤⬥⬦⬧⬨⬩⬪⬫⬬⬭⬮⬯⬰⬱⬲⬳⬴⬵⬶⬷⬸⬹⬺⬻⬼⬽⬾⬿⭀⭁⭂⭃⭄⭅⭆⭇⭈⭉⭊⭋⭌⭍⭎⭏⭐⭑⭒⭓⭔⭕⭖⭗⭘⭙⭚⭛⭜⭝⭞⭟⭠
// ⮡⮢⮣⮤⮥⮦⮧⮨⮩⮪⮫⮬⮭⮮⮯⮰⮱⮲⮳⮴⮵⮶⮷⮸⮹⮽⮾⮿⯀⯁⯂⯃⯄⯅⯆⯇⯈⯉⯊⯋⯌⯍⯎⯏⯐⯑
// 🇦🇧🇨🇩🇪🇫🇬🇭🇮🇯🇰🇱🇲🇳🇴🇵🇶🇷🇸🇹🇺🇻🇼🇽🇾🇿🌀🌁🌂🌃🌄🌅
// 🌆🌇🌈🌉🌊🌋🌌🌍🌎🌏🌐🌑🌒🌓🌔🌕🌖🌗🌘🌙🌚🌛🌜🌝🌞🌟🌠🌡🌢🌣🌤🌥🌦🌧🌨🌩🌪🌭🌮🌯🌰🌱🌲🌳🌴🌵🌶🌷🌸 🌹🌺🌻🌼🌽🌾🌿🍀🍁🍂🍃🍄🍅
// 🍆🍇🍈🍉🍊🍋🍌🍍🍎🍏🍐🍑🍒🍓🍔🍕🍖🍗🍘🍙🍚🍛🍜🍝🍞🍟🍠🍡🍢🍣🍤🍥🍦🍧🍨🍩🍪🍫🍬🍭🍮🍯🍰🍱🍲🍳🍴🍵🍶🍷🍸🍹🍺🍻🍼🍽🍾🍿🎀 🎁🎂🎃🎄🎅
// 🎆🎇🎈🎉🎊🎋🎌🎍🎎🎏🎐🎑🎒🎓🎔🎕🎖🎗🎘🎙🎚🎛🎜🎝🎞🎟🎠🎡🎢🎣🎤🎥🎦🎧🎨🎩🎪🎫🎬🎭🎮🎯🎰🎱🎲🎳🎴🎵🎶🎷🎸🎹🎺🎻🎼🎽🎾🎿🏀🏁🏂🏃🏄🏅
// 🏆🏇🏈🏉🏊🏋🏌🏍🏎🏏🏐🏑🏒🏓🏔🏕🏖🏗🏘🏙🏚🏛🏜🏝🏞🏟🏠🏡🏢🏣 🏤🏥🏦🏧🏨🏩🏪🏫🏬🏭🏮🏯🏰🏱🏲🏳🏴🏵🏶🏷🏸🏹🏺🏻🏼🏽🏾🏿🐀🐁🐂🐃🐄🐅
// 🐆🐇🐈🐉🐊🐋🐌🐍🐎🐏🐐🐑🐒🐓🐔🐕🐖🐗🐘🐙🐚🐛🐜🐝🐞🐟🐠🐡🐢🐣🐤🐥🐦🐧🐨🐩🐪🐫🐬🐭🐮🐯🐰🐱🐲🐳🐴🐵🐶🐷🐸🐹🐺🐻🐼🐽🐾🐿👀👁  👂👃👄👅
// 👆👇👈👉👊👋👌👍👎👏👐👑👒👓👔👕👖👗👘👙👚👛👜👝👞👟👠👡👢👣👤👥👦👧👨👩👪👫👬👭👮👯👰👱👲👳👴👵👶👷👸👹👺👻👼👽👾👿💀💁💂💃💄💅
// 💆💇💈💉💊💋💌💍💎💏💐💑💒💓💔💕💖💗💘💙💚💛💜💝💞💟💠💡💢💣💤💥💦💧💨💩💪💫💬💭💮💯💰💱💲💳💴💵💶💷💸💹💺💻💼💽💾💿📀📁📂📃📄📅
// 📆📇📈📉📊📋📌📍📎📏📐📑📒📓📔📕📖📗📘📙📚📛📜📝📞📟📠📡📢📣📤📥📦📧📨📩📪📫📬📭📮📯📰📱📲📳📴📵📶📷📸📹📺📻📼📽📾📿🔀🔁🔂🔃🔄🔅
// 🔆🔇🔈🔉🔊🔋🔌🔍🔎🔏🔐🔑🔒🔓🔔🔕🔖🔗🔘🔙🔚🔛🔜🔝🔞🔟🔠🔡🔢🔣🔤🔥🔦🔧🔨🔩🔪🔫🔬🔭🔮🔯🔰🔱🔲🔳🔴🔵🔶🔷🔸🔹🔺🔻🔼🔽🔾🔿🕀🕁🕂🕃🕄🕅
// 🕆🕇🕈🕉🕊🕋🕌🕍🕎🕏🕐🕑 🕒🕓🕔🕕🕖🕗🕘🕙🕚🕛🕜🕝🕞🕟🕠🕡🕢🕣🕤🕥🕦🕧🕨🕩🕪🕫🕬🕭🕮🕯🕰🕱🕲🕳🕴🕵🕶🕷🕸🕹🕺🕻🕼🕽🕾🕿🖀🖁🖂🖃🖄🖅
// 🖆🖇🖈🖉🖊🖋🖌🖍🖎🖏🖐🖑🖒🖓🖔🖕🖖🖗🖘🖙🖚🖛🖜🖝🖞🖟🖠🖡🖢🖣🖤🖥🖦🖧🖨🖩🖪🖫🖬🖭🖮🖯🖰🖱🖲🖳🖴🖵🖶🖷🖸🖹🖺🖻🖼🖽🖾🖿🗀🗁🗂🗃🗄🗅
// 🗆🗇🗈🗉🗊🗋🗌🗍🗎🗏🗐🗑🗒🗓🗔🗕🗖🗗🗘🗙🗚🗛🗜🗝🗞🗟🗠🗡🗢🗣🗤🗥🗦🗧🗨🗩🗪🗫🗬🗭🗮🗯🗰🗱🗲🗳🗴🗵🗶🗷🗸🗹🗺🗻🗼🗽🗾 🗿😀😁😂😃😄😅
// 😆😇😈😉😊😋😌😍😎😏😐😑😒😓😔😕😖😗😘😙😚😛😜😝😞😟😠😡😢😣😤😥😦😧😨😩😪😫😬😭😮😯😰😱😲😳😴😵😶😷😸😹😺😻😼😽😾😿🙀🙁🙂🙃🙄🙅
// 🙆🙇🙈🙉🙊🙋🙌🙍🙎🙏🚀🚁🚂🚃🚄🚅🚆🚇🚈🚉🚊🚋🚌🚍🚎🚏🚐🚑🚒🚓🚔🚕🚖🚗🚘🚙🚚🚛🚜🚝🚞🚟🚠🚡🚢🚣🚤🚥🚦🚧🚨🚩🚪🚫🚬🚭🚮🚯🚰🚱🚲🚳🚴🚵
// 🚶🚷🚸🚹🚺🚻🚼🚽🚾🚿🛀🛁🛂🛃🛄🛅🛆🛇🛈🛉🛊🛋🛌🛍🛎🛏🛐🛑🛒🛕🛖🛗🛜🛝🛞🛟🛠🛡🛢🛣🛤🛥🛦🛧🛨🛩🛪🛫🛬🛴🛵
// 🛶🛷🛸🛹🛺🛻🛼🤌🤍🤎🤏🤐🤑🤒🤓🤔🤕🤖🤗🤘🤙🤚🤛🤜🤝🤞🤟🤠🤡🤢🤣🤤🤥🤦🤧🤨🤩🤪🤫🤬🤭🤮🤯🤰🤱🤲🤳🤴🤵
// 🤶🤷🤸🤹🤺🤻🤼🤽🤾🤿🥀🥁🥂🥃🥄🥅🥇🥈🥉🥊🥋🥌🥍🥎🥏🥐🥑🥒🥓🥔🥕🥖🥗🥘🥙🥚🥛🥜🥝🥞🥟🥠🥡🥢🥣🥤🥥🥦🥧🥨🥩🥪🥫🥬🥭🥮🥯🥰🥱🥲🥳🥴🥵
// 🥶🥷🥸🥹🥺🥻🥼🥽🥾🥿🦀🦁🦂🦃🦄🦅🦆🦇🦈🦉🦊🦋🦌🦍🦎🦏🦐🦑🦒🦓🦔🦕🦖🦗🦘🦙🦚🦛🦜🦝🦞🦟🦠🦡🦢🦣🦤🦥🦦🦧🦨🦩🦪🦫🦬🦭🦮🦯🦰🦱🦲🦳🦴🦵
// 🦶🦷🦸🦹🦺🦻🦼🦽🦾🦿🧀🧁🧂🧃🧄🧅🧆🧇🧈🧉🧊🧋🧌🧍🧎🧏🧐🧑🧒🧓🧔🧕🧖🧗🧘🧙🧚🧛🧜🧝🧞🧟🧠🧡🧢🧣🧤🧥🧦🧧🧨🧩🧪🧫🧬🧭🧮🧯🧰🧱🧲🧳🧴🧵
// 🧶🧷🧸🧹🧺🧻🧼🧽🧾🧿🩰🩱🩲🩳🩴🩵🩶🩷🩸🩹🩺🩻🩼🪀🪁🪂🪃🪄🪅🪆🪇🪈🪐🪑🪒🪓🪔🪕🪖🪗🪘🪙🪚🪛🪜🪝🪞🪟🪠🪡🪢🪣🪤🪥
// 🪦🪧🪨🪩🪪🪫🪬🪭🪮🪯🪰🪱🪲🪳🪴🪵🪶🪷🪸🪹🪺🪻🪼🪽🫂🫃🫄🫅🫒🫓🫔🫕🫖🫗🫘🫙🫚🫛🫠🫡🫢🫣🫤🫥
// 🫦🫧🫨🫱🫲🫳🫴🫵🫶🫷🫸

// // Convert a Unicode codepoint (0–0x10FFFF) to UTF‑8 bytes
// int utf8Encode(uint32_t cp, char* out)
// {
//     if (cp <= 0x7F) {
//         out[0] = cp;
//         return 1;
//     }
//     else if (cp <= 0x7FF) {
//         out[0] = 0xC0 | (cp >> 6);
//         out[1] = 0x80 | (cp & 0x3F);
//         return 2;
//     }
//     else if (cp <= 0xFFFF) {
//         out[0] = 0xE0 | (cp >> 12);
//         out[1] = 0x80 | ((cp >> 6) & 0x3F);
//         out[2] = 0x80 | (cp & 0x3F);
//         return 3;
//     }
//     else {
//         out[0] = 0xF0 | (cp >> 18);
//         out[1] = 0x80 | ((cp >> 12) & 0x3F);
//         out[2] = 0x80 | ((cp >> 6) & 0x3F);
//         out[3] = 0x80 | (cp & 0x3F);
//         return 4;
//     }
// }

// // Included for our purposes
// // - ASCII
// // - Latin‑1
// // - arrows
// // - box‑drawing
// // - block elements
// // - geometric shapes
// // - misc symbols
// // - dingbats
// // - emoji
// // - math symbols
// // - music notation
// // Excluded for our purposes
// // - control codes
// // - combining marks
// // - surrogate halves
// // - private‑use areas
// // - CJK
// // - Hangul
// // - ancient scripts
// // - variation selectors (except FE0F)

// bool excludeCP(uint32_t cp)
// {
//     // Control characters
//     if (cp < 0x20) return true;
//     if (cp >= 0x7F && cp <= 0x9F) return true;

//     // Combining diacritics
//     if (cp >= 0x0300 && cp <= 0x036F) return true;
//     if (cp >= 0x1AB0 && cp <= 0x1AFF) return true;
//     if (cp >= 0x1DC0 && cp <= 0x1DFF) return true;

//     // UTF‑16 surrogate range
//     if (cp >= 0xD800 && cp <= 0xDFFF) return true;

//     // Private Use Areas
//     if (cp >= 0xE000 && cp <= 0xF8FF) return true;
//     if (cp >= 0xF0000 && cp <= 0xFFFFD) return true;
//     if (cp >= 0x100000 && cp <= 0x10FFFD) return true;

//     // CJK Unified Ideographs
//     if (cp >= 0x3400 && cp <= 0x4DBF) return true;
//     if (cp >= 0x4E00 && cp <= 0x9FFF) return true;
//     if (cp >= 0x20000 && cp <= 0x2A6DF) return true;
//     if (cp >= 0x2A700 && cp <= 0x2B73F) return true;
//     if (cp >= 0x2B740 && cp <= 0x2B81F) return true;
//     if (cp >= 0x2B820 && cp <= 0x2CEAF) return true;
//     if (cp >= 0x2CEB0 && cp <= 0x2EBEF) return true;

//     // Hangul syllables
//     if (cp >= 0xAC00 && cp <= 0xD7AF) return true;

//     // Variation selectors (except FE0F)
//     if (cp >= 0xFE00 && cp <= 0xFE0E) return true;
//     if (cp >= 0xE0100 && cp <= 0xE01EF) return true;

//     // Ancient scripts (skip)
//     if (cp >= 0x10000 && cp <= 0x102FF) return true; // Linear B etc.
//     if (cp >= 0x10300 && cp <= 0x1034F) return true; // Old Italic
//     if (cp >= 0x10400 && cp <= 0x1044F) return true; // Deseret
//     if (cp >= 0x10800 && cp <= 0x10FFF) return true; // Misc ancient

//     //if (cp >= 0x1D400 && cp <= 0x1D7FF) return true; // Mathematical Alphanumeric Symbols
//     //if (cp >= 0x1D100 && cp <= 0x1D1FF) return true; // Musical Symbols

//     return false;
// }

// // Returns true if the codepoint should be included in the font/renderer
// bool includeCP(uint32_t cp)
// {
//     // --- BASIC ASCII PRINTABLES ---
//     // Space (0x20) through tilde (0x7E)
//     if (cp >= 0x20 && cp <= 0x7E)
//         return true;

//     // --- NEWLINE / TAB (optional) ---
//     //if (cp == '\n' || cp == '\r' || cp == '\t')
//     //    return true;

//     // --- EXTENDED EMOJI / SYMBOL RANGES (VSCode‑safe) ---
//     // Add only the ranges you want to support.
//     // These are the safe ones for Windows + VSCode terminal.

//     // Misc Symbols + Dingbats
//     if (cp >= 0x2600 && cp <= 0x27BF)
//         return true;

//     // Arrows + geometric shapes (safe subset)
//     if (cp >= 0x2B00 && cp <= 0x2BFF)
//         return true;

//     // Emoji blocks
//     if (cp >= 0x1F300 && cp <= 0x1F5FF)  // Misc symbols & pictographs
//         return true;

//     if (cp >= 0x1F600 && cp <= 0x1F64F)  // Emoticons
//         return true;

//     if (cp >= 0x1F680 && cp <= 0x1F6FF)  // Transport & map
//         return true;

//     if (cp >= 0x1F900 && cp <= 0x1F9FF)  // Supplemental symbols
//         return true;

//     if (cp >= 0x1FA70 && cp <= 0x1FAFF)  // Emoji Extended-A
//         return true;

//     // Skin tones
//     if (cp >= 0x1F3FB && cp <= 0x1F3FF)
//         return true;

//     // Regional indicator flags
//     if (cp >= 0x1F1E6 && cp <= 0x1F1FF)
//         return true;

//     // Emoji presentation selector
//     if (cp == 0xFE0F)
//         return true;

//     // Everything else excluded
//     return false;
// }

// // Outputs characters to Serial - we can see what comes out other end as supported chars in terminal
// void Debug::printUnicodeRange()
// {
//     const int maxPerLine = 64;
//     int count = 0;
//     char buf[5] = {0};

//     for (uint32_t cp = 0x0020; cp <= 0x1FFFF; cp++)
//     {
//         //if (excludeCP(cp))
//         if (!includeCP(cp))
//             continue;

//         int len = utf8Encode(cp, buf);
//         Serial.write((uint8_t*)buf, len);
//         count++;

//         if (count >= maxPerLine) {
//             Serial.println();
//             count = 0;
//         }
//     }

//     if (count > 0)
//         Serial.println();

// }

// Possible extra debug assistance on crashes
// https://kotyara12.ru/iot/remote_esp32_backtrace/


char Debug::CrashFile[] = "/debug/crash.log";
RTC_NOINIT_ATTR char Debug::CrashInfo[1024];
RTC_NOINIT_ATTR int MarkVal;

void Debug::Mark(int mark)
{
    MarkVal = mark;
}

void Debug::RecordMark()
{
    snprintf(CrashInfo + strlen(CrashInfo), sizeof(CrashInfo) - strlen(CrashInfo), ", %d", MarkVal);
}

void Debug::PowerOnInit()
{
    memset(CrashInfo, 0, sizeof(CrashInfo));
    sprintf(CrashInfo, "Marks: Init");
}

// Low level warning flashing of LED to indicate errors
void Debug::WarningFlashes(WarningFlashCodes code)
{
    while (1)
    {
        // Forever flash internal LED to let us know theres something wrong!
        for (int i = 0; i < (int)code; i++)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(150);
            digitalWrite(LED_BUILTIN, LOW);
            delay(150);
        }
        delay(750);
    }
}

// Experiments in device crash logging...
// DOESN'T WORK without changing framework from arduino to espidf (or layering in arduino inside espidf)
// but that in itself suddenly opens a whole can of worms!

// extern "C" void panic_handler(void *frame)
// {
//     //Serial.println("PANIC HANDLER - CRITICAL PROBLEM OCCURED");
//     sprintf(CrashInfo, "PANIC IN THE DISCO");

//     return;

//     XtExcFrame *exc = (XtExcFrame *)frame;
//     uint32_t cause = exc->exccause;
//     uint32_t pc = exc->pc;

//     const char* causeDescription;
//             digitalWrite(LED_BUILTIN, HIGH);
//             delay(500);
//             digitalWrite(LED_BUILTIN, LOW);

//     switch (cause) {
//     case 0:
//         causeDescription = "Illegal Instruction";
//         break;
//     case 3:
//         causeDescription = "LoadProhibited";
//         break;
//     case 6:
//         causeDescription = "StoreProhibited";
//         break;
//     case 9:
//         causeDescription = "DivideByZero";
//         break;
//     default:
//         causeDescription = "Unknown";
//         break;
//     }

//    // char buf[256];

//     snprintf(CrashInfo, sizeof(CrashInfo),
//             "Boot Count: <b>%d</b></br>\n"
//             "Cause Code: <b>%u</b></br>\n"
//             "Cause Description: <b>%s</b></br>\n"
//             "Program Counter (PC): <b>0x%08X</b></br>\n",
//             Prefs::BootCount,
//             cause,
//             causeDescription,
//             pc);

//     Serial.println("Low-Level Crash Detected<");
//     Serial.println("Boot Count: " + String(Prefs::BootCount));
//     Serial.println("Cause Code: " + String(cause));
//     Serial.println("Cause Description: "  + String(causeDescription));
//     Serial.println("Program Counter (PC): 0x" + String(pc, HEX));
//     Serial.println("Timestamp: " + String(millis()));

//     //LittleFS.begin(true);
//     File file = LittleFS.open("/debug/test.log", FILE_WRITE);
//     if (file)
//     {
//         file.println("<h1>Low-Level Crash Detected</h1>");
//         file.println("Boot Count: <b>" + String(Prefs::BootCount) + "</b></br>");
//         file.println("Cause Code: <b>" + String(cause) + "</b></br>");
//         file.println("Cause Description: <b>" + String(causeDescription) + "</b></br>");
//         file.println("Program Counter (PC): <b>0x" + String(pc, HEX) + "</b></br>");
//         file.println("Timestamp: <b>" + String(millis()) + "</b></br>");
//         file.close();
//     }

//     Serial.println("Restarting in 5 seconds");
//     delay(5000);

//     while(true)
//     {
//                     digitalWrite(LED_BUILTIN, HIGH);
//                     delay(100);
//                                 digitalWrite(LED_BUILTIN, LOW);
//                                 delay(100);
//     }

//     esp_restart(); // while(true);
// }

void Debug::CheckForCrashInfo()
{
    RecordMark();

    if (CrashInfo[0] == 0)
        Serial.println("No crash info found");
    return;

    Serial.println("Crash info found, logging to " + String(CrashFile) + " - resulting file should also be visible in web interface");

    // Crash file picked up! Save this
    File file = LittleFS.open(CrashFile, FILE_WRITE);
    if (file)
    {
        file.print(CrashInfo);
        file.close();
    }

    Serial.print(CrashInfo);

    CrashInfo[0] = 0;
}

// Primarily for testing purposes
void Debug::CrashOnPurpose()
{
    Serial.println("ATTEMPTING TO CRASH DEVICE");

    Serial.println("Access invalid memory");
    int *ptr = nullptr;
    int crash = *ptr; // 💥 LoadProhibited panic

    Serial.println("Div by zero");
    int x = 10; // Assume x = 0
    x += -x;
    int y = 100 / x; // 💥 Only if x == 0 at runtime

    Serial.println("Access Invalid Memory");
    volatile int *bad = (int *)0xDEADBEEF;
    crash = *bad;
}