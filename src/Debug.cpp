#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <esp_cpu.h>
#include <xtensa/core-macros.h>
#include <climits>
#include <algorithm>
#include <vector>
#if defined(__XTENSA__)
#include <xtensa/xtensa_context.h>
#endif
#include <climits>
#include <algorithm>
#include <vector>
#if defined(__XTENSA__)
#include <xtensa/xtensa_context.h>
#endif
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
// 📺 Display
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
// 🔋 Battery
// ⚡ Voltage
// 💥 Crash
// 🐞 Bug
// 💥 Crash
// 🐞 Bug

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
const char Debug::CrashDir[] = "/debug";

namespace
{
    constexpr size_t DebugMarkCount = 10;
}

RTC_NOINIT_ATTR Debug::DebugMark DebugMarks[DebugMarkCount];
RTC_NOINIT_ATTR int DebugMarkPos;

namespace
{
    constexpr uint32_t PanicMagic = 0x504E4B21; // "PNK!"
    constexpr char CrashPrefix[] = "crash.";
    constexpr char CrashSuffix[] = ".log";

    struct PanicRecord
    {
        uint32_t magic;
        uint32_t pc;
        uint32_t sp;
        uint32_t a0;
        uint32_t exccause;
        uint32_t excvaddr;
    };

    RTC_NOINIT_ATTR PanicRecord PanicInfo;
}

static void ClearDebugMark(Debug::DebugMark &mark)
{
    mark.Value = -1;
    mark.LineNumber = 0;
    mark.Filename[0] = '\0';
    mark.Function[0] = '\0';
    mark.CrashInfo[0] = '\0';
}

static void SetMarkString(char *dest, size_t destSize, const char *src)
{
    if (!dest || destSize == 0)
        return;

    if (src && src[0] != '\0')
    {
        strncpy(dest, src, destSize - 1);
        dest[destSize - 1] = '\0';
    }
    else
    {
        dest[0] = '\0';
    }
}

static void StoreDebugMark(int value, int lineNumber, const char *filename, const char *functionName, const char *details)
{
    if (DebugMarkPos < 0 || DebugMarkPos >= static_cast<int>(DebugMarkCount))
        DebugMarkPos = 0;

    DebugMarkPos = (DebugMarkPos + 1) % static_cast<int>(DebugMarkCount);

    Debug::DebugMark &entry = DebugMarks[DebugMarkPos];
    entry.Value = value;
    entry.LineNumber = lineNumber > 0 ? lineNumber : 0;
    SetMarkString(entry.Filename, sizeof(entry.Filename), filename);
    SetMarkString(entry.Function, sizeof(entry.Function), functionName);
    SetMarkString(entry.CrashInfo, sizeof(entry.CrashInfo), details);
}

static bool ParseCrashIndex(const char *name, uint32_t &index)
{
    if (!name)
        return false;

    const char *base = strrchr(name, '/');
    base = base ? base + 1 : name;

    size_t baseLen = strlen(base);
    size_t prefixLen = strlen(CrashPrefix);
    size_t suffixLen = strlen(CrashSuffix);

    if (baseLen <= prefixLen + suffixLen)
        return false;

    if (strncmp(base, CrashPrefix, prefixLen) != 0)
        return false;

    if (strcmp(base + baseLen - suffixLen, CrashSuffix) != 0)
        return false;

    const char *numStart = base + prefixLen;
    size_t numLen = baseLen - prefixLen - suffixLen;
    if (numLen == 0)
        return false;

    uint32_t value = 0;
    for (size_t i = 0; i < numLen; i++)
    {
        char c = numStart[i];
        if (c < '0' || c > '9')
            return false;
        value = value * 10 + static_cast<uint32_t>(c - '0');
    }

    index = value;
    return true;
}

