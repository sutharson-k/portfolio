/* 音效：通过 Web Audio API 合成方波音效（走子/吃子/将军/易位/终局）。
   这是本项目中除开链接跳转外唯一必须借助浏览器 JS API 的地方——
   WebAssembly 本身无法直接访问 AudioContext，只能通过 Emscripten 的
   EM_JS 桥接调用，桥接代码本身不包含任何游戏逻辑。 */
#include "sound.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(void, js_sound_init, (), {
    if (!Module.__audioCtx) {
        Module.__audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    }
});

EM_JS(void, js_sound_play, (int type), {
    if (!Module.__audioCtx) return;
    var ctx = Module.__audioCtx;
    var now = ctx.currentTime;
    var osc = ctx.createOscillator();
    var gain = ctx.createGain();
    osc.connect(gain);
    gain.connect(ctx.destination);
    switch (type) {
        case 0:
            osc.frequency.setValueAtTime(440, now);
            gain.gain.setValueAtTime(0.08, now);
            gain.gain.exponentialRampToValueAtTime(0.001, now + 0.1);
            osc.start(now); osc.stop(now + 0.1);
            break;
        case 1:
            osc.frequency.setValueAtTime(300, now);
            osc.frequency.setValueAtTime(600, now + 0.05);
            gain.gain.setValueAtTime(0.12, now);
            gain.gain.exponentialRampToValueAtTime(0.001, now + 0.15);
            osc.start(now); osc.stop(now + 0.15);
            break;
        case 2:
            osc.frequency.setValueAtTime(800, now);
            osc.frequency.setValueAtTime(600, now + 0.08);
            gain.gain.setValueAtTime(0.12, now);
            gain.gain.exponentialRampToValueAtTime(0.001, now + 0.2);
            osc.start(now); osc.stop(now + 0.2);
            break;
        case 3:
            osc.frequency.setValueAtTime(500, now);
            osc.frequency.setValueAtTime(700, now + 0.06);
            gain.gain.setValueAtTime(0.08, now);
            gain.gain.exponentialRampToValueAtTime(0.001, now + 0.15);
            osc.start(now); osc.stop(now + 0.15);
            break;
        case 4:
            osc.frequency.setValueAtTime(400, now);
            osc.frequency.setValueAtTime(300, now + 0.2);
            osc.frequency.setValueAtTime(200, now + 0.4);
            gain.gain.setValueAtTime(0.1, now);
            gain.gain.exponentialRampToValueAtTime(0.001, now + 0.6);
            osc.start(now); osc.stop(now + 0.6);
            break;
    }
});

void sound_init(void) { js_sound_init(); }
void sound_play(SoundType type) { js_sound_play((int)type); }

#else

void sound_init(void) {}
void sound_play(SoundType type) { (void)type; }

#endif
