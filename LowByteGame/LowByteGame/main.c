#pragma comment(lib, "raylib.lib")
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include "raylib.h"

// The only headers that are not this file, and both are generated rather than
// written. They hold atlases of exactly the glyphs the UI draws, compressed -
// which is why the game still ships as one binary with no asset directory.
//
// Two sizes because one cannot serve both ends of the range. A pixel font
// scaled far from the size it was baked at turns to mush, and the card list
// draws body text at 15px while the title draws at 56. Baked at 32 alone, the
// small text was a 0.4x downscale and genuinely hard to read.
#include "neodgm16.h"
#include "neodgm32.h"

// ---- localisation --------------------------------------------------------
// Every string is keyed by its own English text rather than by an enum. An
// enum would be faster, but it also means 200 constants that have to be kept
// in step with 200 call sites by hand, and a mismatch there is silent. Keying
// by the English text makes the call site readable on its own and makes a
// missing translation degrade to English instead of to garbage.
//
// The lookup is linear over ~230 entries, run once per drawn string. That is a
// few thousand strcmp calls a frame against a 16ms budget - it does not show up.
//
// What deliberately has NO entry here stays English in both languages, because
// the player reads it as a symbol rather than as a sentence: the title, the
// judgements (PERFECT / GOOD / MISS), the gimmick names (SWAP / BLIND / TRAPS),
// BURST, LEVEL UP, combo, every key name, the result-screen stat labels, and
// all 57 card names.
//
// Format strings are translated at the call site - TextFormat(T("..."), x) -
// because by the time TextFormat has run the key no longer exists. When you add
// one, keep the conversion specifiers identical and in the same order in both
// languages, or the vararg read will walk off the arguments.
typedef struct { const char* en; const char* ko; } Phrase;

static const Phrase phrases[] = {
    // -- title -------------------------------------------------------------
    { "LISTEN to the bar, then PLAY IT BACK", "박자를 듣고, 그대로 연주하세요" },
    { "NORMAL", "노말" },
    { "%s (%d levels)", "%s (%d레벨)" },
    { "HELP", "도움말" },
    { "CARDS", "도감" },
    { "QUIT", "종료" },
    { "RESUME", "계속하기" },
    { "YOUR CARDS", "카드 목록" },
    { "BACK TO TITLE", "타이틀로" },
    { "gacha pull", "뽑기" },
    { "buy cards", "카드 구매" },
    { "LEFT / RIGHT to change", "LEFT / RIGHT로 변경" },
    { "ESC  close", "ESC  닫기" },
    { "ESC to resume", "ESC로 계속하기" },
    { "UP / DOWN / LEFT / RIGHT to choose      SPACE to confirm",
      "UP / DOWN / LEFT / RIGHT 선택      SPACE 결정" },
    { "UP / DOWN to choose      LEFT / RIGHT to pick a skin      SPACE to confirm",
      "UP / DOWN 선택      LEFT / RIGHT 스킨 선택      SPACE 결정" },
    { "UP / DOWN to choose      (#FD) to confirm",
      "UP / DOWN 선택      (#FD) 결정" },
    { "UP / DOWN / LEFT / RIGHT to choose      (#FD) to confirm",
      "UP / DOWN / LEFT / RIGHT 선택      (#FD) 결정" },
    { "UP / DOWN to choose      LEFT / RIGHT to pick a skin      (#FD) to confirm",
      "UP / DOWN 선택      LEFT / RIGHT 스킨 선택      (#FD) 결정" },
    { "(#FR) to go back", "(#FR)로 돌아가기" },
    { "(#FR) to resume", "(#FR)로 계속하기" },
    { "(#FR)  close", "(#FR)  닫기" },
    { "#SB - burst destroys every monster for 4 beats",
      "#SB - 버스트: 4박자 동안 모든 몬스터를 처리합니다" },
    { "UP / DOWN to choose      SPACE to confirm",
      "UP / DOWN 선택      SPACE 결정" },
    { "ENDLESS", "엔드리스" },
    { "BEST  %d", "최고 점수 %d" },
    { "LEVEL  %d      BEST  %d", "최고 레벨 %d      최고 점수 %d" },

    // -- in play -----------------------------------------------------------
    { "SPACE - burst destroys every monster for 4 beats",
      "SPACE - 버스트: 4박자 동안 모든 몬스터를 처리합니다" },
    { "LISTEN  -  hear it, then play it back",
      "LISTEN - 박자를 듣고 그대로 연주하세요" },
    { "%d cards", "카드 %d장" },
    { "ALL CARDS COLLECTED", "모든 카드를 모았습니다" },
    { "BARRIER", "배리어" },
    { "BULWARK", "방벽" },
    { "METAPHOR", "메타포" },
    { "BLOCKED", "방어" },
    { "seed %u", "시드 %u" },

    // -- help --------------------------------------------------------------
    { "HOW TO PLAY", "게임 방법" },
    { "The game plays a bar, then the same rhythm comes at you as monsters.",
      "게임이 한 마디를 연주하면, 같은 리듬의 몬스터가 날아옵니다." },
    { "Press each one the moment its ring meets the yellow gate.",
      "몬스터의 링이 노란 게이트에 겹치는 순간 누르세요." },
    { "ARROW KEYS / WASD\tthe four points on the ring",
      "방향키 / WASD\t링 위의 네 방향" },
    { "\tWASD lets you play with both hands",
      "\tWASD를 사용하면 양손으로 플레이할 수 있습니다." },
    { "SPACE\tfire a BURST once its meter is full",
      "SPACE\t게이지가 차면 BURST를 발동합니다." },
    { "PERFECT is the tight centre of the window, GOOD is the rest of it.",
      "판정 구간의 한가운데가 PERFECT, 나머지가 GOOD입니다." },
    { "PERFECT heals more, scores more, and is the only thing that",
      "PERFECT는 회복량과 점수가 더 높고," },
    { "charges the burst meter.",
      "버스트 게이지는 PERFECT로만 충전됩니다." },

    { "BURST clears every note that reaches the gate for four beats.",
      "BURST는 4박 동안 게이트에 닿는 모든 몬스터를 처리합니다." },
    { "It also wipes out anything that already slipped past you.",
      "이미 지나친 몬스터도 처리합니다." },
    { "While the gate is BLUE the game is playing its call.",
      "게이트가 파란 동안에는 게임이 콜을 연주합니다." },
    { "The call already names where the monsters will land.",
      "콜에서 몬스터가 도착할 위치를 미리 알려줍니다." },
    { "TRAPS\ta diamond with no ring. Never press it.",
      "TRAPS\t링이 없는 마름모입니다. 절대 누르지 마세요." },
    { "SWAP\tthe monsters slide to the opposite point.",
      "SWAP\t몬스터가 링의 반대편으로 이동합니다." },
    { "BLIND\tthe monsters stay hidden until the last moment.",
      "BLIND\t몬스터가 마지막 순간까지 보이지 않습니다." },
    { "Every card has a cost, so taking none is a real choice.",
      "카드에는 모두 대가가 있습니다. 뽑지 않는 것도 하나의 선택입니다." },
    { "DOWN / S\ttake no card", "DOWN / S\t카드를 뽑지 않고 넘어갑니다." },
    { "R\tdeal three new cards", "R\t카드 세 장을 다시 뽑습니다." },
    { "\tthree free a run, then half a burst meter each",
      "\t한 게임에 3회 무료, 이후부터 버스트 게이지 절반 소모" },
    { "The shop sells a card to carry into the next run, more maximum",
      "상점에서는 다음 게임에 가지고 시작할 카드, 최대 체력 증가," },
    { "health, and a burst meter that starts part filled.",
      "일부 채워진 버스트 게이지를 판매합니다." },
    { "A card is drafted at level 2, then every third level after.",
      "카드는 2레벨에 한 번, 이후 3레벨마다 뽑습니다." },
    { "page %d / %d      LEFT / RIGHT", "페이지 %d / %d      LEFT / RIGHT" },
    { "ESC to go back", "ESC로 돌아가기" },

    // -- options -----------------------------------------------------------
    { "OPTIONS", "설정" },
    { "VOLUME", "음량" },
    { "BORDERLESS", "전체 화면" },
    { "MOTION COMFORT", "화면 흔들림 완화" },
    { "UP / DOWN to choose      LEFT / RIGHT to change",
      "UP / DOWN 선택      LEFT / RIGHT 변경" },
    { "no screen shake, no zoom pulse, gentler tunnel",
      "화면 흔들림 및 확대 진동 없음, 터널 휘어짐 완화" },
    { "LANGUAGE", "언어" },
    { "MONITOR", "모니터" },

    // -- credits -----------------------------------------------------------
    { "SHOP", "상점" },
    { "CARD LIST", "카드 도감" },
    { "carrying into the next run", "다음 게임에 가져가는 아이템" },
    { "pull once", "1회 뽑기" },
    { "ODDS", "확률" },
    { "see the odds", "확률 보기" },
    { "a card", "카드 1장" },
    { "two cards", "카드 2장" },
    { "max health +10%", "최대 체력 +10%" },
    { "max health +20%", "최대 체력 +20%" },
    { "max health +30%", "최대 체력 +30%" },
    { "start at 50% burst", "버스트 50%로 시작" },
    { "start at 100% burst", "버스트 100%로 시작" },
    { "a skin", "스킨" },
    { "+3 coins", "+3원" },
    { "+5 coins", "+5원" },
    { "+10 coins", "+10원" },
    { "-3 coins", "-3원" },
    { "+100 coins", "+100원" },
    { "%s and %s", "%s + %s" },
    { "+%d coin", "+%d원" },
    { "duplicate (+%d coin)", "중복 (+%d원)" },
    { "-%d coin", "-%d원" },
    { "SKIN", "스킨" },
    { "%d of %d unlocked", "%d / %d 해금" },
    { "one card, one health item and one burst item, next run only",
      "카드·체력·버스트 각 1개씩만, 다음 게임에서 적용됩니다" },
    { "you have %d", "보유  %d원" },
    { "%d coin", "%d원" },
    { "next run: max health +10%", "다음 게임에서 최대 체력 +10%" },
    { "next run: start at 50% burst", "다음 게임 시작 시 버스트 50%로 시작" },
    { "bought", "구매함" },
    { "new cards  (%d coin)", "카드 다시 뽑기  (%d원)" },
    { "reward  +%d", "보상  +%d원" },
    { "CREDITS", "크레딧" },
    { "FONT", "글꼴" },
    { "DEVELOPER", "개발" },
    { "LIBRARY", "라이브러리" },
    { "SIL Open Font License 1.1 - see LICENSE-FONT.txt",
      "SIL Open Font License 1.1 - LICENSE-FONT.txt 참고" },
    { "zlib / libpng license", "zlib / libpng 라이선스" },

    // -- draft -------------------------------------------------------------
    { "CHOOSE ONE", "카드를 선택하세요" },
    { "play your choice on the ring", "방향키로 선택" },
    { "take no card", "카드를 뽑지 않고 넘어가기" },
    { "reroll  (%d left)",    "다시 뽑기 (%d회 남음)" },
    { "reroll  (burst 50%)",  "다시 뽑기 (버스트 50%)" },

    // -- pause -------------------------------------------------------------
    { "PAUSED", "일시정지" },

    // -- results -----------------------------------------------------------
    { "all %d levels", "%d레벨 전부 클리어" },
    { "reached level %d", "%d레벨 도달" },
    { "%d cards      score x%.2f", "카드 %d장      점수 배율 x%.2f" },
    { "NEW BEST!", "신기록!" },
    { "SEED  %u", "시드  %u" },
    { "CARDS TAKEN", "획득한 카드" },

    // -- card effects ------------------------------------------------------
    // One entry per distinct line in the card table. The vocabulary is fixed
    // on purpose: the same effect always reads with the same words, so a card
    // can be understood by pattern-matching against cards already seen.
    { "recovery x2",    "체력 회복량 x2" },
    { "damage taken x0.8",  "받는 피해량 x0.8" },
    { "burst charge x0.5",  "버스트 충전량 x0.5" },
    { "burst charge x2",    "버스트 충전량 x2" },
    { "burst score x0.5", "버스트 점수 x0.5" },
    { "burst score x2.5", "버스트 점수 x2.5" },
    { "burst score x3",   "버스트 점수 x3" },
    { "burst score x3.5", "버스트 점수 x3.5" },
    { "burst lasts 2 beats longer", "버스트 지속 시간 +2박" },
    { "burst lasts 2 beats less",   "버스트 지속 시간 -2박" },
    { "SWAP chance x3",     "SWAP 확률 x3" },
    { "BLIND chance x3",    "BLIND 확률 x3" },
    { "TRAPS chance x3",    "TRAPS 확률 x3" },
    { "tempo +20%", "템포 +20%" },
    { "+15% gauge on level up", "레벨 업 체력 회복량 +15%" },
    { "+10% score at the end of the run", "게임 종료 시 점수 +10%" },
    { "score x0.8",  "점수 배율 x0.8" },
    { "score x1.3",  "점수 배율 x1.3" },
    { "score x1.5",  "점수 배율 x1.5" },
    { "score x1.6",  "점수 배율 x1.6" },
    { "score x2",    "점수 배율 x2" },


    { "recovery x0.5",  "체력 회복량 x0.5" },
    { "no recovery",    "체력 회복 불가" },
    { "GOOD never heals", "GOOD 체력 회복 불가" },


    { "burst charge x1.4",  "버스트 충전량 x1.4" },
    { "burst score x4", "버스트 점수 x4" },

    { "no SWAP",  "SWAP 삭제" },
    { "no BLIND", "BLIND 삭제" },
    { "no TRAPS", "TRAPS 삭제" },
    { "SWAP every bar",  "SWAP 확률 100%" },
    { "BLIND every bar", "BLIND 확률 100%" },
    { "TRAPS every bar", "TRAPS 확률 100%" },
    { "a gimmick every bar", "기믹 확률 100%" },



    { "the call is half as loud",  "콜 음량 절반" },
    { "warnings are half as loud", "경고 음량 절반" },
    { "monsters are seen later",   "노드 시야 감소" },
    { "traps wear lane colours",   "TRAPS가 레인 색으로 위장" },
    { "the whole ring turns",      "링 전체가 회전" },
    { "half the maximum gauge",    "최대 체력 절반" },

    { "heal to full now",          "즉시 체력 100% 회복" },
    { "fill the burst meter now",  "즉시 버스트 100% 충전" },
    { "the burst can never be fired", "버스트 발동 불가" },

    // -- card names ---------------------------------------------------
    //
    // The English name is the lookup key, exactly as it appears in cards[].
    // Several of these were renamed FROM the Korean rather than the other way
    // round: where the Korean the game wanted had drifted far from the English
    // placeholder, the English was rewritten to match it, not the reverse.
    // tools/checknames.sh compares the two lists.
    { "OVERFLOW", "오버플로우" },
    { "OVERDRIVE", "오버드라이브" },
    { "OVERLOAD", "오버로드" },
    { "A FORTUNE MADE", "부자의 탄생" },
    { "A FORTUNE LOST", "부자의 몰락" },
    { "MUSICAL CHAIRS", "의자 뺏기" },
    { "TURTLE WALTZ", "거북이와 춤을" },
    { "EASY MODE", "이지 모드" },
    { "AWAKENING", "각성" },
    { "DIVINE ARCHER", "신궁" },
    { "CRACK SHOT", "명사수" },
    { "DEAD EYE", "백발백중" },
    { "SHARPSHOOTER", "샤프슈터" },
    { "SLOW FIRE", "완사" },
    { "RAPID FIRE", "속사" },
    { "GAMBIT", "갬빗" },
    { "SKEWER", "스큐어" },
    { "STRATAGEM", "책략" },
    { "DECEPTION", "속임수" },
    { "COUNTER", "파훼하다" },
    { "PASSWORD", "비밀번호" },
    { "ENCRYPTION", "암호화" },
    { "BREAKTHROUGH", "돌파하다" },
    { "BAIT", "미끼" },
    { "SNARE", "덫" },
    { "DISMANTLE", "해체하다" },
    { "YOUR NAME", "너의 이름은" },
    { "FUTURE DIARY", "미래 일기" },
    { "DUNGEON KEEPER", "던전 관리인" },
    { "U-TURN", "유턴" },
    { "MINDS EYE", "마음의 눈" },
    { "POISON TACK", "독압정" },
    { "CALAMITY", "재앙" },
    { "GROWING PAINS", "성장통" },
    { "GROWTH", "성장세" },
    { "THRUST", "추진력" },
    { "ACCELERATE", "가속도" },
    { "CAMOUFLAGE", "전술 위장" },
    { "NIGHT FOG", "안개 낀 밤" },
    { "SILENCE", "침묵하라" },
    { "HUSH", "쉿" },
    { "MUFFLE", "감쇠" },
    { "COMPLACENT", "안전불감증" },
    { "WHIRL", "빙글뱅글" },
    { "FRAGILE", "파손 주의" },
    { "UNDERRATED", "과소평가" },
    { "BATTERED", "상처투성이" },
    { "SHARPER", "좀 더 정밀하게" },
    { "VOLCANO", "활화산" },
    { "A FORTUNE RISKED", "부자의 위기" },
    { "PLATE", "판금 갑옷" },
    { "SEAWALL", "방파제" },
    { "REPAIR", "장비 수리" },
    { "TRUER", "좀 더 정확하게" },
    { "KINDLING", "불쏘시개" },
    { "A FORTUNE GROWN", "부자의 성장" },
    { "FINER", "좀 더 정교하게" },
    { "BANDAGE", "반창고" },
    { "ENERGY DRINK", "에너지 드링크" },
    { "ELIXIR", "엘릭서" },
    { "PENNIES", "티끌 모아 태산" },
    { "REPORT", "보고서" },
    { "TRANSFUSION", "수혈" },
    { "DISCHARGE", "방전" },
    { "MAINTENANCE", "정기 점검" },
    { "HUBRIS", "자기과신" },
    { "SERVICING", "연장 점검" },
    { "BARRICADE", "다층 배리어" },
    { "RAMPART", "차원 방벽" },
    { "SURGE", "과충전" },
    { "FIGHTING GAME", "격투 게임" },
    { "REFIT", "보수 공사" },
    { "TRIAGE", "응급 처치" },
    { "barrier +2", "보호막 +2" },
    { "spend 30% health", "체력 30% 소모" },
    { "spend 50% burst", "버스트 50% 소모" },
    { "+1 barrier when the burst fires (max 1)", "버스트 사용 시 보호막 +1 (최대 1개)" },
    { "+1 barrier every 50 combo (max 2)", "50콤보마다 보호막 +1 (최대 2개)" },
    { "+1 barrier 2 levels after one breaks (max 1)", "보호막 파괴 2레벨 후 보호막 +1 (최대 1개)" },
    { "+4 barriers once, at 20% health", "체력 20% 이하 시 보호막 +4 (1회 한정)" },
    { "damage taken x0.9", "받는 피해량 x0.9" },
    { "barrier +1", "보호막 +1" },
    { "barrier +4", "보호막 +4" },
    { "+1 barrier on level up (max 2)", "레벨 업 보호막 +1 (최대 2개)" },
    { "spend 10% health", "체력 10% 소모" },
    { "spend 50% health", "체력 50% 소모" },
    { "spend 30% burst", "버스트 30% 소모" },
    { "spend 100% burst", "버스트 100% 소모" },
    { "items appear at random", "아이템 노드가 랜덤하게 등장" },
    { "damage taken x2", "받는 피해량 x2" },
    { "+10% burst on level up", "레벨 업 버스트 충전량 +10%" },
    { "+10% gauge on level up", "레벨 업 체력 회복량 +10%" },
    { "+20% burst on level up", "레벨 업 버스트 충전량 +20%" },
    { "+50% coins earned at the end of the run", "게임 종료 후 획득 재화 +50%" },
    { "BLIND chance x0.5", "BLIND 확률 x0.5" },
    { "BLIND chance x2", "BLIND 확률 x2" },
    { "SWAP chance x0.5", "SWAP 확률 x0.5" },
    { "SWAP chance x2", "SWAP 확률 x2" },
    { "TRAPS chance x0.5", "TRAPS 확률 x0.5" },
    { "TRAPS chance x4", "TRAPS 확률 x4" },
    { "score x3", "점수 배율 x3" },
    { "recovery x0.8",  "체력 회복량 x0.8" },
    { "recovery x0.85", "체력 회복량 x0.85" },
    { "recovery x0.9",  "체력 회복량 x0.9" },
    { "recovery x1.1",  "체력 회복량 x1.1" },
    { "recovery x1.2",  "체력 회복량 x1.2" },
    { "recovery x1.4",  "체력 회복량 x1.4" },
    { "PERFECT recovery x0.8", "PERFECT 체력 회복량 x0.8" },
    { "PERFECT recovery x1.3", "PERFECT 체력 회복량 x1.3" },
    { "PERFECT recovery x1.6", "PERFECT 체력 회복량 x1.6" },
    { "PERFECT recovery x1.9", "PERFECT 체력 회복량 x1.9" },
    { "PERFECT recovery x2",   "PERFECT 체력 회복량 x2" },
    { "recovery x1.5", "체력 회복량 x1.5" },
    { "recovery x3",   "체력 회복량 x3" },
    { "burst charge x0.8", "버스트 충전량 x0.8" },
    { "burst charge x0.9", "버스트 충전량 x0.9" },
    { "burst charge x1.2", "버스트 충전량 x1.2" },
    { "burst charge x1.3", "버스트 충전량 x1.3" },
    { "burst charge x1.5", "버스트 충전량 x1.5" },
    { "burst score x2", "버스트 점수 x2" },
    { "damage taken x0.75", "받는 피해량 x0.75" },
    { "damage taken x1.1",  "받는 피해량 x1.1" },
    { "damage taken x1.15", "받는 피해량 x1.15" },
    { "damage taken x1.2",  "받는 피해량 x1.2" },
    { "damage taken x1.5",  "받는 피해량 x1.5" },
    { "score x0.9", "점수 배율 x0.9" },
    { "score x1.2", "점수 배율 x1.2" },
    { "score x1.4", "점수 배율 x1.4" },
    { "tempo +5%",   "템포 +5%" },
    { "tempo +10%",  "템포 +10%" },
    { "tempo -2.5%", "템포 -2.5%" },
    { "tempo -5%",   "템포 -5%" },
    { "window +0.01", "판정 +0.01" },
    { "window +0.02", "판정 +0.02" },
    { "BAD is a note you did hit, too early or too late to count.",
      "BAD는 노트를 치긴 했지만 타이밍이 너무 빠르거나 늦은 판정입니다." },
    { "A BAD and a MISS both cost health.",
      "BAD와 MISS는 체력을 깎습니다." },
    { "Cards change the run itself - what heals you, how you score,",
      "카드는 게임 자체를 바꿉니다. 무엇으로 회복하는지, 어떻게 점수를 내는지," },
    { "how fast the burst charges, even what the bar throws at you.",
      "버스트가 얼마나 빨리 차는지, 어떤 마디가 날아오는지까지." },
    { "Coins earned by clearing levels are spent in the SHOP.",
      "레벨을 클리어하여 얻은 재화는 상점에서 사용할 수 있습니다." },
    { "One of each type at a time, and only for the next run.",
      "카드·체력·버스트 각 1개씩, 다음 게임에만 적용됩니다." },
    { "The GACHA rolls once for a random prize - a card, an item,",
      "뽑기는 한 번에 하나씩 무작위로 나옵니다. 카드, 아이템, 재화," },
    { "coins, or a new skin.", "또는 새 스킨을 얻을 수 있습니다." },
    { "D-PAD / #FC\tthe four points on the ring", "D-PAD / #FC\t링 위의 네 방향" },
    { "\ttwo clusters lets you play with both hands",
      "\t두 곳을 나눠 쓰면 양손으로 플레이할 수 있습니다." },
    { "#SB\tfire a BURST once its meter is full",
      "#SB\t게이지가 차면 BURST를 발동합니다." },
    { "ESC\tpause", "ESC\t일시정지" },
    { "#ST\tpause", "#ST\t일시정지" },
    { "DOWN / #FD\ttake no card", "DOWN / #FD\t카드를 뽑지 않고 넘어갑니다." },
    { "#SR\tdeal three new cards", "#SR\t카드 세 장을 다시 뽑습니다." },
    { "PAD BUTTONS", "패드 버튼" },
    { "SHOULDER SWAP", "키 전환" },
    { "#SB burst   #SR reroll", "#SB 버스트   #SR 다시 뽑기" },
    { "SEED", "시드" },
    { "ENTER SEED (%s)", "시드 입력 (%s)" },
    { "RANDOM", "무작위" },
    { "DEL", "삭제" },
    { "CLEAR ALL", "초기화" },
    { "OK", "확인" },
    { "SPACE to enter      ESC to cancel",
      "SPACE 입력      ESC 취소" },
    { "(#FD) to enter      (#FR) to cancel", "(#FD) 입력      (#FR) 취소" },
};
#define PHRASE_COUNT ((int)(sizeof(phrases) / sizeof(phrases[0])))

#define LANG_EN 0
#define LANG_KO 1
static int lang = LANG_KO;

// ---- pad glyphs ----------------------------------------------------------
// A prompt cannot name a pad button with a letter, because the letter depends
// on the pad. raylib names buttons by POSITION, so the game already knows
// which button it means - what it does not know is what is printed on it:
// the button below the thumb is A on an Xbox pad, B on a Nintendo one, and a
// cross on a PlayStation one.
//
// So prompt strings carry tokens instead of letters, and the letters are
// filled in at draw time. This has to happen after the translation lookup,
// not before: these same strings are the phrase-table keys, and a Korean line
// carrying "A" would freeze one layout into the table forever.
//
//   #FU #FR #FD #FL   face buttons, by position
//   #FC               the whole cluster, for naming it in one word
//   #ST               the start button
//   #SB #SR           the shoulders: burst and reroll, which the player
//                     can trade - so these are named by JOB, not by side
enum { PADFACE_XBOX, PADFACE_NINTENDO, PADFACE_SONY, PADFACE_COUNT };
static int padFace;        // which glyphs the face buttons wear
static int padSwap;        // shoulders traded: burst on LB, reroll on RB

// Nintendo is Xbox mirrored on both axes - the letters are the same four and
// they sit in the other two places, which is exactly the mistake this option
// exists to stop.
static const char* const faceGlyph[PADFACE_COUNT][4] = {
    /* up   right down  left */
    {  "Y",  "B",  "A",  "X"  },
    {  "X",  "A",  "B",  "Y"  },
    {  "△", "○", "×", "□" },
};
static const char* const faceCluster[PADFACE_COUNT] = { "ABXY", "ABXY", "△○×□" };
static const char* const startName[PADFACE_COUNT]   = { "START", "+", "OPTIONS" };
static const char* const shoulderName[PADFACE_COUNT][2] = {
    /* left  right */
    { "LB", "RB" }, { "L", "R" }, { "L1", "R1" },
};

// A small ring, not one buffer: a caller may measure a string and then draw
// it, and both go through here. Nothing holds one of these across a frame.
#define GLYPH_BUFS 8
#define GLYPH_LEN  200
static char glyphBuf[GLYPH_BUFS][GLYPH_LEN];
static int  glyphNext;

static const char* ExpandPad(const char* s) {
    if (!s || !strchr(s, '#')) return s;      // no token, no copy
    char* out = glyphBuf[glyphNext];
    glyphNext = (glyphNext + 1) % GLYPH_BUFS;

    int n = 0;
    for (const char* p = s; *p && n < GLYPH_LEN - 1; ) {
        const char* g = NULL;
        if (p[0] == '#') {
            if      (!strncmp(p, "#FU", 3)) g = faceGlyph[padFace][0];
            else if (!strncmp(p, "#FR", 3)) g = faceGlyph[padFace][1];
            else if (!strncmp(p, "#FD", 3)) g = faceGlyph[padFace][2];
            else if (!strncmp(p, "#FL", 3)) g = faceGlyph[padFace][3];
            else if (!strncmp(p, "#FC", 3)) g = faceCluster[padFace];
            else if (!strncmp(p, "#ST", 3)) g = startName[padFace];
            else if (!strncmp(p, "#SB", 3)) g = shoulderName[padFace][padSwap ? 0 : 1];
            else if (!strncmp(p, "#SR", 3)) g = shoulderName[padFace][padSwap ? 1 : 0];
        }
        if (!g) { out[n++] = *p++; continue; }
        while (*g && n < GLYPH_LEN - 1) out[n++] = *g++;
        p += 3;
    }
    out[n] = '\0';
    return out;
}

// The lookup happens first and the pad glyphs second, always in that order.
// The table is keyed on the English source, tokens and all, so a phrase found
// here still has "#FD" in it - and both languages get the same substitution
// from one place instead of two tables that have to agree.
static const char* T(const char* en) {
    if (!en) return en;
    if (lang != LANG_EN) {
        for (int i = 0; i < PHRASE_COUNT; i++) {
            // Pointer equality first: the compiler pools identical literals, so
            // most lookups settle without touching memory twice.
            if (phrases[i].en == en || strcmp(phrases[i].en, en) == 0)
                return ExpandPad(phrases[i].ko);
        }
    }
    return ExpandPad(en);
}

// ---- text drawing --------------------------------------------------------
// Everything the player reads goes through these two. They translate, so no
// call site has to remember to; a string composed by TextFormat simply misses
// the lookup and passes through unchanged, which is the correct result.
//
// The fallback to raylib's built-in font is not decoration. It keeps the game
// running and legible in English if the baked font ever fails to load, instead
// of drawing 3000 empty boxes.
static Font uiFontS;      // baked at 16px, for body text
static Font uiFontL;      // baked at 32px, for headings
static int fontReady;

// Whichever atlas needs the smaller scale change. 22 is the geometric midpoint
// of 16 and 32, so a 20px draw scales 1.25x off the small one rather than
// 0.63x off the large one - and upscaling a pixel font is far kinder than
// downscaling it.
static Font UiFontFor(int size) { return (size <= 22) ? uiFontS : uiFontL; }

// raylib's own DrawText uses size/10 as its letter spacing. Matching it means
// the fallback and the real font lay out at close to the same width, so the
// screens that were positioned by eye against the built-in font still hold.
static float UiSpacing(int size) { return (float)size / 10.0f; }

static void UiText(const char* txt, int x, int y, int size, Color c) {
    const char* s = T(txt);
    if (!fontReady) { DrawText(s, x, y, size, c); return; }
    Vector2 p = { (float)x, (float)y };
    DrawTextEx(UiFontFor(size), s, p, (float)size, UiSpacing(size), c);
}

static int UiMeasure(const char* txt, int size) {
    const char* s = T(txt);
    if (!fontReady) return MeasureText(s, size);
    return (int)MeasureTextEx(UiFontFor(size), s, (float)size, UiSpacing(size)).x;
}

// World units per beat. Raising it spreads the notes out in depth but shows
// FEWER beats, because the draw cull is a fixed distance - see NOTES.
#define SPACING  6.0f
// DrawCircle3D has no line width - it is always one pixel - so a note ring is
// made thick by drawing it twice at shrinking radii. Two is what it took: one
// was legible but no heavier than a beat ring, and three read as garish once
// the whole screen filled with them.
//
// The gap is in world space, so it shrinks with distance like everything else
// and the pair merges back into one pixel past about two beats. That turned
// out to be fine, because the other half of this change does the distance
// work: the beat rings stepped back from 0.3 to 0.12, and that contrast holds
// at every depth. Thickness near, contrast far.
#define RING_LAYERS 2

// Test-only: pretend the run is this many levels further along. Everything the
// level drives - tempo, judgement window, pattern pool, gimmick rates, music
// layers - jumps with it, while the level counter, the score and the damage
// scaling stay honest, so nobody dies of density before seeing the thing they
// opened the build to look at. /DTEST_LEVEL_BIAS=34 puts level 35 in the first
// bar. Zero here, so a shipped build carries no trace of it.
// Test-only: start every run with these barriers and this item rate, so the new
// cards can be looked at without drafting into them.
#ifndef TEST_BARRIERS
#define TEST_BARRIERS 0
#endif
// Test-only: every bar a disguised trap, for checking the item wire against the
// one colour it sits closest to.
// Test-only: a full skin rack and a full wallet, so the duplicate-skin result
// can be looked at without grinding the gacha for it. The odds put a skin at
// 4.9% and a DUPLICATE skin only once the rack is nearly full.
#ifndef TEST_ALLSKINS
#define TEST_ALLSKINS 0
#endif
#ifndef TEST_WALLET
#define TEST_WALLET 0
#endif

#ifndef TEST_DISGUISE
#define TEST_DISGUISE 0
#endif

#ifndef TEST_ITEM_PCT
#define TEST_ITEM_PCT 0
#endif

#ifndef TEST_LEVEL_BIAS
#define TEST_LEVEL_BIAS 0
#endif
#define SR 44100
// Health is a gauge, not a life count. Three lives made a single bad bar fatal;
// a gauge lets good play claw back ground, which turns a hard level into a
// fight instead of a wall.
//
// The actual numbers live in BaseMiss() and friends below, because they differ
// per mode. Do not reintroduce them as constants here - two sources of truth
// for the same value is exactly how a mode silently stops applying.
#define HP_MAX      1.0f

// Hit shake has to stay small. It is re-set on every hit, so at eighth notes
// it never gets a chance to decay and the screen just vibrates continuously.
// Miss and impact are rare, so they can punch.
#define SHAKE_PERFECT 0.16f
#define SHAKE_GOOD    0.09f
#define SHAKE_MISS    0.70f
#define SHAKE_IMPACT  1.10f

// Perfect is the inner slice of the hit window, so it tightens with the level
// exactly as the window does.
#define PERFECT_FRAC 0.4f


#define TUNNEL_R   4.0f
// Distance from camera to the judgement gate. Free to tune: rings are placed
// relative to gateZ, so a ring meets the gate on the beat at any value.
// Smaller = gate closer and bigger on screen, less room to read what is coming.
#define HIT_OFFSET 8.0f

// The ship sits at the 7:30 position, in the gap between two lane nodes, so it
// never covers one. +x is screen-left, so this is bottom-left.
//
// PLAYER_Z is squeezed between two angles that move in opposite directions.
// The gate ring has an on-screen angular radius of
// atan(TUNNEL_R / HIT_OFFSET) = 26.6 degrees, and the muzzle only reads as
// "outside the ring" when it clears that by a visible margin. But fovy is
// vertical, so the hull's y term alone falls off the screen past 35 degrees.
// Measured: at 4.5 the muzzle sat at 29 and looked stuck to the ring; at 2.8
// only the muzzle was left on screen; 3.5 puts the muzzle at 33.9 and the hull
// at 32.5, which satisfies both.
#define PLAYER_Z   3.5f
#define PLAYER_X   2.23f
#define PLAYER_Y  (-2.23f)
#define PLAYER_BANK 32.0f     // roll, so the wings are seen from above, not edge on
#define PLAYER_NOSE 0.9f      // muzzle distance ahead of the hull
#define PLAYER_AIM 8.0f       // how far past the gate the nose points

#define BEATS_PER_BAR 4
#define SLOTS_PER_BAR 16                 // sixteenth-note resolution
#define SLOT_BEATS ((float)BEATS_PER_BAR / (float)SLOTS_PER_BAR)

// How far past the hit window a press still counts as that note, played wrong.
//
// Twice the hit window. That reaches further than one sixteenth slot, which is
// worth stating plainly rather than discovering later. Measured, in slots:
//
//     level        1      25      50     100
//     hit       1.00    0.79    0.57    0.52
//     bad       2.00    1.58    1.14    1.04
//
// It reaches a neighbour because the HIT window alone already spans a full slot
// at level 1 - the looseness is in `l.window`, not here.
//
// It is still the right shape, for two reasons. The search takes the NEAREST
// note, so in a dense passage there is always a closer note and the long reach
// never fires; it only applies to an isolated note, where being half a beat
// early is an unambiguous mistake and deserves the BAD. And the width of the
// band is what stops mashing: a fixed half-slot band was tried and mashing got
// GOOD on 6 notes in 14 seconds where this gets it none, because a press
// stream steps straight over a band narrower than its own interval.
#define BAD_WINDOW_MUL 2.0f
#define BARS_PER_CHUNK 4                 // how many bars are generated at a time
#define CALL_RESPONSE 1                  // 0 = no call bars, every bar is playable

#if CALL_RESPONSE
#define BAR_STRIDE (BEATS_PER_BAR * 2)   // each response bar is preceded by its call
#else
#define BAR_STRIDE BEATS_PER_BAR
#endif
#define CHUNK_BEATS (BARS_PER_CHUNK * BAR_STRIDE)

// A silent bar at the head of every chunk. The level-up jingle and banner land
// here, so they never fight with the first call of the new level.
#define LEVEL_REST_BEATS BEATS_PER_BAR
#define CHUNK_TOTAL (LEVEL_REST_BEATS + CHUNK_BEATS)

#define LEAD_BEATS 8.0f                  // generate this far ahead of the playhead
// Zero, because every chunk already opens with LEVEL_REST_BEATS of silence and
// the first chunk is no exception. Putting a lead-in here as well made the
// player sit through both before the first call.
#define FIRST_CHUNK_BEAT 0.0f

#define LEVEL_CHUNKS 1                   // chunks per level (1 keeps rest == level head)
// Two runs of very different length out of one set of systems. Normal is the
// ten level tasting menu; Hard is the hundred level one, where the card draft
// finally has room to build something.
typedef enum { MODE_NORMAL, MODE_HARD } GameMode;
static GameMode mode = MODE_NORMAL;

#define NORMAL_LEVELS 10
// Was 100. At that length the first fifth was a warm-up nobody wanted to sit
// through twice; 50 keeps the same ceiling with everything arriving sooner.
// Endless. Not literally infinite - a bound keeps TotalBeats() and the level
// clamp honest - but 9999 levels is about 100 hours at this tempo, so nothing
// reaches it. The mode ends when the player does.
#define HARD_LEVELS 9999

// Hard unlock schedule, zero-based level indices.
#define HARD_SIX1_LEVEL 11   // level 12: sixteenths, one role per run
#define HARD_SIX2_LEVEL 19   // level 20: sixteenths, two roles
#define HARD_CHORD_LEVEL 34  // level 35: chords inside the sixteenths

static int LevelCount(void) { return (mode == MODE_HARD) ? HARD_LEVELS : NORMAL_LEVELS; }
static float TotalBeats(void) {
    return FIRST_CHUNK_BEAT + (float)(LevelCount() * LEVEL_CHUNKS * CHUNK_TOTAL);
}

// Hard also undoes the softening Normal got. Everything that was relaxed to
// make a ten level run approachable is pulled back in.
// Normal is where it should be and is left alone. Hard takes more and gives
// back less: recovery there was outpacing the damage, so a good player simply
// could not be worn down.
// The level the run is on. Declared here rather than with the rest of the run
// state because BaseMiss() below scales with it.
static int level;

// Monsters hit harder as the run goes on.
//
// Endless needed a pressure that keeps rising after BPM and the judgement
// window stop moving, and this is the one the player feels without the chart
// changing shape. Deliberately gentle: a run that is going well should still
// be going well ten levels later, just with less room in it.
//
// +0.4% of the base per level, to 1.20x at level 51 and no further. In Endless
// that turns 0.14 into 0.168, so death moves from about 7 missed notes to 6.
//
// Kept this shallow because cards multiply it: FEVER and SHATTER and their kind
// raise dmgMul by 1.4x to 2.2x, and a level curve on top of a deck like that
// compounds into something the player did not agree to.
//
// Normal tops out at level 10, where it is only 1.04x - the curve is there for
// the mode that has somewhere to climb to.
#define MISS_LEVEL_STEP 0.004f
#define MISS_LEVEL_MAX  1.20f

static float MissScale(void) {
    float k = 1.0f + (float)level * MISS_LEVEL_STEP;
    return (k > MISS_LEVEL_MAX) ? MISS_LEVEL_MAX : k;
}

static float BaseMiss(void) { return ((mode == MODE_HARD) ? 0.14f : 0.07f) * MissScale(); }
// Half a MISS. BAD consumes the note, so it replaces the miss rather than
// stacking with it - the player pays once for the note either way, and less
// for having at least answered it.
static float BaseBad(void)  { return (mode == MODE_HARD) ? 0.07f : 0.035f; }
static float BaseHealPerfect(void) { return (mode == MODE_HARD) ? 0.010f : 0.020f; }
static float BaseHealGood(void) { return (mode == MODE_HARD) ? 0.002f : 0.010f; }

// A tier-4 bar can carry a dozen notes, and up to three chunks are in flight at
// once, so these are far larger than the two-lane version needed. Measured
// worst case is reported in NOTES.md - re-measure if LEAD_BEATS or the pattern
// table changes.
#define MAX_ENEMIES 256
#define MAX_CALLS 256
#define MAX_BEAMS 12
#define MAX_PARTICLES 128

// ---- run modifiers -------------------------------------------------------
// Declared up here because the chart generator reads it. Every card the player
// drafts is a change to one of these numbers; see the card table further down.
typedef struct {
    float scoreMul;
    float dmgMul;         // damage taken
    float healMul;        // gauge recovered per hit
    float healPerfectMul; // extra multiplier on the Perfect half only
    int   healGoodZero;   // Good stops healing entirely
    float windowAdd;      // extra judgement window, in beats
    float bpmMul;
    float callVol;
    float warnVol;
    float burstRate;      // how fast Perfects charge the burst
    float burstBeats;     // how long a burst lasts
    float burstScore;     // score multiplier while bursting
    float burstStart;     // burst granted at the head of each level
    float levelHeal;      // gauge granted at the head of each level
    float hpMaxMul;       // ceiling on the gauge
    float fogBeats;       // 0 = off, else monsters only visible this close
    float gimMul[3];      // swap / blind / trap frequency; 0 removes it
    int   forceGim;       // 0 none, 1 swap, 2 blind, 3 trap, 4 any
    int   trapDisguise;   // traps wear lane colours
    int   spin;           // the whole ring rotates
    int   itemPct;        // percent of notes that carry an item; 0 = none
    int   levelBarrier;   // barriers granted at each level up

    // Every barrier source tops the player UP TO a number rather than adding to
    // it, so holding two of anything never stacks and no card needs to explain a
    // cap beyond the one printed on its own face.
    int   barrierOnBurst; // SURGE: firing the burst tops up to 1
    int   barrierCombo;   // FIGHTING GAME: every N combo tops up to 1; 0 = off
    int   barrierRefit;   // REFIT: N levels after one breaks, top up to 1; 0 = off
    int   barrierTriage;  // TRIAGE: once, at 20% health or below, +4

    // The meter still fills; it just cannot be fired. That leaves the whole
    // gauge as reroll currency, which is a far more interesting trade than
    // switching the meter off entirely - switching it off would have taken the
    // rerolls away too, and a card that removes two things at once for one
    // price is just a worse card.
    int   burstLocked;

    // Applied once, at the end of the run, rather than per hit. Two cards pay
    // out only if the run is actually finished, which makes them a bet on
    // surviving rather than a multiplier on every note.
    float rewardMul;      // shop coins earned by this run
    float endScoreMul;    // final score, after every per-hit multiplier
} Mods;

static Mods mods;

// ---- motion comfort ------------------------------------------------------
// The tunnel is a textbook vection generator: the whole field of view streams
// past at a constant rate with no fixed reference, and the brain reads that as
// the body moving. Three things feed it - the FOV pulse, the camera shake, and
// how hard the tunnel bends - and this switches all three off at once.
//
// The FOV pulse was the quiet offender: a 4 degree zoom twice a second, which
// the inner ear never corroborates. It is 1.5 degrees now even with comfort
// off, and zero with it on.
//
// Declared this early because TunnelOffset() reads it.
static int reducedMotion;
#define FOV_PULSE 1.5f

// Degrees the ring has turned. Zero unless SPIN is in play. Lane positions and
// the twist interpolation both read it, so markers and monsters turn together
// - the lane a note belongs to never changes, only where it appears.
static float spinAngle;
#define SPIN_DEG_PER_SEC 17.0f

// ---- lanes ---------------------------------------------------------------
// Four points on the ring, one per arrow key. Putting notes around a circle
// instead of on a left/right line is what makes the tunnel read as 3D.
#define LANE_COUNT 4

// The camera looks down +z with up = +y, so screen-right is -x. That sign
// flip lives here and nowhere else.
// Lane n sits at 90*(n+1) degrees, plus whatever SPIN has turned the ring by.
// Writing it as an angle rather than four literals is what lets the ring
// rotate at all - and the twist interpolation relies on the same formula.
static Vector2 LanePos(int lane) {
    float a = (90.0f * (float)(lane + 1) + spinAngle) * DEG2RAD;
    return (Vector2) { cosf(a) * TUNNEL_R, sinf(a) * TUNNEL_R };
}

static int LaneKey(int lane) {
    switch (lane) {
    case 0:  return KEY_UP;
    case 1:  return KEY_RIGHT;
    case 2:  return KEY_DOWN;
    default: return KEY_LEFT;
    }
}

// WASD maps to the same four lanes. Four arrow keys is a one-hand grip, and a
// bar that walks around the ring with a chord in it is not physically playable
// that way. With both bindings the player can split the ring across two hands.
static int LaneKeyAlt(int lane) {
    switch (lane) {
    case 0:  return KEY_W;
    case 1:  return KEY_D;
    case 2:  return KEY_S;
    default: return KEY_A;
    }
}


// ---- gamepad -------------------------------------------------------------
// raylib names pad buttons by POSITION, not by letter: RIGHT_FACE_UP is Xbox Y
// and PlayStation Triangle, and the game never has to know which. That matters
// here more than usual, because this game's four lanes ARE positions - up,
// right, down, left - and the face buttons are physically laid out in exactly
// that diamond. The mapping is not a convention anyone has to learn.
//
// Both clusters are live at once, which mirrors what the keyboard already does
// with arrows and WASD: a bar that walks around the ring with a chord in it is
// not playable one-handed, so the player needs to be able to split it.
#define PAD 0

static int LanePadDpad(int lane) {
    switch (lane) {
    case 0:  return GAMEPAD_BUTTON_LEFT_FACE_UP;
    case 1:  return GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
    case 2:  return GAMEPAD_BUTTON_LEFT_FACE_DOWN;
    default: return GAMEPAD_BUTTON_LEFT_FACE_LEFT;
    }
}

static int LanePadFace(int lane) {
    switch (lane) {
    case 0:  return GAMEPAD_BUTTON_RIGHT_FACE_UP;
    case 1:  return GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
    case 2:  return GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
    default: return GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
    }
}

// Every way to answer one lane, in one place. The debounce that sits on top of
// this in the play loop is what keeps a two-handed roll from being read as two
// separate notes, and it works the same whichever pair of hands is used.
static int LaneHit(int lane) {
    return IsKeyPressed(LaneKey(lane))
        || IsKeyPressed(LaneKeyAlt(lane))
        || IsGamepadButtonPressed(PAD, LanePadDpad(lane))
        || IsGamepadButtonPressed(PAD, LanePadFace(lane));
}

// Burst is one shoulder and reroll is the other, because the face buttons are
// all lanes and both actions have to be reachable without letting go of one.
// A single button per action also gives the prompts something to name; "LB /
// RB" told the player two things where one was true.
//
// Which shoulder does which is the player's call - some people want the burst
// under the finger they already rest there. Everything asks by JOB, so the
// setting is read in exactly two places and the prompts follow it for free.
static int PadBurst(void) {
    return IsGamepadButtonPressed(PAD, padSwap ? GAMEPAD_BUTTON_LEFT_TRIGGER_1
                                              : GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
}
static int PadReroll(void) {
    return IsGamepadButtonPressed(PAD, padSwap ? GAMEPAD_BUTTON_RIGHT_TRIGGER_1
                                              : GAMEPAD_BUTTON_LEFT_TRIGGER_1);
}
static int BurstHit(void) {
    return IsKeyPressed(KEY_SPACE) || PadBurst();
}

// ---- pad: menus ----------------------------------------------------------
// The same physical buttons carry two meanings, and they never collide because
// the lane read only happens while PLAYING and these only happen everywhere
// else. The D-pad is directions in both worlds, which is the point - the draft
// already asks for LEFT/UP/RIGHT to pick a card and DOWN to skip, so a pad
// lands on it without a cursor being invented for the occasion.
//
// Confirm is the lower face button and cancel is the right one on purpose:
// that pairing is the same on Xbox and PlayStation once raylib has named the
// buttons by position, so nothing here needs to know which pad is plugged in.
static int PadConfirm(void) { return IsGamepadButtonPressed(PAD, GAMEPAD_BUTTON_RIGHT_FACE_DOWN); }
static int PadCancel(void)  { return IsGamepadButtonPressed(PAD, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT); }
static int PadAux(void)     { return IsGamepadButtonPressed(PAD, GAMEPAD_BUTTON_RIGHT_FACE_UP); }
static int PadCards(void)   { return IsGamepadButtonPressed(PAD, GAMEPAD_BUTTON_RIGHT_FACE_LEFT); }
static int PadStart(void)   { return IsGamepadButtonPressed(PAD, GAMEPAD_BUTTON_MIDDLE_RIGHT); }

static int PadUp(void)    { return IsGamepadButtonPressed(PAD, GAMEPAD_BUTTON_LEFT_FACE_UP); }
static int PadDown(void)  { return IsGamepadButtonPressed(PAD, GAMEPAD_BUTTON_LEFT_FACE_DOWN); }
static int PadLeft(void)  { return IsGamepadButtonPressed(PAD, GAMEPAD_BUTTON_LEFT_FACE_LEFT); }
static int PadRight(void) { return IsGamepadButtonPressed(PAD, GAMEPAD_BUTTON_LEFT_FACE_RIGHT); }

// ---- which device the prompts should name --------------------------------
// The screen has to say either SPACE or (A), and it has to be right without
// being asked. raylib has no "current input device" of its own, but it hands
// over everything needed to keep one:
//
//   IsGamepadAvailable   - connection, updated on hotplug
//   GetGamepadButtonPressed / GetKeyPressed - who spoke last
//
// The rule is the one every console-and-PC game uses: the last device to say
// something owns the prompts, and unplugging falls back to what is left.
//
// A pad appearing takes over immediately rather than waiting for a press.
// This is a PC game, so a keyboard is always attached and a pad is not - a pad
// being plugged in at all is a statement of intent, and on the first frame it
// is what makes the pad the default.
//
// Direction words are deliberately NOT swapped. A D-pad's up is UP, so only the
// named keys - SPACE, ESC, R, P - have anything to translate.
typedef enum { INPUT_KEY, INPUT_PAD } InputKind;
static InputKind inputKind = INPUT_KEY;
static int padWasHere;

#define PROMPT(kb, pad) ((inputKind == INPUT_PAD) ? (pad) : (kb))

// The draft names the two keyboard bindings for each lane. On a pad the same
// two clusters are the D-pad and the face buttons, laid out the same way.
#define PROMPT_KEYS3 { PROMPT("LEFT / A", "LEFT / #FL"), PROMPT("UP / W", "UP / #FU"), PROMPT("RIGHT / D", "RIGHT / #FR") }

static void UpdateInputKind(void) {
    int here = IsGamepadAvailable(PAD);
    if (here && !padWasHere)      inputKind = INPUT_PAD;   // plugged in: it wins
    else if (!here)               inputKind = INPUT_KEY;   // unplugged: fall back
    else if (GetGamepadButtonPressed() != GAMEPAD_BUTTON_UNKNOWN)
                                  inputKind = INPUT_PAD;
    // GetKeyPressed drains the keycode queue, which nothing else here reads -
    // the seed box uses GetCharPressed, a separate queue, and every other key
    // is polled with IsKeyPressed. So emptying this one costs nothing.
    else if (GetKeyPressed() != 0) inputKind = INPUT_KEY;
    padWasHere = here;
}
static Color LaneColor(int lane) {
    switch (lane) {
    case 0:  return (Color) { 255, 210, 60, 255 };
    case 1:  return (Color) { 255, 90, 90, 255 };
    case 2:  return (Color) { 90, 160, 255, 255 };
    default: return (Color) { 110, 230, 140, 255 };
    }
}

// ---- random --------------------------------------------------------------
// Three streams on purpose. The chart stream must stay reproducible from the
// displayed seed, so nothing else may draw from it.
//
// The card stream is separate for the same reason one step removed: a reroll
// is a player choice, and if offers came off the chart stream then rerolling
// would shift every pattern after it. Two players on one seed would get
// different charts depending on how often they rerolled, which is exactly what
// the shared-seed feature promises will not happen.
static unsigned int rngState = 1;
static unsigned int cardRng = 1;
static unsigned int vfxRng = 2463534242u;

static unsigned int Rand(void) {
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return rngState;
}

// Reproducible from the seed like the chart, but on its own timeline.
static unsigned int CardRand(void) {
    cardRng ^= cardRng << 13;
    cardRng ^= cardRng >> 17;
    cardRng ^= cardRng << 5;
    return cardRng;
}

static unsigned int VfxRand(void) {
    vfxRng ^= vfxRng << 13;
    vfxRng ^= vfxRng >> 17;
    vfxRng ^= vfxRng << 5;
    return vfxRng;
}

// -1 .. 1
static float VfxSigned(void) {
    return (float)(VfxRand() & 0xFFFFu) / 32767.5f - 1.0f;
}

// ---- tunnel shape --------------------------------------------------------
// The tunnel is a chain of straight-ish segments. A segment holds one heading
// for SEG_BARS bars, then eases into the next one over the first half of the
// following segment and holds again. Some segments head straight ahead, so the
// tunnel does come back to level instead of endlessly spiralling.
//
// Rings, gate, monsters, camera and ship all read TunnelOffset() at their own
// z, so the whole world bends together and nothing needs realigning.
#define SEG_BARS 4                       // bars (call+response pairs) per segment
#define SEG_BEATS ((float)(BAR_STRIDE * SEG_BARS))
#define SEG_Z (SEG_BEATS * SPACING)
#define SEG_TILT 0.20f                   // max sideways drift per unit of z
#define SEG_TILT_Y 0.55f                 // vertical tilt is scaled down (see below)
#define SEG_STRAIGHT_PCT 30              // chance a segment heads straight ahead
#define SEG_SLOTS 8                      // ring buffer, power of two

static Vector2 segDir[SEG_SLOTS];        // heading at the start of segment n
static Vector2 segCum[SEG_SLOTS];        // tunnel offset at the start of segment n
static int segReady = -1;                // highest segment index generated

// Integral of the blend weight from 0 to t. The weight eases from 0 to 1 over
// the first half of a segment (smoothstep) and stays at 1 for the rest, which
// is what gives "turn, then hold" instead of a constant wobble.
static float BlendIntegral(float t) {
    if (t <= 0.5f) {
        float u = 2.0f * t;
        return (u * u * u - u * u * u * u * 0.5f) * 0.5f;
    }
    return 0.25f + (t - 0.5f);
}

static void EnsureSegments(int upTo) {
    while (segReady < upTo) {
        int n = segReady + 1;
        int slot = n & (SEG_SLOTS - 1);
        Vector2 d, c;

        if (n == 0) {
            d = (Vector2){ 0.0f, 0.0f };          // every run starts dead ahead
            c = (Vector2){ 0.0f, 0.0f };
        }
        else {
            int pslot = (n - 1) & (SEG_SLOTS - 1);
            Vector2 pd = segDir[pslot];
            Vector2 pc = segCum[pslot];

            if ((int)(VfxRand() % 100u) < SEG_STRAIGHT_PCT) {
                d = (Vector2){ 0.0f, 0.0f };
            }
            else {
                float ang = (float)(VfxRand() % 3600u) * 0.1f * DEG2RAD;
                float mag = SEG_TILT * (0.45f + 0.55f * (float)(VfxRand() % 1000u) / 999.0f);
                // Vertical is the tight axis: the view is 16:9 but fovy is
                // vertical, so an up/down tilt crowds the up/down lane nodes
                // against the screen edge long before a sideways one would.
                d = (Vector2){ cosf(ang) * mag, sinf(ang) * mag * SEG_TILT_Y };
            }

            // Offset at the start of n = offset at n-1 plus the heading
            // integrated across segment n-1 (weights 0.25 old, 0.75 new).
            c = (Vector2){ pc.x + SEG_Z * (0.25f * pd.x + 0.75f * d.x),
                           pc.y + SEG_Z * (0.25f * pd.y + 0.75f * d.y) };
        }

        segDir[slot] = d;
        segCum[slot] = c;
        segReady = n;
    }
}

static Vector2 TunnelOffset(float z) {
    if (z < 0.0f) z = 0.0f;
    int s = (int)(z / SEG_Z);
    float t = z / SEG_Z - (float)s;
    EnsureSegments(s + 1);

    Vector2 d0 = segDir[s & (SEG_SLOTS - 1)];
    Vector2 d1 = segDir[(s + 1) & (SEG_SLOTS - 1)];
    Vector2 c = segCum[s & (SEG_SLOTS - 1)];
    float bi = BlendIntegral(t);

    // Flattening the whole curve here rather than at generation time means the
    // comfort toggle takes effect at once - segments are built well ahead of
    // the playhead, so changing SEG_TILT would not show for half a minute.
    float k = reducedMotion ? 0.35f : 1.0f;
    return (Vector2) {
        (c.x + SEG_Z * (d0.x * (t - bi) + d1.x * bi)) * k,
            (c.y + SEG_Z * (d0.y * (t - bi) + d1.y * bi)) * k
    };
}

static Vector3 WorldPos(float z, Vector2 local) {
    Vector2 o = TunnelOffset(z);
    return (Vector3) { o.x + local.x, o.y + local.y, z };
}

// ---- small vector helpers ------------------------------------------------
// Only what the ship needs to build an oriented frame. Not worth pulling in
// raymath.h for four functions.
static Vector3 V3Add(Vector3 a, Vector3 b) {
    return (Vector3) { a.x + b.x, a.y + b.y, a.z + b.z };
}
static Vector3 V3Sub(Vector3 a, Vector3 b) {
    return (Vector3) { a.x - b.x, a.y - b.y, a.z - b.z };
}
static Vector3 V3Scale(Vector3 a, float s) {
    return (Vector3) { a.x* s, a.y* s, a.z* s };
}
static Vector3 V3Cross(Vector3 a, Vector3 b) {
    return (Vector3) { a.y* b.z - a.z * b.y, a.z* b.x - a.x * b.z, a.x* b.y - a.y * b.x };
}
static Vector3 V3Norm(Vector3 a) {
    float l = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 1e-6f) return (Vector3) { 0.0f, 0.0f, 1.0f };
    return (Vector3) { a.x / l, a.y / l, a.z / l };
}

// ---- entities ------------------------------------------------------------
typedef struct {
    float hitBeat;
    int   lane;              // the lane the player must press
    int   laneFrom;          // where it starts out; differs only on a twist bar
    int   alive;
    int   escaped;           // missed - now flying at the camera to ram it
    int   twist;
    int   hidden;            // stays invisible until it is nearly at the gate
    int   trap;              // never announced by the call; hitting it hurts
    float outward;           // 0..1, a spent trap sliding out of the tunnel

    // Extra beats of travel a missed monster has made toward the camera. Zero
    // for everything else, so the same z applies to every monster.
    float rush;
    unsigned char tier;      // drives how big the monster is drawn

    // 0 for an ordinary note. METAPHOR turns a few notes into pickups; the
    // note still has to be played, the item is what it pays on top.
    unsigned char item;
} Enemy;

typedef struct {
    float beat;
    int   lane;
    int   pending;
} CallNote;

// A gimmick warning is scheduled like a call note rather than derived from the
// monsters that carry it. Derived, it fired 2.5 beats before whichever note
// happened to be first, which landed in a different place in every bar and on
// top of the call. Scheduled at the head of the call bar it is always in the
// same spot, so "is this bar a gimmick bar" is answered at a fixed moment.
#define MAX_WARNS 16
typedef struct {
    float beat;
    const char* text;
    int   pending;
} WarnNote;

typedef struct {
    Vector3 to;
    float   life;
    Color   col;
} Beam;

typedef struct {
    Vector3 pos, vel;
    float   life;
    Color   col;
} Particle;

static Enemy    enemies[MAX_ENEMIES];
static CallNote calls[MAX_CALLS];
static WarnNote warns[MAX_WARNS];
static Beam     beams[MAX_BEAMS];
static Particle particles[MAX_PARTICLES];

// Faster subdivisions are drawn smaller: closer together in z, so a full-size
// cube would crowd its neighbour, and the smaller shape doubles as a warning
// that a fast bar is coming.
//
// The sixteenth branch is unreachable while the pattern tables stay
// eighth-limited. It stays for the same reason the sixteenth playability check
// does - it is what would be needed the moment sixteenths come back.
static float SubScale(unsigned char sub) {
    if (sub >= 4) return 0.75f;
    if (sub >= 2) return 1.15f;
    return 1.40f;
}

// On a twist bar the monsters arrive in the lanes opposite the ones the call
// named, then slide around the ring to where they belong. The player reads the
// movement, not a memorised rule, which is what keeps it fair.
//
// The slide used to run 2.5 -> 1.5 beats before the gate, which left only 1.5
// beats to read the destination and move a hand there - under a second at any
// tempo the game plays at, on top of whatever else the bar is doing. It was
// the single most complained-about thing in playtesting.
//
// A monster becomes visible 6.2 beats out (the draw cull is camZ + 45, and
// z = camZ + HIT_OFFSET + beats*SPACING), so starting at 5.0 puts the slide
// just after it appears and leaves 3.5 settled beats instead of 1.5.
//
// Moving it earlier costs nothing in fairness: SpawnCall already names the
// destination lane, not the starting one, so a player going by ear was never
// relying on the slide. It only ever gated the players reading with their eyes.
#define TWIST_START 5.0f     // beats before the hit when the slide begins
#define TWIST_END   3.5f     // and when it has finished

// Beats the monster sits in its starting lane before sliding, under FOG.
// Without it the slide begins on the frame it becomes visible and there is
// nothing to see it move away FROM.
#define TWIST_FOG_LEAD 0.4f

// ---- bar gimmicks --------------------------------------------------------
// A response bar carries at most one of these. Normal only ever sees SWAP,
// which is the gentlest; the other two unlock deep into Hard, where ninety
// levels of the same top pool would otherwise flatten out.
//
// The three rates are split so the total stays near the old single-gimmick
// rate - a bar should still usually be a plain bar.
typedef enum { GIM_NONE, GIM_TWIST, GIM_HIDDEN, GIM_TRAP } Gimmick;

// TRAPS is the only gimmick Normal ever sees: it is the one the player can
// answer by doing nothing, so it is the fairest thing to meet first. SWAP and
// BLIND both demand a reaction and are held back for Hard.
#define NORMAL_TRAP_PCT 18
#define GIM_HARD_LEVEL  4    // zero-based, so Hard level 5
#define GIM_TWIST_PCT   5
#define GIM_HIDDEN_PCT  5
#define GIM_TRAP_PCT    10

// Endless: the gimmick rate climbs with the level, because BPM and the window
// both stop moving around level 50 and something has to keep adding pressure.
//
// Capped twice over, and both caps matter for different reasons.
#define GIM_LEVEL_STEP 0.035f   // per level, multiplying the base rates
#define GIM_LEVEL_MAX  2.20f    // reached at level 35; 5/5/10 becomes 11/11/22

// The three chances are rolled against ONE shared 0..99 in sequence, so a sum
// over 100 does not mean "always a gimmick" - it means whichever one is last in
// the chain never fires at all. TRAPS would silently vanish from a deck that
// stacked SWAP and BLIND cards. Cards multiply these by up to x3 or x4 and the
// level now scales them too, so the sum is clamped PROPORTIONALLY: the ratio
// the deck was built around survives, only the total is held down.
#define GIM_TOTAL_MAX  70

static float GimLevelScale(int lv) {
    lv += TEST_LEVEL_BIAS;
    float k = 1.0f + (float)lv * GIM_LEVEL_STEP;
    return (k > GIM_LEVEL_MAX) ? GIM_LEVEL_MAX : k;
}

#define HIDDEN_REVEAL 1.1f   // beats before the gate when a hidden note appears
#define HP_TRAP 0.22f        // hitting a trap costs more than simply firing wide

// How much faster than chart speed a missed monster closes the last stretch.
//
// At 1.0 it drifts in at the speed everything else moves, and the gap from the
// gate to the camera is HIT_OFFSET / SPACING = 1.33 beats. That is most of a
// bar: the hit lands on top of the next notes, so one dropped note drags the
// following ones down with it. At 3.0 the same distance takes 0.44 beats and
// the consequence arrives while the mistake is still the thing on screen.
#define MISS_RUSH 3.0f

// How far through its slide a twisting monster is: 0 at the starting lane,
// 1 at the destination. Position and colour both ride this, so the monster
// looks like it belongs where it currently is rather than announcing its
// destination from the far end of the tunnel.
// When the slide runs, in beats before the hit. Normally the constants above,
// but FOG moves the whole gesture.
//
// FOG stops drawing a monster until it is `fogBeats` (3.0) from the gate, and
// the normal slide is over by 3.5 - so under FOG the monster was finished
// moving before it could be seen at all. It appeared already swapped, and the
// gimmick simply did not exist for anyone holding that card.
//
// So the gesture is refitted into the window the player can actually see: a
// moment sitting in the starting lane, the slide, then time to answer it.
// At fogBeats 3.0 that is 0.4 / 0.9 / 1.7 beats.
static void TwistWindow(float* start, float* end) {
    if (mods.fogBeats <= 0.0f) {
        *start = TWIST_START;
        *end = TWIST_END;
        return;
    }
    float s = mods.fogBeats - TWIST_FOG_LEAD;
    if (s < 0.5f) s = 0.5f;              // a pathologically tight fog still slides
    float e = s * 0.65f;
    if (s - e < 0.25f) e = s - 0.25f;    // never an instant snap
    if (e < 0.1f) e = 0.1f;
    *start = s;
    *end = e;
}

static float EnemyTwistT(const Enemy* e, float beat) {
    if (!e->twist) return 1.0f;
    float ts, te;
    TwistWindow(&ts, &te);
    float t = (beat - (e->hitBeat - ts)) / (ts - te);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static Color EnemyColor(const Enemy* e, float beat) {
    Color a = LaneColor(e->laneFrom);
    Color b = LaneColor(e->lane);
    float t = EnemyTwistT(e, beat);
    return (Color) { (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t), 255 };
}

// Where the monster is on the ring right now.
static Vector2 EnemyLocal(const Enemy* e, float beat) {
    if (!e->twist) return LanePos(e->lane);

    float t = EnemyTwistT(e, beat);
    if (t <= 0.0f) return LanePos(e->laneFrom);
    if (t >= 1.0f) return LanePos(e->lane);

    // Slide along the ring rather than cutting across it. LanePos(n) sits at
    // 90*(n+1) degrees, so the lanes and this stay in step by construction.
    float a0 = 90.0f * (float)(e->laneFrom + 1);
    float a1 = 90.0f * (float)(e->lane + 1);
    float d = a1 - a0;
    while (d > 180.0f) d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    float a = (a0 + d * t + spinAngle) * DEG2RAD;   // same ring, same rotation
    return (Vector2) { cosf(a) * TUNNEL_R, sinf(a) * TUNNEL_R };
}

// Slots are recycled: a dead slot is reused by the next spawn, so the chart
// can run forever inside a fixed array.
static void SpawnEnemy(float hitBeat, int lane, int laneFrom,
    unsigned char tier, int twist, int hidden, int trap) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive) continue;
        enemies[i].hitBeat = hitBeat;
        enemies[i].lane = lane;
        enemies[i].laneFrom = laneFrom;
        enemies[i].tier = tier;
        enemies[i].twist = twist;
        enemies[i].hidden = hidden;
        enemies[i].trap = trap;

        // A trap is never a pickup: the whole point of a trap is that it must
        // not be hit, and hanging a reward on it would make that a lie.
        enemies[i].item = 0;
        if (!trap && mods.itemPct > 0 && (int)(Rand() % 100u) < mods.itemPct) {
            unsigned int r = Rand() % 9u;
            enemies[i].item = (unsigned char)((r < 2) ? 1 : (r < 3) ? 2 :
                                              (r < 5) ? 3 : (r < 6) ? 4 :
                                              (r < 8) ? 5 : 6);
        }
        enemies[i].outward = 0.0f;
        enemies[i].rush = 0.0f;
        enemies[i].escaped = 0;
        enemies[i].alive = 1;
        return;
    }
}

static void SpawnCall(float beat, int lane) {
    for (int i = 0; i < MAX_CALLS; i++) {
        if (calls[i].pending) continue;
        calls[i].beat = beat;
        calls[i].lane = lane;
        calls[i].pending = 1;
        return;
    }
}

static void SpawnWarn(float beat, const char* text) {
    for (int i = 0; i < MAX_WARNS; i++) {
        if (warns[i].pending) continue;
        warns[i].beat = beat;
        warns[i].text = text;
        warns[i].pending = 1;
        return;
    }
}

static void SpawnBeam(Vector3 to, Color col) {
    int oldest = 0;
    for (int i = 0; i < MAX_BEAMS; i++) {
        if (beams[i].life <= 0.0f) { oldest = i; break; }
        if (beams[i].life < beams[oldest].life) oldest = i;
    }
    beams[oldest].to = to;
    beams[oldest].col = col;
    beams[oldest].life = 1.0f;
}

static void SpawnBurst(Vector3 at, Color col, int count) {
    int made = 0;
    for (int i = 0; i < MAX_PARTICLES && made < count; i++) {
        if (particles[i].life > 0.0f) continue;
        particles[i].pos = at;
        particles[i].vel = (Vector3){ VfxSigned() * 7.0f,
                                      VfxSigned() * 7.0f,
                                      VfxSigned() * 3.0f };
        particles[i].life = 1.0f;
        particles[i].col = col;
        made++;
    }
}

// ---- levels --------------------------------------------------------------
// The arc of the whole run. Tempo climbs gently; the real difficulty comes
// from the pattern pools opening up. The hit window has to tighten alongside,
// because in a pool 3 bar two notes can be only 0.25 beats apart.
//
// Each level names two adjacent pools and picks between them per bar, so the
// difficulty is a slope rather than a staircase - and, crucially, a level
// never reaches back into pools it has already left behind.
typedef struct {
    float bpm;
    unsigned char pool[2];
    float window;
} Level;

// Half the run is spent in pools 0 and 1 on purpose. With sixteenths gone the
// top pools are no longer brutal, so the ramp is free to be gentle early and
// still have somewhere to go.
static const Level normalLevels[NORMAL_LEVELS] = {
    { 120.0f, { 0, 0 }, 0.28f },   //  1  quarters, learning the lanes
    { 124.0f, { 0, 0 }, 0.28f },   //  2
    { 128.0f, { 0, 1 }, 0.27f },   //  3  syncopation and chords bleed in
    { 132.0f, { 1, 1 }, 0.26f },   //  4
    { 136.0f, { 1, 1 }, 0.25f },   //  5
    { 140.0f, { 1, 2 }, 0.24f },   //  6  eighth density and a third role
    { 146.0f, { 2, 2 }, 0.23f },   //  7
    { 152.0f, { 2, 2 }, 0.22f },   //  8  pool 2 gets extra room - it is the big step
    { 158.0f, { 2, 3 }, 0.21f },   //  9  all four points of the ring
    { 164.0f, { 3, 3 }, 0.20f },   // 10
};

// Hard cannot use a table - a hundred hand written rows would be unmaintainable
// and mostly identical. It climbs to the top pool in nine levels and then keeps
// squeezing tempo and the hit window for the remaining ninety.
static Level LevelAt(int lv) {
    lv += TEST_LEVEL_BIAS;
    if (lv < 0) lv = 0;

    if (mode == MODE_NORMAL) {
        if (lv > NORMAL_LEVELS - 1) lv = NORMAL_LEVELS - 1;
        return normalLevels[lv];
    }

    // Hard opens above Normal's tempo rather than repeating its first levels,
    // and climbs roughly twice as fast to reach the same ceiling in half the
    // levels.
    Level l;
    l.bpm = 132.0f + (float)lv * 1.35f;
    if (l.bpm > 200.0f) l.bpm = 200.0f;
    l.window = 0.25f - (float)lv * 0.0022f;
    if (l.window < 0.13f) l.window = 0.13f;

    // Sixteenths come back in two steps rather than one. Dropping the full
    // sixteenth pool in at once would be a cliff.
    //
    // Pool 6 is the last one and lands at level 35, well after BPM and the
    // window have stopped moving. Past that point the chart itself is the only
    // thing still climbing, which is what Endless needs it to be.
    if (lv >= HARD_CHORD_LEVEL) { l.pool[0] = 5; l.pool[1] = 6; return l; }
    if (lv >= HARD_SIX2_LEVEL)  { l.pool[0] = 4; l.pool[1] = 5; return l; }
    if (lv >= HARD_SIX1_LEVEL)  { l.pool[0] = 3; l.pool[1] = 4; return l; }

    int p = lv / 2;
    if (p > 3) p = 3;
    l.pool[0] = (unsigned char)p;
    l.pool[1] = (unsigned char)((p < 3) ? p + 1 : 3);
    return l;
}

// ---- chart generation ----------------------------------------------------
// Patterns are written with abstract role indices, not lanes:
//
//   '.'       rest
//   '1'..'4'  role index, mapped onto a real lane at generation time
//   'A'       roles 1 and 2 together (a chord)
//   '|'       reading aid, ignored
//
// One bar is 16 sixteenth-note slots; slot 0/4/8/12 are the beats. Writing
// roles instead of lanes is what buys the variety: a single line becomes up to
// 24 different lane arrangements, so "1.2.|1.2." covers opposite pairs and
// adjacent pairs both, where a fixed lane spelling could only ever be one.
//
// `geo` opts a pattern out of that freedom. Free permutation destroys
// geometry - a circle written as 1-2-3-4 would come out as a zigzag two thirds
// of the time - so shapes that are meant to travel around the ring are mapped
// only through the ring's own symmetries (4 rotations and 4 reflections),
// which keeps a circle a circle and a pendulum a pendulum.
typedef struct {
    const char* pat;
    unsigned char geo;
} PatEntry;

// NOTHING FINER THAN AN EIGHTH. Sixteenths were tried and pulled back out:
// with roles mapped at random they produced too many bars that read as unfair.
// Only slots 0, 2, 4, 6, 8, 10, 12, 14 may carry a note - in the string that
// means every group of four is "X.Y.", and positions 1 and 3 stay '.'.
//
// That removes subdivision as a difficulty axis, so the pools climb on what is
// left instead: how many roles, whether chords appear, how much syncopation,
// and how dense the bar is.

// Pool 0: quarters only, one or two roles, no chords. One-role bars are three
// or four hits - a two hit bar is too little to read as a phrase.
static const PatEntry POOL0[] = {
    { "1...|1...|1...|....", 0 },
    { "1...|....|1...|1...", 0 },
    { "1...|1...|....|1...", 0 },
    { "....|1...|1...|1...", 0 },
    { "1...|1...|1...|1...", 0 },
    { "1...|2...|1...|2...", 0 },   // two roles, alternating
    { "1...|1...|2...|2...", 0 },
    { "1...|2...|2...|1...", 0 },   // sandwich
    { "1...|2...|1...|1...", 0 },
};

// Pool 1: syncopation and chords - but NOT eighth density. What makes eighths
// hard is how fast the hand has to move, not where the note sits, so the two
// are separated here: every bar in this pool keeps consecutive notes at least
// a full beat apart, and only displaces them off the beat.
//
// INVARIANT: no two notes in a pool 1 bar are closer than 4 slots. Adding a
// bar that breaks it puts eighth density back into the pool that is supposed
// to be free of it - see the check script in NOTES.md.
static const PatEntry POOL1[] = {
    { "..1.|..1.|..1.|..1.", 0 },   // offbeat only, one role
    { "..1.|..2.|..1.|..2.", 0 },   // offbeat only, two roles
    { "1...|2...|..1.|..2.", 0 },   // on the beat, then off it
    { "..1.|..2.|....|1...", 0 },
    { "1...|..2.|....|1...", 0 },
    { "..1.|....|2...|..1.", 0 },
    { "1...|2...|1...|..2.", 0 },
    { "..1.|..1.|....|..1.", 0 },
    { "1...|..1.|..2.|....", 0 },
    { "A...|A...|A...|A...", 0 },   // chords arrive, on the beat
    { "A...|A...|....|A...", 0 },
    { "A...|....|A...|A...", 0 },
    { "....|A...|A...|A...", 0 },
    { "..A.|..A.|..A.|..A.", 0 },   // chords off the beat
    { "A...|1...|A...|2...", 0 },
    { "A...|..1.|....|A...", 0 },
};

// Pool 2: eighth density and a third role. This is the real step up.
static const PatEntry POOL2[] = {
    { "1.1.|1...|1.1.|1...", 0 },
    { "1.1.|1.1.|1.1.|1.1.", 0 },   // straight eighths, one role
    { "1...|1.2.|1...|1.2.", 0 },
    { "1.2.|1...|1.2.|1...", 0 },
    { "1.1.|2.2.|1.1.|2.2.", 0 },
    { "1.2.|1.2.|1.2.|1.2.", 0 },   // straight eighths, alternating
    { "1...|2...|1.1.|2...", 0 },
    { "1...|2.2.|1...|2.2.", 0 },
    { "1.2.|2.1.|1.2.|2.1.", 0 },
    { "1...|2...|3...|1...", 1 },   // third role: arc out and back
    { "1...|2.3.|1...|2.3.", 1 },
    { "1...|2...|3.3.|2...", 1 },   // pendulum
    { "1.2.|3...|1.2.|3...", 1 },
    { "1...|2.1.|3...|2.1.", 1 },
    { "1.2.|3.1.|2.3.|1...", 1 },   // weaving arc
    { "1...|A...|2...|A...", 0 },   // single, chord, single, chord
    { "A...|1.2.|A...|1.2.", 0 },
};

// Pool 3: all four roles, eighths throughout, chords mixed in.
static const PatEntry POOL3[] = {
    { "1...|2...|3...|4...", 1 },   // the circle, on the beat
    { "1.2.|3.4.|1.2.|3.4.", 1 },   // the circle, in eighths
    { "1.2.|3.4.|4.3.|2.1.", 1 },   // round and back
    { "1.1.|2.2.|3.3.|4.4.", 1 },   // each point doubled
    { "1.2.|2.3.|3.4.|4.1.", 1 },   // overlapping steps
    { "1.2.|3.4.|A...|A...", 1 },
    { "A...|1.2.|A...|3.4.", 0 },
    { "A.A.|1.2.|A.A.|3.4.", 0 },
    { "A.A.|A.A.|1.2.|3.4.", 0 },
    { "1...|A...|2...|A...", 0 },   // single, chord, single, chord
};

// Pools 4 and 5 are the only place sixteenths live, and they are reachable
// only deep into Hard. The rule that got them removed the first time still
// holds everywhere else: with roles mapped at random, sixteenth bars produce
// unfair combinations far too often to be part of the general vocabulary.
//
// Pool 4 keeps every sixteenth run on ONE role, so it is a finger tremor and
// nothing more. Pool 5 lets two roles share a run.
static const PatEntry POOL4[] = {
    { "1111|....|1111|....", 0 },   // the plain tremolo
    { "1111|2...|1111|3...", 0 },
    { "1.2.|1111|3.4.|....", 0 },
    { "1111|....|2...|3.4.", 0 },
    { "1...|1111|2...|2222", 0 },
    { "1111|2222|1.2.|....", 0 },
    { "1.1.|1111|2.2.|....", 0 },
    { "1111|3...|2222|4...", 1 },
};

static const PatEntry POOL5[] = {
    { "1212|3.4.|3434|1.2.", 1 },
    { "1212|1212|3434|3434", 0 },
    { "1122|3344|1122|3344", 1 },
    { "1213|1214|3431|3432", 0 },
    { "1111|2222|3333|4444", 1 },
    { "1212|3434|1234|....", 1 },
    { "1.2.|1212|3.4.|3434", 1 },
    { "1231|2312|3123|1231", 1 },
};

typedef struct { const PatEntry* items; int count; } Pool;


// Sixteenths with chords inside them.
//
// POOL5 is already a wall of sixteenths, so density has nowhere left to go -
// the only axis still open is asking for two lanes AT ONCE inside that wall.
// The hands have to split without the tempo letting up, which is a different
// skill from playing fast, not more of the same one.
//
// `A` is roles 1+2 together (see DecodeRoles). Every entry here is free to
// permute, because a chord has no geometry to destroy.
static const PatEntry POOL6[] = {
    { "A.1.|A.2.|A.3.|A.4.", 0 },
    { "A...|1234|A...|4321", 0 },
    { "A.A.|1212|A.A.|3434", 0 },
    { "1212|A.A.|3434|A.A.", 0 },
    { "A123|A234|A341|A412", 0 },
    { "AA..|1234|AA..|4321", 0 },
    { "A1A2|A3A4|A1A2|A3A4", 0 },
    { "1A2A|3434|1A2A|1212", 0 },
};

#define POOL_COUNT 7
static const Pool POOLS[POOL_COUNT] = {
    { POOL0, (int)(sizeof(POOL0) / sizeof(POOL0[0])) },
    { POOL1, (int)(sizeof(POOL1) / sizeof(POOL1[0])) },
    { POOL2, (int)(sizeof(POOL2) / sizeof(POOL2[0])) },
    { POOL3, (int)(sizeof(POOL3) / sizeof(POOL3[0])) },
    { POOL4, (int)(sizeof(POOL4) / sizeof(POOL4[0])) },
    { POOL5, (int)(sizeof(POOL5) / sizeof(POOL5[0])) },
    { POOL6, (int)(sizeof(POOL6) / sizeof(POOL6[0])) },
};

// A bar with its roles resolved to lanes.
typedef struct {
    unsigned char lane[SLOTS_PER_BAR];   // lane bitmask per slot, 0 = rest
    unsigned char sub;                   // finest subdivision used: 1, 2 or 4
} Measure;

#define LANEBIT(l) ((unsigned char)(1u << (l)))
#define UD_PAIR (LANEBIT(0) | LANEBIT(2))

static void DecodeRoles(const char* s, unsigned char out[SLOTS_PER_BAR]) {
    int i = 0;
    for (; *s != '\0' && i < SLOTS_PER_BAR; s++) {
        if (*s == '|') continue;
        if (*s >= '1' && *s <= '4') out[i] = (unsigned char)(1u << (*s - '1'));
        else if (*s == 'A')         out[i] = 0x03;
        else                        out[i] = 0;
        i++;
    }
    for (; i < SLOTS_PER_BAR; i++) out[i] = 0;
}

// Free permutation, or - when the shape matters - one of the ring's own eight
// symmetries: role i lands on lane (r + i) or (r - i), which keeps roles that
// were written in order adjacent on the ring.
static void BuildMap(int geo, int map[4]) {
    if (geo) {
        int r = (int)(Rand() & 3u);
        int mirror = (int)(Rand() & 1u);
        for (int i = 0; i < 4; i++) {
            map[i] = mirror ? ((r - i) & 3) : ((r + i) & 3);
        }
        return;
    }

    int bag[4] = { 0, 1, 2, 3 };
    for (int i = 3; i > 0; i--) {
        int j = (int)(Rand() % (unsigned int)(i + 1));
        int t = bag[i]; bag[i] = bag[j]; bag[j] = t;
    }
    for (int i = 0; i < 4; i++) map[i] = bag[i];
}

static void ApplyMap(const unsigned char roles[SLOTS_PER_BAR], const int map[4],
    unsigned char out[SLOTS_PER_BAR]) {
    for (int i = 0; i < SLOTS_PER_BAR; i++) {
        unsigned char m = 0;
        for (int r = 0; r < 4; r++) {
            if (roles[i] & (1u << r)) m |= LANEBIT(map[r]);
        }
        out[i] = m;
    }
}

// Up and then down is the awkward move: on the arrow keys it is one finger
// asked to be in two places. A sixteenth apart it is impossible, an eighth
// apart merely awkward - so the eighth case is only barred in the early pools,
// while the player is still learning, and opens up later.
//
// The sixteenth check is dead while the pattern tables stay eighth-limited.
// It is kept because it costs nothing and is exactly what would be needed
// again the moment a sixteenth pattern is added back.
//
// Everything else is allowed, including diagonal chords, which read fine on a
// ring.
static int MapPlayable(const unsigned char lane[SLOTS_PER_BAR], int strict) {
    for (int i = 0; i < SLOTS_PER_BAR; i++) {
        if (!lane[i]) continue;

        if (i + 1 < SLOTS_PER_BAR && lane[i + 1]
            && (lane[i] | lane[i + 1]) == UD_PAIR) return 0;

        if (strict && i + 2 < SLOTS_PER_BAR && lane[i + 2] && !lane[i + 1]
            && (lane[i] | lane[i + 2]) == UD_PAIR) return 0;
    }
    return 1;
}

static unsigned char MeasureSub(const Measure* m) {
    unsigned char s = 1;
    for (int i = 0; i < SLOTS_PER_BAR; i++) {
        if (!m->lane[i]) continue;
        if (i & 1) return 4;                 // lands on a sixteenth
        if ((i & 3) == 2) s = 2;             // lands on an eighth
    }
    return s;
}

// A twist sends every note to the opposite point on the ring, so a chord of an
// opposite pair maps onto itself and the swap is invisible. Diagonal chords do
// move, so only opposite pairs disqualify a bar.
// Is there anywhere in this bar a trap could legally go?
//
// The test is per LANE, not per slot. Judgement never compares notes across
// lanes, so a trap in an unused lane is perfectly readable even while a real
// note is sounding at the same instant - a bar that only uses left and right
// can carry traps up and down throughout. Only the trap's own lane has to be
// clear, and only for half a beat either side.
static int CanHostTraps(const Measure* m) {
    for (int i = 0; i < SLOTS_PER_BAR; i += 2) {
        unsigned char busy = 0;
        for (int d = -2; d <= 2; d++) {
            int j = i + d;
            if (j >= 0 && j < SLOTS_PER_BAR) busy |= m->lane[j];
        }
        if ((busy & 0x0F) != 0x0F) return 1;
    }
    return 0;
}

static int HasOppositePairChord(const Measure* m) {
    for (int i = 0; i < SLOTS_PER_BAR; i++) {
        unsigned char v = m->lane[i];
        if (!(v & (v - 1))) continue;                        // not a chord
        if (v == UD_PAIR) return 1;
        if (v == (LANEBIT(1) | LANEBIT(3))) return 1;
    }
    return 0;
}

static void RealizeMeasure(const PatEntry* pe, int strict, Measure* out) {
    unsigned char roles[SLOTS_PER_BAR];
    DecodeRoles(pe->pat, roles);

    int map[4];
    for (int attempt = 0; attempt < 32; attempt++) {
        BuildMap(pe->geo, map);
        ApplyMap(roles, map, out->lane);
        if (MapPlayable(out->lane, strict)) break;
    }
    // If no arrangement passed, the last one stands. Dropping notes here would
    // be worse: the call and the response are emitted from this same measure,
    // and they have to stay identical or the game lies to the player.
    out->sub = MeasureSub(out);
}

static void EmitMeasure(const Measure* m, float barBeat, int isCall, Gimmick g) {
    int twist = (g == GIM_TWIST);
    int hidden = (g == GIM_HIDDEN);

    for (int i = 0; i < SLOTS_PER_BAR; i++) {
        if (!m->lane[i]) continue;
        float b = barBeat + (float)i * SLOT_BEATS;
        for (int lane = 0; lane < LANE_COUNT; lane++) {
            if (!(m->lane[i] & LANEBIT(lane))) continue;
            // The call names the lane the player must actually press, twist
            // included. It used to name the starting lane, which meant an
            // ear-player was told the wrong answer on every twist bar - fatal
            // for a game whose whole premise is that you can play it by ear.
            int to = twist ? ((lane + 2) & 3) : lane;
            if (isCall) SpawnCall(b, to);
            else        SpawnEnemy(b, to, lane, m->sub, twist, hidden, 0);
        }
    }

    // Traps are never called - that is the whole point of them, and it is why
    // they only punish players reading with their eyes.
    //
    // Any eighth slot is a candidate; what has to be free is the trap's LANE,
    // not the moment. See CanHostTraps() - the earlier version required the
    // whole slot to be empty, which meant a left/right bar could never carry a
    // trap even though up and down sat unused the entire time.
    if (isCall || g != GIM_TRAP) return;

    int slots[SLOTS_PER_BAR / 2], n = 0;
    for (int i = 0; i < SLOTS_PER_BAR; i += 2) slots[n++] = i;

    int want = 1 + (int)(Rand() % 2u);       // one or two per bar
    for (int k = 0; k < want && n > 0; k++) {
        int pick = (int)(Rand() % (unsigned int)n);
        int slot = slots[pick];
        slots[pick] = slots[--n];            // never the same slot twice

        // Within its own lane the trap needs half a beat of clearance either
        // side. Otherwise a player merely late on a real note would find the
        // trap nearer to their press and get punished for good intentions.
        unsigned char busy = 0;
        for (int d = -2; d <= 2; d++) {
            int j = slot + d;
            if (j >= 0 && j < SLOTS_PER_BAR) busy |= m->lane[j];
        }
        if ((busy & 0x0F) == 0x0F) continue;      // no safe lane at this slot

        int lane;
        do { lane = (int)(Rand() & 3u); } while (busy & LANEBIT(lane));

        SpawnEnemy(barBeat + (float)slot * SLOT_BEATS, lane, lane,
            m->sub, 0, 0, 1);
    }
}

// Bars are drawn independently. Call-and-response already supplies the
// repetition a rhythm needs to be readable - the player hears each bar before
// playing it - so an AABA structure on top of that only costs variety.
//
// A level draws from two adjacent pools, never the whole history: at level 8 a
// pool 0 bar would be a free pass, and blending two neighbours makes the ramp
// a slope instead of a staircase.
static void GenerateChunk(float startBeat, int lv) {
    for (int k = 0; k < BARS_PER_CHUNK; k++) {
        Level ld = LevelAt(lv);
        int pi = ld.pool[Rand() & 1u];
        const Pool* pool = &POOLS[pi];
        const PatEntry* pe = &pool->items[Rand() % (unsigned int)pool->count];

        Measure m;
        // Pools 0-2 get the stricter check: pool 2 is where eighth density is
        // being learned, so it is the one that most needs the guard.
        RealizeMeasure(pe, pi <= 2, &m);

        // LEVEL_REST_BEATS of silence first, so the level-up jingle gets the
        // stage to itself before the new level's first call comes in.
        float base = startBeat + LEVEL_REST_BEATS + k * BAR_STRIDE;

        // Pool 0 bars are left as breathing room. A swap is also invisible on
        // an opposite-pair chord, so those never twist.
        int roll = (int)(Rand() % 100u);
        int eligible = (pi >= 1);
        Gimmick g = GIM_NONE;

        // A forced gimmick overrides the roll entirely. `forceGim == 4` spreads
        // evenly across the three, so the per-gimmick multipliers still tilt it.
        if (eligible && mods.forceGim) {
            if (mods.forceGim == 4) {
                float w0 = mods.gimMul[0], w1 = mods.gimMul[1], w2 = mods.gimMul[2];
                float sum = w0 + w1 + w2;
                if (sum <= 0.0f) g = GIM_NONE;
                else {
                    float r = (float)(Rand() % 1000u) / 1000.0f * sum;
                    g = (r < w0) ? GIM_TWIST : (r < w0 + w1) ? GIM_HIDDEN : GIM_TRAP;
                }
            }
            else g = (Gimmick)mods.forceGim;
        }
        else if (eligible && mode == MODE_HARD && lv >= GIM_HARD_LEVEL) {
            float ls = GimLevelScale(lv);
            int pT = (int)(GIM_TWIST_PCT  * mods.gimMul[0] * ls);
            int pB = (int)(GIM_HIDDEN_PCT * mods.gimMul[1] * ls);
            int pR = (int)(GIM_TRAP_PCT   * mods.gimMul[2] * ls);
            int gsum = pT + pB + pR;
            if (gsum > GIM_TOTAL_MAX) {
                pT = pT * GIM_TOTAL_MAX / gsum;
                pB = pB * GIM_TOTAL_MAX / gsum;
                pR = pR * GIM_TOTAL_MAX / gsum;
            }
            if (roll < pT) g = GIM_TWIST;
            else if (roll < pT + pB) g = GIM_HIDDEN;
            else if (roll < pT + pB + pR) g = GIM_TRAP;
        }
        else if (eligible && roll < (int)(NORMAL_TRAP_PCT * mods.gimMul[2])) {
            g = GIM_TRAP;        // Normal, and early Hard
        }
        // Normal never shows SWAP or BLIND, and that has to hold even when a
        // card says otherwise. The forceGim cards are kept out of Normal's
        // draft, but the shop and the gacha are deliberately not gated, so one
        // can still be carried in - and "a gimmick every bar" would then turn
        // on the two gimmicks the mode is built never to show.
        //
        // Falling through to TRAPS rather than to nothing: the card still does
        // what it says, with the only gimmick Normal has.
        if (mode == MODE_NORMAL && (g == GIM_TWIST || g == GIM_HIDDEN)) {
            g = mods.forceGim ? GIM_TRAP : GIM_NONE;
        }

        // A gimmick the bar cannot express falls through to one it can, so the
        // overall gimmick rate stays what the constants say it is.
        if (g == GIM_TWIST && HasOppositePairChord(&m)) g = GIM_NONE;
        if (g == GIM_TRAP && !CanHostTraps(&m))         g = GIM_HIDDEN;

        // One fixed warning per gimmick bar, at the head of its call bar.
        if (g == GIM_TWIST)       SpawnWarn(base, "SWAP");
        else if (g == GIM_HIDDEN) SpawnWarn(base, "BLIND");
        else if (g == GIM_TRAP)   SpawnWarn(base, "TRAPS");

#if CALL_RESPONSE
        // The call is emitted with the same gimmick so it announces the
        // destination lanes, not the ones the monsters set off from.
        EmitMeasure(&m, base, 1, g);                    // call: sound only
        EmitMeasure(&m, base + BEATS_PER_BAR, 0, g);    // response: monsters
#else
        EmitMeasure(&m, base, 0, g);
#endif
    }
}

// ---- sound ---------------------------------------------------------------
// Every sound is synthesised at startup. No audio files, no size cost.
// Pitch descends clockwise around the ring: up, right, down, left. That is one
// rule instead of two, and it makes the geometry audible - a bar that walks
// clockwise is a descending arpeggio, and one that walks the other way rises.
//
// The earlier mapping was "high at the top, low at the bottom", which sounds
// reasonable but leaves left and right as an arbitrary pair the ear has to
// memorise separately.
//
// A5 - E5 - C5 - A4: still an A minor spread, so the opposite-pair chords stay
// consonant (up+down a major sixth, left+right a perfect fifth).
static const float laneFreq[LANE_COUNT] = { 880.0f, 659.0f, 523.0f, 440.0f };

static Sound kickSnd;
static Sound dmgSnd;
static Sound levelSnd;
static Sound twistSnd;
static Sound hatSnd;

// Which sound a PERFECT makes.
//
// This is a taste decision that took several passes, so the rejected ones stay
// here rather than being deleted and re-derived. Change the number, rebuild,
// listen. Only the selected one is compiled.
//
//   0 RISE   two notes a fifth apart, rising. E5 -> B5.
//   1 CHORD  a struck major triad, C5+E5+G5 with an octave on top.
//   2 GLIDE  one note sliding up, 620 -> 1300Hz. Continuous, not stepped.
//   3 WOOD   a low inharmonic pluck around 330Hz. Warm, no top end.
//   4 RISE+  0 with each note harmonised into a chord. 0's shape, 1's weight.
//   5 LANE   the lane's OWN note opened out - four sounds, not one.
//
// The six above are all the same experiment: which PITCH to layer on the shot.
// Six passes of that came back "not it", so the axis itself is suspect. These
// three move a different one:
//
//   6 SNAP   no pitch at all - a 2ms crack that makes the shot land harder.
//   7 GOOD-  subtractive: PERFECT adds nothing, GOOD gets a MUFFLED shot.
//   8 STREAK the pitch climbs with the combo. Rewards the run, not the hit.
//
// 6 came back "much better than 0", and two of 8's rungs (659.3Hz, 880Hz) were
// picked out while the climbing itself was not wanted. These two combine what
// was liked instead of guessing again:
//
//   9 SNAP+  6's crack plus those two notes, fixed. Impact and tone.
//  10 FOURTH the two notes alone, to tell which half is doing the work.
//
// What has been tried and what was said about it:
//   a single bell at 1244Hz   "띡띡거린다" - one more high blip among many
//   RISE (0)                  "많이 나아졌는데" - and, after hearing 1-3,
//                             "넷 중엔 0이 제일 낫다"
//   CHORD (1)                 "방향성은 1 쪽이 괜찮다"
//
// 0 and the bell are both thin single lines high in the mix, which is the axis
// worth varying. CHORD answers with weight instead of pitch height, GLIDE with
// movement that is continuous rather than stepped, WOOD by dropping out of the
// register the rest of the game lives in, RISE+ by giving 0 the body 1 had,
// and LANE by not introducing a new note at all.
#define PERFECT_SOUND 10

// Rungs of the streak, for option 8.
#define COMBO_STEPS 6

// One sound for every lane, except the options that need more than one.
#if PERFECT_SOUND == 5
static Sound perfSnd[LANE_COUNT];       // one per lane
#elif PERFECT_SOUND == 7
static Sound hitSndDull[LANE_COUNT];    // GOOD gets this instead; PERFECT adds nothing
#elif PERFECT_SOUND == 8
static Sound perfSnd[COMBO_STEPS];      // one per rung of the streak
#else
static Sound perfSnd;
#endif
static Sound bassSnd[4];
static Sound arpSnd[4];
static Sound padSnd[4];
static Sound denySnd;
static Sound callSnd[LANE_COUNT];
static Sound hitSnd[LANE_COUNT];

// Backing layers stay clear of the lane frequencies so the call is never
// masked by the music underneath it.
static const float bassFreq[4] = { 110.0f, 87.31f, 130.81f, 98.0f };   // A2 F2 C3 G2
static const float arpFreq[4] = { 330.0f, 392.0f, 494.0f, 587.0f };   // E4 G4 B4 D5
static const float padFreq[4] = { 220.0f, 174.61f, 261.63f, 196.0f }; // A3 F3 C4 G3

// Which backing layers are running. Getting further has to be audible, not
// only faster.
// Normal reaches the top layer by level 7, which is right for a ten level run.
// Hard needs somewhere to keep going, so it climbs faster and has two layers
// beyond it - otherwise everything past level 7 sounded identical.
// Endless keeps thickening the backing well past where the chart stops adding
// new shapes, so a long run still sounds like it is going somewhere. Tiers 7-9
// land at 24, 29 and 34 - evenly spaced five levels apart from tier 6 at 19,
// and the last one just before the chord pool at 35.
static int MusicTier(int lv) {
    lv += TEST_LEVEL_BIAS;
    if (mode == MODE_HARD) {
        if (lv >= 33) return 9;      // + a pad underneath the whole thing
        if (lv >= 28) return 8;      // + hat on every sixteenth
        if (lv >= 23) return 7;      // + arpeggio on every sixteenth
        if (lv >= 19) return 6;      // + hat on every eighth
        if (lv >= 11) return 5;      // + bass on every beat
        if (lv >= 5)  return 4;
        if (lv >= 3)  return 3;
        if (lv >= 1)  return 2;
        return 1;
    }
    if (lv >= 6) return 4;      // + arpeggio
    if (lv >= 4) return 3;      // + bass
    if (lv >= 2) return 2;      // + hi-hat
    return 1;                   // kick only
}

// The call and the hit are the same note so the player hears the melody they
// are reproducing. `punchy` is the difference: a click transient and a fast
// downward bend, which is what reads as impact.
// `punchy` is the shot the ship fires when a note is hit; the plain one is the
// call the game plays.
//
// Deliberately restrained. An earlier version put a hard noise burst and a
// clipping peak here, and testers described the result as harsh - a gunshot
// rather than a note being hit. This sound is not the feedback; it only says
// which lane was struck. The feedback is MakePerfect below, and this has to
// leave room for it.
static Sound MakeTone(float freq, int punchy) {
    int len = SR / 7;
    short* d = (short*)MemAlloc(len * sizeof(short));
    float phase = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        float f = punchy ? freq * (1.0f + 0.35f * expf(-t * 60.0f)) : freq;
        phase += 2.0f * PI * f / SR;

        float env = expf(-t * (punchy ? 24.0f : 30.0f));
        float s = sinf(phase);
        if (punchy) {
            // Pulled well back (2026-09-04). This fires on every hit, so it is
            // the sound the player hears most, and it was loud enough to be
            // the thing they heard INSTEAD of the music. It only has to say
            // "that connected"; the PERFECT chime carries the reward.
            s += 0.20f * sinf(phase * 2.0f);
            s += VfxSigned() * expf(-t * 140.0f) * 0.22f;   // an edge, not a strike
            s += sinf(2.0f * PI * 92.0f * t) * expf(-t * 42.0f) * 0.30f;
        }

        float v = s * env * (punchy ? 5800.0f : 8000.0f);
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }

    Wave w = { len, SR, 16, 1, d };
    Sound snd = LoadSoundFromWave(w);
    MemFree(d);
    return snd;
}


#if PERFECT_SOUND <= 4
static Sound MakePerfect(void) {
    int len = SR / 8;                        // ~125ms
    short* d = (short*)MemAlloc(len * sizeof(short));

#if PERFECT_SOUND == 0
    // -- RISE: two notes a fifth apart -------------------------------------
    float p1 = 0.0f, p2 = 0.0f, q1 = 0.0f, q2 = 0.0f;
    const float fA = 659.3f, fB = 987.8f, gap = 0.045f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        p1 += 2.0f * PI * fA / SR;
        p2 += 2.0f * PI * fA * 2.0f / SR;
        float aA = (t < 0.004f) ? t / 0.004f : 1.0f;
        float s = (sinf(p1) + 0.22f * sinf(p2)) * aA * expf(-t * 30.0f);
        if (t >= gap) {
            float u = t - gap;
            q1 += 2.0f * PI * fB / SR;
            q2 += 2.0f * PI * fB * 2.0f / SR;
            float aB = (u < 0.004f) ? u / 0.004f : 1.0f;
            s += (sinf(q1) + 0.20f * sinf(q2)) * aB * expf(-u * 26.0f);
        }
        float v = s * 6600.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }

#elif PERFECT_SOUND == 1
    // -- CHORD: a struck major triad ---------------------------------------
    // Three notes at once rather than one after another. What the earlier
    // attempts lacked was body, not brightness, and a triad is the cheapest
    // way to say "that was right" with weight behind it. The octave on top
    // decays fastest, so it reads as a strike and then a chord ringing.
    float p[4] = { 0 };
    const float f[4] = { 523.25f, 659.26f, 783.99f, 1046.5f };  // C5 E5 G5 C6
    const float amp[4] = { 1.00f, 0.85f, 0.70f, 0.30f };
    const float dec[4] = { 18.0f, 20.0f, 22.0f, 40.0f };
    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        float s = 0.0f;
        for (int k = 0; k < 4; k++) {
            p[k] += 2.0f * PI * f[k] / SR;
            s += sinf(p[k]) * amp[k] * expf(-t * dec[k]);
        }
        float atk = (t < 0.005f) ? t / 0.005f : 1.0f;
        float v = s * atk * 4900.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }

#elif PERFECT_SOUND == 2
    // -- GLIDE: one note sliding up ----------------------------------------
    // The pitch moves continuously instead of stepping, which nothing else in
    // the game does. Exponential in pitch, so it reads as one gesture rather
    // than a sweep through a scale.
    float ph = 0.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        float k = (t < 0.055f) ? t / 0.055f : 1.0f;
        float fr = 620.0f * powf(1300.0f / 620.0f, k);
        ph += 2.0f * PI * fr / SR;
        float atk = (t < 0.004f) ? t / 0.004f : 1.0f;
        float s = (sinf(ph) + 0.25f * sinf(ph * 2.0f)) * atk * expf(-t * 24.0f);
        float v = s * 7400.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }

#elif PERFECT_SOUND == 4
    // -- RISE+: the rising figure, harmonised -------------------------------
    // 0's shape with 1's weight. The two notes stay E5 -> B5, but each is a
    // chord instead of a single line, so the figure keeps the movement that
    // made it read as "good" and gains the body that a bare sine pair lacked.
    //
    // Nothing above 1480Hz, and that partial is the quietest: 1244Hz on its
    // own was the sound described as 띡띡거린다, so the top of this is kept
    // deliberately thin.
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float b1 = 0.0f, b2 = 0.0f, b3 = 0.0f;
    const float gap = 0.045f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        a1 += 2.0f * PI * 659.26f / SR;      // E5
        a2 += 2.0f * PI * 830.61f / SR;      // G#5
        a3 += 2.0f * PI * 987.77f / SR;      // B5
        float aA = (t < 0.004f) ? t / 0.004f : 1.0f;
        float s = (sinf(a1)
                 + sinf(a2) * 0.55f
                 + sinf(a3) * 0.45f) * aA * expf(-t * 28.0f);
        if (t >= gap) {
            float u = t - gap;
            b1 += 2.0f * PI * 987.77f / SR;  // B5
            b2 += 2.0f * PI * 1244.51f / SR; // D#6
            b3 += 2.0f * PI * 1479.98f / SR; // F#6
            float aB = (u < 0.004f) ? u / 0.004f : 1.0f;
            s += (sinf(b1)
                + sinf(b2) * 0.40f
                + sinf(b3) * 0.25f) * aB * expf(-u * 24.0f);
        }
        float v = s * 5800.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }

#elif PERFECT_SOUND == 3
    // -- WOOD: a low inharmonic pluck --------------------------------------
    // Out of the register everything else occupies. The 2.76 partial is not a
    // harmonic, which is what makes struck wood sound like wood rather than
    // like a string.
    float p1 = 0.0f, p2 = 0.0f;
    const float f1 = 330.0f;
    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        p1 += 2.0f * PI * f1 / SR;
        p2 += 2.0f * PI * f1 * 2.76f / SR;
        float s = sinf(p1) * expf(-t * 26.0f)
                + sinf(p2) * 0.45f * expf(-t * 55.0f);
        float atk = (t < 0.002f) ? t / 0.002f : 1.0f;
        float v = s * atk * 7800.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }
#endif

    Wave w = { len, SR, 16, 1, d };
    Sound snd = LoadSoundFromWave(w);
    MemFree(d);
    return snd;
}
#endif  /* PERFECT_SOUND <= 4 */

#if PERFECT_SOUND == 5
// -- PERFECT_SOUND 5: the lane's own note, opened out --------------------
//
// Every other option is one sound for every lane. This one is four, built on
// the same pitches the lane already speaks in (laneFreq), so a Perfect does not
// introduce a new note - it makes the note that was already there fuller.
//
// The brightness is held constant across the four, which is the whole trick.
// A first version simply stacked a fifth, an octave and a sub on the lane
// frequency; the top lane (880Hz) was right and the other three were not, worst
// at the left lane, because both the pitch AND the partials above it fall as
// the lane goes down - 440Hz with a 220Hz sub under it is a thud, not a reward.
//
// So the sub is gone, and the octave and twelfth are weighted by how far the
// lane sits below the top one:
//
//     k = 880 / f     up 1.00   right 1.34   down 1.68   left 2.00
//
// At the left lane the octave (880Hz) ends up louder than the fundamental, so
// it sings where the top lane sings while keeping 440Hz underneath as body.
// The top lane is left exactly as it was, since that one was already right.
static Sound MakePerfectLane(float f) {
    int len = SR / 7;                        // ~140ms, longer than the shot
    short* d = (short*)MemAlloc(len * sizeof(short));
    float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;

    float k = 880.0f / f;
    if (k > 2.0f) k = 2.0f;
    if (k < 1.0f) k = 1.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        p1 += 2.0f * PI * f / SR;
        p2 += 2.0f * PI * (f * 2.0f) / SR;   // octave
        p3 += 2.0f * PI * (f * 3.0f) / SR;   // twelfth

        float s = sinf(p1) * 1.00f * expf(-t * 13.0f)
                + sinf(p2) * 0.55f * k * expf(-t * 17.0f)
                + sinf(p3) * 0.26f * k * expf(-t * 23.0f);

        // Slow enough that the shot's click stays the attack. This is the
        // bloom behind it, not a second strike.
        float atk = (t < 0.008f) ? t / 0.008f : 1.0f;

        // Boosting the partials also raises the level, and the top lane was
        // already the one that sounded right - so divide it back out. Every
        // lane lands at the same loudness; only the brightness moves.
        // The 0.14 term is the tail: boosted partials decay faster, so without
        // it the low lanes came out about a decibel under the top one.
        float norm = 1.81f * (1.0f + 0.14f * (k - 1.0f)) / (1.0f + 0.81f * k);

        float v = s * atk * 5100.0f * norm;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }

    Wave w = { len, SR, 16, 1, d };
    Sound snd = LoadSoundFromWave(w);
    MemFree(d);
    return snd;
}
#endif

// -- PERFECT_SOUND 6: a transient, not a note ---------------------------
//
// Every option before this one layers a PITCH on the shot, and six passes of
// choosing a better pitch all came back "not it". So this one adds no pitch at
// all: a two-millisecond bright crack that makes the same shot land harder.
//
// The premise is that what a Perfect should add is IMPACT, not melody - the
// reward being in the hit rather than beside it.
#if PERFECT_SOUND == 6
static Sound MakePerfectSnap(void) {
    int len = SR / 32;                       // ~31ms, gone before it can sing
    short* d = (short*)MemAlloc(len * sizeof(short));
    float hp1 = 0.0f, prev = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        float n = VfxSigned();
        // One-pole high pass: what is left is the crack, with none of the body
        // the shot already provides.
        hp1 = 0.72f * (hp1 + n - prev);
        prev = n;

        float env = expf(-t * 190.0f);
        float s = hp1 * 1.4f + sinf(2.0f * PI * 2600.0f * t) * 0.35f * env;

        float v = s * env * 11000.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }
    Wave w = { len, SR, 16, 1, d };
    Sound snd = LoadSoundFromWave(w);
    MemFree(d);
    return snd;
}
#endif

// -- PERFECT_SOUND 7: take something away from GOOD ---------------------
//
// Subtractive. A Perfect adds nothing; a GOOD gets a DULLED shot instead, so
// the difference arrives as the full sound versus a muffled one.
//
// Worth trying because every other option makes an already busy mix busier,
// and the ear notices what is missing as readily as what is added. A Perfect
// then sounds like the game's own hit, clean - which is the one thing none of
// the added-tone versions could be.
#if PERFECT_SOUND == 7
static Sound MakeToneDull(float freq) {
    int len = SR / 7;
    short* d = (short*)MemAlloc(len * sizeof(short));
    float phase = 0.0f, lp = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        float f = freq * (1.0f + 0.35f * expf(-t * 60.0f));
        phase += 2.0f * PI * f / SR;

        float env = expf(-t * 30.0f);        // shorter tail than the real one
        float s = sinf(phase) + 0.10f * sinf(phase * 2.0f);
        s += sinf(2.0f * PI * 92.0f * t) * expf(-t * 42.0f) * 0.30f;

        // One-pole low pass around 1.1kHz: the click goes, the note stays.
        lp += (s - lp) * 0.15f;

        float v = lp * env * 5200.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }
    Wave w = { len, SR, 16, 1, d };
    Sound snd = LoadSoundFromWave(w);
    MemFree(d);
    return snd;
}
#endif

// -- PERFECT_SOUND 8: the streak, not the hit ---------------------------
//
// The pitch climbs with the combo and resets when it breaks, so a run of
// Perfects builds instead of repeating. Six steps up a pentatonic scale, held
// at the top - a scale with no semitones in it, so any two steps landing close
// together still agree.
//
// Different axis again: this rewards the RUN. A single Perfect after a miss
// sounds modest on purpose; the twentieth in a row is the payoff.
#if PERFECT_SOUND == 8
static Sound MakePerfectStep(int step) {
    static const float scale[COMBO_STEPS] =
        { 659.3f, 784.0f, 880.0f, 1046.5f, 1174.7f, 1318.5f };   // E G A C D E
    float f = scale[step];
    int len = SR / 10;                       // ~100ms
    short* d = (short*)MemAlloc(len * sizeof(short));
    float p1 = 0.0f, p2 = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        p1 += 2.0f * PI * f / SR;
        p2 += 2.0f * PI * f * 2.0f / SR;
        float atk = (t < 0.003f) ? t / 0.003f : 1.0f;
        float s = (sinf(p1) + 0.30f * sinf(p2)) * atk * expf(-t * 26.0f);
        float v = s * 7200.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }
    Wave w = { len, SR, 16, 1, d };
    Sound snd = LoadSoundFromWave(w);
    MemFree(d);
    return snd;
}
#endif

// -- PERFECT_SOUND 9: the crack from 6, with the pitches from 8 ----------
//
// Both halves are things that were actually liked rather than guessed at.
// 6 (a transient, no pitch) came back "much better than 0". 8 was rejected as
// a mechanic - the climbing was not wanted - but two of its rungs were singled
// out: the one at 659.3Hz and the one at 880Hz. The rungs in between and above
// were not. Those two are E5 and A5, a perfect fourth, and both are already
// lane pitches; 784Hz, the one disliked, is the rung that is not.
//
// So: 6's crack for the impact, and those two notes together, fixed, for the
// tone. No climbing.
#if PERFECT_SOUND == 9
static Sound MakePerfectSnapTone(void) {
    int len = SR / 9;                        // ~110ms: the crack, then the note
    short* d = (short*)MemAlloc(len * sizeof(short));
    float hp1 = 0.0f, prev = 0.0f;
    float a1 = 0.0f, a2 = 0.0f, b1 = 0.0f, b2 = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;

        // 6's transient, unchanged.
        float n = VfxSigned();
        hp1 = 0.72f * (hp1 + n - prev);
        prev = n;
        float ce = expf(-t * 190.0f);
        float crack = (hp1 * 1.4f + sinf(2.0f * PI * 2600.0f * t) * 0.35f * ce) * ce;

        // E5 and A5, each with its octave, the way 8 built its rungs.
        a1 += 2.0f * PI * 659.3f / SR;
        a2 += 2.0f * PI * 659.3f * 2.0f / SR;
        b1 += 2.0f * PI * 880.0f / SR;
        b2 += 2.0f * PI * 880.0f * 2.0f / SR;
        float atk = (t < 0.004f) ? t / 0.004f : 1.0f;
        float tone = ((sinf(a1) + 0.30f * sinf(a2)) * 0.80f
                    + (sinf(b1) + 0.30f * sinf(b2)) * 0.80f)
                    * atk * expf(-t * 26.0f);

        float v = crack * 9000.0f + tone * 3400.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }
    Wave w = { len, SR, 16, 1, d };
    Sound snd = LoadSoundFromWave(w);
    MemFree(d);
    return snd;
}
#endif

// -- PERFECT_SOUND 10: only the two notes -------------------------------
//
// 9 without the crack, to separate the two variables. If 9 is right it is
// worth knowing whether the transient is carrying it or the pitches are.
//
// To hear one note instead of the pair, set the other's 0.80f to 0.
#if PERFECT_SOUND == 10
static Sound MakePerfectFourth(void) {
    int len = SR / 9;
    short* d = (short*)MemAlloc(len * sizeof(short));
    float a1 = 0.0f, a2 = 0.0f, b1 = 0.0f, b2 = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        a1 += 2.0f * PI * 659.3f / SR;       // E5
        a2 += 2.0f * PI * 659.3f * 2.0f / SR;
        b1 += 2.0f * PI * 880.0f / SR;       // A5
        b2 += 2.0f * PI * 880.0f * 2.0f / SR;
        float atk = (t < 0.003f) ? t / 0.003f : 1.0f;
        float s = ((sinf(a1) + 0.30f * sinf(a2)) * 0.80f
                 + (sinf(b1) + 0.30f * sinf(b2)) * 0.80f)
                 * atk * expf(-t * 26.0f);
        float v = s * 5000.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }
    Wave w = { len, SR, 16, 1, d };
    Sound snd = LoadSoundFromWave(w);
    MemFree(d);
    return snd;
}
#endif

// Damage: low, detuned and slow to decay. Deliberately the opposite shape of
// a hit tone so a mistake never gets mistaken for a success.
static Sound MakeDamage(void) {
    int len = SR / 3;
    short* d = (short*)MemAlloc(len * sizeof(short));
    float phase = 0.0f, phase2 = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        float f = 55.0f + 190.0f * expf(-t * 7.0f);
        phase += 2.0f * PI * f / SR;
        phase2 += 2.0f * PI * f * 1.48f / SR;          // detuned partial: sour
        float env = expf(-t * 5.0f);

        float s = sinf(phase) + 0.55f * sinf(phase2);
        s += VfxSigned() * 0.3f * expf(-t * 16.0f);

        float v = s * env *  8000.0f;   // pulled down: a missed note already costs health,
                                     // and the noise on top of it was drowning the
                                     // bar that follows
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }

    Wave w = { len, SR, 16, 1, d };
    Sound s = LoadSoundFromWave(w);
    MemFree(d);
    return s;
}

// Level up: a rising arpeggio. Going up is the whole point of the sound.
static Sound MakeLevelUp(void) {
    int len = SR / 2;
    short* d = (short*)MemAlloc(len * sizeof(short));
    const float steps[4] = { 523.0f, 659.0f, 784.0f, 1047.0f };
    float phase = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        int step = (int)(t / 0.075f);
        if (step > 3) step = 3;
        float local = t - step * 0.075f;

        phase += 2.0f * PI * steps[step] / SR;
        float env = expf(-local * 14.0f) * expf(-t * 1.5f);
        float s = sinf(phase) + 0.3f * sinf(phase * 2.0f);

        float v = s * env * 12000.0f;
        if (v > 32000.0f) v = 32000.0f;
        if (v < -32000.0f) v = -32000.0f;
        d[i] = (short)v;
    }

    Wave w = { len, SR, 16, 1, d };
    Sound s = LoadSoundFromWave(w);
    MemFree(d);
    return s;
}

// Hi-hat: filtered noise, very short. Cheapest possible sense of a groove.
static Sound MakeHat(void) {
    int len = SR / 28;
    short* d = (short*)MemAlloc(len * sizeof(short));
    float prev = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        float n = VfxSigned();
        float hp = n - prev;                  // crude high-pass: keeps it thin
        prev = n;
        float env = expf(-t * 110.0f);
        d[i] = (short)(hp * env * 5200.0f);
    }

    Wave w = { len, SR, 16, 1, d };
    Sound s = LoadSoundFromWave(w);
    MemFree(d);
    return s;
}

static Sound MakeBass(float freq) {
    int len = SR / 3;
    short* d = (short*)MemAlloc(len * sizeof(short));
    float phase = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        phase += 2.0f * PI * freq / SR;
        float env = expf(-t * 4.0f);
        float s = sinf(phase) + 0.25f * sinf(phase * 2.0f);
        d[i] = (short)(s * env * 9000.0f);
    }

    Wave w = { len, SR, 16, 1, d };
    Sound s = LoadSoundFromWave(w);
    MemFree(d);
    return s;
}

static Sound MakeArp(float freq) {
    int len = SR / 9;
    short* d = (short*)MemAlloc(len * sizeof(short));
    float phase = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        phase += 2.0f * PI * freq / SR;
        float env = expf(-t * 26.0f);
        float s = sinf(phase) + 0.2f * sinf(phase * 3.0f);
        d[i] = (short)(s * env * 5000.0f);
    }

    Wave w = { len, SR, 16, 1, d };
    Sound s = LoadSoundFromWave(w);
    MemFree(d);
    return s;
}

// Twist warning: a short upward noise sweep, unlike anything else in the mix.

// A long, soft tone for the menu loops. Every in-game sound is a short pluck -
// right for rhythm, wrong for a background. At menu tempo those plucks leave
// most of the bar silent, which reads as stuttering rather than as music. This
// one sustains across the gaps so the loop is continuous underneath them.
static Sound MakePad(float freq) {
    int len = (SR * 8) / 5;                 // 1.6s
    short* d = (short*)MemAlloc(len * sizeof(short));
    float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        p1 += 2.0f * PI * freq / SR;
        p2 += 2.0f * PI * (freq * 1.5f) / SR;      // a fifth above
        p3 += 2.0f * PI * (freq * 2.01f) / SR;     // octave, detuned so it drifts

        // Slow in, slow out. The attack stops it clicking when retriggered and
        // the long tail is the part that actually fills the silence.
        float atk = t / 0.22f;
        if (atk > 1.0f) atk = 1.0f;
        float env = atk * expf(-t * 1.2f);

        float s = sinf(p1) + 0.5f * sinf(p2) + 0.3f * sinf(p3);
        d[i] = (short)(s * env * 2500.0f);
    }

    Wave w = { len, SR, 16, 1, d };
    Sound s = LoadSoundFromWave(w);
    MemFree(d);
    return s;
}

// Refusal. A short falling square-wave buzz - deliberately unmusical, because
// it has to read as "no" against a screen that is otherwise playing in key.
static Sound MakeDeny(void) {
    int len = SR / 7;
    short* d = (short*)MemAlloc(len * sizeof(short));
    float phase = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        float f = 200.0f - 70.0f * (t * 7.0f);     // falls across its own length
        phase += 2.0f * PI * f / SR;
        float env = expf(-t * 15.0f);
        float s = (sinf(phase) > 0.0f) ? 1.0f : -1.0f;
        d[i] = (short)(s * env * 2600.0f);
    }

    Wave w = { len, SR, 16, 1, d };
    Sound s = LoadSoundFromWave(w);
    MemFree(d);
    return s;
}

static Sound MakeTwist(void) {
    int len = SR / 5;
    short* d = (short*)MemAlloc(len * sizeof(short));
    float phase = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        float f = 300.0f + 1400.0f * (t / (float)(len / (float)SR));
        phase += 2.0f * PI * f / SR;
        float env = expf(-t * 9.0f);
        float s = sinf(phase) * 0.6f + VfxSigned() * 0.4f;
        d[i] = (short)(s * env * 11000.0f);
    }

    Wave w = { len, SR, 16, 1, d };
    Sound s = LoadSoundFromWave(w);
    MemFree(d);
    return s;
}

static Sound MakeKick(void) {
    int len = SR / 6;
    short* d = (short*)MemAlloc(len * sizeof(short));
    float phase = 0.0f;

    for (int i = 0; i < len; i++) {
        float t = (float)i / SR;
        float f = 40.0f + 120.0f * expf(-t * 30.0f);
        phase += 2.0f * PI * f / SR;
        float env = expf(-t * 12.0f);
        d[i] = (short)(sinf(phase) * env * 18000.0f);
    }

    Wave w = { len, SR, 16, 1, d };
    Sound s = LoadSoundFromWave(w);
    MemFree(d);
    return s;
}

// ---- cards ---------------------------------------------------------------
// Every card is a change to numbers the game already reads. Nothing here needs
// new machinery - the whole system is `Mods` plus a screen that hands the
// player three of them.
//
// `needs` and `kills` keep the deck honest. A card that boosts SWAP must not
// be offered after SWAP has been removed from the run, and a card that removes
// SWAP must not be offered to someone already building around it. `group`
// covers plain mutual exclusion, which is what the four "every bar" cards need.
#define GB_SWAP  1
#define GB_BLIND 2
#define GB_TRAP  4

// Description lines are ordered benefit first, cost second, always. A draft
// gives the player a few seconds to compare three cards, and a fixed slot for
// each kind of information is what makes that possible - the eye learns where
// to look instead of reading every line. The draw code colours line 1 warm and
// the rest cold to reinforce it.
//
// Lines run benefits first, then costs. `extraBenefit` says how many lines
// past the first are still benefits, which is 0 for all but two cards - the
// default therefore needs no entry in the table, and the exceptions are the
// only rows that say anything about it.
//
// One effect per line, never two. Two effects sharing a line was how the old
// table fitted, and it stopped fitting the moment the text became Korean:
// MAELSTROM's pair measured 262px against the 248px inside a draft card, so it
// drew straight through the border. Four short lines fit where two long ones
// did not, and the widest line in the deck now clears the box by 15px.
typedef struct {
    const char* name;
    const char* d1;
    const char* d2;
    const char* d3;                // NULL when the card needs fewer lines
    const char* d4;
    void (*apply)(Mods* m);
    unsigned char group;           // nonzero: only one card of this group per run
    unsigned char needs;           // gimmick bits this card is worthless without
    unsigned char kills;           // gimmick bits this card takes out of the run
    unsigned char extraBenefit;    // benefit lines after d1
    // One-shot effects fire in TakeCard, not in `apply`. `apply` only ever
    // touches `mods`, and these touch the run itself - `hp`, `burst` and the
    // barrier count are declared further down the file, and the ordering is
    // not worth breaking to save a branch.
    //
    //   1  heal to full          2  fill the burst meter
    //   4  spend 10% max health  8  spend 30% of the meter
    //  16  spend 50% max health 32  spend the whole meter
    //  64  +1 barrier          128  +4 barriers
    // 256  spend 30% max health
    // 512  spend 50% of the meter
    //1024  +2 barriers
    //
    // Widened from unsigned char when the eighth bit ran out, which is what the
    // note here said to do rather than inventing a cleverer encoding. The costs
    // are no longer orderable by bit value - 4 is 10% and 256 is 30% - so this
    // table is the only place that says what they mean. If a fourth health cost
    // shows up, that is the signal to replace the whole field with three numbers
    // (percent health, percent meter, barriers) and stop counting bits.
    unsigned short instant;

    // What the shop charges for it, 1..5. 3 is the baseline; 1 is a card that
    // barely earns its slot, 5 is one that plainly beats its own cost. Most of
    // the deck sits at 2 and 3 on purpose - a shop where everything is a
    // bargain has no decision in it.
    unsigned char price;

    // Not offered in Normal. Normal never shows SWAP or BLIND, so a card that
    // needs, removes or forces either one is dead weight there - or worse,
    // lies: the "a gimmick every bar" card rolls its gimmick from gimMul[]
    // with no mode check at all, so in Normal it would produce the two
    // gimmicks Normal is built never to show.
    //
    // WHIRL is on the list by hand rather than by gimmick bits. The rotating
    // ring is a difficulty of its own and does not belong in the easy mode.
    unsigned char hardOnly;
} Card;

// The card table below is in codex order, and so is this block: the two read
// top to bottom together. Grouped by what the card trades, not alphabetically -
// a deck is built by looking for "another one of these", never by name.
//
// Burst score is the one axis that needs a rule, because it MULTIPLIES between
// cards while its value is gated by burst uptime. Uptime with no charge cards
// is about 6.7% of the bar, so a x4 burst-score card alone is worth roughly a
// x1.2 score card - deliberately unexciting on its own. Stacked with charge
// cards the same x4 becomes enormous. The tiers are therefore set by what ELSE
// the card gives:
//
//   x4.0   burst score is the only benefit (OVERFLOW, PASSWORD, COMPLACENT)
//   x3.0   comes with burst charge         (VOLCANO, KINDLING, RAPID FIRE, HUSH)
//   x3.5   comes with charge, gimmick cost (MINDS EYE)
//   x2.5   comes with a score multiplier   (AWAKENING, CALAMITY)
//   x2.0   comes with score AND charge     (WHIRL)
//
// Measured against pure score decks: 2 cards the burst build loses, 4 cards it
// draws, 6 cards it wins by about 2.4x. That is the intended shape - a build to
// commit to, not to dabble in.

// -- recovery bought with damage or charge ---------------------------------
static void CdScar(Mods* m)      { m->dmgMul *= 1.50f; m->healMul *= 3.00f; }
static void CdMend(Mods* m)      { m->healMul *= 2.00f; m->burstRate *= 0.50f; }
static void CdFurnace(Mods* m)   { m->dmgMul *= 1.50f; m->burstRate *= 1.50f; m->burstScore *= 3.00f; }
static void CdKindling(Mods* m)  { m->healMul *= 0.50f; m->burstRate *= 1.50f; m->burstScore *= 3.00f; }
static void CdPlate(Mods* m)     { m->dmgMul *= 0.75f; m->healMul *= 0.50f; }
static void CdBulwark(Mods* m)   { m->dmgMul *= 0.75f; m->burstRate *= 0.50f; }

// -- tempo bought with everything else -------------------------------------
static void CdQuickstep(Mods* m) { m->bpmMul *= 1.05f; m->burstScore *= 4.00f; }
static void CdFever(Mods* m)     { m->bpmMul *= 1.10f; m->dmgMul *= 1.10f; m->healPerfectMul *= 1.30f; m->healMul *= 1.10f; }
static void CdOverclock(Mods* m) { m->bpmMul *= 1.20f; m->healPerfectMul *= 1.60f; m->burstRate *= 1.20f; }

// -- score bought with survivability ---------------------------------------
static void CdGlass(Mods* m)     { m->scoreMul *= 1.20f; m->healMul *= 0.90f; }
static void CdWither(Mods* m)    { m->scoreMul *= 1.40f; m->dmgMul *= 1.10f; }
static void CdShatter(Mods* m)   { m->scoreMul *= 1.60f; m->burstRate *= 0.90f; }
static void CdEmber(Mods* m)     { m->scoreMul *= 2.00f; m->dmgMul *= 1.15f; m->healMul *= 0.85f; }

// -- recovery given up entirely --------------------------------------------
static void CdOverdrive(Mods* m) { m->healMul = 0.0f; m->burstRate *= 1.20f; m->burstScore *= 2.50f; m->scoreMul *= 1.50f; }
static void CdZealot(Mods* m)    { m->healGoodZero = 1; m->healPerfectMul *= 1.60f; }
static void CdPurist(Mods* m)    { m->healGoodZero = 1; m->healMul *= 1.50f; m->scoreMul *= 1.20f; }
static void CdMartyr(Mods* m)    { m->healGoodZero = 1; m->dmgMul *= 1.10f; m->healPerfectMul *= 1.90f; }
static void CdAscetic(Mods* m)   { m->healGoodZero = 1; m->dmgMul *= 1.10f; m->burstRate *= 1.50f; }

// -- PERFECT sharpened, one axis given up ----------------------------------
static void CdTemper(Mods* m)    { m->dmgMul *= 1.10f; m->healPerfectMul *= 1.60f; }
static void CdPrecision(Mods* m) { m->healPerfectMul *= 1.60f; m->burstRate *= 0.90f; }
static void CdFocus(Mods* m)     { m->healMul *= 0.80f; m->healPerfectMul *= 2.00f; }

// -- half the gauge, and the burst sealed ----------------------------------
static void CdFragile(Mods* m)   { m->hpMaxMul *= 0.50f; m->healMul *= 2.00f; m->scoreMul *= 2.00f; }
static void CdBrittle(Mods* m)   { m->hpMaxMul *= 0.50f; m->burstRate *= 1.50f; m->scoreMul *= 2.00f; }
static void CdAshes(Mods* m)     { m->burstLocked = 1; m->burstRate *= 1.30f; m->scoreMul *= 2.00f; }
static void CdVow(Mods* m)       { m->burstLocked = 1; m->healMul *= 1.20f; m->dmgMul *= 0.80f; m->scoreMul *= 1.40f; }

// -- burst duration ---------------------------------------------------------
static void CdLinger(Mods* m)    { m->burstBeats += 2.0f; m->burstRate *= 0.80f; m->scoreMul *= 0.80f; }
static void CdFlash(Mods* m)     { m->burstBeats -= 2.0f; if (m->burstBeats < 1.0f) m->burstBeats = 1.0f;
                                   m->burstRate *= 1.50f; m->burstScore *= 3.00f; }

// -- the game made easier, paid for in score -------------------------------
// Tempo and window are at their limits: the window cannot widen much before a
// sixteenth's BAD range reaches the next slot, and the tempo floor is where the
// backing stops sounding like the same song. Small numbers here are the point.
static void CdTuning(Mods* m)    { m->windowAdd += 0.010f; m->scoreMul *= 0.90f; }
static void CdDrag(Mods* m)      { m->bpmMul *= 0.975f; m->scoreMul *= 0.90f; }
static void CdLull(Mods* m)      { m->windowAdd += 0.020f; m->bpmMul *= 0.95f; m->burstRate *= 0.80f; m->scoreMul *= 0.80f; }

// -- granted at the head of each level -------------------------------------
static void CdRevive(Mods* m)    { m->levelHeal += 0.10f; m->healMul *= 0.90f; m->dmgMul *= 1.10f; }
static void CdRenew(Mods* m)     { m->levelHeal += 0.15f; m->burstRate *= 0.80f; }
static void CdPrimed(Mods* m)    { m->burstStart += 0.10f; m->burstRate *= 0.90f; }
static void CdMomentum(Mods* m)  { m->burstStart += 0.20f; m->healMul *= 0.80f; m->dmgMul *= 1.20f; }
static void CdOverhaul(Mods* m)  { m->levelBarrier = 1; m->healMul *= 0.80f; m->dmgMul *= 1.20f; }
static void CdServicing(Mods* m) { m->levelBarrier = 1; m->burstRate *= 0.80f; m->dmgMul *= 1.20f; }

// -- gimmick frequency ------------------------------------------------------
static void CdFeint(Mods* m)     { m->gimMul[0] *= 2.00f; m->healMul *= 1.20f; }
static void CdTrick(Mods* m)     { m->gimMul[0] *= 3.00f; m->healPerfectMul *= 1.60f; m->scoreMul *= 1.50f; }
static void CdVeil(Mods* m)      { m->gimMul[1] *= 2.00f; m->burstScore *= 4.00f; }
static void CdShroud(Mods* m)    { m->gimMul[1] *= 3.00f; m->burstRate *= 1.20f; }
static void CdBait(Mods* m)      { m->gimMul[2] *= 3.00f; m->scoreMul *= 1.30f; }
static void CdSnare(Mods* m)     { m->gimMul[2] *= 4.00f; m->bpmMul *= 0.975f; }
static void CdSteady(Mods* m)    { m->gimMul[0] *= 0.50f; m->healPerfectMul *= 0.80f; }
static void CdClarity(Mods* m)   { m->gimMul[1] *= 0.50f; m->burstScore *= 0.50f; }
static void CdDisarm(Mods* m)    { m->gimMul[2] *= 0.50f; m->bpmMul *= 1.05f; }

// -- gimmick removal, paid for heavily -------------------------------------
static void CdCalm(Mods* m)      { m->gimMul[0] = 0.0f; m->healMul = 0.0f; m->scoreMul *= 0.90f; }
static void CdDaylight(Mods* m)  { m->gimMul[1] = 0.0f; m->burstLocked = 1; m->scoreMul *= 0.90f; }
static void CdSweep(Mods* m)     { m->gimMul[2] = 0.0f; m->bpmMul *= 1.10f; m->scoreMul *= 0.90f; }

// -- one gimmick, every bar -------------------------------------------------
static void CdMaelstrom(Mods* m) { m->forceGim = 4; m->burstScore *= 2.50f; m->scoreMul *= 2.00f; }
static void CdAlwSwap(Mods* m)   { m->forceGim = 1; m->healMul *= 1.40f; m->dmgMul *= 0.80f; m->scoreMul *= 1.50f; }
static void CdAlwBlind(Mods* m)  { m->forceGim = 2; m->burstRate *= 1.40f; m->burstScore *= 3.50f; }
static void CdAlwTrap(Mods* m)   { m->forceGim = 3; m->bpmMul *= 0.975f; m->scoreMul *= 2.00f; }

// -- what you can see and hear ----------------------------------------------
static void CdDisguise(Mods* m)  { m->trapDisguise = 1; m->scoreMul *= 2.00f; }
static void CdFog(Mods* m)       { m->fogBeats = 3.0f; m->burstRate *= 1.50f; }
static void CdSilence(Mods* m)   { m->callVol *= 0.50f; m->healMul *= 1.20f; m->scoreMul *= 1.50f; m->dmgMul *= 0.90f; }
static void CdHush(Mods* m)      { m->callVol *= 0.50f; m->burstRate *= 1.30f; m->burstScore *= 3.00f; }
static void CdMuffle(Mods* m)    { m->warnVol *= 0.50f; m->healPerfectMul *= 2.00f; }
static void CdDampen(Mods* m)    { m->warnVol *= 0.50f; m->burstScore *= 4.00f; }
static void CdSpin(Mods* m)      { m->spin = 1; m->burstRate *= 2.00f; m->burstScore *= 2.00f; m->scoreMul *= 3.00f; }

// -- one-shot, and paid once at the moment they are taken -------------------
// The barrier itself is granted in TakeCard, not here - `apply` may only touch
// `mods`, and a charge spent during play does not belong in the deck's
// description. Both barrier cards are otherwise empty, which is the point:
// what they cost is paid once, when they are taken.
static void CdSalve(Mods* m)     { (void)m; }
static void CdIgnition(Mods* m)  { (void)m; }
static void CdRenewal(Mods* m)   { (void)m; }
static void CdTransfuse(Mods* m) { m->healMul *= 1.20f; }
static void CdDischarge(Mods* m) { m->burstRate *= 1.20f; }
static void CdHubris(Mods* m)    { m->dmgMul *= 2.00f; }
static void CdSurge(Mods* m)     { m->barrierOnBurst = 1; m->burstRate *= 1.50f; m->dmgMul *= 1.20f; }
static void CdFightGame(Mods* m) { m->barrierCombo = 50; }
static void CdRefit(Mods* m)     { m->barrierRefit = 2; m->dmgMul *= 1.10f; m->healMul *= 0.90f; }
static void CdTriage(Mods* m)    { m->barrierTriage = 1; m->burstRate *= 0.80f; m->burstScore *= 0.50f; }
static void CdBarrier(Mods* m)   { (void)m; }
static void CdMetaphor(Mods* m)  { m->itemPct = 5; }

// -- paid out at the end, not per note --------------------------------------
static void CdTithe(Mods* m)     { m->rewardMul *= 1.50f; }
static void CdLedger(Mods* m)    { m->endScoreMul *= 1.10f; }

// Fields: name, d1..d4, apply, group, needs, kills, extraBenefit, instant, price.
// Lines run benefits first then costs; `extraBenefit` says how many lines after
// the first are still benefits. A trailing 1 marks a card Normal never drafts.
//
// The order here IS the codex order, and it matches the apply block above.
static const Card cards[] = {
    // -- recovery bought with damage or charge ------------------------------
    { "BATTERED",  "recovery x3", "damage taken x1.5", 0, 0, CdScar, 0,0,0, 0, 0, 3 },
    { "REPAIR",    "recovery x2", "burst charge x0.5", 0, 0, CdMend, 0,0,0, 0, 0, 3 },
    { "VOLCANO",   "burst charge x1.5", "burst score x3", "damage taken x1.5", 0, CdFurnace, 0,0,0, 1, 0, 3 },
    { "KINDLING",  "burst charge x1.5", "burst score x3", "recovery x0.5", 0, CdKindling, 0,0,0, 1, 0, 3 },
    { "PLATE",     "damage taken x0.75", "recovery x0.5", 0, 0, CdPlate, 0,0,0, 0, 0, 3 },
    { "SEAWALL",   "damage taken x0.75", "burst charge x0.5", 0, 0, CdBulwark, 0,0,0, 0, 0, 3 },

    // -- tempo bought with everything else ----------------------------------
    { "OVERFLOW",  "burst score x4", "tempo +5%", 0, 0, CdQuickstep, 0,0,0, 0, 0, 3 },
    { "OVERDRIVE", "PERFECT recovery x1.3", "recovery x1.1", "tempo +10%", "damage taken x1.1", CdFever, 0,0,0, 1, 0, 2 },
    { "OVERLOAD",  "PERFECT recovery x1.6", "burst charge x1.2", "tempo +20%", 0, CdOverclock, 0,0,0, 1, 0, 1 },

    // -- score bought with survivability ------------------------------------
    { "A FORTUNE MADE",   "score x1.2", "recovery x0.9", 0, 0, CdGlass, 0,0,0, 0, 0, 1 },
    { "A FORTUNE GROWN",  "score x1.4", "damage taken x1.1", 0, 0, CdWither, 0,0,0, 0, 0, 1 },
    { "A FORTUNE RISKED", "score x1.6", "burst charge x0.9", 0, 0, CdShatter, 0,0,0, 0, 0, 1 },
    { "A FORTUNE LOST",   "score x2", "damage taken x1.15", "recovery x0.85", 0, CdEmber, 0,0,0, 0, 0, 1 },

    // -- recovery given up entirely -----------------------------------------
    { "AWAKENING",     "burst charge x1.2", "burst score x2.5", "score x1.5", "no recovery", CdOverdrive, 0,0,0, 2, 0, 1 },
    { "CRACK SHOT",    "PERFECT recovery x1.6", "GOOD never heals", 0, 0, CdZealot, 0,0,0, 0, 0, 3 },
    { "DIVINE ARCHER", "recovery x1.5", "score x1.2", "GOOD never heals", 0, CdPurist, 0,0,0, 1, 0, 3 },
    { "DEAD EYE",      "PERFECT recovery x1.9", "GOOD never heals", "damage taken x1.1", 0, CdMartyr, 0,0,0, 0, 0, 3 },
    { "SHARPSHOOTER",  "burst charge x1.5", "GOOD never heals", "damage taken x1.1", 0, CdAscetic, 0,0,0, 0, 0, 3 },

    // -- PERFECT sharpened, one axis given up -------------------------------
    { "SHARPER", "PERFECT recovery x1.6", "damage taken x1.1", 0, 0, CdTemper, 0,0,0, 0, 0, 3 },
    { "FINER",   "PERFECT recovery x1.6", "burst charge x0.9", 0, 0, CdPrecision, 0,0,0, 0, 0, 3 },
    { "TRUER",   "PERFECT recovery x2", "recovery x0.8", 0, 0, CdFocus, 0,0,0, 0, 0, 3 },

    // -- half the gauge, and the burst sealed -------------------------------
    { "FRAGILE",    "recovery x2", "score x2", "half the maximum gauge", 0, CdFragile, 0,0,0, 1, 0, 2 },
    { "UNDERRATED", "burst charge x1.5", "score x2", "half the maximum gauge", 0, CdBrittle, 0,0,0, 1, 0, 2 },
    { "GAMBIT",     "burst charge x1.3", "score x2", "the burst can never be fired", 0, CdAshes, 0,0,0, 1, 0, 4 },
    { "SKEWER",     "recovery x1.2", "damage taken x0.8", "score x1.4", "the burst can never be fired", CdVow, 0,0,0, 2, 0, 4 },

    // -- burst duration -----------------------------------------------------
    { "SLOW FIRE",  "burst lasts 2 beats longer", "burst charge x0.8", "score x0.8", 0, CdLinger, 0,0,0, 0, 0, 1 },
    { "RAPID FIRE", "burst charge x1.5", "burst score x3", "burst lasts 2 beats less", 0, CdFlash, 0,0,0, 1, 0, 3 },

    // -- the game made easier, paid for in score ----------------------------
    { "MUSICAL CHAIRS", "window +0.01", "score x0.9", 0, 0, CdTuning, 0,0,0, 0, 0, 4 },
    { "TURTLE WALTZ",   "tempo -2.5%", "score x0.9", 0, 0, CdDrag, 0,0,0, 0, 0, 4 },
    { "EASY MODE",      "window +0.02", "tempo -5%", "burst charge x0.8", "score x0.8", CdLull, 0,0,0, 1, 0, 5 },

    // -- granted at the head of each level ----------------------------------
    { "GROWING PAINS", "+10% gauge on level up", "recovery x0.9", "damage taken x1.1", 0, CdRevive, 0,0,0, 0, 0, 3 },
    { "GROWTH",        "+15% gauge on level up", "burst charge x0.8", 0, 0, CdRenew, 0,0,0, 0, 0, 4 },
    { "THRUST",        "+10% burst on level up", "burst charge x0.9", 0, 0, CdPrimed, 0,0,0, 0, 0, 3 },
    { "ACCELERATE",    "+20% burst on level up", "recovery x0.8", "damage taken x1.2", 0, CdMomentum, 0,0,0, 0, 0, 4 },
    { "MAINTENANCE",   "+1 barrier on level up (max 2)", "recovery x0.8", "damage taken x1.2", 0, CdOverhaul, 2,0,0, 0, 0, 3 },
    { "SERVICING",     "+1 barrier on level up (max 2)", "burst charge x0.8", "damage taken x1.2", 0, CdServicing, 2,0,0, 0, 0, 3 },

    // -- gimmick frequency --------------------------------------------------
    { "DECEPTION",    "recovery x1.2", "SWAP chance x2", 0, 0, CdFeint, 0, GB_SWAP, 0, 0, 0, 3 , 1 },
    { "STRATAGEM",    "PERFECT recovery x1.6", "score x1.5", "SWAP chance x3", 0, CdTrick, 0, GB_SWAP, 0, 1, 0, 4 , 1 },
    { "PASSWORD",     "burst score x4", "BLIND chance x2", 0, 0, CdVeil, 0, GB_BLIND, 0, 0, 0, 3 , 1 },
    { "ENCRYPTION",   "burst charge x1.2", "BLIND chance x3", 0, 0, CdShroud, 0, GB_BLIND, 0, 0, 0, 4 , 1 },
    { "BAIT",         "score x1.3", "TRAPS chance x3", 0, 0, CdBait, 0, GB_TRAP, 0, 0, 0, 3 },
    { "SNARE",        "tempo -2.5%", "TRAPS chance x4", 0, 0, CdSnare, 0, GB_TRAP, 0, 0, 0, 4 },
    { "COUNTER",      "SWAP chance x0.5", "PERFECT recovery x0.8", 0, 0, CdSteady, 0, GB_SWAP, 0, 0, 0, 2 , 1 },
    { "BREAKTHROUGH", "BLIND chance x0.5", "burst score x0.5", 0, 0, CdClarity, 0, GB_BLIND, 0, 0, 0, 2 , 1 },
    { "DISMANTLE",    "TRAPS chance x0.5", "tempo +5%", 0, 0, CdDisarm, 0, GB_TRAP, 0, 0, 0, 2 },

    // -- gimmick removal, paid for heavily ----------------------------------
    { "YOUR NAME",      "no SWAP", "no recovery", "score x0.9", 0, CdCalm, 0, 0, GB_SWAP, 0, 0, 1 , 1 },
    { "FUTURE DIARY",   "no BLIND", "the burst can never be fired", "score x0.9", 0, CdDaylight, 0, 0, GB_BLIND, 0, 0, 1 , 1 },
    { "DUNGEON KEEPER", "no TRAPS", "tempo +10%", "score x0.9", 0, CdSweep, 0, 0, GB_TRAP, 0, 0, 1 },

    // -- one gimmick, every bar ---------------------------------------------
    { "CALAMITY",    "score x2", "burst score x2.5", "a gimmick every bar", 0, CdMaelstrom, 1,
                     (GB_SWAP | GB_BLIND | GB_TRAP), 0, 1, 0, 2 , 1 },
    { "U-TURN",      "recovery x1.4", "damage taken x0.8", "score x1.5", "SWAP every bar", CdAlwSwap, 1, GB_SWAP, 0, 2, 0, 2 , 1 },
    { "MINDS EYE",   "burst charge x1.4", "burst score x3.5", "BLIND every bar", 0, CdAlwBlind, 1, GB_BLIND, 0, 1, 0, 2 , 1 },
    { "POISON TACK", "score x2", "tempo -2.5%", "TRAPS every bar", 0, CdAlwTrap, 1, GB_TRAP, 0, 1, 0, 2 },

    // -- what you can see and hear ------------------------------------------
    { "CAMOUFLAGE", "score x2", "traps wear lane colours", 0, 0, CdDisguise, 0, GB_TRAP, 0, 0, 0, 2 },
    { "NIGHT FOG",  "burst charge x1.5", "monsters are seen later", 0, 0, CdFog, 0,0,0, 0, 0, 3 },
    { "SILENCE",    "recovery x1.2", "score x1.5", "damage taken x0.9", "the call is half as loud", CdSilence, 0,0,0, 2, 0, 4 },
    { "HUSH",       "burst charge x1.3", "burst score x3", "the call is half as loud", 0, CdHush, 0,0,0, 1, 0, 4 },
    { "MUFFLE",     "PERFECT recovery x2", "warnings are half as loud", 0, 0, CdMuffle, 0,0,0, 0, 0, 3 },
    { "COMPLACENT", "burst score x4", "warnings are half as loud", 0, 0, CdDampen, 0,0,0, 0, 0, 3 },
    { "WHIRL",      "score x3", "burst charge x2", "burst score x2", "the whole ring turns", CdSpin, 0,0,0, 2, 0, 1 , 1 },

    // -- paid once, at the moment they are taken ----------------------------
    // Health spent at full health is health that was going to be wasted; the
    // barrier cards move it to the bar where it will actually be missed. The
    // barrier is a charge, so two of them are two saves and the question of
    // what happens when both are held answers itself.
    { "BANDAGE",      "heal to full now", 0, 0, 0, CdSalve, 0,0,0, 0, 1, 1 },
    { "ENERGY DRINK", "fill the burst meter now", 0, 0, 0, CdIgnition, 0,0,0, 0, 2, 2 },
    { "ELIXIR",       "heal to full now", "fill the burst meter now", 0, 0, CdRenewal, 0,0,0, 1, 3, 2 },
    { "TRANSFUSION",  "heal to full now", "recovery x1.2", "spend 100% burst", 0,
                      CdTransfuse, 0,0,0, 1, 1 | 32, 3 },
    { "DISCHARGE",    "fill the burst meter now", "burst charge x1.2", "spend 50% health", 0,
                      CdDischarge, 0,0,0, 1, 2 | 16, 3 },
    { "BARRIER",      "barrier +1", "spend 10% health", 0, 0,
                      CdBarrier, 0,0,0, 0, 4 | 64, 2 },
    { "BARRICADE",    "barrier +2", "spend 30% health", 0, 0,
                      CdBarrier, 0,0,0, 0, 256 | 1024, 3 },
    { "BULWARK",      "barrier +1", "spend 30% burst", 0, 0,
                      CdBarrier, 0,0,0, 0, 8 | 64, 2 },
    { "RAMPART",      "barrier +2", "spend 50% burst", 0, 0,
                      CdBarrier, 0,0,0, 0, 512 | 1024, 3 },
    { "HUBRIS",       "barrier +4", "damage taken x2", 0, 0,
                      CdHubris, 0,0,0, 0, 128, 1 },
    { "SURGE",        "+1 barrier when the burst fires (max 1)", "burst charge x1.5",
                      "damage taken x1.2", 0, CdSurge, 0,0,0, 1, 0, 4 },
    { "FIGHTING GAME", "+1 barrier every 50 combo (max 2)", 0, 0, 0,
                      CdFightGame, 0,0,0, 0, 0, 4 },
    { "REFIT",        "+1 barrier 2 levels after one breaks (max 1)", "damage taken x1.1",
                      "recovery x0.9", 0, CdRefit, 0,0,0, 0, 0, 3 },
    { "TRIAGE",       "+4 barriers once, at 20% health", "burst charge x0.8",
                      "burst score x0.5", 0, CdTriage, 0,0,0, 0, 0, 5 },
    { "METAPHOR",     "items appear at random", "spend 50% health", "spend 100% burst", 0,
                      CdMetaphor, 0,0,0, 0, 16 | 32, 5 },

    // -- paid out at the end, not per note ----------------------------------
    { "PENNIES", "+50% coins earned at the end of the run", 0, 0, 0, CdTithe, 0,0,0, 0, 0, 5 },
    { "REPORT",  "+10% score at the end of the run", 0, 0, 0, CdLedger, 0,0,0, 0, 0, 1 },
};

#define CARD_COUNT ((int)(sizeof(cards) / sizeof(cards[0])))

// Normal is built never to show SWAP or BLIND, so the cards built around them
// are withheld from its DRAFT entirely - along with WHIRL, whose rotating ring
// is a difficulty of its own.
//
// The shop and the gacha are deliberately NOT gated. They are entered between
// runs, where the mode showing is not necessarily the mode about to be played,
// and Normal is not hard enough to need bought cards anyway - taking one in is
// the player's own call.
static int CardAllowed(int c) {
    return (mode == MODE_HARD) || !cards[c].hardOnly;
}


// Both places that show a card - the draft and the card list - colour it the
// same way, so the layout carries the same meaning in both: every line above
// the last is what the card gives, the last line is what it costs.
// `muted` drains the colour for a card that cannot be taken. Green and red are
// the loudest things on the shop screen, and on a card you have no money for
// they pull the eye to the one option you cannot pick. The two hues stay
// faintly apart so gain and cost are still distinguishable, just quiet.
// A line that will not fit is folded at a space, not shrunk.
//
// Shrinking was the obvious fix - the card NAME already does it a few lines
// down - but the draft puts three cards side by side, and the worst string here
// needs 11px to fit a box where its neighbours are drawing at 16px. One card in
// a smaller face reads as a rendering fault, not as a long card.
//
// The continuation is indented. Two full-width lines read as two separate
// effects, and this screen colours effects by whether they are a gain or a
// cost; the indent says "still the same one". It also costs a row, which the
// card screens have to spare - the draft box fits five and the longest card
// uses four.
//
// `maxW` is the drawable width, not the box width: every caller has its own
// padding and its own column stride, and a single constant here would be wrong
// on four screens out of five.
static void DrawCardDesc(const Card* cd, int x, int y, int fs, int step,
                         int muted, int maxW) {
    const char* line[4] = { cd->d1, cd->d2, cd->d3, cd->d4 };
    int n = 0;
    while (n < 4 && line[n]) n++;    // one-shot cards have no cost line at all
    int row = 0;
    for (int i = 0; i < n; i++) {
        Color c;
        if (i > cd->extraBenefit) {                                     // cost
            c = muted ? (Color) { 118, 100, 103, 255 }
            : (Color) { 214, 132, 138, 255 };
        }
        else {                                                          // gain
            c = muted ? (Color) { 102, 116, 108, 255 }
            : (Color) { 156, 214, 176, 255 };
        }

        // Translate before folding. The fold has to happen on the text that
        // will actually be drawn, and Korean is the half that overflows -
        // measuring the English and folding the Korean would fold nothing.
        // Passing the fragment back through UiText is safe: T() returns its
        // argument unchanged when the table has no entry, which a Korean
        // fragment never will.
        const char* s = T(line[i]);
        int first = 1;
        while (*s) {
            while (*s == ' ') s++;
            if (!*s) break;

            char buf[256];
            int cut = 0, depth = 0;
            for (const char* p = s; ; p++) {
                // A bracketed tail is one thought - "(max 2)" folded to leave
                // "(max" hanging reads as a typo. Spaces inside brackets are
                // not offered as break points at all.
                if (*p == '(') { depth++; continue; }
                if (*p == ')') { if (depth > 0) depth--; continue; }
                if (*p != ' ' && *p != '\0') continue;
                if (*p == ' ' && depth > 0) continue;
                int len = (int)(p - s);
                if (len <= 0 || len >= (int)sizeof buf) break;
                memcpy(buf, s, (size_t)len);
                buf[len] = '\0';
                if (UiMeasure(buf, fs) > maxW) break;
                cut = len;                          // longest prefix that fits
                if (!*p) break;
            }
            // A single word wider than the box still gets drawn. Overflowing by
            // a little is worse than dropping the line, and there is no third
            // option that does not involve breaking mid-syllable.
            if (cut <= 0) cut = (int)strlen(s);

            memcpy(buf, s, (size_t)cut);
            buf[cut] = '\0';
            UiText(buf, first ? x : x + 10, y + row * step, fs, c);
            row++;
            first = 0;
            s += cut;
        }
    }
}

// A card on entering level 2, then every third level: 2, 5, 8, 11, ...
// Normal hands out three, which is all a ten level run has room for. Hard
// hands out thirty three, which is where a build actually gets to happen.
static int IsDraftLevel(int li) {      // li is the zero-based level index
    return li >= 1 && ((li - 1) % 3) == 0;
}

// Hard runs offer more picks than there are cards, so cards stack. Counts, not
// a list: stacking is then free, and the HUD can show "GLASS x3".
static int cardCount[CARD_COUNT];
static int takenTotal;
static unsigned char takenNeeds, takenKills;
static unsigned int  takenGroups;

// ---- burst ---------------------------------------------------------------
// The third resource. Perfects charge it, SPACE spends it, and while it runs
// every hit counts as Perfect, score is multiplied and nothing can hurt you.
//
// It also gives Perfect a job. Before this, Perfect over Good was 200 points
// against 100 and a hair more healing - hitting exactly was barely worth
// aiming for.
// Per Perfect, scaled by how finely the bar was subdivided.
//
// A flat rate charges the meter per NOTE, so a sixteenth bar - which has three
// or four times the notes of a quarter bar - filled it three or four times as
// fast. Early Hard sat at a sensible three levels per burst while late Hard
// was handing one out every level. Scaling by subdivision makes a bar worth
// roughly the same wherever it appears.
#define BURST_GAIN 0.022f
#define BURST_HARD_MUL 0.80f

static float BurstGain(unsigned char sub) {
    float k = (sub >= 4) ? 0.40f : (sub >= 2) ? 0.70f : 1.0f;
    if (mode == MODE_HARD) k *= BURST_HARD_MUL;
    return BURST_GAIN * k;
}

// ---- run state -----------------------------------------------------------
typedef enum { TITLE, PLAYING, DEAD, WON, DRAFT, HELP, SETTINGS, CREDITS, SHOP, CODEX } GameState;
static int codexPage;   // the card list is 62 entries, eight to a page
static int helpPage;    // 0 how to press, 1 what the game does back, 2 the meta

// Every coordinate in this file is written against a fixed 960x540 stage. The
// scene is drawn into a render texture of exactly that size and then blitted
// to the real window, so full screen is a scale at the very end rather than a
// rewrite of every DrawText call.
#define STAGE_W 960
#define STAGE_H 540

// One string, one place. 1.0 is the build submitted to the contest; anything
// after it is post-submission work, and a player reporting a bug has to be
// able to say which one they are holding.
#define GAME_VERSION "Ver 1.1"

// Volume moves in discrete tenths. A continuous slider on a held key ran the
// whole range in under a second and was impossible to land on a value.
#define VOL_STEPS 10
static int   volStep = 8;

static GameState state = TITLE;
static int    score, combo;
static int    hiScores[2];            // indexed by GameMode
static int    hiLevels[2];            // furthest level reached, same indexing
#define HI_SCORE (hiScores[mode])
#define HI_LEVEL (hiLevels[mode])
static float  hp;
static float  burst, burstLeft;       // charge 0..1, and beats remaining

// Counts down after the meter fills, so the announcement is an event rather
// than a sign. See where it is drawn for why that matters.
static float  burstReadyFlash;

// Barriers are charges, not a toggle. Two barrier cards mean two saves, each
// paid for separately - which is also the answer to what happens when both are
// held, without a special case for it.
// The most barriers that can be held at once. Four is not a rule the player has
// to learn: the shield shells are drawn one per charge and there are four of
// them, so a cap you can see is a cap nobody has to be told.
#define BARRIER_CAP 4

// What the level-up grant tops you up TO - not how many it adds. Holding two
// cards that grant on level up still leaves you at one, because the card says
// "+1 on level up", not "+1 per card".
#define LEVEL_BARRIER_CAP 2
static int    barrierLeft;
static float  barrierBreak;    // countdown for the shatter, seconds
// The options screen, as rows. Two of them are conditional - the monitor row
// on a single-display machine, the pad rows with no pad plugged in - so the
// screen builds its own list every frame and the cursor indexes THAT, not the
// enum. Fixed indices were fine while the one optional row was last; they stop
// being fine the moment something optional sits in the middle.
enum { OPT_VOLUME, OPT_BORDERLESS, OPT_MOTION, OPT_LANG,
       OPT_MONITOR, OPT_PADFACE, OPT_PADSWAP, OPT_MAX };
static int optRow[OPT_MAX];

// The title, as rows. The two modes sit at the top because they are the thing
// the screen is for; everything under them is a place to go rather than a game
// to start.
enum { T_NORMAL, T_ENDLESS, T_SEED, T_SHOP, T_CODEX, T_OPTIONS, T_HELP, T_CREDITS, T_QUIT };
#define TITLE_ROWS 9
static int    titleSel;        // cursor on the title screen
static int    optSel;          // cursor on the options screen

// The shop, as rows. The three cards are a horizontal group at the top and
// everything else is a column under them, so the cursor moves the way the
// screen looks. The last row on each page is the door to the other one - a
// visible menu entry rather than a key nobody would guess.
// No reroll row. It is R here exactly as it is in the draft - the same action
// on the same key - and it belongs to the cards rather than sitting under them
// as a fourth thing to buy.
enum { SC_CARD0, SC_CARD1, SC_CARD2, SC_HP, SC_BURST, SC_TOGACHA, SHOP_C_ROWS };
// No SKIN row. It was the only row on this page with a value to sweep, so
// LEFT/RIGHT changes the skin from anywhere on the page instead - one less
// stop on a three-stop list, and the arrows never sit idle.
enum { SG_PULL, SG_ODDS, SG_TOCARDS, SHOP_G_ROWS };
static int    shopSel;         // cursor on the shop
static int    shopCard;        // which card the cursor left the top row on
// Fills optRow[] and returns how many rows the screen actually has. Called
// from the input, the highlight and the draw, all in the same frame, so it has
// to be cheap and it has to give the same answer three times - which it does,
// since nothing it reads can change mid-frame.
static int OptionRows(void) {
    int n = 0;
    optRow[n++] = OPT_VOLUME;
    optRow[n++] = OPT_BORDERLESS;
    optRow[n++] = OPT_MOTION;
    optRow[n++] = OPT_LANG;
    if (GetMonitorCount() > 1) optRow[n++] = OPT_MONITOR;
    // No pad, no pad rows. Both of them only change what pad prompts say, so
    // with nothing plugged in they are two controls that cannot be seen to do
    // anything - and the screen is already the longest one in the game.
    if (IsGamepadAvailable(PAD)) {
        optRow[n++] = OPT_PADFACE;
        optRow[n++] = OPT_PADSWAP;
    }
    return n;
}

static int    comboBarrierAt;  // last combo milestone that paid out
static int    refitCountdown;  // levels left until REFIT pays; -1 = idle
static int    triageUsed;      // TRIAGE fires once per run

static int    burstWasReady;
static int    offer[3];
static int    lastDraftLevel = -1;

// Three free rerolls a run, then the meter pays. Free-per-draft was the first
// shape and it gave away too much: a reroll with no cost at all, seventeen
// times over on Hard. A fixed budget makes the first three a plan and every
// one after that a trade.
//
// Burst is the right thing to trade because it is bounded, it is earned by
// playing well - only Perfects charge it - and running low on it does not
// spiral the way running low on health does. Paying health would have made the
// reroll unaffordable exactly when it was most needed.
#define DRAFT_REROLLS 3
#define REROLL_BURST  0.50f
static int    rerollsLeft = DRAFT_REROLLS;

// ---- shop ----------------------------------------------------------------
// Meta progression, and deliberately outside the run. This is a rhythm game;
// the card draft already interrupts the tempo once every three levels, and a
// shop in the middle of a run would make that worse. So the shop sits between
// runs, and everything bought lasts exactly one run.
//
// The currency is levels cleared, never score. Score is multiplied by cards
// and runs away from any price you could set against it.
#define SHOP_CARDS    3
#define PRICE_HP      3        // the +10% tier, the cheapest thing the gacha gives
#define PRICE_BURST   1        // the 50% tier - the shop never undercuts the gacha

#define REROLL_PRICE0 1        // and it doubles with every reroll after that


// Settings, wallet and shop state all persist, and several of the things
// below write immediately rather than on exit, so the writer is declared
// here - well above its first caller in the shop and gacha code.
static void SaveAll(void);

// ---- skins ---------------------------------------------------------------
// Colour only. The ship is a stack of cylinders, so a shape variant means a
// branch in the draw code for every part; a palette swap is two colours and
// reads just as clearly at the size the ship is actually seen.
//
// Skin 0 is the ship as it always looked, and is owned from the start so the
// select screen is never empty.
typedef struct { const char* name; Color hull, trim; } Skin;

static const Skin skins[] = {
    { "STEEL",   {  70,  90, 130, 255 }, { 102, 191, 255, 255 } },   // the original
    { "RUST",    { 132,  66,  48, 255 }, { 255, 160,  70, 255 } },
    { "MOSS",    {  58, 104,  74, 255 }, { 150, 230, 140, 255 } },
    { "PLUM",    {  96,  62, 128, 255 }, { 205, 150, 255, 255 } },
    { "ROSE",    { 138,  62,  88, 255 }, { 255, 150, 185, 255 } },
    { "SAND",    { 128, 112,  62, 255 }, { 245, 225, 140, 255 } },
    { "ICE",     {  66, 116, 128, 255 }, { 175, 245, 255, 255 } },
    { "VOID",    {  44,  44,  56, 255 }, { 190, 190, 210, 255 } },
};
#define SKIN_COUNT ((int)(sizeof(skins) / sizeof(skins[0])))

static unsigned int skinOwned = TEST_ALLSKINS ? 0xFFFFFFFFu : 1u;   // bit per skin; bit 0 is always set
static int skinSel;

static int SkinOwned(int i) { return (skinOwned >> i) & 1u; }
static int SkinOwnedCount(void) {
    int n = 0;
    for (int i = 0; i < SKIN_COUNT; i++) n += SkinOwned(i);
    return n;
}

static int wallet = TEST_WALLET;
static int shopOffer[SHOP_CARDS] = { -1, -1, -1 };
static int shopRerolls;        // rerolls bought since the last run
static int shopPage;           // 0 = things to buy, 1 = gacha and skins
static int pendCard = -1;      // card the next run starts holding
static int pendCard2 = -1;     // a second one, only ever won from the gacha
static int pendHp;
static int pendBurst;
static int runReward;          // what the finished run paid, shown on the result screen

// The shop is not part of a seeded run, so it gets its own stream seeded from
// the clock. Drawing from the chart stream would let a shop visit change the
// chart; drawing from the effects stream would make the first offer on a fresh
// install identical for everyone.
static unsigned int shopRng = 1;
static unsigned int ShopRand(void) {
    shopRng ^= shopRng << 13;
    shopRng ^= shopRng >> 17;
    shopRng ^= shopRng << 5;
    return shopRng;
}


// ---- gacha ---------------------------------------------------------------
// Coins pile up once the per-run items are bought, and a currency with nothing
// left to buy stops being a reward. This is where the surplus goes.
//
// **There is no blank.** A pull that lands on something already owned pays
// coins instead, so every pull moves the wallet or the next run. What makes it
// a gamble is that some outcomes are losses, not that some are nothing -
// nothing is just a wasted button press.
#define PRICE_GACHA 3

// Weights are per mille: they sum to 1000, and the odds screen prints one
// decimal place straight out of this table with no arithmetic. Percentages
// would be the obvious unit, but the jackpot wants a rarity of 0.1% and an
// integer percent cannot say that; a tenth is fine enough for every row and
// still lets tuning one row obviously cost another.
//
// `coins` is the fixed payout for a coin row and 0 for everything else, which
// keeps the amount and the label that states it on the same line. It used to
// live in a switch several hundred lines away from the text describing it.
typedef struct { int kind; int weight; int coins; const char* label; } GachaSlot;

#define GA_CARD1    0
#define GA_CARD2    1
#define GA_HP1      2          // max health +10%
#define GA_HP2      3          // +20%
#define GA_HP3      4          // +30%
#define GA_BURST1   5          // start at 50%
#define GA_BURST2   6          // start at 100%
#define GA_SKIN     7
#define GA_COINS    8          // amount comes from the row, not the kind

// The jackpot is a flat amount, never a multiple of the wallet. A payout
// proportional to what the player is holding makes the expected value of a
// pull grow with the balance, so past a few hundred coins every pull profits
// and the currency never stops growing. Capping such a payout patches the
// symptom; a fixed number does not have the problem to begin with, and "three
// coins in, a hundred out" is a far clearer moment than "your pile doubled".
//
// 100 sits far under the ceiling. With the weights below the expected value
// of a pull turns positive - and the wallet runs away again - only at a
// jackpot of 1500, so this leaves 15x of margin. That margin is slack, not
// safety: at 0.1% the jackpot contributes 0.1 coins to a pull, so the machine
// is a near-flat tax of 1.4 coins per 3-coin pull. 100 coins lasts about 70
// pulls and a player ends a session ahead 0.1% of the time. The gamble now
// lives in the 64% of rows that pay a card, health or burst rather than in
// the coin rows, which is the sink the shop wants.
//
// CAREFUL: that ceiling moves whenever a coin row moves. The +5, +10 and
// duplicate skin payouts all feed the same sum, so raising any of them lowers
// the jackpot the table can carry. Recompute before changing one:
//
//     ceiling = 1000 * (PRICE_GACHA - sum(weight_i/1000 * coins_i, i != jackpot))
//
static const GachaSlot gachaTable[] = {
    { GA_CARD1,  150,   0, "a card" },
    { GA_HP1,    120,   0, "max health +10%" },
    { GA_BURST1, 120,   0, "start at 50% burst" },
    { GA_COINS,  100,   3, "+3 coins" },
    { GA_HP2,     90,   0, "max health +20%" },
    { GA_COINS,   80,  -3, "-3 coins" },
    { GA_COINS,   70,   5, "+5 coins" },
    { GA_BURST2,  70,   0, "start at 100% burst" },
    { GA_COINS,   60,  10, "+10 coins" },
    { GA_HP3,     50,   0, "max health +30%" },
    { GA_SKIN,    49,   0, "a skin" },
    { GA_CARD2,   40,   0, "two cards" },
    { GA_COINS,    1, 100, "+100 coins" },
};
#define GACHA_SLOTS ((int)(sizeof(gachaTable) / sizeof(gachaTable[0])))

// Tier 1..3 for health, 1..2 for burst. Index 0 is "none", so the stored value
// doubles as a "was one bought" flag and the save format did not have to grow.
static const float hpItemMul[3] = { 1.10f, 1.20f, 1.30f };
static const float burstItemAmt[2] = { 0.50f, 1.00f };

// The same strings the gacha rows use, so a tier is described identically
// wherever it appears - in the odds table, in the pull result, and on the
// title screen reminder.
static const char* hpItemName[3] = {
    "max health +10%", "max health +20%", "max health +30%"
};
static const char* burstItemName[2] = {
    "start at 50% burst", "start at 100% burst"
};
#define HP_TIER_SHOP    1      // lowest tier: the guaranteed buy must be the
#define BURST_TIER_SHOP 1      // weakest, or the gacha has nothing worth winning

// Roll state. The reel spins for a moment before the result lands, because a
// number that simply appears is not a gamble - the wait is the whole point.
#define GACHA_SPIN 1.35f
static float gachaTimer;       // counts down while the reel is spinning
static int   gachaSlot = -1;   // row it landed on, once it has landed
static int   gachaCoins;       // signed coin delta, for the result line
static int   gachaCardA = -1;  // cards won, for the result line
static int   gachaCardB = -1;
static int   gachaSkin = -1;
static int   gachaOdds;        // the odds panel is open

// Doubles: 1, 2, 4, 8... A flat +1 never bit - ten rerolls in a row still
// only cost eleven coins, so "do not stand here rerolling" was not actually
// enforced by anything. Doubling makes the fourth one a real decision.
//
// The loop caps rather than shifting, so a long sitting cannot overflow the
// price into a negative number and hand the player free rerolls.
static int RerollPrice(void) {
    int p = REROLL_PRICE0;
    for (int i = 0; i < shopRerolls && p < 100000; i++) p *= 2;
    return p;
}


// Resolve one pull. Called when the reel stops, not when the button is
// pressed, so the outcome is decided at the moment the player sees it.
//
// Winning something you already hold OVERWRITES it, with no refund - that is
// the risk of pulling again. Two cards then one card leaves you holding one.
// The single exception is a duplicate skin, which has nothing to overwrite:
// skins are permanent, so that pays coins instead of doing nothing.
static void GachaResolve(void) {
    int total = 0;
    for (int i = 0; i < GACHA_SLOTS; i++) total += gachaTable[i].weight;
    int r = (int)(ShopRand() % (unsigned int)total);
    int si = GACHA_SLOTS - 1;
    for (int i = 0; i < GACHA_SLOTS; i++) {
        if (r < gachaTable[i].weight) { si = i; break; }
        r -= gachaTable[i].weight;
    }

    gachaCoins = gachaTable[si].coins;     // 0 for everything that is not coins
    gachaCardA = gachaCardB = -1;
    gachaSkin = -1;

    switch (gachaTable[si].kind) {
    case GA_CARD1:
        pendCard = (int)(ShopRand() % (unsigned int)CARD_COUNT);
        pendCard2 = -1;                       // one card means one card
        gachaCardA = pendCard;
        break;
    case GA_CARD2:
        pendCard  = (int)(ShopRand() % (unsigned int)CARD_COUNT);
        pendCard2 = (int)(ShopRand() % (unsigned int)CARD_COUNT);
        gachaCardA = pendCard; gachaCardB = pendCard2;
        break;
    case GA_HP1: case GA_HP2: case GA_HP3:
        pendHp = gachaTable[si].kind - GA_HP1 + 1;
        break;
    case GA_BURST1: case GA_BURST2:
        pendBurst = gachaTable[si].kind - GA_BURST1 + 1;
        break;
    case GA_SKIN: {
        int missing = SKIN_COUNT - SkinOwnedCount();
        if (missing > 0) {
            int k = (int)(ShopRand() % (unsigned int)missing);
            for (int i = 0; i < SKIN_COUNT; i++) {
                if (SkinOwned(i)) continue;
                if (k == 0) { gachaSkin = i; break; }
                k--;
            }
            skinOwned |= 1u << gachaSkin;
        }
        else gachaCoins = 10;                 // nothing left to unlock
        break;
    }
    default: break;                           // GA_COINS: the row said it all
    }

    wallet += gachaCoins;
    if (wallet < 0) wallet = 0;    // a loss can never put the player in debt
    gachaSlot = si;
    PlaySound(gachaCoins < 0 ? denySnd : levelSnd);
    SaveAll();
}

static void DealShop(void) {
    for (int i = 0; i < SHOP_CARDS; i++) {
        int c, dup;
        do {
            c = (int)(ShopRand() % (unsigned int)CARD_COUNT);
            dup = 0;
            for (int j = 0; j < i; j++) if (shopOffer[j] == c) dup = 1;
        } while (dup);
        shopOffer[i] = c;
    }
}
static int    showCards;              // card list open over the result screen

// Run tally. Doubles as the tuning instrument: "too hard" is not actionable,
// "42 misses, 18 of them stray presses" is.
static int    cntPerfect, cntGood, cntMiss, cntBad, maxCombo;

// Beat of the last input this lane accepted. Anything arriving closer than
// half a slot is the other key of the same press - two hands on one note -
// not a second note, so it is folded into the first.
static float  lanePressAt[LANE_COUNT];

// Latest judgement, shown at the gate and faded out fast so a run of notes
// does not stack text on screen.
static const char* judgeText;
static Color  judgeCol;
static float  judgeTimer;

static void SetJudge(const char* t, Color c) {
    judgeText = t;
    judgeCol = c;
    judgeTimer = 0.45f;
}
static int    lastSub, chunkIndex;          // lastSub: last sixteenth the music played
static float  nextChunkBeat;
static float  prevBeat;        // last frame's beat, for edge-detected events
static float  freezeBeat;      // beat captured when the run ended
static float  hitWindow;
static float  levelBanner;     // countdown for the LEVEL banner
static float  deckBanner;      // countdown for the ALL CARDS COLLECTED banner
static unsigned int seed;      // the whole chart is reproducible from this

// The playhead is rebased whenever the tempo changes, so `beat` stays
// continuous across a level up. Everything downstream is in beat space, so
// nothing else has to know the tempo moved.
static float  baseBeat;
static double baseTime;
static float  curBpm;
static int    paused;

// The pause screen and the result screen are the same shape: a short vertical
// list over a frozen scene. One builder, so the input and the drawing can never
// disagree about which row is which - that bug is silent until someone picks
// GIVE UP while reading CARDS.
//
// The CARDS row is absent, not greyed, when there is nothing to show. A run
// with no cards yet has no card list, and a disabled row is a question the
// player has to answer before they can ignore it.
enum { M_CARDS, M_RESUME, M_TITLE };
static int runSel;

static int RunMenu(int* out) {
    int n = 0;
    if (takenTotal > 0) out[n++] = M_CARDS;
    if (paused) out[n++] = M_RESUME;
    out[n++] = M_TITLE;
    return n;
}

static const char* RunMenuLabel(int row) {
    switch (row) {
    // "YOUR CARDS", not "CARDS": the title screen already uses that key for the
    // codex, and this row is the deck in hand rather than the book of them all.
    case M_CARDS:  return "YOUR CARDS";
    case M_RESUME: return "RESUME";
    default:       return "BACK TO TITLE";
    }
}

// Typed on the title screen. An empty box means "pick one from the clock",
// which is what makes a run shareable: the result screen prints the number
// that produced the chart, and typing it back in reproduces it exactly.
// A seed is a full unsigned int, so it prints as up to ten digits. The input
// box has to accept ten too, or a displayed seed cannot be typed back in.
#define SEED_TEXT_MAX 10
static char seedText[SEED_TEXT_MAX + 1];
static int  seedLen;

// ---- the seed box --------------------------------------------------------
// Ten digits is not much to type, but a pad cannot type at all - and the seed
// was the one thing on the title screen that only a keyboard could reach. A
// full virtual keyboard would be absurd for an alphabet of ten characters, so
// this is the alphabet itself, laid out as a grid:
//
//     0 1 2 3 4
//     5 6 7 8 9
//      DEL  CLR  OK
//
// The keyboard keeps working inside it - digits, backspace, enter - because
// the box is where the digits go now. Before this they were read straight off
// the title screen with nothing drawn to say so, which meant a stray number
// key silently changed the chart the player was about to start.
#define SEED_DEL   10
#define SEED_CLR   11
#define SEED_OK    12
#define SEED_CELLS 13
static int seedBox;        // the panel is open over the title
static int seedSel;        // which cell the cursor is on

static int SeedRowOf(int i)    { return (i < 5) ? 0 : (i < 10) ? 1 : 2; }
static int SeedRowStart(int r) { return (r == 0) ? 0 : (r == 1) ? 5 : SEED_DEL; }
static int SeedRowLen(int r)   { return (r == 2) ? 3 : 5; }

// One function for where a cell sits, used by the cursor and by the drawing.
// Two copies of this arithmetic would drift the moment either got nudged, and
// the up/down move depends on the drawn positions being the real ones.
static int SeedCellX(int i) {
    int r = SeedRowOf(i);
    int c = i - SeedRowStart(r);
    return (r == 2) ? (480 + (c - 1) * 128) : (480 + (c - 2) * 76);
}
static int SeedCellY(int i) { return 286 + SeedRowOf(i) * 54; }

// TextToInteger() tops out at INT_MAX, which is only nine and a bit digits, so
// the parse is done here against the real range.
static unsigned int ParseSeed(const char* s) {
    unsigned long long v = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        v = v * 10ull + (unsigned long long)(s[i] - '0');
        if (v > 4294967295ull) return 4294967295u;
    }
    return (unsigned int)v;
}

static float laneFlash[LANE_COUNT];   // player pressed this lane
static float callFlash[LANE_COUNT];   // the game called this lane
static float shake, missFlash, twistWarn;
static const char* warnText = "SWAP";

// Tunnel tint, eased towards the current level's colour so a level up is a
// visible change of scenery rather than a hard cut.
static float tintR = 150.0f, tintG = 150.0f, tintB = 255.0f;

static void RingTint(int lv, float* r, float* g, float* b) {
    switch (lv % 5) {
    case 0:  *r = 150; *g = 150; *b = 255; break;   // blue
    case 1:  *r = 120; *g = 235; *b = 220; break;   // teal
    case 2:  *r = 205; *g = 155; *b = 255; break;   // violet
    case 3:  *r = 255; *g = 180; *b = 130; break;   // amber
    default: *r = 145; *g = 255; *b = 175; break;   // green
    }
}


// The gauge ceiling can be lowered mid-run by FRAGILE. Defined before its
// first use on purpose: C would otherwise assume it returns int and silently
// truncate the clamp below to whole numbers.
static float HpMax(void) { return HP_MAX * mods.hpMaxMul; }

static void ApplyLevel(int lv) {
    if (lv < 0) lv = 0;
    if (lv > LevelCount() - 1) lv = LevelCount() - 1;
    level = lv;
    Level l = LevelAt(lv);
    curBpm = l.bpm * mods.bpmMul;
    hitWindow = l.window + mods.windowAdd;
    burst += mods.burstStart;
    if (burst > 1.0f) burst = 1.0f;
    hp += mods.levelHeal;
    if (hp > HpMax()) hp = HpMax();
}

// Every knob at its neutral value. A run with no cards behaves exactly as it
// did before the card system existed.
static void ResetMods(void) {
    mods.scoreMul = 1.0f;
    mods.dmgMul = 1.0f;
    mods.healMul = 1.0f;
    mods.healPerfectMul = 1.0f;
    mods.healGoodZero = 0;
    mods.windowAdd = 0.0f;
    mods.bpmMul = 1.0f;
    mods.callVol = 1.0f;
    mods.warnVol = 1.0f;
    mods.burstRate = 1.0f;
    mods.burstBeats = 4.0f;
    mods.burstScore = 2.0f;
    mods.burstStart = 0.0f;
    mods.levelHeal = 0.0f;
    mods.hpMaxMul = 1.0f;
    mods.fogBeats = 0.0f;
    mods.gimMul[0] = mods.gimMul[1] = mods.gimMul[2] = 1.0f;
    mods.forceGim = TEST_DISGUISE ? 3 : 0;
    mods.trapDisguise = TEST_DISGUISE;
    mods.spin = 0;
    mods.itemPct = TEST_ITEM_PCT;
    mods.levelBarrier = 0;
    mods.barrierOnBurst = 0;
    mods.barrierCombo = 0;
    mods.barrierRefit = 0;
    mods.barrierTriage = 0;
    mods.burstLocked = 0;
    mods.rewardMul = 1.0f;
    mods.endScoreMul = 1.0f;
    spinAngle = 0.0f;
}

// Up to three cards the player does not already hold. No duplicates: with a
// deck large enough to cover a run there is no reason to stack, and stacking
// is where the clamps in TakeCard had to fight to keep the game playable.
//
// Offers fewer than three - or none - when the deck runs low. `offerCount` is
// what the draft screen draws, so an empty offer is skipped rather than shown.
static int offerCount;

// Can this card still be offered? Held already, excluded by its group, built
// around a gimmick the run has removed, or removing a gimmick the run is built
// around - any of those and it is out.
static int CardAvailable(int c) {
    if (!CardAllowed(c)) return 0;
    if (cardCount[c] > 0) return 0;
    if (cards[c].group && (takenGroups & (1u << cards[c].group))) return 0;
    if (cards[c].needs & takenKills) return 0;
    if (cards[c].kills & takenNeeds) return 0;
    return 1;
}

static void DealOffer(void) {
    offerCount = 0;
    for (int i = 0; i < 3; i++) offer[i] = -1;

    for (int i = 0; i < 3; i++) {
        int fresh = 0;
        for (int c = 0; c < CARD_COUNT; c++) {
            if (!CardAvailable(c)) continue;
            int already = 0;
            for (int o = 0; o < i; o++) if (offer[o] == c) already = 1;
            if (!already) fresh++;
        }
        if (fresh == 0) break;

        int k = (int)(CardRand() % (unsigned int)fresh);
        for (int c = 0; c < CARD_COUNT; c++) {
            if (!CardAvailable(c)) continue;
            int already = 0;
            for (int o = 0; o < i; o++) if (offer[o] == c) already = 1;
            if (already) continue;
            if (k == 0) { offer[i] = c; offerCount++; break; }
            k--;
        }
    }
}

static void TakeCard(int c) {
    cardCount[c]++;
    takenTotal++;
    takenNeeds |= cards[c].needs;
    takenKills |= cards[c].kills;
    if (cards[c].group) takenGroups |= (1u << cards[c].group);
    cards[c].apply(&mods);

    // Cards no longer stack, but they still combine, and some pairs multiply
    // into a game that cannot be lost or cannot be played. These are the edges.
    if (mods.dmgMul < 0.25f) mods.dmgMul = 0.25f;
    if (mods.healMul > 5.0f) mods.healMul = 5.0f;
    if (mods.healPerfectMul > 5.0f) mods.healPerfectMul = 5.0f;
    if (mods.windowAdd > 0.05f) mods.windowAdd = 0.05f;
    if (mods.bpmMul > 1.45f) mods.bpmMul = 1.45f;
    if (mods.bpmMul < 0.70f) mods.bpmMul = 0.70f;
    if (mods.callVol < 0.20f) mods.callVol = 0.20f;
    if (mods.warnVol < 0.20f) mods.warnVol = 0.20f;
    if (mods.burstRate > 4.0f) mods.burstRate = 4.0f;
    if (mods.burstScore > 40.0f) mods.burstScore = 40.0f;
    if (mods.burstStart > 0.60f) mods.burstStart = 0.60f;
    if (mods.levelHeal > 0.50f) mods.levelHeal = 0.50f;
    if (mods.hpMaxMul < 0.40f) mods.hpMaxMul = 0.40f;

    if (hp > HpMax()) hp = HpMax();      // FRAGILE can lower it under us

    // The level's numbers are derived from the mods, so re-derive them now.
    ApplyLevel(level);

    // After ApplyLevel, which has its own heal and its own clamp - otherwise a
    // full heal would be immediately re-clamped against the old ceiling.
    // Bit 0 heals, bit 1 fills the meter, so 3 does both.
    if (cards[c].instant & 1) hp = HpMax();
    if (cards[c].instant & 2) burst = 1.0f;

    // Costs are charged after the heals above, so a card that both heals and
    // charges a share of health charges it against the healed figure. TRANSFUSION
    // and DISCHARGE both rely on that: each fills one gauge and empties the other,
    // and the order is what makes them a trade rather than a wash.
    //
    // The share is of the MAXIMUM, not of what is in hand. A card that took a
    // tenth of the current bar would grow cheaper exactly as the player got
    // closer to dying, which is backwards - the moment insurance is worth most
    // is the moment it should cost most.
    if (cards[c].instant & 4)   hp -= HpMax() * 0.10f;
    if (cards[c].instant & 8)   burst -= 0.30f;
    if (cards[c].instant & 16)  hp -= HpMax() * 0.50f;
    if (cards[c].instant & 32)  burst = 0.0f;
    if (cards[c].instant & 64)  barrierLeft += 1;
    if (cards[c].instant & 128) barrierLeft += 4;
    if (cards[c].instant & 256)  hp -= HpMax() * 0.30f;
    if (cards[c].instant & 512)  burst -= 0.50f;
    if (cards[c].instant & 1024) barrierLeft += 2;
    if (barrierLeft > BARRIER_CAP) barrierLeft = BARRIER_CAP;

    // A cost against the maximum can exceed what the player is holding, and the
    // card is still allowed - being unable to pick it would be a worse rule than
    // paying dearly for it. What it must never do is end the run at the moment
    // it is taken, before a single bar has been played with it. So the payment
    // stops at a sliver.
    //
    // A share of the maximum, not a flat figure, so the sliver looks the same on
    // a FRAGILE bar as on a full one.
    //
    // The meter has no such floor on purpose: an empty meter costs the player a
    // burst and some reroll currency, and neither of those ends anything.
    if (hp < HpMax() * 0.01f) hp = HpMax() * 0.01f;
    if (burst < 0.0f) burst = 0.0f;
    for (int i = 0; i < LANE_COUNT; i++) SetSoundVolume(callSnd[i], mods.callVol);
    SetSoundVolume(twistSnd, 0.55f * mods.warnVol);
}

static void StartRun(unsigned int s) {
    seed = s ? s : 1;          // xorshift must not start at zero
    rngState = seed;
    // Same seed, same offers - but on a stream of its own, so a reroll costs
    // the player only what they paid for it, never the chart. Mixed rather
    // than copied, or the two streams would open on correlated values.
    cardRng = seed ^ 0x9E3779B9u;
    if (!cardRng) cardRng = 1;

    for (int i = 0; i < MAX_ENEMIES; i++)   enemies[i].alive = 0;
    for (int i = 0; i < MAX_CALLS; i++)     calls[i].pending = 0;
    for (int i = 0; i < MAX_WARNS; i++)     warns[i].pending = 0;
    for (int i = 0; i < MAX_BEAMS; i++)     beams[i].life = 0.0f;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0.0f;
    for (int i = 0; i < LANE_COUNT; i++)    laneFlash[i] = callFlash[i] = 0.0f;
    segReady = -1;             // every run starts pointing straight ahead

    score = 0;
    combo = 0;
    hp = HP_MAX;
    judgeTimer = 0.0f;
    judgeText = 0;
    cntPerfect = cntGood = cntMiss = cntBad = maxCombo = 0;
    for (int i = 0; i < LANE_COUNT; i++) lanePressAt[i] = -1e9f;
    lastSub = -1;
    chunkIndex = 0;
    twistWarn = 0.0f;
    nextChunkBeat = FIRST_CHUNK_BEAT;
    prevBeat = 0.0f;
    shake = 0.0f;
    missFlash = 0.0f;
    levelBanner = 0.0f;
    deckBanner = 0.0f;
    paused = 0;
    burst = 0.0f;
    burstLeft = 0.0f;
    burstReadyFlash = 0.0f;
    barrierLeft = TEST_BARRIERS;
    barrierBreak = 0.0f;
    comboBarrierAt = 0;
    refitCountdown = -1;
    triageUsed = 0;
    burstWasReady = 0;
    for (int i = 0; i < CARD_COUNT; i++) cardCount[i] = 0;
    takenTotal = 0;
    takenNeeds = takenKills = 0;
    takenGroups = 0;
    lastDraftLevel = -1;
    rerollsLeft = DRAFT_REROLLS;
    showCards = 0;

    ResetMods();
    for (int i = 0; i < LANE_COUNT; i++) SetSoundVolume(callSnd[i], mods.callVol);
    ApplyLevel(0);

    // Shop purchases land before the first note and are spent by landing.
    // Gated on PLAYING because the attract loop calls StartRun too, and it
    // would otherwise eat what the player just bought without playing a note.
    if (state == PLAYING) {
        if (pendHp) mods.hpMaxMul *= hpItemMul[pendHp - 1];
        if (pendCard >= 0)  TakeCard(pendCard);   // re-derives the level from mods
        if (pendCard2 >= 0) TakeCard(pendCard2);  // the gacha two-card roll
        if (pendHp) hp = HpMax();
        if (pendBurst) burst = burstItemAmt[pendBurst - 1];
        pendCard = pendCard2 = -1;
        pendHp = pendBurst = 0;
        shopRerolls = 0;                          // the reroll price resets with the run

        // Playing deals a fresh hand, for free. Done HERE rather than on the
        // next shop visit so that it lands in `shopOffer`, which is saved -
        // a flag checked at the door would be lost the moment the game is
        // closed, and "I played, so the shop moved on" has to survive that.
        //
        // Starting a run and quitting it immediately gets the same free deal.
        // That is deliberate: the effort of going in and out is worth more
        // than the reroll price it dodges, and policing it would spend code
        // on making someone feel watched.
        DealShop();
        SaveAll();
    }

    baseBeat = 0.0f;
    baseTime = GetTime();
}

// The seed the next run should use: whatever is typed on the title, or a
// throwaway one from the clock.
static unsigned int ChosenSeed(void) {
    if (seedLen > 0) return ParseSeed(seedText);
    return (unsigned int)time(NULL) ^ Rand();
}

// The three things the box can do to the text. Kept together and next to
// ChosenSeed so the whole life of the number - typed, edited, read - is on one
// screen, and so both input paths (a digit key, a cell press) go through the
// same two calls instead of each editing the buffer their own way.
static void SeedPush(int digit) {
    if (seedLen >= SEED_TEXT_MAX) { PlaySound(denySnd); return; }
    seedText[seedLen++] = (char)digit;
    seedText[seedLen] = '\0';
    PlaySound(hatSnd);
}
static void SeedBack(void) {
    if (seedLen <= 0) { PlaySound(denySnd); return; }
    seedText[--seedLen] = '\0';
    PlaySound(hatSnd);
}
// OK keeps the number, ESC puts back the one that was there when the box
// opened. The row behind the panel shows the edit as it happens, which is what
// makes cancelling mean something: the player can watch a wrong number being
// built and back out of it, instead of being told afterwards that it took.
//
// Committing restarts the attract run on the new number - the title screen is
// already playing a chart, and leaving it on the old one would say the box had
// done nothing. Cancelling changes nothing, so it restarts nothing.
static char seedUndo[SEED_TEXT_MAX + 1];
static int  seedUndoLen;

static void SeedOpen(void) {
    memcpy(seedUndo, seedText, sizeof(seedUndo));
    seedUndoLen = seedLen;
    seedBox = 1;
    seedSel = 0;
    PlaySound(twistSnd);
}
static void SeedCommit(void) {
    seedBox = 0;
    PlaySound(hatSnd);
    if (seedLen != seedUndoLen || strcmp(seedText, seedUndo) != 0)
        StartRun(ChosenSeed());
}
static void SeedCancel(void) {
    memcpy(seedText, seedUndo, sizeof(seedText));
    seedLen = seedUndoLen;
    seedBox = 0;
    PlaySound(hatSnd);
}

// Shop currency for a finished run. Hard pays one a level, so a full clear is
// fifty. Normal is ten levels across four pools, so paying it by the level
// would make the same work worth a fifth as much; it pays by pool instead -
// one for clearing anything at all, one more for each pool reached, five for
// the whole thing.
//
// Only ever called from EndRun, which the attract loop never reaches.
static int RunReward(GameState how) {
    int base;
    if (mode == MODE_HARD) base = (how == WON) ? LevelCount() : level;
    else if (how == WON)   base = 5;
    else if (level < 1)    base = 0;
    else                   base = 1 + (int)LevelAt(level).pool[0];
    return (int)((float)base * mods.rewardMul);
}

static void EndRun(GameState how, float beat) {
    state = how;
    freezeBeat = beat;
    shake = 0.0f;              // the scene freezes from here; a live shake would never decay
    missFlash = 0.0f;

    // The end-of-run multipliers land here, before anything reads the score.
    score = (int)((float)score * mods.endScoreMul);
    runReward = RunReward(how);
    wallet += runReward;
    if (score > HI_SCORE) HI_SCORE = score;
    if (level + 1 > HI_LEVEL) HI_LEVEL = level + 1;
    SaveAll();                 // the wallet has to survive the app closing
}

// One entry point for every way of losing a life: a missed note, or a press
// with nothing under it.
// `amount` differs by cause: letting a note through costs more than firing at
// nothing. The result screen counts them separately for the same reason.
// ---- items ---------------------------------------------------------------
// METAPHOR turns a share of the notes into pickups. Six kinds, in three pairs;
// the lesser of each pair is twice as common as the greater.
//
//   1  health  x5      2  health  x8
//   3  meter   x5      4  meter   x8
//   5  score   x10     6  score   x16
//
// Health and meter are multiples of what THAT note would have paid for a
// Perfect, and score is a multiple of what that note actually scored - so an
// item is worth more in a deck built to recover, and more on a slow bar than a
// fast one. It rides the numbers the player already chose rather than adding a
// flat figure that ignores them.
//
// The multipliers look enormous and are not. One note's reward is tiny - an
// Endless Perfect heals 0.6% of the bar - so a multiple of it is still small
// change. These are sized backwards from the only figure that matters, which is
// what the card is worth across a whole run at a 5% rate:
//
//   expected gain = 0.05 * (2*lesser + 1*greater) / 3      per note
//     health  0.05 * (2*5  + 8 ) / 9  = +10%
//     meter   0.05 * (2*5  + 8 ) / 9  = +10%
//     score   0.05 * (2*10 + 16) / 9  = +20%
//
// Change the rate and these have to move with it, or the card silently becomes
// a different card. The rate is the knob the player never sees; the multipliers
// are the ones they feel.

// Picking one up throws a brighter, whiter spray than an ordinary hit - enough
// to say "that one was worth something" and nothing more. Which of the six it
// was is never shown, before or after. The player was going to hit the note
// either way, so the only thing the information could do is give them
// something extra to read in the middle of a bar.
#define ITEM_SPARK (Color){ 255, 245, 210, 255 }

static void CollectItem(int kind, int gained, Vector3 at) {
    switch (kind) {
    case 1: case 2: {
        float n = (kind == 1) ? 5.0f : 8.0f;
        hp += BaseHealPerfect() * mods.healMul * mods.healPerfectMul * n;
        break;
    }
    case 3: case 4: {
        float n = (kind == 3) ? 5.0f : 8.0f;
        burst += BURST_GAIN * mods.burstRate * n;
        break;
    }
    default: {
        float n = (kind == 5) ? 10.0f : 16.0f;
        score += (int)((float)gained * n);
        break;
    }
    }
    SpawnBurst(at, ITEM_SPARK, 14);
}
// Add `n`, but never past `cap`, and never take away what another card granted.
// Every card in the family reads "+N (max M)" and this is that sentence: the
// grant is a rate, the cap is a depth. Raising the cap does not make a source
// pay more often, it lets the player bank one for the bar that needs two.
//
// The guard is on entry, not on the sum, so a stack of four from HUBRIS is not
// clipped back to two by the next level up.
static void BarrierGain(int n, int cap) {
    if (cap > BARRIER_CAP) cap = BARRIER_CAP;
    if (barrierLeft >= cap) return;
    barrierLeft += n;
    if (barrierLeft > cap) barrierLeft = cap;
}

// TRIAGE is the exception: it ADDS, once, and only when the run is nearly over.
// The threshold is deliberately low. A deck that has stacked incoming damage can
// fall from above it to zero in one hit and never collect - the insurance is
// hardest to claim for exactly the build that most wants it.
static void TriageCheck(void) {
    if (!mods.barrierTriage || triageUsed) return;
    if (hp > HpMax() * 0.20f) return;
    triageUsed = 1;
    barrierLeft += 4;
    if (barrierLeft > BARRIER_CAP) barrierLeft = BARRIER_CAP;
    barrierBreak = 0.7f;                 // borrow the shell effect as a tell
}

// FIGHTING GAME pays at every Nth combo, and the milestone resets with the
// combo. One long streak therefore pays repeatedly while a broken one pays
// nothing - the card rewards the run staying clean, which is the opposite of
// what a shield usually rewards, and the reason it can afford to have no cost.
static void ComboBarrier(void) {
    if (mods.barrierCombo <= 0) return;
    int step = combo / mods.barrierCombo;
    // Fills to two, not one: the trigger is the hardest in the family - fifty
    // notes without a mistake - so it earns the deeper ceiling. Keep this number
    // and the card text in step.
    if (step > comboBarrierAt) { comboBarrierAt = step; BarrierGain(1, 2); }
}


// A barrier eats the whole penalty - the damage, the combo break, the lot - and
// it covers traps as well as misses. It covers everything on purpose: a shield
// with an exception list makes the player work out which mistakes are insured
// mid-bar, and the answer arrives long after the moment they needed it.
static void Damage(float beat, float amount) {
    if (burstLeft > 0.0f) return;        // a burst protects the combo too
    if (barrierLeft > 0 && state == PLAYING) {
        barrierLeft--;
        barrierBreak = 0.7f;
        PlaySound(twistSnd);
        SetJudge("BLOCKED", (Color) { 140, 220, 255, 255 });
        // REFIT starts its clock the moment a shell is spent, not when the card
        // is taken - it rebuilds what was lost, so there has to be a loss first.
        if (mods.barrierRefit > 0 && refitCountdown < 0) refitCountdown = mods.barrierRefit;
        return;                          // the combo survives too: nothing happened
    }
    combo = 0;                           // otherwise the combo always breaks
    comboBarrierAt = 0;                  // and the FIGHTING GAME milestone with it
    if (state != PLAYING) return;        // the bot's mistakes cost nothing
    PlaySound(dmgSnd);
    SetJudge("MISS", (Color) { 255, 90, 90, 255 });
    missFlash = 1.0f;
    shake = SHAKE_MISS;
    hp -= amount * mods.dmgMul;

    if (hp <= 0.0f) {
        hp = 0.0f;
        EndRun(DEAD, beat);
    }
    // After the death check, not before: a shell handed to a player who is
    // already dead is not insurance, it is a receipt.
    TriageCheck();
}
// True while the game is the one making noise: the silent bar at the head of a
// level, or a call bar. Stray presses cost nothing here - this is where the
// player is learning the bar, not performing it, and tapping along to the call
// is exactly the habit the game wants to encourage.
//
// Only stray presses are excused. A note that got missed still charges its
// damage when it rams the camera, even if the impact lands inside a call bar -
// otherwise the last notes of every response bar would be free.
static int InCallPhase(float beat) {
#if CALL_RESPONSE
    float local = beat - FIRST_CHUNK_BEAT;
    if (local < 0.0f) return 1;
    local = fmodf(local, (float)CHUNK_TOTAL);
    if (local < (float)LEVEL_REST_BEATS) return 1;
    int bar = (int)((local - (float)LEVEL_REST_BEATS) / (float)BEATS_PER_BAR);
    return (bar % 2) == 0;
#else
    (void)beat;
    return 0;               // no call bars means no safe window
#endif
}

// Attract-mode player: presses exactly when a note crosses the gate. Only the
// input source differs from a human run, everything else is the same code.
static int BotPress(float from, float to, int lane) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive || enemies[i].lane != lane) continue;
        if (enemies[i].trap) continue;          // the bot knows better
        if (enemies[i].hitBeat > from && enemies[i].hitBeat <= to) return 1;
    }
    return 0;
}

// ---- persistence ---------------------------------------------------------
// One file next to the exe, holding scores and settings together. Two files
// would be two things to remember to leave out of the shipped package.
//
// The magic number is what makes it safe to grow: a file written by an older
// build simply fails the check and the defaults stand, rather than being read
// as garbage settings.
#define SAVE_FILE "save.dat"
// Each version appends to the end, so every older file is a byte-for-byte
// prefix of the current one: the fields it does have sit at the same offsets
// and only the new ones fall back to defaults. Rejecting old files outright
// would be shorter and would throw away the player's high scores and wallet.
//
// V1 settings only. V2 added the language. V3 added the shop.
#define SAVE_MAGIC    0x4C425437u       // 'LBT7', current
#define SAVE_MAGIC_V6 0x4C425436u       // 'LBT6', before the pad options
#define SAVE_MAGIC_V5 0x4C425435u       // 'LBT5', before the best level
#define SAVE_MAGIC_V4 0x4C425434u       // 'LBT4', before the monitor choice
#define SAVE_MAGIC_V3 0x4C425433u       // 'LBT3', before skins and the gacha
#define SAVE_MAGIC_V2 0x4C425432u       // 'LBT2', before the shop
#define SAVE_MAGIC_V1 0x4C425431u       // 'LBT1', before the language setting
#define HISCORE_FILE "hiscore.dat"      // pre-settings builds; read once, never written

typedef struct {
    unsigned int magic;
    int hi[2];
    int volStep;
    int reducedMotion;
    int borderless;
    int mode;
    int lang;                   // V2 and later
    int wallet;                 // V3 and later
    int shopOffer[SHOP_CARDS];  // -1 = nothing dealt yet
    int shopRerolls;            // saved, or quitting would reset the price
    int pendCard;
    int pendHp;
    int pendBurst;
    int skinOwned;              // V4 and later - bit per skin
    int skinSel;
    int pendCard2;              // second run-start card, only won from the gacha
    int monitor;                // V5 and later - which display to open on

    // V6 and later. Endless has no ending, so how far you got is as much of a
    // record as the score - and the two do not always move together.
    int hiLevels[2];

    // V7 and later. Which glyphs the pad prompts wear, and which shoulder
    // fires the burst. Neither changes how the game plays - they change what
    // the screen calls the buttons - but a player who set them once should
    // never be asked again.
    int padFace;
    int padSwap;
} SaveData;
#define SAVE_SIZE_V1 ((int)(offsetof(SaveData, lang)))
#define SAVE_SIZE_V2 ((int)(offsetof(SaveData, wallet)))
#define SAVE_SIZE_V3 ((int)(offsetof(SaveData, skinOwned)))
#define SAVE_SIZE_V4 ((int)(offsetof(SaveData, monitor)))
#define SAVE_SIZE_V5 ((int)(offsetof(SaveData, hiLevels)))
#define SAVE_SIZE_V6 ((int)(offsetof(SaveData, padFace)))

// Which display the game opens on. Stored as an index rather than derived from
// the window each time: GetCurrentMonitor() reads back from the window's
// position, so it answers "where did the window end up", which is not the same
// question as "where does the player want it" once the window has been moved
// by anything else.
static int monitorSel;

static void ForceComposited(void);

// Put the window on monitor m, in whichever window mode is current.
static void ApplyMonitor(int m) {
    if (m < 0 || m >= GetMonitorCount()) m = 0;
    monitorSel = m;
    Vector2 mp = GetMonitorPosition(m);
    if (IsWindowState(FLAG_WINDOW_UNDECORATED)) {
        SetWindowSize(GetMonitorWidth(m), GetMonitorHeight(m) + 2);
        SetWindowPosition((int)mp.x, (int)mp.y);
        ForceComposited();       // the layered flag does not survive the move
    }
    else {
        SetWindowPosition((int)mp.x + (GetMonitorWidth(m) - STAGE_W) / 2,
            (int)mp.y + (GetMonitorHeight(m) - STAGE_H) / 2);
    }
}

// Keep the window in DWM composition, so a screen capturer can see it.
//
// A borderless window is undecorated and covers the whole monitor, and Windows
// treats that shape as a candidate for taking out of composition: the window
// presents straight to scan-out and its content in the composed desktop stops
// updating. A recorder or a Discord share then shows one frozen frame for as
// long as the game holds focus, and recovers a moment after focus moves away
// and DWM recomposes. That focus dependence is what identified it.
//
// A layered window cannot take that path - DWM has to composite it - so this
// is the switch that actually settles it. Confirmed against a real Discord
// share; the window-size change below was not enough on its own.
//
// windows.h collides with raylib on Rectangle, CloseWindow, DrawText and more,
// so the three entry points used here are declared directly.
__declspec(dllimport) long long __stdcall GetWindowLongPtrA(void*, int);
__declspec(dllimport) long long __stdcall SetWindowLongPtrA(void*, int, long long);
__declspec(dllimport) int __stdcall SetLayeredWindowAttributes(void*, unsigned long,
                                                               unsigned char, unsigned long);
static void ForceComposited(void) {
    void* h = GetWindowHandle();
    if (!h) return;
    long long ex = GetWindowLongPtrA(h, -20);            // GWL_EXSTYLE
    SetWindowLongPtrA(h, -20, ex | 0x00080000L);         // WS_EX_LAYERED
    SetLayeredWindowAttributes(h, 0, 255, 0x00000002);   // LWA_ALPHA, fully opaque
}

// The window is also made two pixels taller than the monitor, so its rect does
// not match the monitor rect. That is a second, weaker guard than the layered
// flag above, and it is kept because it fixes a different set of capturers:
// measured with the game drawing its attract loop, 50 samples 100ms apart,
// counting how many differ from the one before.
//
//                        exact size, focused    +2px, focused
//     desktop BitBlt            0 / 49            49 / 49
//     PrintWindow               0 / 49            49 / 49
//     window DC BitBlt          0 / 49            49 / 49
//
// Those are the GDI capture paths. Discord does not use them - it kept showing
// a frozen frame with this change alone - so do not read the table as the fix.
// It covers older recorders; ForceComposited covers the rest.
//
// Two pixels and not one: the blit centres the stage with
// (sh - STAGE_H*k) * 0.5f, so an odd number of spare pixels puts the whole
// screen on a half-pixel offset, and this is a pixel font sampled bilinearly.
// An even number keeps that offset whole on every 16:9 resolution. The spare
// rows fall off the bottom edge and are never seen.
static void ApplyBorderless(int on) {
    int m = monitorSel;
    if (m < 0 || m >= GetMonitorCount()) m = 0;
    Vector2 mp = GetMonitorPosition(m);
    if (on && !IsWindowState(FLAG_WINDOW_UNDECORATED)) {
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(GetMonitorWidth(m), GetMonitorHeight(m) + 2);
        SetWindowPosition((int)mp.x, (int)mp.y);
        ForceComposited();
    }
    else if (!on && IsWindowState(FLAG_WINDOW_UNDECORATED)) {
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(STAGE_W, STAGE_H);
        SetWindowPosition((int)mp.x + (GetMonitorWidth(m) - STAGE_W) / 2,
            (int)mp.y + (GetMonitorHeight(m) - STAGE_H) / 2);
    }
}

static void SaveAll(void) {
    SaveData s;
    s.magic = SAVE_MAGIC;
    s.hi[0] = hiScores[0];
    s.hi[1] = hiScores[1];
    s.volStep = volStep;
    s.reducedMotion = reducedMotion;
    s.borderless = IsWindowState(FLAG_WINDOW_UNDECORATED) ? 1 : 0;
    s.mode = (int)mode;
    s.lang = lang;
    s.wallet = wallet;
    for (int i = 0; i < SHOP_CARDS; i++) s.shopOffer[i] = shopOffer[i];
    s.shopRerolls = shopRerolls;
    s.pendCard = pendCard;
    s.pendHp = pendHp;
    s.pendBurst = pendBurst;
    s.skinOwned = (int)skinOwned;
    s.skinSel = skinSel;
    s.pendCard2 = pendCard2;
    s.monitor = monitorSel;
    s.hiLevels[0] = hiLevels[0];
    s.hiLevels[1] = hiLevels[1];
    s.padFace = padFace;
    s.padSwap = padSwap;
    SaveFileData(SAVE_FILE, &s, (int)sizeof(s));
}

static void LoadAll(void) {
    hiScores[0] = hiScores[1] = 0;
    hiLevels[0] = hiLevels[1] = 0;

    int size = 0;
    unsigned char* d = LoadFileData(SAVE_FILE, &size);
    if (d) {
        unsigned int magic = (size >= (int)sizeof(unsigned int))
            ? *(unsigned int*)d : 0u;
        int v7 = (magic == SAVE_MAGIC    && size == (int)sizeof(SaveData));
        int v6 = (magic == SAVE_MAGIC_V6 && size == SAVE_SIZE_V6);
        int v5 = (magic == SAVE_MAGIC_V5 && size == SAVE_SIZE_V5);
        int v4 = (magic == SAVE_MAGIC_V4 && size == SAVE_SIZE_V4);
        int v3 = (magic == SAVE_MAGIC_V3 && size == SAVE_SIZE_V3);
        int v2 = (magic == SAVE_MAGIC_V2 && size == SAVE_SIZE_V2);
        int v1 = (magic == SAVE_MAGIC_V1 && size == SAVE_SIZE_V1);
        if (v1 || v2 || v3 || v4 || v5 || v6 || v7) {
            SaveData* s = (SaveData*)d;
            hiScores[0] = s->hi[0];
            hiScores[1] = s->hi[1];
            volStep = s->volStep;
            reducedMotion = s->reducedMotion ? 1 : 0;
            mode = (s->mode == (int)MODE_HARD) ? MODE_HARD : MODE_NORMAL;
            // Before the window mode: ApplyBorderless sizes to monitorSel.
            if (v5 || v6 || v7) monitorSel = (s->monitor >= 0 && s->monitor < GetMonitorCount())
                ? s->monitor : 0;
            if (s->borderless) ApplyBorderless(1);
            else if (monitorSel != 0) ApplyMonitor(monitorSel);
            // Each field is only read from a file long enough to hold it.
            // Reading past the version that introduced it would read past the
            // end of the allocation.
            if (v2 || v3 || v4 || v5 || v6 || v7) lang = (s->lang == LANG_EN) ? LANG_EN : LANG_KO;
            if (v3 || v4 || v5 || v6 || v7) {
                wallet = s->wallet < 0 ? 0 : s->wallet;
                for (int i = 0; i < SHOP_CARDS; i++) {
                    int c = s->shopOffer[i];
                    shopOffer[i] = (c >= 0 && c < CARD_COUNT) ? c : -1;
                }
                shopRerolls = s->shopRerolls < 0 ? 0 : s->shopRerolls;
                pendCard = (s->pendCard >= 0 && s->pendCard < CARD_COUNT)
                    ? s->pendCard : -1;
                pendHp = s->pendHp ? 1 : 0;
                pendBurst = s->pendBurst ? 1 : 0;
            }
            if (v6 || v7) {
                hiLevels[0] = s->hiLevels[0] < 0 ? 0 : s->hiLevels[0];
                hiLevels[1] = s->hiLevels[1] < 0 ? 0 : s->hiLevels[1];
            }
            if (v7) {
                padFace = (s->padFace >= 0 && s->padFace < PADFACE_COUNT)
                    ? s->padFace : PADFACE_XBOX;
                padSwap = s->padSwap ? 1 : 0;
            }
            if (v4 || v5 || v6 || v7) {
                // Bit 0 is forced on: the default skin must always be owned
                // or the select screen has nothing to select.
                skinOwned = (unsigned int)s->skinOwned | 1u;
                skinSel = (s->skinSel >= 0 && s->skinSel < SKIN_COUNT
                    && SkinOwned(s->skinSel)) ? s->skinSel : 0;
                pendCard2 = (s->pendCard2 >= 0 && s->pendCard2 < CARD_COUNT)
                    ? s->pendCard2 : -1;
            }
        }
        UnloadFileData(d);
    }
    else if (FileExists(HISCORE_FILE)) {
        // Carry the scores over from a build that predated settings.
        int sz = 0;
        unsigned char* o = LoadFileData(HISCORE_FILE, &sz);
        if (o) {
            if (sz >= (int)(sizeof(int) * 2)) {
                hiScores[0] = ((int*)o)[0];
                hiScores[1] = ((int*)o)[1];
            }
            else if (sz == (int)sizeof(int)) {
                hiScores[0] = *(int*)o;
            }
            UnloadFileData(o);
        }
    }

    for (int i = 0; i < 2; i++) if (hiScores[i] < 0) hiScores[i] = 0;
    if (volStep < 0) volStep = 0;
    if (volStep > VOL_STEPS) volStep = VOL_STEPS;

    // Test knobs win over the save file, or the first load would put the rack
    // and the wallet straight back where they were.
    if (TEST_ALLSKINS) skinOwned = 0xFFFFFFFFu;
    if (TEST_WALLET)   wallet = TEST_WALLET;
}

// ---- hud -----------------------------------------------------------------
// A heart is a 45-degree square with two circles capping its upper edges.
// Built from rectangle/circle calls so there is no triangle winding to get wrong.
static void DrawHeart(float cx, float cy, float a, Color c) {
    float d = a * 0.35355f;                       // a / (2 * sqrt(2))
    DrawRectanglePro((Rectangle) { cx, cy, a, a },
        (Vector2) {
        a * 0.5f, a * 0.5f
    }, 45.0f, c);
    DrawCircleV((Vector2) { cx - d, cy - d }, a * 0.5f, c);
    DrawCircleV((Vector2) { cx + d, cy - d }, a * 0.5f, c);
}

// Darkened edges. Vection is generated mostly in peripheral vision, so taking
// the edges down is the cheapest real reduction in how much the tunnel makes
// people feel like they are moving - and it flatters a dark game anyway.
static void DrawVignette(void) {
    unsigned char a = (unsigned char)(reducedMotion ? 215 : 140);
    Color dark = { 0, 0, 0, a };
    Color clear = { 0, 0, 0, 0 };
    int w = reducedMotion ? 190 : 130;

    DrawRectangleGradientH(0, 0, w, STAGE_H, dark, clear);
    DrawRectangleGradientH(STAGE_W - w, 0, w, STAGE_H, clear, dark);
    DrawRectangleGradientV(0, 0, STAGE_W, w, dark, clear);
    DrawRectangleGradientV(0, STAGE_H - w, STAGE_W, w, clear, dark);
}

static void DrawCentered(const char* txt, int y, int size, Color c) {
    int w = UiMeasure(txt, size);
    UiText(txt, (960 - w) / 2, y, size, c);
}

// The hand, with what each card actually did, paged. Shared by the pause
// screen and the result screen so there is one place to change when the deck
// grows past what a page can hold.
// Three rows in two columns. It was four rows until a card needed a fourth
// description line, and four rows of five lines do not fit above the footer.
// Paging costs one keypress; a row overlapping the row under it costs the
// player the ability to read either.
#define CARDS_PER_PAGE 6
#define CARD_ROWS 3
static int cardPage;


// ---- menu music ----------------------------------------------------------
// The screens that pause the game freeze `beat`, and `beat` is what drives the
// in-game backing - so those screens fall completely silent. This is a second,
// much smaller scheduler that runs off the wall clock instead, so it keeps
// playing while the game itself is stopped.
//
// Two moods, because the screens mean different things. The reference screens
// (help, settings, credits) get something slow with no drum: a kick under a
// help page reads as urgency, and the player is reading, not deciding. The two
// screens that follow a run (shop, results) get a pulse, because there the
// player is choosing and the screen should feel like it is going somewhere.
//
// It reuses the sounds the game already synthesises. No new audio, no bytes.
#define MENU_BPM_CALM  92.0f
#define MENU_BPM_SHOP 104.0f

#define MENU_OFF   (-1)
#define MENU_CALM  0
#define MENU_SHOP  1

static double menuT0 = -1.0;    // wall-clock origin of the running loop
static int    menuSub = -1;     // last sixteenth played
static int    menuKind = MENU_OFF;

// Silence what the loop has in the air. Starting a sound is not the same as
// owning it: the pad runs 1.6s, so leaving the help screen used to carry a
// held note out over the title music behind it. Only the long voices need
// this - the arp and hat are gone in a tenth of a second and cutting them
// would be more audible than letting them finish.
static void MenuMusicStop(void) {
    for (int i = 0; i < 4; i++) {
        StopSound(padSnd[i]);
        StopSound(bassSnd[i]);
    }
}

static void MenuMusic(int kind) {
    if (kind == MENU_OFF) {
        if (menuKind != MENU_OFF) MenuMusicStop();
        menuT0 = -1.0;
        menuSub = -1;
        menuKind = MENU_OFF;
        return;
    }

    // Restart the loop on entry and on a change of mood, so a screen always
    // opens on the downbeat instead of halfway through someone else's bar.
    if (menuT0 < 0.0 || kind != menuKind) {
        if (menuKind != MENU_OFF) MenuMusicStop();   // do not overlap two moods
        menuT0 = GetTime();
        menuSub = -1;
        menuKind = kind;
    }

    float bpm = (kind == MENU_SHOP) ? MENU_BPM_SHOP : MENU_BPM_CALM;
    int sub = (int)((GetTime() - menuT0) * (double)(bpm / 60.0f) * 4.0);
    if (sub - menuSub > 8) menuSub = sub - 1;   // never replay a backlog

    while (menuSub < sub) {
        menuSub++;
        int s = menuSub;
        int inBar = s & 15;

        if (kind == MENU_CALM) {
            // The pad is what makes this music rather than a metronome. It is
            // 1.6s long against a 1.04s bar, so each one is still sounding when
            // the next arrives and the loop never actually stops.
            //
            // The first version had none, and a quarter-note pluck at 84 BPM
            // left six tenths of a second of silence between every note.
            if (inBar == 0) PlaySound(padSnd[(s >> 4) & 3]);
            if (inBar == 8) PlaySound(padSnd[((s >> 4) + 2) & 3]);

            // Bass on the half bar, and a falling figure on the eighths. Over
            // a sustaining pad these read as motion, not as separate events.
            if (inBar == 0 || inBar == 8) PlaySound(bassSnd[(s >> 4) & 3]);
            if ((s & 1) == 0) PlaySound(arpSnd[3 - ((s >> 1) & 3)]);
        }
        else {
            if ((s & 7) == 0) PlaySound(kickSnd);
            if ((s & 3) == 2) PlaySound(hatSnd);
            if (inBar == 0 || inBar == 6 || inBar == 12)
                PlaySound(bassSnd[((s >> 4) + 2) & 3]);
            if ((s & 1) == 0) PlaySound(arpSnd[(s >> 1) & 3]);
        }
    }
}


// One vertical centre for the reel and for every result that replaces it.
// DrawCentered anchors the TOP of the text, so three font sizes at three
// hand-picked y values do not line up - the result appeared to jump upward the
// moment the reel stopped. Deriving y from the size fixes it for every size.
// Where a pull result is centred: midway between the shop's one-run notice
// (ending at 114) and the "pull once" row at 206, with room left above the
// headline for the coins footnote that hangs there on a duplicate skin.
#define GACHA_MID 168
static void GachaHeadline(const char* s, int size, Color c) {
    DrawCentered(s, GACHA_MID - size / 2, size, c);
}

// Shop page two: the gacha and the skin rack. Split out because page one was

// One caret for every list row in the shop, so the marker is the same shape and
// the same place on both pages.
static void ShopCaret(int on, int x, int y, int fs) {
    if (on) UiText(">", x, y, fs, (Color) { 255, 210, 90, 255 });
}

// already the longest draw block in the file, not because it stands alone.
static void DrawShopGacha(Color gold, Color dead, Color body) {
    Color win = { 150, 220, 175, 255 };
    Color loss = { 225, 130, 135, 255 };
    Color faint = { 120, 120, 145, 255 };

    // The odds are published rather than hidden. A machine whose numbers you
    // cannot see is a machine you assume is lying to you.
    if (gachaOdds) {
        DrawRectangle(60, 128, 840, 280, (Color) { 16, 16, 22, 250 });
        DrawRectangleLines(60, 128, 840, 280, (Color) { 110, 100, 170, 255 });
        DrawCentered(T("ODDS"), 142, 24, (Color) { 200, 190, 255, 255 });
        for (int i = 0; i < GACHA_SLOTS; i++) {
            // Seven to a column, not eight. Thirteen slots split 7/6 leaves the
            // panel with room under the last row; 8/5 pushed it to the border.
            int col = i / 7, row = i % 7;
            int x = 96 + col * 410;
            int y = 184 + row * 31;
            UiText(gachaTable[i].label, x, y, 17, body);
            UiText(TextFormat("%d.%d%%", gachaTable[i].weight / 10, gachaTable[i].weight % 10),
                x + 320, y, 17, gold);
        }
        DrawCentered(T(PROMPT("ESC  close", "(#FR)  close")), 424, 16, faint);
        return;
    }

    // The result sits above the button that produced it, so the eye lands on
    // the outcome first and the button is where the hand already is.
    if (gachaTimer > 0.0f) {
        // Names flicker past on a decelerating tick, so the wait reads as the
        // machine deciding rather than as the game being slow.
        float k = gachaTimer / GACHA_SPIN;                 // 1 -> 0
        int tick = (int)(GetTime() * (6.0 + 26.0 * (double)k));
        GachaHeadline(gachaTable[(tick * 5) % GACHA_SLOTS].label, 28,
            (Color) { 255, (unsigned char)(200 + 55 * k), 90, 255 });
    }
    else if (gachaSlot >= 0) {
        // The headline size is settled first, because the footnote hangs off
        // it - the line above has to know how tall the thing below it is.
        int hs = (gachaCardA >= 0 && gachaCardB < 0) || gachaSkin >= 0 ? 30 : 26;

        // Only when the row has not already said it. A row that carries its
        // own amount - "+3 coins" - would otherwise print the same sentence
        // twice. A duplicate skin pays coins the row does not mention, so
        // that one still needs the line.
        //
        // ABOVE the result, not below. It is a footnote on the outcome, and a
        // footnote hung underneath a headline reads as a second, quieter
        // result - which is the one thing it is not.
        if (gachaCoins != 0 && gachaTable[gachaSlot].coins == 0) {
            const char* line = (gachaSkin >= 0)
                ? TextFormat(T("duplicate (+%d coin)"), gachaCoins)
                : TextFormat(gachaCoins > 0 ? T("+%d coin") : T("-%d coin"),
                             gachaCoins > 0 ? gachaCoins : -gachaCoins);
            DrawCentered(line, GACHA_MID - hs / 2 - 26, 18,
                gachaCoins > 0 ? gold : loss);
        }

        if (gachaCardB >= 0) {
            GachaHeadline(TextFormat(T("%s and %s"),
                T(cards[gachaCardA].name), T(cards[gachaCardB].name)), hs, win);
        }
        else if (gachaCardA >= 0) GachaHeadline(cards[gachaCardA].name, hs, win);
        else if (gachaSkin >= 0)  GachaHeadline(skins[gachaSkin].name, hs, win);
        else GachaHeadline(gachaTable[gachaSlot].label, hs,
            gachaCoins < 0 ? loss : win);
    }

    ShopCaret(shopPage && shopSel == SG_PULL, 56, 206, 20);
    UiText("pull once", 100, 206, 20, wallet >= PRICE_GACHA ? body : dead);
    UiText(TextFormat(T("%d coin"), PRICE_GACHA), 300, 206, 20,
        wallet >= PRICE_GACHA ? gold : dead);
    ShopCaret(shopPage && shopSel == SG_ODDS, 56, 238, 16);
    UiText("see the odds", 100, 238, 16, faint);

    // The rack. Locked skins are drawn as an empty outline - the shape of what
    // is missing is more of a pull than a blank space would be.
    UiText("SKIN", 56, 280, 20, gold);
    UiText("LEFT / RIGHT to change", 140, 282, 16, faint);
    for (int i = 0; i < SKIN_COUNT; i++) {
        int x = 56 + i * 104;
        int owned = SkinOwned(i);
        DrawRectangle(x, 314, 84, 46, owned ? skins[i].hull : (Color) { 26, 26, 30, 255 });
        if (i == skinSel) {
            // Three nested outlines and a wider box. One thin line the same
            // weight as every other border was not visible at a glance, which
            // is the only job this marker has.
            Color sel = { 255, 230, 140, 255 };
            for (int k = 0; k < 3; k++)
                DrawRectangleLines(x - 3 + k, 311 + k, 90 - 2 * k, 52 - 2 * k, sel);
        }
        else {
            DrawRectangleLines(x, 314, 84, 46,
                owned ? skins[i].trim : (Color) { 62, 62, 70, 255 });
        }
        if (owned) DrawRectangle(x + 8, 344, 68, 6, skins[i].trim);
        UiText(owned ? skins[i].name : "???", x + 6, 368, 14,
            (i == skinSel) ? (Color) { 255, 230, 140, 255 } : owned ? body : dead);
    }
    UiText(TextFormat(T("%d of %d unlocked"), SkinOwnedCount(), SKIN_COUNT),
        56, 396, 16, faint);

    // The door to the other page, as a row. A key nobody would guess is worse
    // than one more line in a list they are already reading. Centred, matching
    // the same row on the card page.
    {
        const char* lbl = T("buy cards");
        int lw = UiMeasure(lbl, 18);
        int px = (960 - lw) / 2;
        ShopCaret(shopPage && shopSel == SG_TOCARDS, px - 26, 434, 18);
        UiText(lbl, px, 434, 18,
            (shopSel == SG_TOCARDS) ? (Color) { 200, 190, 255, 255 } : faint);
    }
}

// Which loop, if any, belongs to the screen currently on top.
static int MenuMusicFor(GameState st, int pausedNow) {
    if (st == HELP || st == SETTINGS || st == CREDITS) return MENU_CALM;
    if (st == SHOP || st == DEAD || st == WON)         return MENU_SHOP;
    (void)pausedNow;      // pause keeps its silence: the run is still mid-flight
    return MENU_OFF;
}

// Draws the pause / result list. Same caret, same colours and the same builder
// as the input, so what is highlighted is what will happen.
static void DrawRunMenu(int y, int step, int fs) {
    int rows[4];
    int n = RunMenu(rows);
    Color pick = { 255, 210, 90, 255 };
    Color idle = { 150, 150, 172, 255 };
    for (int i = 0; i < n; i++) {
        const char* s = T(RunMenuLabel(rows[i]));
        int w = UiMeasure(s, fs);
        int x = (960 - w) / 2;
        UiText(s, x, y + i * step, fs, (i == runSel) ? pick : idle);
        if (i == runSel) UiText(">", x - 26, y + i * step, fs, pick);
    }
}


static void DrawCardPanel(void) {
    int uniq = 0;
    for (int i = 0; i < CARD_COUNT; i++) if (cardCount[i]) uniq++;
    int pages = (uniq + CARDS_PER_PAGE - 1) / CARDS_PER_PAGE;
    if (pages < 1) pages = 1;
    if (cardPage >= pages) cardPage = pages - 1;
    if (cardPage < 0) cardPage = 0;

    // Near-opaque. At 235 the result screen underneath stayed legible through
    // it, and two full screens of text competing for the same pixels is worse
    // than either alone.
    DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 248 });
    DrawCentered("CARDS TAKEN", 34, 32, (Color) { 200, 190, 255, 255 });

    int seen = 0, drawn = 0;
    for (int i = 0; i < CARD_COUNT; i++) {
        if (!cardCount[i]) continue;
        if (seen++ < cardPage * CARDS_PER_PAGE) continue;
        if (drawn >= CARDS_PER_PAGE) break;

        int x = 70 + (drawn / CARD_ROWS) * 440;
        int y = 92 + (drawn % CARD_ROWS) * 106;
        const char* nm = (cardCount[i] > 1)
            ? TextFormat("%s x%d", T(cards[i].name), cardCount[i])
            : cards[i].name;
        UiText(nm, x, y, 21, (Color) { 205, 195, 255, 255 });
        DrawCardDesc(&cards[i], x, y + 26, 15, 17, 0, 410);
        drawn++;
    }

    DrawCentered(TextFormat(T("%d cards      score x%.2f"), takenTotal, mods.scoreMul),
        438, 18, (Color) { 170, 155, 235, 255 });
    if (pages > 1) {
        DrawCentered(TextFormat(T("page %d / %d      LEFT / RIGHT"), cardPage + 1, pages),
            466, 15, (Color) { 110, 110, 135, 255 });
    }
    DrawCentered(PROMPT("ESC to go back", "(#FR) to go back"), 496, 15, (Color) { 110, 110, 135, 255 });
}


// What the next run will start holding. Drawn on the title screen because that
// is the last thing between the player and the run: a purchase made before a
// phone call is otherwise invisible from here, and they would find out only by
// noticing their health bar was longer than usual.
//
// Draws nothing at all when there is nothing pending, so the title stays clean
// for anyone who has not been to the shop.

// Every card in the deck, eight to a page. Two audiences: a player deciding
// what to hope for in a draft, and whoever is tuning the deck - laid out the
// same way a draft card is, so a benefit sitting on a cost line is visible at
// a glance rather than only in the source.
#define CODEX_PER_PAGE 8
#define CODEX_ROWS 4

static void DrawCodex(void) {
    int pages = (CARD_COUNT + CODEX_PER_PAGE - 1) / CODEX_PER_PAGE;
    if (codexPage >= pages) codexPage = pages - 1;
    if (codexPage < 0) codexPage = 0;

    DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 235 });
    DrawCentered("CARD LIST", 26, 30, (Color) { 200, 190, 255, 255 });

    // No price here. It is a reference for what the cards DO; the price only
    // means anything in front of a wallet, which is the shop.
    Color nameCol = { 205, 195, 255, 255 };

    for (int i = 0; i < CODEX_PER_PAGE; i++) {
        int idx = codexPage * CODEX_PER_PAGE + i;
        if (idx >= CARD_COUNT) break;
        const Card* cd = &cards[idx];
        int x = 50 + (i / CODEX_ROWS) * 460;
        int y = 78 + (i % CODEX_ROWS) * 96;

        UiText(cd->name, x, y, 18, nameCol);
        DrawCardDesc(cd, x + 12, y + 24, 15, 17, 0, 410);
    }

    DrawCentered(TextFormat(T("page %d / %d      LEFT / RIGHT"), codexPage + 1, pages),
        470, 15, (Color) { 110, 110, 135, 255 });
    DrawCentered(PROMPT("ESC to go back", "(#FR) to go back"), 498, 17, (Color) { 110, 110, 135, 255 });
}

static void DrawPendingPanel(int x, int y) {
    if (pendCard < 0 && pendCard2 < 0 && !pendHp && !pendBurst) return;

    Color head = { 255, 210, 90, 255 };
    Color name = { 205, 195, 255, 255 };
    Color item = { 150, 210, 175, 255 };

    UiText("carrying into the next run", x, y, 15, head);
    y += 26;

    for (int s = 0; s < 2; s++) {
        int c = s ? pendCard2 : pendCard;
        if (c < 0) continue;
        UiText(cards[c].name, x + 10, y, 18, name);
        y += 23;
        DrawCardDesc(&cards[c], x + 20, y, 14, 17, 0, 280);
        const char* l[4] = { cards[c].d1, cards[c].d2, cards[c].d3, cards[c].d4 };
        int n = 0;
        while (n < 4 && l[n]) n++;
        y += n * 17 + 8;
    }

    if (pendHp)    { UiText(hpItemName[pendHp - 1], x + 10, y, 15, item); y += 22; }
    if (pendBurst) { UiText(burstItemName[pendBurst - 1], x + 10, y, 15, item); }
}

int main(void) {
    // Present on the display's own clock, not on a timer of our own.
    //
    // raylib defaults to glfwSwapInterval(0), and SetTargetFPS(60) then paced
    // the loop with a sleep-and-spin. On a 60Hz display that is close enough
    // to invisible; on the 144Hz display this was tested on it means every
    // frame is held for either two or three refreshes (144/60 = 2.4, never a
    // whole number) and each swap lands at an arbitrary phase of the
    // compositor's cycle. Anything that samples the composited desktop -
    // screen recording, a Discord share - is then resampling a source that
    // never lines up with its own clock, which aliases into the heavy
    // stuttering the stream showed while the game itself looked fine locally.
    //
    // One frame of this game costs 0.47ms measured (2100fps uncapped), so
    // running at the refresh rate instead of at 60 spends about 3% of a
    // 144Hz frame budget. Nothing in the game counts frames - every quantity
    // is in beats and multiplied by time last - so the rate is free to float.
    //
    // The cap stays only as a safety net: a driver set to force vsync off
    // would otherwise leave the loop free-running at 2000fps. At twice the
    // refresh rate it never engages while vsync is working.
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(STAGE_W, STAGE_H, "Rhythm Tunnel");
    {
        int hz = GetMonitorRefreshRate(GetCurrentMonitor());
        SetTargetFPS(hz > 0 ? hz * 2 : 120);
    }
    SetExitKey(KEY_NULL);      // ESC is pause / back, not quit

    RenderTexture2D stage = LoadRenderTexture(STAGE_W, STAGE_H);
    SetTextureFilter(stage.texture, TEXTURE_FILTER_BILINEAR);

    // The UI draws text between 15px and 56px, so the atlas is scaled either
    // way whatever size it is baked at. Bilinear is the honest choice: point
    // sampling only pays off when every draw is an exact whole multiple of the
    // baked size, and pinning twenty hand-placed layouts to multiples of 32
    // would cost more than the sharpness is worth.
    uiFontS = LoadFont_Neodgm16();
    uiFontL = LoadFont_Neodgm32();
    fontReady = (uiFontS.glyphCount > 0 && uiFontS.texture.id != 0
              && uiFontL.glyphCount > 0 && uiFontL.texture.id != 0);
    if (fontReady) {
        SetTextureFilter(uiFontS.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(uiFontL.texture, TEXTURE_FILTER_BILINEAR);
    }

    int quit = 0;

    InitAudioDevice();
    kickSnd = MakeKick();
    dmgSnd = MakeDamage();
    levelSnd = MakeLevelUp();
    twistSnd = MakeTwist();
    hatSnd = MakeHat();
    denySnd = MakeDeny();
#if PERFECT_SOUND == 5
    for (int i = 0; i < LANE_COUNT; i++) perfSnd[i] = MakePerfectLane(laneFreq[i]);
#elif PERFECT_SOUND == 6
    perfSnd = MakePerfectSnap();
#elif PERFECT_SOUND == 9
    perfSnd = MakePerfectSnapTone();
#elif PERFECT_SOUND == 10
    perfSnd = MakePerfectFourth();
#elif PERFECT_SOUND == 7
    for (int i = 0; i < LANE_COUNT; i++) hitSndDull[i] = MakeToneDull(laneFreq[i]);
#elif PERFECT_SOUND == 8
    for (int i = 0; i < COMBO_STEPS; i++) perfSnd[i] = MakePerfectStep(i);
#else
    perfSnd = MakePerfect();
#endif
    for (int i = 0; i < 4; i++) {
        bassSnd[i] = MakeBass(bassFreq[i]);
        arpSnd[i] = MakeArp(arpFreq[i]);
        padSnd[i] = MakePad(padFreq[i]);
    }
    for (int i = 0; i < LANE_COUNT; i++) {
        callSnd[i] = MakeTone(laneFreq[i], 0);
        hitSnd[i] = MakeTone(laneFreq[i], 1);
    }

    SetMasterVolume((float)volStep / VOL_STEPS);

    // The backing is background: it sits well under the call, the hit and the
    // damage, which are the sounds the player is actually reading.
    SetSoundVolume(kickSnd, 0.45f);
    // Also the gacha reel tick and the draft skip. Raised from 0.22 for the
    // menu cursor, which was inaudible next to the mode-change tone.
    SetSoundVolume(hatSnd, 0.38f);
    // The warning lands during a call bar, so it has to sit under the call
    // rather than fight it. Its job is "something is different", not detail.
    SetSoundVolume(twistSnd, 0.55f);
    for (int i = 0; i < 4; i++) {
        SetSoundVolume(bassSnd[i], 0.32f);
        SetSoundVolume(arpSnd[i], 0.18f);
    }

    // The shop stream is NOT seeded anywhere else, and xorshift is a pure
    // function of its state - so with the fixed 1 it started on, every launch
    // replayed the same sequence and the first gacha pull of a session always
    // gave the same thing. The chart stream is reproducible on purpose; this
    // one has no reason to be.
    shopRng = (unsigned int)time(NULL) * 2654435761u;
    if (!shopRng) shopRng = 1;                 // xorshift must not start at zero

    LoadAll();
    SetMasterVolume((float)volStep / VOL_STEPS);   // LoadAll may have changed it
    StartRun((unsigned int)time(NULL));

    Camera3D cam = { 0 };
    cam.up = (Vector3){ 0, 1, 0 };
    cam.fovy = 70.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose() && !quit) {
        // DEAD, WON, DRAFT and PAUSED all freeze the scene; only the reason differs.
        int running = (state == PLAYING || state == TITLE) && !paused;
        float dt = GetFrameTime();

        float beat;
        if (state == DEAD || state == WON) beat = freezeBeat;
        else if (paused || state == DRAFT || state == HELP || state == SETTINGS
            || state == SHOP || state == CODEX
            || state == CREDITS)
            beat = baseBeat;
        else beat = baseBeat + (float)(GetTime() - baseTime) * (curBpm / 60.0f);
        float camZ = beat * SPACING;
        float gateZ = camZ + HIT_OFFSET;
        float playerZ = camZ + PLAYER_Z;

        float frac = beat - (int)beat;
        float pulse = 1.0f - frac;
        if (pulse < 0.0f) pulse = 0.0f;

        int press[LANE_COUNT] = { 0, 0, 0, 0 };

        if (running) {
            // Level up when the playhead crosses a level boundary. Rebasing
            // here is what keeps `beat` continuous while the tempo changes.
            int want = (int)((beat - FIRST_CHUNK_BEAT) / (CHUNK_TOTAL * LEVEL_CHUNKS));
            if (beat < FIRST_CHUNK_BEAT) want = 0;
            if (want > level && want < LevelCount()) {
                baseBeat = beat;
                baseTime = GetTime();
                ApplyLevel(want);

                // Granted here and not in ApplyLevel, because TakeCard calls
                // ApplyLevel too - to re-derive the level from the new mods -
                // and a shield handed out every time a card is picked is not
                // what "on level up" says.
                //
                // The cap is on the total, not on the grant, so this tops the
                // player up rather than stacking without end. HUBRIS can still
                // push past it; what it cannot do is be topped back up to four.
                if (mods.levelBarrier > 0) BarrierGain(1, LEVEL_BARRIER_CAP);

                // REFIT counts down only while it is armed, so the card does
                // nothing at all until the player has actually lost a shell.
                if (refitCountdown > 0) {
                    refitCountdown--;
                    if (refitCountdown == 0) { BarrierGain(1, 1); refitCountdown = -1; }
                }
                if (state == PLAYING) {
                    PlaySound(levelSnd);
                    levelBanner = 1.6f;

                    // Draft on the way into levels 2, 5, 8, ... The playhead is
                    // already rebased, so freezing here costs no beats.
                    if (IsDraftLevel(want) && want != lastDraftLevel) {
                        lastDraftLevel = want;
                        DealOffer();
                        if (offerCount > 0) state = DRAFT;
                        else deckBanner = 2.6f;              // deck exhausted
                    }
                }
            }

            // The run is finished once the playhead passes the last chunk.
            if (state == PLAYING && beat >= TotalBeats()) {
                EndRun(WON, beat);
            }

            // Keep the chart generated a little ahead of the playhead. TITLE
            // loops forever at the top level so the attract mode never runs dry.
            while (nextChunkBeat - beat < LEAD_BEATS) {
                int lv = chunkIndex / LEVEL_CHUNKS;
                if (state == PLAYING && lv >= LevelCount()) break;
                if (lv > LevelCount() - 1) lv = LevelCount() - 1;
                GenerateChunk(nextChunkBeat, lv);
                nextChunkBeat += CHUNK_TOTAL;
                chunkIndex++;
            }

            // One scheduler at sixteenth resolution drives the whole backing.
            // Layers switch on with the level, so progress is audible.
            int sub = (int)(beat * 4.0f);
            if (sub - lastSub > 16) lastSub = sub - 1;   // never replay a backlog
            int mt = MusicTier(level);
            while (lastSub < sub) {
                lastSub++;
                int s = lastSub;
                int inBar = s & 15;
                // The bass loop is rotated by level block, so the harmony
                // actually moves over a long run instead of looping forever.
                int bassRot = (level >> 2) & 3;

                if ((s & 3) == 0)                      PlaySound(kickSnd);
                if (mt >= 2 && (s & 3) == 2)           PlaySound(hatSnd);
                if (mt >= 6 && (s & 3) == 0)           PlaySound(hatSnd);
                if (mt >= 3) {
                    int onBass = (mt >= 5) ? ((s & 3) == 0)
                        : (inBar == 0 || inBar == 8);
                    if (onBass) PlaySound(bassSnd[(((s >> 4) + bassRot)) & 3]);
                }
                // Tier 7 doubles the arpeggio rather than adding a voice: the same
                // figure at twice the rate reads as the track speeding up, which
                // is what a level climb should sound like.
                if (mt >= 7)                           PlaySound(arpSnd[s & 3]);
                else if (mt >= 4 && (s & 1) == 0)      PlaySound(arpSnd[(s >> 1) & 3]);
                if (mt >= 8 && (s & 1) == 1)           PlaySound(hatSnd);

                // The pad is the menu voice, borrowed. It is 1.6s long against a
                // bar of about 1.4s at this tempo, so one per bar sustains into
                // the next without the four of them fighting each other.
                if (mt >= 9 && inBar == 0)             PlaySound(padSnd[(s >> 4) & 3]);
            }

            // Call notes: the game plays the bar the player is about to play.
            for (int i = 0; i < MAX_CALLS; i++) {
                if (!calls[i].pending) continue;
                if (calls[i].beat > prevBeat && calls[i].beat <= beat) {
                    PlaySound(callSnd[calls[i].lane]);
                    callFlash[calls[i].lane] = 1.0f;
                    calls[i].pending = 0;
                }
                else if (calls[i].beat < beat - 1.0f) {
                    calls[i].pending = 0;              // never leave a stale call
                }
            }

            // TITLE runs the same simulation with the bot driving the input.
            //
            // Per-KEY edges, with a debounce measured in beats.
            //
            // Two keys are bound to each lane, and both jobs have to work at
            // once: pressing W and UP together for one note must count once,
            // and ROLLING them - press the next before releasing the last, the
            // way sixteenths are actually played - must count every one.
            //
            // Taking the edge on the lane's combined held state got the first
            // and destroyed the second: rolling never lets the lane go up, so
            // every input after the first was silently dropped. Measured, at
            // 8 presses per second: tapping fed the game 8, rolling fed it 0.
            //
            // Time tells the two apart, and beats are the right unit because
            // the tempo moves. Two inputs closer together than half a slot
            // cannot be two different notes at any tempo this game reaches -
            // sixteenths are a whole slot apart - while two hands landing on
            // one note are a few milliseconds apart, far inside it.
            for (int lane = 0; lane < LANE_COUNT; lane++) {
                if (state == PLAYING) {
                    int hit = LaneHit(lane);
                    press[lane] = hit
                        && (beat - lanePressAt[lane]) >= SLOT_BEATS * 0.5f;
                    if (press[lane]) lanePressAt[lane] = beat;
                }
                else press[lane] = BotPress(prevBeat, beat, lane);
            }
        }

        for (int lane = 0; lane < LANE_COUNT; lane++) {
            laneFlash[lane] -= dt * 6.0f;
            callFlash[lane] -= dt * 4.0f;
            if (laneFlash[lane] < 0.0f) laneFlash[lane] = 0.0f;
            if (callFlash[lane] < 0.0f) callFlash[lane] = 0.0f;
            if (press[lane]) laneFlash[lane] = 1.0f;
        }

        if (running) {
            // A burst is a bomb, not a shield. While it runs, every note that
            // reaches the gate is destroyed for the player, and anything that
            // already slipped past is vaporised where it flies.
            //
            // That second half is the important one. Before it, monsters missed
            // during a burst came back to hit the moment the burst ended - by
            // which point they were already past the gate and there was nothing
            // the player could do about it.
            if (burstLeft > 0.0f) {
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (!enemies[i].alive) continue;
                    int arrived = (enemies[i].hitBeat > prevBeat
                        && enemies[i].hitBeat <= beat);
                    if (!enemies[i].escaped && !arrived) continue;

                    float z = gateZ + (enemies[i].hitBeat - beat - enemies[i].rush) * SPACING;
                    Vector3 at = WorldPos(z, EnemyLocal(&enemies[i], beat));
                    Color c = LaneColor(enemies[i].lane);
                    int wasEscaped = enemies[i].escaped;
                    int wasTrap = enemies[i].trap;
                    int wasItem = enemies[i].item;

                    enemies[i].alive = 0;
                    PlaySound(hitSnd[enemies[i].lane]);
                    SpawnBeam(at, c);
                    SpawnBurst(at, c, 10);

                    // Score and combo, but no healing and no meter: a burst is a
                    // scoring window, not a recovery one. That is deliberate -
                    // do not add an hp line here without meaning to change it.
                    // Already-missed monsters are cleaned up, not rewarded, and
                    // a trap swept up by a burst is neither scored nor punished.
                    if (!wasEscaped && !wasTrap) {
                        combo++;
                        if (combo > maxCombo) maxCombo = combo;
                        ComboBarrier();
                        cntPerfect++;
                        int g = (int)((200 + combo * 10)
                            * mods.scoreMul * mods.burstScore);
                        score += g;

                        // A burst is a scoring window, so it collects the score
                        // items and only those. Letting it hoover up health and
                        // meter would quietly turn the burst into the recovery
                        // window the comment above says it is not.
                        if (wasItem >= 5) CollectItem(wasItem, g, at);
                    }
                }
            }

            for (int lane = 0; lane < LANE_COUNT; lane++) {
                if (!press[lane]) continue;
                int best = -1;
                float bestDist = hitWindow;

                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (!enemies[i].alive || enemies[i].escaped) continue;
                    if (enemies[i].lane != lane) continue;
                    float d = fabsf(enemies[i].hitBeat - beat);
                    if (d < bestDist) { bestDist = d; best = i; }
                }

                // Hitting a trap is worse than firing at nothing: the player
                // was fooled into an action, not merely early.
                if (best >= 0 && enemies[best].trap) {
                    enemies[best].alive = 0;
                    Damage(beat, HP_TRAP);
                    continue;
                }

                if (best >= 0) {
                    float z = gateZ + (enemies[best].hitBeat - beat - enemies[best].rush) * SPACING;
                    Vector3 at = WorldPos(z, EnemyLocal(&enemies[best], beat));
                    Color c = LaneColor(lane);

                    // Inside a burst everything lands Perfect. That is the
                    // whole payoff, so it does not also feed the meter.
                    int bursting = (burstLeft > 0.0f);
                    int perfect = bursting || (bestDist <= hitWindow * PERFECT_FRAC);
                    int gained = 0;
                    float mul = mods.scoreMul * (bursting ? mods.burstScore : 1.0f);

                    enemies[best].alive = 0;
                    combo++;
                    if (combo > maxCombo) maxCombo = combo;
                    ComboBarrier();
#if PERFECT_SOUND == 7
                    // The whole difference lives here: a Perfect gets the real
                    // hit, a Good gets the muffled one.
                    PlaySound(perfect ? hitSnd[lane] : hitSndDull[lane]);
#else
                    PlaySound(hitSnd[lane]);
#endif
                    SpawnBeam(at, c);

                    if (perfect) {
                        cntPerfect++;
                        gained = (int)((200 + combo * 10) * mul);
                        score += gained;
                        hp += BaseHealPerfect() * mods.healMul * mods.healPerfectMul;
                        SetJudge("PERFECT", (Color) { 255, 220, 80, 255 });
                        // On top of the hit, not instead of it. The player is
                        // watching the ring, not the word on screen, so the
                        // difference between the two judgements has to arrive
                        // through the ears.
#if PERFECT_SOUND == 5
                        PlaySound(perfSnd[lane]);
#elif PERFECT_SOUND == 7
                        /* nothing: the contrast is in the GOOD sound above */
#elif PERFECT_SOUND == 8
                        {
                            int st = combo / 8;
                            if (st >= COMBO_STEPS) st = COMBO_STEPS - 1;
                            PlaySound(perfSnd[st]);
                        }
#else
                        PlaySound(perfSnd);
#endif
                        SpawnBurst(at, c, 12);
                        shake = SHAKE_PERFECT;
                        if (!bursting) {
                            burst += BurstGain(enemies[best].tier) * mods.burstRate;
                        }
                    }
                    else {
                        cntGood++;
                        gained = (int)((100 + combo * 5) * mul);
                        score += gained;
                        if (!mods.healGoodZero) hp += BaseHealGood() * mods.healMul;
                        SetJudge("GOOD", (Color) { 120, 210, 255, 255 });
                        SpawnBurst(at, c, 6);
                        shake = SHAKE_GOOD;
                    }
                    // The item pays on top of the note, never instead of it -
                    // the note still had to be played, and a pickup that
                    // replaced the ordinary reward would make a lucky bar worth
                    // less than a clean one.
                    if (enemies[best].item) CollectItem(enemies[best].item, gained, at);
                    if (hp > HpMax()) hp = HpMax();
                    if (burst > 1.0f) burst = 1.0f;
                }
                else {
                    // Nothing inside the hit window. Look wider: a press that
                    // lands near a real note is that note, played wrong.
                    //
                    // BAD is a fourth judgement, not a side penalty. PERFECT
                    // and GOOD answer the note, BAD gets it wrong, MISS never
                    // answers it at all - and all four consume it, so a note is
                    // judged exactly once. That is what makes mashing lose: a
                    // mashed lane lands outside the window, takes the BAD and
                    // destroys the note, so it never earns the hit or the heal.
                    // Leaving the note alive was tried first and did not work -
                    // mashing simply hit it again on the next press and healed.
                    //
                    // Pressing where there is no note at all stays free. That
                    // is what four-key games allow and what two-handed play
                    // needs.
                    int near = -1;
                    float nearDist = hitWindow * BAD_WINDOW_MUL;
                    for (int i = 0; i < MAX_ENEMIES; i++) {
                        if (!enemies[i].alive || enemies[i].escaped) continue;
                        // Traps are meant to be left alone. Pressing near one
                        // and missing it is a dodge, not a mistake.
                        if (enemies[i].trap) continue;
                        if (enemies[i].lane != lane) continue;
                        float d = fabsf(enemies[i].hitBeat - beat);
                        if (d < nearDist) { nearDist = d; near = i; }
                    }

                    // No call-phase exemption here, deliberately.
                    //
                    // The first note of a response bar has its windows open
                    // BEFORE the bar starts: the hit window at hitBeat minus
                    // one window, the BAD window at twice that, both of which
                    // land inside the preceding call bar. Suppressing BAD
                    // there while leaving the hit path open meant that note
                    // could never be consumed by a BAD but could still be
                    // taken by a GOOD - so mashing, which BADs out every other
                    // note in the bar, collected a free GOOD on the first one
                    // every single time. A press is judged by how near it is
                    // to a note, not by which bar the clock happens to be in.
                    if (near >= 0 && burstLeft <= 0.0f) {
                        enemies[near].alive = 0;      // judged, and gone
                        cntBad++;
                        Damage(beat, BaseBad());
                        // Damage says MISS; this note was answered, just wrong.
                        if (state == PLAYING) {
                            SetJudge("BAD", (Color) { 255, 150, 60, 255 });
                        }
                    }
                }
            }

            // A missed monster is not deleted at the gate - it keeps coming and
            // rams the camera. The combo breaks the moment it slips past, but
            // the life is lost on impact, so the punishment is something you
            // watch arrive instead of a number that silently ticks down.
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!enemies[i].alive) continue;

                if (!enemies[i].escaped && beat > enemies[i].hitBeat + hitWindow) {
                    enemies[i].escaped = 1;
                    // Letting a trap through is the correct play, so it costs
                    // nothing and does not break the combo.
                    if (!enemies[i].trap && burstLeft <= 0.0f) {
                        combo = 0;
                        if (state == PLAYING) cntMiss++;
                    }
                }
                if (!enemies[i].escaped) continue;

                // An untouched trap slides out through the tunnel wall rather
                // than charging the camera.
                if (enemies[i].trap) {
                    enemies[i].outward += dt * 2.2f;
                    if (enemies[i].outward >= 1.0f) enemies[i].alive = 0;
                    continue;
                }

                // Closing faster than the chart moves. Accumulated rather than
                // applied as a speed, so the drawn position and the impact
                // test read the same number.
                enemies[i].rush += (beat - prevBeat) * (MISS_RUSH - 1.0f);

                float z = gateZ + (enemies[i].hitBeat - beat - enemies[i].rush) * SPACING;
                if (z > camZ) continue;
                enemies[i].alive = 0;
                Damage(beat, BaseMiss());
                // No impact kick during a burst - Damage already declined it,
                // and a full shake with no consequence just reads as a bug.
                if (state == PLAYING && burstLeft <= 0.0f) shake = SHAKE_IMPACT;
            }

            // Every gimmick announces itself, always at the head of its call
            // bar. A bar that behaves differently without warning is not a
            // gimmick, it is a bug the player blames themselves for.
            for (int i = 0; i < MAX_WARNS; i++) {
                if (!warns[i].pending) continue;
                if (warns[i].beat > prevBeat && warns[i].beat <= beat) {
                    PlaySound(twistSnd);
                    twistWarn = 1.0f;
                    warnText = warns[i].text;
                    warns[i].pending = 0;
                }
                else if (warns[i].beat < beat - 1.0f) {
                    warns[i].pending = 0;          // never leave a stale warning
                }
            }

            for (int i = 0; i < MAX_BEAMS; i++) {
                if (beams[i].life > 0.0f) beams[i].life -= dt * 6.0f;
            }
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (particles[i].life <= 0.0f) continue;
                particles[i].pos.x += particles[i].vel.x * dt;
                particles[i].pos.y += particles[i].vel.y * dt;
                particles[i].pos.z += particles[i].vel.z * dt;
                particles[i].life -= dt * 2.2f;
            }
            shake -= dt * 2.5f;
            missFlash -= dt * 2.0f;
            levelBanner -= dt;
            deckBanner -= dt;
            twistWarn -= dt * 1.1f;
            judgeTimer -= dt;
            burstReadyFlash -= dt;
            barrierBreak -= dt;
            {   // rising edge only: the meter filling is the news, not it being full
                int ready = (burst >= 1.0f && burstLeft <= 0.0f && !mods.burstLocked);
                if (ready && !burstWasReady) burstReadyFlash = 1.6f;
                burstWasReady = ready;
            }
            if (mods.spin) spinAngle += dt * SPIN_DEG_PER_SEC;
            if (burstLeft > 0.0f) {
                burstLeft -= (beat - prevBeat);
                if (burstLeft < 0.0f) burstLeft = 0.0f;
            }

            float tr, tg, tb;
            RingTint(level, &tr, &tg, &tb);
            // A burst washes the whole tunnel out. It is the only time the
            // level's colour is overridden, so it reads instantly.
            if (burstLeft > 0.0f) { tr = 255.0f; tg = 250.0f; tb = 210.0f; }
            float k = dt * (burstLeft > 0.0f ? 9.0f : 1.8f);
            if (k > 1.0f) k = 1.0f;
            tintR += (tr - tintR) * k;
            tintG += (tg - tintG) * k;
            tintB += (tb - tintB) * k;

            if (shake < 0.0f) shake = 0.0f;
            if (missFlash < 0.0f) missFlash = 0.0f;
            if (levelBanner < 0.0f) levelBanner = 0.0f;
            if (deckBanner < 0.0f) deckBanner = 0.0f;
            if (twistWarn < 0.0f) twistWarn = 0.0f;
            if (judgeTimer < 0.0f) judgeTimer = 0.0f;
            if (burstReadyFlash < 0.0f) burstReadyFlash = 0.0f;
            if (barrierBreak < 0.0f) barrierBreak = 0.0f;
        }

        UpdateInputKind();

        // ---- meta input ----
        // One chain, not several ifs. A screen change made in one branch must
        // not be seen by a later branch in the same frame: H opening HELP and
        // the HELP handler both firing on that one keypress closed it again
        // instantly, and the next ESC then reached the title and quit.
        if (state == TITLE && seedBox) {
            // Digits are read here and nowhere else now. The box is the only
            // place a seed can change, so there is always something on screen
            // saying it is changing.
            int ch = GetCharPressed();
            while (ch > 0) {
                if (ch >= '0' && ch <= '9') SeedPush(ch);
                ch = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) SeedBack();

            int was = seedSel;
            int r = SeedRowOf(seedSel);
            int start = SeedRowStart(r), len = SeedRowLen(r);

            if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A) || PadLeft())
                seedSel = start + (seedSel - start + len - 1) % len;
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || PadRight())
                seedSel = start + (seedSel - start + 1) % len;

            // Up and down land on whatever is nearest on screen rather than on
            // a fixed column, because the bottom row has three cells against
            // the other rows' five. Nothing lines up, so the cursor follows the
            // eye instead of the index.
            int dy = (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W) || PadUp())   ? -1
                   : (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || PadDown()) ?  1 : 0;
            if (dy) {
                int nr = (r + dy + 3) % 3;
                int ns = SeedRowStart(nr), nl = SeedRowLen(nr), best = ns;
                int x = SeedCellX(seedSel);
                for (int i = ns; i < ns + nl; i++) {
                    int d = SeedCellX(i) - x,    dd = (d < 0) ? -d : d;
                    int b = SeedCellX(best) - x, bb = (b < 0) ? -b : b;
                    if (dd < bb) best = i;
                }
                seedSel = best;
            }
            if (seedSel != was) PlaySound(hatSnd);

            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || PadConfirm()) {
                if (seedSel < SEED_DEL) SeedPush('0' + seedSel);
                else if (seedSel == SEED_DEL) SeedBack();
                else if (seedSel == SEED_CLR) {
                    if (seedLen) { seedLen = 0; seedText[0] = '\0'; PlaySound(twistSnd); }
                    else PlaySound(denySnd);
                }
                else SeedCommit();
            }
            if (IsKeyPressed(KEY_ESCAPE) || PadCancel()) SeedCancel();
        }
        else if (state == TITLE) {
            // No digit reader here any more - see the seed box above. It used
            // to sit right at the top of this branch, swallowing number keys
            // with nothing drawn to show for it.

            // ESC and cancel do nothing here. Quitting is a row on the menu, and
            // this game asks no "are you sure?" anywhere - so a cancel button
            // that closes the whole program, one slip away from a QUIT row that
            // does the same thing on purpose, is a trap with no net under it.

            // One vertical list, not a grid. Two columns would need LEFT/RIGHT
            // to mean "other column", and the whole reason this screen grew a
            // cursor is that a player with no mouse should be able to hold one
            // idea - press a direction, watch the highlight move - and reach
            // everything with it.
            int wasSel = titleSel;
            if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W) || PadUp())
                titleSel = (titleSel + TITLE_ROWS - 1) % TITLE_ROWS;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || PadDown())
                titleSel = (titleSel + 1) % TITLE_ROWS;

            // The two mode rows ARE the mode setting. Landing on one selects it,
            // so the records underneath follow the highlight without the player
            // being told that they will - and starting a run is then one press
            // on the row already under the cursor, not a mode toggle followed
            // by a separate confirm.
            int want = (titleSel == T_ENDLESS) ? MODE_HARD
                     : (titleSel == T_NORMAL)  ? MODE_NORMAL : mode;
            if (want != mode) {
                mode = want;
                // Two sounds, because two different things happen. Crossing into
                // a mode row changes what the whole screen is about and gets the
                // louder call tone; every other step is just a cursor moving and
                // gets the hat. A menu where every row sounds like a decision
                // has no way left to say that one of them IS one.
                PlaySound(callSnd[0]);
                SaveAll();
                // The attract run is mid-flight and its level index may now be
                // past the end of the other mode, so restart it cleanly.
                StartRun(ChosenSeed());
            }
            else if (titleSel != wasSel) PlaySound(hatSnd);

            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || PadConfirm()) {
                baseBeat = beat;              // the attract run freezes behind these
                switch (titleSel) {
                case T_NORMAL:
                case T_ENDLESS:
                    state = PLAYING;
                    StartRun(ChosenSeed());
                    break;
                case T_HELP:    helpPage = 0;  state = HELP; break;
                case T_SHOP:
                    if (shopOffer[0] < 0) DealShop();     // first visit ever
                    state = SHOP;
                    break;
                case T_SEED:    SeedOpen(); break;
                case T_CODEX:   codexPage = 0; state = CODEX; break;
                case T_OPTIONS: state = SETTINGS; break;
                case T_CREDITS: state = CREDITS; break;
                default:        quit = 1; break;
                }
            }
        }
        else if (state == HELP) {
            if (IsKeyPressed(KEY_ESCAPE) || PadCancel()) {
                state = TITLE;
                baseTime = GetTime();      // resume the attract where it froze
            }
            // The arrows, like every other paged screen. Q / E was tried and
            // read as arbitrary: nothing else in the game pages with letters.
            if ((IsKeyPressed(KEY_LEFT)  || PadLeft())  && helpPage > 0) helpPage--;
            if ((IsKeyPressed(KEY_RIGHT) || PadRight()) && helpPage < 2) helpPage++;
        }
        else if (state == SHOP) {
            // ESC closes the odds sheet before it closes the shop. A cancel that
            // skips past the thing actually covering the screen is a cancel the
            // player has to press twice and think about once.
            if (IsKeyPressed(KEY_ESCAPE) || PadCancel()) {
                if (gachaOdds) { gachaOdds = 0; PlaySound(hatSnd); }
                else { state = TITLE; baseTime = GetTime(); }
            }
            else if (gachaOdds) {
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || PadConfirm()) {
                    gachaOdds = 0;
                    PlaySound(hatSnd);
                }
            }
            // Everything is locked while the reel spins. Leaving the screen or
            // changing the page mid-pull would strand a roll already paid for.
            else if (gachaTimer <= 0.0f) {
                int up    = IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W) || PadUp();
                int down  = IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S) || PadDown();
                int left  = IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A) || PadLeft();
                int right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || PadRight();
                int fire  = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || PadConfirm();
                int was = shopSel;

                if (shopPage) {
                    if (up)   shopSel = (shopSel + SHOP_G_ROWS - 1) % SHOP_G_ROWS;
                    if (down) shopSel = (shopSel + 1) % SHOP_G_ROWS;

                    // Only the skin row has a value to sweep, and only owned
                    // skins can be landed on - the locked ones are drawn so the
                    // player knows they exist, not so they can sit on them.
                    if (left || right) {
                        int d = right ? 1 : -1;
                        int n = skinSel;
                        for (int i = 0; i < SKIN_COUNT; i++) {
                            n = (n + d + SKIN_COUNT) % SKIN_COUNT;
                            if (SkinOwned(n)) break;
                        }
                        if (n != skinSel) { skinSel = n; PlaySound(hatSnd); SaveAll(); }
                        else PlaySound(denySnd);
                    }

                    if (fire) {
                        switch (shopSel) {
                        case SG_PULL:
                            // Each branch tests the press first and the price
                            // second. Folding the cost into the test would make
                            // a rejected press indistinguishable from a dead
                            // one, and the player would not know they were heard.
                            if (wallet >= PRICE_GACHA) {
                                wallet -= PRICE_GACHA;
                                gachaTimer = GACHA_SPIN;
                                gachaSlot = -1;
                                PlaySound(twistSnd);
                                SaveAll();
                            }
                            else PlaySound(denySnd);
                            break;
                        case SG_ODDS: gachaOdds = 1; PlaySound(hatSnd); break;
                        default:
                            shopPage = 0; shopSel = SC_CARD0;
                            gachaSlot = -1;
                            PlaySound(hatSnd);
                            break;
                        }
                    }
                }
                else {
                    // R / shoulder, from anywhere on the page - the same key and
                    // the same shoulder as the draft's reroll. Locked once a card
                    // is bought: that purchase settled this offer, and rerolling
                    // after it would let the player keep shopping a hand they
                    // have already spent their card slot on.
                    if (IsKeyPressed(KEY_R) || PadReroll()) {
                        if (pendCard < 0 && wallet >= RerollPrice()) {
                            wallet -= RerollPrice();
                            shopRerolls++;             // and the next costs double
                            DealShop();
                            PlaySound(twistSnd);
                            SaveAll();
                        }
                        else PlaySound(denySnd);
                    }

                    // The three cards are a row and everything under them is a
                    // column, so the cursor moves the way the screen looks:
                    // sideways between cards, down out of them.
                    int onCards = (shopSel <= SC_CARD2);
                    if (up) {
                        if (onCards) shopSel = SC_TOGACHA;
                        else if (shopSel == SC_HP) shopSel = shopCard;
                        else shopSel--;
                    }
                    if (down) {
                        if (onCards) { shopCard = shopSel; shopSel = SC_HP; }
                        else if (shopSel == SC_TOGACHA) shopSel = shopCard;
                        else shopSel++;
                    }
                    if (onCards && (left || right)) {
                        shopSel = (shopSel + (right ? 1 : SHOP_CARDS - 1)) % SHOP_CARDS;
                        shopCard = shopSel;
                    }

                    if (fire) {
                        if (shopSel <= SC_CARD2) {
                            int i = shopSel;
                            // Cards lock after one purchase: the next run gets
                            // one head start, not three.
                            if (shopOffer[i] >= 0 && pendCard < 0
                                && wallet >= cards[shopOffer[i]].price) {
                                wallet -= cards[shopOffer[i]].price;
                                pendCard = shopOffer[i];
                                PlaySound(levelSnd);
                                SaveAll();
                            }
                            else PlaySound(denySnd);
                        }
                        else if (shopSel == SC_HP) {
                            if (!pendHp && wallet >= PRICE_HP) {
                                wallet -= PRICE_HP;
                                pendHp = HP_TIER_SHOP;
                                PlaySound(levelSnd);
                                SaveAll();
                            }
                            else PlaySound(denySnd);
                        }
                        else if (shopSel == SC_BURST) {
                            if (!pendBurst && wallet >= PRICE_BURST) {
                                wallet -= PRICE_BURST;
                                pendBurst = BURST_TIER_SHOP;
                                PlaySound(levelSnd);
                                SaveAll();
                            }
                            else PlaySound(denySnd);
                        }
                        else {
                            shopPage = 1; shopSel = SG_PULL;
                            gachaSlot = -1; gachaOdds = 0;
                            PlaySound(hatSnd);
                        }
                    }
                }

                if (shopSel != was) PlaySound(hatSnd);
            }
        }
        else if (state == CREDITS) {
            if (IsKeyPressed(KEY_ESCAPE) || PadCancel()) {
                state = TITLE;
                baseTime = GetTime();
            }
        }
        else if (state == CODEX) {
            if (IsKeyPressed(KEY_ESCAPE) || PadCancel()) {
                state = TITLE;
                baseTime = GetTime();
            }
            if ((IsKeyPressed(KEY_LEFT)  || PadLeft())  && codexPage > 0) codexPage--;
            if (IsKeyPressed(KEY_RIGHT) || PadRight()) codexPage++;
        }
        else if (state == SETTINGS) {
            if (IsKeyPressed(KEY_ESCAPE) || PadCancel()) {
                state = TITLE;
                baseTime = GetTime();
            }

            // A cursor, finally. This screen gave every option its own letter,
            // which works with thirty-one keys under both hands and not at all
            // with fourteen buttons under two thumbs.
            //
            // The letters are gone with them: once every row is worked the same
            // way, one key per option is overhead, not a shortcut.
            int rows = OptionRows();
            if (optSel >= rows) optSel = rows - 1;   // a pad can leave mid-frame
            if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W) || PadUp())
                optSel = (optSel + rows - 1) % rows;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || PadDown())
                optSel = (optSel + 1) % rows;

            // LEFT/RIGHT act on the focused row. That USED to mean volume and
            // nothing else, and it still does until the player moves the
            // cursor - which they could only do with UP/DOWN, which did nothing
            // here before. So no habit is broken by this.
            int step = (IsKeyPressed(KEY_RIGHT) || PadRight()) ? 1
                     : (IsKeyPressed(KEY_LEFT)  || PadLeft())  ? -1 : 0;

            // Confirm flips a boolean row, on either device. It is redundant
            // with LEFT/RIGHT and that is fine: reaching a two-state option and
            // pressing the obvious button should not do nothing.
            int fire = PadConfirm() || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER);

            // The letter shortcuts are gone. They were the reason this screen
            // needed a cursor in the first place - one key per option does not
            // survive contact with a pad - and once every row is worked the
            // same way, remembering that BORDERLESS is J is pure overhead.
            // The cursor indexes the built list, not the enum, so a row that
            // is not on screen cannot be acted on by accident.
            int id = optRow[optSel];
            int doVol    = (id == OPT_VOLUME     && step);
            int doBl     = (id == OPT_BORDERLESS && (step || fire));
            int doMotion = (id == OPT_MOTION     && (step || fire));
            int doLang   = (id == OPT_LANG       && (step || fire));
            int monDir   = (id == OPT_MONITOR)   ? step : 0;
            // Three layouts, so confirm advances rather than toggles - the
            // same button doing "next" on a list and "flip" on a switch is
            // what everyone already expects from a settings screen.
            int faceDir  = (id == OPT_PADFACE) ? (step ? step : (fire ? 1 : 0)) : 0;
            int doSwap   = (id == OPT_PADSWAP  && (step || fire));

            // One press, one step. There was a hold-to-repeat here and it made
            // a single tap move the slider by two or six steps depending on how
            // long the key looked held - ten presses across the whole range is
            // not enough travel to be worth that unpredictability.
            if (doVol) {
                volStep += step;
                if (volStep < 0) volStep = 0;
                if (volStep > VOL_STEPS) volStep = VOL_STEPS;
                SetMasterVolume((float)volStep / VOL_STEPS);
                PlaySound(hitSnd[0]);        // audible reference for the new level
                SaveAll();
            }
            if (doMotion) { reducedMotion = !reducedMotion; SaveAll(); }
            if (doLang) {
                lang = (lang == LANG_KO) ? LANG_EN : LANG_KO;
                SaveAll();
            }
            if (doBl) {
                // Borderless windowed, not exclusive full screen: alt-tabbing
                // away from an exclusive mode forces a display mode change and
                // a black flash, and this game has nothing to gain from it.
                ApplyBorderless(!IsWindowState(FLAG_WINDOW_UNDECORATED));
                SaveAll();
            }
            {
                int mc = GetMonitorCount();
                if (monDir && mc > 1) {
                    ApplyMonitor((monitorSel + monDir + mc) % mc);
                    PlaySound(hatSnd);
                    SaveAll();
                }
            }
            if (faceDir) {
                padFace = (padFace + faceDir + PADFACE_COUNT) % PADFACE_COUNT;
                PlaySound(hatSnd);
                SaveAll();
            }
            if (doSwap) {
                padSwap = !padSwap;
                PlaySound(hatSnd);
                SaveAll();
            }
        }
        // `!showCards` matters: with the card panel open, ESC has to close the
        // panel, not fall through and unpause the game underneath it.
        else if (state == PLAYING && !showCards &&
            (IsKeyPressed(KEY_ESCAPE) || PadStart())) {
            if (!paused) {
                baseBeat = beat;          // freeze the playhead where it stands
                paused = 1;
            }
            else {
                baseTime = GetTime();     // resume from exactly there
                paused = 0;
            }
        }

        if (state == DRAFT) {
            // DOWN is the fourth point on the ring and the draft never used it.
            // Refusing a card matters: every card carries a cost, and being
            // forced to take one that fights the build is worse than taking
            // nothing. The ring has four directions, so the gesture is already
            // in the player's hands.
            // The draft was already positional - three cards on left, up and right,
            // skip below - so the D-pad maps onto it with nothing invented.
            // Both clusters, laid out the same way, for the same reason the lanes
            // take both: the hand that is already on the face buttons should not
            // have to travel to the D-pad to answer a card.
            int skip = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)
                    || PadDown() || PadConfirm();
            int pick = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || PadLeft()  || PadCards() ? 0
                : IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W) || PadUp()    || PadAux()   ? 1
                : IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || PadRight() || PadCancel() ? 2 : -1;
            if (pick >= offerCount) pick = -1;

            int canReroll = rerollsLeft > 0 || burst >= REROLL_BURST;
            if (!skip && pick < 0 && (IsKeyPressed(KEY_R) || PadReroll())) {
                if (!canReroll) PlaySound(denySnd);   // out of both budgets
                else {
                    if (rerollsLeft > 0) rerollsLeft--;
                    else burst -= REROLL_BURST;
                    DealOffer();
                    PlaySound(twistSnd);       // "something is different"
                    // Every card may be gone; do not strand the player on an
                    // empty draft with nothing to press.
                    if (offerCount == 0) {
                        state = PLAYING; baseTime = GetTime(); deckBanner = 2.6f;
                    }
                }
            }
            else if (skip || pick >= 0) {
                if (pick >= 0) { TakeCard(offer[pick]); PlaySound(levelSnd); }
                else PlaySound(hatSnd);
                state = PLAYING;
                baseTime = GetTime();      // resume from where the playhead froze
            }
        }

        // Spend the burst. Only while actually playing - the bot never uses it.
        if (state == PLAYING && !paused && !mods.burstLocked
            && burst >= 1.0f && burstLeft <= 0.0f
            && BurstHit()) {
            burst = 0.0f;
            burstLeft = mods.burstBeats;
            if (mods.barrierOnBurst) BarrierGain(1, 1);   // SURGE
            PlaySound(levelSnd);
            shake = 0.5f;
        }

        // The card list is a panel, not its own state: the frozen scene behind
        // it has to keep rendering either way. It opens from the pause screen
        // and from the result screen, which are the two places the run is
        // already stopped.
        if (showCards) {
            if (IsKeyPressed(KEY_ESCAPE) || PadCancel()) showCards = 0;
            if (IsKeyPressed(KEY_LEFT)  || PadLeft())  cardPage--;
            if (IsKeyPressed(KEY_RIGHT) || PadRight()) cardPage++;
        }
        else if (paused || state == DEAD || state == WON) {
            int rows[4];
            int n = RunMenu(rows);
            if (runSel >= n) runSel = n - 1;
            if (runSel < 0) runSel = 0;

            int was = runSel;
            if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W) || PadUp())
                runSel = (runSel + n - 1) % n;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || PadDown())
                runSel = (runSel + 1) % n;
            if (runSel != was) PlaySound(hatSnd);

            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || PadConfirm()) {
                switch (rows[runSel]) {
                case M_CARDS:
                    showCards = 1;
                    cardPage = 0;
                    break;
                case M_RESUME:
                    paused = 0;
                    baseTime = GetTime();      // resume where the playhead froze
                    break;
                default:                        // GIVE UP and BACK TO TITLE
                    paused = 0;
                    state = TITLE;
                    StartRun(ChosenSeed());
                    break;
                }
            }
        }
        prevBeat = beat;

        // Runs off the wall clock, so it plays on exactly the screens where
        // the beat-driven backing has stopped.
        MenuMusic(MenuMusicFor(state, paused));

        // The reel. Resolving on the timer rather than on the keypress is the
        // point: the outcome is decided at the moment the player sees it stop.
        if (gachaTimer > 0.0f) {
            gachaTimer -= dt;
            if (gachaTimer <= 0.0f) { gachaTimer = 0.0f; GachaResolve(); }
        }

        // The camera rides the tunnel and always looks straight at the gate,
        // so the judgement point stays dead centre however much the tunnel bends.
        cam.position = WorldPos(camZ, (Vector2) { 0.0f, 0.0f });
        cam.target = WorldPos(gateZ, (Vector2) { 0.0f, 0.0f });
        cam.fovy = 70.0f + pulse * (reducedMotion ? 0.0f : FOV_PULSE);
        if (running && shake > 0.0f && !reducedMotion) {
            cam.position.x += VfxSigned() * shake * 0.5f;
            cam.position.y += VfxSigned() * shake * 0.5f;
        }

        int listening = InCallPhase(beat);

        BeginTextureMode(stage);
        ClearBackground(BLACK);
        BeginMode3D(cam);

        // One ring per beat, positioned relative to the gate so that a ring
        // lands exactly on the gate when beat reaches that ring's beat.
        int beatFloor = (int)beat;
        for (int i = -2; i < 40; i++) {
            int ringBeat = beatFloor + i;
            float z = gateZ + ((float)ringBeat - beat) * SPACING;
            if (z < camZ + 1.0f) continue;
            // The beat rings are the ruler, not the reading. They step back so
            // the note rings can be told apart from them - both are full-radius
            // circles from the same call, and at equal weight the eye cannot
            // tell a beat from a note.
            float bright = (ringBeat % 4 == 0) ? (0.22f + 0.26f * pulse) : 0.12f;
            unsigned char v = (unsigned char)(bright * 255.0f);
            Color c = { (unsigned char)(tintR * bright),
                        (unsigned char)(tintG * bright),
                        (unsigned char)(tintB * bright), 255 };
            DrawCircle3D(WorldPos(z, (Vector2) { 0.0f, 0.0f }), TUNNEL_R,
                (Vector3) {
                1, 0, 0
            }, 0.0f, c);
        }

        // Judgement gate. Radius is fixed at exactly TUNNEL_R so an arriving
        // ring overlaps it perfectly; the beat is shown by brightness instead.
        // Gold means the player's turn; cool blue means the game is playing and
        // presses are free.
        unsigned char gv = (unsigned char)(140.0f + 115.0f * pulse);
        Color gateCol = listening ? (Color) { 70, gv, 255, 255 }
        : (Color) { 255, gv, 30, 255 };
        for (int i = 0; i < 3; i++) {
            DrawCircle3D(WorldPos(gateZ, (Vector2) { 0.0f, 0.0f }), TUNNEL_R - i * 0.05f,
                (Vector3) {
                1, 0, 0
            }, 0.0f, gateCol);
        }

        // Lane markers sit on the gate ring, where the monster will land.
        // They light up when the player presses and when the game calls.
        //
        // The flash used to be a colour shift on a one-pixel wireframe, and
        // both testers said they could not follow the call. A thin outline
        // changing hue is close to invisible against a moving tunnel, so a lit
        // marker now grows a solid core and a second outline around it. At rest
        // (g == 0) nothing extra is drawn and the screen looks as it did.
        for (int lane = 0; lane < LANE_COUNT; lane++) {
            Vector3 m = WorldPos(gateZ, LanePos(lane));
            Color lc = LaneColor(lane);
            float g = laneFlash[lane] > callFlash[lane] ? laneFlash[lane] : callFlash[lane];
            Color mk = { (unsigned char)(lc.r * (0.35f + 0.65f * g)),
                         (unsigned char)(lc.g * (0.35f + 0.65f * g)),
                         (unsigned char)(lc.b * (0.35f + 0.65f * g)), 255 };
            float s = 1.5f + g * 0.5f;

            if (g > 0.02f) {
                // Filled centre: area, not line weight, is what carries at a
                // glance. It fades out with the flash so it never sits there.
                Color fill = { lc.r, lc.g, lc.b, (unsigned char)(g * 200.0f) };
                float fs = s * (0.45f + 0.35f * g);
                DrawCube(m, fs, fs, 0.22f, fill);
                DrawCubeWires(m, s * 1.22f, s * 1.22f, 0.3f, mk);
            }
            DrawCubeWires(m, s, s, 0.3f, mk);
        }

        // Player ship. It is angled at the gate rather than straight down +z:
        // pointed away from the camera it collapses into a flat silhouette,
        // and the wings and fin are what actually sell it as a solid object.
        Vector3 ship = WorldPos(playerZ, (Vector2) { PLAYER_X, PLAYER_Y });
        // Aiming past the gate rather than at it keeps the tilt readable while
        // stopping the nose from swinging back inside the ring on screen: the
        // further forward the tip goes, the more its z grows against its radius.
        Vector3 aim = WorldPos(gateZ + PLAYER_AIM, (Vector2) { 0.0f, 0.0f });
        Vector3 fwd = V3Norm(V3Sub(aim, ship));
        Vector3 rgt = V3Norm(V3Cross(fwd, (Vector3) { 0.0f, 1.0f, 0.0f }));
        Vector3 up = V3Cross(rgt, fwd);

        float bank = PLAYER_BANK * DEG2RAD;
        Vector3 rb = V3Add(V3Scale(rgt, cosf(bank)), V3Scale(up, sinf(bank)));
        Vector3 ub = V3Add(V3Scale(rgt, -sinf(bank)), V3Scale(up, cosf(bank)));

        Vector3 muzzle = V3Add(ship, V3Scale(fwd, PLAYER_NOSE));   // beams start here
        Vector3 tail = V3Sub(ship, V3Scale(fwd, 0.3f));

        Color hull = skins[skinSel].hull;
        Color trim = skins[skinSel].trim;
        DrawCylinderEx(tail, muzzle, 0.22f, 0.02f, 4, hull);
        DrawCylinderWiresEx(tail, muzzle, 0.22f, 0.02f, 4, trim);

        Vector3 wingBase = V3Sub(ship, V3Scale(fwd, 0.1f));
        DrawCylinderEx(wingBase, V3Add(wingBase, V3Scale(rb, 0.5f)), 0.12f, 0.02f, 3, hull);
        DrawCylinderEx(wingBase, V3Sub(wingBase, V3Scale(rb, 0.5f)), 0.12f, 0.02f, 3, hull);
        DrawCylinderEx(tail, V3Add(tail, V3Scale(ub, 0.4f)), 0.10f, 0.02f, 3, trim);

        // The barrier: a bubble around the craft, one shell per charge so the
        // count is read off the picture rather than a number somewhere else.
        // It breathes on the beat, because a thing that does nothing until it
        // is needed has to prove it is still there.
        for (int b = 0; b < barrierLeft && b < 4; b++) {
            float rad = 0.62f + (float)b * 0.13f + pulse * 0.05f;
            unsigned char a = (unsigned char)(120 + 70 * pulse);
            Color sc = { 140, 220, 255, a };
            DrawCircle3D(ship, rad, rgt, 90.0f, sc);
            DrawCircle3D(ship, rad, ub, 90.0f, sc);
        }

        // And the moment it is spent. Expanding and fading: the shell did not
        // vanish, it took the hit and came apart.
        if (barrierBreak > 0.0f) {
            float t = 1.0f - barrierBreak / 0.7f;
            float rad = 0.62f + t * 1.5f;
            Color sc = { 190, 240, 255, (unsigned char)(220.0f * (1.0f - t)) };
            DrawCircle3D(ship, rad, rgt, 90.0f, sc);
            DrawCircle3D(ship, rad, ub, 90.0f, sc);
            DrawCircle3D(ship, rad, fwd, 90.0f, sc);
        }

        // Hit beams: the visible reason the monster died.
        for (int i = 0; i < MAX_BEAMS; i++) {
            if (beams[i].life <= 0.0f) continue;
            Color c = beams[i].col;
            c.a = (unsigned char)(beams[i].life * 255.0f);
            DrawCylinderEx(muzzle, beams[i].to, 0.02f, 0.18f * beams[i].life, 6, c);
        }


        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            float z = gateZ + (enemies[i].hitBeat - beat - enemies[i].rush) * SPACING;
            if (z < camZ - 2.0f) continue;                       // escaped ones fly past
            // FOG pulls the horizon in until the ear has to do the reading.
            float far = mods.fogBeats > 0.0f
                ? gateZ + mods.fogBeats * SPACING
                : camZ + 45.0f;
            if (z > far) continue;

            // A hidden note is simply not drawn until it is nearly at the gate.
            // The call still played it, so the ear has the answer all along.
            if (enemies[i].hidden && (enemies[i].hitBeat - beat) > HIDDEN_REVEAL) {
                continue;
            }

            Vector2 local = EnemyLocal(&enemies[i], beat);
            if (enemies[i].outward > 0.0f) {
                float k = 1.0f + enemies[i].outward * 2.2f;
                local.x *= k;
                local.y *= k;
            }
            Vector3 p = WorldPos(z, local);
            // DISGUISE strips the trap's tell-tale violet and dresses it in the
            // lane colour, leaving only the shape to give it away.
            Color c = (enemies[i].trap && !mods.trapDisguise)
                ? (Color) {
                190, 90, 230, 255
            }
            : EnemyColor(&enemies[i], beat);

            float s = SubScale(enemies[i].tier);

            // An escaped monster goes hostile: it whitens out and swells as it
            // closes on the camera. Traps never do this - they leave sideways.
            if (enemies[i].escaped && !enemies[i].trap) {
                float near = (gateZ - z) / HIT_OFFSET;      // 0 at the gate, 1 at the camera
                if (near < 0.0f) near = 0.0f;
                if (near > 1.0f) near = 1.0f;
                c = (Color){ 255,
                             (unsigned char)(90 + 60 * (1.0f - near)),
                             (unsigned char)(90 * (1.0f - near)), 255 };
                s *= 1.0f + near * 0.9f;
            }
            if (enemies[i].outward > 0.0f) {
                c.a = (unsigned char)(255.0f * (1.0f - enemies[i].outward));
            }

            // A trap has no ring - the ring is the promise that something must
            // be hit, and a trap is the one thing that must not be.
            if (!enemies[i].trap) {
                // The monster rides its own ring; when that ring merges with
                // the gate ring, the hit is exactly on the beat.
                //
                // The ring is also how the player reads ORDER, and that turned
                // out to be the whole visibility problem. Two notes in
                // different lanes at different depths share nothing on screen,
                // so "which do I hit first" was a comparison of two sizes
                // between two separated points - the judgement eyes are worst
                // at, and it gets worse as the BPM closes the gap. But every
                // note's ring is concentric with every other note's, and
                // "inside" versus "outside" needs no reference and no matching
                // of screen positions. The cue was always there; it was drawn
                // with the same call, the same radius and the same weight as
                // the beat rings and vanished into them. Now it is three
                // circles deep, and the beat rings have stepped back.
                //
                // A chord shares one beat, so it gets ONE ring, in a colour
                // neither note has on its own. Taking one member's colour made
                // yellow+blue read as a plain yellow, and in a bar like
                // [YB] Y [YB] B that mistake is the entire read. Mixing them
                // means a chord never wears a single note's colour.
                // An escaped note has left the chord - it is on its own way to
                // the camera and keeps its own ring, so it is excluded here and
                // its partner goes back to being a single note.
                int gn = 0, gr = 0, gg = 0, gb = 0, low = i;
                if (!enemies[i].escaped) {
                    for (int j = 0; j < MAX_ENEMIES; j++) {
                        if (!enemies[j].alive || enemies[j].escaped || enemies[j].trap) continue;
                        if (enemies[j].hitBeat != enemies[i].hitBeat) continue;
                        Color jc = EnemyColor(&enemies[j], beat);
                        gr += jc.r; gg += jc.g; gb += jc.b; gn++;
                        if (j < low) low = j;
                    }
                }
                Color rc = c;
                if (gn > 0) {
                    rc.r = (unsigned char)(gr / gn);
                    rc.g = (unsigned char)(gg / gn);
                    rc.b = (unsigned char)(gb / gn);
                }

                // One ring per beat, drawn by the lowest member, so a chord's
                // two notes cannot lay down two rings that z-fight and let
                // whichever wins decide the colour.
                if (low == i) {
                    for (int r = 0; r < RING_LAYERS; r++) {
                        DrawCircle3D(WorldPos(z, (Vector2) { 0.0f, 0.0f }),
                            TUNNEL_R - r * 0.05f, (Vector3) {
                            1, 0, 0
                        }, 0.0f, rc);
                    }
                }

                // A chord is two notes on one beat, and head-on the two cubes
                // read as two separate notes that happen to be near each other.
                // The ear cannot separate them either - both pitches arrive at
                // once. So the pair is drawn joined: one bar, one press.
                //
                // The bar wears the mixed colour too. It is the same statement
                // as the ring - this is one press - and saying it in one note's
                // colour would contradict the ring it hangs under.
                //
                // Only the lower lane index draws it, or the bar would be laid
                // down twice and show through itself.
                for (int j = i + 1; j < MAX_ENEMIES; j++) {
                    if (!enemies[j].alive || enemies[j].escaped || enemies[j].trap) continue;
                    if (enemies[j].hitBeat != enemies[i].hitBeat) continue;
                    Vector2 lb = EnemyLocal(&enemies[j], beat);
                    if (enemies[j].outward > 0.0f) {
                        float k2 = 1.0f + enemies[j].outward * 2.2f;
                        lb.x *= k2; lb.y *= k2;
                    }
                    Vector3 pb = WorldPos(z, lb);
                    DrawCylinderEx(p, pb, 0.07f, 0.07f, 4, rc);
                }
            }

            if (enemies[i].trap && !mods.trapDisguise) {
                // Different silhouette, not just a different colour: a spun
                // diamond where the real monsters carry a boxed core, so it is
                // still readable at a glance in a dense bar.
                Vector3 dTop = p; dTop.y += s * 0.62f;
                Vector3 dBot = p; dBot.y -= s * 0.62f;
                DrawCylinderEx(dBot, dTop, 0.0f, s * 0.52f, 4, c);
                DrawCylinderEx(dTop, dBot, 0.0f, s * 0.52f, 4, c);
                DrawCylinderWiresEx(dBot, dTop, 0.0f, s * 0.52f, 4, WHITE);
            }
            else {
                // A disguised trap borrows the monster's body as well as its
                // colour, so the shape stops giving it away at a glance.
                //
                // What it never borrows is the white wire box. That box is this
                // game's word for "this one must be hit", and the ring is the
                // other; both stay honest, because a lie the player cannot see
                // through is not a trap, it is a dice roll. What DISGUISE takes
                // is the free read - the one that used to come from colour and
                // shape without looking. The tells that remain cost attention
                // to check, and attention is what the card is really charging.
                Vector3 spikeBack = p; spikeBack.z -= s * 1.07f;
                Vector3 spikeFront = p; spikeFront.z -= s * 0.29f;
                DrawCylinderEx(spikeFront, spikeBack, s * 0.39f, 0.02f, 4, c);

                // An item note swaps two things and nothing else: the core and
                // the colour of the cage.
                //
                // Head-on, this game already spends two silhouettes - a square
                // for a note and a diamond for a trap - so the item takes a
                // hexagon, a coin facing the player. A circle would work and
                // was the obvious pick; the hexagon wins narrowly because the
                // screen is already full of circles (the gate, the beat rings,
                // every note's own ring) and a small filled one competes with
                // them at distance. Far away the hexagon degrades to a round
                // blob anyway, so the read never becomes wrong, only less
                // specific. To go back to a sphere, this is one line:
                // DrawSphereEx(p, s * 0.40f, 6, 6, c).
                //
                // The cage takes the lane colour instead of white. White is
                // this game's word for "an ordinary note that must be hit", so
                // the item is marked by its ABSENCE - the note goes monochrome
                // in its own lane colour. Nothing new had to be taught, and the
                // lane read is untouched because the colour is still the lane's.
                //
                // Which item it is, is deliberately not shown. The player needs
                // to know it is worth taking, and they were going to take it
                // either way.
                if (enemies[i].item) {
                    Vector3 hf = p; hf.z -= s * 0.20f;
                    Vector3 hb = p; hb.z += s * 0.20f;
                    DrawCylinderEx(hf, hb, s * 0.40f, s * 0.40f, 6, c);
                    DrawCylinderWiresEx(hf, hb, s * 0.40f, s * 0.40f, 6, c);
                }
                else {
                    DrawCube(p, s * 0.71f, s * 0.71f, s * 0.71f, c);
                }
                if (!enemies[i].trap) {
                    DrawCubeWires(p, s * 1.11f, s * 1.11f, s * 1.11f,
                        enemies[i].item ? c : WHITE);
                }
            }
        }

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].life <= 0.0f) continue;
            float s = 0.38f * particles[i].life;
            Color c = particles[i].col;
            c.a = (unsigned char)(particles[i].life * 255.0f);
            DrawCube(particles[i].pos, s, s, s, c);
        }

        EndMode3D();
        DrawVignette();       // over the tunnel, under the HUD, so text stays crisp

        if (missFlash > 0.0f) {
            DrawRectangle(0, 0, 960, 540,
                (Color) {
                255, 40, 40, (unsigned char)(missFlash * 70.0f)
            });

            // Glitch bands: torn slices of colour across the screen. Drawn only
            // while running, otherwise the randomness would keep flickering on
            // a paused or frozen frame.
            if (running) {
                int bands = 3 + (int)(missFlash * 5.0f);
                for (int i = 0; i < bands; i++) {
                    int by = (int)(VfxRand() % 540u);
                    int bh = 3 + (int)(VfxRand() % 18u);
                    int bx = (int)(VfxSigned() * 26.0f);
                    unsigned char ba = (unsigned char)(missFlash * 110.0f);
                    Color bc = (VfxRand() & 1u) ? (Color) { 255, 90, 90, ba }
                    : (Color) { 210, 230, 255, ba };
                    DrawRectangle(bx, by, 960, bh, bc);
                }
            }
        }

        if (state == PLAYING) {
            UiText(TextFormat("%d", score), 20, 20, 40, RAYWHITE);
            if (combo > 1) UiText(TextFormat("%d combo", combo), 20, 66, 24, YELLOW);
            UiText(mode == MODE_HARD ? TextFormat("LV %d", level + 1)
                                     : TextFormat("LV %d / %d", level + 1, LevelCount()),
                20, 500, 22,
                (Color) {
                150, 150, 170, 255
            });
            // Health gauge.
            DrawHeart(791.0f, 40.0f, 15.0f, (Color) { 255, 80, 100, 255 });
            DrawRectangle(810, 33, 130, 15, (Color) { 38, 38, 48, 255 });
            Color gc = (hp > 0.5f) ? (Color) { 90, 220, 130, 255 }
            : (hp > 0.25f) ? (Color) { 250, 200, 70, 255 }
                : (Color) { 255, 80, 90, 255 };
            // The bar shows the gauge against its own ceiling, which FRAGILE
            // can halve; the empty remainder is drawn dimmer so the loss shows.
            DrawRectangle(810, 33, (int)(130.0f * HpMax()), 15,
                (Color) {
                52, 52, 64, 255
            });
            DrawRectangle(810, 33, (int)(130.0f * hp), 15, gc);
            DrawRectangleLines(810, 33, 130, 15, (Color) { 95, 95, 115, 255 });

            // Burst meter, right under the gauge. It pulses once full so the
            // player notices they are holding something spendable.
            DrawRectangle(810, 54, 130, 9, (Color) { 38, 38, 48, 255 });
            Color bc = (burstLeft > 0.0f) ? (Color) { 255, 255, 255, 255 }
            : (burst >= 1.0f) ? (Color) {
                255, (unsigned char)(170 + 85 * pulse), 60, 255
            }
                : (Color) { 150, 120, 240, 255 };
            float bw = (burstLeft > 0.0f) ? (burstLeft / mods.burstBeats) : burst;
            DrawRectangle(810, 54, (int)(130.0f * bw), 9, bc);
            DrawRectangleLines(810, 54, 130, 9, (Color) { 95, 95, 115, 255 });
            if (burst >= 1.0f && burstLeft <= 0.0f && !mods.burstLocked
                && ((int)(GetTime() * 3.0)) % 2 == 0) {
                // Right aligned to the gauge, not parked at a fixed x. "SPACE"
                // is five characters and "RB" is two, so a shared left edge
                // leaves the pad label floating half a word away from the bar
                // it belongs to.
                const char* bk = PROMPT("SPACE", "#SB");
                UiText(bk, 800 - UiMeasure(bk, 16), 53, 16, (Color) { 255, 200, 70, 255 });
            }
            // Both of these used to sit at y=96, which is exactly where the up
            // lane's monsters come through, and the "ready" one stayed up for
            // as long as the meter was full - so a player saving their burst
            // had a permanent caption over the top of the pattern.
            //
            // Now: in the empty band along the very top - above the ring, between
            // the score at the left and the gauge at the right. Below the ring
            // was tried first and runs into the down lane's marker.
            //
            // And the ready line is an EVENT: it fires on the frame the meter
            // fills and fades over 1.6s.
            // The news is that the meter filled; after that the blinking SPACE
            // beside the gauge is the standing reminder, and it is out of the
            // way. Fading rather than cutting, so it does not read as a glitch.
            if (burstLeft > 0.0f) {
                DrawCentered("BURST", 18, 30, (Color) { 255, 245, 190, 255 });
            }
            else if (burstReadyFlash > 0.0f) {
                float a = burstReadyFlash / 1.6f;
                if (a > 1.0f) a = 1.0f;
                unsigned char al = (unsigned char)(235.0f * a);
                // The first fill of a run explains the button; every fill after
                // that only has to say it is there.
                if (takenTotal == 0) {
                    DrawCentered(PROMPT("SPACE - burst destroys every monster for 4 beats",
                                        "#SB - burst destroys every monster for 4 beats"), 24, 18,
                        (Color) { 235, 205, 130, al });
                }
                else {
                    DrawCentered("BURST READY", 20, 26, (Color) { 255, 200, 70, al });
                }
            }

            // The hand used to be listed down the left edge. In a long run that
            // is a wall of text over the playfield, so it moved to the pause
            // screen - the run only needs a count while it is being played.
            if (takenTotal > 0) {
                UiText(TextFormat(T("%d cards"), takenTotal), 20, 470, 17,
                    (Color) {
                    120, 112, 170, 255
                });
            }

            // Only the safe half is labelled. Saying nothing during the
            // player's turn keeps the screen quiet when it matters most.
            if (listening) {
                DrawCentered("LISTEN  -  hear it, then play it back", 480, 18,
                    (Color) {
                    90, 130, 190, 255
                });
            }

            if (judgeTimer > 0.0f && judgeText) {
                Color jc = judgeCol;
                float a = judgeTimer / 0.45f;
                jc.a = (unsigned char)(a * 255.0f);
                DrawCentered(judgeText, 300, 30, jc);
            }
            if (twistWarn > 0.0f) {
                DrawCentered(warnText, 120, 34,
                    (Color) {
                    255, 120, 220, (unsigned char)(twistWarn * 255.0f)
                });
            }
            if (levelBanner > 0.0f) {
                float a = levelBanner > 1.0f ? 1.0f : levelBanner;
                DrawCentered("LEVEL UP", 200, 54,
                    (Color) {
                    255, 220, 70, (unsigned char)(a * 255.0f)
                });
                DrawCentered(TextFormat("%d", level + 1), 262, 72,
                    (Color) {
                    255, 255, 255, (unsigned char)(a * 255.0f)
                });
            }
            // The deck can run dry - every card held, or every remaining one
            // locked out by a group or a gimmick this run removed. The draft is
            // skipped in that case, and a skipped draft with no explanation
            // reads as a bug. Say it instead.
            if (deckBanner > 0.0f) {
                float a = deckBanner > 1.0f ? 1.0f : deckBanner;
                DrawCentered("ALL CARDS COLLECTED", 348, 30,
                    (Color) {
                    180, 230, 255, (unsigned char)(a * 255.0f)
                });
            }
        }
        UiText(TextFormat(T("seed %u"), seed), 760, 510, 20, DARKGRAY);

        // Overlays sit on top of the scene rather than replacing it.
        if (state == TITLE) {
            // Only what a first-time player needs to start. Everything else
            // moved to HELP - the screen used to overlap its own PRESS SPACE.
            DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 150 });
            DrawCentered("RHYTHM TUNNEL", 62, 48, RAYWHITE);

            // Top left, clear of the centred column the rest of the screen uses.
            DrawPendingPanel(24, 40);

            // Bottom left, opposite the attract run's seed in the other
            // corner. Both are the same kind of thing - a number you only
            // look at when you have a reason to - so they are drawn the same
            // way and kept out of the middle of the screen.
            UiText(GAME_VERSION, 20, 512, 18, DARKGRAY);
            DrawCentered("LISTEN to the bar, then PLAY IT BACK", 116, 18,
                (Color) {
                180, 180, 200, 255
            });

            // Records sit ABOVE the menu, not between its halves. They belong to
            // whichever mode is highlighted, and reading them means glancing up
            // from the two rows that change them - not down past them.
            if (HI_SCORE > 0 || HI_LEVEL > 0) {
                // Endless has no ending, so how far you got is a record in its own
                // right - and it does not always move with the score, since a
                // scoring deck and a surviving deck are not the same deck.
                DrawCentered(mode == MODE_HARD
                    ? TextFormat(T("LEVEL  %d      BEST  %d"), HI_LEVEL, HI_SCORE)
                    : TextFormat(T("BEST  %d"), HI_SCORE), 156, 20, DARKGRAY);
            }
            // Two groups, one list. The modes are set larger and given their own
            // block of air because they are what the screen is for; the rest are
            // places to go. Navigation does not change between them - it is all
            // one column - but the eye is told which half it is in.
            {
                Color pick = { 255, 210, 90, 255 };
                Color idle = { 165, 165, 185, 255 };
                Color sub = { 130, 130, 155, 255 };
                Color subIdle = { 110, 110, 132, 255 };

                const char* rowText[TITLE_ROWS] = {
                    "NORMAL", "ENDLESS", "SEED", "SHOP", "CARDS",
                    "OPTIONS", "HELP", "CREDITS", "QUIT"
                };
                for (int i = 0; i < TITLE_ROWS; i++) {
                    // Three groups of sizing, two groups of meaning. NORMAL,
                    // ENDLESS and SEED are one set - what the run will be -
                    // and they sit together with a gap under them. Everything
                    // below is somewhere to go instead. SEED is set smaller
                    // than the two modes so it reads as belonging to them
                    // rather than being a third one.
                    int top = (i <= T_SEED);
                    int fs = (i <= T_ENDLESS) ? 28 : top ? 19 : 21;
                    int y = (i <= T_ENDLESS) ? (198 + i * 36)
                          : top              ? 272
                                             : (312 + (i - T_SHOP) * 29);
                    int on = (titleSel == i);

                    // Normal carries its level count and SEED carries its
                    // number. Both belong beside their own row rather than on
                    // a line of their own - a value parked somewhere else has
                    // to be labelled all over again to say what it belongs to.
                    const char* label = rowText[i];
                    // NORMAL_LEVELS, not LevelCount() - the latter reads the CURRENT
                    // mode, so highlighting ENDLESS made the NORMAL row claim
                    // 9999 levels.
                    if (i == T_NORMAL) {
                        label = TextFormat(T("%s (%d levels)"),
                                           T(rowText[i]), NORMAL_LEVELS);
                    }
                    else if (i == T_SEED) {
                        label = TextFormat(T("ENTER SEED (%s)"),
                                           seedLen ? seedText : T("RANDOM"));
                    }

                    int w = UiMeasure(label, fs);
                    int x = (960 - w) / 2;
                    UiText(label, x, y, fs,
                           on ? pick : (top ? idle : subIdle));

                    // The marker sits beside the row rather than under it. A
                    // highlight bar behind centred text of varying width jitters
                    // as the cursor moves; a fixed caret does not.
                    if (on) UiText(">", x - 26, y, fs, pick);
                }
                (void)sub;
            }

            DrawCentered(PROMPT("UP / DOWN to choose      SPACE to confirm",
                          "UP / DOWN to choose      (#FD) to confirm"),
                508, 16, (Color) { 100, 100, 120, 255 });

            // The seed box, over the top of everything. An overlay rather than
            // a state of its own: the title is still the screen you are on, and
            // the number being edited is the one the row behind it shows.
            if (seedBox) {
                DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 200 });
                DrawRectangle(268, 148, 424, 288, (Color) { 16, 16, 22, 245 });
                DrawRectangleLines(268, 148, 424, 288, (Color) { 95, 95, 115, 255 });

                DrawCentered("SEED", 168, 22, (Color) { 200, 190, 255, 255 });

                // The number itself, in a slot that stays the same size whether
                // it holds ten digits or none - a box that grows as you type
                // moves the thing you are reading while you read it.
                DrawRectangle(300, 204, 360, 40, (Color) { 30, 30, 40, 255 });
                DrawRectangleLines(300, 204, 360, 40, (Color) { 70, 70, 90, 255 });
                if (seedLen > 0) {
                    DrawCentered(seedText, 212, 26, RAYWHITE);
                }
                else {
                    // Empty is not blank: it is a promise about what happens
                    // next, so it says so.
                    DrawCentered("RANDOM", 214, 22, (Color) { 120, 120, 145, 255 });
                }

                const char* cellText[SEED_CELLS] = {
                    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                    "DEL", "CLEAR ALL", "OK"
                };
                for (int i = 0; i < SEED_CELLS; i++) {
                    int cx = SeedCellX(i), cy = SeedCellY(i);
                    int on = (seedSel == i);
                    int hw = (i < SEED_DEL) ? 26 : 58;
                    if (on) {
                        DrawRectangle(cx - hw, cy - 4, hw * 2, 34,
                            (Color) { 255, 200, 70, 255 });
                    }
                    Color c = on ? (Color) { 20, 20, 26, 255 }
                    : (i < SEED_DEL) ? (Color) { 200, 200, 220, 255 }
                    : (Color) { 150, 150, 180, 255 };
                    int fs = (i < SEED_DEL) ? 24 : 18;
                    int w = UiMeasure(cellText[i], fs);
                    UiText(cellText[i], cx - w / 2, cy + (i < SEED_DEL ? 0 : 4), fs, c);
                }

                DrawCentered(PROMPT("SPACE to enter      ESC to cancel",
                                    "(#FD) to enter      (#FR) to cancel"),
                    448, 15, (Color) { 110, 110, 135, 255 });
            }
        }
        else if (state == HELP) {
            DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 205 });
            DrawCentered("HOW TO PLAY", 42, 36, (Color) { 200, 190, 255, 255 });

            // Page 1 is what you need to press a button at all; page 2 is what
            // the game does to you afterwards. The old single page mixed the
            // two, explained SWAP - which Normal never shows - and never
            // mentioned health, TRAPS, or the difference between the two hits.
            //
            // A tab splits a row into a key and its description. Padding with
            // spaces is what the English version used to do, and it cannot work
            // here: a Hangul glyph is twice the width of a space, so any column
            // aligned by counting spaces comes apart in translation.
            const char* help1[] = {
                "The game plays a bar, then the same rhythm comes at you as monsters.",
                "Press each one the moment its ring meets the yellow gate.",
                "",
                PROMPT("ARROW KEYS / WASD\tthe four points on the ring",
                       "D-PAD / #FC\tthe four points on the ring"),
                PROMPT("\tWASD lets you play with both hands",
                       "\ttwo clusters lets you play with both hands"),
                PROMPT("SPACE\tfire a BURST once its meter is full",
                       "#SB\tfire a BURST once its meter is full"),
                PROMPT("ESC\tpause", "#ST\tpause"),
                "",
                "PERFECT is the tight centre of the window, GOOD is the rest of it.",
                "PERFECT heals more, scores more, and is the only thing that",
                "charges the burst meter.",
                "BAD is a note you did hit, too early or too late to count.",
                "",
                "A BAD and a MISS both cost health.",
            };
            const char* help2[] = {
                "BURST clears every note that reaches the gate for four beats.",
                "It also wipes out anything that already slipped past you.",
                "",
                "While the gate is BLUE the game is playing its call.",
                "The call already names where the monsters will land.",
                "",
                "TRAPS\ta diamond with no ring. Never press it.",
                "SWAP\tthe monsters slide to the opposite point.",
                "BLIND\tthe monsters stay hidden until the last moment.",
                "",
                "A card is drafted at level 2, then every third level after.",
                "Cards change the run itself - what heals you, how you score,",
                "how fast the burst charges, even what the bar throws at you.",
            };
            // Page 3 is everything around the draft rather than inside it -
            // refusing a card, rerolling, and the shop those coins are for.
            // None of it was written down anywhere before, so a player could
            // finish a run without learning that any of it existed.
            const char* help3[] = {
                "Every card has a cost, so taking none is a real choice.",
                "",
                PROMPT("DOWN / S\ttake no card", "DOWN / #FD\ttake no card"),
                PROMPT("R\tdeal three new cards", "#SR\tdeal three new cards"),
                "\tthree free a run, then half a burst meter each",
                "",
                "Coins earned by clearing levels are spent in the SHOP.",
                "",
                "The shop sells a card to carry into the next run, more maximum",
                "health, and a burst meter that starts part filled.",
                "One of each type at a time, and only for the next run.",
                "",
                "The GACHA rolls once for a random prize - a card, an item,",
                "coins, or a new skin.",
            };

            const char** page = (helpPage == 2) ? help3 : helpPage ? help2 : help1;
            int nLines = (helpPage == 2) ? (int)(sizeof(help3) / sizeof(help3[0]))
                       : helpPage        ? (int)(sizeof(help2) / sizeof(help2[0]))
                                         : (int)(sizeof(help1) / sizeof(help1[0]));
            Color body = { 170, 170, 195, 255 };
            Color key = { 235, 210, 130, 255 };
            for (int i = 0; i < nLines; i++) {
                const char* s = T(page[i]);
                int y = 118 + i * 24;
                const char* tab = strchr(s, '\t');
                if (!tab) { UiText(s, 70, y, 17, body); continue; }

                // The halves are cut out of an already-translated line, so the
                // second lookup inside UiText finds nothing and passes them
                // through untouched, which is what we want.
                if (tab != s) UiText(TextSubtext(s, 0, (int)(tab - s)), 70, y, 17, key);
                UiText(tab + 1, 300, y, 17, body);
            }
            DrawCentered(TextFormat(T("page %d / %d      LEFT / RIGHT"),
                helpPage + 1, 3), 476, 15, (Color) { 110, 110, 135, 255 });
            DrawCentered(PROMPT("ESC to go back", "(#FR) to go back"), 502, 17, (Color) { 110, 110, 135, 255 });
        }
        else if (state == SETTINGS) {
            DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 205 });
            DrawCentered("OPTIONS", 46, 36, (Color) { 200, 190, 255, 255 });

            // Labels and values sit in two fixed columns. They used to be 12px
            // apart, which read as one word: "FULL SCREENOFF".
            Color hint = { 100, 100, 120, 255 };
            Color on = { 150, 230, 170, 255 };
            Color off = { 140, 140, 160, 255 };
            // Rows are drawn from the same list the cursor indexes, so the
            // screen cannot disagree with the input about what row 5 is.
            int monitors = GetMonitorCount();
            int rows = OptionRows();
            if (optSel >= rows) optSel = rows - 1;

            // The list stretches to fit instead of holding one spacing and
            // running off the bottom at seven rows. The cap stops four rows
            // from drifting apart into a list of unrelated things.
            const int top = (rows >= 6) ? 108 : 125;
            int row = (432 - top) / (rows - 1);
            if (row > 90) row = 90;

            // The cursor: a bar down the left edge, not a filled row. These
            // rows already carry two columns of colour - a white label and a
            // green or grey value - and a third wash behind them made the
            // values harder to read than they were without any cursor at all.
            DrawRectangle(138, top + optSel * row - 2, 5, 28,
                (Color) { 255, 200, 70, 255 });

            for (int i = 0; i < rows; i++) {
                int y = top + i * row;
                switch (optRow[i]) {
                case OPT_VOLUME:
                    UiText("VOLUME", 160, y, 24, RAYWHITE);
                    DrawRectangle(490, y + 4, 220, 18, (Color) { 38, 38, 48, 255 });
                    for (int k = 0; k < VOL_STEPS; k++) {
                        int x = 490 + (int)(220.0f * k / VOL_STEPS);
                        int w = (int)(220.0f / VOL_STEPS) - 2;
                        Color c = (k < volStep) ? (Color) { 150, 200, 255, 255 }
                        : (Color) { 55, 55, 68, 255 };
                        DrawRectangle(x + 1, y + 6, w, 14, c);
                    }
                    DrawRectangleLines(490, y + 4, 220, 18, (Color) { 95, 95, 115, 255 });
                    UiText(TextFormat("%d%%", volStep * 10), 732, y + 2, 22,
                        (Color) {
                        150, 150, 175, 255
                    });
                    break;

                case OPT_BORDERLESS: {
                    int bl = IsWindowState(FLAG_WINDOW_UNDECORATED);
                    UiText("BORDERLESS", 160, y, 24, RAYWHITE);
                    UiText(bl ? "ON" : "OFF", 490, y, 24, bl ? on : off);
                    break;
                }

                case OPT_MOTION:
                    UiText("MOTION COMFORT", 160, y, 24, RAYWHITE);
                    UiText(reducedMotion ? "ON" : "OFF", 490, y, 24,
                        reducedMotion ? on : off);
                    UiText("no screen shake, no zoom pulse, gentler tunnel",
                        160, y + 28, 15, hint);
                    break;

                case OPT_LANG:
                    UiText("LANGUAGE", 160, y, 24, RAYWHITE);
                    // Each language names itself. "KOREAN" written in English is
                    // no use to someone who cannot read the English UI in the
                    // first place.
                    UiText((lang == LANG_KO) ? "한국어" : "ENGLISH", 490, y, 24, on);
                    break;

                case OPT_MONITOR:
                    UiText("MONITOR", 160, y, 24, RAYWHITE);
                    // Numbered from 1: the player is counting displays, not
                    // indexing an array.
                    UiText(TextFormat("%d / %d", monitorSel + 1, monitors),
                        490, y, 24, on);
                    break;

                case OPT_PADFACE:
                    UiText("PAD BUTTONS", 160, y, 24, RAYWHITE);
                    UiText(padFace == PADFACE_XBOX ? "XBOX"
                         : padFace == PADFACE_NINTENDO ? "NINTENDO"
                         : "PLAYSTATION", 490, y, 24, on);
                    // The four glyphs, in ring order, in the ring's own
                    // colours. Nintendo and Xbox use the same four letters in
                    // different places, so a name alone would not say which
                    // one is under the thumb - this does, and it says it in
                    // the colours the player already reads in the tunnel.
                    for (int k = 0; k < 4; k++) {
                        const char* g = faceGlyph[padFace][k];
                        int gw = UiMeasure(g, 24);
                        UiText(g, 675 + k * 34 + (24 - gw) / 2, y, 24, LaneColor(k));
                    }
                    break;

                case OPT_PADSWAP:
                    UiText("SHOULDER SWAP", 160, y, 24, RAYWHITE);
                    UiText("#SB burst   #SR reroll", 490, y, 22, on);
                    break;
                }
            }
            // One instruction for the whole screen instead of five identical ones
            // down the left margin. Every row is worked the same way now, so the
            // sentence only needs saying once.
            DrawCentered("UP / DOWN to choose      LEFT / RIGHT to change", 478, 16,
                (Color) { 130, 130, 155, 255 });
            DrawCentered(PROMPT("ESC to go back", "(#FR) to go back"), 506, 17, (Color) { 110, 110, 135, 255 });
        }
        else if (state == SHOP) {
            DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 225 });
            DrawCentered("SHOP", 28, 34, (Color) { 200, 190, 255, 255 });
            DrawCentered(TextFormat(T("you have %d"), wallet), 70, 22,
                (Color) {
                255, 210, 90, 255
            });
            // Stated up front. Everything here is a one-run consumable, and a
            // player who assumes otherwise will feel robbed the second time
            // they open this screen.
            DrawCentered("one card, one health item and one burst item, next run only",
                98, 16, (Color) {
                130, 130, 155, 255
            });

            Color gold = { 255, 200, 70, 255 };
            Color dead = { 95, 95, 115, 255 };
            Color body = { 150, 150, 175, 255 };

            if (shopPage) { DrawShopGacha(gold, dead, body); }
            else {

            // A bought card locks the row. The next run gets one head start,
            // not a whole hand bought in a single sitting.
            for (int i = 0; i < SHOP_CARDS; i++) {
                if (shopOffer[i] < 0) continue;
                int x = 40 + i * 300;
                const Card* cd = &cards[shopOffer[i]];
                int cost = cd->price;
                int can = (pendCard < 0) && wallet >= cost;

                // The whole box goes grey when it cannot be bought, not just
                // the text on it. A violet card that happens to have grey
                // lettering still reads as available at a glance; a grey card
                // reads as unavailable before you have read a word of it.
                DrawRectangle(x, 126, 280, 196,
                    can ? (Color) { 22, 22, 34, 255 } : (Color) { 25, 25, 27, 255 });
                DrawRectangleLines(x, 126, 280, 196,
                    can ? (Color) { 110, 100, 170, 255 } : (Color) { 74, 74, 78, 255 });

                // The focused card gets a thick gold frame rather than a caret.
                // A caret beside a 280px box reads as pointing at the gap next
                // to it; the border is the box saying it is the one.
                if (!shopPage && shopSel == i) {
                    Color sel = { 255, 210, 90, 255 };
                    for (int k = 0; k < 3; k++)
                        DrawRectangleLines(x - 2 + k, 124 + k, 284 - 2 * k, 200 - 2 * k, sel);
                }
                // The key hangs off the left inner edge, the price off the right
                // one, so the two read as a pair bracketing the card. A fixed
                // offset put the price wherever the number happened to end.
                {
                    const char* pt = TextFormat(T("%d coin"), cost);
                    UiText(pt, x + 264 - UiMeasure(pt, 16), 142, 16,
                        can ? gold : dead);
                }

                int fs = 24;
                int w = UiMeasure(cd->name, fs);
                while (fs > 13 && w > 250) { fs -= 2; w = UiMeasure(cd->name, fs); }
                UiText(cd->name, x + (280 - w) / 2, 178, fs,
                    can ? RAYWHITE : (Color) { 130, 130, 150, 255 });
                DrawCardDesc(cd, x + 16, 224, 16, 20, !can, 248);
            }

            // Directly under the cards, because it belongs to them: rerolling
            // is part of choosing a card, not a fourth thing to buy. Rerolling
            // without playing gets more expensive each time.
            // Left-aligned to the same edge as the leftmost card's inner text.
            // A key name and its label cannot be centred as a pair without one
            // of the two columns wandering, so all three rows hang off the left
            // instead. The price column sits well clear of the labels because
            // the Korean ones are half again as wide as the English.
            int rp = RerollPrice();
            int canRr = (pendCard < 0) && wallet >= rp;
            // Centred under the cards and keyed to R, the same as the draft.
            // Rerolling is part of choosing a card, not a fourth thing to buy,
            // and a row in the list below said the opposite.
            {
                const char* lbl = TextFormat(T("new cards  (%d coin)"), rp);
                int lw = UiMeasure(lbl, 18);
                int kw = UiMeasure(PROMPT("R", "#SR"), 18);
                int rx = (960 - (kw + 14 + lw)) / 2;
                UiText(PROMPT("R", "#SR"), rx, 334, 18, canRr ? gold : dead);
                UiText(lbl, rx + kw + 14, 334, 18, canRr ? body : dead);
            }

            // Two consumables, one of each per run.
            ShopCaret(!shopPage && shopSel == SC_HP, 56, 378, 18);
            UiText("next run: max health +10%", 100, 378, 18, pendHp ? dead : body);
            UiText(pendHp ? "bought" : TextFormat(T("%d coin"), PRICE_HP),
                560, 378, 18, (pendHp || wallet < PRICE_HP) ? dead : gold);

            ShopCaret(!shopPage && shopSel == SC_BURST, 56, 406, 18);
            UiText("next run: start at 50% burst", 100, 406, 18, pendBurst ? dead : body);
            UiText(pendBurst ? "bought" : TextFormat(T("%d coin"), PRICE_BURST),
                560, 406, 18, (pendBurst || wallet < PRICE_BURST) ? dead : gold);

            // Centred like the reroll above it, so the column of left-aligned
            // buy rows is bracketed by two centred lines that belong to the
            // page rather than to the list.
            {
                const char* lbl = T("gacha pull");
                int lw = UiMeasure(lbl, 18);
                int px = (960 - lw) / 2;
                ShopCaret(!shopPage && shopSel == SC_TOGACHA, px - 26, 442, 18);
                UiText(lbl, px, 442, 18,
                    (!shopPage && shopSel == SC_TOGACHA)
                        ? (Color) { 200, 190, 255, 255 } : body);
            }

            }

            // Two hints, because the arrows do two different things here. On
            // the card page LEFT/RIGHT walks the three cards - another way of
            // choosing. On the gacha page there is nothing to walk, so they
            // change the skin instead. One string covering both would be wrong
            // on one of them.
            DrawCentered(shopPage
                ? PROMPT("UP / DOWN to choose      LEFT / RIGHT to pick a skin      SPACE to confirm",
                         "UP / DOWN to choose      LEFT / RIGHT to pick a skin      (#FD) to confirm")
                : PROMPT("UP / DOWN / LEFT / RIGHT to choose      SPACE to confirm",
                         "UP / DOWN / LEFT / RIGHT to choose      (#FD) to confirm"),
                478, 16, (Color) { 110, 110, 135, 255 });
            DrawCentered(PROMPT("ESC to go back", "(#FR) to go back"), 502, 16, (Color) { 110, 110, 135, 255 });
        }
        else if (state == CODEX) {
            DrawCodex();
        }
        else if (state == CREDITS) {
            // The font is here because the OFL asks for it, and the licence
            // text ships beside the exe. raylib is zlib, which does not require
            // this, but taking someone's library and saying nothing would be a
            // poor way to use it.
            DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 215 });
            DrawCentered("CREDITS", 44, 36, (Color) { 200, 190, 255, 255 });

            Color lab = { 235, 210, 130, 255 };
            Color body = { 180, 180, 205, 255 };
            Color dim = { 120, 120, 145, 255 };
            Color mail = { 125, 145, 210, 255 };

            DrawCentered("RHYTHM TUNNEL", 106, 40, RAYWHITE);
            DrawCentered("1.44MB GAME_DEV CONTEST", 150, 18, dim);

            // Same two-column shape as the attributions below it, so the
            // author reads as one entry in the list rather than a headline.
            UiText("DEVELOPER", 185, 208, 20, lab);
            UiText("정수용", 390, 208, 20, RAYWHITE);
            UiText("verebell2020@gmail.com", 390, 236, 16, mail);

            UiText("FONT", 185, 292, 20, lab);
            UiText("네오둥근모  (Neo Dunggeunmo)", 390, 292, 20, body);
            UiText("(c) 2017-2024 Eunbin Jeong (Dalgona.)", 390, 320, 16, dim);
            UiText("SIL Open Font License 1.1 - see LICENSE-FONT.txt",
                390, 344, 16, dim);

            UiText("LIBRARY", 185, 396, 20, lab);
            UiText("raylib 6.0", 390, 396, 20, body);
            UiText("(c) 2013-2026 Ramon Santamaria (@raysan5)", 390, 424, 16, dim);
            UiText("zlib / libpng license", 390, 448, 16, dim);

            DrawCentered(PROMPT("ESC to go back", "(#FR) to go back"), 506, 17, (Color) { 110, 110, 135, 255 });
        }
        else if (state == DRAFT) {
            DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 200 });
            DrawCentered("CHOOSE ONE", 70, 40, (Color) { 200, 190, 255, 255 });
            DrawCentered("play your choice on the ring", 118, 18,
                (Color) {
                110, 110, 135, 255
            });

            // Not static: PROMPT is a runtime choice, so this rebuilds each frame.
            const char* keyName[3] = PROMPT_KEYS3;
            for (int i = 0; i < offerCount; i++) {
                int x = 40 + i * 300;
                const Card* cd = &cards[offer[i]];
                DrawRectangle(x, 170, 280, 210, (Color) { 22, 22, 34, 255 });
                DrawRectangleLines(x, 170, 280, 210, (Color) { 110, 100, 170, 255 });
                UiText(keyName[i], x + 16, 188, 17, (Color) { 255, 200, 70, 255 });

                // Shrink to fit rather than trusting every future card name to
                // be short enough. Names used to run off the edge of the box.
                int fs = 26;
                int w = UiMeasure(cd->name, fs);
                while (fs > 13 && w > 256) { fs -= 2; w = UiMeasure(cd->name, fs); }
                UiText(cd->name, x + (280 - w) / 2, 232 + (26 - fs) / 2, fs, RAYWHITE);

                DrawCardDesc(cd, x + 16, 286, 16, 20, 0, 248);
            }

            // Both escape hatches, stated plainly. A player who does not know
            // refusal exists will take a card that hurts them.
            //
            // Bottom left, not centred. A key name and its description cannot
            // be centred as a pair without one of the two columns wandering,
            // so they are anchored to the left edge instead - the same edge the
            // leftmost card sits on.
            UiText("DOWN / S", 40, 434, 18, (Color) { 255, 200, 70, 255 });
            UiText("take no card", 160, 434, 18, (Color) { 150, 150, 175, 255 });

            // Say the price, not just that there is one. The meter is on the
            // other screen, so "burst 50%" has to carry the whole cost.
            int payable = rerollsLeft > 0 || burst >= REROLL_BURST;
            Color rc = payable ? (Color) { 255, 200, 70, 255 }
            : (Color) { 95, 95, 115, 255 };
            Color rd = payable ? (Color) { 150, 150, 175, 255 }
            : (Color) { 95, 95, 115, 255 };
            UiText(PROMPT("R", "#SR"), 40, 462, 18, rc);
            UiText(rerollsLeft > 0
                ? TextFormat(T("reroll  (%d left)"), rerollsLeft)
                : "reroll  (burst 50%)",
                160, 462, 18, rd);
        }
        else if (paused) {
            DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 175 });
            DrawCentered("PAUSED", 190, 52, RAYWHITE);
            DrawRunMenu(286, 40, 26);
            DrawCentered(PROMPT("UP / DOWN to choose      SPACE to confirm",
                          "UP / DOWN to choose      (#FD) to confirm"),
                462, 16, (Color) { 100, 100, 120, 255 });
            DrawCentered(PROMPT("ESC to resume", "(#FR) to resume"), 486, 16, (Color) { 100, 100, 120, 255 });
        }
        else if (state == DEAD || state == WON) {
            DrawRectangle(0, 0, 960, 540, (Color) { 0, 0, 0, 195 });
            if (state == WON) {
                DrawCentered("CLEAR", 56, 56, (Color) { 120, 240, 160, 255 });
                DrawCentered(TextFormat(T("all %d levels"), LevelCount()), 112, 20,
                    (Color) {
                    150, 150, 170, 255
                });
            }
            else {
                DrawCentered("GAME OVER", 56, 50, (Color) { 255, 90, 90, 255 });
                DrawCentered(TextFormat(T("reached level %d"), level + 1), 112, 20,
                    (Color) {
                    150, 150, 170, 255
                });
            }
            DrawCentered(TextFormat("SCORE  %d", score), 142, 30, RAYWHITE);

            // The record sits directly under this run's score, because that is
            // the comparison the player is making. It used to be four lines
            // down, past the judgement table and the reward.
            if (score >= HI_SCORE && score > 0) {
                DrawCentered("NEW BEST!", 178, 20, YELLOW);
            }
            else {
                DrawCentered(TextFormat(T("BEST  %d"), HI_SCORE), 178, 20, DARKGRAY);
            }

            // Judgement breakdown, two columns. Left is what the hands did well
            // and how long they kept it up; right is the three ways a note can
            // go wrong plus what that came to. Reading down a column is now a
            // sentence rather than an alphabet.
            int judged = cntPerfect + cntGood + cntBad + cntMiss;
            float acc = judged > 0
                ? (float)(cntPerfect + cntGood) * 100.0f / (float)judged
                : 0.0f;
            UiText(TextFormat("PERFECT   %d", cntPerfect), 286, 216, 22,
                (Color) {
                255, 220, 80, 255
            });
            UiText(TextFormat("BAD       %d", cntBad), 286, 246, 22,
                (Color) {
                255, 150, 60, 255
            });
            UiText(TextFormat("COMBO     %d", maxCombo), 286, 276, 22,
                (Color) {
                220, 220, 140, 255
            });
            UiText(TextFormat("GOOD      %d", cntGood), 540, 216, 22,
                (Color) {
                120, 210, 255, 255
            });
            UiText(TextFormat("MISS      %d", cntMiss), 540, 246, 22,
                (Color) {
                255, 100, 100, 255
            });
            UiText(TextFormat("ACCURACY  %.1f%%", acc), 540, 276, 22, RAYWHITE);

            // The build that produced this score. Together with the seed it is
            // the whole run in two lines of text.
            // A hard run can hold thirty three cards. Listing them here made
            // the result screen grow without limit, so it is one summary line
            // and the list lives on its own screen.
            if (takenTotal > 0) {
                DrawCentered(TextFormat(T("%d cards      score x%.2f"),
                    takenTotal, mods.scoreMul), 318, 18,
                    (Color) {
                    170, 155, 235, 255
                });
            }

            // What the run paid into the shop. Levels, not score - so it reads
            // as a reward for getting further, which is what it is.
            if (runReward > 0) {
                DrawCentered(TextFormat(T("reward  +%d"), runReward), 344, 20,
                    (Color) {
                    255, 210, 90, 255
                });
            }

            // The number that made this chart, big enough to read off a screenshot.
            DrawCentered(TextFormat(T("SEED  %u"), seed), 374, 22,
                (Color) {
                150, 200, 255, 255
            });

            DrawRunMenu(414, 34, 24);
            DrawCentered(PROMPT("UP / DOWN to choose      SPACE to confirm",
                          "UP / DOWN to choose      (#FD) to confirm"),
                492, 16, (Color) { 100, 100, 120, 255 });
        }

        if (showCards) DrawCardPanel();
        EndTextureMode();

        // Blit the stage to the window, letterboxed and centred. The source
        // height is negative because a render texture comes out upside down.
        BeginDrawing();
        ClearBackground(BLACK);
        {
            float sw = (float)GetScreenWidth();
            float sh = (float)GetScreenHeight();
            float k = sw / STAGE_W;
            if (sh / STAGE_H < k) k = sh / STAGE_H;
            Rectangle src = { 0.0f, 0.0f, (float)STAGE_W, -(float)STAGE_H };
            Rectangle dst = { (sw - STAGE_W * k) * 0.5f, (sh - STAGE_H * k) * 0.5f,
                              STAGE_W * k, STAGE_H * k };
            DrawTexturePro(stage.texture, src, dst, (Vector2) { 0.0f, 0.0f }, 0.0f, WHITE);
        }
        EndDrawing();
    }

    UnloadRenderTexture(stage);

    for (int i = 0; i < LANE_COUNT; i++) {
        UnloadSound(callSnd[i]);
        UnloadSound(hitSnd[i]);
    }
    for (int i = 0; i < 4; i++) {
        UnloadSound(bassSnd[i]);
        UnloadSound(arpSnd[i]);
        UnloadSound(padSnd[i]);
    }
    UnloadSound(hatSnd);
    UnloadSound(denySnd);
#if PERFECT_SOUND == 5
    for (int i = 0; i < LANE_COUNT; i++) UnloadSound(perfSnd[i]);
#elif PERFECT_SOUND == 7
    for (int i = 0; i < LANE_COUNT; i++) UnloadSound(hitSndDull[i]);
#elif PERFECT_SOUND == 8
    for (int i = 0; i < COMBO_STEPS; i++) UnloadSound(perfSnd[i]);
#else
    UnloadSound(perfSnd);
#endif
    UnloadSound(twistSnd);
    UnloadSound(levelSnd);
    UnloadSound(dmgSnd);
    UnloadSound(kickSnd);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