const char *Debug::GetLatestCrashFilePath()
{
    static char latestPath[32];

    File dir = LittleFS.open(CrashDir);
    if (!dir || !dir.isDirectory())
        return nullptr;

    uint32_t maxIndex = 0;
    bool found = false;

    File file = dir.openNextFile();
    while (file)
    {
        uint32_t index = 0;
        if (ParseCrashIndex(file.name(), index))
        {
            if (!found || index > maxIndex)
            {
                maxIndex = index;
                found = true;
            }
        }
        file = dir.openNextFile();
    }

    if (!found)
        return nullptr;

    snprintf(latestPath, sizeof(latestPath), "%s/crash.%05lu.log", CrashDir, static_cast<unsigned long>(maxIndex));
    return latestPath;
}

void Debug::GetCrashLogPaths(std::vector<String> &outPaths, bool newestFirst)
{
    outPaths.clear();

    File dir = LittleFS.open(CrashDir);
    if (!dir || !dir.isDirectory())
        return;

    struct CrashItem
    {
        uint32_t index;
        String path;
    };

    std::vector<CrashItem> items;

    File file = dir.openNextFile();
    while (file)
    {
        uint32_t index = 0;
        if (ParseCrashIndex(file.name(), index))
        {
            String name = String(file.name());
            String path;

            // Just incase the file name has a full path from some unforseen changes to the FS processing used
            String crashPrefix = String(CrashDir) + "/";
            if (name.startsWith(crashPrefix))
                path = name;
            else
                path = crashPrefix + name;

            items.push_back({index, path});
        }
        file = dir.openNextFile();
    }

    std::sort(items.begin(), items.end(), [newestFirst](const CrashItem &a, const CrashItem &b)
              { return newestFirst ? (a.index > b.index) : (a.index < b.index); });

    for (const auto &item : items)
        outPaths.push_back(item.path);
}

// Crash files survive firmware re-flashes - will only blank to nothing
// when filesystem image is re-written
// Crash files are written on a rolling basis, keeping up to 10 most recent (to save space)
// If we reach the max reference number (99999) we wipe all existing crash files and start again from 0
bool Debug::GetNextCrashFilePath(char *outPath, size_t outPathSize)
{
    if (!outPath || outPathSize == 0)
        return false;

    // Return default file if crash folder doesn't exist
    File dir = LittleFS.open(CrashDir);
    if (!dir || !dir.isDirectory())
    {
        if (!LittleFS.mkdir(CrashDir))
            return false;

        // Default filename
        snprintf(outPath, outPathSize, "%s/crash.%05lu.log", CrashDir, 1UL);
        return true;
    }

    uint32_t minIndex = UINT_MAX;
    uint32_t maxIndex = 0;
    uint8_t count = 0;

    File file = dir.openNextFile();
    while (file)
    {
        uint32_t index = 0;
        if (ParseCrashIndex(file.name(), index))
        {
            count++;
            if (index < minIndex)
                minIndex = index;
            if (index > maxIndex)
                maxIndex = index;
        }
        file = dir.openNextFile();
    }

    /// 99999 is a lot of crashes, but you never know over the years!
    if (maxIndex >= 99999)
    {
        File wipeDir = LittleFS.open(CrashDir);
        File wipeFile = wipeDir.openNextFile();
        while (wipeFile)
        {
            uint32_t index = 0;
            // Check if file is a crash file before deleting
            if (ParseCrashIndex(wipeFile.name(), index))
            {
                LittleFS.remove(wipeFile.name());
            }
            wipeFile = wipeDir.openNextFile();
        }

        // Return default crash file, starting from 0 so we have a reference that it's a fresh start
        snprintf(outPath, outPathSize, "%s/crash.%05lu.log", CrashDir, 0UL);
        return true;
    }

    // If there are 10 files already, get rid of the first to keep down storage usage on device
    if (count >= 10 && minIndex != UINT_MAX)
    {
        char removePath[32];
        snprintf(removePath, sizeof(removePath), "%s/crash.%05lu.log", CrashDir, static_cast<unsigned long>(minIndex));
        LittleFS.remove(removePath);
    }

    uint32_t nextIndex = (maxIndex > 0) ? (maxIndex + 1) : 1;
    snprintf(outPath, outPathSize, "%s/crash.%05lu.log", CrashDir, static_cast<unsigned long>(nextIndex));
    return true;
}

extern "C" void __real_esp_panic_handler(void *info);

extern "C" void IRAM_ATTR __wrap_esp_panic_handler(void *info)
{
#if defined(__XTENSA__)
    if (info)
    {
        XtExcFrame *frame = reinterpret_cast<XtExcFrame *>(info);
        PanicInfo.magic = PanicMagic;
        PanicInfo.pc = frame->pc;
        PanicInfo.sp = frame->a1; // stack pointer is a1
        PanicInfo.a0 = frame->a0;
        PanicInfo.exccause = frame->exccause;
        PanicInfo.excvaddr = frame->excvaddr;
    }
#endif
    __real_esp_panic_handler(info);
}

void Debug::Mark(int mark)
{
    StoreDebugMark(mark, 0, nullptr, nullptr, nullptr);
    StoreDebugMark(mark, 0, nullptr, nullptr, nullptr);
}

void Debug::Mark(int mark, const char *details)
void Debug::Mark(int mark, const char *details)
{
    StoreDebugMark(mark, 0, nullptr, nullptr, details);
}

void Debug::Mark(int mark, int lineNumber, const char*filename, const char *function)
{
    StoreDebugMark(mark, lineNumber, filename, function, nullptr);
}

void Debug::Mark(int mark, int lineNumber, const char* filename, const char *function, const char *details)
{
    StoreDebugMark(mark, lineNumber, filename, function, details);
}

// Reset any possible crash info - we've just powered on so there won't be any!
// Reset any possible crash info - we've just powered on so there won't be any!
void Debug::PowerOnInit()
{
    for (size_t i = 0; i < DebugMarkCount; i++)
        ClearDebugMark(DebugMarks[i]);
    DebugMarkPos = 0;
}

static const char *ResetReasonToString(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_POWERON:
        return "Power on";
    case ESP_RST_EXT:
        return "External reset";
    case ESP_RST_SW:
        return "Software reset";
    case ESP_RST_PANIC:
        return "Panic";
    case ESP_RST_INT_WDT:
        return "Interrupt watchdog";
    case ESP_RST_TASK_WDT:
        return "Task watchdog";
    case ESP_RST_WDT:
        return "Other watchdog";
    case ESP_RST_DEEPSLEEP:
        return "Deep sleep";
    case ESP_RST_BROWNOUT:
        return "Brownout";
    case ESP_RST_SDIO:
        return "SDIO";
    default:
        return "Unknown";
    }
    for (size_t i = 0; i < DebugMarkCount; i++)
        ClearDebugMark(DebugMarks[i]);
    DebugMarkPos = 0;
}

static const char *ResetReasonToString(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_POWERON:
        return "Power on";
    case ESP_RST_EXT:
        return "External reset";
    case ESP_RST_SW:
        return "Software reset";
    case ESP_RST_PANIC:
        return "Panic";
    case ESP_RST_INT_WDT:
        return "Interrupt watchdog";
    case ESP_RST_TASK_WDT:
        return "Task watchdog";
    case ESP_RST_WDT:
        return "Other watchdog";
    case ESP_RST_DEEPSLEEP:
        return "Deep sleep";
    case ESP_RST_BROWNOUT:
        return "Brownout";
    case ESP_RST_SDIO:
        return "SDIO";
    default:
        return "Unknown";
    }
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

void Debug::CheckForCrashInfo(esp_reset_reason_t reason)
void Debug::CheckForCrashInfo(esp_reset_reason_t reason)
{
    bool hasMarks = false;
    for (size_t i = 0; i < DebugMarkCount; i++)
    {
        if (DebugMarks[i].Value != -1)
        {
            hasMarks = true;
            break;
        }
    }

    if (!hasMarks && PanicInfo.magic != PanicMagic)
    {
    bool hasMarks = false;
    for (size_t i = 0; i < DebugMarkCount; i++)
    {
        if (DebugMarks[i].Value != -1)
        {
            hasMarks = true;
            break;
        }
    }

    if (!hasMarks && PanicInfo.magic != PanicMagic)
    {
        Serial.println("No crash info found");
        return;
    }

    char crashPath[32];
    if (!GetNextCrashFilePath(crashPath, sizeof(crashPath))) {
        Serial.print("💥 ⛔ Critical failure, unable to create crash file. No crash details will be saved.");
        return;
    }

    Serial.println("💥 Crash info found, logging to " + String(crashPath) + " - resulting file should also be visible in web interface");

    // Crash file picked up! Save this
    File file = LittleFS.open(crashPath, FILE_WRITE);
    File file = LittleFS.open(crashPath, FILE_WRITE);
    if (file)
    {
        file.println("ResetReason: " + String(ResetReasonToString(reason)) + " (" + String((int)reason) + ")");
        
        if (PanicInfo.magic == PanicMagic)
        {
            file.println("PanicInfo:");
            file.printf("  pc=0x%08lx sp=0x%08lx a0=0x%08lx\n", PanicInfo.pc, PanicInfo.sp, PanicInfo.a0);
            file.printf("  exccause=%lu excvaddr=0x%08lx\n", PanicInfo.exccause, PanicInfo.excvaddr);
        }
        
        file.println("Marks (newest first):");

        for (size_t i = 0; i < DebugMarkCount; i++)
        {
            int idx = DebugMarkPos - static_cast<int>(i);
            if (idx < 0)
                idx += static_cast<int>(DebugMarkCount);

            const Debug::DebugMark &mark = DebugMarks[idx];
            if (mark.Value == -1)
                continue;

            file.print("Value: ");
            file.print(mark.Value);

            if (mark.LineNumber > 0)
            {
                file.print(" Line: ");
                file.print(mark.LineNumber);
            }

            if (mark.Filename[0] != '\0')
            {
                file.print(" File: ");
                file.print(mark.Filename);
            }

            if (mark.Function[0] != '\0')
            {
                file.print(" Function: ");
                file.print(mark.Function);
            }

            file.println();

            if (mark.CrashInfo[0] != '\0')
            {
                file.print("Info: ");
                file.println(mark.CrashInfo);
            }
        }
        file.println("ResetReason: " + String(ResetReasonToString(reason)) + " (" + String((int)reason) + ")");
        
        if (PanicInfo.magic == PanicMagic)
        {
            file.println("PanicInfo:");
            file.printf("  pc=0x%08lx sp=0x%08lx a0=0x%08lx\n", PanicInfo.pc, PanicInfo.sp, PanicInfo.a0);
            file.printf("  exccause=%lu excvaddr=0x%08lx\n", PanicInfo.exccause, PanicInfo.excvaddr);
        }
        
        file.println("Marks (newest first):");

        for (size_t i = 0; i < DebugMarkCount; i++)
        {
            int idx = DebugMarkPos - static_cast<int>(i);
            if (idx < 0)
                idx += static_cast<int>(DebugMarkCount);

            const Debug::DebugMark &mark = DebugMarks[idx];
            if (mark.Value == -1)
                continue;

            file.print("Value: ");
            file.print(mark.Value);

            if (mark.LineNumber > 0)
            {
                file.print(" Line: ");
                file.print(mark.LineNumber);
            }

            if (mark.Filename[0] != '\0')
            {
                file.print(" File: ");
                file.print(mark.Filename);
            }

            if (mark.Function[0] != '\0')
            {
                file.print(" Function: ");
                file.print(mark.Function);
            }

            file.println();

            if (mark.CrashInfo[0] != '\0')
            {
                file.print("Info: ");
                file.println(mark.CrashInfo);
            }
        }
        file.close();
    }

    Serial.print("ResetReason: ");
    Serial.print(ResetReasonToString(reason));
    Serial.print(" (");
    Serial.print((int)reason);
    Serial.println(")");


    if (PanicInfo.magic == PanicMagic)
    {
        Serial.printf("PanicInfo: pc=0x%08lx sp=0x%08lx a0=0x%08lx\n", PanicInfo.pc, PanicInfo.sp, PanicInfo.a0);
        Serial.printf("           exccause=%lu excvaddr=0x%08lx\n", PanicInfo.exccause, PanicInfo.excvaddr);
    }

    Serial.println("Marks (newest first):");

    for (size_t i = 0; i < DebugMarkCount; i++)
    {
        int idx = DebugMarkPos - static_cast<int>(i);
        if (idx < 0)
            idx += static_cast<int>(DebugMarkCount);

        const Debug::DebugMark &mark = DebugMarks[idx];
        if (mark.Value == -1)
            continue;

        Serial.print("Value: ");
        Serial.print(mark.Value);

        if (mark.LineNumber > 0)
        {
            Serial.print(" Line: ");
            Serial.print(mark.LineNumber);
        }

        if (mark.Filename[0] != '\0')
        {
            Serial.print(" File: ");
            Serial.print(mark.Filename);
        }

        if (mark.Function[0] != '\0')
        {
            Serial.print(" Function: ");
            Serial.print(mark.Function);
        }

        Serial.println();

        if (mark.CrashInfo[0] != '\0')
        {
            Serial.println("Info: ");
            Serial.println(mark.CrashInfo);
        }
    }

    for (size_t i = 0; i < DebugMarkCount; i++)
        ClearDebugMark(DebugMarks[i]);
    DebugMarkPos = 0;
    PanicInfo.magic = 0;
    Serial.print("ResetReason: ");
    Serial.print(ResetReasonToString(reason));
    Serial.print(" (");
    Serial.print((int)reason);
    Serial.println(")");


    if (PanicInfo.magic == PanicMagic)
    {
        Serial.printf("PanicInfo: pc=0x%08lx sp=0x%08lx a0=0x%08lx\n", PanicInfo.pc, PanicInfo.sp, PanicInfo.a0);
        Serial.printf("           exccause=%lu excvaddr=0x%08lx\n", PanicInfo.exccause, PanicInfo.excvaddr);
    }

    Serial.println("Marks (newest first):");

    for (size_t i = 0; i < DebugMarkCount; i++)
    {
        int idx = DebugMarkPos - static_cast<int>(i);
        if (idx < 0)
            idx += static_cast<int>(DebugMarkCount);

        const Debug::DebugMark &mark = DebugMarks[idx];
        if (mark.Value == -1)
            continue;

        Serial.print("Value: ");
        Serial.print(mark.Value);

        if (mark.LineNumber > 0)
        {
            Serial.print(" Line: ");
            Serial.print(mark.LineNumber);
        }

        if (mark.Filename[0] != '\0')
        {
            Serial.print(" File: ");
            Serial.print(mark.Filename);
        }

        if (mark.Function[0] != '\0')
        {
            Serial.print(" Function: ");
            Serial.print(mark.Function);
        }

        Serial.println();

        if (mark.CrashInfo[0] != '\0')
        {
            Serial.println("Info: ");
            Serial.println(mark.CrashInfo);
        }
    }

    for (size_t i = 0; i < DebugMarkCount; i++)
        ClearDebugMark(DebugMarks[i]);
    DebugMarkPos = 0;
    PanicInfo.magic = 0;
}

// Primarily for testing purposes
void Debug::CrashOnPurpose()
{
    Serial.println("💥 ATTEMPTING TO CRASH DEVICE");
    Serial.println("💥 ATTEMPTING TO CRASH DEVICE");

    Serial.println("💥 Access invalid memory");
    Serial.println("💥 Access invalid memory");
    int *ptr = nullptr;
    int crash = *ptr; // 💥 LoadProhibited panic

    Serial.println("Div by zero");
    int x = 10; // Assume x = 0
    x += -x;
    int y = 100 / x; // 💥 Only if x == 0 at runtime

    Serial.println("💥 Access Invalid Memory");
    Serial.println("💥 Access Invalid Memory");
    volatile int *bad = (int *)0xDEADBEEF;
    crash = *bad;
}